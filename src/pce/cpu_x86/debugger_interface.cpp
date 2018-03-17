#include "pce/cpu_x86/debugger_interface.h"
#include "pce/cpu_x86/cpu.h"
#include "pce/cpu_x86/decode.h"
#include "pce/system.h"

namespace CPU_X86 {

DebuggerInterface::DebuggerInterface(CPU* cpu, System* system) : m_cpu(cpu), m_system(system) {}

DebuggerInterface::~DebuggerInterface() {}

uint32 DebuggerInterface::GetRegisterCount() const
{
  return Reg32_Count + Segment_Count + 2 + 2;
}

DebuggerInterface::RegisterType DebuggerInterface::GetRegisterType(uint32 index) const
{
  if (index < Reg32_Count)
    return DebuggerInterface::RegisterType::DWord;
  else if (index < (Reg32_Count + Segment_Count))
    return DebuggerInterface::RegisterType::Word;
  else if (index < (Reg32_Count + Segment_Count + 2)) // LDTR, TR
    return DebuggerInterface::RegisterType::Word;
  else if (index < (Reg32_Count + Segment_Count + 2 + 2)) // GDTR, IDTR
    return DebuggerInterface::RegisterType::DWord;
  else
    return DebuggerInterface::RegisterType::Byte;
}

const char* DebuggerInterface::GetRegisterName(uint32 index) const
{
  // GDTR + IDTR + LDTR + TR
  static const char* name_table[Reg32_Count + Segment_Count + 2 + 2] = {
    "EAX",  "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI", "EIP", "EFLAGS", "CR0", "CR2", "CR3", "CR4",
    "DR0",  "DR1", "DR2", "DR3", "DR4", "DR5", "DR6", "DR7", "TR3", "TR4",    "TR5", "TR6", "TR7",

    "ES",   "CS",  "SS",  "DS",  "FS",  "GS",

    "LDTR", "TR",

    "GDTR", "IDTR"};

  DebugAssert(index < countof(name_table));
  return name_table[index];
}

DebuggerInterface::RegisterValue DebuggerInterface::GetRegisterValue(uint32 index) const
{
  RegisterValue value = {};
  if (index < Reg32_Count)
    value.val_dword = m_cpu->GetRegisters()->reg32[index];
  else if (index < Reg32_Count + Segment_Count)
    value.val_word = m_cpu->GetRegisters()->segment_selectors[index - Reg32_Count];
  else if (index == Reg32_Count + Segment_Count + 0)
    value.val_word = m_cpu->GetRegisters()->LDTR;
  else if (index == Reg32_Count + Segment_Count + 1)
    value.val_word = m_cpu->GetRegisters()->LDTR;
  else if (index == Reg32_Count + Segment_Count + 2)
    value.val_dword = m_cpu->GetRegisters()->TR;
  else if (index == Reg32_Count + Segment_Count + 3)
    value.val_dword = 0;

  return value;
}

void DebuggerInterface::SetRegisterValue(uint32 index, RegisterValue value) const
{
  Panic("Fix me");
}

bool DebuggerInterface::ReadMemoryByte(LinearMemoryAddress address, uint8* value)
{
  return m_cpu->SafeReadMemoryByte(address, value, false, false);
}

bool DebuggerInterface::ReadMemoryWord(LinearMemoryAddress address, uint16* value)
{
  return m_cpu->SafeReadMemoryWord(address, value, false, false);
}

bool DebuggerInterface::ReadMemoryDWord(LinearMemoryAddress address, uint32* value)
{
  return m_cpu->SafeReadMemoryDWord(address, value, false, false);
}

bool DebuggerInterface::WriteMemoryByte(LinearMemoryAddress address, uint8 value)
{
  return m_cpu->SafeWriteMemoryByte(address, value, false, false);
}

bool DebuggerInterface::WriteMemoryWord(LinearMemoryAddress address, uint16 value)
{
  return m_cpu->SafeWriteMemoryWord(address, value, false, false);
}

bool DebuggerInterface::WriteMemoryDWord(LinearMemoryAddress address, uint32 value)
{
  return m_cpu->SafeWriteMemoryDWord(address, value, false, false);
}

bool DebuggerInterface::ReadPhysicalMemoryByte(PhysicalMemoryAddress address, uint8* value)
{
  return m_cpu->SafeReadMemoryByte(address, value, false, false);
}

bool DebuggerInterface::ReadPhysicalMemoryWord(PhysicalMemoryAddress address, uint16* value)
{
  return m_cpu->SafeReadMemoryWord(address, value, false, false);
}

bool DebuggerInterface::ReadPhysicalMemoryDWord(PhysicalMemoryAddress address, uint32* value)
{
  return m_cpu->SafeReadMemoryDWord(address, value, false, false);
}

bool DebuggerInterface::WritePhysicalMemoryByte(PhysicalMemoryAddress address, uint8 value)
{
  return m_cpu->SafeWriteMemoryByte(address, value, false, false);
}

bool DebuggerInterface::WritePhysicalMemoryWord(PhysicalMemoryAddress address, uint16 value)
{
  return m_cpu->SafeWriteMemoryWord(address, value, false, false);
}

bool DebuggerInterface::WritePhysicalMemoryDWord(PhysicalMemoryAddress address, uint32 value)
{
  return m_cpu->SafeWriteMemoryDWord(address, value, false, false);
}

LinearMemoryAddress DebuggerInterface::GetInstructionPointer() const
{
  // This needs the linear address of the instruction pointer
  return m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->GetRegisters()->EIP);
}

DebuggerInterface::RegisterType DebuggerInterface::GetStackValueType() const
{
  if (m_cpu->GetCurrentOperandSize() == OperandSize_32)
    return RegisterType::DWord;
  else
    return RegisterType::Word;
}

LinearMemoryAddress DebuggerInterface::GetStackTop() const
{
  LinearMemoryAddress address;
  if (m_cpu->GetStackAddressSize() == AddressSize_16)
    address = ZeroExtend32(m_cpu->GetRegisters()->SP);
  else
    address = m_cpu->GetRegisters()->ESP;

  return address;
}

LinearMemoryAddress DebuggerInterface::GetStackBottom() const
{
  // TODO: Pull this from the segment descriptors..
  if (m_cpu->GetStackAddressSize() == AddressSize_16)
    return UINT32_C(0xFFFF);
  else
    return UINT32_C(0xFFFFFFFF);
}

struct DebuggerFetchInstructionByteCallback : InstructionFetchCallback
{
  DebuggerFetchInstructionByteCallback(CPU_X86::CPU* cpu_, uint32 address_) : cpu(cpu_), address(address_), fail(false)
  {
  }

  virtual uint8 FetchByte() override
  {
    uint8 value;
    if (!cpu->SafeReadMemoryByte(address++, &value, false, false))
    {
      fail = true;
      return 0;
    }
    return value;
  }

  CPU_X86::CPU* cpu;
  uint32 address;
  bool fail;
};

bool DebuggerInterface::DisassembleCode(LinearMemoryAddress address, String* out_line, uint32* out_size) const
{
  OldInstruction instruction;

  DebuggerFetchInstructionByteCallback fetch_callback(m_cpu, address);
  if (!CPU_X86::DecodeInstruction(&instruction, m_cpu->GetCurrentAddressingSize(), m_cpu->GetCurrentOperandSize(),
                                  fetch_callback) ||
      fetch_callback.fail)
  {
    return false;
  }
  if (out_line)
  {
    if (!CPU_X86::DisassembleToString(&instruction, address, out_line))
      return false;
  }
  if (out_size)
    *out_size = instruction.length;
  return true;
}

bool DebuggerInterface::IsStepping() const
{
  return m_stepping;
}

void DebuggerInterface::SetStepping(bool enabled, uint32 instructions_to_execute)
{
  m_stepping = enabled;
  m_stepping_instructions = instructions_to_execute;
  if (!enabled || m_stepping_instructions > 0)
    m_cpu->GetSystem()->SetState(System::State::Running);
  else
    m_cpu->GetSystem()->SetState(System::State::Paused);
}

uint32 DebuggerInterface::GetSteppingInstructionCount()
{
  uint32 temp = m_stepping_instructions;
  m_stepping_instructions = 0;
  return temp;
}

} // namespace CPU_X86
