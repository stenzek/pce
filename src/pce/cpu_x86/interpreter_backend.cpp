#include "pce/cpu_x86/interpreter_backend.h"
#include "YBaseLib/Endian.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/String.h"
#include "pce/bus.h"
#include "pce/cpu_x86/decoder.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cstdint>
#include <cstdio>

#ifdef Y_COMPILER_MSVC
#include <intrin.h>
#endif

namespace CPU_X86 {
Log_SetChannel(CPU_X86::Interpreter);

extern bool TRACE_EXECUTION;
extern uint32 TRACE_EXECUTION_LAST_EIP;

InterpreterBackend::InterpreterBackend(CPU* cpu) : m_cpu(cpu), m_system(cpu->GetSystem()), m_bus(cpu->GetBus()) {}

InterpreterBackend::~InterpreterBackend() {}

void InterpreterBackend::Reset() {}

void InterpreterBackend::Execute()
{
  fastjmp_set(&m_jmp_buf);

  while (!m_cpu->IsHalted() && m_cpu->m_execution_downcount > 0)
  {
    // Check for external interrupts.
    if (m_cpu->HasExternalInterrupt())
      m_cpu->DispatchExternalInterrupt();

    ExecuteInstruction(m_cpu);

    // Run events if needed.
    m_cpu->CommitPendingCycles();
  }
}

void InterpreterBackend::AbortCurrentInstruction()
{
  // Log_WarningPrintf("Executing longjmp()");
  m_cpu->CommitPendingCycles();
  fastjmp_jmp(&m_jmp_buf);
}

void InterpreterBackend::BranchTo(uint32 new_EIP) {}

void InterpreterBackend::BranchFromException(uint32 new_EIP) {}

void InterpreterBackend::OnControlRegisterLoaded(Reg32 reg, uint32 old_value, uint32 new_value) {}

void InterpreterBackend::FlushCodeCache() {}

void InterpreterBackend::ExecuteInstruction(CPU* cpu)
{
  // The instruction that sets the trap flag should not trigger an interrupt.
  // To handle this, we store the trap flag state before processing the instruction.
  cpu->m_trap_after_instruction = cpu->m_registers.EFLAGS.TF;

  // Store current instruction address in m_current_EIP.
  // The address of the current instruction is needed when exceptions occur.
  cpu->m_current_EIP = cpu->m_registers.EIP;
  cpu->m_current_ESP = cpu->m_registers.ESP;

#if 0
  LinearMemoryAddress linear_address = cpu->CalculateLinearAddress(Segment_CS, cpu->m_registers.EIP);
  if (linear_address == 0xFFE53DC6)
    TRACE_EXECUTION = true;
#endif

  if (TRACE_EXECUTION)
  {
    if (TRACE_EXECUTION_LAST_EIP != cpu->m_current_EIP)
      cpu->PrintCurrentStateAndInstruction();
    TRACE_EXECUTION_LAST_EIP = cpu->m_current_EIP;
  }

  // Cycle for this execution
  cpu->AddCycle();

  // Initialize istate for this instruction
  std::memset(&cpu->idata, 0, sizeof(cpu->idata));
  cpu->idata.address_size = cpu->m_current_address_size;
  cpu->idata.operand_size = cpu->m_current_operand_size;
  cpu->idata.segment = Segment_DS;

  // Read and decode an instruction from the current IP.
  Dispatch_Base(cpu);

  if (cpu->m_trap_after_instruction)
  {
    // We should push the next instruction pointer, not the instruction that's trapping,
    // since it has already executed. We also can't use m_cpu->RaiseException since this would
    // reset the stack pointer too (and it could be a stack-modifying instruction). We
    // also don't need to abort the current instruction since we're looping anyway.
    cpu->SetupInterruptCall(Interrupt_Debugger, false, false, 0, cpu->m_registers.EIP);
  }
}

void InterpreterBackend::RaiseInvalidOpcode(CPU* cpu)
{
  // This set is only here because of the Print call. RaiseException will reset it itself.
  // cpu->m_registers.EIP = cpu->m_current_EIP;
  // cpu->PrintCurrentStateAndInstruction("Invalid opcode raised");
  cpu->RaiseException(Interrupt_InvalidOpcode);
}

void InterpreterBackend::FetchModRM(CPU* cpu)
{
  cpu->idata.modrm = cpu->FetchInstructionByte();
}

template<OperandSize op_size, OperandMode op_mode, uint32 op_constant>
void CPU_X86::InterpreterBackend::FetchImmediate(CPU* cpu)
{
  switch (op_mode)
  {
    case OperandMode_Immediate:
    {
      OperandSize actual_size = (op_size == OperandSize_Count) ? cpu->idata.operand_size : op_size;
      switch (actual_size)
      {
        case OperandSize_8:
          cpu->idata.imm8 = cpu->FetchInstructionByte();
          break;
        case OperandSize_16:
          cpu->idata.imm16 = cpu->FetchInstructionWord();
          break;
        case OperandSize_32:
          cpu->idata.imm32 = cpu->FetchInstructionDWord();
          break;
      }
    }
    break;

    case OperandMode_Immediate2:
    {
      OperandSize actual_size = (op_size == OperandSize_Count) ? cpu->idata.operand_size : op_size;
      switch (actual_size)
      {
        case OperandSize_8:
          cpu->idata.imm2_8 = cpu->FetchInstructionByte();
          break;
        case OperandSize_16:
          cpu->idata.imm2_16 = cpu->FetchInstructionWord();
          break;
        case OperandSize_32:
          cpu->idata.imm2_32 = cpu->FetchInstructionDWord();
          break;
      }
    }
    break;

    case OperandMode_Relative:
    {
      OperandSize actual_size = (op_size == OperandSize_Count) ? cpu->idata.operand_size : op_size;
      switch (actual_size)
      {
        case OperandSize_8:
          cpu->idata.disp32 = SignExtend32(cpu->FetchInstructionByte());
          break;
        case OperandSize_16:
          cpu->idata.disp32 = SignExtend32(cpu->FetchInstructionWord());
          break;
        case OperandSize_32:
          cpu->idata.disp32 = SignExtend32(cpu->FetchInstructionDWord());
          break;
      }
    }
    break;

    case OperandMode_Memory:
    {
      if (cpu->idata.address_size == AddressSize_16)
        cpu->idata.disp32 = ZeroExtend32(cpu->FetchInstructionWord());
      else
        cpu->idata.disp32 = cpu->FetchInstructionDWord();
    }
    break;

    case OperandMode_FarAddress:
    {
      if (cpu->idata.operand_size == OperandSize_16)
        cpu->idata.disp32 = ZeroExtend32(cpu->FetchInstructionWord());
      else
        cpu->idata.disp32 = cpu->FetchInstructionDWord();
      cpu->idata.imm16 = cpu->FetchInstructionWord();
    }
    break;

    case OperandMode_ModRM_RM:
    {
      const Decoder::ModRMAddress* addr = Decoder::DecodeModRMAddress(cpu->idata.address_size, cpu->idata.modrm);
      if (addr->addressing_mode == ModRMAddressingMode::Register)
      {
        // not a memory address
        cpu->idata.modrm_rm_register = true;
      }
      else
      {
        uint8 displacement_size = addr->displacement_size;
        if (addr->addressing_mode == ModRMAddressingMode::SIB)
        {
          // SIB has a displacement instead of base if set to EBP
          cpu->idata.sib = cpu->FetchInstructionByte();
          const Reg32 base_reg = cpu->idata.GetSIBBaseRegister();
          if (!cpu->idata.HasSIBBase())
            displacement_size = 4;
          else if (!cpu->idata.has_segment_override && (base_reg == Reg32_ESP || base_reg == Reg32_EBP))
            cpu->idata.segment = Segment_SS;
        }
        else
        {
          if (!cpu->idata.has_segment_override)
            cpu->idata.segment = addr->default_segment;
        }

        switch (displacement_size)
        {
          case 1:
            cpu->idata.disp32 = SignExtend32(cpu->FetchInstructionByte());
            break;
          case 2:
            cpu->idata.disp32 = SignExtend32(cpu->FetchInstructionWord());
            break;
          case 4:
            cpu->idata.disp32 = cpu->FetchInstructionDWord();
            break;
        }
      }
    }
    break;
  }
}

} // namespace CPU_X86

#include "pce/cpu_x86/interpreter.h"
#include "pce/cpu_x86/interpreter.inl"
#include "pce/cpu_x86/interpreter_x87.inl"
#include "pce/cpu_x86/opcodes.h"

namespace CPU_X86 {

// Dispatchers shared by both one and two byte (0x0F prefix) opcodes
#define MakeInvalidOpcode(opcode)                                                                                      \
  case opcode:                                                                                                         \
    RaiseInvalidOpcode(cpu);                                                                                           \
    return;
#define MakeSegmentPrefix(opcode, seg)                                                                                 \
  case opcode:                                                                                                         \
    cpu->idata.segment = seg;                                                                                          \
    cpu->idata.has_segment_override = true;                                                                            \
    continue;
#define MakeOperandSizePrefix(opcode)                                                                                  \
  case opcode:                                                                                                         \
    cpu->idata.operand_size = (cpu->m_current_operand_size == OperandSize_16) ? OperandSize_32 : OperandSize_16;       \
    continue;
#define MakeAddressSizePrefix(opcode)                                                                                  \
  case opcode:                                                                                                         \
    cpu->idata.address_size = (cpu->m_current_address_size == AddressSize_16) ? AddressSize_32 : AddressSize_16;       \
    continue;
#define MakeLockPrefix(opcode)                                                                                         \
  case opcode:                                                                                                         \
    cpu->idata.has_lock = true;                                                                                        \
    continue;
#define MakeRepPrefix(opcode)                                                                                          \
  case opcode:                                                                                                         \
    cpu->idata.has_rep = true;                                                                                         \
    continue;
#define MakeRepNEPrefix(opcode)                                                                                        \
  case opcode:                                                                                                         \
    cpu->idata.has_rep = true;                                                                                         \
    cpu->idata.has_repne = true;                                                                                       \
    continue;
#define MakeNop(opcode)                                                                                                \
  case opcode:                                                                                                         \
    cpu->AddCycles(CYCLES_NOP);                                                                                        \
    return;

#define MakeNoOperands(opcode, inst)                                                                                   \
  case opcode:                                                                                                         \
    Interpreter::Execute_##inst(cpu);                                                                                  \
    return;
#define MakeOneOperand(opcode, inst, op1)                                                                              \
  case opcode:                                                                                                         \
    if constexpr (OperandMode_NeedsModRM(op1))                                                                         \
      FetchModRM(cpu);                                                                                                 \
    if constexpr (OperandMode_NeedsImmediate(op1))                                                                     \
      FetchImmediate<op1>(cpu);                                                                                        \
    Interpreter::Execute_##inst<op1>(cpu);                                                                             \
    return;
#define MakeOneOperandCC(opcode, inst, cc, op1)                                                                        \
  case opcode:                                                                                                         \
    if constexpr (OperandMode_NeedsModRM(op1))                                                                         \
      FetchModRM(cpu);                                                                                                 \
    if constexpr (OperandMode_NeedsImmediate(op1))                                                                     \
      FetchImmediate<op1>(cpu);                                                                                        \
    Interpreter::Execute_##inst<cc, op1>(cpu);                                                                         \
    return;
#define MakeTwoOperands(opcode, inst, op1, op2)                                                                        \
  case opcode:                                                                                                         \
    if constexpr (OperandMode_NeedsModRM(op1) || OperandMode_NeedsModRM(op2))                                          \
      FetchModRM(cpu);                                                                                                 \
    if constexpr (OperandMode_NeedsImmediate(op1))                                                                     \
      FetchImmediate<op1>(cpu);                                                                                        \
    if constexpr (OperandMode_NeedsImmediate(op2))                                                                     \
      FetchImmediate<op2>(cpu);                                                                                        \
    Interpreter::Execute_##inst<op1, op2>(cpu);                                                                        \
    return;
#define MakeTwoOperandsCC(opcode, inst, cc, op1, op2)                                                                  \
  case opcode:                                                                                                         \
    if constexpr (OperandMode_NeedsModRM(op1) || OperandMode_NeedsModRM(op2))                                          \
      FetchModRM(cpu);                                                                                                 \
    if constexpr (OperandMode_NeedsImmediate(op1))                                                                     \
      FetchImmediate<op1>(cpu);                                                                                        \
    if constexpr (OperandMode_NeedsImmediate(op2))                                                                     \
      FetchImmediate<op2>(cpu);                                                                                        \
    Interpreter::Execute_##inst<cc, op1, op2>(cpu);                                                                    \
    return;
#define MakeThreeOperands(opcode, inst, op1, op2, op3)                                                                 \
  case opcode:                                                                                                         \
    if constexpr (OperandMode_NeedsModRM(op1) || OperandMode_NeedsModRM(op2) || OperandMode_NeedsModRM(op3))           \
      FetchModRM(cpu);                                                                                                 \
    if constexpr (OperandMode_NeedsImmediate(op1))                                                                     \
      FetchImmediate<op1>(cpu);                                                                                        \
    if constexpr (OperandMode_NeedsImmediate(op2))                                                                     \
      FetchImmediate<op2>(cpu);                                                                                        \
    if constexpr (OperandMode_NeedsImmediate(op3))                                                                     \
      FetchImmediate<op3>(cpu);                                                                                        \
    Interpreter::Execute_##inst<op1, op2, op3>(cpu);                                                                   \
    return;
#define MakeExtension(opcode, prefix)                                                                                  \
  case opcode:                                                                                                         \
    Dispatch_Prefix_##prefix(cpu);                                                                                     \
    return;
#define MakeModRMRegExtension(opcode, prefix)                                                                          \
  case opcode:                                                                                                         \
    Dispatch_Prefix_##prefix(cpu);                                                                                     \
    return;
#define MakeX87Extension(opcode, prefix)                                                                               \
  case opcode:                                                                                                         \
    Dispatch_Prefix_##prefix(cpu);                                                                                     \
    return;

void InterpreterBackend::Dispatch_Base(CPU* cpu)
{
  for (;;)
  {
    uint8 opcode = cpu->FetchInstructionByte();
    switch (opcode)
    {
      EnumBaseOpcodes()
    }
  }
}

// Clear out no-longer-referenced forms
#undef MakeSegmentPrefix
#undef MakeOperandSizePrefix
#undef MakeAddressSizePrefix
#undef MakeLockPrefix
#undef MakeRepPrefix
#undef MakeRepNEPrefix
#undef MakeExtension
#undef MakeX87Extension

void InterpreterBackend::Dispatch_Prefix_0f(CPU* cpu)
{
  for (;;)
  {
    uint8 opcode = cpu->FetchInstructionByte();
    switch (opcode)
    {
      EnumPrefix0FOpcodes()
    }
  }
}

// The remaining dispatchers assume modrm has been fetched already.
#undef MakeModRMRegExtension
#undef MakeNoOperands
#undef MakeOneOperand
#undef MakeOneOperandCC
#undef MakeTwoOperands
#undef MakeTwoOperandsCC
#undef MakeThreeOperands
#define MakeNoOperands(opcode, inst)                                                                                   \
  case opcode:                                                                                                         \
    Interpreter::Execute_##inst(cpu);                                                                                  \
    return;
#define MakeOneOperand(opcode, inst, op1)                                                                              \
  case opcode:                                                                                                         \
    if constexpr (OperandMode_NeedsImmediate(op1))                                                                     \
      FetchImmediate<op1>(cpu);                                                                                        \
    Interpreter::Execute_##inst<op1>(cpu);                                                                             \
    return;
#define MakeOneOperandCC(opcode, inst, cc, op1)                                                                        \
  case opcode:                                                                                                         \
    if constexpr (OperandMode_NeedsImmediate(op1))                                                                     \
      FetchImmediate<op1>(cpu);                                                                                        \
    Interpreter::Execute_##inst<cc, op1>(cpu);                                                                         \
    return;
#define MakeTwoOperands(opcode, inst, op1, op2)                                                                        \
  case opcode:                                                                                                         \
    if constexpr (OperandMode_NeedsImmediate(op1))                                                                     \
      FetchImmediate<op1>(cpu);                                                                                        \
    if constexpr (OperandMode_NeedsImmediate(op2))                                                                     \
      FetchImmediate<op2>(cpu);                                                                                        \
    Interpreter::Execute_##inst<op1, op2>(cpu);                                                                        \
    return;
#define MakeTwoOperandsCC(opcode, inst, cc, op1, op2)                                                                  \
  case opcode:                                                                                                         \
    if constexpr (OperandMode_NeedsImmediate(op1))                                                                     \
      FetchImmediate<op1>(cpu);                                                                                        \
    if constexpr (OperandMode_NeedsImmediate(op2))                                                                     \
      FetchImmediate<op2>(cpu);                                                                                        \
    Interpreter::Execute_##inst<cc, op1, op2>(cpu);                                                                    \
    return;
#define MakeThreeOperands(opcode, inst, op1, op2, op3)                                                                 \
  case opcode:                                                                                                         \
    if constexpr (OperandMode_NeedsImmediate(op1))                                                                     \
      FetchImmediate<op1>(cpu);                                                                                        \
    if constexpr (OperandMode_NeedsImmediate(op2))                                                                     \
      FetchImmediate<op2>(cpu);                                                                                        \
    if constexpr (OperandMode_NeedsImmediate(op3))                                                                     \
      FetchImmediate<op3>(cpu);                                                                                        \
    Interpreter::Execute_##inst<op1, op2, op3>(cpu);                                                                   \
    return;

void InterpreterBackend::Dispatch_Prefix_0f00(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp6Opcodes()
  }
}

void InterpreterBackend::Dispatch_Prefix_0f01(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp7Opcodes()
  }
}

void InterpreterBackend::Dispatch_Prefix_0fba(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp8Opcodes(Ev, Ib)
  }
}

void InterpreterBackend::Dispatch_Prefix_80(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp1Opcodes(Eb, Ib)
  }
}

void InterpreterBackend::Dispatch_Prefix_81(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp1Opcodes(Ev, Iv)
  }
}

void InterpreterBackend::Dispatch_Prefix_82(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp1Opcodes(Eb, Ib)
  }
}

void InterpreterBackend::Dispatch_Prefix_83(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp1Opcodes(Ev, Ib)
  }
}

void InterpreterBackend::Dispatch_Prefix_c0(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp2Opcodes(Eb, Ib)
  }
}

void InterpreterBackend::Dispatch_Prefix_c1(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp2Opcodes(Ev, Ib)
  }
}

void InterpreterBackend::Dispatch_Prefix_d0(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp2Opcodes(Eb, Cb(1))
  }
}

void InterpreterBackend::Dispatch_Prefix_d1(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp2Opcodes(Ev, Cb(1))
  }
}

void InterpreterBackend::Dispatch_Prefix_d2(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp2Opcodes(Eb, CL)
  }
}

void InterpreterBackend::Dispatch_Prefix_d3(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp2Opcodes(Ev, CL)
  }
}

// Invalid x87 opcodes should still fetch the modrm operands, but fail silently.
#define MakeInvalidX87Opcode(opcode)                                                                                   \
  case opcode:                                                                                                         \
    FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);                                                   \
    Interpreter::StartX87Instruction(cpu);                                                                             \
    return;

void InterpreterBackend::Dispatch_Prefix_d8(CPU* cpu)
{
  FetchModRM(cpu);
  if (!cpu->idata.ModRM_RM_IsReg())
  {
    switch (cpu->idata.GetModRM_Reg() & 0x07)
    {
      EnumX87D8RegOpcodes()
    }
  }
  else
  {
    switch (cpu->idata.modrm & 0x3F)
    {
      EnumX87D8MemOpcodes()
    }
  }
}

void InterpreterBackend::Dispatch_Prefix_d9(CPU* cpu)
{
  FetchModRM(cpu);
  if (!cpu->idata.ModRM_RM_IsReg())
  {
    switch (cpu->idata.GetModRM_Reg() & 0x07)
    {
      EnumX87D9RegOpcodes()
    }
  }
  else
  {
    switch (cpu->idata.modrm & 0x3F)
    {
      EnumX87D9MemOpcodes()
    }
  }
}

void InterpreterBackend::Dispatch_Prefix_da(CPU* cpu)
{
  FetchModRM(cpu);
  if (!cpu->idata.ModRM_RM_IsReg())
  {
    switch (cpu->idata.GetModRM_Reg() & 0x07)
    {
      EnumX87DARegOpcodes()
    }
  }
  else
  {
    switch (cpu->idata.modrm & 0x3F)
    {
      EnumX87DAMemOpcodes()
    }
  }
}

void InterpreterBackend::Dispatch_Prefix_db(CPU* cpu)
{
  FetchModRM(cpu);
  if (!cpu->idata.ModRM_RM_IsReg())
  {
    switch (cpu->idata.GetModRM_Reg() & 0x07)
    {
      EnumX87DBRegOpcodes()
    }
  }
  else
  {
    switch (cpu->idata.modrm & 0x3F)
    {
      EnumX87DBMemOpcodes()
    }
  }
}

void InterpreterBackend::Dispatch_Prefix_dc(CPU* cpu)
{
  FetchModRM(cpu);
  if (!cpu->idata.ModRM_RM_IsReg())
  {
    switch (cpu->idata.GetModRM_Reg() & 0x07)
    {
      EnumX87DCRegOpcodes()
    }
  }
  else
  {
    switch (cpu->idata.modrm & 0x3F)
    {
      EnumX87DCMemOpcodes()
    }
  }
}

void InterpreterBackend::Dispatch_Prefix_dd(CPU* cpu)
{
  FetchModRM(cpu);
  if (!cpu->idata.ModRM_RM_IsReg())
  {
    switch (cpu->idata.GetModRM_Reg() & 0x07)
    {
      EnumX87DDRegOpcodes()
    }
  }
  else
  {
    switch (cpu->idata.modrm & 0x3F)
    {
      EnumX87DDMemOpcodes()
    }
  }
}

void InterpreterBackend::Dispatch_Prefix_de(CPU* cpu)
{
  FetchModRM(cpu);
  if (!cpu->idata.ModRM_RM_IsReg())
  {
    switch (cpu->idata.GetModRM_Reg() & 0x07)
    {
      EnumX87DERegOpcodes()
    }
  }
  else
  {
    switch (cpu->idata.modrm & 0x3F)
    {
      EnumX87DEMemOpcodes()
    }
  }
}

void InterpreterBackend::Dispatch_Prefix_df(CPU* cpu)
{
  FetchModRM(cpu);
  if (!cpu->idata.ModRM_RM_IsReg())
  {
    switch (cpu->idata.GetModRM_Reg() & 0x07)
    {
      EnumX87DFRegOpcodes()
    }
  }
  else
  {
    switch (cpu->idata.modrm & 0x3F)
    {
      EnumX87DFMemOpcodes()
    }
  }
}

void InterpreterBackend::Dispatch_Prefix_f6(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp3aOpcodes(Eb)
  }
}

void InterpreterBackend::Dispatch_Prefix_f7(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp3bOpcodes(Ev)
  }
}

void InterpreterBackend::Dispatch_Prefix_fe(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp4Opcodes(Eb)
  }
}

void InterpreterBackend::Dispatch_Prefix_ff(CPU* cpu)
{
  FetchModRM(cpu);
  switch (cpu->idata.GetModRM_Reg() & 0x07)
  {
    EnumGrp5Opcodes(Ev)
  }
}

} // namespace CPU_X86