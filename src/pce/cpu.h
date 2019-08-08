#pragma once
#include "pce/component.h"
#include <cmath>

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

  /// Returns the cycle period (amount of time for each tick).
  float GetCyclePeriod() const { return m_cycle_period; }

  /// Converts simulation time to clock cycles, rounding up.
  CycleCount SimulationTimeToCycles(SimulationTime time)
  {
    return static_cast<CycleCount>(std::ceil(static_cast<float>(time) / m_cycle_period));
  }

  /// Converts clock cycles to simulation time, rounding down/truncating.
  SimulationTime CyclesToSimulationTime(CycleCount cycles)
  {
    return static_cast<SimulationTime>(static_cast<float>(cycles) * m_cycle_period);
  }

  /// Updates the downcount, or how long the CPU can execute for before running events.
  void SetExecutionDowncount(SimulationTime time_downcount)
  {
    m_execution_downcount = SimulationTimeToCycles(time_downcount);
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
  /// Updates the period (time for a clock tick). Call when frequency changes.
  void UpdateCyclePeriod();

  // Pending cycles, used for some jit backends.
  // Pending time is added at the start of the block, then committed at the next block execution.
  CycleCount m_pending_cycles = 0;

  // Number of cycles until the next event.
  CycleCount m_execution_downcount = 0;

  // Number of cycles per second.
  float m_frequency;

  // Time for each cycle.
  float m_cycle_period;

  // Currently-active backend type.
  BackendType m_backend_type;
};
