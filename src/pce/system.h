#pragma once
#include <atomic>
#include <memory>

#include "YBaseLib/Common.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/String.h"

#include "component.h"
#include "timing_event.h"
#include "types.h"

class Bus;
class ByteStream;
class BinaryReader;
class BinaryWriter;
class Component;
class CPU;
class Error;
class HostInterface;
class TimingEvent;

class System : public Object
{
  DECLARE_OBJECT_TYPE_INFO(System, Object);
  DECLARE_OBJECT_NO_FACTORY(System);
  DECLARE_OBJECT_PROPERTY_MAP(System);

  friend HostInterface;
  friend TimingEvent;

public:
  enum class State : u32
  {
    Initializing,
    Paused,
    Running,
    Stopped
  };

  System(const ObjectTypeInfo* type_info = &s_type_info);
  virtual ~System();

  // Parse a config file, and return the resulting system, if successful.
  static std::unique_ptr<System> ParseConfig(const char* filename, Error* error);

  // Host outputs
  HostInterface* GetHostInterface() const { return m_host_interface; }
  void SetHostInterface(HostInterface* iface) { m_host_interface = iface; }

  CPU* GetCPU() const { return m_cpu; }
  Bus* GetBus() const { return m_bus; }

  // Returns true if the CPU run loop should continue.
  bool ShouldRunCPU() const { return m_state == System::State::Running && !m_interrupt_execution.load(); }

  // State changes. Use with care.
  State GetState() const { return m_state; }
  void SetState(State state);

  // Initialize all components, no need to call reset when starting for the first time
  virtual bool Initialize();

  // Reset all components
  virtual void Reset();

  // State loading/saving
  bool LoadState(BinaryReader& reader);
  bool SaveState(BinaryWriter& writer);

  // Main CPU run loop. Does not return until the system is interrupted or stopped.
  void Run();

  // Interrupts the run loop. Use for external events.
  void InterruptRunLoop();

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

  // Returns the base path for the system, based on the ini path.
  const String& GetConfigBasePath() const { return m_base_path; }

  // Hold the bus, stalling the main CPU for the specified amount of time.
  void StallBus(SimulationTime time);

  // Returns a filename based on the ini path for the system for system-specific storage.
  // e.g. CMOS/NVRAM data - GetMiscDataFilename(".nvr") -> "/path/mysystem.nvr".
  String GetMiscDataFilename(const char* suffix) const;

  // Returns the amount of time the system has been running for.
  SimulationTime GetSimulationTime() const { return m_simulation_time; }

  // Calculates the difference between the specified timestamp and the current emulated time.
  SimulationTime GetSimulationTimeSince(SimulationTime timestamp) const
  {
    return GetSimulationTimeDifference(timestamp, m_simulation_time);
  }

  // Returns the time since the last time events were run, or the "pending event time".
  SimulationTime GetPendingEventTime() const { return GetSimulationTimeSince(m_last_event_run_time); }

  // Adds time to the total simulation time, but does not execute events.
  void AddSimulationTime(SimulationTime time) { m_simulation_time += time; }

  // Runs any pending events. Call when CPU downcount is zero.
  void RunEvents();

  // Updates the downcount of the CPU (event scheduling).
  void UpdateCPUDowncount();

  // Create an event which runs at a clock frequency, occuring every N cycles on the rising edge.
  std::unique_ptr<TimingEvent> CreateClockedEvent(const char* name, float frequency, CycleCount interval,
                                                  TimingEventCallback callback, bool activate);

  // Create an event that executes x times a second.
  std::unique_ptr<TimingEvent> CreateFrequencyEvent(const char* name, float frequency, TimingEventCallback callback,
                                                    bool activate);

  // Create specific timed events.
  std::unique_ptr<TimingEvent> CreateMillisecondEvent(const char* name, SimulationTime ms, TimingEventCallback callback,
                                                      bool activate);
  std::unique_ptr<TimingEvent> CreateMicrosecondEvent(const char* name, SimulationTime us, TimingEventCallback callback,
                                                      bool activate);
  std::unique_ptr<TimingEvent> CreateNanosecondEvent(const char* name, SimulationTime ns, TimingEventCallback callback,
                                                     bool activate);

  // Helper for reading a file to a buffer.
  // TODO: Find a better place for this.. result is pair<ptr, size>.
  static std::pair<std::unique_ptr<byte[]>, u32> ReadFileToBuffer(const char* filename, u32 offset, u32 expected_size);

protected:
  // State loading/saving.
  virtual bool LoadSystemState(BinaryReader& reader);
  virtual bool SaveSystemState(BinaryWriter& writer);

  HostInterface* m_host_interface = nullptr;
  CPU* m_cpu = nullptr;
  Bus* m_bus = nullptr;

private:
  static constexpr u32 SYSTEM_SERIALIZATION_ID = Component::MakeSerializationID('S', 'Y', 'S');
  static constexpr u32 COMPONENTS_SERIALIZATION_ID = Component::MakeSerializationID('C', 'O', 'M', 'P');
  static constexpr u32 EVENTS_SERIALIZATION_ID = Component::MakeSerializationID('E', 'V', 'T', 'S');

  // The downcount used when there are no active events.
  static constexpr SimulationTime POLL_FREQUENCY = INT64_C(100000000);

  // Inner serialization of components.
  bool LoadComponentsState(BinaryReader& reader);
  bool SaveComponentsState(BinaryWriter& writer);

  // Active event management
  void AddActiveEvent(TimingEvent* event);
  void RemoveActiveEvent(TimingEvent* event);
  void SortEvents();

  // Event lookup, use with care.
  // If you modify an event, call SortEvents afterwards.
  TimingEvent* FindActiveEvent(const char* name);

  // Event enumeration, use with care.
  // Don't remove an event while enumerating the list, as it will invalidate the iterator.
  template<typename T>
  void EnumerateActiveEvents(T callback) const
  {
    for (const TimingEvent* ev : m_events)
      callback(ev);
  }

  // Inner serialization of events.
  bool LoadEventsState(BinaryReader& reader);
  bool SaveEventsState(BinaryWriter& writer);

  PODArray<Component*> m_components;
  State m_state = State::Initializing;
  std::atomic_bool m_interrupt_execution{false};
  String m_base_path;

  std::vector<TimingEvent*> m_events;
  SimulationTime m_simulation_time = 0;
  SimulationTime m_last_event_run_time = 0;
  bool m_running_events = false;
  bool m_events_need_sorting = false;
};

template<typename T, typename... Args>
T* System::CreateComponent(const String& identifier, Args... args)
{
  T* component = new T(identifier, args...);
  AddComponent(static_cast<typename T::BaseClass*>(component));
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
