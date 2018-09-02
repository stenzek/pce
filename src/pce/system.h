#pragma once

#include <atomic>
#include <cstring>
#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>
#include <utility>

#include "YBaseLib/Barrier.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Event.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/TaskQueue.h"
#include "YBaseLib/Timer.h"

#include "common/clock.h"
#include "common/timing.h"
#include "pce/component.h"
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

class System : public Object
{
  DECLARE_OBJECT_TYPE_INFO(System, Object);
  DECLARE_OBJECT_NO_FACTORY(System);
  DECLARE_OBJECT_PROPERTY_MAP(System);

public:
  static const uint32 SERIALIZATION_ID = Component::MakeSerializationID('S', 'Y', 'S');

  enum class State : uint32
  {
    Uninitialized,
    Stopped,
    Running,
    Paused
  };

  System(const ObjectTypeInfo* type_info = &s_type_info);
  virtual ~System();

  // Host outputs
  HostInterface* GetHostInterface() const { return m_host_interface; }
  void SetHostInterface(HostInterface* iface) { m_host_interface = iface; }

  const TimingManager* GetTimingManager() const { return &m_timing_manager; }
  TimingManager* GetTimingManager() { return &m_timing_manager; }

  CPUBase* GetCPU() const { return m_cpu; }
  Bus* GetBus() const { return m_bus; }
  State GetState() const { return m_state; }
  void SetState(State state);

  // Enable/disable the speed limiter.
  bool IsSpeedLimiterEnabled() const { return m_speed_limiter_enabled; }
  void SetSpeedLimiterEnabled(bool enabled);

  // Pointer ownership is transferred
  void AddComponent(Component* component);

  // Creates a new component, and adds it.
  template<typename T, typename... Args>
  T* CreateComponent(const String& identifier, Args...);

  // Returns the nth component of the specified type.
  template<typename T>
  T* GetComponentByType(u32 index = 0);

  // Returns the component with the specified name, or nullptr.
  template<typename T = Component>
  T* GetComponentByIdentifier(const char* name);

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

  // Helper for reading a file to a buffer.
  // TODO: Find a better place for this.. result is pair<ptr, size>.
  static std::pair<std::unique_ptr<byte[]>, uint32> ReadFileToBuffer(const char* filename, uint32 expected_size);

protected:
  // Initialize all components, no need to call reset when starting for the first time
  virtual bool Initialize();

  // Reset all components
  virtual void Reset();

  // Shutdown stuff
  void Cleanup();

  // State loading/saving
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

  // Host outputs
  HostInterface* m_host_interface = nullptr;
  CPUBase* m_cpu = nullptr;
  Bus* m_bus = nullptr;
  TimingManager m_timing_manager;
  PODArray<Component*> m_components;
  State m_state = State::Uninitialized;

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

template<typename T, typename... Args>
T* System::CreateComponent(const String& identifier, Args... args)
{
  T* component = new T(identifier, args...);
  AddComponent(component);
  return component;
}

template<typename T>
T* System::GetComponentByType(u32 index /*= 0*/)
{
  u32 counter = 0;
  for (Component* component : m_components)
  {
    if (!component->IsDerived<T>())
      continue;

    if ((counter++) == index)
      return component->Cast<T>();
  }

  return nullptr;
}

template<typename T>
T* System::GetComponentByIdentifier(const char* name)
{
  for (Component* component : m_components)
  {
    if (component->GetIdentifier().Compare(name))
      return component->SafeCast<T>();
  }

  return nullptr;
}
