#include "pce/system.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/Thread.h"
#include "common/audio.h"
#include "common/display.h"
#include "pce/bus.h"
#include "pce/component.h"
#include "pce/cpu.h"
#include "pce/host_interface.h"
#include "pce/interrupt_controller.h"
#include "pce/mmio.h"
#include "pce/save_state_version.h"
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
Log_SetChannel(System);

DEFINE_OBJECT_TYPE_INFO(System);
BEGIN_OBJECT_PROPERTY_MAP(System)
END_OBJECT_PROPERTY_MAP()

System::System(const ObjectTypeInfo* type_info /* = &s_type_info */) : BaseClass(type_info) {}

System::~System()
{
  // We should be stopped first.
  Assert(m_state == State::Stopped || m_state == State::Uninitialized);

  // We want to be sure to clean up the events before the manager.
  m_throttle_event.reset();

  for (Component* component : m_components)
    delete component;

  delete m_bus;
}

void System::SetState(State state)
{
  // Are we running threaded?
  if (IsOnSimulationThread())
  {
    // Running on calling thread, so set everything immediately.
    if (m_state == state)
      return;

    m_state = state;
    m_pending_state.store(state);
    switch (state)
    {
      case State::Stopped:
        OnSimulationStopped();
        break;
      case State::Paused:
        OnSimulationPaused();
        break;
      case State::Running:
        OnSimulationResumed();
        break;
    }

    return;
  }

  // Running on background thread, so we need to set a pending state and wait.
  bool wake_thread = (m_state == State::Paused);
  m_pending_state.store(state);
  if (wake_thread)
    m_thread_wakeup_event.Signal();
  m_thread_action_barrier.Wait();

  // If we were stopped, and self-managing our threads, wait for the thread to exit.
  if (state == State::Stopped && m_thread.joinable())
    m_thread.join();
}

void System::SetSpeedLimiterEnabled(bool enabled)
{
  if (!IsOnSimulationThread())
  {
    QueueExternalEvent([this, enabled]() { SetSpeedLimiterEnabled(enabled); });
    return;
  }

  if (m_speed_limiter_enabled == enabled)
    return;

  m_speed_limiter_enabled = enabled;
  if (enabled)
  {
    // Clear the emulation timers, since the real time will be out of sync now.
    m_timing_manager.ResetTotalEmulatedTime();
    m_elapsed_real_time.Reset();
    m_last_emulated_time = 0;
    m_speed_elapsed_real_time.Reset();
    m_speed_elapsed_simulation_time = 0;
    m_host_interface->GetDisplay()->ResetFramesRendered();

    // If the speed limiter is being re-enabled, clear all audio buffers so we don't introduce lag.
    m_host_interface->GetAudioMixer()->ClearBuffers();
  }

  m_host_interface->ReportFormattedMessage("Speed limiter %s.", enabled ? "enabled" : "disabled");
}

void System::AddComponent(Component* component)
{
  m_components.Add(component);
}

bool System::Initialize()
{
  Assert(m_state == State::Uninitialized);

  // Set up task queue to buffer events.
  if (!m_external_event_queue.Initialize(TaskQueue::DefaultQueueSize, 0))
    Panic("Failed to initialize event queue.");

  // Set the simulation thread to the caller, so that SetState works as intended.
  m_simulation_thread_id = std::this_thread::get_id();

  // Set up host interface first, as this can fail.
  // TODO: Error handling.
  if (!m_host_interface->Initialize(this))
    Panic("Failed to initialize host interface.");

  m_bus->Initialize(this);
  m_cpu->Initialize(this, m_bus);
  for (Component* component : m_components)
  {
    if (!component->Initialize(this, m_bus))
    {
      Panic("Component failed to initialize.");
      return false;
    }
  }

  // Add audio mix event, render audio every 40ms (25hz).
  /*m_audio_render_event = m_timing_manager.CreateFrequencyEvent("Audio Mix/Render Event", float(Audio::MixFrequency),
                                                               std::bind(&System::AudioRenderEventCallback, this));*/

  // Add throttle event. Try to throttle every 100ms.
  m_throttle_event =
    m_timing_manager.CreateFrequencyEvent("Throttle Event", 60, std::bind(&System::ThrottleEventCallback, this));

  // We're now ready to go. Start in the paused state.
  m_state = State::Paused;
  m_pending_state.store(State::Paused);
  return true;
}

void System::Reset()
{
  m_host_interface->Reset();
  m_cpu->Reset();

  for (Component* component : m_components)
    component->Reset();
}

void System::Cleanup()
{
  // This should be called on the simulation thread.
  m_host_interface->Cleanup();

  // Ensure all events are drained.
  // Nothing should queue anything after this.
  for (;;)
  {
    if (!m_external_event_queue.ExecuteQueuedTasks())
      break;
  }

  // Swap state to stopped.
  SetState(State::Stopped);
}

void System::ClearSimulationThreadID()
{
  m_simulation_thread_id = std::thread::id();
}

void System::Run(bool return_on_pause)
{
  // This should be called by whatever thread initialized the system.
  DebugAssert(IsOnSimulationThread());

  // Execute slices of 1 second, the CPU will return if the system is paused anyway.
  // A lower time here will mean external events are processed quicker.
  CycleCount slice_cycles = CycleCount(m_cpu->GetFrequency() / 60);

  while (m_state != State::Stopped)
  {
    m_has_external_events.store(false);
    m_external_event_queue.ExecuteQueuedTasks();

    // Load the pending state into the new state.
    State pending_state = m_pending_state.load();
    if (pending_state != m_state)
    {
      // New state! Need to let the main thread know.
      m_state = pending_state;
      m_thread_action_barrier.Wait();

      // Were we paused/unpaused?
      switch (pending_state)
      {
        case State::Stopped:
          OnSimulationStopped();
          break;
        case State::Paused:
          OnSimulationPaused();
          break;
        case State::Running:
          OnSimulationResumed();
          break;
      }

      continue;
    }

    if (m_state == State::Paused)
    {
      // If we're not returning on pause, wait until we're unpaused (or stopped)
      if (return_on_pause)
        break;

      m_thread_wakeup_event.Wait();
      m_thread_wakeup_event.Reset();
      continue;
    }

    // CPU will call back to us to run components.
    m_cpu->ExecuteCycles(slice_cycles);
  }

  if (m_state == State::Stopped)
    Cleanup();
}

void System::Start(bool start_paused /* = false */)
{
  // We shouldn't be running when this is called.
  Assert(!m_thread.joinable() && m_state == State::Uninitialized);
  m_thread = std::thread([this, start_paused]() {
    if (!Initialize())
    {
      Panic("System initialization failed.");
      SetState(State::Uninitialized);
      return;
    }
    Reset();
    SetState(start_paused ? State::Paused : State::Running);
    Run(false);
  });
}

void System::Stop()
{
  if (IsOnSimulationThread())
  {
    SetState(State::Stopped);
    Cleanup();
    return;
  }

  SetState(State::Stopped);
}

void System::ExternalReset()
{
  // Always run after exiting the current simulation slice.
  QueueExternalEvent([this]() {
    m_host_interface->ReportMessage("System reset.");
    Reset();
  });
}

bool System::IsOnSimulationThread() const
{
  return (m_simulation_thread_id == std::this_thread::get_id());
}

void System::OnSimulationStopped()
{
  Log_InfoPrintf("Simulation stopped");

  // Pass to host.
  m_host_interface->ReportMessage("Simulation stopped.");
  m_host_interface->OnSimulationStopped();
}

void System::OnSimulationPaused()
{
  Log_InfoPrintf("Simulation paused.");

  // Pass to host.
  m_host_interface->ReportMessage("Simulation paused.");
  m_host_interface->OnSimulationPaused();
}

void System::OnSimulationResumed()
{
  Log_InfoPrintf("Simulation resumed.");

  // Ensure timers are zeroed before starting.
  m_timing_manager.ResetTotalEmulatedTime();
  m_elapsed_real_time.Reset();
  m_last_emulated_time = 0;
  m_speed_elapsed_real_time.Reset();
  m_speed_elapsed_simulation_time = 0;
  m_host_interface->GetDisplay()->ResetFramesRendered();

  // Pass to host.
  m_host_interface->ReportMessage("Simulation resumed.");
  m_host_interface->OnSimulationResumed();
}

void System::ThrottleEventCallback()
{
  // Update emulation speed.
  m_speed_elapsed_simulation_time += m_timing_manager.GetTotalEmulatedTime() - m_last_emulated_time;
  m_last_emulated_time = m_timing_manager.GetTotalEmulatedTime();
  double speed_real_time = m_speed_elapsed_real_time.GetTimeNanoseconds();
  if (speed_real_time >= 1000000000.0)
  {
    double fraction = double(m_speed_elapsed_simulation_time) / speed_real_time;
    m_speed_elapsed_simulation_time = 0;
    m_speed_elapsed_real_time.Reset();
    m_host_interface->OnSimulationSpeedUpdate(float(fraction * 100.0));

#ifdef Y_BUILD_CONFIG_RELEASE
    uint64 elapsed_kernel_time_ns = 0;
    uint64 elapsed_user_time_ns = 0;

#ifdef Y_PLATFORM_WINDOWS
    {
      FILETIME creation_time, exit_time, kernel_time, user_time;
      uint64 kernel_time_100ns, user_time_100ns;
      GetThreadTimes(GetCurrentThread(), &creation_time, &exit_time, &kernel_time, &user_time);
      kernel_time_100ns = ZeroExtend64(kernel_time.dwHighDateTime) << 32 | ZeroExtend64(kernel_time.dwLowDateTime);
      user_time_100ns = ZeroExtend64(user_time.dwHighDateTime) << 32 | ZeroExtend64(user_time.dwLowDateTime);
      elapsed_kernel_time_ns = (kernel_time_100ns - m_speed_elapsed_kernel_time) * 100;
      elapsed_user_time_ns = (user_time_100ns - m_speed_elapsed_user_time) * 100;
      m_speed_elapsed_kernel_time = kernel_time_100ns;
      m_speed_elapsed_user_time = user_time_100ns;
    }
#endif

    uint64 total_cpu_time_ns = elapsed_kernel_time_ns + elapsed_user_time_ns;
    double busy_cpu_time = double(total_cpu_time_ns) / speed_real_time;
    Log_ErrorPrintf("Main thread CPU usage: %f%%", busy_cpu_time * 100.0);
#endif
  }

  if (m_speed_limiter_enabled)
  {
    SimulationTime total_real_time = SimulationTime(m_elapsed_real_time.GetTimeNanoseconds());
    SimulationTime total_simulated_time = m_timing_manager.GetTotalEmulatedTime();
    SimulationTime diff = total_simulated_time - total_real_time;
    // Log_ErrorPrintf("Throttle: System too %s, diff %.4f", (diff > 0) ? "fast" : "slow", float(diff) / 1000000.0f);

    if (diff > 0)
    {
      int32 sleep_time = int32(diff / 1000000);
      if (sleep_time > 0)
        Thread::Sleep(sleep_time);
    }
    else
    {
      // If we fall below a certain point, reset the timers.
      const SimulationTime max_variance_ms = 50;
      if (diff < -(max_variance_ms * 1000000))
      {
#ifdef Y_BUILD_CONFIG_RELEASE
        Log_ErrorPrintf("System too slow, lost %d ms", int32(-diff / 1000000));
#endif
        m_elapsed_real_time.Reset();
        m_timing_manager.ResetTotalEmulatedTime();
        m_last_emulated_time = 0;
      }
    }
  }
}

template<class Callback>
static bool LoadComponentStateHelper(BinaryReader& reader, Callback callback)
{
  uint32 component_state_size;
  if (!reader.SafeReadUInt32(&component_state_size))
    return false;

  uint64 expected_offset = reader.GetStreamPosition() + component_state_size;
  if (!callback())
    return false;

  if (reader.GetStreamPosition() != expected_offset)
  {
    Log_ErrorPrintf("Incorrect offset after reading component");
    return false;
  }

  if (reader.GetErrorState())
    return false;

  return true;
}

template<class Callback>
static bool SaveComponentStateHelper(BinaryWriter& writer, Callback callback)
{
  uint64 size_offset = writer.GetStreamPosition();

  // Reserve space for component size
  if (!writer.SafeWriteUInt32(0))
    return false;

  uint64 start_offset = writer.GetStreamPosition();
  if (!callback())
    return false;

  uint64 end_offset = writer.GetStreamPosition();
  uint32 component_size = Truncate32(end_offset - start_offset);
  if (!writer.SafeSeekAbsolute(size_offset) || !writer.SafeWriteUInt32(component_size) ||
      !writer.SafeSeekAbsolute(end_offset))
  {
    return false;
  }

  return true;
}

void System::LoadState(ByteStream* stream)
{
  stream->AddRef();

  // We always load as an event, that way we don't load the state mid-execution.
  QueueExternalEvent([this, stream]() {
    BinaryReader reader(stream);
    if (LoadState(reader))
    {
      m_host_interface->ReportMessage("Loaded state.");
    }
    else
    {
      // Stream load failed, reset system, as it is now in an unknown state.
      m_host_interface->ReportMessage("Load state failed, resetting system.");
      Reset();
    }

    stream->Release();
  });
}

bool System::LoadState(BinaryReader& reader)
{
  uint32 signature, version;
  if (!reader.SafeReadUInt32(&signature) || !reader.SafeReadUInt32(&version))
    return false;

  if (signature != SAVE_STATE_SIGNATURE)
  {
    Log_ErrorPrintf("Incorrect save state signature");
    return false;
  }
  if (version != SAVE_STATE_VERSION)
  {
    Log_ErrorPrintf("Incorrect save state version");
    return false;
  }

  // Load system (this class) state
  if (!LoadComponentStateHelper(reader, [&]() { return LoadSystemState(reader); }))
    return false;

  // Load CPU state next
  if (!LoadComponentStateHelper(reader, [&]() { return m_cpu->LoadState(reader); }))
    return false;

  // Load bus state next
  if (!LoadComponentStateHelper(reader, [&]() { return m_bus->LoadState(reader); }))
    return false;

  // And finally the components
  if (!LoadComponentsState(reader))
    return false;

  // Make sure we're not in an error state and failed a read somewhere
  if (reader.GetErrorState())
    return false;

  // Load clock events
  if (!Clock::LoadTimingState(&m_timing_manager, reader))
    return false;

  return !reader.GetErrorState();
}

void System::SaveState(ByteStream* stream)
{
  stream->AddRef();

  auto save_callback = [this, stream]() {
    BinaryWriter writer(stream);
    if (SaveState(writer))
    {
      m_host_interface->ReportMessage("State saved.");
      stream->Commit();
    }
    else
    {
      m_host_interface->ReportMessage("Failed to save state.");
      stream->Discard();
    }

    stream->Release();
  };

  // Saving state can be done mid-execution.
  if (!IsOnSimulationThread())
    QueueExternalEvent(save_callback);
  else
    save_callback();
}

bool System::SaveState(BinaryWriter& writer)
{
  if (!writer.SafeWriteUInt32(SAVE_STATE_SIGNATURE) || !writer.SafeWriteUInt32(SAVE_STATE_VERSION))
  {
    return false;
  }

  // Save system (this class) state
  if (!SaveComponentStateHelper(writer, [&]() { return SaveSystemState(writer); }))
    return false;

  // Save CPU state next
  if (!SaveComponentStateHelper(writer, [&]() { return m_cpu->SaveState(writer); }))
    return false;

  // Save bus state next
  if (!SaveComponentStateHelper(writer, [&]() { return m_bus->SaveState(writer); }))
    return false;

  // And finally the components
  if (!SaveComponentsState(writer))
    return false;

  // Make sure we're not in an error state and failed a write somewhere
  if (writer.InErrorState())
    return false;

  // Write clock events
  if (!Clock::SaveTimingState(&m_timing_manager, writer))
    return false;

  return !writer.InErrorState();
}

bool System::LoadSystemState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  return true;
}

bool System::SaveSystemState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  return true;
}

bool System::LoadComponentsState(BinaryReader& reader)
{
  uint32 num_components = 0;
  if (!reader.SafeReadUInt32(&num_components) || num_components != m_components.GetSize())
  {
    Log_ErrorPrintf("Incorrect number of components");
    return false;
  }

  for (uint32 i = 0; i < num_components; i++)
  {
    Component* component = m_components[i];
    auto callback = [component, &reader]() { return component->LoadState(reader); };
    if (!LoadComponentStateHelper(reader, callback))
      return false;
  }

  return true;
}

bool System::SaveComponentsState(BinaryWriter& writer)
{
  writer.WriteUInt32(Truncate32(m_components.GetSize()));

  for (uint32 i = 0; i < m_components.GetSize(); i++)
  {
    Component* component = m_components[i];
    auto callback = [component, &writer]() { return component->SaveState(writer); };
    if (!SaveComponentStateHelper(writer, callback))
      return false;
  }

  return true;
}

bool System::SetCPUBackend(CPUBackendType backend)
{
  if (!m_cpu->SupportsBackend(backend))
    return false;

  // Since we can't call this during execution, we use an event in all cases.
  QueueExternalEvent([this, backend]() { m_cpu->SetBackend(backend); });
  return true;
}

std::pair<std::unique_ptr<byte[]>, uint32> System::ReadFileToBuffer(const char* filename, uint32 expected_size)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &stream))
  {
    Log_ErrorPrintf("Failed to open ROM file: %s", filename);
    return std::make_pair(std::unique_ptr<byte[]>(), 0);
  }

  const uint32 size = Truncate32(stream->GetSize());
  if (expected_size != 0 && size != expected_size)
  {
    Log_ErrorPrintf("ROM file %s mismatch - expected %u bytes, got %u bytes", filename, expected_size, size);
    stream->Release();
    return std::make_pair(std::unique_ptr<byte[]>(), 0);
  }

  std::unique_ptr<byte[]> data = std::make_unique<byte[]>(size);
  if (!stream->Read2(data.get(), size))
  {
    Log_ErrorPrintf("Failed to read %u bytes from ROM file %s", size, filename);
    stream->Release();
    return std::make_pair(std::unique_ptr<byte[]>(), 0);
  }

  stream->Release();
  return std::make_pair(std::move(data), size);
}
