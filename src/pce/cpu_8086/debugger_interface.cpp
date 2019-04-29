#include "pce/cpu_8086/debugger_interface.h"
#include "pce/bus.h"
#include "pce/cpu_8086/cpu.h"
#include "pce/cpu_8086/decoder.h"
#include "pce/system.h"

namespace CPU_8086 {

DebuggerInterface::DebuggerInterface(CPU* cpu, System* system) : m_cpu(cpu), m_system(system) {}

DebuggerInterface::~DebuggerInterface() {}

u32 DebuggerInterface::GetRegisterCount() const
{
  return Reg16_Count + Segment_Count;
}

DebuggerInterface::RegisterType DebuggerInterface::GetRegisterType(u32 index) const
{
  return DebuggerInterface::RegisterType::Word;
}

const char* DebuggerInterface::GetRegisterName(u32 index) const
{
  static const char* name_table[Reg16_Count + Segment_Count] = {"AX", "CX", "DX",    "BX", "SP", "BP", "SI",
                                                                "DI", "IP", "FLAGS", "ES", "CS", "SS", "DS"};

  DebugAssert(index < countof(name_table));
  return name_table[index];
}

DebuggerInterface::RegisterValue DebuggerInterface::GetRegisterValue(u32 index) const
{
  RegisterValue value = {};
  if (index < Reg16_Count)
    value.val_word = m_cpu->GetRegisters()->reg16[index];
  else
    value.val_word = m_cpu->GetRegisters()->segment_selectors[index - Reg16_Count];

  return value;
}

void DebuggerInterface::SetRegisterValue(u32 index, RegisterValue value) const
{
  Panic("Fix me");
}

bool DebuggerInterface::ReadMemoryByte(LinearMemoryAddress address, u8* value)
{
  return m_cpu->SafeReadMemoryByte(address, value);
}

bool DebuggerInterface::ReadMemoryWord(LinearMemoryAddress address, u16* value)
{
  return m_cpu->SafeReadMemoryWord(address, value);
}

bool DebuggerInterface::ReadMemoryDWord(LinearMemoryAddress address, u32* value)
{
  u16 low, high;
  bool res = m_cpu->SafeReadMemoryWord(address, &low) & m_cpu->SafeReadMemoryWord(address + 2, &high);
  *value = ZeroExtend32(low) | (ZeroExtend32(high) << 16);
  return res;
}

bool DebuggerInterface::WriteMemoryByte(LinearMemoryAddress address, u8 value)
{
  return m_cpu->SafeWriteMemoryByte(address, value);
}

bool DebuggerInterface::WriteMemoryWord(LinearMemoryAddress address, u16 value)
{
  return m_cpu->SafeWriteMemoryWord(address, value);
}

bool DebuggerInterface::WriteMemoryDWord(LinearMemoryAddress address, u32 value)
{
  return m_cpu->SafeWriteMemoryWord(address, Truncate16(value)) &
         m_cpu->SafeWriteMemoryWord(address + 2, Truncate16(value >> 16));
}

bool DebuggerInterface::ReadPhysicalMemoryByte(PhysicalMemoryAddress address, u8* value)
{
  return m_cpu->GetBus()->CheckedReadMemoryByte(address, value);
}

bool DebuggerInterface::ReadPhysicalMemoryWord(PhysicalMemoryAddress address, u16* value)
{
  return m_cpu->GetBus()->CheckedReadMemoryWord(address, value);
}

bool DebuggerInterface::ReadPhysicalMemoryDWord(PhysicalMemoryAddress address, u32* value)
{
  return m_cpu->GetBus()->CheckedReadMemoryDWord(address, value);
}

bool DebuggerInterface::WritePhysicalMemoryByte(PhysicalMemoryAddress address, u8 value)
{
  return m_cpu->GetBus()->CheckedWriteMemoryByte(address, value);
}

bool DebuggerInterface::WritePhysicalMemoryWord(PhysicalMemoryAddress address, u16 value)
{
  return m_cpu->GetBus()->CheckedWriteMemoryWord(address, value);
}

bool DebuggerInterface::WritePhysicalMemoryDWord(PhysicalMemoryAddress address, u32 value)
{
  return m_cpu->GetBus()->CheckedWriteMemoryDWord(address, value);
}

LinearMemoryAddress DebuggerInterface::GetInstructionPointer() const
{
  // This needs the linear address of the instruction pointer
  return ZeroExtend32(m_cpu->GetRegisters()->IP);
}

DebuggerInterface::RegisterType DebuggerInterface::GetStackValueType() const
{
  return RegisterType::Word;
}

LinearMemoryAddress DebuggerInterface::GetStackTop() const
{
  return ZeroExtend32(m_cpu->GetRegisters()->SP);
}

LinearMemoryAddress DebuggerInterface::GetStackBottom() const
{
  return UINT32_C(0xFFFF);
}

bool DebuggerInterface::DisassembleCode(LinearMemoryAddress instruction_pointer,
                                        LinearMemoryAddress* out_linear_address, String* out_formatted_address,
                                        String* out_code_bytes, String* out_disassembly, u32* out_size) const
{
  VirtualMemoryAddress fetch_IP = static_cast<VirtualMemoryAddress>(instruction_pointer);
  auto fetchb = [this, &fetch_IP](u8* val) {
    if (!m_cpu->SafeReadMemoryByte(m_cpu->CalculateLinearAddress(Segment_CS, fetch_IP), val))
      return false;
    fetch_IP += sizeof(u8);
    return true;
  };
  auto fetchw = [this, &fetch_IP](u16* val) {
    if (!m_cpu->SafeReadMemoryWord(m_cpu->CalculateLinearAddress(Segment_CS, fetch_IP), val))
      return false;
    fetch_IP += sizeof(u16);
    return true;
  };

  // Try to decode the instruction first.
  Instruction instruction;
  if (!Decoder::DecodeInstruction(&instruction, fetch_IP, std::move(fetchb), std::move(fetchw)))
    return false;

  if (out_linear_address)
    *out_linear_address =
      m_cpu->CalculateLinearAddress(Segment_CS, static_cast<VirtualMemoryAddress>(instruction_pointer));
  if (out_formatted_address)
    out_formatted_address->Format("%04X:%04X", ZeroExtend32(m_cpu->m_registers.CS), instruction_pointer);
  if (out_code_bytes)
  {
    out_code_bytes->Clear();
    for (u32 offset = 0; offset < instruction.length; offset++)
    {
      u8 val;
      if (m_cpu->SafeReadMemoryByte(
            m_cpu->CalculateLinearAddress(Segment_CS, static_cast<VirtualMemoryAddress>(instruction_pointer + offset)),
            &val))
      {
        out_code_bytes->AppendFormattedString("%s%02X", offset == 0 ? "" : " ", ZeroExtend32(val));
      }
      else
      {
        break;
      }
    }
  }
  if (out_disassembly)
    Decoder::DisassembleToString(&instruction, out_disassembly);
  if (out_size)
    *out_size = instruction.length;
  return true;
}

bool DebuggerInterface::IsStepping() const
{
  return m_stepping;
}

void DebuggerInterface::SetStepping(bool enabled, u32 instructions_to_execute)
{
  m_stepping = enabled;
  m_stepping_instructions = instructions_to_execute;
  if (!enabled || m_stepping_instructions > 0)
    m_cpu->GetSystem()->SetState(System::State::Running);
  else
    m_cpu->GetSystem()->SetState(System::State::Paused);
}

u32 DebuggerInterface::GetSteppingInstructionCount()
{
  u32 temp = m_stepping_instructions;
  m_stepping_instructions = 0;
  return temp;
}

} // namespace CPU_8086
