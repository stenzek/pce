#include "host_interface.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Thread.h"
#include "common/display_renderer.h"
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
      m_last_system_state = System::State::Paused;
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

      OnSystemStateLoaded();
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
                                     BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE);
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
  if (m_system)
    m_system->InterruptRunLoop();
  m_external_events_lock.unlock();
  if (wait)
    WaitForSimulationThread();
}

CPU::BackendType HostInterface::GetCPUBackend() const
{
  return m_system ? m_system->GetCPU()->GetBackend() : CPU::BackendType::Interpreter;
}

float HostInterface::GetCPUFrequency() const
{
  return m_system ? m_system->GetCPU()->GetFrequency() : 0.0f;
}

bool HostInterface::SetCPUBackend(CPU::BackendType backend)
{
  Assert(m_system);
  if (!m_system->GetCPU()->SupportsBackend(backend))
    return false;

  QueueExternalEvent(
    [this, backend]() {
      if (!m_system->GetCPU()->SupportsBackend(backend))
      {
        ReportError("Backend is not supported by CPU Core");
        return;
      }
      m_system->GetCPU()->SetBackend(backend);
    },
    true);
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
    m_throttle_timer.Reset();
    m_last_throttle_time = 0;
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

std::unique_ptr<Display> HostInterface::CreateDisplay(const char* name, Display::Type type,
                                                      u8 priority /*= Display::DEFAULT_PRIORITY*/)
{
  return GetDisplayRenderer()->CreateDisplay(name, type, priority);
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

void HostInterface::SendCtrlAltDel()
{
  // This has no delay, but the scancodes will still get enqueued.
  InjectKeyEvent(GenScanCode_LeftControl, true);
  InjectKeyEvent(GenScanCode_LeftAlt, true);
  InjectKeyEvent(GenScanCode_Delete, true);
  InjectKeyEvent(GenScanCode_LeftControl, false);
  InjectKeyEvent(GenScanCode_LeftAlt, false);
  InjectKeyEvent(GenScanCode_Delete, false);
  ReportMessage("Sent CTRL+ALT+DEL to machine.");
}

void HostInterface::OnSystemInitialized()
{
  // Create throttle event.
  m_throttle_timer.Reset();
  m_speed_elapsed_real_time.Reset();
  m_throttle_event = m_system->CreateNanosecondEvent("Simulation Throttle", GetSimulationSliceTime(),
                                                     std::bind(&HostInterface::ThrottleEvent, this), true);

  ReportFormattedMessage("System initialized: %s", m_system->GetTypeInfo()->GetTypeName());
}

void HostInterface::OnSystemReset()
{
  m_throttle_timer.Reset();
  m_last_throttle_time = 0;
  m_speed_elapsed_real_time.Reset();
  m_speed_elapsed_simulation_time = m_system->GetSimulationTime();
  m_system->GetCPU()->GetExecutionStats(&m_last_cpu_execution_stats);
  ReportFormattedMessage("System reset.");
  Log_InfoPrintf("System reset.");
}

void HostInterface::OnSystemStateLoaded()
{
  ReportFormattedMessage("State loaded.");
  m_throttle_timer.Reset();
  m_last_throttle_time = 0;
  m_speed_elapsed_real_time.Reset();
  m_speed_elapsed_simulation_time = m_system->GetSimulationTime();
  m_speed_elapsed_kernel_time = 0;
  m_speed_elapsed_user_time = 0;
  m_system->GetCPU()->GetExecutionStats(&m_last_cpu_execution_stats);
}

void HostInterface::OnSimulationStatsUpdate(const SimulationStats& stats) {}

void HostInterface::OnSystemDestroy()
{
  // Clear all callbacks, as they will no longer be valid.
  m_throttle_event.reset();
  m_keyboard_callbacks.clear();
  m_mouse_position_change_callbacks.clear();
  m_mouse_button_change_callbacks.clear();
  m_component_ui_elements.clear();
}

void HostInterface::OnSimulationResumed()
{
  // Reset the last wall time, otherwise all the missed time will be executed.
  m_throttle_timer.Reset();
  m_last_throttle_time = 0;
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

void HostInterface::AddOSDMessage(const char* message, float duration /* = 2.0f */)
{
  OSDMessage msg;
  msg.text = message;
  msg.duration = duration;

  std::unique_lock<std::mutex> lock(m_osd_messages_lock);
  m_osd_messages.push_back(std::move(msg));
}

void HostInterface::ExecuteKeyboardCallbacks(GenScanCode scancode, bool key_down)
{
  Log_DevPrintf("Key scancode %u %s", u32(scancode), key_down ? "down" : "up");
  QueueExternalEvent(
    [this, scancode, key_down]() {
      for (const auto& it : m_keyboard_callbacks)
        it.second(scancode, key_down);
    },
    false);
}

void HostInterface::ExecuteMousePositionChangeCallbacks(s32 dx, s32 dy)
{
  Log_DevPrintf("Mouse position change: %d %d", dx, dy);
  QueueExternalEvent(
    [this, dx, dy]() {
      for (const auto& it : m_mouse_position_change_callbacks)
        it.second(dx, dy);
    },
    false);
}

void HostInterface::ExecuteMouseButtonChangeCallbacks(u32 button, bool state)
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
    ExecuteExternalEvents();
    if (!m_system)
      continue;

    // Main execution loop.
    HandleStateChange();
    while (m_system && m_system->GetState() == System::State::Running)
    {
      m_system->Run();
      ExecuteExternalEvents();
      HandleStateChange();
    }
  }

  // Thread exiting is a barrier.
  WaitForCallingThread();
}

SimulationTime HostInterface::GetSimulationSliceTime() const
{
  // Target 60hz for event checks, to reduce input latency.
  // TODO: We should align this with the system scheduler quantum..
  return MillisecondsToSimulationTime(16);
}

SimulationTime HostInterface::GetMaxSimulationVarianceTime() const
{
  // If we're over 40ms behind, reset things.
  return INT64_C(40000000);
}

void HostInterface::HandleStateChange()
{
  const System::State new_state = m_system->GetState();
  if (m_last_system_state == new_state)
    return;

  if (new_state == System::State::Paused)
    OnSimulationPaused();
  else if (new_state == System::State::Running)
    OnSimulationResumed();
  else if (new_state == System::State::Stopped)
    ShutdownSystem();

  m_last_system_state = new_state;
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

void HostInterface::ThrottleEvent()
{
  // Basically, the throttler works by firing an event every SLICE_TIME (16) milliseconds, and sleeping for the
  // difference between the real time elapsed and SLICE_TIME. If the real time difference is greater than the variance
  // (40ms), this time is considered "lost" and will be skipped. This can happen in both directions, if the previous
  // sleep overshot, or if we are using too much host CPU.
  // TODO: We could throttle to display instead..
  static constexpr SimulationTime MINIMUM_SLEEP_TIME = MillisecondsToSimulationTime(1);

  // Statistics...
  UpdateExecutionSpeed();

  // If the speed limiter is off, we don't need to worry about any of this.
  if (!m_speed_limiter_enabled)
    return;

  // Use unsigned for defined overflow/wrap-around.
  const u64 time = static_cast<u64>(m_throttle_timer.GetTimeNanoseconds());
  SimulationTime sleep_time = static_cast<SimulationTime>(m_last_throttle_time - time);
  if (std::abs(sleep_time) >= GetMaxSimulationVarianceTime())
  {
#ifdef Y_BUILD_CONFIG_RELEASE
    // Don't display the slow messages in debug, it'll always be slow...
    // Limit how often the messages are displayed.
    if (m_speed_lost_time_timestamp.GetTimeSeconds() >= 1.0f)
    {
      Log_WarningPrintf("System too %s, lost %.2f ms", sleep_time < 0 ? "slow" : "fast",
                        static_cast<double>(std::abs(sleep_time) - GetMaxSimulationVarianceTime()) / 1000000.0);
      m_speed_lost_time_timestamp.Reset();
    }
#endif
    m_last_throttle_time = time - GetMaxSimulationVarianceTime();
  }
  else if (sleep_time >= MINIMUM_SLEEP_TIME)
  {
    Thread::Sleep(static_cast<u32>(sleep_time / 1000000));
  }

  m_last_throttle_time += GetSimulationSliceTime();
}

void HostInterface::UpdateExecutionSpeed()
{
  // Update emulation speed.
  const SimulationTime elapsed_sim_time = m_system->GetSimulationTimeSince(m_speed_elapsed_simulation_time);
  if (elapsed_sim_time < SecondsToSimulationTime(1))
    return;

  double speed_real_time = m_speed_elapsed_real_time.GetTimeNanoseconds();
  m_speed_elapsed_simulation_time = m_system->GetSimulationTime();
  m_speed_elapsed_real_time.Reset();

  SimulationStats stats;
  stats.simulation_speed = static_cast<float>(elapsed_sim_time) / static_cast<float>(speed_real_time);
  stats.total_time_simulated = m_speed_elapsed_simulation_time;
  stats.delta_time_simulated = elapsed_sim_time;
  m_system->GetCPU()->GetExecutionStats(&stats.cpu_stats);
  stats.cpu_delta_cycles_executed = stats.cpu_stats.cycles_executed - m_last_cpu_execution_stats.cycles_executed;
  stats.cpu_delta_instructions_interpreted =
    stats.cpu_stats.instructions_interpreted - m_last_cpu_execution_stats.instructions_interpreted;
  stats.cpu_delta_exceptions_raised = stats.cpu_stats.exceptions_raised - m_last_cpu_execution_stats.exceptions_raised;
  stats.cpu_delata_interrupts_serviced =
    stats.cpu_stats.interrupts_serviced - m_last_cpu_execution_stats.interrupts_serviced;
  stats.cpu_delta_code_cache_blocks_executed =
    stats.cpu_stats.code_cache_blocks_executed - m_last_cpu_execution_stats.code_cache_blocks_executed;
  stats.cpu_delta_code_cache_instructions_executed =
    stats.cpu_stats.code_cache_instructions_executed - m_last_cpu_execution_stats.code_cache_instructions_executed;

  u64 elapsed_kernel_time_ns = 0;
  u64 elapsed_user_time_ns = 0;

#ifdef Y_PLATFORM_WINDOWS
  {
    FILETIME creation_time, exit_time, kernel_time, user_time;
    u64 kernel_time_100ns, user_time_100ns;
    GetThreadTimes(GetCurrentThread(), &creation_time, &exit_time, &kernel_time, &user_time);
    kernel_time_100ns = ZeroExtend64(kernel_time.dwHighDateTime) << 32 | ZeroExtend64(kernel_time.dwLowDateTime);
    user_time_100ns = ZeroExtend64(user_time.dwHighDateTime) << 32 | ZeroExtend64(user_time.dwLowDateTime);
    elapsed_kernel_time_ns = (kernel_time_100ns - m_speed_elapsed_kernel_time) * 100;
    elapsed_user_time_ns = (user_time_100ns - m_speed_elapsed_user_time) * 100;
    m_speed_elapsed_kernel_time = kernel_time_100ns;
    m_speed_elapsed_user_time = user_time_100ns;
  }
#endif

  u64 total_cpu_time_ns = elapsed_kernel_time_ns + elapsed_user_time_ns;
  stats.host_cpu_usage = static_cast<float>(total_cpu_time_ns) / static_cast<float>(speed_real_time);

  OnSimulationStatsUpdate(stats);

  m_last_cpu_execution_stats = stats.cpu_stats;
}

void HostInterface::WaitForCallingThread()
{
  m_simulation_thread_barrier.Wait();
}

void HostInterface::ShutdownSystem()
{
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
