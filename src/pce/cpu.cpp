#include "cpu.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"

DEFINE_OBJECT_TYPE_INFO(CPU);
BEGIN_OBJECT_PROPERTY_MAP(CPU)
PROPERTY_TABLE_MEMBER_FLOAT("Frequency", 0, offsetof(CPU, m_frequency), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

CPU::CPU(const String& identifier, float frequency, BackendType backend_type,
         const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_cycle_period(SimulationTime(double(1000000000) / double(frequency))),
    m_frequency(frequency), m_backend_type(backend_type)
{
}

bool CPU::Initialize(System* system, Bus* bus)
{
  UpdateCyclePeriod();
  return BaseClass::Initialize(system, bus);
}

void CPU::Reset()
{
  BaseClass::Reset();
}

bool CPU::LoadState(BinaryReader& reader)
{
  if (!reader.SafeReadFloat(&m_frequency) || m_frequency <= 0.0f)
    return false;

  UpdateCyclePeriod();
  return true;
}

bool CPU::SaveState(BinaryWriter& writer)
{
  if (!writer.SafeWriteFloat(m_frequency))
    return false;

  return true;
}

void CPU::SetFrequency(float frequency)
{
  m_frequency = frequency;
  UpdateCyclePeriod();
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
  m_cycle_period = SimulationTime(double(1000000000) / double(m_frequency));
}
