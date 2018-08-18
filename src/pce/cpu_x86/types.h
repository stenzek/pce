#pragma once

#include "YBaseLib/Common.h"
#include "pce/bitfield.h"
#include "pce/types.h"

namespace CPU_X86 {

// We use 32-bit virtual memory addresses
// When operating in the lower modes these are zero-extended.
using VirtualMemoryAddress = uint32;

enum Model : uint8
{
  MODEL_386,
  MODEL_486,
  MODEL_PENTIUM
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

  Reg16_Count
};

enum Reg32 : uint8
{
  // General-purpose registers
  Reg32_EAX,
  Reg32_ECX,
  Reg32_EDX,
  Reg32_EBX,

  // Stack pointers
  Reg32_ESP,
  Reg32_EBP,

  // Source/destination index registers
  Reg32_ESI,
  Reg32_EDI,

  // Instruction pointer
  Reg32_EIP,

  // Flags register
  Reg32_EFLAGS,

  // Control registers
  Reg32_CR0,
  Reg32_CR2,
  Reg32_CR3,
  Reg32_CR4,

  // Debug registers
  Reg32_DR0,
  Reg32_DR1,
  Reg32_DR2,
  Reg32_DR3,
  Reg32_DR4,
  Reg32_DR5,
  Reg32_DR6,
  Reg32_DR7,

  // Test registers
  Reg32_TR3,
  Reg32_TR4,
  Reg32_TR5,
  Reg32_TR6,
  Reg32_TR7,

  Reg32_Count
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
  Flag_OF = (1u << 11),
  Flag_IOPL = (1u << 12) | (1u << 13),
  Flag_NT = (1u << 14),
  Flag_RF = (1u << 16),
  Flag_VM = (1u << 17),
  Flag_AC = (1u << 18),
  Flag_VIF = (1u << 19),
  Flag_VIP = (1u << 20),
  Flag_ID = (1u << 21)
};

enum CR0Bit : uint32
{
  CR0Bit_PE = (1u << 0),  // Protection Enable
  CR0Bit_MP = (1u << 1),  // Monitor Coprocessor
  CR0Bit_EM = (1u << 2),  // Emulation
  CR0Bit_TS = (1u << 3),  // Task Switched
  CR0Bit_ET = (1u << 4),  // Extension Type
  CR0Bit_NE = (1u << 5),  // Numeric Error
  CR0Bit_WP = (1u << 16), // Write Protect
  CR0Bit_AM = (1u << 18), // Alignment Mask
  CR0Bit_NW = (1u << 29), // Not Write Through
  CR0Bit_CD = (1u << 30), // Cache Disable
  CR0Bit_PG = (1u << 31)  // Paging Enable
};

enum class AccessType : uint8
{
  Read = 0,
  Write = 1,
  Execute = 2
};
IMPLEMENT_ENUM_CLASS_BITWISE_OPERATORS(AccessType);
enum class AccessTypeMask : uint8
{
  Read = (1 << 0),
  Write = (1 << 1),
  Execute = (1 << 2),
  ReadWrite = (1 << 0) | (1 << 1),
  All = (1 << 0) | (1 << 1) | (1 << 2),
  None = 0,
};
IMPLEMENT_ENUM_CLASS_BITWISE_OPERATORS(AccessTypeMask);

// this should match the struct in softfloat
struct float80
{
  uint64 low;
  uint16 high;
};
enum FPUPrecision : uint8
{
  FPUPrecision_24 = 0,
  FPUPrecision_53 = 2,
  FPUPrecision_64 = 3
};
enum FPURoundingControl : uint8
{
  FPURoundingControl_Nearest = 0,  // To nearest/even
  FPURoundingControl_Down = 1,     // Towards -Infinity
  FPURoundingControl_Up = 2,       // Towards Infinity
  FPURoundingControl_Truncate = 3, // Towards 0
};
union FPUControlWord
{
  static const uint16 ReservedBits = uint16(0xE0C0);
  static const uint16 FixedBits = uint16(0x0040);

  BitField<uint16, bool, 0, 1> IM;                // Invalid Operation Mask
  BitField<uint16, bool, 1, 1> DM;                // Denormalized Operand Mask
  BitField<uint16, bool, 2, 1> ZM;                // Zero Divide Mask
  BitField<uint16, bool, 3, 1> OM;                // Overflow Mask
  BitField<uint16, bool, 4, 1> UM;                // Underflow Mask
  BitField<uint16, bool, 5, 1> PM;                // Precision Mask
  BitField<uint16, bool, 7, 1> IEM;               // Interrupt Enable Mask
  BitField<uint16, FPUPrecision, 8, 2> PC;        // Precision Control
  BitField<uint16, FPURoundingControl, 10, 2> RC; // Rounding Control
  // BitField<uint16, bool, 12, 1> IM;    // Rounding Control
  uint16 bits;
};
union FPUStatusWord
{
  BitField<uint16, bool, 0, 1> I;
  BitField<uint16, bool, 1, 1> D;
  BitField<uint16, bool, 2, 1> Z;
  BitField<uint16, bool, 3, 1> O;
  BitField<uint16, bool, 4, 1> U;
  BitField<uint16, bool, 5, 1> P;
  BitField<uint16, bool, 6, 1> SF;
  BitField<uint16, bool, 7, 1> IR;
  BitField<uint16, uint8, 8, 1> C0;
  BitField<uint16, uint8, 9, 1> C1;
  BitField<uint16, uint8, 10, 1> C2;
  BitField<uint16, uint8, 11, 3> TOP;
  BitField<uint16, uint8, 14, 1> C3;
  BitField<uint16, bool, 15, 1> B;
  uint16 bits;
};
union FPUTagWord
{
  uint8 Get(uint8 index) const { return Truncate8((bits >> (index * 2)) & 0x03); }
  void Set(uint8 index, uint8 value) { bits = (bits & ~(0x03 << (index * 2))) | ((value & 0x03) << (index * 2)); }
  void SetNonZero(uint8 index) { bits = (bits & ~(0x03 << (index * 2))); }
  void SetZero(uint8 index) { bits = (bits & ~(0x02 << (index * 2))) | (0x01 << (index * 2)); }
  void SetSpecial(uint8 index) { bits = (bits & ~(0x01 << (index * 2))) | (0x02 << (index * 2)); }
  void SetEmpty(uint8 index) { bits |= (0x03 << (index * 2)); }
  bool IsEmpty(uint8 index) { return ((bits >> (index * 2)) & 0x03) == 0x03; }
  uint16 bits;
};

enum Operation : uint8
{
  Operation_Invalid,
  Operation_Extension,
  Operation_Extension_ModRM_Reg,
  Operation_Extension_ModRM_X87,
  Operation_Segment_Prefix,
  Operation_Rep_Prefix,
  Operation_RepNE_Prefix,
  Operation_Lock_Prefix,
  Operation_OperandSize_Prefix,
  Operation_AddressSize_Prefix,

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

  // 80286+
  Operation_BOUND,
  Operation_BSF,
  Operation_BSR,
  Operation_BT,
  Operation_BTS,
  Operation_BTR,
  Operation_BTC,
  Operation_SHLD,
  Operation_SHRD,
  Operation_INS,
  Operation_OUTS,
  Operation_CLTS,
  Operation_ENTER,
  Operation_LEAVE,
  Operation_LSS,
  Operation_LGDT,
  Operation_SGDT,
  Operation_LIDT,
  Operation_SIDT,
  Operation_LLDT,
  Operation_SLDT,
  Operation_LTR,
  Operation_STR,
  Operation_LMSW,
  Operation_SMSW,
  Operation_VERR,
  Operation_VERW,
  Operation_ARPL,
  Operation_LAR,
  Operation_LSL,
  Operation_PUSHA,
  Operation_POPA,

  // 80386+
  Operation_LFS,
  Operation_LGS,
  Operation_MOVSX,
  Operation_MOVZX,
  Operation_MOV_CR,
  Operation_MOV_DR,
  Operation_MOV_TR,
  Operation_SETcc,

  // 80486+
  Operation_BSWAP,
  Operation_CMPXCHG,
  Operation_CMOVcc,
  Operation_INVD,
  Operation_WBINVD,
  Operation_INVLPG,
  Operation_XADD,

  // Pentium+
  Operation_CPUID,
  Operation_RDTSC,
  Operation_CMPXCHG8B,

  // 8087+
  Operation_F2XM1,
  Operation_FABS,
  Operation_FADD,
  Operation_FADDP,
  Operation_FBLD,
  Operation_FBSTP,
  Operation_FCHS,
  Operation_FCOM,
  Operation_FCOMP,
  Operation_FCOMPP,
  Operation_FDECSTP,
  Operation_FDIV,
  Operation_FDIVP,
  Operation_FDIVR,
  Operation_FDIVRP,
  Operation_FFREE,
  Operation_FIADD,
  Operation_FICOM,
  Operation_FICOMP,
  Operation_FIDIV,
  Operation_FIDIVR,
  Operation_FILD,
  Operation_FIMUL,
  Operation_FINCSTP,
  Operation_FINIT,
  Operation_FIST,
  Operation_FISTP,
  Operation_FISUB,
  Operation_FISUBR,
  Operation_FLD,
  Operation_FLD1,
  Operation_FLDCW,
  Operation_FLDENV,
  Operation_FLDL2E,
  Operation_FLDL2T,
  Operation_FLDLG2,
  Operation_FLDLN2,
  Operation_FLDPI,
  Operation_FLDZ,
  Operation_FMUL,
  Operation_FMULP,
  Operation_FNCLEX,
  Operation_FNDISI,
  Operation_FNENI,
  Operation_FNINIT,
  Operation_FNOP,
  Operation_FNSAVE,
  Operation_FNSTCW,
  Operation_FNSTENV,
  Operation_FNSTSW,
  Operation_FPATAN,
  Operation_FPREM,
  Operation_FPTAN,
  Operation_FRNDINT,
  Operation_FRSTOR,
  Operation_FSCALE,
  Operation_FSQRT,
  Operation_FST,
  Operation_FSTP,
  Operation_FSUB,
  Operation_FSUBP,
  Operation_FSUBR,
  Operation_FSUBRP,
  Operation_FTST,
  Operation_FXAM,
  Operation_FXCH,
  Operation_FXTRACT,
  Operation_FYL2X,
  Operation_FYL2XP1,

  // 80287+
  Operation_FSETPM,

  // 80387+
  Operation_FCOS,
  Operation_FPREM1,
  Operation_FSIN,
  Operation_FSINCOS,
  Operation_FUCOM,
  Operation_FUCOMP,
  Operation_FUCOMPP,

  Operation_Count
};

enum Segment : uint8
{
  Segment_ES,
  Segment_CS,
  Segment_SS,
  Segment_DS,
  Segment_FS,
  Segment_GS,

  Segment_Count
};

enum AddressSize : uint8
{
  AddressSize_16,
  AddressSize_32,

  AddressSize_Count
};

enum OperandSize : uint8
{
  OperandSize_8,  // byte
  OperandSize_16, // word
  OperandSize_32, // dword

  OperandSize_64, // double-precision float
  OperandSize_80, // extended-precision float

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
    uint32 imm32;
    uint16 imm16;
    uint8 imm8;
  };
  union // +4
  {
    uint32 disp32;
    uint16 disp16;
    uint8 disp8;
    uint32 imm2_32;
    uint16 imm2_16;
    uint8 imm2_8;
  };

  OperandSize operand_size; // +8
  AddressSize address_size; // +9
  Segment segment;          // +10
  union                     // +11
  {
    BitField<uint8, uint8, 0, 3> modrm_rm;
    BitField<uint8, uint8, 3, 3> modrm_reg;
    BitField<uint8, uint8, 6, 2> modrm_mod;
    uint8 modrm;
  };
  union // +12
  {
    BitField<uint8, Reg32, 0, 3> sib_base_reg;
    BitField<uint8, Reg32, 3, 3> sib_index_reg;
    BitField<uint8, uint8, 6, 2> sib_scaling_factor;
    uint8 sib;
  };
  union // +13
  {
    struct
    {
      uint8 modrm_rm_register : 1;
      uint8 has_segment_override : 1;
      uint8 has_rep : 1;
      uint8 has_repne : 1;
      uint8 has_lock : 1;
      uint8 : 3;
    };
    uint8 flags;
  };

  // total size: 16 bytes (2 padding)

  uint8 GetModRM_Reg() const { return ((modrm >> 3) & 7); }
  uint8 GetModRM_RM() { return (modrm & 7); }
  bool ModRM_RM_IsReg() const { return (modrm_mod == 0b11); }
  bool HasSIB() const { return (modrm_mod != 0b11 && modrm_rm == 0b100); }
  Reg32 GetSIBBaseRegister() const { return static_cast<Reg32>(sib & 0x07); }
  Reg32 GetSIBIndexRegister() const { return static_cast<Reg32>((sib >> 3) & 0x07); }
  bool HasSIBBase() const { return (GetSIBBaseRegister() != Reg32_EBP || modrm_mod != 0b00); }
  bool HasSIBIndex() const { return (GetSIBIndexRegister() != Reg32_ESP); }
  uint8 GetSIBScaling() const { return ((sib >> 6) & 0x03); }
  uint32 GetAddressMask() const { return (address_size == AddressSize_16) ? 0xFFFF : 0xFFFFFFFF; }
  bool Is32Bit() const { return (operand_size == OperandSize_32); }
};

enum class ModRMAddressingMode : uint8
{
  Register,                 // register contains value
  Direct,                   // operand contains address of value
  Indirect,                 // register contains address of value
  Indexed,                  // address = displacement + register
  BasedIndexed,             // address = register + register
  BasedIndexedDisplacement, // address = displacement + register + register
  SIB,                      // address = displacement + base + index * scale
};

// Processor interrupts
enum Interrupt : uint32
{
  Interrupt_DivideError = 0x00,
  Interrupt_Debugger = 0x01,
  Interrupt_NMI = 0x02,
  Interrupt_Breakpoint = 0x03,
  Interrupt_Overflow = 0x04,
  Interrupt_Bounds = 0x05,
  Interrupt_InvalidOpcode = 0x06,
  Interrupt_CoprocessorNotAvailable = 0x07,
  Interrupt_DoubleFault = 0x08,
  Interrupt_CoprocessorSegmentOverflow = 0x09,
  Interrupt_InvalidTaskStateSegment = 0x0A,
  Interrupt_SegmentNotPresent = 0x0B,
  Interrupt_StackFault = 0x0C,
  Interrupt_GeneralProtectionFault = 0x0D,
  Interrupt_PageFault = 0x0E,
  Interrupt_Reserved = 0x0F,
  Interrupt_MathFault = 0x10,
  Interrupt_AlignmentCheck = 0x11,
  //     Interrupt_MachineCheck                  = 0x12,
  //     Interrupt_SIMDFloatingPointException    = 0x13,
  //     Interrupt_VirtualizationException       = 0x14,
  //     Interrupt_ControlProtectionException    = 0x15

  Interrupt_Count = 256
};

enum SEGMENT_DESCRIPTOR_ACCESS_FLAG
{
  SEGMENT_DESCRIPTOR_ACCESS_FLAG_ACCESSED = 0x01,
  SEGMENT_DESCRIPTOR_ACCESS_FLAG_CODE = 0x08,
  SEGMENT_DESCRIPTOR_ACCESS_FLAG_CODE_READABLE = 0x02,
  SEGMENT_DESCRIPTOR_ACCESS_FLAG_CODE_CONFORMING = 0x04,
  SEGMENT_DESCRIPTOR_ACCESS_FLAG_DATA_EXPAND_DOWN = 0x02,
  SEGMENT_DESCRIPTOR_ACCESS_FLAG_DATA_WRITABLE = 0x04,
  SEGMENT_DESCRIPTOR_ACCESS_FLAG_PRIVILEGE = 0x60,
  SEGMENT_DESCRIPTOR_ACCESS_FLAG_PRESENT = 0x80
};

enum DESCRIPTOR_TYPE : uint8
{
  DESCRIPTOR_TYPE_INVALID = 0x00,
  DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16 = 0x01,
  DESCRIPTOR_TYPE_LDT = 0x02,
  DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16 = 0x03,
  DESCRIPTOR_TYPE_CALL_GATE_16 = 0x04,
  DESCRIPTOR_TYPE_TASK_GATE = 0x05,
  DESCRIPTOR_TYPE_INTERRUPT_GATE_16 = 0x06,
  DESCRIPTOR_TYPE_TRAP_GATE_16 = 0x07,
  // ???                                      = 0x08,
  DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32 = 0x09,
  // ???                                      = 0x0A,
  DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32 = 0x0B,
  DESCRIPTOR_TYPE_CALL_GATE_32 = 0x0C,
  // ???                                      = 0x0D,
  DESCRIPTOR_TYPE_INTERRUPT_GATE_32 = 0x0E,
  DESCRIPTOR_TYPE_TRAP_GATE_32 = 0x0F,
};

union SEGMENT_DESCRIPTOR_ACCESS_BITS
{
  uint8 bits;

  BitField<uint8, bool, 0, 1> accessed;
  BitField<uint32, bool, 3, 1> is_code;
  BitField<uint32, bool, 1, 1> code_readable;
  BitField<uint32, bool, 2, 1> code_confirming;
  BitField<uint32, bool, 1, 1> data_expand_down;
  BitField<uint32, bool, 2, 1> data_writable;
  BitField<uint32, uint8, 5, 2> dpl;
  BitField<uint32, bool, 7, 1> present;
};

union DESCRIPTOR_ENTRY
{
  struct
  {
    uint32 bits0;
    uint32 bits1;
  };
  uint64 bits;

  BitField<uint64, uint8, 40, 4> type; // only valid if memory_descriptor = 1
  BitField<uint64, bool, 44, 1> is_memory_descriptor;
  BitField<uint64, uint8, 45, 2> dpl;
  BitField<uint64, bool, 47, 1> present;

  // Memory (code/data) segments
  struct
  {
    union
    {
      BitField<uint64, uint32, 0, 16> limit_0_15;
      BitField<uint64, uint32, 16, 16> base_0_15;
      BitField<uint64, uint32, 32, 8> base_16_23;
      BitField<uint64, uint8, 40, 8> access_bits;
      BitField<uint64, uint32, 48, 4> limit_16_19;
      BitField<uint64, uint8, 52, 4> flags_raw;
      BitField<uint64, uint32, 56, 8> base_24_31;

      union
      {
        BitField<uint64, bool, 40, 1> accessed;
        BitField<uint64, bool, 43, 1> is_code;
        BitField<uint64, bool, 41, 1> code_readable;
        BitField<uint64, bool, 42, 1> code_conforming;
        BitField<uint64, bool, 41, 1> data_writable;
        BitField<uint64, bool, 42, 1> data_expand_down;
      } access;

      union
      {
        BitField<uint64, bool, 54, 1> size;        // on = 32-bit mode
        BitField<uint64, bool, 55, 1> granularity; // on = 4kb limit, 0 = 1 byte limit
      } flags;
    };

    uint32 GetBase() const
    {
      return base_0_15.GetValue() | (base_16_23.GetValue() << 16) | (base_24_31.GetValue() << 24);
    }

    uint32 GetLimit() const
    {
      uint32 limit = limit_0_15.GetValue() | (limit_16_19.GetValue() << 16);
      if (flags.granularity)
        limit = (limit << 12) | 0xFFF;
      return limit;
    }

    uint32 GetLimitLow() const
    {
      // if (!access.is_code && access.data_expand_down)
      if ((access_bits & 0b1100) == 0b0100)
      {
        // limit=c000: limit < address <= ffff/ffffffff
        return GetLimit() + 1;
      }
      else
      {
        // limit=c000: 0 <= address <= c000.
        return 0;
      }
    }

    uint32 GetLimitHigh() const
    {
      if ((access_bits & 0b1100) == 0b0100)
        return flags.size ? UINT32_C(0xFFFFFFFF) : UINT32_C(0xFFFF);
      else
        return GetLimit();
    }

    AddressSize GetAddressSize() const { return flags.size ? AddressSize_32 : AddressSize_16; }
    OperandSize GetOperandSize() const { return flags.size ? OperandSize_32 : OperandSize_16; }
    bool Is32BitSegment() const { return flags.size; }

    bool IsConformingCodeSegment() const { return (access.is_code.GetValue() & access.code_conforming.GetValue()); }

  } memory;

  // System (TSS/LDT segments)
  struct
  {
    union
    {
      BitField<uint64, uint32, 0, 16> limit_0_15;
      BitField<uint64, uint32, 16, 24> base_0_24;
      BitField<uint64, uint32, 48, 4> limit_16_19;
      BitField<uint64, uint32, 56, 8> base_24_31;
      BitField<uint64, bool, 55, 1> granularity; // on = 4kb limit, 0 = 1 byte limit
    };

    uint32 GetBase() const { return base_0_24.GetValue() | (base_24_31.GetValue() << 24); }

    uint32 GetLimit() const
    {
      uint32 limit = (limit_0_15.GetValue() | (limit_16_19.GetValue() << 16));
      if (granularity)
        limit = (limit << 12) | 0xFFF;
      return limit;
    }
  } tss, ldt;

  struct
  {
    union
    {
      BitField<uint64, uint32, 0, 16> offset_0_15;
      BitField<uint64, uint16, 16, 16> selector;
      BitField<uint64, uint32, 48, 16> offset_16_31;
    };

    uint32 GetOffset() const { return offset_0_15.GetValue() | (offset_16_31.GetValue() << 16); }
  } interrupt_gate;

  struct
  {
    union
    {
      BitField<uint64, uint32, 0, 16> offset_0_15;
      BitField<uint64, uint16, 16, 16> selector;
      BitField<uint64, uint8, 32, 4> parameter_count;
      BitField<uint64, uint32, 48, 16> offset_16_31;
    };

    uint32 GetOffset() const { return offset_0_15.GetValue() | (offset_16_31.GetValue() << 16); }
  } call_gate;

  struct
  {
    union
    {
      BitField<uint64, uint16, 16, 16> selector;
    };
  } task_gate;

  bool IsPresent() const { return (present); }

  bool IsSystemSegment() const { return !is_memory_descriptor; }

  bool IsCodeSegment() const { return (is_memory_descriptor && memory.access.is_code); }

  bool IsDataSegment() const { return (is_memory_descriptor && !memory.access.is_code); }

  bool IsWritableDataSegment() const
  {
    return (is_memory_descriptor && !memory.access.is_code && memory.access.data_writable);
  }

  bool IsReadableCodeSegment() const
  {
    return (is_memory_descriptor && memory.access.is_code && memory.access.code_readable);
  }

  bool IsConformingCodeSegment() const
  {
    return (is_memory_descriptor && memory.access.is_code && memory.access.code_conforming);
  }

  bool IsReadableSegment() const
  {
    return (is_memory_descriptor && ((memory.access.is_code && memory.access.code_readable) || !memory.access.is_code));
  }
};

// Visible segment register portion in protected mode
union SEGMENT_SELECTOR_VALUE
{
  uint16 bits;

  BitField<uint16, uint8, 0, 2> rpl;     // requested privilege level
  BitField<uint16, bool, 2, 1> ti;       // use local descriptor table
  BitField<uint16, uint16, 3, 13> index; // descriptor table index

  uint16 ValueForException() const { return (bits & 0xFFFC); }
  bool IsNullSelector() const { return ((bits & 0xFFFC) == 0); }
};

struct INTERRUPT_DESCRIPTOR_ENTRY
{
  union
  {
    uint32 bits0;

    BitField<uint32, uint32, 0, 16> offset_0_15;
    BitField<uint32, uint32, 16, 16> selector;
  };

  union
  {
    uint32 bits1;

    // BitField<uint32, uint8, 0, 8> zero;
    BitField<uint32, uint32, 8, 4> type;
    BitField<uint32, bool, 12, 1> storage_segment;
    BitField<uint32, uint32, 13, 2> descriptor_privilege_level;
    BitField<uint32, bool, 15, 1> present;
    BitField<uint32, uint32, 16, 16> offset_16_31;
  };
};

#pragma pack(push, 1)
struct TASK_STATE_SEGMENT_16
{
  uint16 LINK;
  union
  {
    struct
    {
      uint16 SP;
      uint16 SS;
    } stacks[3];

    struct
    {
      uint16 SP0;
      uint16 SS0;
      uint16 SP1;
      uint16 SS1;
      uint16 SP2;
      uint16 SS2;
    };
  };

  uint16 IP;
  uint16 FLAGS;
  uint16 AX;
  uint16 CX;
  uint16 DX;
  uint16 BX;
  uint16 SP;
  uint16 BP;
  uint16 SI;
  uint16 DI;
  uint16 ES;
  uint16 CS;
  uint16 SS;
  uint16 DS;
  uint16 LDTR;
};
static_assert(sizeof(TASK_STATE_SEGMENT_16) == 0x2C, "Size of TSS16 is correct");
static_assert(((sizeof(TASK_STATE_SEGMENT_16) % sizeof(uint16)) == 0), "Size of TSS16 is word-aligned");
#pragma pack(pop)

#pragma pack(push, 1)
struct TASK_STATE_SEGMENT_32
{
  uint16 LINK;
  uint16 reserved_02;
  union
  {
    struct
    {
      uint32 ESP;
      uint16 SS;
      uint16 reserved;
    } stacks[3];

    struct
    {
      uint32 ESP0;
      uint16 SS0;
      uint16 reserved_08;
      uint32 ESP1;
      uint16 SS1;
      uint16 reserved_10;
      uint32 ESP2;
      uint16 SS2;
      uint16 reserved_18;
    };
  };
  uint32 CR3;
  uint32 EIP;
  uint32 EFLAGS;
  uint32 EAX;
  uint32 ECX;
  uint32 EDX;
  uint32 EBX;
  uint32 ESP;
  uint32 EBP;
  uint32 ESI;
  uint32 EDI;
  uint16 ES;
  uint16 reserved_4A;
  uint16 CS;
  uint16 reserved_4E;
  uint16 SS;
  uint16 reserved_52;
  uint16 DS;
  uint16 reserved_56;
  uint16 FS;
  uint16 reserved_5A;
  uint16 GS;
  uint16 reserved_5E;
  uint16 LDTR;
  uint16 reserved_62;
  uint8 T;
  uint8 reserved_64;
  uint16 IOPB_offset;
  // uint32 SSP;
  // Optional data
  // Optional interrupt redirection bitmap
  // Optional IO permission bitmap
  // 0x7
};
static_assert(sizeof(TASK_STATE_SEGMENT_32) == 0x68, "Size of TSS32 is correct");
static_assert(((sizeof(TASK_STATE_SEGMENT_32) % sizeof(uint32)) == 0), "Size of TSS32 is dword-aligned");
#pragma pack(pop)

union PAGE_DIRECTORY_ENTRY
{
  uint32 bits;

  BitField<uint32, bool, 0, 1> present;
  BitField<uint32, bool, 1, 1> read_write;
  BitField<uint32, bool, 2, 1> user_supervisor;
  BitField<uint32, bool, 3, 1> write_through;
  BitField<uint32, bool, 4, 1> cache_disabled;
  BitField<uint32, bool, 5, 1> accessed;
  BitField<uint32, bool, 7, 1> page_size;
  // BitField<uint32, bool, 8, 1> ignored;
  BitField<uint32, uint32, 12, 20> page_table_address;
};

union PAGE_TABLE_ENTRY
{
  uint32 bits;

  BitField<uint32, bool, 0, 1> present;
  BitField<uint32, bool, 1, 1> read_write;
  BitField<uint32, bool, 2, 1> user_supervisor;
  BitField<uint32, bool, 3, 1> write_through;
  BitField<uint32, bool, 4, 1> cache_disabled;
  BitField<uint32, bool, 5, 1> accessed;
  BitField<uint32, bool, 6, 1> dirty;
  BitField<uint32, bool, 8, 1> global;
  BitField<uint32, uint32, 12, 20> physical_address;
};

} // namespace CPU_X86
