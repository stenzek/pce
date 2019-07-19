#include "pce/cpu_x86/code_cache_types.h"

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

  return false;
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

}