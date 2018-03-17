#include "pce/cpu_x86/decode.h"

#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"

//#define DEBUG_DECODER 1

namespace CPU_X86 {
enum DefaultSegment : uint8
{
  // DS placed here so it doesn't need to specified (is zero)
  DefaultSegment_DS,
  DefaultSegment_ES,
  DefaultSegment_CS,
  DefaultSegment_SS,
  DefaultSegment_FS,
  DefaultSegment_GS,
  DefaultSegment_Unspecified
};

struct InstructionTableEntry
{
  Operation operation;
  struct
  {
    OperandSize size;
    OperandMode mode;
    union
    {
      uint32 data;

      Reg8 reg8;
      Reg16 reg16;
      Reg32 reg32;
      uint32 constant;
    };
  } operands[3];
  JumpCondition jump_condition;
  DefaultSegment default_segment;
  const InstructionTableEntry* extension_table;
};

#define Eb                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_ModRM_RM                                                                                \
  }
#define Ev                                                                                                             \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_ModRM_RM                                                                            \
  }
#define Ew                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_ModRM_RM                                                                               \
  }
#define Gb                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_ModRM_Reg                                                                               \
  }
#define Gw                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_ModRM_Reg                                                                              \
  }
#define Gv                                                                                                             \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_ModRM_Reg                                                                           \
  }
#define Sw                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_ModRM_SegmentReg                                                                       \
  }
#define Ib                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Immediate                                                                               \
  }
#define Iw                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Immediate                                                                              \
  }
#define Iv                                                                                                             \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Immediate                                                                           \
  }
#define M                                                                                                              \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_ModRM_RM                                                                            \
  }
#define Ap                                                                                                             \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_FarAddress                                                                          \
  }
#define Mp                                                                                                             \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_ModRM_RM                                                                            \
  }
#define Mw                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_ModRM_RM                                                                               \
  }
#define Md                                                                                                             \
  {                                                                                                                    \
    OperandSize_32, OperandMode_ModRM_RM                                                                               \
  }
#define Mq                                                                                                             \
  {                                                                                                                    \
    OperandSize_64, OperandMode_ModRM_RM                                                                               \
  }
#define Mt                                                                                                             \
  {                                                                                                                    \
    OperandSize_80, OperandMode_ModRM_RM                                                                               \
  }
#define Ob                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Memory                                                                                  \
  }
#define Ow                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Memory                                                                                 \
  }
#define Ov                                                                                                             \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Memory                                                                              \
  }
#define Jb                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Relative                                                                                \
  }
#define Jw                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Relative                                                                               \
  }
#define Jv                                                                                                             \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Relative                                                                            \
  }
#define Cb(n)                                                                                                          \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Constant, (n)                                                                           \
  }
#define Cw(n)                                                                                                          \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Constant, (n)                                                                          \
  }
#define Cv(n)                                                                                                          \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Constant, (n)                                                                       \
  }
#define Ms                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_ModRM_RM                                                                               \
  }
#define Ma                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_ModRM_RM                                                                               \
  }
#define Cd                                                                                                             \
  {                                                                                                                    \
    OperandSize_32, OperandMode_ModRM_ControlRegister                                                                  \
  }
#define Td                                                                                                             \
  {                                                                                                                    \
    OperandSize_32, OperandMode_ModRM_TestRegister                                                                     \
  }
#define Dd                                                                                                             \
  {                                                                                                                    \
    OperandSize_32, OperandMode_ModRM_DebugRegister                                                                    \
  }
#define Rd                                                                                                             \
  {                                                                                                                    \
    OperandSize_32, OperandMode_ModRM_RM                                                                               \
  }
#define Xb                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_RegisterIndirect, Reg32_ESI                                                             \
  }
#define Xv                                                                                                             \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_RegisterIndirect, Reg32_ESI                                                         \
  }
#define Yb                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_RegisterIndirect, Reg32_EDI                                                             \
  }
#define Yv                                                                                                             \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_RegisterIndirect, Reg32_EDI                                                         \
  }
#define AL                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Register, Reg8_AL                                                                       \
  }
#define AH                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Register, Reg8_AH                                                                       \
  }
#define CL                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Register, Reg8_CL                                                                       \
  }
#define CH                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Register, Reg8_CH                                                                       \
  }
#define DL                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Register, Reg8_DL                                                                       \
  }
#define DH                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Register, Reg8_DH                                                                       \
  }
#define BL                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Register, Reg8_BL                                                                       \
  }
#define BH                                                                                                             \
  {                                                                                                                    \
    OperandSize_8, OperandMode_Register, Reg8_BH                                                                       \
  }
#define AX                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Register, Reg16_AX                                                                     \
  }
#define CX                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Register, Reg16_CX                                                                     \
  }
#define DX                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Register, Reg16_DX                                                                     \
  }
#define BX                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Register, Reg16_BX                                                                     \
  }
#define SP                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Register, Reg16_SP                                                                     \
  }
#define BP                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Register, Reg16_BP                                                                     \
  }
#define SI                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Register, Reg16_SI                                                                     \
  }
#define DI                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_Register, Reg16_DI                                                                     \
  }
#define EAX                                                                                                            \
  {                                                                                                                    \
    OperandSize_32, OperandMode_Register, Reg32_EAX                                                                    \
  }
#define ECX                                                                                                            \
  {                                                                                                                    \
    OperandSize_32, OperandMode_Register, Reg32_ECX                                                                    \
  }
#define EDX                                                                                                            \
  {                                                                                                                    \
    OperandSize_32, OperandMode_Register, Reg32_EDX                                                                    \
  }
#define EBX                                                                                                            \
  {                                                                                                                    \
    OperandSize_32, OperandMode_Register, Reg32_EBX                                                                    \
  }
#define ESP                                                                                                            \
  {                                                                                                                    \
    OperandSize_32, OperandMode_Register, Reg32_ESP                                                                    \
  }
#define EBP                                                                                                            \
  {                                                                                                                    \
    OperandSize_32, OperandMode_Register, Reg32_EBP                                                                    \
  }
#define ESI                                                                                                            \
  {                                                                                                                    \
    OperandSize_32, OperandMode_Register, Reg32_ESI                                                                    \
  }
#define EDI                                                                                                            \
  {                                                                                                                    \
    OperandSize_32, OperandMode_Register, Reg32_EDI                                                                    \
  }
#define eAX                                                                                                            \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Register, Reg32_EAX                                                                 \
  }
#define eCX                                                                                                            \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Register, Reg32_ECX                                                                 \
  }
#define eDX                                                                                                            \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Register, Reg32_EDX                                                                 \
  }
#define eBX                                                                                                            \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Register, Reg32_EBX                                                                 \
  }
#define eSP                                                                                                            \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Register, Reg32_ESP                                                                 \
  }
#define eBP                                                                                                            \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Register, Reg32_EBP                                                                 \
  }
#define eSI                                                                                                            \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Register, Reg32_ESI                                                                 \
  }
#define eDI                                                                                                            \
  {                                                                                                                    \
    OperandSize_Count, OperandMode_Register, Reg32_EDI                                                                 \
  }
#define CS                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_SegmentRegister, Segment_CS                                                            \
  }
#define DS                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_SegmentRegister, Segment_DS                                                            \
  }
#define ES                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_SegmentRegister, Segment_ES                                                            \
  }
#define SS                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_SegmentRegister, Segment_SS                                                            \
  }
#define FS                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_SegmentRegister, Segment_FS                                                            \
  }
#define GS                                                                                                             \
  {                                                                                                                    \
    OperandSize_16, OperandMode_SegmentRegister, Segment_GS                                                            \
  }

#define ST(n)                                                                                                          \
  {                                                                                                                    \
    OperandSize_80, OperandMode_FPRegister, (n)                                                                        \
  }

extern const InstructionTableEntry instruction_table[256];
extern const InstructionTableEntry extension_table_grp1[8];
extern const InstructionTableEntry extension_table_grp2[8];
extern const InstructionTableEntry extension_table_grp3a[8];
extern const InstructionTableEntry extension_table_grp3b[8];
extern const InstructionTableEntry extension_table_grp4[8];
extern const InstructionTableEntry extension_table_grp5[8];
extern const InstructionTableEntry extension_table_0f[256];
extern const InstructionTableEntry extension_table_grp6[8];
extern const InstructionTableEntry extension_table_grp7[8];
extern const InstructionTableEntry extension_table_grp8[8];
extern const InstructionTableEntry extension_table_x87_D8[8 + 64];
extern const InstructionTableEntry extension_table_x87_D9[8 + 64];
extern const InstructionTableEntry extension_table_x87_DA[8 + 64];
extern const InstructionTableEntry extension_table_x87_DB[8 + 64];
extern const InstructionTableEntry extension_table_x87_DC[8 + 64];
extern const InstructionTableEntry extension_table_x87_DD[8 + 64];
extern const InstructionTableEntry extension_table_x87_DE[8 + 64];
extern const InstructionTableEntry extension_table_x87_DF[8 + 64];

const InstructionTableEntry instruction_table[256] = {
  /* 0x00 */ {Operation_ADD, {Eb, Gb}},
  /* 0x01 */ {Operation_ADD, {Ev, Gv}},
  /* 0x02 */ {Operation_ADD, {Gb, Eb}},
  /* 0x03 */ {Operation_ADD, {Gv, Ev}},
  /* 0x04 */ {Operation_ADD, {AL, Ib}},
  /* 0x05 */ {Operation_ADD, {eAX, Iv}},
  /* 0x06 */ {Operation_PUSH, {ES}},
  /* 0x07 */ {Operation_POP, {ES}},
  /* 0x08 */ {Operation_OR, {Eb, Gb}},
  /* 0x09 */ {Operation_OR, {Ev, Gv}},
  /* 0x0A */ {Operation_OR, {Gb, Eb}},
  /* 0x0B */ {Operation_OR, {Gv, Ev}},
  /* 0x0C */ {Operation_OR, {AL, Ib}},
  /* 0x0D */ {Operation_OR, {eAX, Iv}},
  /* 0x0E */ {Operation_PUSH, {CS}},
  /* 0x0F */ {Operation_Extension, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_0f}, // 80286+
  /* 0x10 */ {Operation_ADC, {Eb, Gb}},
  /* 0x11 */ {Operation_ADC, {Ev, Gv}},
  /* 0x12 */ {Operation_ADC, {Gb, Eb}},
  /* 0x13 */ {Operation_ADC, {Gv, Ev}},
  /* 0x14 */ {Operation_ADC, {AL, Ib}},
  /* 0x15 */ {Operation_ADC, {eAX, Iv}},
  /* 0x16 */ {Operation_PUSH, {SS}},
  /* 0x17 */ {Operation_POP, {SS}},
  /* 0x18 */ {Operation_SBB, {Eb, Gb}},
  /* 0x19 */ {Operation_SBB, {Ev, Gv}},
  /* 0x1A */ {Operation_SBB, {Gb, Eb}},
  /* 0x1B */ {Operation_SBB, {Gv, Ev}},
  /* 0x1C */ {Operation_SBB, {AL, Ib}},
  /* 0x1D */ {Operation_SBB, {eAX, Iv}},
  /* 0x1E */ {Operation_PUSH, {DS}},
  /* 0x1F */ {Operation_POP, {DS}},
  /* 0x20 */ {Operation_AND, {Eb, Gb}},
  /* 0x21 */ {Operation_AND, {Ev, Gv}},
  /* 0x22 */ {Operation_AND, {Gb, Eb}},
  /* 0x23 */ {Operation_AND, {Gv, Ev}},
  /* 0x24 */ {Operation_AND, {AL, Ib}},
  /* 0x25 */ {Operation_AND, {eAX, Iv}},
  /* 0x26 */ {Operation_Segment_Prefix, {}, JumpCondition_Always, DefaultSegment_ES},
  /* 0x27 */ {Operation_DAA},
  /* 0x28 */ {Operation_SUB, {Eb, Gb}},
  /* 0x29 */ {Operation_SUB, {Ev, Gv}},
  /* 0x2A */ {Operation_SUB, {Gb, Eb}},
  /* 0x2B */ {Operation_SUB, {Gv, Ev}},
  /* 0x2C */ {Operation_SUB, {AL, Ib}},
  /* 0x2D */ {Operation_SUB, {eAX, Iv}},
  /* 0x2E */ {Operation_Segment_Prefix, {}, JumpCondition_Always, DefaultSegment_CS},
  /* 0x2F */ {Operation_DAS},
  /* 0x30 */ {Operation_XOR, {Eb, Gb}},
  /* 0x31 */ {Operation_XOR, {Ev, Gv}},
  /* 0x32 */ {Operation_XOR, {Gb, Eb}},
  /* 0x33 */ {Operation_XOR, {Gv, Ev}},
  /* 0x34 */ {Operation_XOR, {AL, Ib}},
  /* 0x35 */ {Operation_XOR, {eAX, Iv}},
  /* 0x36 */ {Operation_Segment_Prefix, {}, JumpCondition_Always, DefaultSegment_SS},
  /* 0x37 */ {Operation_AAA},
  /* 0x38 */ {Operation_CMP, {Eb, Gb}},
  /* 0x39 */ {Operation_CMP, {Ev, Gv}},
  /* 0x3A */ {Operation_CMP, {Gb, Eb}},
  /* 0x3B */ {Operation_CMP, {Gv, Ev}},
  /* 0x3C */ {Operation_CMP, {AL, Ib}},
  /* 0x3D */ {Operation_CMP, {eAX, Iv}},
  /* 0x3E */ {Operation_Segment_Prefix, {}, JumpCondition_Always, DefaultSegment_DS},
  /* 0x3F */ {Operation_AAS},
  /* 0x40 */ {Operation_INC, {eAX}},
  /* 0x41 */ {Operation_INC, {eCX}},
  /* 0x42 */ {Operation_INC, {eDX}},
  /* 0x43 */ {Operation_INC, {eBX}},
  /* 0x44 */ {Operation_INC, {eSP}},
  /* 0x45 */ {Operation_INC, {eBP}},
  /* 0x46 */ {Operation_INC, {eSI}},
  /* 0x47 */ {Operation_INC, {eDI}},
  /* 0x48 */ {Operation_DEC, {eAX}},
  /* 0x49 */ {Operation_DEC, {eCX}},
  /* 0x4A */ {Operation_DEC, {eDX}},
  /* 0x4B */ {Operation_DEC, {eBX}},
  /* 0x4C */ {Operation_DEC, {eSP}},
  /* 0x4D */ {Operation_DEC, {eBP}},
  /* 0x4E */ {Operation_DEC, {eSI}},
  /* 0x4F */ {Operation_DEC, {eDI}},
  /* 0x50 */ {Operation_PUSH, {eAX}},
  /* 0x51 */ {Operation_PUSH, {eCX}},
  /* 0x52 */ {Operation_PUSH, {eDX}},
  /* 0x53 */ {Operation_PUSH, {eBX}},
  /* 0x54 */ {Operation_PUSH, {eSP}},
  /* 0x55 */ {Operation_PUSH, {eBP}},
  /* 0x56 */ {Operation_PUSH, {eSI}},
  /* 0x57 */ {Operation_PUSH, {eDI}},
  /* 0x58 */ {Operation_POP, {eAX}},
  /* 0x59 */ {Operation_POP, {eCX}},
  /* 0x5A */ {Operation_POP, {eDX}},
  /* 0x5B */ {Operation_POP, {eBX}},
  /* 0x5C */ {Operation_POP, {eSP}},
  /* 0x5D */ {Operation_POP, {eBP}},
  /* 0x5E */ {Operation_POP, {eSI}},
  /* 0x5F */ {Operation_POP, {eDI}},
  /* 0x60 */ {Operation_PUSHA},           // 80286+
  /* 0x61 */ {Operation_POPA},            // 80286+
  /* 0x62 */ {Operation_BOUND, {Gv, Ma}}, // 80286+
  /* 0x63 */ {Operation_ARPL, {Ew, Gw}},
  /* 0x64 */ {Operation_Segment_Prefix, {}, JumpCondition_Always, DefaultSegment_FS}, // 80386+
  /* 0x65 */ {Operation_Segment_Prefix, {}, JumpCondition_Always, DefaultSegment_GS}, // 80386+
  /* 0x66 */ {Operation_OperandSize_Prefix},                                          // 80386+
  /* 0x67 */ {Operation_AddressSize_Prefix},                                          // 80386+
  /* 0x68 */ {Operation_PUSH, {Iv}},                                                  // 80286+
  /* 0x69 */ {Operation_IMUL, {Gv, Ev, Iv}},                                          // 80286+
  /* 0x6A */ {Operation_PUSH, {Ib}},                                                  // 80286+
  /* 0x6B */ {Operation_IMUL, {Gv, Ev, Ib}},                                          // 80286+
  /* 0x6C */ {Operation_INS, {Yb, DX}},
  /* 0x6D */ {Operation_INS, {Yv, DX}},
  /* 0x6E */ {Operation_OUTS, {DX, Xb}},
  /* 0x6F */ {Operation_OUTS, {DX, Yv}},
  /* 0x70 */ {Operation_Jcc, {Jb}, JumpCondition_Overflow},
  /* 0x71 */ {Operation_Jcc, {Jb}, JumpCondition_NotOverflow},
  /* 0x72 */ {Operation_Jcc, {Jb}, JumpCondition_Below},
  /* 0x73 */ {Operation_Jcc, {Jb}, JumpCondition_AboveOrEqual},
  /* 0x74 */ {Operation_Jcc, {Jb}, JumpCondition_Equal},
  /* 0x75 */ {Operation_Jcc, {Jb}, JumpCondition_NotEqual},
  /* 0x76 */ {Operation_Jcc, {Jb}, JumpCondition_BelowOrEqual},
  /* 0x77 */ {Operation_Jcc, {Jb}, JumpCondition_Above},
  /* 0x78 */ {Operation_Jcc, {Jb}, JumpCondition_Sign},
  /* 0x79 */ {Operation_Jcc, {Jb}, JumpCondition_NotSign},
  /* 0x7A */ {Operation_Jcc, {Jb}, JumpCondition_Parity},
  /* 0x7B */ {Operation_Jcc, {Jb}, JumpCondition_NotParity},
  /* 0x7C */ {Operation_Jcc, {Jb}, JumpCondition_Less},
  /* 0x7D */ {Operation_Jcc, {Jb}, JumpCondition_GreaterOrEqual},
  /* 0x7E */ {Operation_Jcc, {Jb}, JumpCondition_LessOrEqual},
  /* 0x7F */ {Operation_Jcc, {Jb}, JumpCondition_Greater},
  /* 0x80 */ {Operation_Extension_ModRM_Reg, {Eb, Ib}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp1},
  /* 0x81 */ {Operation_Extension_ModRM_Reg, {Ev, Iv}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp1},
  /* 0x82 */ {Operation_Extension_ModRM_Reg, {Eb, Ib}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp1},
  /* 0x83 */ {Operation_Extension_ModRM_Reg, {Ev, Ib}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp1},
  /* 0x84 */ {Operation_TEST, {Gb, Eb}},
  /* 0x85 */ {Operation_TEST, {Gv, Ev}},
  /* 0x86 */ {Operation_XCHG, {Eb, Gb}},
  /* 0x87 */ {Operation_XCHG, {Ev, Gv}},
  /* 0x88 */ {Operation_MOV, {Eb, Gb}},
  /* 0x89 */ {Operation_MOV, {Ev, Gv}},
  /* 0x8A */ {Operation_MOV, {Gb, Eb}},
  /* 0x8B */ {Operation_MOV, {Gv, Ev}},
  /* 0x8C */ {Operation_MOV_Sreg, {Ew, Sw}},
  /* 0x8D */ {Operation_LEA, {Gv, M}},
  /* 0x8E */ {Operation_MOV_Sreg, {Sw, Ew}},
  /* 0x8F */ {Operation_POP, {Ev}},
  /* 0x90 */ {Operation_NOP},
  /* 0x91 */ {Operation_XCHG, {eCX, eAX}},
  /* 0x92 */ {Operation_XCHG, {eDX, eAX}},
  /* 0x93 */ {Operation_XCHG, {eBX, eAX}},
  /* 0x94 */ {Operation_XCHG, {eSP, eAX}},
  /* 0x95 */ {Operation_XCHG, {eBP, eAX}},
  /* 0x96 */ {Operation_XCHG, {eSI, eAX}},
  /* 0x97 */ {Operation_XCHG, {eDI, eAX}},
  /* 0x98 */ {Operation_CBW},
  /* 0x99 */ {Operation_CWD},
  /* 0x9A */ {Operation_CALL_Far, {Ap}},
  /* 0x9B */ {Operation_WAIT},
  /* 0x9C */ {Operation_PUSHF},
  /* 0x9D */ {Operation_POPF},
  /* 0x9E */ {Operation_SAHF},
  /* 0x9F */ {Operation_LAHF},
  /* 0xA0 */ {Operation_MOV, {AL, Ob}},
  /* 0xA1 */ {Operation_MOV, {eAX, Ov}},
  /* 0xA2 */ {Operation_MOV, {Ob, AL}},
  /* 0xA3 */ {Operation_MOV, {Ov, eAX}},
  /* 0xA4 */ {Operation_MOVS, {Yb, Xb}},
  /* 0xA5 */ {Operation_MOVS, {Yv, Xv}},
  /* 0xA6 */ {Operation_CMPS, {Xb, Yb}},
  /* 0xA7 */ {Operation_CMPS, {Xv, Yv}},
  /* 0xA8 */ {Operation_TEST, {AL, Ib}},
  /* 0xA9 */ {Operation_TEST, {eAX, Iv}},
  /* 0xAA */ {Operation_STOS, {Yb, AL}},
  /* 0xAB */ {Operation_STOS, {Yv, eAX}},
  /* 0xAC */ {Operation_LODS, {AL, Xb}},
  /* 0xAD */ {Operation_LODS, {eAX, Xv}},
  /* 0xAE */ {Operation_SCAS, {AL, Xb}},
  /* 0xAF */ {Operation_SCAS, {eAX, Xv}},
  /* 0xB0 */ {Operation_MOV, {AL, Ib}},
  /* 0xB1 */ {Operation_MOV, {CL, Ib}},
  /* 0xB2 */ {Operation_MOV, {DL, Ib}},
  /* 0xB3 */ {Operation_MOV, {BL, Ib}},
  /* 0xB4 */ {Operation_MOV, {AH, Ib}},
  /* 0xB5 */ {Operation_MOV, {CH, Ib}},
  /* 0xB6 */ {Operation_MOV, {DH, Ib}},
  /* 0xB7 */ {Operation_MOV, {BH, Ib}},
  /* 0xB8 */ {Operation_MOV, {eAX, Iv}},
  /* 0xB9 */ {Operation_MOV, {eCX, Iv}},
  /* 0xBA */ {Operation_MOV, {eDX, Iv}},
  /* 0xBB */ {Operation_MOV, {eBX, Iv}},
  /* 0xBC */ {Operation_MOV, {eSP, Iv}},
  /* 0xBD */ {Operation_MOV, {eBP, Iv}},
  /* 0xBE */ {Operation_MOV, {eSI, Iv}},
  /* 0xBF */ {Operation_MOV, {eDI, Iv}},
  /* 0xC0 */
  {Operation_Extension_ModRM_Reg, {Eb, Ib}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp2}, // 80286+
                                                                                                            /* 0xC1 */
  {Operation_Extension_ModRM_Reg, {Ev, Ib}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp2}, // 80286+
  /* 0xC2 */ {Operation_RET_Near, {Iw}},
  /* 0xC3 */ {Operation_RET_Near},
  /* 0xC4 */ {Operation_LES, {Gv, Mp}},
  /* 0xC5 */ {Operation_LDS, {Gv, Mp}},
  /* 0xC6 */ {Operation_MOV, {Eb, Ib}},
  /* 0xC7 */ {Operation_MOV, {Ev, Iv}},
  /* 0xC8 */ {Operation_ENTER, {Iw, Ib}},
  /* 0xC9 */ {Operation_LEAVE},
  /* 0xCA */ {Operation_RET_Far, {Iw}},
  /* 0xCB */ {Operation_RET_Far},
  /* 0xCC */ {Operation_INT, {Cb(3)}},
  /* 0xCD */ {Operation_INT, {Ib}},
  /* 0xCE */ {Operation_INTO},
  /* 0xCF */ {Operation_IRET},
  /* 0xD0 */
  {Operation_Extension_ModRM_Reg, {Eb, Cb(1)}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp2},
  /* 0xD1 */
  {Operation_Extension_ModRM_Reg, {Ev, Cv(1)}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp2},
  /* 0xD2 */ {Operation_Extension_ModRM_Reg, {Eb, CL}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp2},
  /* 0xD3 */ {Operation_Extension_ModRM_Reg, {Ev, CL}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp2},
  /* 0xD4 */ {Operation_AAM, {Ib}},
  /* 0xD5 */ {Operation_AAD, {Ib}},
  /* 0xD6 */ {Operation_Invalid},
  /* 0xD7 */ {Operation_XLAT},
  /* 0xD8 */ {Operation_Extension_ModRM_X87, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_x87_D8},
  /* 0xD9 */ {Operation_Extension_ModRM_X87, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_x87_D9},
  /* 0xDA */ {Operation_Extension_ModRM_X87, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_x87_DA},
  /* 0xDB */ {Operation_Extension_ModRM_X87, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_x87_DB},
  /* 0xDC */ {Operation_Extension_ModRM_X87, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_x87_DC},
  /* 0xDD */ {Operation_Extension_ModRM_X87, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_x87_DD},
  /* 0xDE */ {Operation_Extension_ModRM_X87, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_x87_DE},
  /* 0xDF */ {Operation_Extension_ModRM_X87, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_x87_DF},
  /* 0xE0 */ {Operation_LOOP, {Jb}, JumpCondition_NotEqual},
  /* 0xE1 */ {Operation_LOOP, {Jb}, JumpCondition_Equal},
  /* 0xE2 */ {Operation_LOOP, {Jb}},
  /* 0xE3 */ {Operation_Jcc, {Jb}, JumpCondition_CXZero},
  /* 0xE4 */ {Operation_IN, {AL, Ib}},
  /* 0xE5 */ {Operation_IN, {eAX, Ib}},
  /* 0xE6 */ {Operation_OUT, {Ib, AL}},
  /* 0xE7 */ {Operation_OUT, {Ib, eAX}},
  /* 0xE8 */ {Operation_CALL_Near, {Jv}},
  /* 0xE9 */ {Operation_JMP_Near, {Jv}},
  /* 0xEA */ {Operation_JMP_Far, {Ap}},
  /* 0xEB */ {Operation_JMP_Near, {Jb}},
  /* 0xEC */ {Operation_IN, {AL, DX}},
  /* 0xED */ {Operation_IN, {eAX, DX}},
  /* 0xEE */ {Operation_OUT, {DX, AL}},
  /* 0xEF */ {Operation_OUT, {DX, eAX}},
  /* 0xF0 */ {Operation_Lock_Prefix},
  /* 0xF1 */ {Operation_Invalid},
  /* 0xF2 */ {Operation_RepNE_Prefix},
  /* 0xF3 */ {Operation_Rep_Prefix},
  /* 0xF4 */ {Operation_HLT},
  /* 0xF5 */ {Operation_CMC},
  /* 0xF6 */ {Operation_Extension_ModRM_Reg, {Eb}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp3a},
  /* 0xF7 */ {Operation_Extension_ModRM_Reg, {Ev}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp3b},
  /* 0xF8 */ {Operation_CLC},
  /* 0xF9 */ {Operation_STC},
  /* 0xFA */ {Operation_CLI},
  /* 0xFB */ {Operation_STI},
  /* 0xFC */ {Operation_CLD},
  /* 0xFD */ {Operation_STD},
  /* 0xFE */ {Operation_Extension_ModRM_Reg, {Eb}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp4},
  /* 0xFF */ {Operation_Extension_ModRM_Reg, {Ev}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp5}};

const InstructionTableEntry extension_table_grp1[8] = {
  /* 0x00 */ {Operation_ADD},
  /* 0x01 */ {Operation_OR},
  /* 0x02 */ {Operation_ADC},
  /* 0x03 */ {Operation_SBB},
  /* 0x04 */ {Operation_AND},
  /* 0x05 */ {Operation_SUB},
  /* 0x06 */ {Operation_XOR},
  /* 0x07 */ {Operation_CMP}};

const InstructionTableEntry extension_table_grp2[8] = {
  /* 0x00 */ {Operation_ROL},
  /* 0x01 */ {Operation_ROR},
  /* 0x02 */ {Operation_RCL},
  /* 0x03 */ {Operation_RCR},
  /* 0x04 */ {Operation_SHL},
  /* 0x05 */ {Operation_SHR},
  /* 0x06 */ {Operation_Invalid},
  /* 0x07 */ {Operation_SAR}};

const InstructionTableEntry extension_table_grp3a[8] = {
  /* 0x00 */ {Operation_TEST, {Eb, Ib}},
  /* 0x01 */ {Operation_Invalid},
  /* 0x02 */ {Operation_NOT},
  /* 0x03 */ {Operation_NEG},
  /* 0x04 */ {Operation_MUL},
  /* 0x05 */ {Operation_IMUL},
  /* 0x06 */ {Operation_DIV},
  /* 0x07 */ {Operation_IDIV}};

const InstructionTableEntry extension_table_grp3b[8] = {
  /* 0x00 */ {Operation_TEST, {Ev, Iv}},
  /* 0x01 */ {Operation_Invalid},
  /* 0x02 */ {Operation_NOT},
  /* 0x03 */ {Operation_NEG},
  /* 0x04 */ {Operation_MUL},
  /* 0x05 */ {Operation_IMUL},
  /* 0x06 */ {Operation_DIV},
  /* 0x07 */ {Operation_IDIV}};

const InstructionTableEntry extension_table_grp4[8] = {
  /* 0x00 */ {Operation_INC},
  /* 0x01 */ {Operation_DEC},
  /* 0x02 */ {Operation_Invalid},
  /* 0x03 */ {Operation_Invalid},
  /* 0x04 */ {Operation_Invalid},
  /* 0x05 */ {Operation_Invalid},
  /* 0x06 */ {Operation_Invalid},
  /* 0x07 */ {Operation_Invalid}};

const InstructionTableEntry extension_table_grp5[8] = {
  /* 0x00 */ {Operation_INC},
  /* 0x01 */ {Operation_DEC},
  /* 0x02 */ {Operation_CALL_Near},
  /* 0x03 */ {Operation_CALL_Far, {Mp}},
  /* 0x04 */ {Operation_JMP_Near},
  /* 0x05 */ {Operation_JMP_Far, {Mp}},
  /* 0x06 */ {Operation_PUSH},
  /* 0x07 */ {Operation_Invalid}};

// 80286+
const InstructionTableEntry extension_table_0f[256] = {
  /* 0x00 */ {Operation_Extension_ModRM_Reg, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp6},
  /* 0x01 */ {Operation_Extension_ModRM_Reg, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp7},
  /* 0x02 */ {Operation_LAR, {Gv, Ew}},
  /* 0x03 */ {Operation_LSL, {Gv, Ew}},
  /* 0x04 */ {Operation_Invalid},
  /* 0x05 */ {Operation_LOADALL_286},
  /* 0x06 */ {Operation_CLTS},
  /* 0x07 */ {Operation_Invalid},
  /* 0x08 */ {Operation_Invalid},
  /* 0x09 */ {Operation_WBINVD}, // 80486+
  /* 0x0A */ {Operation_Invalid},
  /* 0x0B */ {Operation_Invalid},
  /* 0x0C */ {Operation_Invalid},
  /* 0x0D */ {Operation_Invalid},
  /* 0x1E */ {Operation_Invalid},
  /* 0x1F */ {Operation_Invalid},
  /* 0x10 */ {Operation_Invalid},
  /* 0x11 */ {Operation_Invalid},
  /* 0x12 */ {Operation_Invalid},
  /* 0x13 */ {Operation_Invalid},
  /* 0x14 */ {Operation_Invalid},
  /* 0x15 */ {Operation_Invalid},
  /* 0x16 */ {Operation_Invalid},
  /* 0x17 */ {Operation_Invalid},
  /* 0x18 */ {Operation_Invalid},
  /* 0x19 */ {Operation_Invalid},
  /* 0x1A */ {Operation_Invalid},
  /* 0x1B */ {Operation_Invalid},
  /* 0x1C */ {Operation_Invalid},
  /* 0x1D */ {Operation_Invalid},
  /* 0x1E */ {Operation_Invalid},
  /* 0x1F */ {Operation_Invalid},
  /* 0x20 */ {Operation_MOV_CR, {Rd, Cd}},
  /* 0x21 */ {Operation_MOV_DR, {Rd, Dd}},
  /* 0x22 */ {Operation_MOV_CR, {Cd, Rd}},
  /* 0x23 */ {Operation_MOV_DR, {Dd, Rd}},
  /* 0x24 */ {Operation_MOV_TR, {Rd, Td}},
  /* 0x25 */ {Operation_Invalid},
  /* 0x26 */ {Operation_MOV_TR, {Td, Rd}},
  /* 0x27 */ {Operation_Invalid},
  /* 0x28 */ {Operation_Invalid},
  /* 0x29 */ {Operation_Invalid},
  /* 0x2A */ {Operation_Invalid},
  /* 0x2B */ {Operation_Invalid},
  /* 0x2C */ {Operation_Invalid},
  /* 0x2D */ {Operation_Invalid},
  /* 0x2E */ {Operation_Invalid},
  /* 0x2F */ {Operation_Invalid},
  /* 0x30 */ {Operation_Invalid},
  /* 0x31 */ {Operation_RDTSC},
  /* 0x32 */ {Operation_Invalid},
  /* 0x33 */ {Operation_Invalid},
  /* 0x34 */ {Operation_Invalid},
  /* 0x35 */ {Operation_Invalid},
  /* 0x36 */ {Operation_Invalid},
  /* 0x37 */ {Operation_Invalid},
  /* 0x38 */ {Operation_Invalid},
  /* 0x39 */ {Operation_Invalid},
  /* 0x3A */ {Operation_Invalid},
  /* 0x3B */ {Operation_Invalid},
  /* 0x3C */ {Operation_Invalid},
  /* 0x3D */ {Operation_Invalid},
  /* 0x3E */ {Operation_Invalid},
  /* 0x3F */ {Operation_Invalid},
  /* 0x40 */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_Overflow},       // 80486+
  /* 0x41 */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_NotOverflow},    // 80486+
  /* 0x42 */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_Below},          // 80486+
  /* 0x43 */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_AboveOrEqual},   // 80486+
  /* 0x44 */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_Equal},          // 80486+
  /* 0x45 */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_NotEqual},       // 80486+
  /* 0x46 */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_BelowOrEqual},   // 80486+
  /* 0x47 */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_Above},          // 80486+
  /* 0x48 */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_Sign},           // 80486+
  /* 0x49 */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_NotSign},        // 80486+
  /* 0x4A */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_Parity},         // 80486+
  /* 0x4B */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_NotParity},      // 80486+
  /* 0x4C */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_Less},           // 80486+
  /* 0x4D */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_GreaterOrEqual}, // 80486+
  /* 0x4E */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_LessOrEqual},    // 80486+
  /* 0x4F */ {Operation_CMOVcc, {Gv, Ev}, JumpCondition_Greater},        // 80486+
  /* 0x50 */ {Operation_Invalid},
  /* 0x51 */ {Operation_Invalid},
  /* 0x52 */ {Operation_Invalid},
  /* 0x53 */ {Operation_Invalid},
  /* 0x54 */ {Operation_Invalid},
  /* 0x55 */ {Operation_Invalid},
  /* 0x56 */ {Operation_Invalid},
  /* 0x57 */ {Operation_Invalid},
  /* 0x58 */ {Operation_Invalid},
  /* 0x59 */ {Operation_Invalid},
  /* 0x5A */ {Operation_Invalid},
  /* 0x5B */ {Operation_Invalid},
  /* 0x5C */ {Operation_Invalid},
  /* 0x5D */ {Operation_Invalid},
  /* 0x5E */ {Operation_Invalid},
  /* 0x5F */ {Operation_Invalid},
  /* 0x60 */ {Operation_Invalid},
  /* 0x61 */ {Operation_Invalid},
  /* 0x62 */ {Operation_Invalid},
  /* 0x63 */ {Operation_Invalid},
  /* 0x64 */ {Operation_Invalid},
  /* 0x65 */ {Operation_Invalid},
  /* 0x66 */ {Operation_Invalid},
  /* 0x67 */ {Operation_Invalid},
  /* 0x68 */ {Operation_Invalid},
  /* 0x69 */ {Operation_Invalid},
  /* 0x6A */ {Operation_Invalid},
  /* 0x6B */ {Operation_Invalid},
  /* 0x6C */ {Operation_Invalid},
  /* 0x6D */ {Operation_Invalid},
  /* 0x6E */ {Operation_Invalid},
  /* 0x6F */ {Operation_Invalid},
  /* 0x70 */ {Operation_Invalid},
  /* 0x71 */ {Operation_Invalid},
  /* 0x72 */ {Operation_Invalid},
  /* 0x73 */ {Operation_Invalid},
  /* 0x74 */ {Operation_Invalid},
  /* 0x75 */ {Operation_Invalid},
  /* 0x76 */ {Operation_Invalid},
  /* 0x77 */ {Operation_Invalid},
  /* 0x78 */ {Operation_Invalid},
  /* 0x79 */ {Operation_Invalid},
  /* 0x7A */ {Operation_Invalid},
  /* 0x7B */ {Operation_Invalid},
  /* 0x7C */ {Operation_Invalid},
  /* 0x7D */ {Operation_Invalid},
  /* 0x7E */ {Operation_Invalid},
  /* 0x7F */ {Operation_Invalid},
  /* 0x80 */ {Operation_Jcc, {Jv}, JumpCondition_Overflow},
  /* 0x81 */ {Operation_Jcc, {Jv}, JumpCondition_NotOverflow},
  /* 0x82 */ {Operation_Jcc, {Jv}, JumpCondition_Below},
  /* 0x83 */ {Operation_Jcc, {Jv}, JumpCondition_AboveOrEqual},
  /* 0x84 */ {Operation_Jcc, {Jv}, JumpCondition_Equal},
  /* 0x85 */ {Operation_Jcc, {Jv}, JumpCondition_NotEqual},
  /* 0x86 */ {Operation_Jcc, {Jv}, JumpCondition_BelowOrEqual},
  /* 0x87 */ {Operation_Jcc, {Jv}, JumpCondition_Above},
  /* 0x88 */ {Operation_Jcc, {Jv}, JumpCondition_Sign},
  /* 0x89 */ {Operation_Jcc, {Jv}, JumpCondition_NotSign},
  /* 0x8A */ {Operation_Jcc, {Jv}, JumpCondition_Parity},
  /* 0x8B */ {Operation_Jcc, {Jv}, JumpCondition_NotParity},
  /* 0x8C */ {Operation_Jcc, {Jv}, JumpCondition_Less},
  /* 0x8D */ {Operation_Jcc, {Jv}, JumpCondition_GreaterOrEqual},
  /* 0x8E */ {Operation_Jcc, {Jv}, JumpCondition_LessOrEqual},
  /* 0x8F */ {Operation_Jcc, {Jv}, JumpCondition_Greater},
  /* 0x90 */ {Operation_SETcc, {Eb}, JumpCondition_Overflow},
  /* 0x91 */ {Operation_SETcc, {Eb}, JumpCondition_NotOverflow},
  /* 0x92 */ {Operation_SETcc, {Eb}, JumpCondition_Below},
  /* 0x93 */ {Operation_SETcc, {Eb}, JumpCondition_AboveOrEqual},
  /* 0x94 */ {Operation_SETcc, {Eb}, JumpCondition_Equal},
  /* 0x95 */ {Operation_SETcc, {Eb}, JumpCondition_NotEqual},
  /* 0x96 */ {Operation_SETcc, {Eb}, JumpCondition_BelowOrEqual},
  /* 0x97 */ {Operation_SETcc, {Eb}, JumpCondition_Above},
  /* 0x98 */ {Operation_SETcc, {Eb}, JumpCondition_Sign},
  /* 0x99 */ {Operation_SETcc, {Eb}, JumpCondition_NotSign},
  /* 0x9A */ {Operation_SETcc, {Eb}, JumpCondition_Parity},
  /* 0x9B */ {Operation_SETcc, {Eb}, JumpCondition_NotParity},
  /* 0x9C */ {Operation_SETcc, {Eb}, JumpCondition_Less},
  /* 0x9D */ {Operation_SETcc, {Eb}, JumpCondition_GreaterOrEqual},
  /* 0x9E */ {Operation_SETcc, {Eb}, JumpCondition_LessOrEqual},
  /* 0x9F */ {Operation_SETcc, {Eb}, JumpCondition_Greater},
  /* 0xA0 */ {Operation_PUSH, {FS}},
  /* 0xA1 */ {Operation_POP, {FS}},
  /* 0xA2 */ {Operation_Invalid},
  /* 0xA3 */ {Operation_BT, {Ev, Gv}},
  /* 0xA4 */ {Operation_SHLD, {Ev, Gv, Ib}},
  /* 0xA5 */ {Operation_SHLD, {Ev, Gv, CL}},
  /* 0xA6 */ {Operation_Invalid},
  /* 0xA7 */ {Operation_Invalid},
  /* 0xA8 */ {Operation_PUSH, {GS}},
  /* 0xA9 */ {Operation_POP, {GS}},
  /* 0xAA */ {Operation_Invalid},
  /* 0xAB */ {Operation_BTS, {Ev, Gv}},
  /* 0xAC */ {Operation_SHRD, {Ev, Gv, Ib}},
  /* 0xAD */ {Operation_SHRD, {Ev, Gv, CL}},
  /* 0xAE */ {Operation_Invalid},
  /* 0xAF */ {Operation_IMUL, {Gv, Ev}},
  /* 0xB0 */ {Operation_CMPXCHG, {Eb, Gb}}, // 80486+
  /* 0xB1 */ {Operation_CMPXCHG, {Ev, Gv}}, // 80486+
  /* 0xB2 */ {Operation_LSS, {Gv, Mp}},
  /* 0xB3 */ {Operation_BTR, {Ev, Gv}},
  /* 0xB4 */ {Operation_LFS, {Gv, Mp}},
  /* 0xB5 */ {Operation_LGS, {Gv, Mp}},
  /* 0xB6 */ {Operation_MOVZX, {Gv, Eb}}, // 80386+
  /* 0xB7 */ {Operation_MOVZX, {Gv, Ew}}, // 80386+
  /* 0xB8 */ {Operation_Invalid},
  /* 0xB9 */ {Operation_Invalid},
  /* 0xBA */ {Operation_Extension_ModRM_Reg, {Ev, Ib}, JumpCondition_Always, DefaultSegment_DS, extension_table_grp8},
  /* 0xBB */ {Operation_BTC, {Ev, Gv}},
  /* 0xBC */ {Operation_BSF, {Gv, Ev}},
  /* 0xBD */ {Operation_BSR, {Gv, Ev}},
  /* 0xBE */ {Operation_MOVSX, {Gv, Eb}}, // 80386+
  /* 0xBF */ {Operation_MOVSX, {Gv, Ew}}, // 80386+
  /* 0xC0 */ {Operation_XADD, {Eb, Gb}},  // 80486+
  /* 0xC1 */ {Operation_XADD, {Ev, Gv}},  // 80486+
  /* 0xC2 */ {Operation_Invalid},
  /* 0xC3 */ {Operation_Invalid},
  /* 0xC4 */ {Operation_Invalid},
  /* 0xC5 */ {Operation_Invalid},
  /* 0xC6 */ {Operation_Invalid},
  /* 0xC7 */ {Operation_Invalid},
  /* 0xC8 */ {Operation_BSWAP, {EAX}},
  /* 0xC9 */ {Operation_BSWAP, {ECX}},
  /* 0xCA */ {Operation_BSWAP, {EDX}},
  /* 0xCB */ {Operation_BSWAP, {EBX}},
  /* 0xCC */ {Operation_BSWAP, {ESP}},
  /* 0xCD */ {Operation_BSWAP, {EBP}},
  /* 0xCE */ {Operation_BSWAP, {ESI}},
  /* 0xCF */ {Operation_BSWAP, {EDI}},
  /* 0xD0 */ {Operation_Invalid},
  /* 0xD1 */ {Operation_Invalid},
  /* 0xD2 */ {Operation_Invalid},
  /* 0xD3 */ {Operation_Invalid},
  /* 0xD4 */ {Operation_Invalid},
  /* 0xD5 */ {Operation_Invalid},
  /* 0xD6 */ {Operation_Invalid},
  /* 0xD7 */ {Operation_Invalid},
  /* 0xD8 */ {Operation_Invalid},
  /* 0xD9 */ {Operation_Invalid},
  /* 0xDA */ {Operation_Invalid},
  /* 0xDB */ {Operation_Invalid},
  /* 0xDC */ {Operation_Invalid},
  /* 0xDD */ {Operation_Invalid},
  /* 0xDE */ {Operation_Invalid},
  /* 0xDF */ {Operation_Invalid},
  /* 0xE0 */ {Operation_Invalid},
  /* 0xE1 */ {Operation_Invalid},
  /* 0xE2 */ {Operation_Invalid},
  /* 0xE3 */ {Operation_Invalid},
  /* 0xE4 */ {Operation_Invalid},
  /* 0xE5 */ {Operation_Invalid},
  /* 0xE6 */ {Operation_Invalid},
  /* 0xE7 */ {Operation_Invalid},
  /* 0xE8 */ {Operation_Invalid},
  /* 0xE9 */ {Operation_Invalid},
  /* 0xEA */ {Operation_Invalid},
  /* 0xEB */ {Operation_Invalid},
  /* 0xEC */ {Operation_Invalid},
  /* 0xED */ {Operation_Invalid},
  /* 0xEE */ {Operation_Invalid},
  /* 0xEF */ {Operation_Invalid},
  /* 0xF0 */ {Operation_Invalid},
  /* 0xF1 */ {Operation_Invalid},
  /* 0xF2 */ {Operation_Invalid},
  /* 0xF3 */ {Operation_Invalid},
  /* 0xF4 */ {Operation_Invalid},
  /* 0xF5 */ {Operation_Invalid},
  /* 0xF6 */ {Operation_Invalid},
  /* 0xF7 */ {Operation_Invalid},
  /* 0xF8 */ {Operation_Invalid},
  /* 0xF9 */ {Operation_Invalid},
  /* 0xFA */ {Operation_Invalid},
  /* 0xFB */ {Operation_Invalid},
  /* 0xFC */ {Operation_Invalid},
  /* 0xFD */ {Operation_Invalid},
  /* 0xFE */ {Operation_Invalid},
  /* 0xFF */ {Operation_Invalid}};

// 80286+
const InstructionTableEntry extension_table_grp6[8] = {
  /* 0x00 */ {Operation_SLDT, {Ew}},
  /* 0x01 */ {Operation_STR, {Ew}},
  /* 0x02 */ {Operation_LLDT, {Ew}},
  /* 0x03 */ {Operation_LTR, {Ew}},
  /* 0x04 */ {Operation_VERR, {Ew}},
  /* 0x05 */ {Operation_VERW, {Ew}},
  /* 0x06 */ {Operation_Invalid},
  /* 0x07 */ {Operation_Invalid}};

// 80286+
const InstructionTableEntry extension_table_grp7[8] = {
  /* 0x00 */ {Operation_SGDT, {Ms}},
  /* 0x01 */ {Operation_SIDT, {Ms}},
  /* 0x02 */ {Operation_LGDT, {Ms}},
  /* 0x03 */ {Operation_LIDT, {Ms}},
  /* 0x04 */ {Operation_SMSW, {Ew}},
  /* 0x05 */ {Operation_Invalid},
  /* 0x06 */ {Operation_LMSW, {Ew}},
  /* 0x07 */ {Operation_INVLPG, {Ev}} // 80486+
};

// 80286+
const InstructionTableEntry extension_table_grp8[8] = {
  /* 0x00 */ {Operation_Invalid},
  /* 0x01 */ {Operation_Invalid},
  /* 0x02 */ {Operation_Invalid},
  /* 0x03 */ {Operation_Invalid},
  /* 0x04 */ {Operation_BT},
  /* 0x05 */ {Operation_BTS},
  /* 0x06 */ {Operation_BTR},
  /* 0x07 */ {Operation_BTC}};

// 8087+
const InstructionTableEntry extension_table_x87_D8[8 + 64] = {
  /* 0x00 */ {Operation_FADD, {ST(0), Md}},
  /* 0x01 */ {Operation_FMUL, {ST(0), Md}},
  /* 0x02 */ {Operation_FCOM, {ST(0), Md}},
  /* 0x03 */ {Operation_FCOMP, {ST(0), Md}},
  /* 0x04 */ {Operation_FSUB, {ST(0), Md}},
  /* 0x05 */ {Operation_FSUBR, {ST(0), Md}},
  /* 0x06 */ {Operation_FDIV, {ST(0), Md}},
  /* 0x07 */ {Operation_FDIVR, {ST(0), Md}},

  /* 0xC0 */ {Operation_FADD, {ST(0), ST(0)}},
  /* 0xC1 */ {Operation_FADD, {ST(0), ST(1)}},
  /* 0xC2 */ {Operation_FADD, {ST(0), ST(2)}},
  /* 0xC3 */ {Operation_FADD, {ST(0), ST(3)}},
  /* 0xC4 */ {Operation_FADD, {ST(0), ST(4)}},
  /* 0xC5 */ {Operation_FADD, {ST(0), ST(5)}},
  /* 0xC6 */ {Operation_FADD, {ST(0), ST(6)}},
  /* 0xC7 */ {Operation_FADD, {ST(0), ST(7)}},
  /* 0xC8 */ {Operation_FMUL, {ST(0), ST(0)}},
  /* 0xC9 */ {Operation_FMUL, {ST(0), ST(1)}},
  /* 0xCA */ {Operation_FMUL, {ST(0), ST(2)}},
  /* 0xCB */ {Operation_FMUL, {ST(0), ST(3)}},
  /* 0xCC */ {Operation_FMUL, {ST(0), ST(4)}},
  /* 0xCD */ {Operation_FMUL, {ST(0), ST(5)}},
  /* 0xCE */ {Operation_FMUL, {ST(0), ST(6)}},
  /* 0xCF */ {Operation_FMUL, {ST(0), ST(7)}},
  /* 0xD0 */ {Operation_FCOM, {ST(0), ST(0)}},
  /* 0xD1 */ {Operation_FCOM, {ST(0), ST(1)}},
  /* 0xD2 */ {Operation_FCOM, {ST(0), ST(2)}},
  /* 0xD3 */ {Operation_FCOM, {ST(0), ST(3)}},
  /* 0xD4 */ {Operation_FCOM, {ST(0), ST(4)}},
  /* 0xD5 */ {Operation_FCOM, {ST(0), ST(5)}},
  /* 0xD6 */ {Operation_FCOM, {ST(0), ST(6)}},
  /* 0xD7 */ {Operation_FCOM, {ST(0), ST(7)}},
  /* 0xD8 */ {Operation_FCOMP, {ST(0), ST(0)}},
  /* 0xD9 */ {Operation_FCOMP, {ST(0), ST(1)}},
  /* 0xDA */ {Operation_FCOMP, {ST(0), ST(2)}},
  /* 0xDB */ {Operation_FCOMP, {ST(0), ST(3)}},
  /* 0xDC */ {Operation_FCOMP, {ST(0), ST(4)}},
  /* 0xDD */ {Operation_FCOMP, {ST(0), ST(5)}},
  /* 0xDE */ {Operation_FCOMP, {ST(0), ST(6)}},
  /* 0xDF */ {Operation_FCOMP, {ST(0), ST(7)}},
  /* 0xE0 */ {Operation_FSUB, {ST(0), ST(0)}},
  /* 0xE1 */ {Operation_FSUB, {ST(0), ST(1)}},
  /* 0xE2 */ {Operation_FSUB, {ST(0), ST(2)}},
  /* 0xE3 */ {Operation_FSUB, {ST(0), ST(3)}},
  /* 0xE4 */ {Operation_FSUB, {ST(0), ST(4)}},
  /* 0xE5 */ {Operation_FSUB, {ST(0), ST(5)}},
  /* 0xE6 */ {Operation_FSUB, {ST(0), ST(6)}},
  /* 0xE7 */ {Operation_FSUB, {ST(0), ST(7)}},
  /* 0xE8 */ {Operation_FSUBR, {ST(0), ST(0)}},
  /* 0xE9 */ {Operation_FSUBR, {ST(0), ST(1)}},
  /* 0xEA */ {Operation_FSUBR, {ST(0), ST(2)}},
  /* 0xEB */ {Operation_FSUBR, {ST(0), ST(3)}},
  /* 0xEC */ {Operation_FSUBR, {ST(0), ST(4)}},
  /* 0xED */ {Operation_FSUBR, {ST(0), ST(5)}},
  /* 0xEE */ {Operation_FSUBR, {ST(0), ST(6)}},
  /* 0xEF */ {Operation_FSUBR, {ST(0), ST(7)}},
  /* 0xF0 */ {Operation_FDIV, {ST(0), ST(0)}},
  /* 0xF1 */ {Operation_FDIV, {ST(0), ST(1)}},
  /* 0xF2 */ {Operation_FDIV, {ST(0), ST(2)}},
  /* 0xF3 */ {Operation_FDIV, {ST(0), ST(3)}},
  /* 0xF4 */ {Operation_FDIV, {ST(0), ST(4)}},
  /* 0xF5 */ {Operation_FDIV, {ST(0), ST(5)}},
  /* 0xF6 */ {Operation_FDIV, {ST(0), ST(6)}},
  /* 0xF7 */ {Operation_FDIV, {ST(0), ST(7)}},
  /* 0xF8 */ {Operation_FDIVR, {ST(0), ST(0)}},
  /* 0xF9 */ {Operation_FDIVR, {ST(0), ST(1)}},
  /* 0xFA */ {Operation_FDIVR, {ST(0), ST(2)}},
  /* 0xFB */ {Operation_FDIVR, {ST(0), ST(3)}},
  /* 0xFC */ {Operation_FDIVR, {ST(0), ST(4)}},
  /* 0xFD */ {Operation_FDIVR, {ST(0), ST(5)}},
  /* 0xFE */ {Operation_FDIVR, {ST(0), ST(6)}},
  /* 0xFF */ {Operation_FDIVR, {ST(0), ST(7)}}};

const InstructionTableEntry extension_table_x87_D9[8 + 64] = {
  /* 0x00 */ {Operation_FLD, {Md}},
  /* 0x01 */ {Operation_Invalid},
  /* 0x02 */ {Operation_FST, {Md}},
  /* 0x03 */ {Operation_FSTP, {Md}},
  /* 0x04 */ {Operation_FLDENV, {M}},
  /* 0x05 */ {Operation_FLDCW, {Mw}},
  /* 0x06 */ {Operation_FNSTENV, {M}},
  /* 0x07 */ {Operation_FNSTCW, {Mw}},

  /* 0xC0 */ {Operation_FLD, {ST(0)}},
  /* 0xC1 */ {Operation_FLD, {ST(1)}},
  /* 0xC2 */ {Operation_FLD, {ST(2)}},
  /* 0xC3 */ {Operation_FLD, {ST(3)}},
  /* 0xC4 */ {Operation_FLD, {ST(4)}},
  /* 0xC5 */ {Operation_FLD, {ST(5)}},
  /* 0xC6 */ {Operation_FLD, {ST(6)}},
  /* 0xC7 */ {Operation_FLD, {ST(7)}},
  /* 0xC8 */ {Operation_FXCH, {ST(0)}},
  /* 0xC9 */ {Operation_FXCH, {ST(1)}},
  /* 0xCA */ {Operation_FXCH, {ST(2)}},
  /* 0xCB */ {Operation_FXCH, {ST(3)}},
  /* 0xCC */ {Operation_FXCH, {ST(4)}},
  /* 0xCD */ {Operation_FXCH, {ST(5)}},
  /* 0xCE */ {Operation_FXCH, {ST(6)}},
  /* 0xCF */ {Operation_FXCH, {ST(7)}},

  /* 0xD0 */ {Operation_FNOP},
  /* 0xD1 */ {Operation_Invalid},
  /* 0xD2 */ {Operation_Invalid},
  /* 0xD3 */ {Operation_Invalid},
  /* 0xD4 */ {Operation_Invalid},
  /* 0xD5 */ {Operation_Invalid},
  /* 0xD6 */ {Operation_Invalid},
  /* 0xD7 */ {Operation_Invalid},
  /* 0xD8 */ {Operation_Invalid},
  /* 0xD9 */ {Operation_Invalid},
  /* 0xDA */ {Operation_Invalid},
  /* 0xDB */ {Operation_Invalid},
  /* 0xDC */ {Operation_Invalid},
  /* 0xDD */ {Operation_Invalid},
  /* 0xDE */ {Operation_Invalid},
  /* 0xDF */ {Operation_Invalid},

  /* 0xE0 */ {Operation_FCHS},
  /* 0xE1 */ {Operation_FABS},
  /* 0xE2 */ {Operation_Invalid},
  /* 0xE3 */ {Operation_Invalid},
  /* 0xE4 */ {Operation_FTST},
  /* 0xE5 */ {Operation_FXAM},
  /* 0xE6 */ {Operation_Invalid},
  /* 0xE7 */ {Operation_Invalid},
  /* 0xE8 */ {Operation_FLD1},
  /* 0xE9 */ {Operation_FLDL2E},
  /* 0xEA */ {Operation_FLDL2T},
  /* 0xEB */ {Operation_FLDPI},
  /* 0xEC */ {Operation_FLDLG2},
  /* 0xED */ {Operation_FLDLN2},
  /* 0xEE */ {Operation_FLDZ},
  /* 0xEF */ {Operation_Invalid},

  /* 0xF0 */ {Operation_F2XM1},
  /* 0xF1 */ {Operation_FYL2X},
  /* 0xF2 */ {Operation_FPTAN},
  /* 0xF3 */ {Operation_FPATAN},
  /* 0xF4 */ {Operation_FXTRACT},
  /* 0xF5 */ {Operation_FPREM1},
  /* 0xF6 */ {Operation_FDECSTP},
  /* 0xF7 */ {Operation_FINCSTP},
  /* 0xF8 */ {Operation_FPREM},
  /* 0xF9 */ {Operation_FYL2XP1},
  /* 0xFA */ {Operation_FSQRT},
  /* 0xFB */ {Operation_FSINCOS},
  /* 0xFC */ {Operation_FRNDINT},
  /* 0xFD */ {Operation_FSCALE},
  /* 0xFE */ {Operation_FSIN},
  /* 0xFF */ {Operation_FCOS}};

const InstructionTableEntry extension_table_x87_DA[8 + 64] = {
  /* 0x00 */ {Operation_FIADD, {ST(0), Md}},
  /* 0x01 */ {Operation_FIMUL, {ST(0), Md}},
  /* 0x02 */ {Operation_FICOM, {ST(0), Md}},
  /* 0x03 */ {Operation_FICOMP, {ST(0), Md}},
  /* 0x04 */ {Operation_FISUB, {ST(0), Md}},
  /* 0x05 */ {Operation_FISUBR, {ST(0), Md}},
  /* 0x06 */ {Operation_FIDIV, {ST(0), Md}},
  /* 0x07 */ {Operation_FIDIVR, {ST(0), Md}},

  /* 0xC0 */ {Operation_Invalid},
  /* 0xC1 */ {Operation_Invalid},
  /* 0xC2 */ {Operation_Invalid},
  /* 0xC3 */ {Operation_Invalid},
  /* 0xC4 */ {Operation_Invalid},
  /* 0xC5 */ {Operation_Invalid},
  /* 0xC6 */ {Operation_Invalid},
  /* 0xC7 */ {Operation_Invalid},
  /* 0xC8 */ {Operation_Invalid},
  /* 0xC9 */ {Operation_Invalid},
  /* 0xCA */ {Operation_Invalid},
  /* 0xCB */ {Operation_Invalid},
  /* 0xCC */ {Operation_Invalid},
  /* 0xCD */ {Operation_Invalid},
  /* 0xCE */ {Operation_Invalid},
  /* 0xCF */ {Operation_Invalid},
  /* 0xD0 */ {Operation_Invalid},
  /* 0xD1 */ {Operation_Invalid},
  /* 0xD2 */ {Operation_Invalid},
  /* 0xD3 */ {Operation_Invalid},
  /* 0xD4 */ {Operation_Invalid},
  /* 0xD5 */ {Operation_Invalid},
  /* 0xD6 */ {Operation_Invalid},
  /* 0xD7 */ {Operation_Invalid},
  /* 0xD8 */ {Operation_Invalid},
  /* 0xD9 */ {Operation_Invalid},
  /* 0xDA */ {Operation_Invalid},
  /* 0xDB */ {Operation_Invalid},
  /* 0xDC */ {Operation_Invalid},
  /* 0xDD */ {Operation_Invalid},
  /* 0xDE */ {Operation_Invalid},
  /* 0xDF */ {Operation_Invalid},
  /* 0xE0 */ {Operation_Invalid},
  /* 0xE1 */ {Operation_Invalid},
  /* 0xE2 */ {Operation_Invalid},
  /* 0xE3 */ {Operation_Invalid},
  /* 0xE4 */ {Operation_Invalid},
  /* 0xE5 */ {Operation_Invalid},
  /* 0xE6 */ {Operation_Invalid},
  /* 0xE7 */ {Operation_Invalid},
  /* 0xE8 */ {Operation_Invalid},
  /* 0xE9 */ {Operation_FUCOMPP, {ST(0), ST(1)}},
  /* 0xEA */ {Operation_Invalid},
  /* 0xEB */ {Operation_Invalid},
  /* 0xEC */ {Operation_Invalid},
  /* 0xED */ {Operation_Invalid},
  /* 0xEE */ {Operation_Invalid},
  /* 0xEF */ {Operation_Invalid},
  /* 0xF0 */ {Operation_Invalid},
  /* 0xF1 */ {Operation_Invalid},
  /* 0xF2 */ {Operation_Invalid},
  /* 0xF3 */ {Operation_Invalid},
  /* 0xF4 */ {Operation_Invalid},
  /* 0xF5 */ {Operation_Invalid},
  /* 0xF6 */ {Operation_Invalid},
  /* 0xF7 */ {Operation_Invalid},
  /* 0xF8 */ {Operation_Invalid},
  /* 0xF9 */ {Operation_Invalid},
  /* 0xFA */ {Operation_Invalid},
  /* 0xFB */ {Operation_Invalid},
  /* 0xFC */ {Operation_Invalid},
  /* 0xFD */ {Operation_Invalid},
  /* 0xFE */ {Operation_Invalid},
  /* 0xFF */ {Operation_Invalid},
};

const InstructionTableEntry extension_table_x87_DB[8 + 64] = {
  /* 0x00 */ {Operation_FILD, {Md}},
  /* 0x01 */ {Operation_Invalid},
  /* 0x02 */ {Operation_FIST, {Md}},
  /* 0x03 */ {Operation_FISTP, {Md}},
  /* 0x04 */ {Operation_Invalid},
  /* 0x05 */ {Operation_FLD, {Mt}},
  /* 0x06 */ {Operation_Invalid},
  /* 0x07 */ {Operation_FSTP, {Mt}},

  /* 0xC0 */ {Operation_Invalid},
  /* 0xC1 */ {Operation_Invalid},
  /* 0xC2 */ {Operation_Invalid},
  /* 0xC3 */ {Operation_Invalid},
  /* 0xC4 */ {Operation_Invalid},
  /* 0xC5 */ {Operation_Invalid},
  /* 0xC6 */ {Operation_Invalid},
  /* 0xC7 */ {Operation_Invalid},
  /* 0xC8 */ {Operation_Invalid},
  /* 0xC9 */ {Operation_Invalid},
  /* 0xCA */ {Operation_Invalid},
  /* 0xCB */ {Operation_Invalid},
  /* 0xCC */ {Operation_Invalid},
  /* 0xCD */ {Operation_Invalid},
  /* 0xCE */ {Operation_Invalid},
  /* 0xCF */ {Operation_Invalid},
  /* 0xD0 */ {Operation_Invalid},
  /* 0xD1 */ {Operation_Invalid},
  /* 0xD2 */ {Operation_Invalid},
  /* 0xD3 */ {Operation_Invalid},
  /* 0xD4 */ {Operation_Invalid},
  /* 0xD5 */ {Operation_Invalid},
  /* 0xD6 */ {Operation_Invalid},
  /* 0xD7 */ {Operation_Invalid},
  /* 0xD8 */ {Operation_Invalid},
  /* 0xD9 */ {Operation_Invalid},
  /* 0xDA */ {Operation_Invalid},
  /* 0xDB */ {Operation_Invalid},
  /* 0xDC */ {Operation_Invalid},
  /* 0xDD */ {Operation_Invalid},
  /* 0xDE */ {Operation_Invalid},
  /* 0xDF */ {Operation_Invalid},
  /* 0xE0 */ {Operation_FENI},
  /* 0xE1 */ {Operation_FDISI},
  /* 0xE2 */ {Operation_FNCLEX},
  /* 0xE3 */ {Operation_FNINIT},
  /* 0xE4 */ {Operation_FSETPM},
  /* 0xE5 */ {Operation_Invalid},
  /* 0xE6 */ {Operation_Invalid},
  /* 0xE7 */ {Operation_Invalid},
  /* 0xE8 */ {Operation_Invalid},
  /* 0xE9 */ {Operation_Invalid},
  /* 0xEA */ {Operation_Invalid},
  /* 0xEB */ {Operation_Invalid},
  /* 0xEC */ {Operation_Invalid},
  /* 0xED */ {Operation_Invalid},
  /* 0xEE */ {Operation_Invalid},
  /* 0xEF */ {Operation_Invalid},
  /* 0xF0 */ {Operation_Invalid},
  /* 0xF1 */ {Operation_Invalid},
  /* 0xF2 */ {Operation_Invalid},
  /* 0xF3 */ {Operation_Invalid},
  /* 0xF4 */ {Operation_Invalid},
  /* 0xF5 */ {Operation_Invalid},
  /* 0xF6 */ {Operation_Invalid},
  /* 0xF7 */ {Operation_Invalid},
  /* 0xF8 */ {Operation_Invalid},
  /* 0xF9 */ {Operation_Invalid},
  /* 0xFA */ {Operation_Invalid},
  /* 0xFB */ {Operation_Invalid},
  /* 0xFC */ {Operation_Invalid},
  /* 0xFD */ {Operation_Invalid},
  /* 0xFE */ {Operation_Invalid},
  /* 0xFF */ {Operation_Invalid},
};

const InstructionTableEntry extension_table_x87_DC[8 + 64] = {
  /* 0x00 */ {Operation_FADD, {ST(0), Mq}},
  /* 0x01 */ {Operation_FMUL, {ST(0), Mq}},
  /* 0x02 */ {Operation_FCOM, {ST(0), Mq}},
  /* 0x03 */ {Operation_FCOMP, {ST(0), Mq}},
  /* 0x04 */ {Operation_FSUB, {ST(0), Mq}},
  /* 0x05 */ {Operation_FSUBR, {ST(0), Mq}},
  /* 0x06 */ {Operation_FDIV, {ST(0), Mq}},
  /* 0x07 */ {Operation_FDIVR, {ST(0), Mq}},

  /* 0xC0 */ {Operation_FADD, {ST(0), ST(0)}},
  /* 0xC1 */ {Operation_FADD, {ST(1), ST(0)}},
  /* 0xC2 */ {Operation_FADD, {ST(2), ST(0)}},
  /* 0xC3 */ {Operation_FADD, {ST(3), ST(0)}},
  /* 0xC4 */ {Operation_FADD, {ST(4), ST(0)}},
  /* 0xC5 */ {Operation_FADD, {ST(5), ST(0)}},
  /* 0xC6 */ {Operation_FADD, {ST(6), ST(0)}},
  /* 0xC7 */ {Operation_FADD, {ST(7), ST(0)}},
  /* 0xC8 */ {Operation_FMUL, {ST(0), ST(0)}},
  /* 0xC9 */ {Operation_FMUL, {ST(1), ST(0)}},
  /* 0xCA */ {Operation_FMUL, {ST(2), ST(0)}},
  /* 0xCB */ {Operation_FMUL, {ST(3), ST(0)}},
  /* 0xCC */ {Operation_FMUL, {ST(4), ST(0)}},
  /* 0xCD */ {Operation_FMUL, {ST(5), ST(0)}},
  /* 0xCE */ {Operation_FMUL, {ST(6), ST(0)}},
  /* 0xCF */ {Operation_FMUL, {ST(7), ST(0)}},
  /* 0xD0 */ {Operation_FCOM, {ST(0), ST(0)}},
  /* 0xD1 */ {Operation_FCOM, {ST(1), ST(0)}},
  /* 0xD2 */ {Operation_FCOM, {ST(2), ST(0)}},
  /* 0xD3 */ {Operation_FCOM, {ST(3), ST(0)}},
  /* 0xD4 */ {Operation_FCOM, {ST(4), ST(0)}},
  /* 0xD5 */ {Operation_FCOM, {ST(5), ST(0)}},
  /* 0xD6 */ {Operation_FCOM, {ST(6), ST(0)}},
  /* 0xD7 */ {Operation_FCOM, {ST(7), ST(0)}},
  /* 0xD8 */ {Operation_FCOMP, {ST(0), ST(0)}},
  /* 0xD9 */ {Operation_FCOMP, {ST(1), ST(0)}},
  /* 0xDA */ {Operation_FCOMP, {ST(2), ST(0)}},
  /* 0xDB */ {Operation_FCOMP, {ST(3), ST(0)}},
  /* 0xDC */ {Operation_FCOMP, {ST(4), ST(0)}},
  /* 0xDD */ {Operation_FCOMP, {ST(5), ST(0)}},
  /* 0xDE */ {Operation_FCOMP, {ST(6), ST(0)}},
  /* 0xDF */ {Operation_FCOMP, {ST(7), ST(0)}},
  /* 0xE0 */ {Operation_FSUB, {ST(0), ST(0)}},
  /* 0xE1 */ {Operation_FSUB, {ST(1), ST(0)}},
  /* 0xE2 */ {Operation_FSUB, {ST(2), ST(0)}},
  /* 0xE3 */ {Operation_FSUB, {ST(3), ST(0)}},
  /* 0xE4 */ {Operation_FSUB, {ST(4), ST(0)}},
  /* 0xE5 */ {Operation_FSUB, {ST(5), ST(0)}},
  /* 0xE6 */ {Operation_FSUB, {ST(6), ST(0)}},
  /* 0xE7 */ {Operation_FSUB, {ST(7), ST(0)}},
  /* 0xE8 */ {Operation_FSUBR, {ST(0), ST(0)}},
  /* 0xE9 */ {Operation_FSUBR, {ST(1), ST(0)}},
  /* 0xEA */ {Operation_FSUBR, {ST(2), ST(0)}},
  /* 0xEB */ {Operation_FSUBR, {ST(3), ST(0)}},
  /* 0xEC */ {Operation_FSUBR, {ST(4), ST(0)}},
  /* 0xED */ {Operation_FSUBR, {ST(5), ST(0)}},
  /* 0xEE */ {Operation_FSUBR, {ST(6), ST(0)}},
  /* 0xEF */ {Operation_FSUBR, {ST(7), ST(0)}},
  /* 0xF0 */ {Operation_FDIV, {ST(0), ST(0)}},
  /* 0xF1 */ {Operation_FDIV, {ST(1), ST(0)}},
  /* 0xF2 */ {Operation_FDIV, {ST(2), ST(0)}},
  /* 0xF3 */ {Operation_FDIV, {ST(3), ST(0)}},
  /* 0xF4 */ {Operation_FDIV, {ST(4), ST(0)}},
  /* 0xF5 */ {Operation_FDIV, {ST(5), ST(0)}},
  /* 0xF6 */ {Operation_FDIV, {ST(6), ST(0)}},
  /* 0xF7 */ {Operation_FDIV, {ST(7), ST(0)}},
  /* 0xF8 */ {Operation_FDIVR, {ST(0), ST(0)}},
  /* 0xF9 */ {Operation_FDIVR, {ST(1), ST(0)}},
  /* 0xFA */ {Operation_FDIVR, {ST(2), ST(0)}},
  /* 0xFB */ {Operation_FDIVR, {ST(3), ST(0)}},
  /* 0xFC */ {Operation_FDIVR, {ST(4), ST(0)}},
  /* 0xFD */ {Operation_FDIVR, {ST(5), ST(0)}},
  /* 0xFE */ {Operation_FDIVR, {ST(6), ST(0)}},
  /* 0xFF */ {Operation_FDIVR, {ST(7), ST(0)}}};

const InstructionTableEntry extension_table_x87_DD[8 + 64] = {
  /* 0x00 */ {Operation_FLD, {Mq}},
  /* 0x01 */ {Operation_Invalid},
  /* 0x02 */ {Operation_FST, {Mq}},
  /* 0x03 */ {Operation_FSTP, {Mq}},
  /* 0x04 */ {Operation_FRSTOR},
  /* 0x05 */ {Operation_Invalid},
  /* 0x06 */ {Operation_FNSAVE},
  /* 0x07 */ {Operation_FNSTSW, {Mw}},

  /* 0xC0 */ {Operation_FFREE, {ST(0)}},
  /* 0xC1 */ {Operation_FFREE, {ST(1)}},
  /* 0xC2 */ {Operation_FFREE, {ST(2)}},
  /* 0xC3 */ {Operation_FFREE, {ST(3)}},
  /* 0xC4 */ {Operation_FFREE, {ST(4)}},
  /* 0xC5 */ {Operation_FFREE, {ST(5)}},
  /* 0xC6 */ {Operation_FFREE, {ST(6)}},
  /* 0xC7 */ {Operation_FFREE, {ST(7)}},
  /* 0xC8 */ {Operation_Invalid},
  /* 0xC9 */ {Operation_Invalid},
  /* 0xCA */ {Operation_Invalid},
  /* 0xCB */ {Operation_Invalid},
  /* 0xCC */ {Operation_Invalid},
  /* 0xCD */ {Operation_Invalid},
  /* 0xCE */ {Operation_Invalid},
  /* 0xCF */ {Operation_Invalid},
  /* 0xE0 */ {Operation_FST, {ST(0)}},
  /* 0xE1 */ {Operation_FST, {ST(1)}},
  /* 0xE2 */ {Operation_FST, {ST(2)}},
  /* 0xE3 */ {Operation_FST, {ST(3)}},
  /* 0xE4 */ {Operation_FST, {ST(4)}},
  /* 0xE5 */ {Operation_FST, {ST(5)}},
  /* 0xE6 */ {Operation_FST, {ST(6)}},
  /* 0xE7 */ {Operation_FST, {ST(7)}},
  /* 0xE8 */ {Operation_FSTP, {ST(0)}},
  /* 0xE9 */ {Operation_FSTP, {ST(1)}},
  /* 0xEA */ {Operation_FSTP, {ST(2)}},
  /* 0xEB */ {Operation_FSTP, {ST(3)}},
  /* 0xEC */ {Operation_FSTP, {ST(4)}},
  /* 0xED */ {Operation_FSTP, {ST(5)}},
  /* 0xEE */ {Operation_FSTP, {ST(6)}},
  /* 0xEF */ {Operation_FSTP, {ST(7)}},
  /* 0xE0 */ {Operation_FUCOM, {ST(0), ST(0)}},
  /* 0xE1 */ {Operation_FUCOM, {ST(0), ST(1)}},
  /* 0xE2 */ {Operation_FUCOM, {ST(0), ST(2)}},
  /* 0xE3 */ {Operation_FUCOM, {ST(0), ST(3)}},
  /* 0xE4 */ {Operation_FUCOM, {ST(0), ST(4)}},
  /* 0xE5 */ {Operation_FUCOM, {ST(0), ST(5)}},
  /* 0xE6 */ {Operation_FUCOM, {ST(0), ST(6)}},
  /* 0xE7 */ {Operation_FUCOM, {ST(0), ST(7)}},
  /* 0xE8 */ {Operation_FUCOMP, {ST(0), ST(0)}},
  /* 0xE9 */ {Operation_FUCOMP, {ST(0), ST(1)}},
  /* 0xEA */ {Operation_FUCOMP, {ST(0), ST(2)}},
  /* 0xEB */ {Operation_FUCOMP, {ST(0), ST(3)}},
  /* 0xEC */ {Operation_FUCOMP, {ST(0), ST(4)}},
  /* 0xED */ {Operation_FUCOMP, {ST(0), ST(5)}},
  /* 0xEE */ {Operation_FUCOMP, {ST(0), ST(6)}},
  /* 0xEF */ {Operation_FUCOMP, {ST(0), ST(7)}},
  /* 0xF0 */ {Operation_Invalid},
  /* 0xF1 */ {Operation_Invalid},
  /* 0xF2 */ {Operation_Invalid},
  /* 0xF3 */ {Operation_Invalid},
  /* 0xF4 */ {Operation_Invalid},
  /* 0xF5 */ {Operation_Invalid},
  /* 0xF6 */ {Operation_Invalid},
  /* 0xF7 */ {Operation_Invalid},
  /* 0xF8 */ {Operation_Invalid},
  /* 0xF9 */ {Operation_Invalid},
  /* 0xFA */ {Operation_Invalid},
  /* 0xFB */ {Operation_Invalid},
  /* 0xFC */ {Operation_Invalid},
  /* 0xFD */ {Operation_Invalid},
  /* 0xFE */ {Operation_Invalid},
  /* 0xFF */ {Operation_Invalid},
};

const InstructionTableEntry extension_table_x87_DE[8 + 64] = {
  /* 0x00 */ {Operation_FIADD, {Md}},
  /* 0x01 */ {Operation_FIMUL, {Md}},
  /* 0x02 */ {Operation_FICOM, {Md}},
  /* 0x03 */ {Operation_FICOMP, {Md}},
  /* 0x04 */ {Operation_FISUB, {Md}},
  /* 0x05 */ {Operation_FISUBR, {Md}},
  /* 0x06 */ {Operation_FIDIV, {Md}},
  /* 0x07 */ {Operation_FIDIVR, {Md}},

  /* 0xC0 */ {Operation_FADDP, {ST(0), ST(0)}},
  /* 0xC1 */ {Operation_FADDP, {ST(1), ST(0)}},
  /* 0xC2 */ {Operation_FADDP, {ST(2), ST(0)}},
  /* 0xC3 */ {Operation_FADDP, {ST(3), ST(0)}},
  /* 0xC4 */ {Operation_FADDP, {ST(4), ST(0)}},
  /* 0xC5 */ {Operation_FADDP, {ST(5), ST(0)}},
  /* 0xC6 */ {Operation_FADDP, {ST(6), ST(0)}},
  /* 0xC7 */ {Operation_FADDP, {ST(7), ST(0)}},
  /* 0xC8 */ {Operation_FMULP, {ST(0), ST(0)}},
  /* 0xC9 */ {Operation_FMULP, {ST(1), ST(0)}},
  /* 0xCA */ {Operation_FMULP, {ST(2), ST(0)}},
  /* 0xCB */ {Operation_FMULP, {ST(3), ST(0)}},
  /* 0xCC */ {Operation_FMULP, {ST(4), ST(0)}},
  /* 0xCD */ {Operation_FMULP, {ST(5), ST(0)}},
  /* 0xCE */ {Operation_FMULP, {ST(6), ST(0)}},
  /* 0xCF */ {Operation_FMULP, {ST(7), ST(0)}},
  /* 0xD0 */ {Operation_Invalid},
  /* 0xD1 */ {Operation_Invalid},
  /* 0xD2 */ {Operation_Invalid},
  /* 0xD3 */ {Operation_Invalid},
  /* 0xD4 */ {Operation_Invalid},
  /* 0xD5 */ {Operation_Invalid},
  /* 0xD6 */ {Operation_Invalid},
  /* 0xD7 */ {Operation_Invalid},
  /* 0xD8 */ {Operation_Invalid},
  /* 0xD9 */ {Operation_FCOMPP, {ST(0), ST(1)}},
  /* 0xDA */ {Operation_Invalid},
  /* 0xDB */ {Operation_Invalid},
  /* 0xDC */ {Operation_Invalid},
  /* 0xDD */ {Operation_Invalid},
  /* 0xDE */ {Operation_Invalid},
  /* 0xDF */ {Operation_Invalid},
  /* 0xE0 */ {Operation_FSUBRP, {ST(0), ST(0)}},
  /* 0xE1 */ {Operation_FSUBRP, {ST(1), ST(0)}},
  /* 0xE2 */ {Operation_FSUBRP, {ST(2), ST(0)}},
  /* 0xE3 */ {Operation_FSUBRP, {ST(3), ST(0)}},
  /* 0xE4 */ {Operation_FSUBRP, {ST(4), ST(0)}},
  /* 0xE5 */ {Operation_FSUBRP, {ST(5), ST(0)}},
  /* 0xE6 */ {Operation_FSUBRP, {ST(6), ST(0)}},
  /* 0xE7 */ {Operation_FSUBRP, {ST(7), ST(0)}},
  /* 0xE8 */ {Operation_FSUBP, {ST(0), ST(0)}},
  /* 0xE9 */ {Operation_FSUBP, {ST(1), ST(0)}},
  /* 0xEA */ {Operation_FSUBP, {ST(2), ST(0)}},
  /* 0xEB */ {Operation_FSUBP, {ST(3), ST(0)}},
  /* 0xEC */ {Operation_FSUBP, {ST(4), ST(0)}},
  /* 0xED */ {Operation_FSUBP, {ST(5), ST(0)}},
  /* 0xEE */ {Operation_FSUBP, {ST(6), ST(0)}},
  /* 0xEF */ {Operation_FSUBP, {ST(7), ST(0)}},
  /* 0xF0 */ {Operation_FDIVRP, {ST(0), ST(0)}},
  /* 0xF1 */ {Operation_FDIVRP, {ST(1), ST(0)}},
  /* 0xF2 */ {Operation_FDIVRP, {ST(2), ST(0)}},
  /* 0xF3 */ {Operation_FDIVRP, {ST(3), ST(0)}},
  /* 0xF4 */ {Operation_FDIVRP, {ST(4), ST(0)}},
  /* 0xF5 */ {Operation_FDIVRP, {ST(5), ST(0)}},
  /* 0xF6 */ {Operation_FDIVRP, {ST(6), ST(0)}},
  /* 0xF7 */ {Operation_FDIVRP, {ST(7), ST(0)}},
  /* 0xF8 */ {Operation_FDIVP, {ST(0), ST(0)}},
  /* 0xF9 */ {Operation_FDIVP, {ST(1), ST(0)}},
  /* 0xFA */ {Operation_FDIVP, {ST(2), ST(0)}},
  /* 0xFB */ {Operation_FDIVP, {ST(3), ST(0)}},
  /* 0xFC */ {Operation_FDIVP, {ST(4), ST(0)}},
  /* 0xFD */ {Operation_FDIVP, {ST(5), ST(0)}},
  /* 0xFE */ {Operation_FDIVP, {ST(6), ST(0)}},
  /* 0xFF */ {Operation_FDIVP, {ST(7), ST(0)}}};

const InstructionTableEntry extension_table_x87_DF[8 + 64] = {
  /* 0x00 */ {Operation_FILD, {Md}},
  /* 0x01 */ {Operation_Invalid},
  /* 0x02 */ {Operation_FIST, {Md}},
  /* 0x03 */ {Operation_FISTP, {Md}},
  /* 0x04 */ {Operation_FBLD, {Mt}},
  /* 0x05 */ {Operation_FILD, {Mq}},
  /* 0x06 */ {Operation_FBSTP, {Mt}},
  /* 0x07 */ {Operation_FISTP, {Mq}},

  /* 0xC0 */ {Operation_Invalid},
  /* 0xC1 */ {Operation_Invalid},
  /* 0xC2 */ {Operation_Invalid},
  /* 0xC3 */ {Operation_Invalid},
  /* 0xC4 */ {Operation_Invalid},
  /* 0xC5 */ {Operation_Invalid},
  /* 0xC6 */ {Operation_Invalid},
  /* 0xC7 */ {Operation_Invalid},
  /* 0xC8 */ {Operation_Invalid},
  /* 0xC9 */ {Operation_Invalid},
  /* 0xCA */ {Operation_Invalid},
  /* 0xCB */ {Operation_Invalid},
  /* 0xCC */ {Operation_Invalid},
  /* 0xCD */ {Operation_Invalid},
  /* 0xCE */ {Operation_Invalid},
  /* 0xCF */ {Operation_Invalid},
  /* 0xD0 */ {Operation_Invalid},
  /* 0xD1 */ {Operation_Invalid},
  /* 0xD2 */ {Operation_Invalid},
  /* 0xD3 */ {Operation_Invalid},
  /* 0xD4 */ {Operation_Invalid},
  /* 0xD5 */ {Operation_Invalid},
  /* 0xD6 */ {Operation_Invalid},
  /* 0xD7 */ {Operation_Invalid},
  /* 0xD8 */ {Operation_Invalid},
  /* 0xD9 */ {Operation_Invalid},
  /* 0xDA */ {Operation_Invalid},
  /* 0xDB */ {Operation_Invalid},
  /* 0xDC */ {Operation_Invalid},
  /* 0xDD */ {Operation_Invalid},
  /* 0xDE */ {Operation_Invalid},
  /* 0xDF */ {Operation_Invalid},
  /* 0xE0 */ {Operation_FNSTSW, {AX}},
  /* 0xE1 */ {Operation_Invalid},
  /* 0xE2 */ {Operation_Invalid},
  /* 0xE3 */ {Operation_Invalid},
  /* 0xE4 */ {Operation_Invalid},
  /* 0xE5 */ {Operation_Invalid},
  /* 0xE6 */ {Operation_Invalid},
  /* 0xE7 */ {Operation_Invalid},
  /* 0xE8 */ {Operation_Invalid},
  /* 0xE9 */ {Operation_Invalid},
  /* 0xEA */ {Operation_Invalid},
  /* 0xEB */ {Operation_Invalid},
  /* 0xEC */ {Operation_Invalid},
  /* 0xED */ {Operation_Invalid},
  /* 0xEE */ {Operation_Invalid},
  /* 0xEF */ {Operation_Invalid},
  /* 0xF0 */ {Operation_Invalid},
  /* 0xF1 */ {Operation_Invalid},
  /* 0xF2 */ {Operation_Invalid},
  /* 0xF3 */ {Operation_Invalid},
  /* 0xF4 */ {Operation_Invalid},
  /* 0xF5 */ {Operation_Invalid},
  /* 0xF6 */ {Operation_Invalid},
  /* 0xF7 */ {Operation_Invalid},
  /* 0xF8 */ {Operation_Invalid},
  /* 0xF9 */ {Operation_Invalid},
  /* 0xFA */ {Operation_Invalid},
  /* 0xFB */ {Operation_Invalid},
  /* 0xFC */ {Operation_Invalid},
  /* 0xFD */ {Operation_Invalid},
  /* 0xFE */ {Operation_Invalid},
  /* 0xFF */ {Operation_Invalid},
};

#undef Eb
#undef Ev
#undef Ew
#undef Gb
#undef Gv
#undef Sw
#undef Ib
#undef Iw
#undef Iv
#undef M
#undef Ap
#undef AL
#undef AX
#undef CX
#undef DX
#undef BX
#undef SP
#undef BP
#undef SI
#undef DI
#undef CS
#undef DS
#undef ES
#undef SS

struct ModRMTableEntry
{
  AddressingMode addressing_mode;
  uint8 base_register;
  uint8 index_register;
  uint8 displacement_size;
  DefaultSegment default_segment;
};

// This could probably be implemented procedurally
// http://www.sandpile.org/x86/opc_rm16.htm
static const ModRMTableEntry modrm_table_16[32] = {
  /* 00 000 - [BX + SI]           */ {AddressingMode_BasedIndexed, Reg16_BX, Reg16_SI, 0, DefaultSegment_Unspecified},
  /* 00 001 - [BX + DI]           */ {AddressingMode_BasedIndexed, Reg16_BX, Reg16_DI, 0, DefaultSegment_Unspecified},
  /* 00 010 - [BP + SI]           */ {AddressingMode_BasedIndexed, Reg16_BP, Reg16_SI, 0, DefaultSegment_SS},
  /* 00 011 - [BP + DI]           */ {AddressingMode_BasedIndexed, Reg16_BP, Reg16_DI, 0, DefaultSegment_SS},
  /* 00 100 - [SI]                */ {AddressingMode_RegisterIndirect, Reg16_SI, 0, 0, DefaultSegment_Unspecified},
  /* 00 101 - [DI]                */ {AddressingMode_RegisterIndirect, Reg16_DI, 0, 0, DefaultSegment_Unspecified},
  /* 00 110 - [sword]             */ {AddressingMode_Direct, 0, 0, 0, DefaultSegment_Unspecified},
  /* 00 111 - [BX]                */ {AddressingMode_RegisterIndirect, Reg16_BX, 0, 0, DefaultSegment_Unspecified},
  /* 01 000 - [BX + SI + sbyte]   */
  {AddressingMode_BasedIndexedDisplacement, Reg16_BX, Reg16_SI, 1, DefaultSegment_Unspecified},
  /* 01 001 - [BX + DI + sbyte]   */
  {AddressingMode_BasedIndexedDisplacement, Reg16_BX, Reg16_DI, 1, DefaultSegment_Unspecified},
  /* 01 010 - [BP + SI + sbyte]   */
  {AddressingMode_BasedIndexedDisplacement, Reg16_BP, Reg16_SI, 1, DefaultSegment_SS},
  /* 01 011 - [BP + DI + sbyte]   */
  {AddressingMode_BasedIndexedDisplacement, Reg16_BP, Reg16_DI, 1, DefaultSegment_SS},
  /* 01 100 - [SI + sbyte]        */ {AddressingMode_Indexed, Reg16_SI, 0, 1, DefaultSegment_Unspecified},
  /* 01 101 - [DI + sbyte]        */ {AddressingMode_Indexed, Reg16_DI, 0, 1, DefaultSegment_Unspecified},
  /* 01 110 - [BP + sbyte]        */ {AddressingMode_Indexed, Reg16_BP, 0, 1, DefaultSegment_SS},
  /* 01 111 - [BX + sbyte]        */ {AddressingMode_Indexed, Reg16_BX, 0, 1, DefaultSegment_Unspecified},
  /* 10 000 - [BX + SI + sword]   */
  {AddressingMode_BasedIndexedDisplacement, Reg16_BX, Reg16_SI, 2, DefaultSegment_Unspecified},
  /* 10 001 - [BX + DI + sword]   */
  {AddressingMode_BasedIndexedDisplacement, Reg16_BX, Reg16_DI, 2, DefaultSegment_Unspecified},
  /* 10 010 - [BP + SI + sword]   */
  {AddressingMode_BasedIndexedDisplacement, Reg16_BP, Reg16_SI, 2, DefaultSegment_SS},
  /* 10 011 - [BP + DI + sword]   */
  {AddressingMode_BasedIndexedDisplacement, Reg16_BP, Reg16_DI, 2, DefaultSegment_SS},
  /* 10 100 - [SI + sword]        */ {AddressingMode_Indexed, Reg16_SI, 0, 2, DefaultSegment_Unspecified},
  /* 10 101 - [DI + sword]        */ {AddressingMode_Indexed, Reg16_DI, 0, 2, DefaultSegment_Unspecified},
  /* 10 110 - [BP + sword]        */ {AddressingMode_Indexed, Reg16_BP, 0, 2, DefaultSegment_SS},
  /* 10 111 - [BX + sword]        */ {AddressingMode_Indexed, Reg16_BX, 0, 2, DefaultSegment_Unspecified},
  /* 11 000 - AL/AX               */ {AddressingMode_Register, Reg16_AX, 0, 0, DefaultSegment_Unspecified},
  /* 11 001 - CL/CX               */ {AddressingMode_Register, Reg16_CX, 0, 0, DefaultSegment_Unspecified},
  /* 11 010 - DL/DX               */ {AddressingMode_Register, Reg16_DX, 0, 0, DefaultSegment_Unspecified},
  /* 11 011 - BL/BX               */ {AddressingMode_Register, Reg16_BX, 0, 0, DefaultSegment_Unspecified},
  /* 11 100 - AH/SP               */ {AddressingMode_Register, Reg16_SP, 0, 0, DefaultSegment_Unspecified},
  /* 11 101 - CH/BP               */ {AddressingMode_Register, Reg16_BP, 0, 0, DefaultSegment_Unspecified},
  /* 11 110 - DH/SI               */ {AddressingMode_Register, Reg16_SI, 0, 0, DefaultSegment_Unspecified},
  /* 11 111 - BH/DI               */ {AddressingMode_Register, Reg16_DI, 0, 0, DefaultSegment_Unspecified},
};

// This could probably be implemented procedurally
// http://www.sandpile.org/x86/opc_rm.htm / http://www.sandpile.org/x86/opc_sib.htm
static const ModRMTableEntry modrm_table_32[32] = {
  /* 00 000 - [eAX]               */ {AddressingMode_RegisterIndirect, Reg32_EAX, 0, 0, DefaultSegment_Unspecified},
  /* 00 001 - [eCX]               */ {AddressingMode_RegisterIndirect, Reg32_ECX, 0, 0, DefaultSegment_Unspecified},
  /* 00 010 - [eDX]               */ {AddressingMode_RegisterIndirect, Reg32_EDX, 0, 0, DefaultSegment_Unspecified},
  /* 00 011 - [eBX]               */ {AddressingMode_RegisterIndirect, Reg32_EBX, 0, 0, DefaultSegment_Unspecified},
  /* 00 100 - [sib]               */ {AddressingMode_SIB, 0, 0, 0, DefaultSegment_Unspecified},
  /* 00 101 - [dword]             */ {AddressingMode_Direct, 0, 0, 0, DefaultSegment_Unspecified},
  /* 00 110 - [SI]                */ {AddressingMode_RegisterIndirect, Reg32_ESI, 0, 0, DefaultSegment_Unspecified},
  /* 00 111 - [DI]                */ {AddressingMode_RegisterIndirect, Reg32_EDI, 0, 0, DefaultSegment_Unspecified},
  /* 01 000 - [eAX + sbyte]       */ {AddressingMode_Indexed, Reg32_EAX, 0, 1, DefaultSegment_Unspecified},
  /* 01 001 - [eCX + sbyte]       */ {AddressingMode_Indexed, Reg32_ECX, 0, 1, DefaultSegment_Unspecified},
  /* 01 010 - [eDX + sbyte]       */ {AddressingMode_Indexed, Reg32_EDX, 0, 1, DefaultSegment_Unspecified},
  /* 01 011 - [eBX + sbyte]       */ {AddressingMode_Indexed, Reg32_EBX, 0, 1, DefaultSegment_Unspecified},
  /* 01 100 - [sib + sbyte]       */ {AddressingMode_SIB, 0, 0, 1, DefaultSegment_Unspecified},
  /* 01 101 - [eBP + sbyte]       */ {AddressingMode_Indexed, Reg32_EBP, 0, 1, DefaultSegment_SS},
  /* 01 110 - [eSI + sbyte]       */ {AddressingMode_Indexed, Reg32_ESI, 0, 1, DefaultSegment_Unspecified},
  /* 01 111 - [eDI + sbyte]       */ {AddressingMode_Indexed, Reg32_EDI, 0, 1, DefaultSegment_Unspecified},
  /* 10 000 - [eAX + sdword]      */ {AddressingMode_Indexed, Reg32_EAX, 0, 4, DefaultSegment_Unspecified},
  /* 10 001 - [eCX + sdword]      */ {AddressingMode_Indexed, Reg32_ECX, 0, 4, DefaultSegment_Unspecified},
  /* 10 010 - [eDX + sdword]      */ {AddressingMode_Indexed, Reg32_EDX, 0, 4, DefaultSegment_Unspecified},
  /* 10 011 - [eBX + sdword]      */ {AddressingMode_Indexed, Reg32_EBX, 0, 4, DefaultSegment_Unspecified},
  /* 10 100 - [sib + sdword]      */ {AddressingMode_SIB, 0, 0, 4, DefaultSegment_Unspecified},
  /* 10 101 - [eBP + sdword]      */ {AddressingMode_Indexed, Reg32_EBP, 0, 4, DefaultSegment_SS},
  /* 10 110 - [eSI + sdword]      */ {AddressingMode_Indexed, Reg32_ESI, 0, 4, DefaultSegment_Unspecified},
  /* 10 111 - [eDI + sdword]      */ {AddressingMode_Indexed, Reg32_EDI, 0, 4, DefaultSegment_Unspecified},
  /* 11 000 - AL/AX/EAX           */ {AddressingMode_Register, Reg32_EAX, 0, 0, DefaultSegment_Unspecified},
  /* 11 001 - CL/CX/ECX           */ {AddressingMode_Register, Reg32_ECX, 0, 0, DefaultSegment_Unspecified},
  /* 11 010 - DL/DX/EDX           */ {AddressingMode_Register, Reg32_EDX, 0, 0, DefaultSegment_Unspecified},
  /* 11 011 - BL/BX/EBX           */ {AddressingMode_Register, Reg32_EBX, 0, 0, DefaultSegment_Unspecified},
  /* 11 100 - AH/SP/ESP           */ {AddressingMode_Register, Reg32_ESP, 0, 0, DefaultSegment_Unspecified},
  /* 11 101 - CH/BP/EBP           */ {AddressingMode_Register, Reg32_EBP, 0, 0, DefaultSegment_Unspecified},
  /* 11 110 - DH/SI/ESI           */ {AddressingMode_Register, Reg32_ESI, 0, 0, DefaultSegment_Unspecified},
  /* 11 111 - BH/DI/EDI           */ {AddressingMode_Register, Reg32_EDI, 0, 0, DefaultSegment_Unspecified},
};

static Segment DefaultSegmentToSegment(DefaultSegment default_segment)
{
  DebugAssert(default_segment != DefaultSegment_Unspecified);
  switch (default_segment)
  {
    case DefaultSegment_DS:
      return Segment_DS;
    case DefaultSegment_ES:
      return Segment_ES;
    case DefaultSegment_SS:
      return Segment_SS;
    case DefaultSegment_CS:
      return Segment_CS;
    case DefaultSegment_FS:
      return Segment_FS;
    case DefaultSegment_GS:
      return Segment_GS;
    default:
      UnreachableCode();
      return Segment_DS;
  }
}

class InstructionDecoder
{
public:
  InstructionDecoder(OldInstruction* instruction_, AddressSize address_size_, OperandSize operand_size_,
                     InstructionFetchCallback& fetch_callback_)
    : instruction(instruction_), default_address_size(address_size_), default_operand_size(operand_size_),
      fetch_callback(fetch_callback_)

  {
  }

  bool DecodeInstruction();

private:
  uint8 FetchByte();
  uint16 FetchWord();
  uint32 FetchDWord();
  int8 FetchSignedByte();
  int16 FetchSignedWord();
  int32 FetchSignedDWord();
  uint8 FetchModRM();
  uint8 FetchModRM_ModField();
  uint8 FetchModRM_RegField();
  uint8 FetchModRM_RMField();

  void DecodePrefixes();
  bool DecodeOperation();
  bool DecodeOperand(size_t index);
  bool DecodeOperands();

  void SetRepeatFlags();

  OldInstruction* instruction;
  InstructionFetchCallback& fetch_callback;

  uint8 current_byte = 0;
  uint8 rep_prefix = 0;
  uint8 modrm = 0;

  AddressSize default_address_size;
  OperandSize default_operand_size;

  bool has_segment_override = false;
  bool has_operand_size_override = false;
  bool has_address_size_override = false;
  bool has_modrm = false;

  struct OperandData
  {
    OperandSize size;
    OperandMode mode;
    union
    {
      uint32 data;

      uint8 reg;
      uint32 constant;
    };
  };
  OperandData operand_data[3] = {};

#ifdef DEBUG_DECODER
  uint8 decode_buffer[16] = {};
#endif
};

uint8 InstructionDecoder::FetchByte()
{
  uint8 value = fetch_callback.FetchByte();
#ifdef DEBUG_DECODER
  DebugAssert((instruction->length + 1) <= countof(decode_buffer));
  decode_buffer[instruction->length++] = value;
#else
  instruction->length++;
#endif
  return value;
}

uint16 InstructionDecoder::FetchWord()
{
  uint16 value = fetch_callback.FetchWord();
#ifdef DEBUG_DECODER
  DebugAssert((instruction->length + sizeof(value)) <= countof(decode_buffer));
  std::memcpy(&decode_buffer[instruction->length], &value, sizeof(value));
  instruction->length += sizeof(value);
#else
  instruction->length += sizeof(value);
#endif
  return value;
}

uint32 InstructionDecoder::FetchDWord()
{
  uint32 value = fetch_callback.FetchDWord();
#ifdef DEBUG_DECODER
  DebugAssert((instruction->length + sizeof(value)) <= countof(decode_buffer));
  std::memcpy(&decode_buffer[instruction->length], &value, sizeof(value));
  instruction->length += sizeof(value);
#else
  instruction->length += sizeof(value);
#endif
  return value;
}

int8 InstructionDecoder::FetchSignedByte()
{
  return int8(FetchByte());
}

int16 InstructionDecoder::FetchSignedWord()
{
  return int16(FetchWord());
}

int32 InstructionDecoder::FetchSignedDWord()
{
  return int32(FetchDWord());
}

uint8 InstructionDecoder::FetchModRM()
{
  if (!has_modrm)
  {
    modrm = FetchByte();
    has_modrm = true;
  }

  return modrm;
}

uint8 InstructionDecoder::FetchModRM_ModField()
{
  uint8 value = FetchModRM();
  return (value >> 6);
}

uint8 InstructionDecoder::FetchModRM_RegField()
{
  uint8 value = FetchModRM();
  return ((value >> 3) & 7);
}

uint8 InstructionDecoder::FetchModRM_RMField()
{
  uint8 value = FetchModRM();
  return (value & 7);
}

void InstructionDecoder::DecodePrefixes()
{
  for (;;)
  {
    const InstructionTableEntry* table_entry = &instruction_table[current_byte];
    switch (table_entry->operation)
    {
      case Operation_Lock_Prefix:
        instruction->flags |= InstructionFlag_Lock;
        break;

      case Operation_Segment_Prefix:
        instruction->segment = DefaultSegmentToSegment(table_entry->default_segment);
        has_segment_override = true;
        break;

      case Operation_Rep_Prefix:
      case Operation_RepNE_Prefix:
        rep_prefix = current_byte;
        break;

      case Operation_OperandSize_Prefix:
      {
        // "Flipping" like this causes issues if the prefix is specified more than once
        if (!has_operand_size_override)
        {
          default_operand_size = (default_operand_size == OperandSize_32) ? OperandSize_16 : OperandSize_32;
          has_operand_size_override = true;
        }
      }
      break;

      case Operation_AddressSize_Prefix:
      {
        // "Flipping" like this causes issues if the prefix is specified more than once
        if (!has_address_size_override)
        {
          default_address_size = (default_address_size == AddressSize_32) ? AddressSize_16 : AddressSize_32;
          has_address_size_override = true;
        }
      }
      break;

      default:
        // Not a prefix byte.
        return;
    }

    // Fetch next byte
    current_byte = FetchByte();
  }
}

bool InstructionDecoder::DecodeOperation()
{
  const InstructionTableEntry* table_entry = &instruction_table[current_byte];

  // Handle extension instructions.
  if (table_entry->operation == Operation_Extension)
  {
    current_byte = FetchByte();
    table_entry = &table_entry->extension_table[current_byte];
  }
  else if (table_entry->operation == Operation_Extension_ModRM_X87)
  {
    current_byte = FetchModRM();
    if ((current_byte & 0xC0) == 0xC0)
      table_entry = &table_entry->extension_table[8 + (current_byte & 0x3F)];
    else
      table_entry = &table_entry->extension_table[(current_byte >> 3) & 0x07];
  }

  // Handle invalid instructions.
  if (table_entry->operation == Operation_Invalid)
    return false;

  // Copy in operand data first, since it can be overridden by extensions.
  instruction->operation = table_entry->operation;
  for (size_t i = 0; i < countof(table_entry->operands); i++)
  {
    operand_data[i].mode = table_entry->operands[i].mode;
    operand_data[i].size = table_entry->operands[i].size;
    operand_data[i].data = table_entry->operands[i].data;
  }

  // Handle extension instructions.
  if (table_entry->operation == Operation_Extension_ModRM_Reg)
  {
    // Extract the index from the modrm byte (reg field)
    uint8 index = FetchModRM_RegField();
    DebugAssert(table_entry->extension_table && index < 8);
    table_entry = &table_entry->extension_table[index];

    // Handle invalid instructions.
    if (table_entry->operation == Operation_Invalid)
      return false;

    // Re-copy operands, allowing overwrites
    instruction->operation = table_entry->operation;
    for (size_t i = 0; i < countof(table_entry->operands); i++)
    {
      if (table_entry->operands[i].mode != OperandMode_None)
      {
        operand_data[i].mode = table_entry->operands[i].mode;
        operand_data[i].size = table_entry->operands[i].size;
        operand_data[i].data = table_entry->operands[i].data;
      }
    }
  }

  // Set data from instruction table.
  instruction->jump_condition = table_entry->jump_condition;
  instruction->operand_size = default_operand_size;
  instruction->address_size = default_address_size;
  if (!has_segment_override)
    instruction->segment = DefaultSegmentToSegment(table_entry->default_segment);

  return true;
}

bool InstructionDecoder::DecodeOperand(size_t index)
{
  OperandData* data = &operand_data[index];
  OldInstruction::Operand* operand = &instruction->operands[index];
  DebugAssert(index < countof(operand_data));

  // Handle "native" operand sizes
  OperandSize operand_size = data->size;
  if (operand_size == OperandSize_Count)
    operand_size = default_operand_size;

  switch (data->mode)
  {
    case OperandMode_None:
    {
      // Copy the size across, needed for some instructions e.g. LODSB
      // TODO: Cleaner solution
      operand->mode = AddressingMode_None;
      operand->size = operand_size;
      return true;
    }

      // Opcodes with the operand immediately after the opcode
    case OperandMode_Immediate:
    {
      operand->mode = AddressingMode_Immediate;
      operand->size = operand_size;
      if (operand_size == OperandSize_8)
        operand->immediate.value8 = FetchByte();
      else if (operand_size == OperandSize_16)
        operand->immediate.value16 = FetchWord();
      else if (operand_size == OperandSize_32)
        operand->immediate.value32 = FetchDWord();
      else
        return false;

      return true;
    }

      // Constants
    case OperandMode_Constant:
    {
      operand->mode = AddressingMode_Immediate;
      operand->size = operand_size;
      operand->immediate.value32 = data->constant;
      return true;
    }
    break;

      // Opcodes with register embedded as part of the opcode
    case OperandMode_Register:
    {
      operand->mode = AddressingMode_Register;
      operand->size = operand_size;
      operand->reg.raw = data->reg;
      return true;
    }

      // Segment register embedded as part of the opcode
    case OperandMode_SegmentRegister:
    {
      // Always 16 bits
      operand->mode = AddressingMode_SegmentRegister;
      operand->size = OperandSize_16;
      operand->reg.raw = data->reg;
      return true;
    }

      // Register indirect, i.e. MOVS/CMPS/STOS/LODS instructions
    case OperandMode_RegisterIndirect:
    {
      operand->mode = AddressingMode_RegisterIndirect;
      operand->size = operand_size;
      operand->reg.raw = data->reg;
      return true;
    }

      // Opcodes with address specified without mod/rm
    case OperandMode_Memory:
    {
      // Should only have word addresses.
      operand->mode = AddressingMode_Direct;
      operand->size = operand_size;
      if (default_address_size == AddressSize_16)
        operand->direct.address = FetchWord();
      else if (default_address_size == AddressSize_32)
        operand->direct.address = FetchDWord();
      else
        return false;

      return true;
    }

      // Far direct addresses (eg CALL 0000:1111)
    case OperandMode_FarAddress:
    {
      operand->mode = AddressingMode_FarAddress;
      operand->size = operand_size;
      if (operand_size == OperandSize_16)
        operand->far_address.address = FetchWord();
      else if (operand_size == OperandSize_32)
        operand->far_address.address = FetchDWord();
      else
        return false;

      operand->far_address.segment_selector = FetchWord();
      return true;
    }

      // Opcodes with mod/rm, and this operand uses the reg field
    case OperandMode_ModRM_Reg:
    {
      operand->mode = AddressingMode_Register;
      operand->size = operand_size;
      operand->reg.raw = FetchModRM_RegField();
      return true;
    }
    break;

      // Operands with mod/rm, and this operand uses the rm field (dynamic)
    case OperandMode_ModRM_RM:
    {
      uint8 mod = FetchModRM_ModField();
      uint8 rm = FetchModRM_RMField();

      // Use modrm table to determine addressing mode
      uint8 table_index = mod << 3 | rm;
      DebugAssert(table_index < countof(modrm_table_32));

      // 32-bit operand size *or* address size triggers new addressing????
      const ModRMTableEntry* modrm_entry;
      // if (address_size == AddressSize_32 || operand_size == OperandSize_32)
      if (default_address_size == AddressSize_32)
        modrm_entry = &modrm_table_32[table_index];
      else
        modrm_entry = &modrm_table_16[table_index];

      DefaultSegment default_segment = modrm_entry->default_segment;
      operand->mode = modrm_entry->addressing_mode;
      operand->size = operand_size;

      // SIB must be read before displacement
      uint8 sib = 0;
      if (modrm_entry->addressing_mode == AddressingMode_SIB)
        sib = FetchByte();

      // Read displacement if present
      int32 displacement = 0;
      if (modrm_entry->displacement_size == 1)
        displacement = int32(FetchSignedByte());
      else if (modrm_entry->displacement_size == 2)
        displacement = int32(FetchSignedWord());
      else if (modrm_entry->displacement_size == 4)
        displacement = FetchSignedDWord();

      // TODO: Pad fields so they can be initialized with one code path
      switch (modrm_entry->addressing_mode)
      {
        case AddressingMode_Register:
          operand->reg.raw = modrm_entry->base_register;
          break;

        case AddressingMode_RegisterIndirect:
          operand->reg.raw = modrm_entry->base_register;
          break;

        case AddressingMode_Direct:
        {
          if (default_address_size == AddressSize_16)
            operand->direct.address = ZeroExtend32(FetchWord());
          else if (default_address_size == AddressSize_32)
            operand->direct.address = FetchDWord();
          else
            return false;
        }
        break;

        case AddressingMode_Indexed:
          operand->indexed.reg.raw = modrm_entry->base_register;
          operand->indexed.displacement = displacement;
          break;

        case AddressingMode_BasedIndexed:
          operand->based_indexed.base.raw = modrm_entry->base_register;
          operand->based_indexed.index.raw = modrm_entry->index_register;
          break;

        case AddressingMode_BasedIndexedDisplacement:
          operand->based_indexed_displacement.base.raw = modrm_entry->base_register;
          operand->based_indexed_displacement.index.raw = modrm_entry->index_register;
          operand->based_indexed_displacement.displacement = displacement;
          break;

        case AddressingMode_SIB:
        {
          uint8 base_register = (sib & 0x7);
          uint8 index_register = ((sib >> 3) & 0x7);
          uint8 scaling_factor = ((sib >> 6) & 0x3);

          operand->sib.base.raw = base_register;
          if (operand->sib.base.reg32 == Reg32_ESP)
          {
            // Default to SS for ESP register
            default_segment = DefaultSegment_SS;
          }
          else if (operand->sib.base.reg32 == Reg32_EBP)
          {
            // EBP means no base if mod == 00, BP otherwise
            if (mod == 0b00)
            {
              // Though we do have a displacement dword
              DebugAssert(modrm_entry->displacement_size == 0);
              operand->sib.base.reg32 = Reg32_Count;
              displacement = FetchSignedDWord();
            }
            else
            {
              // EBP register also defaults to stack segment
              // This isn't documented in the Intel manual...
              default_segment = DefaultSegment_SS;
            }
          }

          operand->sib.index.raw = index_register;
          operand->sib.scale_shift = scaling_factor;
          if (operand->sib.index.reg32 == Reg32_ESP)
          {
            // ESP means no index
            // TODO: Convert to different mode?
            operand->sib.index.reg32 = Reg32_Count;
            operand->sib.scale_shift = 0;
          }

          operand->sib.displacement = displacement;
        }
        break;
      }

      // Override segment if using the default segment
      if (!has_segment_override && default_segment != DefaultSegment_Unspecified)
        instruction->segment = DefaultSegmentToSegment(default_segment);

      return true;
    }

    case OperandMode_ModRM_SegmentReg:
    {
      uint8 reg = FetchModRM_RegField();
      operand->mode = AddressingMode_SegmentRegister;
      operand->size = operand_size;
      switch (reg)
      {
        case 0b000:
          operand->reg.sreg = Segment_ES;
          return true;
        case 0b001:
          operand->reg.sreg = Segment_CS;
          return true;
        case 0b010:
          operand->reg.sreg = Segment_SS;
          return true;
        case 0b011:
          operand->reg.sreg = Segment_DS;
          return true;
        case 0b100:
          operand->reg.sreg = Segment_FS;
          return true; // Only valid in 32-bit addressing mode?
        case 0b101:
          operand->reg.sreg = Segment_GS;
          return true; // Only valid in 32-bit addressing mode?
      }

      return false;
    }

      // Relative
    case OperandMode_Relative:
    {
      operand->mode = AddressingMode_Relative;
      operand->size = operand_size;
      if (operand_size == OperandSize_8)
        operand->relative.displacement = int32(FetchSignedByte());
      else if (operand_size == OperandSize_16)
        operand->relative.displacement = int32(FetchSignedWord());
      else if (operand_size == OperandSize_32)
        operand->relative.displacement = int32(FetchSignedDWord());
      else
        return false;

      return true;
    }

    case OperandMode_ModRM_ControlRegister:
    {
      operand->mode = AddressingMode_ControlRegister;
      operand->size = operand_size;
      operand->reg.raw = FetchModRM_RegField();
      return true;
    }

    case OperandMode_ModRM_DebugRegister:
    {
      operand->mode = AddressingMode_DebugRegister;
      operand->size = operand_size;
      operand->reg.raw = FetchModRM_RegField();
      return true;
    }

    case OperandMode_ModRM_TestRegister:
    {
      operand->mode = AddressingMode_TestRegister;
      operand->size = operand_size;
      operand->reg.raw = FetchModRM_RegField();
      return true;
    }

    case OperandMode_FPRegister:
    {
      operand->mode = AddressingMode_ST;
      operand->size = data->size;
      operand->st.index = data->reg;
      return true;
    }
  }

  return false;
}

bool InstructionDecoder::DecodeOperands()
{
  for (size_t i = 0; i < countof(operand_data); i++)
  {
    if (!DecodeOperand(i))
      return false;
  }

  return true;
}

void InstructionDecoder::SetRepeatFlags()
{
  if (rep_prefix == 0)
    return;

  // Only certain instructions can be used with REP prefixes, everything else is ignored
  switch (instruction->operation)
  {
      // Non-string operations
    case Operation_INS:
    case Operation_OUTS:
    case Operation_MOVS:
    case Operation_LODS:
    case Operation_STOS:
    {
      // TODO: How should we handle non-string instructions with REPNE prefix?
      instruction->flags |= InstructionFlag_Rep;
    }
    break;

      // String operations
    case Operation_CMPS:
    case Operation_SCAS:
    {
      instruction->flags |= InstructionFlag_Rep;
      if (rep_prefix == 0xF2)
        instruction->flags |= InstructionFlag_RepNotEqual;
      else
        instruction->flags |= InstructionFlag_RepEqual;
    }
    break;

      // Everything else - we ignore the rep prefix.
    default:
      break;
  }
}

bool InstructionDecoder::DecodeInstruction()
{
  Y_memzero(instruction, sizeof(OldInstruction));

  // Fetch the first byte
  current_byte = FetchByte();

  // Decode prefixes
  DecodePrefixes();

  // Decode the operation itself
  if (!DecodeOperation())
    return false;

  // Decode operands to the operation
  if (!DecodeOperands())
    return false;

  // Fix up the rep prefix if it was present
  SetRepeatFlags();

  // Successful decode if we reached here
  return true;
}

bool DecodeInstruction(OldInstruction* instruction, AddressSize address_size, OperandSize operand_size,
                       InstructionFetchCallback& fetch_callback)
{
  InstructionDecoder decoder(instruction, address_size, operand_size, fetch_callback);
  return decoder.DecodeInstruction();
}

uint16 InstructionFetchCallback::FetchWord()
{
  uint8 b0 = FetchByte();
  uint8 b1 = FetchByte();
  return ZeroExtend16(b0) | (ZeroExtend16(b1) << 8);
}

uint32 InstructionFetchCallback::FetchDWord()
{
  uint16 w0 = FetchWord();
  uint16 w1 = FetchWord();
  return ZeroExtend32(w0) | (ZeroExtend16(w1) << 16);
}

} // namespace CPU_X86