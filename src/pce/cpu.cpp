#include "cpu.h"

DEFINE_OBJECT_TYPE_INFO(CPUBase);
BEGIN_OBJECT_PROPERTY_MAP(CPUBase)
END_OBJECT_PROPERTY_MAP()

CPUBase::CPUBase(const String& identifier, float frequency, CPUBackendType backend_type,
                 const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_cycle_period(SimulationTime(double(1000000000) / double(frequency))),
    m_frequency(frequency), m_backend_type(backend_type)
{
}

void CPUBase::SetFrequency(float frequency)
{
  m_frequency = frequency;
  m_cycle_period = SimulationTime(double(1000000000) / double(frequency));
}

SimulationTime CPUBase::GetCyclePeriod() const
{
  return m_cycle_period;
}

void CPUBase::SetIRQState(bool state) {}

void CPUBase::SignalNMI() {}

DebuggerInterface* CPUBase::GetDebuggerInterface()
{
  return nullptr;
}

const char* CPUBase::GetCurrentBackendString() const
{
  switch (m_backend_type)
  {
    case CPUBackendType::Interpreter:
      return "Interpreter";

    case CPUBackendType::CachedInterpreter:
      return "Cached Interpreter";

    case CPUBackendType::Recompiler:
      return "Recompiler";

    default:
      return "Unknown";
  }
}
