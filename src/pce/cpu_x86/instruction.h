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
  uint8 GetModRMByte() const { return data.modrm; }
  uint8 GetModRM_Reg() const { return data.GetModRM_Reg(); }
  bool ModRM_RM_IsReg() const { return data.ModRM_RM_IsReg(); }
  bool HasSIB() const { return data.HasSIB(); }
  Reg32 GetSIBBaseRegister() const { return data.GetSIBBaseRegister(); }
  Reg32 GetSIBIndexRegister() const { return data.GetSIBIndexRegister(); }
  bool HasSIBBase() const { return data.HasSIBBase(); }
  bool HasSIBIndex() const { return data.HasSIBIndex(); }
  uint8 GetSIBScaling() const { return data.GetSIBScaling(); }
  uint32 GetAddressMask() const { return data.GetAddressMask(); }
  Segment GetMemorySegment() const { return data.segment; }
  bool Is32Bit() const { return data.Is32Bit(); }
  bool IsRegisterOperand(size_t index) const
  {
    const OperandMode mode = operands[index].mode;
    return (mode == OperandMode_Register ||
            (mode == OperandMode_ModRM_SegmentReg || mode == OperandMode_ModRM_ControlRegister ||
             mode == OperandMode_ModRM_DebugRegister || mode == OperandMode_ModRM_TestRegister ||
             mode == OperandMode_ModRM_Reg || (mode == OperandMode_ModRM_RM && data.ModRM_RM_IsReg())));
  }
};

} // namespace CPU_X86
