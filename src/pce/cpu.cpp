#include "cpu.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"

DEFINE_OBJECT_TYPE_INFO(CPUBase);
BEGIN_OBJECT_PROPERTY_MAP(CPUBase)
PROPERTY_TABLE_MEMBER_FLOAT("Frequency", 0, offsetof(CPUBase, m_frequency), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

CPUBase::CPUBase(const String& identifier, float frequency, CPUBackendType backend_type,
                 const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_cycle_period(SimulationTime(double(1000000000) / double(frequency))),
    m_frequency(frequency), m_backend_type(backend_type)
{
}

bool CPUBase::Initialize(System* system, Bus* bus)
{
  UpdateCyclePeriod();
  return BaseClass::Initialize(system, bus);
}

void CPUBase::Reset()
{
  BaseClass::Reset();
}

bool CPUBase::LoadState(BinaryReader& reader)
{
  if (!reader.SafeReadFloat(&m_frequency) || m_frequency <= 0.0f)
    return false;

  UpdateCyclePeriod();
  return true;
}

bool CPUBase::SaveState(BinaryWriter& writer)
{
  if (!writer.SafeWriteFloat(m_frequency))
    return false;

  return true;
}

void CPUBase::SetFrequency(float frequency)
{
  m_frequency = frequency;
  UpdateCyclePeriod();
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

void CPUBase::UpdateCyclePeriod()
{
  m_cycle_period = SimulationTime(double(1000000000) / double(m_frequency));
}
