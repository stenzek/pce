#pragma once
#include "pce/cpu_8086/types.h"

namespace CPU_8086 {
class CPU;

struct Instruction
{
  struct Operand
  {
    OperandSize size;
    OperandMode mode;
    union
    {
      u32 data;

      Reg8 reg8;
      Reg16 reg16;
      Segment segreg;
      JumpCondition jump_condition;
      u32 constant;
    };
  };

  Operation operation;
  Operand operands[3];
  InstructionData data;
  VirtualMemoryAddress address;
  u8 length;

  // Helper methods here.
  u8 GetModRMByte() const { return data.modrm; }
  u8 GetModRM_Reg() const { return data.GetModRM_Reg(); }
  bool ModRM_RM_IsReg() const { return data.ModRM_RM_IsReg(); }
  Segment GetMemorySegment() const { return data.segment; }
  bool IsRegisterOperand(size_t index) const
  {
    const OperandMode mode = operands[index].mode;
    return (mode == OperandMode_Register ||
            (mode == OperandMode_ModRM_SegmentReg || mode == OperandMode_ModRM_ControlRegister ||
             mode == OperandMode_ModRM_DebugRegister || mode == OperandMode_ModRM_TestRegister ||
             mode == OperandMode_ModRM_Reg || (mode == OperandMode_ModRM_RM && data.ModRM_RM_IsReg())));
  }
};

} // namespace CPU_8086
