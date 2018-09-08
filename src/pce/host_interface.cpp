#include "host_interface.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Thread.h"
#include "system.h"
#include <cmath>
#include <cstdint>
#include <limits>
Log_SetChannel(HostInterface);

HostInterface::HostInterface() : m_simulation_thread_semaphore(0, std::numeric_limits<int>::max()) {}

HostInterface::~HostInterface() {}

bool HostInterface::CreateSystem(const char* inifile, Error* error)
{
  Assert(!m_system);

  // Initialization happens on the simulation thread.
  bool initialization_result = false;
  QueueExternalEvent(
    [this, inifile, error, &initialization_result]() {
      m_system = System::ParseConfig(inifile, error);
      if (!m_system)
        return;

      Log_InfoPrintf("Initializing system '%s'...", m_system->GetTypeInfo()->GetTypeName());
      m_system->SetHostInterface(this);
      if (!m_system->Initialize())
      {
        error->SetErrorUserFormatted(0, "System initialization failed.");
        m_system.reset();
        return;
      }

      // All good.
      m_system->Reset();
      OnSystemInitialized();
      Log_InfoPrintf("System initialized successfully.");
      m_system->SetState(System::State::Paused);
      initialization_result = true;
    },
    true);

  return initialization_result;
}

void HostInterface::ResetSystem()
{
  // Always run after exiting the current simulation slice.
  QueueExternalEvent(
    [this]() {
      Log_InfoPrintf("Resetting system...");
      m_system->Reset();
      OnSystemReset();
      Log_InfoPrintf("System reset.");
      ReportMessage("System reset.");
    },
    false);
}

bool HostInterface::LoadSystemState(const char* filename, Error* error)
{
  ByteStream* stream = FileSystem::OpenFile(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
  if (!stream)
  {
    error->SetErrorUserFormatted(0, "Failed to open file '%s'", filename);
    return false;
  }

  bool result = false;
  QueueExternalEvent(
    [this, stream, error, &result]() {
      BinaryReader reader(stream);
      if (!m_system->LoadState(reader))
      {
        // Stream load failed, reset system, as it is now in an unknown state.
        error->SetErrorUserFormatted(0, "Loading state failed.");
        ReportMessage("Load state failed, resetting system.");
        m_system->Reset();
        OnSystemReset();
        return;
      }

      result = true;
    },
    true);

  stream->Release();
  return result;
}

void HostInterface::SaveSystemState(const char* filename)
{
  ByteStream* stream =
    FileSystem::OpenFile(filename, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_WRITE |
                                     BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_ATOMIC_UPDATE);
  if (!stream)
  {
    ReportFormattedError("Failed to open file '%s'", filename);
    return;
  }

  QueueExternalEvent(
    [this, stream]() {
      BinaryWriter writer(stream);
      if (!m_system->SaveState(writer))
      {
        // Stream load failed, reset system, as it is now in an unknown state.
        stream->Discard();
        stream->Release();
        ReportFormattedError("Saving state failed.");
        return;
      }

      stream->Commit();
      stream->Release();
      ReportMessage("State saved.");
    },
    false);
}

void HostInterface::QueueExternalEvent(ExternalEventCallback callback, bool wait)
{
  m_external_events_lock.lock();
  m_external_events.emplace(std::make_pair(std::move(callback), wait));
  m_simulation_thread_semaphore.Post();
  m_external_events_lock.unlock();
  if (wait)
    WaitForSimulationThread();
}

CPUBackendType HostInterface::GetCPUBackend() const
{
  return m_system ? m_system->GetCPU()->GetCurrentBackend() : CPUBackendType::Interpreter;
}

float HostInterface::GetCPUFrequency() const
{
  return m_system ? m_system->GetCPU()->GetFrequency() : 0.0f;
}

bool HostInterface::SetCPUBackend(CPUBackendType backend)
{
  Assert(m_system);
  if (!m_system->GetCPU()->SupportsBackend(backend))
    return false;

  QueueExternalEvent([this, backend]() { m_system->GetCPU()->SetBackend(backend); }, true);
  return true;
}

void HostInterface::SetCPUFrequency(float frequency)
{
  Assert(m_system);
  m_system->GetCPU()->SetFrequency(frequency);
}

void HostInterface::FlushCPUCodeCache()
{
  Assert(m_system);
  m_system->GetCPU()->FlushCodeCache();
}

void HostInterface::SetSpeedLimiterEnabled(bool enabled)
{
  if (m_speed_limiter_enabled == enabled)
    return;

  // Reset the wall time, so that we don't execute the "missed" time.
  if (enabled)
  {
    m_elapsed_real_time.Reset();
    m_pending_execution_time = 0;
  }

  // Not a big deal if this races.
  m_speed_limiter_enabled = enabled;
}

void HostInterface::PauseSimulation()
{
  Assert(m_system);
  Log_InfoPrintf("Pausing simulation...");
  QueueExternalEvent([this]() { m_system->SetState(System::State::Paused); }, true);
  Log_InfoPrintf("Simulation paused.");
}

void HostInterface::ResumeSimulation()
{
  Assert(m_system);
  Log_InfoPrintf("Resuming simulation.");
  QueueExternalEvent(
    [this]() {
      // Post the semaphore so the thread stays awake.
      m_system->SetState(System::State::Running);
      m_simulation_thread_semaphore.Post();
    },
    true);
}

void HostInterface::StopSimulation()
{
  Assert(m_system);
  Log_InfoPrintf("Stopping simulation...");
  QueueExternalEvent([this]() { m_system->SetState(System::State::Stopped); }, false);
  WaitForSimulationThread();
  Log_InfoPrintf("Simulation stopped.");
}

void HostInterface::RemoveAllCallbacks(const void* owner)
{
  auto callback = [owner](const auto& it) { return (it.first == owner); };
  std::remove_if(m_keyboard_callbacks.begin(), m_keyboard_callbacks.end(), callback);
  std::remove_if(m_mouse_position_change_callbacks.begin(), m_mouse_position_change_callbacks.end(), callback);
  std::remove_if(m_mouse_button_change_callbacks.begin(), m_mouse_button_change_callbacks.end(), callback);
}

void HostInterface::AddKeyboardCallback(const void* owner, KeyboardCallback callback)
{
  m_keyboard_callbacks.emplace_back(owner, std::move(callback));
}

void HostInterface::InjectKeyEvent(GenScanCode sc, bool down)
{
  QueueExternalEvent([this, sc, down]() { ExecuteKeyboardCallbacks(sc, down); }, false);
}

void HostInterface::AddMousePositionChangeCallback(const void* owner, MousePositionChangeCallback callback)
{
  m_mouse_position_change_callbacks.emplace_back(owner, std::move(callback));
}

void HostInterface::AddMouseButtonChangeCallback(const void* owner, MouseButtonChangeCallback callback)
{
  m_mouse_button_change_callbacks.emplace_back(owner, std::move(callback));
}

void HostInterface::ReportError(const char* message)
{
  Log_ErrorPrintf("Report error: %s", message);
}

void HostInterface::ReportFormattedError(const char* format, ...)
{
  std::va_list ap;
  va_start(ap, format);

  SmallString message;
  message.FormatVA(format, ap);
  va_end(ap);

  ReportError(message);
}

void HostInterface::ReportMessage(const char* message)
{
  Log_InfoPrintf("Report message: %s", message);
}

void HostInterface::ReportFormattedMessage(const char* format, ...)
{
  std::va_list ap;
  va_start(ap, format);

  SmallString message;
  message.FormatVA(format, ap);
  va_end(ap);

  ReportMessage(message);
}

bool HostInterface::IsOnSimulationThread() const
{
  return (m_simulation_thread_id == std::this_thread::get_id());
}

void HostInterface::OnSystemInitialized()
{
  m_elapsed_real_time.Reset();
  ReportFormattedMessage("System initialized: %s", m_system->GetTypeInfo()->GetTypeName());
}

void HostInterface::OnSystemReset()
{
  ReportFormattedMessage("System reset.");
}

void HostInterface::OnSimulationSpeedUpdate(float speed_percent) {}

void HostInterface::OnSystemDestroy()
{
  // Clear all callbacks, as they will no longer be valid.
  m_keyboard_callbacks.clear();
  m_mouse_position_change_callbacks.clear();
  m_mouse_button_change_callbacks.clear();
  m_component_ui_elements.clear();
}

void HostInterface::OnSimulationResumed()
{
  // Reset the last wall time, otherwise all the missed time will be executed.
  m_elapsed_real_time.Reset();
  m_pending_execution_time = 0;
  m_speed_elapsed_real_time.Reset();
}

void HostInterface::OnSimulationPaused() {}

void HostInterface::YieldToUI() {}

void HostInterface::AddUIIndicator(const Component* component, IndicatorType type)
{
  ComponentUIElement* ui = GetOrCreateComponentUIElement(component);
  ui->indicator_type = type;
}

void HostInterface::SetUIIndicatorState(const Component* component, IndicatorState state)
{
  ComponentUIElement* ui = GetComponentUIElement(component);
  ui->indicator_state = state;
}

void HostInterface::AddUICallback(const Component* component, const String& label, UICallback callback)
{
  ComponentUIElement* ui = GetOrCreateComponentUIElement(component);
  ui->callbacks.emplace_back(label, std::move(callback));
}

void HostInterface::AddUIFileCallback(const Component* component, const String& label, UIFileCallback callback)
{
  ComponentUIElement* ui = GetOrCreateComponentUIElement(component);
  ui->file_callbacks.emplace_back(label, std::move(callback));
}

void HostInterface::ExecuteKeyboardCallbacks(GenScanCode scancode, bool key_down)
{
  Log_DevPrintf("Key scancode %u %s", uint32(scancode), key_down ? "down" : "up");
  QueueExternalEvent(
    [this, scancode, key_down]() {
      for (const auto& it : m_keyboard_callbacks)
        it.second(scancode, key_down);
    },
    false);
}

void HostInterface::ExecuteMousePositionChangeCallbacks(int32 dx, int32 dy)
{
  Log_DevPrintf("Mouse position change: %d %d", dx, dy);
  QueueExternalEvent(
    [this, dx, dy]() {
      for (const auto& it : m_mouse_position_change_callbacks)
        it.second(dx, dy);
    },
    false);
}

void HostInterface::ExecuteMouseButtonChangeCallbacks(uint32 button, bool state)
{
  Log_DevPrintf("Mouse button change: %u %s", button, state ? "down" : "up");
  QueueExternalEvent(
    [this, button, state]() {
      for (const auto& it : m_mouse_button_change_callbacks)
        it.second(button, state);
    },
    false);
}

HostInterface::ComponentUIElement* HostInterface::CreateComponentUIElement(const Component* component)
{
  ComponentUIElement ui;
  ui.component = component;
  m_component_ui_elements.push_back(std::move(ui));
  return &m_component_ui_elements.back();
}

HostInterface::ComponentUIElement* HostInterface::GetOrCreateComponentUIElement(const Component* component)
{
  ComponentUIElement* ui = GetComponentUIElement(component);
  if (!ui)
    ui = CreateComponentUIElement(component);

  return ui;
}

HostInterface::ComponentUIElement* HostInterface::GetComponentUIElement(const Component* component)
{
  for (ComponentUIElement& it : m_component_ui_elements)
  {
    if (it.component == component)
      return &it;
  }

  return nullptr;
}

void HostInterface::SimulationThreadRoutine()
{
  // Set the simulation thread to the caller, so that SetState works as intended.
  m_simulation_thread_id = std::this_thread::get_id();
  while (m_simulation_thread_running.load())
  {
    // Wait until something wakes us up.
    m_simulation_thread_semaphore.Wait();

    if (ExecuteExternalEvents())
    {
      // These take precedence over normal execution.
      continue;
    }

    // If we're running, execute a slice.
    if (!m_system)
      continue;

    // Main execution loop.
    while (m_system->GetState() != System::State::Paused)
    {
      if (m_system->GetState() == System::State::Stopped)
      {
        ShutdownSystem();
        break;
      }

      // Normal execution.
      ExecuteSlice();
      ExecuteExternalEvents();
    }
  }

  // Thread exiting is a barrier.
  WaitForCallingThread();
}

SimulationTime HostInterface::GetSimulationSliceTime() const
{
  // Target 60hz for event checks, to reduce input latency.
  // TODO: We should align this with the system scheduler quantum..
  return INT64_C(16666667);
}

SimulationTime HostInterface::GetMaxSimulationVarianceTime() const
{
  // If we're over 40ms behind, reset things.
  return INT64_C(40000000);
}

bool HostInterface::ExecuteExternalEvents()
{
  bool did_any_work = false;

  m_external_events_lock.lock();
  while (!m_external_events.empty())
  {
    auto elem = std::move(m_external_events.front());
    m_external_events.pop();
    m_external_events_lock.unlock();

    elem.first();
    if (elem.second)
      WaitForCallingThread();

    did_any_work = true;
    m_external_events_lock.lock();
  }

  m_external_events_lock.unlock();
  return did_any_work;
}

void HostInterface::ExecuteSlice()
{
  static constexpr SimulationTime MINIMUM_SLEEP_TIME = INT64_C(1000000); // 1ms
  const SimulationTime slice_time = GetSimulationSliceTime();

  // If the speed limiter is off, we don't need to worry about any of this.
  if (!m_speed_limiter_enabled)
  {
    m_system->ExecuteSlice(slice_time);
    UpdateExecutionSpeed();
    return;
  }

  // Work out how much time we need to simulate.
  Timer execution_timer;
  m_pending_execution_time += static_cast<SimulationTime>(std::ceil(m_elapsed_real_time.GetTimeNanoseconds()));
  m_elapsed_real_time.Reset();

  // Simulate the system for the passed time.
  const SimulationTime time_to_simulate = m_pending_execution_time;
  const SimulationTime actual_time_simulated = m_system->ExecuteSlice(time_to_simulate);
  m_pending_execution_time -= actual_time_simulated;
  UpdateExecutionSpeed();

  // Our sleep time is therefore the difference between the wall and simulation time.
  const SimulationTime execution_wall_time =
    static_cast<SimulationTime>(std::ceil(m_elapsed_real_time.GetTimeNanoseconds()));
  const SimulationTime sleep_time = slice_time - execution_wall_time;
#if 0
  Log_WarningPrintf("Walltime: %f ms, exectime: %f (%f) ms, sleeptime: %f ms", execution_wall_time / 1000000.0,
                    time_to_simulate / 1000000.0, actual_time_simulated / 1000000.0, sleep_time / 1000000.0);
#endif

  if (sleep_time >= MINIMUM_SLEEP_TIME)
  {
    Thread::Sleep(static_cast<u32>(sleep_time / 1000000));
  }
  else if (-sleep_time >= GetMaxSimulationVarianceTime())
  {
#if 0
    Log_WarningPrintf("System too slow, lost %u ms", -sleep_time / 1000000);
#endif
    m_elapsed_real_time.Reset();

    // Try not to waste too much time. If we zero this, we sleep next loop, since no real time has passed.
    m_pending_execution_time = slice_time;
  }
}

void HostInterface::UpdateExecutionSpeed()
{
  // Update emulation speed.
  double speed_real_time = m_speed_elapsed_real_time.GetTimeNanoseconds();
  if (speed_real_time >= 1000000000.0)
  {
    const SimulationTime elapsed_sim_time =
      m_system->GetTimingManager()->GetEmulatedTimeDifference(m_speed_elapsed_simulation_time);
    m_speed_elapsed_simulation_time = m_system->GetTimingManager()->GetTotalEmulatedTime();
    m_speed_elapsed_real_time.Reset();

    const double fraction = double(elapsed_sim_time) / speed_real_time;
    OnSimulationSpeedUpdate(float(fraction * 100.0));

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
    printf("Main thread CPU usage: %f%%\n", busy_cpu_time * 100.0);
#endif
  }
}

void HostInterface::WaitForCallingThread()
{
  m_simulation_thread_barrier.Wait();
}

void HostInterface::ShutdownSystem()
{
  Log_InfoPrintf("Stopping simulation");
  m_system->SetState(System::State::Paused);
  OnSimulationPaused();
  m_system->SetState(System::State::Stopped);
  OnSystemDestroy();
  m_system.reset();
  WaitForCallingThread();
}

void HostInterface::WaitForSimulationThread()
{
  // TODO: We need to yield to the UI, but the barrier doesn't let us do that.
  m_simulation_thread_barrier.Wait();
}

void HostInterface::StopSimulationThread()
{
  m_simulation_thread_running.store(false);
  m_simulation_thread_semaphore.Post();
  WaitForSimulationThread();
}
