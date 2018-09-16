#pragma once

#include "YBaseLib/Common.h"
#include "common/bitfield.h"
#include "pce/types.h"

namespace CPU_8086 {

// We use 32-bit virtual memory addresses
// When operating in the lower modes these are zero-extended.
using VirtualMemoryAddress = uint16;

enum Model : uint8
{
  MODEL_8088,
  MODEL_8086,
  MODEL_V20,
  MODEL_V30,
  MODEL_80188,
  MODEL_80186,
  NUM_MODELS
};

enum Reg8 : uint8
{
  Reg8_AL,
  Reg8_CL,
  Reg8_DL,
  Reg8_BL,
  Reg8_AH,
  Reg8_CH,
  Reg8_DH,
  Reg8_BH,

  Reg8_Count
};

enum Reg16 : uint8
{
  // General-purpose registers
  Reg16_AX,
  Reg16_CX,
  Reg16_DX,
  Reg16_BX,

  // Stack pointers
  Reg16_SP,
  Reg16_BP,

  // Source/destination index registers
  Reg16_SI,
  Reg16_DI,

  Reg16_IP,
  Reg16_FLAGS,

  Reg16_ES,
  Reg16_CS,
  Reg16_SS,
  Reg16_DS,

  Reg16_Count
};

enum Flags : uint32
{
  Flag_CF = (1u << 0),
  Flag_Reserved = (1u << 1),
  Flag_PF = (1u << 2),
  Flag_AF = (1u << 4),
  Flag_ZF = (1u << 6),
  Flag_SF = (1u << 7),
  Flag_TF = (1u << 8),
  Flag_IF = (1u << 9),
  Flag_DF = (1u << 10),
  Flag_OF = (1u << 11)
};

enum Operation : uint8
{
  Operation_Invalid,
  Operation_Extension,
  Operation_Extension_ModRM_Reg,
  Operation_Segment_Prefix,
  Operation_Rep_Prefix,
  Operation_RepNE_Prefix,
  Operation_Lock_Prefix,
  Operation_Escape,

  Operation_AAA,
  Operation_AAD,
  Operation_AAM,
  Operation_AAS,
  Operation_ADC,
  Operation_ADD,
  Operation_AND,
  Operation_CALL_Near,
  Operation_CALL_Far,
  Operation_CBW,
  Operation_CLC,
  Operation_CLD,
  Operation_CLI,
  Operation_CMC,
  Operation_CMP,
  Operation_CMPS,
  Operation_CWD,
  Operation_DAA,
  Operation_DAS,
  Operation_DEC,
  Operation_DIV,
  Operation_ESC,
  Operation_HLT,
  Operation_IDIV,
  Operation_IMUL,
  Operation_IN,
  Operation_INC,
  Operation_INT,
  Operation_INTO,
  Operation_IRET,
  Operation_Jcc,
  Operation_JCXZ,
  Operation_JMP_Near,
  Operation_JMP_Far,
  Operation_LAHF,
  Operation_LDS,
  Operation_LEA,
  Operation_LES,
  Operation_LOCK,
  Operation_LODS,
  Operation_LOOP,
  Operation_LXS,
  Operation_MOV,
  Operation_MOVS,
  Operation_MOV_Sreg,
  Operation_MUL,
  Operation_NEG,
  Operation_NOP,
  Operation_NOT,
  Operation_OR,
  Operation_OUT,
  Operation_POP,
  Operation_POP_Sreg,
  Operation_POPF,
  Operation_PUSH,
  Operation_PUSH_Sreg,
  Operation_PUSHF,
  Operation_RCL,
  Operation_RCR,
  Operation_REPxx,
  Operation_RET_Near,
  Operation_RET_Far,
  Operation_ROL,
  Operation_ROR,
  Operation_SAHF,
  Operation_SAL,
  Operation_SALC,
  Operation_SAR,
  Operation_SBB,
  Operation_SCAS,
  Operation_SHL,
  Operation_SHR,
  Operation_STC,
  Operation_STD,
  Operation_STI,
  Operation_STOS,
  Operation_SUB,
  Operation_TEST,
  Operation_WAIT,
  Operation_XCHG,
  Operation_XLAT,
  Operation_XOR,

  Operation_Count
};

enum Segment : uint8
{
  Segment_ES,
  Segment_CS,
  Segment_SS,
  Segment_DS,

  Segment_Count
};

enum OperandSize : uint8
{
  OperandSize_8,  // byte
  OperandSize_16, // word

  OperandSize_Count // TODO: Rename to "inherit"
};

enum OperandMode : uint8
{
  OperandMode_None,
  OperandMode_Constant,
  OperandMode_Register,
  OperandMode_RegisterIndirect,
  OperandMode_SegmentRegister,
  OperandMode_Immediate,
  OperandMode_Immediate2,
  OperandMode_Relative,
  OperandMode_Memory,
  OperandMode_FarAddress,
  OperandMode_ModRM_Reg,
  OperandMode_ModRM_RM,
  OperandMode_ModRM_SegmentReg,
  OperandMode_ModRM_ControlRegister,
  OperandMode_ModRM_DebugRegister,
  OperandMode_ModRM_TestRegister,
  OperandMode_FPRegister,
  OperandMode_JumpCondition,
};

inline constexpr bool OperandMode_NeedsModRM(OperandSize op_size, OperandMode op_mode, uint32 op_idx)
{
  return op_mode == OperandMode_ModRM_Reg || op_mode == OperandMode_ModRM_RM ||
         op_mode == OperandMode_ModRM_ControlRegister || op_mode == OperandMode_ModRM_DebugRegister ||
         op_mode == OperandMode_ModRM_TestRegister;
}

inline constexpr bool OperandMode_NeedsImmediate(OperandSize op_size, OperandMode op_mode, uint32 op_idx)
{
  return op_mode == OperandMode_Immediate || op_mode == OperandMode_Immediate2 || op_mode == OperandMode_Relative ||
         op_mode == OperandMode_Memory || op_mode == OperandMode_ModRM_RM || op_mode == OperandMode_FarAddress;
}

enum InstructionFlags : uint8
{
  InstructionFlag_Lock = (1 << 0),
  InstructionFlag_Rep = (1 << 1),
  InstructionFlag_RepEqual = (1 << 2),
  InstructionFlag_RepNotEqual = (1 << 3)
};

enum JumpCondition : uint8
{
  JumpCondition_Always,

  JumpCondition_Overflow,       // OF = 1
  JumpCondition_NotOverflow,    // OF = 0
  JumpCondition_Sign,           // SF = 1
  JumpCondition_NotSign,        // SF = 0
  JumpCondition_Equal,          // ZF = 1
  JumpCondition_NotEqual,       // ZF = 0
  JumpCondition_Below,          // CF = 1
  JumpCondition_AboveOrEqual,   // CF = 0
  JumpCondition_BelowOrEqual,   // CF = 1 or ZF = 1
  JumpCondition_Above,          // CF = 0 and ZF = 0
  JumpCondition_Less,           // SF != OF
  JumpCondition_GreaterOrEqual, // SF = OF
  JumpCondition_LessOrEqual,    // ZF = 1 or SF != OF
  JumpCondition_Greater,        // ZF = 0 and SF = OF
  JumpCondition_Parity,         // PF = 1
  JumpCondition_NotParity,      // PF = 0
  JumpCondition_CXZero,         // CX/ECX = 0

  JumpCondition_Count
};

struct InstructionData
{
  union // +0
  {
    uint16 imm16;
    uint8 imm8;
  };
  union // +2
  {
    uint16 disp16;
    uint8 disp8;
    uint16 imm2_16;
    uint8 imm2_8;
  };

  Segment segment; // +3
  union            // +4
  {
    BitField<uint8, uint8, 0, 3> modrm_rm;
    BitField<uint8, uint8, 3, 3> modrm_reg;
    BitField<uint8, uint8, 6, 2> modrm_mod;
    uint8 modrm;
  };
  union // +5
  {
    struct
    {
      uint8 has_segment_override : 1;
      uint8 has_rep : 1;
      uint8 has_repne : 1;
      uint8 has_lock : 1;
      uint8 : 3;
    };
    uint8 flags;
  };

  // total size: 6 bytes

  uint8 GetModRM_Reg() const { return ((modrm >> 3) & 7); }
  uint8 GetModRM_RM() { return (modrm & 7); }
  bool ModRM_RM_IsReg() const { return (modrm_mod == 0b11); }
};

enum class ModRMAddressingMode : uint8
{
  Register,                 // register contains value
  Direct,                   // operand contains address of value
  Indirect,                 // register contains address of value
  Indexed,                  // address = displacement + register
  BasedIndexed,             // address = register + register
  BasedIndexedDisplacement, // address = displacement + register + register
};

// Processor interrupts
enum Interrupt : uint32
{
  Interrupt_DivideError = 0x00,
  Interrupt_Debugger = 0x01,
  Interrupt_NMI = 0x02,
  Interrupt_Breakpoint = 0x03,
  Interrupt_Overflow = 0x04
};

} // namespace CPU_8086
