#include "interpreter.h"
#include "recompiler_code_generator.h"

namespace CPU_X86::Recompiler {

#if !defined(Y_CPU_X64)

bool CodeGenerator::Compile_Bitwise_Impl(const Instruction& instruction, CycleCount cycles)
{
  CycleCount cycles = 0;
  if (instruction.DestinationMode() == OperandMode_Register && instruction.SourceMode() == OperandMode_Immediate)
    cycles = m_cpu->GetCycles(CYCLES_ALU_REG_IMM);
  else if (instruction.DestinationMode() == OperandMode_ModRM_RM)
    cycles = m_cpu->GetCyclesRM(CYCLES_ALU_RM_MEM_REG, instruction.ModRM_RM_IsReg());
  else if (instruction.SourceMode() == OperandMode_ModRM_RM)
    cycles = m_cpu->GetCyclesRM(CYCLES_ALU_REG_RM_MEM, instruction.ModRM_RM_IsReg());

  InstructionPrologue(instruction, cycles);
  CalculateEffectiveAddress(instruction);

  // TODO: constant folding here
  const OperandSize size = instruction.operands[0].size;
  Value result;
  if (instruction.operation == Operation_TEST)
  {
    // TEST doesn't write the destination back, otherwise it's the same as AND.
    Value lhs = ReadOperand(instruction, 0, size, false);
    Value rhs = ReadOperand(instruction, 1, size, true);
    result = m_register_cache.AllocateScratch(size);
    EmitCopyValue(result.GetHostRegister(), lhs);
    EmitAnd(result.GetHostRegister(), rhs);
  }
  else
  {
    Value lhs = ReadOperand(instruction, 0, size, false, true);
    Value rhs = ReadOperand(instruction, 1, size, true);

    switch (instruction.operation)
    {
      case Operation_AND:
        EmitAnd(lhs.GetHostRegister(), rhs);
        break;

      case Operation_OR:
        EmitOr(lhs.GetHostRegister(), rhs);
        break;

      case Operation_XOR:
        EmitXor(lhs.GetHostRegister(), rhs);
        break;

      default:
        break;
    }

    result = WriteOperand(instruction, 0, std::move(lhs));
  }

  Value eflags = m_register_cache.ReadGuestRegister(Reg32_EFLAGS, true, true);
  EmitAnd(eflags.GetHostRegister(),
          Value::FromConstantU32(~(Flag_OF | Flag_CF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)));
  EmitOr(eflags.GetHostRegister(), GetSignFlag(result));
  EmitOr(eflags.GetHostRegister(), GetParityFlag(result));
  EmitOr(eflags.GetHostRegister(), GetZeroFlag(result));
  m_register_cache.WriteGuestRegister(Reg32_EFLAGS, std::move(eflags));

  if (OperandIsESP(&instruction, instruction.operands[0]))
    SyncCurrentESP();

  return true;
}

#endif

#if !defined(Y_CPU_X64)

bool CodeGenerator::Compile_AddSub_Impl(const Instruction& instruction, CycleCount cycles)
{
  return Compile_Fallback(instruction);
}

#endif

} // namespace CPU_X86::Recompiler