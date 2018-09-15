#pragma once

#include <memory>

#include "YBaseLib/Common.h"
#include "YBaseLib/PODArray.h"
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
class Error;
class HostInterface;

class System : public Object
{
  DECLARE_OBJECT_TYPE_INFO(System, Object);
  DECLARE_OBJECT_NO_FACTORY(System);
  DECLARE_OBJECT_PROPERTY_MAP(System);

  friend HostInterface;

public:
  enum class State : uint32
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

  const TimingManager* GetTimingManager() const { return &m_timing_manager; }
  TimingManager* GetTimingManager() { return &m_timing_manager; }

  CPUBase* GetCPU() const { return m_cpu; }
  Bus* GetBus() const { return m_bus; }

  // State changes. Use with care.
  State GetState() const { return m_state; }
  void SetState(State state);

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

  // Helper for reading a file to a buffer.
  // TODO: Find a better place for this.. result is pair<ptr, size>.
  static std::pair<std::unique_ptr<byte[]>, uint32> ReadFileToBuffer(const char* filename, uint32 expected_size);

  // Execute the system. Returns the time actually executed.
  SimulationTime ExecuteSlice(SimulationTime time);

  // Initialize all components, no need to call reset when starting for the first time
  virtual bool Initialize();

  // Reset all components
  virtual void Reset();

  // State loading/saving.
  virtual bool LoadSystemState(BinaryReader& reader);
  virtual bool SaveSystemState(BinaryWriter& writer);

  // State loading/saving
  bool LoadState(BinaryReader& reader);
  bool SaveState(BinaryWriter& writer);

private:
  bool LoadComponentsState(BinaryReader& reader);
  bool SaveComponentsState(BinaryWriter& writer);

protected:
  HostInterface* m_host_interface = nullptr;
  CPUBase* m_cpu = nullptr;
  Bus* m_bus = nullptr;
  TimingManager m_timing_manager;

private:
  static const uint32 SERIALIZATION_ID = Component::MakeSerializationID('S', 'Y', 'S');

  PODArray<Component*> m_components;
  State m_state = State::Initializing;
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
