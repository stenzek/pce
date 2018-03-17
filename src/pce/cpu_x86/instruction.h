#pragma once
#include "pce/cpu_x86/types.h"

namespace CPU_X86 {
class CPU;

struct Instruction
{
  using IntepreterHandler = void (*)(CPU*);
  struct Operand
  {
    OperandSize size;
    OperandMode mode;
    union
    {
      uint32 data;

      Reg8 reg8;
      Reg16 reg16;
      Reg32 reg32;
      Segment segreg;
      JumpCondition jump_condition;
      uint32 constant;
    };
  };

  Operation operation;
  Operand operands[3];
  InstructionData data;
  IntepreterHandler interpreter_handler;
  VirtualMemoryAddress address;
  uint32 length;

  // Helper methods here.
  OperandSize GetOperandSize() const { return data.operand_size; }
  AddressSize GetAddressSize() const { return data.address_size; }
  uint8 GetModRM_Reg() const { return data.GetModRM_Reg(); }
  bool ModRM_RM_IsReg() const { return data.ModRM_RM_IsReg(); }
};

} // namespace CPU_X86
