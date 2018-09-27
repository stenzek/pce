#pragma once
#include "pce/component.h"

class DebuggerInterface;

class CPU : public Component
{
  DECLARE_OBJECT_TYPE_INFO(CPU, Component);
  DECLARE_OBJECT_NO_FACTORY(CPU);
  DECLARE_OBJECT_PROPERTY_MAP(CPU);

public:
  enum class BackendType
  {
    Interpreter,
    CachedInterpreter,
    Recompiler
  };

  CPU(const String& identifier, float frequency, BackendType backend_type,
      const ObjectTypeInfo* type_info = &s_type_info);

  virtual ~CPU() = default;

  virtual bool Initialize(System* system, Bus* bus) override;
  virtual void Reset() override;

  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

  virtual const char* GetModelString() const = 0;

  float GetFrequency() const { return m_frequency; }
  void SetFrequency(float frequency);

  SimulationTime GetCyclePeriod() const;

  // IRQs are level-triggered
  virtual void SetIRQState(bool state);

  // NMIs are edge-triggered
  virtual void SignalNMI();

  // Debugger interface
  virtual DebuggerInterface* GetDebuggerInterface();

  // Execution mode
  BackendType GetBackend() const { return m_backend_type; }
  virtual bool SupportsBackend(BackendType mode) = 0;
  virtual void SetBackend(BackendType mode) = 0;

  // Execute cycles
  virtual void ExecuteCycles(CycleCount cycles) = 0;

  // Immediatley stops execution of the CPU, i.e. zeros the downcount.
  virtual void StopExecution() = 0;

  // Code cache flushing - for recompiler backends
  virtual void FlushCodeCache();

  // Backend to string.
  static const char* BackendTypeToString(BackendType type);

protected:
  void UpdateCyclePeriod();

  SimulationTime m_cycle_period;
  float m_frequency;
  BackendType m_backend_type;
};
