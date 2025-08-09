#include "host_interface.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/FileSystem.h"
#include "common/log.h"
#include "common/audio.h"
#include "system.h"
#include <cmath>
#include <cstdint>
#include <limits>
#include <retro2/retro2_session.h>
Log_SetChannel(HostInterface);

HostInterface::HostInterface()
{
  m_mixer = Audio::NullMixer::Create();
}

HostInterface::~HostInterface() {}

bool HostInterface::CreateSystem(const char* inifile, Error* error)
{
  Assert(!m_system);

  m_system = System::ParseConfig(inifile, error);
  if (!m_system)
    return false;

  Log_InfoPrintf("Initializing system '%s'...", m_system->GetTypeInfo()->GetTypeName());
  m_system->SetHostInterface(this);
  if (!m_system->Initialize())
  {
    error->SetErrorUserFormatted(0, "System initialization failed.");
    m_system.reset();
    return false;
  }

  // All good.
  m_system->Reset();
  Log_InfoPrintf("System initialized successfully.");
  return true;
}

bool HostInterface::LoadSystemState(const char* filename, Error* error)
{
  ByteStream* stream = FileSystem::OpenFile(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
  if (!stream)
  {
    error->SetErrorUserFormatted(0, "Failed to open file '%s'", filename);
    return false;
  }

  const bool result = m_system->LoadState(stream);
  stream->Release();
  if (result)
    GetAudioMixer()->ClearBuffers();

  return result;
}

bool HostInterface::SaveSystemState(const char* filename)
{
  ByteStream* stream =
    FileSystem::OpenFile(filename, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_WRITE |
                                     BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE);
  if (!stream)
  {
    ReportFormattedError("Failed to open file '%s'", filename);
    return false;
  }

  if (!m_system->SaveState(stream))
  {
    // Stream load failed, reset system, as it is now in an unknown state.
    stream->Discard();
    stream->Release();
    ReportFormattedError("Saving state failed.");
    return false;
  }

  stream->Commit();
  stream->Release();
  return true;
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
  {
    ReportError("Backend is not supported by CPU Core");
    return false;
  }

  m_system->GetCPU()->SetBackend(backend);
  return true;
}

void HostInterface::SetCPUFrequency(float frequency)
{
  Assert(m_system);
  m_system->GetCPU()->SetFrequency(frequency);
  ;
}

void HostInterface::FlushCPUCodeCache()
{
  Assert(m_system);
  m_system->GetCPU()->FlushCodeCache();
  ReportMessage("Flushed code cache.");
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

#if 0
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
  GetAudioMixer()->ClearBuffers();
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
  GetAudioMixer()->ClearBuffers();
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
#endif

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
  R2Session.AddOSDMessage(nullptr, message, duration);
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

#if 0
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
#endif
