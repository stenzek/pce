#include "cpu.h"
#include "common/state_wrapper.h"

DEFINE_OBJECT_TYPE_INFO(CPU);
BEGIN_OBJECT_PROPERTY_MAP(CPU)
PROPERTY_TABLE_MEMBER_FLOAT("Frequency", 0, offsetof(CPU, m_frequency), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

CPU::CPU(const String& identifier, float frequency, BackendType backend_type,
         const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_frequency(frequency), m_backend_type(backend_type)
{
  UpdateCyclePeriod();
}

bool CPU::Initialize(System* system, Bus* bus)
{
  UpdateCyclePeriod();
  return BaseClass::Initialize(system, bus);
}

void CPU::Reset()
{
  BaseClass::Reset();
  m_pending_cycles = 0;
  m_execution_downcount = 0;
}

bool CPU::DoState(StateWrapper& sw)
{
  if (!BaseClass::DoState(sw))
    return false;

  sw.Do(&m_pending_cycles);
  sw.Do(&m_execution_downcount);

  double frequency = m_frequency;
  sw.Do(&frequency);

  if (sw.IsReading())
  {
    if (frequency <= 0.0f)
      return false;

    m_frequency = frequency;
    UpdateCyclePeriod();
  }

  return true;
}

void CPU::SetFrequency(float frequency)
{
  m_frequency = frequency;
  UpdateCyclePeriod();
}

void CPU::SetExecutionDowncount(SimulationTime time_downcount)
{
  m_execution_downcount = static_cast<CycleCount>(static_cast<float>(time_downcount) * m_rcp_cycle_period);
  if ((static_cast<SimulationTime>(m_execution_downcount) * m_cycle_period) < time_downcount)
    m_execution_downcount++;
}

void CPU::SetIRQState(bool state) {}

void CPU::SignalNMI() {}

DebuggerInterface* CPU::GetDebuggerInterface()
{
  return nullptr;
}

const char* CPU::BackendTypeToString(BackendType type)
{
  switch (type)
  {
    case BackendType::Interpreter:
      return "Interpreter";

    case BackendType::CachedInterpreter:
      return "Cached Interpreter";

    case BackendType::Recompiler:
      return "Recompiler";

    default:
      return "Unknown";
  }
}

void CPU::UpdateCyclePeriod()
{
  m_cycle_period = static_cast<float>(double(1000000000) / double(m_frequency));
  m_rcp_cycle_period = static_cast<float>(1.0 / (double(1000000000) / double(m_frequency)));
}
