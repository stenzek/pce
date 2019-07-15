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

  struct ExecutionStats
  {
    u64 cycles_executed;
    u64 instructions_interpreted;
    u64 exceptions_raised;
    u64 interrupts_serviced;
    u64 num_code_cache_blocks;
    u64 code_cache_blocks_executed;
    u64 code_cache_instructions_executed;
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

  SimulationTime GetCyclePeriod() const { return m_cycle_period; }

  // Updates the downcount.
  void SetExecutionDowncount(SimulationTime time_downcount)
  {
    m_execution_downcount = (time_downcount + (m_cycle_period - 1)) / m_cycle_period;
  }

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

  // Main run loop
  virtual void Execute() = 0;

  // Code cache flushing - for recompiler backends
  virtual void FlushCodeCache() = 0;

  // Reads stats from CPU.
  virtual void GetExecutionStats(ExecutionStats* stats) const = 0;

  // Backend to string.
  static const char* BackendTypeToString(BackendType type);

protected:
  void UpdateCyclePeriod();

  // Pending cycles, used for some jit backends.
  // Pending time is added at the start of the block, then committed at the next block execution.
  CycleCount m_pending_cycles = 0;

  // Number of cycles until the next event.
  CycleCount m_execution_downcount = 0;

  // Time for each cycle.
  SimulationTime m_cycle_period;

  // Number of cycles per second.
  float m_frequency;
  BackendType m_backend_type;
};
