#include "pce/cpu_x86/code_cache_types.h"
#include <optional>

namespace CPU_X86 {

BlockBase::BlockBase(const BlockKey key_) : key(key_) {}

bool IsExitBlockInstruction(const Instruction* instruction)
{
  switch (instruction->operation)
  {
    case Operation_JMP_Near:
    case Operation_JMP_Far:
    case Operation_LOOP:
    case Operation_Jcc:
    case Operation_JCXZ:
    case Operation_CALL_Near:
    case Operation_CALL_Far:
    case Operation_RET_Near:
    case Operation_RET_Far:
    case Operation_INT:
    case Operation_INT3:
    case Operation_INTO:
    case Operation_IRET:
    case Operation_HLT:
    case Operation_INVLPG:
    case Operation_BOUND:
      return true;

      // STI strictly shouldn't be an issue, but if a block has STI..CLI in the same block,
      // the interrupt flag will never be checked, resulting in hangs.
    case Operation_STI:
      return true;

    case Operation_MOV_CR:
    {
      // Changing CR0 changes processor behavior, and changing CR3 modifies page mappings.
      if (instruction->operands[0].mode == OperandMode_ModRM_ControlRegister)
      {
        const u8 cr_index = instruction->data.GetModRM_Reg();
        if (cr_index == 0 || cr_index == 3 || cr_index == 4)
          return true;
      }
    }
    break;

    case Operation_MOV_Sreg:
    {
      // Since we use SS as a block key, mov ss, <val> should exit the block.
      if (instruction->operands[0].mode == OperandMode_ModRM_SegmentReg &&
          instruction->data.GetModRM_Reg() == Segment_SS)
      {
        return true;
      }
    }
    break;

    case Operation_MOV_DR:
    {
      // Enabling debug registers should disable the code cache backend.
      if (instruction->operands[0].mode == OperandMode_ModRM_DebugRegister && instruction->data.GetModRM_Reg() >= 3)
        return true;
    }
    break;
  }

  // Exit block on invalid instruction.
  return IsInvalidInstruction(*instruction);
}

bool IsLinkableExitInstruction(const Instruction* instruction)
{
  switch (instruction->operation)
  {
    case Operation_JMP_Near:
    case Operation_Jcc:
    case Operation_JCXZ:
    case Operation_LOOP:
    case Operation_CALL_Near:
    case Operation_RET_Near:
    case Operation_INVLPG:
      return true;

    default:
      break;
  }

  return false;
}

bool CanInstructionFault(const Instruction* instruction)
{
  switch (instruction->operation)
  {
    case Operation_AAA:
    case Operation_AAD:
    case Operation_AAM:
    case Operation_AAS:
    case Operation_CLD:
    case Operation_CLC:
    case Operation_STC:
    case Operation_STD:
    case Operation_LEA:
      return false;

    case Operation_ADD:
    case Operation_ADC:
    case Operation_SUB:
    case Operation_SBB:
    case Operation_AND:
    case Operation_XOR:
    case Operation_OR:
    case Operation_CMP:
    case Operation_TEST:
    case Operation_MOV:
    {
      for (u32 i = 0; i < 2; i++)
      {
        if (!instruction->IsRegisterOperand(i) && instruction->operands[i].mode != OperandMode_Immediate)
        {
          return true;
        }
      }
      return false;
    }

    case Operation_INC:
    case Operation_DEC:
    case Operation_NEG:
    case Operation_NOT:
    {
      return (!instruction->IsRegisterOperand(0) && instruction->operands[0].mode != OperandMode_Immediate);
    }

    default:
      return true;
  }
}

std::optional<u8> GetOperandRegister(const Instruction& instruction, u32 index)
{
  const Instruction::Operand& operand = instruction.operands[index];
  if (operand.mode == OperandMode_Register)
    return static_cast<u8>(operand.reg32);
  if (operand.mode == OperandMode_ModRM_Reg)
    return static_cast<u8>(instruction.GetModRM_Reg());
  if (operand.mode == OperandMode_ModRM_RM && instruction.ModRM_RM_IsReg())
    return static_cast<u8>(instruction.GetModRM_RM_Reg());
  else
    return std::nullopt;
}

bool OperandIsESP(const Instruction& instruction, u32 index)
{
  // If any instructions manipulate ESP, we need to update the shadow variable for the next instruction.
  if (instruction.operands[index].size <= OperandSize_8)
    return false;

  const auto reg = GetOperandRegister(instruction, index);
  return reg && *reg == Reg32_ESP;
}

bool OperandRegistersMatch(const Instruction& instruction, u32 index1, u32 index2)
{
  if (instruction.operands[index1].size != instruction.operands[index2].size)
    return false;

  const auto reg1 = GetOperandRegister(instruction, index1);
  const auto reg2 = GetOperandRegister(instruction, index2);
  if (!reg1 || !reg2)
    return false;

  return *reg1 == *reg2;
}

bool IsInvalidInstruction(const Instruction& instruction)
{
  if (instruction.data.has_lock)
  {
    // many instructions with lock prefix is invalid
    switch (instruction.operation)
    {
      case Operation_MOV:
      case Operation_INVLPG:
      case Operation_INVD:
      case Operation_WBINVD:
        return true;
    }
  }

  return false;
}

} // namespace CPU_X86
