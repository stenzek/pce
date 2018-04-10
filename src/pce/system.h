#pragma once

#include <atomic>
#include <cstring>
#include <functional>
#include <thread>
#include <unordered_map>

#include "YBaseLib/Barrier.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Event.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/TaskQueue.h"
#include "YBaseLib/Timer.h"

#include "pce/clock.h"
#include "pce/component.h"
#include "pce/timing.h"
#include "pce/types.h"

class Bus;
class ByteStream;
class BinaryReader;
class BinaryWriter;
class Component;
class CPUBase;
class InterruptController;
class MMIO;
class HostInterface;

class System
{
public:
  static const uint32 SERIALIZATION_ID = Component::MakeSerializationID('S', 'Y', 'S');

  enum class State : uint32
  {
    Uninitialized,
    Stopped,
    Running,
    Paused
  };

  System(HostInterface* host_interface);
  virtual ~System();

  // Host outputs
  HostInterface* GetHostInterface() const { return m_host_interface; }

  const TimingManager* GetTimingManager() const { return &m_timing_manager; }
  TimingManager* GetTimingManager() { return &m_timing_manager; }

  CPUBase* GetCPU() const { return m_cpu; }
  Bus* GetBus() const { return m_bus; }
  State GetState() const { return m_state; }
  void SetState(State state);

  // Enable/disable the speed limiter.
  bool IsSpeedLimiterEnabled() const { return m_speed_limiter_enabled; }
  void SetSpeedLimiterEnabled(bool enabled);

  virtual const char* GetSystemName() const = 0;
  virtual InterruptController* GetInterruptController() const = 0;

  // Pointer ownership is transferred
  void AddComponent(Component* component);

  // Initialize all components, no need to call reset when starting for the first time
  virtual void Initialize();

  // Reset all components
  virtual void Reset();

  // Clears the simulation thread ID. Call this when moving simulation from one thread to another.
  void ClearSimulationThreadID();

  // Begins simulation of the system. Only returns when the state is not set to Running.
  // This will simulate the system on the calling thread. State should be set to Running already.
  void Run(bool return_on_pause = true);

  // Starts a background thread for simulating the system.
  // When the caller wants the thread to begin work, set the state to Running.
  void Start(bool start_paused = false);

  // Exits when running off-thread.
  // Ensure that it is not executing when calling, otherwise use SetState(Stopped).
  void Stop();

  // Resets the system, when called externally.
  void ExternalReset();

  // Load/save state. If load fails, system is in an undefined state, Reset it.
  // This occurs asynchronously, the event maintains a reference to the stream.
  // The stream is committed upon success, or discarded upon fail.
  void LoadState(ByteStream* stream);
  void SaveState(ByteStream* stream);

  // External events, will interrupt the CPU and execute.
  template<typename T>
  void QueueExternalEvent(const T& callback)
  {
    m_external_event_queue.QueueLambdaTask(callback);
    m_has_external_events.store(true);
    if (m_state == State::Paused)
      m_thread_wakeup_event.Signal();
  }

  // Use care when calling this variant, deadlocks can occur.
  template<typename T>
  void QueueExternalEventAndWait(const T& callback)
  {
    m_external_event_queue.QueueBlockingLambdaTask(callback);
    m_has_external_events.store(true);
    if (m_state == State::Paused)
      m_thread_wakeup_event.Signal();
  }

  // Has any external events waiting?
  bool HasExternalEvents() const { return m_has_external_events; }

  // Safely changes CPU backend.
  // This change is done asynchronously.
  bool SetCPUBackend(CPUBackendType backend);

protected:
  // State loading/saving
  void InternalReset();
  void InternalCleanup();
  bool LoadState(BinaryReader& reader);
  bool SaveState(BinaryWriter& writer);
  virtual bool LoadSystemState(BinaryReader& reader);
  virtual bool SaveSystemState(BinaryWriter& writer);
  bool LoadComponentsState(BinaryReader& reader);
  bool SaveComponentsState(BinaryWriter& writer);

  // Helper to check if the caller is on the simulation thread.
  bool IsOnSimulationThread() const;

  // Reset timer values on pause/resume.
  void OnSimulationStopped();
  void OnSimulationPaused();
  void OnSimulationResumed();

  // Throttle event callback.
  void ThrottleEventCallback();
  void AudioRenderEventCallback();

  // Host outputs
  HostInterface* m_host_interface;
  CPUBase* m_cpu = nullptr;
  Bus* m_bus = nullptr;
  TimingManager m_timing_manager;
  PODArray<Component*> m_components;
  State m_state = State::Uninitialized;

  // Audio mixing/render event
  TimingEvent::Pointer m_audio_render_event;
  SimulationTime m_last_audio_render_time = 0;

  // Throttle event
  Timer m_elapsed_real_time;
  SimulationTime m_last_emulated_time = 0;
  TimingEvent::Pointer m_throttle_event;
  bool m_speed_limiter_enabled = true;

  // Emulation speed tracking
  Timer m_speed_elapsed_real_time;
  SimulationTime m_speed_elapsed_simulation_time = 0;
  uint64 m_speed_elapsed_user_time = 0;
  uint64 m_speed_elapsed_kernel_time = 0;

  // Threaded running state
  std::atomic<State> m_pending_state{State::Uninitialized};
  std::thread m_thread;
  std::thread::id m_simulation_thread_id;
  Barrier m_thread_action_barrier{2};
  Event m_thread_wakeup_event{false};

  // External event queue: TODO ensure that there aren't any races on the flag?
  TaskQueue m_external_event_queue;
  std::atomic_bool m_has_external_events{false};
};
