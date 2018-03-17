#pragma once
#include "pce/component.h"

class DebuggerInterface;

class CPUBase : public Component
{
public:
  CPUBase(float frequency, CPUBackendType backend_type)
    : m_cycle_period(SimulationTime(double(1000000000) / double(frequency))), m_frequency(frequency),
      m_backend_type(backend_type)
  {
  }

  virtual ~CPUBase() = default;

  float GetFrequency() const { return m_frequency; }
  void SetFrequency(float frequency)
  {
    m_frequency = frequency;
    m_cycle_period = SimulationTime(double(1000000000) / double(frequency));
  }

  SimulationTime GetCyclePeriod() const { return m_cycle_period; }

  // IRQs are level-triggered
  virtual void SetIRQState(bool state) {}

  // NMIs are edge-triggered
  virtual void SignalNMI() {}

  // Debugger interface
  virtual DebuggerInterface* GetDebuggerInterface() { return nullptr; }

  // Execution mode
  CPUBackendType GetCurrentBackend() const { return m_backend_type; }
  virtual bool SupportsBackend(CPUBackendType mode) = 0;
  virtual void SetBackend(CPUBackendType mode) = 0;

  // Execute cycles
  virtual void ExecuteCycles(CycleCount cycles) = 0;

  // Memory lock action
  virtual void OnLockedMemoryAccess(PhysicalMemoryAddress address, PhysicalMemoryAddress range_start,
                                    PhysicalMemoryAddress range_end, MemoryLockAccess access) = 0;

  // Code cache flushing - for recompiler backends
  virtual void FlushCodeCache() = 0;

protected:
  SimulationTime m_cycle_period;
  float m_frequency;
  CPUBackendType m_backend_type;
};
