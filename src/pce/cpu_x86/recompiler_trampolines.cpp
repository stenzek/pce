#include "pce/cpu_x86/recompiler_trampolines.h"

namespace CPU_X86 {

uint8 RecompilerTrampolines::ReadMemoryByteTrampoline(CPU* cpu, uint32 segment, uint32 offset)
{
  return cpu->ReadMemoryByte(static_cast<Segment>(segment), offset);
}

uint16 RecompilerTrampolines::ReadMemoryWordTrampoline(CPU* cpu, uint32 segment, uint32 offset)
{
  return cpu->ReadMemoryWord(static_cast<Segment>(segment), offset);
}

uint32 RecompilerTrampolines::ReadMemoryDWordTrampoline(CPU* cpu, uint32 segment, uint32 offset)
{
  return cpu->ReadMemoryDWord(static_cast<Segment>(segment), offset);
}

void RecompilerTrampolines::WriteMemoryByteTrampoline(CPU* cpu, uint32 segment, uint32 offset, uint8 value)
{
  cpu->WriteMemoryByte(static_cast<Segment>(segment), offset, value);
}

void RecompilerTrampolines::WriteMemoryWordTrampoline(CPU* cpu, uint32 segment, uint32 offset, uint16 value)
{
  cpu->WriteMemoryWord(static_cast<Segment>(segment), offset, value);
}

void RecompilerTrampolines::WriteMemoryDWordTrampoline(CPU* cpu, uint32 segment, uint32 offset, uint32 value)
{
  cpu->WriteMemoryDWord(static_cast<Segment>(segment), offset, value);
}

// Necessary due to BranchTo being a member function.
void RecompilerTrampolines::BranchToTrampoline(CPU* cpu, uint32 address)
{
  cpu->BranchTo(address);
}

void RecompilerTrampolines::PushWordTrampoline(CPU* cpu, uint16 value)
{
  cpu->PushWord(value);
}

void RecompilerTrampolines::PushDWordTrampoline(CPU* cpu, uint32 value)
{
  cpu->PushDWord(value);
}

uint16 RecompilerTrampolines::PopWordTrampoline(CPU* cpu)
{
  return cpu->PopWord();
}

uint32 RecompilerTrampolines::PopDWordTrampoline(CPU* cpu)
{
  return cpu->PopDWord();
}

void RecompilerTrampolines::LoadSegmentRegisterTrampoline(CPU* cpu, uint32 segment, uint16 value)
{
  cpu->LoadSegmentRegister(static_cast<Segment>(segment), value);
}

void RecompilerTrampolines::RaiseExceptionTrampoline(CPU* cpu, uint32 interrupt, uint32 error_code)
{
  cpu->RaiseException(interrupt, error_code);
}

void RecompilerTrampolines::SetFlagsTrampoline(CPU* cpu, uint32 flags)
{
  cpu->SetFlags(flags);
}

void RecompilerTrampolines::SetFlags16Trampoline(CPU* cpu, uint16 flags)
{
  cpu->SetFlags16(flags);
}

void RecompilerTrampolines::FarJumpTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size)
{
  cpu->FarJump(segment_selector, offset, static_cast<OperandSize>(op_size));
}

void RecompilerTrampolines::FarCallTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size)
{
  cpu->FarCall(segment_selector, offset, static_cast<OperandSize>(op_size));
}

void RecompilerTrampolines::FarReturnTrampoline(CPU* cpu, uint32 op_size, uint32 pop_count)
{
  cpu->FarReturn(static_cast<OperandSize>(op_size), pop_count);
}

void RecompilerTrampolines::InterpretInstructionTrampoline(CPU* cpu, const Instruction* instruction)
{
  std::memcpy(&cpu->idata, &instruction->data, sizeof(cpu->idata));
  instruction->interpreter_handler(cpu);
}

} // namespace CPU_X86
