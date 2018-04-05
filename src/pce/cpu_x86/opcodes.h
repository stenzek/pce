// clang-format off
#define Eb OperandSize_8, OperandMode_ModRM_RM, 0
#define Ev OperandSize_Count, OperandMode_ModRM_RM, 0
#define Ew OperandSize_16, OperandMode_ModRM_RM, 0
#define Gb OperandSize_8, OperandMode_ModRM_Reg, 0
#define Gw OperandSize_16, OperandMode_ModRM_Reg, 0
#define Gv OperandSize_Count, OperandMode_ModRM_Reg, 0
#define Sw OperandSize_16, OperandMode_ModRM_SegmentReg, 0
#define Ib OperandSize_8, OperandMode_Immediate, 0
#define Iw OperandSize_16, OperandMode_Immediate, 0
#define Iv OperandSize_Count, OperandMode_Immediate, 0
#define Ib2 OperandSize_8, OperandMode_Immediate2, 0
#define Iw2 OperandSize_16, OperandMode_Immediate2, 0
#define Iv2 OperandSize_Count, OperandMode_Immediate2, 0
#define M OperandSize_Count, OperandMode_ModRM_RM, 0
#define Ap OperandSize_Count, OperandMode_FarAddress, 0
#define Mp OperandSize_Count, OperandMode_ModRM_RM, 0
#define Mw OperandSize_16, OperandMode_ModRM_RM, 0
#define Md OperandSize_32, OperandMode_ModRM_RM, 0
#define Mq OperandSize_64, OperandMode_ModRM_RM, 0
#define Mt OperandSize_80, OperandMode_ModRM_RM, 0
#define Ob OperandSize_8, OperandMode_Memory, 0
#define Ow OperandSize_16, OperandMode_Memory, 0
#define Ov OperandSize_Count, OperandMode_Memory, 0
#define Jb OperandSize_8, OperandMode_Relative, 0
#define Jw OperandSize_16, OperandMode_Relative, 0
#define Jv OperandSize_Count, OperandMode_Relative, 0
#define Cb(n) OperandSize_8, OperandMode_Constant, (n)
#define Cw(n) OperandSize_16, OperandMode_Constant, (n)
#define Cv(n) OperandSize_Count, OperandMode_Constant, (n)
#define Ms OperandSize_16, OperandMode_ModRM_RM, 0
#define Ma OperandSize_16, OperandMode_ModRM_RM, 0
#define Cd OperandSize_32, OperandMode_ModRM_ControlRegister, 0
#define Td OperandSize_32, OperandMode_ModRM_TestRegister, 0
#define Dd OperandSize_32, OperandMode_ModRM_DebugRegister, 0
#define Rd OperandSize_32, OperandMode_ModRM_RM, 0
#define Xb OperandSize_8, OperandMode_RegisterIndirect, Reg32_ESI
#define Xv OperandSize_Count, OperandMode_RegisterIndirect, Reg32_ESI
#define Yb OperandSize_8, OperandMode_RegisterIndirect, Reg32_EDI
#define Yv OperandSize_Count, OperandMode_RegisterIndirect, Reg32_EDI
#define AL OperandSize_8, OperandMode_Register, Reg8_AL
#define AH OperandSize_8, OperandMode_Register, Reg8_AH
#define CL OperandSize_8, OperandMode_Register, Reg8_CL
#define CH OperandSize_8, OperandMode_Register, Reg8_CH
#define DL OperandSize_8, OperandMode_Register, Reg8_DL
#define DH OperandSize_8, OperandMode_Register, Reg8_DH
#define BL OperandSize_8, OperandMode_Register, Reg8_BL
#define BH OperandSize_8, OperandMode_Register, Reg8_BH
#define AX OperandSize_16, OperandMode_Register, Reg16_AX
#define CX OperandSize_16, OperandMode_Register, Reg16_CX
#define DX OperandSize_16, OperandMode_Register, Reg16_DX
#define BX OperandSize_16, OperandMode_Register, Reg16_BX
#define SP OperandSize_16, OperandMode_Register, Reg16_SP
#define BP OperandSize_16, OperandMode_Register, Reg16_BP
#define SI OperandSize_16, OperandMode_Register, Reg16_SI
#define DI OperandSize_16, OperandMode_Register, Reg16_DI
#define EAX OperandSize_32, OperandMode_Register, Reg32_EAX
#define ECX OperandSize_32, OperandMode_Register, Reg32_ECX
#define EDX OperandSize_32, OperandMode_Register, Reg32_EDX
#define EBX OperandSize_32, OperandMode_Register, Reg32_EBX
#define ESP OperandSize_32, OperandMode_Register, Reg32_ESP
#define EBP OperandSize_32, OperandMode_Register, Reg32_EBP
#define ESI OperandSize_32, OperandMode_Register, Reg32_ESI
#define EDI OperandSize_32, OperandMode_Register, Reg32_EDI
#define eAX OperandSize_Count, OperandMode_Register, Reg32_EAX
#define eCX OperandSize_Count, OperandMode_Register, Reg32_ECX
#define eDX OperandSize_Count, OperandMode_Register, Reg32_EDX
#define eBX OperandSize_Count, OperandMode_Register, Reg32_EBX
#define eSP OperandSize_Count, OperandMode_Register, Reg32_ESP
#define eBP OperandSize_Count, OperandMode_Register, Reg32_EBP
#define eSI OperandSize_Count, OperandMode_Register, Reg32_ESI
#define eDI OperandSize_Count, OperandMode_Register, Reg32_EDI
#define CS OperandSize_16, OperandMode_SegmentRegister, Segment_CS
#define DS OperandSize_16, OperandMode_SegmentRegister, Segment_DS
#define ES OperandSize_16, OperandMode_SegmentRegister, Segment_ES
#define SS OperandSize_16, OperandMode_SegmentRegister, Segment_SS
#define FS OperandSize_16, OperandMode_SegmentRegister, Segment_FS
#define GS OperandSize_16, OperandMode_SegmentRegister, Segment_GS
#define ST(n) OperandSize_80, OperandMode_FPRegister, (n)

#define EnumBaseOpcodes() \
MakeTwoOperands(0x00, Operation_ADD, Eb, Gb) \
MakeTwoOperands(0x01, Operation_ADD, Ev, Gv) \
MakeTwoOperands(0x02, Operation_ADD, Gb, Eb) \
MakeTwoOperands(0x03, Operation_ADD, Gv, Ev) \
MakeTwoOperands(0x04, Operation_ADD, AL, Ib) \
MakeTwoOperands(0x05, Operation_ADD, eAX, Iv) \
MakeOneOperand(0x06, Operation_PUSH_Sreg, ES) \
MakeOneOperand(0x07, Operation_POP_Sreg, ES) \
MakeTwoOperands(0x08, Operation_OR, Eb, Gb) \
MakeTwoOperands(0x09, Operation_OR, Ev, Gv) \
MakeTwoOperands(0x0A, Operation_OR, Gb, Eb) \
MakeTwoOperands(0x0B, Operation_OR, Gv, Ev) \
MakeTwoOperands(0x0C, Operation_OR, AL, Ib) \
MakeTwoOperands(0x0D, Operation_OR, eAX, Iv) \
MakeOneOperand(0x0E, Operation_PUSH_Sreg, CS) \
MakeExtension(0x0F, 0f) /* 80286+ */ \
MakeTwoOperands(0x10, Operation_ADC, Eb, Gb) \
MakeTwoOperands(0x11, Operation_ADC, Ev, Gv) \
MakeTwoOperands(0x12, Operation_ADC, Gb, Eb) \
MakeTwoOperands(0x13, Operation_ADC, Gv, Ev) \
MakeTwoOperands(0x14, Operation_ADC, AL, Ib) \
MakeTwoOperands(0x15, Operation_ADC, eAX, Iv) \
MakeOneOperand(0x16, Operation_PUSH_Sreg, SS) \
MakeOneOperand(0x17, Operation_POP_Sreg, SS) \
MakeTwoOperands(0x18, Operation_SBB, Eb, Gb) \
MakeTwoOperands(0x19, Operation_SBB, Ev, Gv) \
MakeTwoOperands(0x1A, Operation_SBB, Gb, Eb) \
MakeTwoOperands(0x1B, Operation_SBB, Gv, Ev) \
MakeTwoOperands(0x1C, Operation_SBB, AL, Ib) \
MakeTwoOperands(0x1D, Operation_SBB, eAX, Iv) \
MakeOneOperand(0x1E, Operation_PUSH_Sreg, DS) \
MakeOneOperand(0x1F, Operation_POP_Sreg, DS) \
MakeTwoOperands(0x20, Operation_AND, Eb, Gb) \
MakeTwoOperands(0x21, Operation_AND, Ev, Gv) \
MakeTwoOperands(0x22, Operation_AND, Gb, Eb) \
MakeTwoOperands(0x23, Operation_AND, Gv, Ev) \
MakeTwoOperands(0x24, Operation_AND, AL, Ib) \
MakeTwoOperands(0x25, Operation_AND, eAX, Iv) \
MakeSegmentPrefix(0x26, Segment_ES) \
MakeNoOperands(0x27, Operation_DAA) \
MakeTwoOperands(0x28, Operation_SUB, Eb, Gb) \
MakeTwoOperands(0x29, Operation_SUB, Ev, Gv) \
MakeTwoOperands(0x2A, Operation_SUB, Gb, Eb) \
MakeTwoOperands(0x2B, Operation_SUB, Gv, Ev) \
MakeTwoOperands(0x2C, Operation_SUB, AL, Ib) \
MakeTwoOperands(0x2D, Operation_SUB, eAX, Iv) \
MakeSegmentPrefix(0x2E, Segment_CS) \
MakeNoOperands(0x2F, Operation_DAS) \
MakeTwoOperands(0x30, Operation_XOR, Eb, Gb) \
MakeTwoOperands(0x31, Operation_XOR, Ev, Gv) \
MakeTwoOperands(0x32, Operation_XOR, Gb, Eb) \
MakeTwoOperands(0x33, Operation_XOR, Gv, Ev) \
MakeTwoOperands(0x34, Operation_XOR, AL, Ib) \
MakeTwoOperands(0x35, Operation_XOR, eAX, Iv) \
MakeSegmentPrefix(0x36, Segment_SS) \
MakeNoOperands(0x37, Operation_AAA) \
MakeTwoOperands(0x38, Operation_CMP, Eb, Gb) \
MakeTwoOperands(0x39, Operation_CMP, Ev, Gv) \
MakeTwoOperands(0x3A, Operation_CMP, Gb, Eb) \
MakeTwoOperands(0x3B, Operation_CMP, Gv, Ev) \
MakeTwoOperands(0x3C, Operation_CMP, AL, Ib) \
MakeTwoOperands(0x3D, Operation_CMP, eAX, Iv) \
MakeSegmentPrefix(0x3E, Segment_DS) \
MakeNoOperands(0x3F, Operation_AAS) \
MakeOneOperand(0x40, Operation_INC, eAX) \
MakeOneOperand(0x41, Operation_INC, eCX) \
MakeOneOperand(0x42, Operation_INC, eDX) \
MakeOneOperand(0x43, Operation_INC, eBX) \
MakeOneOperand(0x44, Operation_INC, eSP) \
MakeOneOperand(0x45, Operation_INC, eBP) \
MakeOneOperand(0x46, Operation_INC, eSI) \
MakeOneOperand(0x47, Operation_INC, eDI) \
MakeOneOperand(0x48, Operation_DEC, eAX) \
MakeOneOperand(0x49, Operation_DEC, eCX) \
MakeOneOperand(0x4A, Operation_DEC, eDX) \
MakeOneOperand(0x4B, Operation_DEC, eBX) \
MakeOneOperand(0x4C, Operation_DEC, eSP) \
MakeOneOperand(0x4D, Operation_DEC, eBP) \
MakeOneOperand(0x4E, Operation_DEC, eSI) \
MakeOneOperand(0x4F, Operation_DEC, eDI) \
MakeOneOperand(0x50, Operation_PUSH, eAX) \
MakeOneOperand(0x51, Operation_PUSH, eCX) \
MakeOneOperand(0x52, Operation_PUSH, eDX) \
MakeOneOperand(0x53, Operation_PUSH, eBX) \
MakeOneOperand(0x54, Operation_PUSH, eSP) \
MakeOneOperand(0x55, Operation_PUSH, eBP) \
MakeOneOperand(0x56, Operation_PUSH, eSI) \
MakeOneOperand(0x57, Operation_PUSH, eDI) \
MakeOneOperand(0x58, Operation_POP, eAX) \
MakeOneOperand(0x59, Operation_POP, eCX) \
MakeOneOperand(0x5A, Operation_POP, eDX) \
MakeOneOperand(0x5B, Operation_POP, eBX) \
MakeOneOperand(0x5C, Operation_POP, eSP) \
MakeOneOperand(0x5D, Operation_POP, eBP) \
MakeOneOperand(0x5E, Operation_POP, eSI) \
MakeOneOperand(0x5F, Operation_POP, eDI) \
MakeNoOperands(0x60, Operation_PUSHA) /* 80286+ */ \
MakeNoOperands(0x61, Operation_POPA) /* 80286+ */ \
MakeTwoOperands(0x62, Operation_BOUND, Gv, Ma) /* 80286+ */ \
MakeTwoOperands(0x63, Operation_ARPL, Ew, Gw) \
MakeSegmentPrefix(0x64, Segment_FS) /* 80386+ */ \
MakeSegmentPrefix(0x65, Segment_GS) /* 80386+ */ \
MakeOperandSizePrefix(0x66) /* 80386+ */\
MakeAddressSizePrefix(0x67) /* 80386+ */\
MakeOneOperand(0x68, Operation_PUSH, Iv) /* 80286+ */ \
MakeThreeOperands(0x69, Operation_IMUL, Gv, Ev, Iv) /* 80286+ */ \
MakeOneOperand(0x6A, Operation_PUSH, Ib) /* 80286+ */ \
MakeThreeOperands(0x6B, Operation_IMUL, Gv, Ev, Ib) /* 80286+ */ \
MakeTwoOperands(0x6C, Operation_INS, Yb, DX) \
MakeTwoOperands(0x6D, Operation_INS, Yv, DX) \
MakeTwoOperands(0x6E, Operation_OUTS, DX, Xb) \
MakeTwoOperands(0x6F, Operation_OUTS, DX, Yv) \
MakeOneOperandCC(0x70, Operation_Jcc, JumpCondition_Overflow, Jb) \
MakeOneOperandCC(0x71, Operation_Jcc, JumpCondition_NotOverflow, Jb) \
MakeOneOperandCC(0x72, Operation_Jcc, JumpCondition_Below, Jb) \
MakeOneOperandCC(0x73, Operation_Jcc, JumpCondition_AboveOrEqual, Jb) \
MakeOneOperandCC(0x74, Operation_Jcc, JumpCondition_Equal, Jb) \
MakeOneOperandCC(0x75, Operation_Jcc, JumpCondition_NotEqual, Jb) \
MakeOneOperandCC(0x76, Operation_Jcc, JumpCondition_BelowOrEqual, Jb) \
MakeOneOperandCC(0x77, Operation_Jcc, JumpCondition_Above, Jb) \
MakeOneOperandCC(0x78, Operation_Jcc, JumpCondition_Sign, Jb) \
MakeOneOperandCC(0x79, Operation_Jcc, JumpCondition_NotSign, Jb) \
MakeOneOperandCC(0x7A, Operation_Jcc, JumpCondition_Parity, Jb) \
MakeOneOperandCC(0x7B, Operation_Jcc, JumpCondition_NotParity, Jb) \
MakeOneOperandCC(0x7C, Operation_Jcc, JumpCondition_Less, Jb) \
MakeOneOperandCC(0x7D, Operation_Jcc, JumpCondition_GreaterOrEqual, Jb) \
MakeOneOperandCC(0x7E, Operation_Jcc, JumpCondition_LessOrEqual, Jb) \
MakeOneOperandCC(0x7F, Operation_Jcc, JumpCondition_Greater, Jb) \
MakeModRMRegExtension(0x80, 80) \
MakeModRMRegExtension(0x81, 81) \
MakeModRMRegExtension(0x82, 82) \
MakeModRMRegExtension(0x83, 83) \
MakeTwoOperands(0x84, Operation_TEST, Gb, Eb) \
MakeTwoOperands(0x85, Operation_TEST, Gv, Ev) \
MakeTwoOperands(0x86, Operation_XCHG, Eb, Gb) \
MakeTwoOperands(0x87, Operation_XCHG, Ev, Gv) \
MakeTwoOperands(0x88, Operation_MOV, Eb, Gb) \
MakeTwoOperands(0x89, Operation_MOV, Ev, Gv) \
MakeTwoOperands(0x8A, Operation_MOV, Gb, Eb) \
MakeTwoOperands(0x8B, Operation_MOV, Gv, Ev) \
MakeTwoOperands(0x8C, Operation_MOV_Sreg, Ew, Sw) \
MakeTwoOperands(0x8D, Operation_LEA, Gv, M) \
MakeTwoOperands(0x8E, Operation_MOV_Sreg, Sw, Ew) \
MakeOneOperand(0x8F, Operation_POP, Ev) \
MakeNop(0x90) \
MakeTwoOperands(0x91, Operation_XCHG, eCX, eAX) \
MakeTwoOperands(0x92, Operation_XCHG, eDX, eAX) \
MakeTwoOperands(0x93, Operation_XCHG, eBX, eAX) \
MakeTwoOperands(0x94, Operation_XCHG, eSP, eAX) \
MakeTwoOperands(0x95, Operation_XCHG, eBP, eAX) \
MakeTwoOperands(0x96, Operation_XCHG, eSI, eAX) \
MakeTwoOperands(0x97, Operation_XCHG, eDI, eAX) \
MakeNoOperands(0x98, Operation_CBW) \
MakeNoOperands(0x99, Operation_CWD) \
MakeOneOperand(0x9A, Operation_CALL_Far, Ap) \
MakeNoOperands(0x9B, Operation_WAIT) \
MakeNoOperands(0x9C, Operation_PUSHF) \
MakeNoOperands(0x9D, Operation_POPF) \
MakeNoOperands(0x9E, Operation_SAHF) \
MakeNoOperands(0x9F, Operation_LAHF) \
MakeTwoOperands(0xA0, Operation_MOV, AL, Ob) \
MakeTwoOperands(0xA1, Operation_MOV, eAX, Ov) \
MakeTwoOperands(0xA2, Operation_MOV, Ob, AL) \
MakeTwoOperands(0xA3, Operation_MOV, Ov, eAX) \
MakeTwoOperands(0xA4, Operation_MOVS, Yb, Xb) \
MakeTwoOperands(0xA5, Operation_MOVS, Yv, Xv) \
MakeTwoOperands(0xA6, Operation_CMPS, Xb, Yb) \
MakeTwoOperands(0xA7, Operation_CMPS, Xv, Yv) \
MakeTwoOperands(0xA8, Operation_TEST, AL, Ib) \
MakeTwoOperands(0xA9, Operation_TEST, eAX, Iv) \
MakeTwoOperands(0xAA, Operation_STOS, Yb, AL) \
MakeTwoOperands(0xAB, Operation_STOS, Yv, eAX) \
MakeTwoOperands(0xAC, Operation_LODS, AL, Xb) \
MakeTwoOperands(0xAD, Operation_LODS, eAX, Xv) \
MakeTwoOperands(0xAE, Operation_SCAS, AL, Xb) \
MakeTwoOperands(0xAF, Operation_SCAS, eAX, Xv) \
MakeTwoOperands(0xB0, Operation_MOV, AL, Ib) \
MakeTwoOperands(0xB1, Operation_MOV, CL, Ib) \
MakeTwoOperands(0xB2, Operation_MOV, DL, Ib) \
MakeTwoOperands(0xB3, Operation_MOV, BL, Ib) \
MakeTwoOperands(0xB4, Operation_MOV, AH, Ib) \
MakeTwoOperands(0xB5, Operation_MOV, CH, Ib) \
MakeTwoOperands(0xB6, Operation_MOV, DH, Ib) \
MakeTwoOperands(0xB7, Operation_MOV, BH, Ib) \
MakeTwoOperands(0xB8, Operation_MOV, eAX, Iv) \
MakeTwoOperands(0xB9, Operation_MOV, eCX, Iv) \
MakeTwoOperands(0xBA, Operation_MOV, eDX, Iv) \
MakeTwoOperands(0xBB, Operation_MOV, eBX, Iv) \
MakeTwoOperands(0xBC, Operation_MOV, eSP, Iv) \
MakeTwoOperands(0xBD, Operation_MOV, eBP, Iv) \
MakeTwoOperands(0xBE, Operation_MOV, eSI, Iv) \
MakeTwoOperands(0xBF, Operation_MOV, eDI, Iv) \
MakeModRMRegExtension(0xC0, c0) /* 80286+ */ \
MakeModRMRegExtension(0xC1, c1) /* 80286+ */ \
MakeOneOperand(0xC2, Operation_RET_Near, Iw) \
MakeNoOperands(0xC3, Operation_RET_Near) \
MakeThreeOperands(0xC4, Operation_LXS, ES, Gv, Mp) \
MakeThreeOperands(0xC5, Operation_LXS, DS, Gv, Mp) \
MakeTwoOperands(0xC6, Operation_MOV, Eb, Ib) \
MakeTwoOperands(0xC7, Operation_MOV, Ev, Iv) \
MakeTwoOperands(0xC8, Operation_ENTER, Iw, Ib2) \
MakeNoOperands(0xC9, Operation_LEAVE) \
MakeOneOperand(0xCA, Operation_RET_Far, Iw) \
MakeNoOperands(0xCB, Operation_RET_Far) \
MakeOneOperand(0xCC, Operation_INT, Cb(3)) \
MakeOneOperand(0xCD, Operation_INT, Ib) \
MakeNoOperands(0xCE, Operation_INTO) \
MakeNoOperands(0xCF, Operation_IRET) \
MakeModRMRegExtension(0xD0, d0) \
MakeModRMRegExtension(0xD1, d1) \
MakeModRMRegExtension(0xD2, d2) \
MakeModRMRegExtension(0xD3, d3) \
MakeOneOperand(0xD4, Operation_AAM, Ib) \
MakeOneOperand(0xD5, Operation_AAD, Ib) \
MakeNoOperands(0xD6, Operation_SALC) \
MakeNoOperands(0xD7, Operation_XLAT) \
MakeX87Extension(0xD8, d8) \
MakeX87Extension(0xD9, d9) \
MakeX87Extension(0xDA, da) \
MakeX87Extension(0xDB, db) \
MakeX87Extension(0xDC, dc) \
MakeX87Extension(0xDD, dd) \
MakeX87Extension(0xDE, de) \
MakeX87Extension(0xDF, df) \
MakeOneOperandCC(0xE0, Operation_LOOP, JumpCondition_NotEqual, Jb) \
MakeOneOperandCC(0xE1, Operation_LOOP, JumpCondition_Equal, Jb) \
MakeOneOperandCC(0xE2, Operation_LOOP, JumpCondition_Always, Jb) \
MakeOneOperandCC(0xE3, Operation_Jcc, JumpCondition_CXZero, Jb) \
MakeTwoOperands(0xE4, Operation_IN, AL, Ib) \
MakeTwoOperands(0xE5, Operation_IN, eAX, Ib) \
MakeTwoOperands(0xE6, Operation_OUT, Ib, AL) \
MakeTwoOperands(0xE7, Operation_OUT, Ib, eAX) \
MakeOneOperand(0xE8, Operation_CALL_Near, Jv) \
MakeOneOperand(0xE9, Operation_JMP_Near, Jv) \
MakeOneOperand(0xEA, Operation_JMP_Far, Ap) \
MakeOneOperand(0xEB, Operation_JMP_Near, Jb) \
MakeTwoOperands(0xEC, Operation_IN, AL, DX) \
MakeTwoOperands(0xED, Operation_IN, eAX, DX) \
MakeTwoOperands(0xEE, Operation_OUT, DX, AL) \
MakeTwoOperands(0xEF, Operation_OUT, DX, eAX) \
MakeLockPrefix(0xF0) \
MakeInvalidOpcode(0xF1) \
MakeRepNEPrefix(0xF2) \
MakeRepPrefix(0xF3) \
MakeNoOperands(0xF4, Operation_HLT) \
MakeNoOperands(0xF5, Operation_CMC) \
MakeModRMRegExtension(0xF6, f6) \
MakeModRMRegExtension(0xF7, f7) \
MakeNoOperands(0xF8, Operation_CLC) \
MakeNoOperands(0xF9, Operation_STC) \
MakeNoOperands(0xFA, Operation_CLI) \
MakeNoOperands(0xFB, Operation_STI) \
MakeNoOperands(0xFC, Operation_CLD) \
MakeNoOperands(0xFD, Operation_STD) \
MakeModRMRegExtension(0xFE, fe) \
MakeModRMRegExtension(0xFF, ff)

#define EnumPrefix0FOpcodes() \
MakeModRMRegExtension(0x00, 0f00) \
MakeModRMRegExtension(0x01, 0f01) \
MakeTwoOperands(0x02, Operation_LAR, Gv, Ew) \
MakeTwoOperands(0x03, Operation_LSL, Gv, Ew) \
MakeInvalidOpcode(0x04) \
MakeNoOperands(0x05, Operation_LOADALL_286) \
MakeNoOperands(0x06, Operation_CLTS) \
MakeInvalidOpcode(0x07) \
MakeInvalidOpcode(0x08) \
MakeNoOperands(0x09, Operation_WBINVD) /* 80486+ */ \
MakeInvalidOpcode(0x0A) \
MakeInvalidOpcode(0x0B) \
MakeInvalidOpcode(0x0C) \
MakeInvalidOpcode(0x0D) \
MakeInvalidOpcode(0x0E) \
MakeInvalidOpcode(0x0F) \
MakeInvalidOpcode(0x10) \
MakeInvalidOpcode(0x11) \
MakeInvalidOpcode(0x12) \
MakeInvalidOpcode(0x13) \
MakeInvalidOpcode(0x14) \
MakeInvalidOpcode(0x15) \
MakeInvalidOpcode(0x16) \
MakeInvalidOpcode(0x17) \
MakeInvalidOpcode(0x18) \
MakeInvalidOpcode(0x19) \
MakeInvalidOpcode(0x1A) \
MakeInvalidOpcode(0x1B) \
MakeInvalidOpcode(0x1C) \
MakeInvalidOpcode(0x1D) \
MakeInvalidOpcode(0x1E) \
MakeInvalidOpcode(0x1F) \
MakeTwoOperands(0x20, Operation_MOV_CR, Rd, Cd) \
MakeTwoOperands(0x21, Operation_MOV_DR, Rd, Dd) \
MakeTwoOperands(0x22, Operation_MOV_CR, Cd, Rd) \
MakeTwoOperands(0x23, Operation_MOV_DR, Dd, Rd) \
MakeTwoOperands(0x24, Operation_MOV_TR, Rd, Td) \
MakeInvalidOpcode(0x25) \
MakeTwoOperands(0x26, Operation_MOV_TR, Td, Rd) \
MakeInvalidOpcode(0x27) \
MakeInvalidOpcode(0x28) \
MakeInvalidOpcode(0x29) \
MakeInvalidOpcode(0x2A) \
MakeInvalidOpcode(0x2B) \
MakeInvalidOpcode(0x2C) \
MakeInvalidOpcode(0x2D) \
MakeInvalidOpcode(0x2E) \
MakeInvalidOpcode(0x2F) \
MakeInvalidOpcode(0x30) \
MakeNoOperands(0x31, Operation_RDTSC) \
MakeInvalidOpcode(0x32) \
MakeInvalidOpcode(0x33) \
MakeInvalidOpcode(0x34) \
MakeInvalidOpcode(0x35) \
MakeInvalidOpcode(0x36) \
MakeInvalidOpcode(0x37) \
MakeInvalidOpcode(0x38) \
MakeInvalidOpcode(0x39) \
MakeInvalidOpcode(0x3A) \
MakeInvalidOpcode(0x3B) \
MakeInvalidOpcode(0x3C) \
MakeInvalidOpcode(0x3D) \
MakeInvalidOpcode(0x3E) \
MakeInvalidOpcode(0x3F) \
MakeTwoOperandsCC(0x40, Operation_CMOVcc, JumpCondition_Overflow, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x41, Operation_CMOVcc, JumpCondition_NotOverflow, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x42, Operation_CMOVcc, JumpCondition_Below, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x43, Operation_CMOVcc, JumpCondition_AboveOrEqual, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x44, Operation_CMOVcc, JumpCondition_Equal, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x45, Operation_CMOVcc, JumpCondition_NotEqual, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x46, Operation_CMOVcc, JumpCondition_BelowOrEqual, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x47, Operation_CMOVcc, JumpCondition_Above, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x48, Operation_CMOVcc, JumpCondition_Sign, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x49, Operation_CMOVcc, JumpCondition_NotSign, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x4A, Operation_CMOVcc, JumpCondition_Parity, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x4B, Operation_CMOVcc, JumpCondition_NotParity, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x4C, Operation_CMOVcc, JumpCondition_Less, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x4D, Operation_CMOVcc, JumpCondition_GreaterOrEqual, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x4E, Operation_CMOVcc, JumpCondition_LessOrEqual, Gv, Ev) /* 80486+ */ \
MakeTwoOperandsCC(0x4F, Operation_CMOVcc, JumpCondition_Greater, Gv, Ev) /* 80486+ */ \
MakeInvalidOpcode(0x50) \
MakeInvalidOpcode(0x51) \
MakeInvalidOpcode(0x52) \
MakeInvalidOpcode(0x53) \
MakeInvalidOpcode(0x54) \
MakeInvalidOpcode(0x55) \
MakeInvalidOpcode(0x56) \
MakeInvalidOpcode(0x57) \
MakeInvalidOpcode(0x58) \
MakeInvalidOpcode(0x59) \
MakeInvalidOpcode(0x5A) \
MakeInvalidOpcode(0x5B) \
MakeInvalidOpcode(0x5C) \
MakeInvalidOpcode(0x5D) \
MakeInvalidOpcode(0x5E) \
MakeInvalidOpcode(0x5F) \
MakeInvalidOpcode(0x60) \
MakeInvalidOpcode(0x61) \
MakeInvalidOpcode(0x62) \
MakeInvalidOpcode(0x63) \
MakeInvalidOpcode(0x64) \
MakeInvalidOpcode(0x65) \
MakeInvalidOpcode(0x66) \
MakeInvalidOpcode(0x67) \
MakeInvalidOpcode(0x68) \
MakeInvalidOpcode(0x69) \
MakeInvalidOpcode(0x6A) \
MakeInvalidOpcode(0x6B) \
MakeInvalidOpcode(0x6C) \
MakeInvalidOpcode(0x6D) \
MakeInvalidOpcode(0x6E) \
MakeInvalidOpcode(0x6F) \
MakeInvalidOpcode(0x70) \
MakeInvalidOpcode(0x71) \
MakeInvalidOpcode(0x72) \
MakeInvalidOpcode(0x73) \
MakeInvalidOpcode(0x74) \
MakeInvalidOpcode(0x75) \
MakeInvalidOpcode(0x76) \
MakeInvalidOpcode(0x77) \
MakeInvalidOpcode(0x78) \
MakeInvalidOpcode(0x79) \
MakeInvalidOpcode(0x7A) \
MakeInvalidOpcode(0x7B) \
MakeInvalidOpcode(0x7C) \
MakeInvalidOpcode(0x7D) \
MakeInvalidOpcode(0x7E) \
MakeInvalidOpcode(0x7F) \
MakeOneOperandCC(0x80, Operation_Jcc, JumpCondition_Overflow, Jv) \
MakeOneOperandCC(0x81, Operation_Jcc, JumpCondition_NotOverflow, Jv) \
MakeOneOperandCC(0x82, Operation_Jcc, JumpCondition_Below, Jv) \
MakeOneOperandCC(0x83, Operation_Jcc, JumpCondition_AboveOrEqual, Jv) \
MakeOneOperandCC(0x84, Operation_Jcc, JumpCondition_Equal, Jv) \
MakeOneOperandCC(0x85, Operation_Jcc, JumpCondition_NotEqual, Jv) \
MakeOneOperandCC(0x86, Operation_Jcc, JumpCondition_BelowOrEqual, Jv) \
MakeOneOperandCC(0x87, Operation_Jcc, JumpCondition_Above, Jv) \
MakeOneOperandCC(0x88, Operation_Jcc, JumpCondition_Sign, Jv) \
MakeOneOperandCC(0x89, Operation_Jcc, JumpCondition_NotSign, Jv) \
MakeOneOperandCC(0x8A, Operation_Jcc, JumpCondition_Parity, Jv) \
MakeOneOperandCC(0x8B, Operation_Jcc, JumpCondition_NotParity, Jv) \
MakeOneOperandCC(0x8C, Operation_Jcc, JumpCondition_Less, Jv) \
MakeOneOperandCC(0x8D, Operation_Jcc, JumpCondition_GreaterOrEqual, Jv) \
MakeOneOperandCC(0x8E, Operation_Jcc, JumpCondition_LessOrEqual, Jv) \
MakeOneOperandCC(0x8F, Operation_Jcc, JumpCondition_Greater, Jv) \
MakeOneOperandCC(0x90, Operation_SETcc, JumpCondition_Overflow, Eb) \
MakeOneOperandCC(0x91, Operation_SETcc, JumpCondition_NotOverflow, Eb) \
MakeOneOperandCC(0x92, Operation_SETcc, JumpCondition_Below, Eb) \
MakeOneOperandCC(0x93, Operation_SETcc, JumpCondition_AboveOrEqual, Eb) \
MakeOneOperandCC(0x94, Operation_SETcc, JumpCondition_Equal, Eb) \
MakeOneOperandCC(0x95, Operation_SETcc, JumpCondition_NotEqual, Eb) \
MakeOneOperandCC(0x96, Operation_SETcc, JumpCondition_BelowOrEqual, Eb) \
MakeOneOperandCC(0x97, Operation_SETcc, JumpCondition_Above, Eb) \
MakeOneOperandCC(0x98, Operation_SETcc, JumpCondition_Sign, Eb) \
MakeOneOperandCC(0x99, Operation_SETcc, JumpCondition_NotSign, Eb) \
MakeOneOperandCC(0x9A, Operation_SETcc, JumpCondition_Parity, Eb) \
MakeOneOperandCC(0x9B, Operation_SETcc, JumpCondition_NotParity, Eb) \
MakeOneOperandCC(0x9C, Operation_SETcc, JumpCondition_Less, Eb) \
MakeOneOperandCC(0x9D, Operation_SETcc, JumpCondition_GreaterOrEqual, Eb) \
MakeOneOperandCC(0x9E, Operation_SETcc, JumpCondition_LessOrEqual, Eb) \
MakeOneOperandCC(0x9F, Operation_SETcc, JumpCondition_Greater, Eb) \
MakeOneOperand(0xA0, Operation_PUSH_Sreg, FS) \
MakeOneOperand(0xA1, Operation_POP_Sreg, FS) \
MakeInvalidOpcode(0xA2) \
MakeTwoOperands(0xA3, Operation_BT, Ev, Gv) \
MakeThreeOperands(0xA4, Operation_SHLD, Ev, Gv, Ib) \
MakeThreeOperands(0xA5, Operation_SHLD, Ev, Gv, CL) \
MakeInvalidOpcode(0xA6) \
MakeInvalidOpcode(0xA7) \
MakeOneOperand(0xA8, Operation_PUSH_Sreg, GS) \
MakeOneOperand(0xA9, Operation_POP_Sreg, GS) \
MakeInvalidOpcode(0xAA) \
MakeTwoOperands(0xAB, Operation_BTS, Ev, Gv) \
MakeThreeOperands(0xAC, Operation_SHRD, Ev, Gv, Ib) \
MakeThreeOperands(0xAD, Operation_SHRD, Ev, Gv, CL) \
MakeInvalidOpcode(0xAE) \
MakeTwoOperands(0xAF, Operation_IMUL, Gv, Ev) \
MakeTwoOperands(0xB0, Operation_CMPXCHG, Eb, Gb) /* 80486+ */ \
MakeTwoOperands(0xB1, Operation_CMPXCHG, Ev, Gv) /* 80486+ */ \
MakeThreeOperands(0xB2, Operation_LXS, SS, Gv, Mp) \
MakeTwoOperands(0xB3, Operation_BTR, Ev, Gv) \
MakeThreeOperands(0xB4, Operation_LXS, FS, Gv, Mp) \
MakeThreeOperands(0xB5, Operation_LXS, GS, Gv, Mp) \
MakeTwoOperands(0xB6, Operation_MOVZX, Gv, Eb) /* 80386+ */ \
MakeTwoOperands(0xB7, Operation_MOVZX, Gv, Ew) /* 80386+ */ \
MakeInvalidOpcode(0xB8) \
MakeInvalidOpcode(0xB9) \
MakeModRMRegExtension(0xBA, 0fba) \
MakeTwoOperands(0xBB, Operation_BTC, Ev, Gv) \
MakeTwoOperands(0xBC, Operation_BSF, Gv, Ev) \
MakeTwoOperands(0xBD, Operation_BSR, Gv, Ev) \
MakeTwoOperands(0xBE, Operation_MOVSX, Gv, Eb) /* 80386+ */ \
MakeTwoOperands(0xBF, Operation_MOVSX, Gv, Ew) /* 80386+ */ \
MakeTwoOperands(0xC0, Operation_XADD, Eb, Gb) /* 80486+ */ \
MakeTwoOperands(0xC1, Operation_XADD, Ev, Gv) /* 80486+ */ \
MakeInvalidOpcode(0xC2) \
MakeInvalidOpcode(0xC3) \
MakeInvalidOpcode(0xC4) \
MakeInvalidOpcode(0xC5) \
MakeInvalidOpcode(0xC6) \
MakeInvalidOpcode(0xC7) \
MakeOneOperand(0xC8, Operation_BSWAP, EAX) \
MakeOneOperand(0xC9, Operation_BSWAP, ECX) \
MakeOneOperand(0xCA, Operation_BSWAP, EDX) \
MakeOneOperand(0xCB, Operation_BSWAP, EBX) \
MakeOneOperand(0xCC, Operation_BSWAP, ESP) \
MakeOneOperand(0xCD, Operation_BSWAP, EBP) \
MakeOneOperand(0xCE, Operation_BSWAP, ESI) \
MakeOneOperand(0xCF, Operation_BSWAP, EDI) \
MakeInvalidOpcode(0xD0) \
MakeInvalidOpcode(0xD1) \
MakeInvalidOpcode(0xD2) \
MakeInvalidOpcode(0xD3) \
MakeInvalidOpcode(0xD4) \
MakeInvalidOpcode(0xD5) \
MakeInvalidOpcode(0xD6) \
MakeInvalidOpcode(0xD7) \
MakeInvalidOpcode(0xD8) \
MakeInvalidOpcode(0xD9) \
MakeInvalidOpcode(0xDA) \
MakeInvalidOpcode(0xDB) \
MakeInvalidOpcode(0xDC) \
MakeInvalidOpcode(0xDD) \
MakeInvalidOpcode(0xDE) \
MakeInvalidOpcode(0xDF) \
MakeInvalidOpcode(0xE0) \
MakeInvalidOpcode(0xE1) \
MakeInvalidOpcode(0xE2) \
MakeInvalidOpcode(0xE3) \
MakeInvalidOpcode(0xE4) \
MakeInvalidOpcode(0xE5) \
MakeInvalidOpcode(0xE6) \
MakeInvalidOpcode(0xE7) \
MakeInvalidOpcode(0xE8) \
MakeInvalidOpcode(0xE9) \
MakeInvalidOpcode(0xEA) \
MakeInvalidOpcode(0xEB) \
MakeInvalidOpcode(0xEC) \
MakeInvalidOpcode(0xED) \
MakeInvalidOpcode(0xEE) \
MakeInvalidOpcode(0xEF) \
MakeInvalidOpcode(0xF0) \
MakeInvalidOpcode(0xF1) \
MakeInvalidOpcode(0xF2) \
MakeInvalidOpcode(0xF3) \
MakeInvalidOpcode(0xF4) \
MakeInvalidOpcode(0xF5) \
MakeInvalidOpcode(0xF6) \
MakeInvalidOpcode(0xF7) \
MakeInvalidOpcode(0xF8) \
MakeInvalidOpcode(0xF9) \
MakeInvalidOpcode(0xFA) \
MakeInvalidOpcode(0xFB) \
MakeInvalidOpcode(0xFC) \
MakeInvalidOpcode(0xFD) \
MakeInvalidOpcode(0xFE) \
MakeInvalidOpcode(0xFF)

#define EnumGrp1Opcodes(op1, op2) \
MakeTwoOperands(0x00, Operation_ADD, op1, op2) \
MakeTwoOperands(0x01, Operation_OR, op1, op2) \
MakeTwoOperands(0x02, Operation_ADC, op1, op2) \
MakeTwoOperands(0x03, Operation_SBB, op1, op2) \
MakeTwoOperands(0x04, Operation_AND, op1, op2) \
MakeTwoOperands(0x05, Operation_SUB, op1, op2) \
MakeTwoOperands(0x06, Operation_XOR, op1, op2) \
MakeTwoOperands(0x07, Operation_CMP, op1, op2)

#define EnumGrp2Opcodes(op1, op2) \
MakeTwoOperands(0x00, Operation_ROL, op1, op2) \
MakeTwoOperands(0x01, Operation_ROR, op1, op2) \
MakeTwoOperands(0x02, Operation_RCL, op1, op2) \
MakeTwoOperands(0x03, Operation_RCR, op1, op2) \
MakeTwoOperands(0x04, Operation_SHL, op1, op2) \
MakeTwoOperands(0x05, Operation_SHR, op1, op2) \
MakeInvalidOpcode(0x06) \
MakeTwoOperands(0x07, Operation_SAR, op1, op2)

#define EnumGrp3aOpcodes(op1) \
MakeTwoOperands(0x00, Operation_TEST, Eb, Ib) \
MakeInvalidOpcode(0x01) \
MakeOneOperand(0x02, Operation_NOT, op1) \
MakeOneOperand(0x03, Operation_NEG, op1) \
MakeOneOperand(0x04, Operation_MUL, op1) \
MakeOneOperand(0x05, Operation_IMUL, op1) \
MakeOneOperand(0x06, Operation_DIV, op1) \
MakeOneOperand(0x07, Operation_IDIV, op1)

#define EnumGrp3bOpcodes(op1) \
MakeTwoOperands(0x00, Operation_TEST, Ev, Iv) \
MakeInvalidOpcode(0x01) \
MakeOneOperand(0x02, Operation_NOT, op1) \
MakeOneOperand(0x03, Operation_NEG, op1) \
MakeOneOperand(0x04, Operation_MUL, op1) \
MakeOneOperand(0x05, Operation_IMUL, op1) \
MakeOneOperand(0x06, Operation_DIV, op1) \
MakeOneOperand(0x07, Operation_IDIV, op1)

#define EnumGrp4Opcodes(op1) \
MakeOneOperand(0x00, Operation_INC, op1) \
MakeOneOperand(0x01, Operation_DEC, op1) \
MakeInvalidOpcode(0x02) \
MakeInvalidOpcode(0x03) \
MakeInvalidOpcode(0x04) \
MakeInvalidOpcode(0x05) \
MakeInvalidOpcode(0x06) \
MakeInvalidOpcode(0x07)

#define EnumGrp5Opcodes(op1) \
MakeOneOperand(0x00, Operation_INC, op1) \
MakeOneOperand(0x01, Operation_DEC, op1) \
MakeOneOperand(0x02, Operation_CALL_Near, op1) \
MakeOneOperand(0x03, Operation_CALL_Far, Mp) \
MakeOneOperand(0x04, Operation_JMP_Near, op1) \
MakeOneOperand(0x05, Operation_JMP_Far, Mp) \
MakeOneOperand(0x06, Operation_PUSH, op1) \
MakeInvalidOpcode(0x07)

#define EnumGrp6Opcodes() \
MakeOneOperand(0x00, Operation_SLDT, Ew) \
MakeOneOperand(0x01, Operation_STR, Ew) \
MakeOneOperand(0x02, Operation_LLDT, Ew) \
MakeOneOperand(0x03, Operation_LTR, Ew) \
MakeOneOperand(0x04, Operation_VERR, Ew) \
MakeOneOperand(0x05, Operation_VERW, Ew) \
MakeInvalidOpcode(0x06) \
MakeInvalidOpcode(0x07)

#define EnumGrp7Opcodes() \
MakeOneOperand(0x00, Operation_SGDT, Ms) \
MakeOneOperand(0x01, Operation_SIDT, Ms) \
MakeOneOperand(0x02, Operation_LGDT, Ms) \
MakeOneOperand(0x03, Operation_LIDT, Ms) \
MakeOneOperand(0x04, Operation_SMSW, Ew) \
MakeInvalidOpcode(0x05) \
MakeOneOperand(0x06, Operation_LMSW, Ew) \
MakeOneOperand(0x07, Operation_INVLPG, Ev) /* 80486+ */

#define EnumGrp8Opcodes(op1, op2) \
MakeInvalidOpcode(0x00) \
MakeInvalidOpcode(0x01) \
MakeInvalidOpcode(0x02) \
MakeInvalidOpcode(0x03) \
MakeTwoOperands(0x04, Operation_BT, op1, op2) \
MakeTwoOperands(0x05, Operation_BTS, op1, op2) \
MakeTwoOperands(0x06, Operation_BTR, op1, op2) \
MakeTwoOperands(0x07, Operation_BTC, op1, op2)

#define EnumX87D8RegOpcodes() \
  MakeTwoOperands(0x00, Operation_FADD, ST(0), Md) \
  MakeTwoOperands(0x01, Operation_FMUL, ST(0), Md) \
  MakeTwoOperands(0x02, Operation_FCOM, ST(0), Md) \
  MakeTwoOperands(0x03, Operation_FCOMP, ST(0), Md) \
  MakeTwoOperands(0x04, Operation_FSUB, ST(0), Md) \
  MakeTwoOperands(0x05, Operation_FSUBR, ST(0), Md) \
  MakeTwoOperands(0x06, Operation_FDIV, ST(0), Md) \
  MakeTwoOperands(0x07, Operation_FDIVR, ST(0), Md)

#define EnumX87D8MemOpcodes() \
  MakeTwoOperands(0x00, Operation_FADD, ST(0), ST(0)) \
  MakeTwoOperands(0x01, Operation_FADD, ST(0), ST(1)) \
  MakeTwoOperands(0x02, Operation_FADD, ST(0), ST(2)) \
  MakeTwoOperands(0x03, Operation_FADD, ST(0), ST(3)) \
  MakeTwoOperands(0x04, Operation_FADD, ST(0), ST(4)) \
  MakeTwoOperands(0x05, Operation_FADD, ST(0), ST(5)) \
  MakeTwoOperands(0x06, Operation_FADD, ST(0), ST(6)) \
  MakeTwoOperands(0x07, Operation_FADD, ST(0), ST(7)) \
  MakeTwoOperands(0x08, Operation_FMUL, ST(0), ST(0)) \
  MakeTwoOperands(0x09, Operation_FMUL, ST(0), ST(1)) \
  MakeTwoOperands(0x0A, Operation_FMUL, ST(0), ST(2)) \
  MakeTwoOperands(0x0B, Operation_FMUL, ST(0), ST(3)) \
  MakeTwoOperands(0x0C, Operation_FMUL, ST(0), ST(4)) \
  MakeTwoOperands(0x0D, Operation_FMUL, ST(0), ST(5)) \
  MakeTwoOperands(0x0E, Operation_FMUL, ST(0), ST(6)) \
  MakeTwoOperands(0x0F, Operation_FMUL, ST(0), ST(7)) \
  MakeTwoOperands(0x10, Operation_FCOM, ST(0), ST(0)) \
  MakeTwoOperands(0x11, Operation_FCOM, ST(0), ST(1)) \
  MakeTwoOperands(0x12, Operation_FCOM, ST(0), ST(2)) \
  MakeTwoOperands(0x13, Operation_FCOM, ST(0), ST(3)) \
  MakeTwoOperands(0x14, Operation_FCOM, ST(0), ST(4)) \
  MakeTwoOperands(0x15, Operation_FCOM, ST(0), ST(5)) \
  MakeTwoOperands(0x16, Operation_FCOM, ST(0), ST(6)) \
  MakeTwoOperands(0x17, Operation_FCOM, ST(0), ST(7)) \
  MakeTwoOperands(0x18, Operation_FCOMP, ST(0), ST(0)) \
  MakeTwoOperands(0x19, Operation_FCOMP, ST(0), ST(1)) \
  MakeTwoOperands(0x1A, Operation_FCOMP, ST(0), ST(2)) \
  MakeTwoOperands(0x1B, Operation_FCOMP, ST(0), ST(3)) \
  MakeTwoOperands(0x1C, Operation_FCOMP, ST(0), ST(4)) \
  MakeTwoOperands(0x1D, Operation_FCOMP, ST(0), ST(5)) \
  MakeTwoOperands(0x1E, Operation_FCOMP, ST(0), ST(6)) \
  MakeTwoOperands(0x1F, Operation_FCOMP, ST(0), ST(7)) \
  MakeTwoOperands(0x20, Operation_FSUB, ST(0), ST(0)) \
  MakeTwoOperands(0x21, Operation_FSUB, ST(0), ST(1)) \
  MakeTwoOperands(0x22, Operation_FSUB, ST(0), ST(2)) \
  MakeTwoOperands(0x23, Operation_FSUB, ST(0), ST(3)) \
  MakeTwoOperands(0x24, Operation_FSUB, ST(0), ST(4)) \
  MakeTwoOperands(0x25, Operation_FSUB, ST(0), ST(5)) \
  MakeTwoOperands(0x26, Operation_FSUB, ST(0), ST(6)) \
  MakeTwoOperands(0x27, Operation_FSUB, ST(0), ST(7)) \
  MakeTwoOperands(0x28, Operation_FSUBR, ST(0), ST(0)) \
  MakeTwoOperands(0x29, Operation_FSUBR, ST(0), ST(1)) \
  MakeTwoOperands(0x2A, Operation_FSUBR, ST(0), ST(2)) \
  MakeTwoOperands(0x2B, Operation_FSUBR, ST(0), ST(3)) \
  MakeTwoOperands(0x2C, Operation_FSUBR, ST(0), ST(4)) \
  MakeTwoOperands(0x2D, Operation_FSUBR, ST(0), ST(5)) \
  MakeTwoOperands(0x2E, Operation_FSUBR, ST(0), ST(6)) \
  MakeTwoOperands(0x2F, Operation_FSUBR, ST(0), ST(7)) \
  MakeTwoOperands(0x30, Operation_FDIV, ST(0), ST(0)) \
  MakeTwoOperands(0x31, Operation_FDIV, ST(0), ST(1)) \
  MakeTwoOperands(0x32, Operation_FDIV, ST(0), ST(2)) \
  MakeTwoOperands(0x33, Operation_FDIV, ST(0), ST(3)) \
  MakeTwoOperands(0x34, Operation_FDIV, ST(0), ST(4)) \
  MakeTwoOperands(0x35, Operation_FDIV, ST(0), ST(5)) \
  MakeTwoOperands(0x36, Operation_FDIV, ST(0), ST(6)) \
  MakeTwoOperands(0x37, Operation_FDIV, ST(0), ST(7)) \
  MakeTwoOperands(0x38, Operation_FDIVR, ST(0), ST(0)) \
  MakeTwoOperands(0x39, Operation_FDIVR, ST(0), ST(1)) \
  MakeTwoOperands(0x3A, Operation_FDIVR, ST(0), ST(2)) \
  MakeTwoOperands(0x3B, Operation_FDIVR, ST(0), ST(3)) \
  MakeTwoOperands(0x3C, Operation_FDIVR, ST(0), ST(4)) \
  MakeTwoOperands(0x3D, Operation_FDIVR, ST(0), ST(5)) \
  MakeTwoOperands(0x3E, Operation_FDIVR, ST(0), ST(6)) \
  MakeTwoOperands(0x3F, Operation_FDIVR, ST(0), ST(7))

#define EnumX87D9RegOpcodes() \
  MakeOneOperand(0x00, Operation_FLD, Md) \
  MakeInvalidOpcode(0x01) \
  MakeOneOperand(0x02, Operation_FST, Md) \
  MakeOneOperand(0x03, Operation_FSTP, Md) \
  MakeOneOperand(0x04, Operation_FLDENV, M) \
  MakeOneOperand(0x05, Operation_FLDCW, Mw) \
  MakeOneOperand(0x06, Operation_FNSTENV, M) \
  MakeOneOperand(0x07, Operation_FNSTCW, Mw)

#define EnumX87D9MemOpcodes() \
  MakeOneOperand(0x00, Operation_FLD, ST(0)) \
  MakeOneOperand(0x01, Operation_FLD, ST(1)) \
  MakeOneOperand(0x02, Operation_FLD, ST(2)) \
  MakeOneOperand(0x03, Operation_FLD, ST(3)) \
  MakeOneOperand(0x04, Operation_FLD, ST(4)) \
  MakeOneOperand(0x05, Operation_FLD, ST(5)) \
  MakeOneOperand(0x06, Operation_FLD, ST(6)) \
  MakeOneOperand(0x07, Operation_FLD, ST(7)) \
  MakeOneOperand(0x08, Operation_FXCH, ST(0)) \
  MakeOneOperand(0x09, Operation_FXCH, ST(1)) \
  MakeOneOperand(0x0A, Operation_FXCH, ST(2)) \
  MakeOneOperand(0x0B, Operation_FXCH, ST(3)) \
  MakeOneOperand(0x0C, Operation_FXCH, ST(4)) \
  MakeOneOperand(0x0D, Operation_FXCH, ST(5)) \
  MakeOneOperand(0x0E, Operation_FXCH, ST(6)) \
  MakeOneOperand(0x0F, Operation_FXCH, ST(7)) \
  MakeNoOperands(0x10, Operation_FNOP) \
  MakeInvalidOpcode(0x11) \
  MakeInvalidOpcode(0x12) \
  MakeInvalidOpcode(0x13) \
  MakeInvalidOpcode(0x14) \
  MakeInvalidOpcode(0x15) \
  MakeInvalidOpcode(0x16) \
  MakeInvalidOpcode(0x17) \
  MakeInvalidOpcode(0x18) \
  MakeInvalidOpcode(0x19) \
  MakeInvalidOpcode(0x1A) \
  MakeInvalidOpcode(0x1B) \
  MakeInvalidOpcode(0x1C) \
  MakeInvalidOpcode(0x1D) \
  MakeInvalidOpcode(0x1E) \
  MakeInvalidOpcode(0x1F) \
  MakeNoOperands(0x20, Operation_FCHS) \
  MakeNoOperands(0x21, Operation_FABS) \
  MakeInvalidOpcode(0x22) \
  MakeInvalidOpcode(0x23) \
  MakeNoOperands(0x24, Operation_FTST) \
  MakeNoOperands(0x25, Operation_FXAM) \
  MakeInvalidOpcode(0x26) \
  MakeInvalidOpcode(0x27) \
  MakeNoOperands(0x28, Operation_FLD1) \
  MakeNoOperands(0x29, Operation_FLDL2E) \
  MakeNoOperands(0x2A, Operation_FLDL2T) \
  MakeNoOperands(0x2B, Operation_FLDPI) \
  MakeNoOperands(0x2C, Operation_FLDLG2) \
  MakeNoOperands(0x2D, Operation_FLDLN2) \
  MakeNoOperands(0x2E, Operation_FLDZ) \
  MakeInvalidOpcode(0x2F) \
  MakeNoOperands(0x30, Operation_F2XM1) \
  MakeNoOperands(0x31, Operation_FYL2X) \
  MakeNoOperands(0x32, Operation_FPTAN) \
  MakeNoOperands(0x33, Operation_FPATAN) \
  MakeNoOperands(0x34, Operation_FXTRACT) \
  MakeNoOperands(0x35, Operation_FPREM1) \
  MakeNoOperands(0x36, Operation_FDECSTP) \
  MakeNoOperands(0x37, Operation_FINCSTP) \
  MakeNoOperands(0x38, Operation_FPREM) \
  MakeNoOperands(0x39, Operation_FYL2XP1) \
  MakeNoOperands(0x3A, Operation_FSQRT) \
  MakeNoOperands(0x3B, Operation_FSINCOS) \
  MakeNoOperands(0x3C, Operation_FRNDINT) \
  MakeNoOperands(0x3D, Operation_FSCALE) \
  MakeNoOperands(0x3E, Operation_FSIN) \
  MakeNoOperands(0x3F, Operation_FCOS)

#define EnumX87DARegOpcodes() \
  MakeTwoOperands(0x00, Operation_FIADD, ST(0), Md) \
  MakeTwoOperands(0x01, Operation_FIMUL, ST(0), Md) \
  MakeTwoOperands(0x02, Operation_FICOM, ST(0), Md) \
  MakeTwoOperands(0x03, Operation_FICOMP, ST(0), Md) \
  MakeTwoOperands(0x04, Operation_FISUB, ST(0), Md) \
  MakeTwoOperands(0x05, Operation_FISUBR, ST(0), Md) \
  MakeTwoOperands(0x06, Operation_FIDIV, ST(0), Md) \
  MakeTwoOperands(0x07, Operation_FIDIVR, ST(0), Md)

#define EnumX87DAMemOpcodes() \
  MakeInvalidOpcode(0x00) \
  MakeInvalidOpcode(0x01) \
  MakeInvalidOpcode(0x02) \
  MakeInvalidOpcode(0x03) \
  MakeInvalidOpcode(0x04) \
  MakeInvalidOpcode(0x05) \
  MakeInvalidOpcode(0x06) \
  MakeInvalidOpcode(0x07) \
  MakeInvalidOpcode(0x08) \
  MakeInvalidOpcode(0x09) \
  MakeInvalidOpcode(0x0A) \
  MakeInvalidOpcode(0x0B) \
  MakeInvalidOpcode(0x0C) \
  MakeInvalidOpcode(0x0D) \
  MakeInvalidOpcode(0x0E) \
  MakeInvalidOpcode(0x0F) \
  MakeInvalidOpcode(0x10) \
  MakeInvalidOpcode(0x11) \
  MakeInvalidOpcode(0x12) \
  MakeInvalidOpcode(0x13) \
  MakeInvalidOpcode(0x14) \
  MakeInvalidOpcode(0x15) \
  MakeInvalidOpcode(0x16) \
  MakeInvalidOpcode(0x17) \
  MakeInvalidOpcode(0x18) \
  MakeInvalidOpcode(0x19) \
  MakeInvalidOpcode(0x1A) \
  MakeInvalidOpcode(0x1B) \
  MakeInvalidOpcode(0x1C) \
  MakeInvalidOpcode(0x1D) \
  MakeInvalidOpcode(0x1E) \
  MakeInvalidOpcode(0x1F) \
  MakeInvalidOpcode(0x20) \
  MakeInvalidOpcode(0x21) \
  MakeInvalidOpcode(0x22) \
  MakeInvalidOpcode(0x23) \
  MakeInvalidOpcode(0x24) \
  MakeInvalidOpcode(0x25) \
  MakeInvalidOpcode(0x26) \
  MakeInvalidOpcode(0x27) \
  MakeInvalidOpcode(0x28) \
  MakeTwoOperands(0x29, Operation_FUCOMPP, ST(0), ST(1)) \
  MakeInvalidOpcode(0x2A) \
  MakeInvalidOpcode(0x2B) \
  MakeInvalidOpcode(0x2C) \
  MakeInvalidOpcode(0x2D) \
  MakeInvalidOpcode(0x2E) \
  MakeInvalidOpcode(0x2F) \
  MakeInvalidOpcode(0x30) \
  MakeInvalidOpcode(0x31) \
  MakeInvalidOpcode(0x32) \
  MakeInvalidOpcode(0x33) \
  MakeInvalidOpcode(0x34) \
  MakeInvalidOpcode(0x35) \
  MakeInvalidOpcode(0x36) \
  MakeInvalidOpcode(0x37) \
  MakeInvalidOpcode(0x38) \
  MakeInvalidOpcode(0x39) \
  MakeInvalidOpcode(0x3A) \
  MakeInvalidOpcode(0x3B) \
  MakeInvalidOpcode(0x3C) \
  MakeInvalidOpcode(0x3D) \
  MakeInvalidOpcode(0x3E) \
  MakeInvalidOpcode(0x3F)

#define EnumX87DBRegOpcodes() \
  MakeOneOperand(0x00, Operation_FILD, Md) \
  MakeInvalidOpcode(0x01) \
  MakeOneOperand(0x02, Operation_FIST, Md) \
  MakeOneOperand(0x03, Operation_FISTP, Md) \
  MakeInvalidOpcode(0x04) \
  MakeOneOperand(0x05, Operation_FLD, Mt) \
  MakeInvalidOpcode(0x06) \
  MakeOneOperand(0x07, Operation_FSTP, Mt)

#define EnumX87DBMemOpcodes() \
MakeInvalidOpcode(0x00) \
  MakeInvalidOpcode(0x01) \
  MakeInvalidOpcode(0x02) \
  MakeInvalidOpcode(0x03) \
  MakeInvalidOpcode(0x04) \
  MakeInvalidOpcode(0x05) \
  MakeInvalidOpcode(0x06) \
  MakeInvalidOpcode(0x07) \
  MakeInvalidOpcode(0x08) \
  MakeInvalidOpcode(0x09) \
  MakeInvalidOpcode(0x0A) \
  MakeInvalidOpcode(0x0B) \
  MakeInvalidOpcode(0x0C) \
  MakeInvalidOpcode(0x0D) \
  MakeInvalidOpcode(0x0E) \
  MakeInvalidOpcode(0x0F) \
  MakeInvalidOpcode(0x10) \
  MakeInvalidOpcode(0x11) \
  MakeInvalidOpcode(0x12) \
  MakeInvalidOpcode(0x13) \
  MakeInvalidOpcode(0x14) \
  MakeInvalidOpcode(0x15) \
  MakeInvalidOpcode(0x16) \
  MakeInvalidOpcode(0x17) \
  MakeInvalidOpcode(0x18) \
  MakeInvalidOpcode(0x19) \
  MakeInvalidOpcode(0x1A) \
  MakeInvalidOpcode(0x1B) \
  MakeInvalidOpcode(0x1C) \
  MakeInvalidOpcode(0x1D) \
  MakeInvalidOpcode(0x1E) \
  MakeInvalidOpcode(0x1F) \
  MakeNoOperands(0x20, Operation_FENI) \
  MakeNoOperands(0x21, Operation_FDISI) \
  MakeNoOperands(0x22, Operation_FNCLEX) \
  MakeNoOperands(0x23, Operation_FNINIT) \
  MakeNoOperands(0x24, Operation_FSETPM) \
  MakeInvalidOpcode(0x25) \
  MakeInvalidOpcode(0x26) \
  MakeInvalidOpcode(0x27) \
  MakeInvalidOpcode(0x28) \
  MakeInvalidOpcode(0x29) \
  MakeInvalidOpcode(0x2A) \
  MakeInvalidOpcode(0x2B) \
  MakeInvalidOpcode(0x2C) \
  MakeInvalidOpcode(0x2D) \
  MakeInvalidOpcode(0x2E) \
  MakeInvalidOpcode(0x2F) \
  MakeInvalidOpcode(0x30) \
  MakeInvalidOpcode(0x31) \
  MakeInvalidOpcode(0x32) \
  MakeInvalidOpcode(0x33) \
  MakeInvalidOpcode(0x34) \
  MakeInvalidOpcode(0x35) \
  MakeInvalidOpcode(0x36) \
  MakeInvalidOpcode(0x37) \
  MakeInvalidOpcode(0x38) \
  MakeInvalidOpcode(0x39) \
  MakeInvalidOpcode(0x3A) \
  MakeInvalidOpcode(0x3B) \
  MakeInvalidOpcode(0x3C) \
  MakeInvalidOpcode(0x3D) \
  MakeInvalidOpcode(0x3E) \
  MakeInvalidOpcode(0x3F)

#define EnumX87DCRegOpcodes() \
  MakeTwoOperands(0x00, Operation_FADD, ST(0), Mq) \
  MakeTwoOperands(0x01, Operation_FMUL, ST(0), Mq) \
  MakeTwoOperands(0x02, Operation_FCOM, ST(0), Mq) \
  MakeTwoOperands(0x03, Operation_FCOMP, ST(0), Mq) \
  MakeTwoOperands(0x04, Operation_FSUB, ST(0), Mq) \
  MakeTwoOperands(0x05, Operation_FSUBR, ST(0), Mq) \
  MakeTwoOperands(0x06, Operation_FDIV, ST(0), Mq) \
  MakeTwoOperands(0x07, Operation_FDIVR, ST(0), Mq)

#define EnumX87DCMemOpcodes() \
  MakeTwoOperands(0x00, Operation_FADD, ST(0), ST(0)) \
  MakeTwoOperands(0x01, Operation_FADD, ST(1), ST(0)) \
  MakeTwoOperands(0x02, Operation_FADD, ST(2), ST(0)) \
  MakeTwoOperands(0x03, Operation_FADD, ST(3), ST(0)) \
  MakeTwoOperands(0x04, Operation_FADD, ST(4), ST(0)) \
  MakeTwoOperands(0x05, Operation_FADD, ST(5), ST(0)) \
  MakeTwoOperands(0x06, Operation_FADD, ST(6), ST(0)) \
  MakeTwoOperands(0x07, Operation_FADD, ST(7), ST(0)) \
  MakeTwoOperands(0x08, Operation_FMUL, ST(0), ST(0)) \
  MakeTwoOperands(0x09, Operation_FMUL, ST(1), ST(0)) \
  MakeTwoOperands(0x0A, Operation_FMUL, ST(2), ST(0)) \
  MakeTwoOperands(0x0B, Operation_FMUL, ST(3), ST(0)) \
  MakeTwoOperands(0x0C, Operation_FMUL, ST(4), ST(0)) \
  MakeTwoOperands(0x0D, Operation_FMUL, ST(5), ST(0)) \
  MakeTwoOperands(0x0E, Operation_FMUL, ST(6), ST(0)) \
  MakeTwoOperands(0x0F, Operation_FMUL, ST(7), ST(0)) \
  MakeTwoOperands(0x10, Operation_FCOM, ST(0), ST(0)) \
  MakeTwoOperands(0x11, Operation_FCOM, ST(1), ST(0)) \
  MakeTwoOperands(0x12, Operation_FCOM, ST(2), ST(0)) \
  MakeTwoOperands(0x13, Operation_FCOM, ST(3), ST(0)) \
  MakeTwoOperands(0x14, Operation_FCOM, ST(4), ST(0)) \
  MakeTwoOperands(0x15, Operation_FCOM, ST(5), ST(0)) \
  MakeTwoOperands(0x16, Operation_FCOM, ST(6), ST(0)) \
  MakeTwoOperands(0x17, Operation_FCOM, ST(7), ST(0)) \
  MakeTwoOperands(0x18, Operation_FCOMP, ST(0), ST(0)) \
  MakeTwoOperands(0x19, Operation_FCOMP, ST(1), ST(0)) \
  MakeTwoOperands(0x1A, Operation_FCOMP, ST(2), ST(0)) \
  MakeTwoOperands(0x1B, Operation_FCOMP, ST(3), ST(0)) \
  MakeTwoOperands(0x1C, Operation_FCOMP, ST(4), ST(0)) \
  MakeTwoOperands(0x1D, Operation_FCOMP, ST(5), ST(0)) \
  MakeTwoOperands(0x1E, Operation_FCOMP, ST(6), ST(0)) \
  MakeTwoOperands(0x1F, Operation_FCOMP, ST(7), ST(0)) \
  MakeTwoOperands(0x20, Operation_FSUB, ST(0), ST(0)) \
  MakeTwoOperands(0x21, Operation_FSUB, ST(1), ST(0)) \
  MakeTwoOperands(0x22, Operation_FSUB, ST(2), ST(0)) \
  MakeTwoOperands(0x23, Operation_FSUB, ST(3), ST(0)) \
  MakeTwoOperands(0x24, Operation_FSUB, ST(4), ST(0)) \
  MakeTwoOperands(0x25, Operation_FSUB, ST(5), ST(0)) \
  MakeTwoOperands(0x26, Operation_FSUB, ST(6), ST(0)) \
  MakeTwoOperands(0x27, Operation_FSUB, ST(7), ST(0)) \
  MakeTwoOperands(0x28, Operation_FSUBR, ST(0), ST(0)) \
  MakeTwoOperands(0x29, Operation_FSUBR, ST(1), ST(0)) \
  MakeTwoOperands(0x2A, Operation_FSUBR, ST(2), ST(0)) \
  MakeTwoOperands(0x2B, Operation_FSUBR, ST(3), ST(0)) \
  MakeTwoOperands(0x2C, Operation_FSUBR, ST(4), ST(0)) \
  MakeTwoOperands(0x2D, Operation_FSUBR, ST(5), ST(0)) \
  MakeTwoOperands(0x2E, Operation_FSUBR, ST(6), ST(0)) \
  MakeTwoOperands(0x2F, Operation_FSUBR, ST(7), ST(0)) \
  MakeTwoOperands(0x30, Operation_FDIV, ST(0), ST(0)) \
  MakeTwoOperands(0x31, Operation_FDIV, ST(1), ST(0)) \
  MakeTwoOperands(0x32, Operation_FDIV, ST(2), ST(0)) \
  MakeTwoOperands(0x33, Operation_FDIV, ST(3), ST(0)) \
  MakeTwoOperands(0x34, Operation_FDIV, ST(4), ST(0)) \
  MakeTwoOperands(0x35, Operation_FDIV, ST(5), ST(0)) \
  MakeTwoOperands(0x36, Operation_FDIV, ST(6), ST(0)) \
  MakeTwoOperands(0x37, Operation_FDIV, ST(7), ST(0)) \
  MakeTwoOperands(0x38, Operation_FDIVR, ST(0), ST(0)) \
  MakeTwoOperands(0x39, Operation_FDIVR, ST(1), ST(0)) \
  MakeTwoOperands(0x3A, Operation_FDIVR, ST(2), ST(0)) \
  MakeTwoOperands(0x3B, Operation_FDIVR, ST(3), ST(0)) \
  MakeTwoOperands(0x3C, Operation_FDIVR, ST(4), ST(0)) \
  MakeTwoOperands(0x3D, Operation_FDIVR, ST(5), ST(0)) \
  MakeTwoOperands(0x3E, Operation_FDIVR, ST(6), ST(0)) \
  MakeTwoOperands(0x3F, Operation_FDIVR, ST(7), ST(0))

#define EnumX87DDRegOpcodes() \
  MakeOneOperand(0x00, Operation_FLD, Mq) \
  MakeInvalidOpcode(0x01) \
  MakeOneOperand(0x02, Operation_FST, Mq) \
  MakeOneOperand(0x03, Operation_FSTP, Mq) \
  MakeOneOperand(0x04, Operation_FRSTOR, M) \
  MakeInvalidOpcode(0x05) \
  MakeOneOperand(0x06, Operation_FNSAVE, M) \
  MakeOneOperand(0x07, Operation_FNSTSW, Mw)

#define EnumX87DDMemOpcodes() \
  MakeOneOperand(0x00, Operation_FFREE, ST(0)) \
  MakeOneOperand(0x01, Operation_FFREE, ST(1)) \
  MakeOneOperand(0x02, Operation_FFREE, ST(2)) \
  MakeOneOperand(0x03, Operation_FFREE, ST(3)) \
  MakeOneOperand(0x04, Operation_FFREE, ST(4)) \
  MakeOneOperand(0x05, Operation_FFREE, ST(5)) \
  MakeOneOperand(0x06, Operation_FFREE, ST(6)) \
  MakeOneOperand(0x07, Operation_FFREE, ST(7)) \
  MakeInvalidOpcode(0x08) \
  MakeInvalidOpcode(0x09) \
  MakeInvalidOpcode(0x0A) \
  MakeInvalidOpcode(0x0B) \
  MakeInvalidOpcode(0x0C) \
  MakeInvalidOpcode(0x0D) \
  MakeInvalidOpcode(0x0E) \
  MakeInvalidOpcode(0x0F) \
  MakeOneOperand(0x10, Operation_FST, ST(0)) \
  MakeOneOperand(0x11, Operation_FST, ST(1)) \
  MakeOneOperand(0x12, Operation_FST, ST(2)) \
  MakeOneOperand(0x13, Operation_FST, ST(3)) \
  MakeOneOperand(0x14, Operation_FST, ST(4)) \
  MakeOneOperand(0x15, Operation_FST, ST(5)) \
  MakeOneOperand(0x16, Operation_FST, ST(6)) \
  MakeOneOperand(0x17, Operation_FST, ST(7)) \
  MakeOneOperand(0x18, Operation_FSTP, ST(0)) \
  MakeOneOperand(0x19, Operation_FSTP, ST(1)) \
  MakeOneOperand(0x1A, Operation_FSTP, ST(2)) \
  MakeOneOperand(0x1B, Operation_FSTP, ST(3)) \
  MakeOneOperand(0x1C, Operation_FSTP, ST(4)) \
  MakeOneOperand(0x1D, Operation_FSTP, ST(5)) \
  MakeOneOperand(0x1E, Operation_FSTP, ST(6)) \
  MakeOneOperand(0x1F, Operation_FSTP, ST(7)) \
  MakeTwoOperands(0x20, Operation_FUCOM, ST(0), ST(0)) \
  MakeTwoOperands(0x21, Operation_FUCOM, ST(0), ST(1)) \
  MakeTwoOperands(0x22, Operation_FUCOM, ST(0), ST(2)) \
  MakeTwoOperands(0x23, Operation_FUCOM, ST(0), ST(3)) \
  MakeTwoOperands(0x24, Operation_FUCOM, ST(0), ST(4)) \
  MakeTwoOperands(0x25, Operation_FUCOM, ST(0), ST(5)) \
  MakeTwoOperands(0x26, Operation_FUCOM, ST(0), ST(6)) \
  MakeTwoOperands(0x27, Operation_FUCOM, ST(0), ST(7)) \
  MakeTwoOperands(0x28, Operation_FUCOMP, ST(0), ST(0)) \
  MakeTwoOperands(0x29, Operation_FUCOMP, ST(0), ST(1)) \
  MakeTwoOperands(0x2A, Operation_FUCOMP, ST(0), ST(2)) \
  MakeTwoOperands(0x2B, Operation_FUCOMP, ST(0), ST(3)) \
  MakeTwoOperands(0x2C, Operation_FUCOMP, ST(0), ST(4)) \
  MakeTwoOperands(0x2D, Operation_FUCOMP, ST(0), ST(5)) \
  MakeTwoOperands(0x2E, Operation_FUCOMP, ST(0), ST(6)) \
  MakeTwoOperands(0x2F, Operation_FUCOMP, ST(0), ST(7)) \
  MakeInvalidOpcode(0x30) \
  MakeInvalidOpcode(0x31) \
  MakeInvalidOpcode(0x32) \
  MakeInvalidOpcode(0x33) \
  MakeInvalidOpcode(0x34) \
  MakeInvalidOpcode(0x35) \
  MakeInvalidOpcode(0x36) \
  MakeInvalidOpcode(0x37) \
  MakeInvalidOpcode(0x38) \
  MakeInvalidOpcode(0x39) \
  MakeInvalidOpcode(0x3A) \
  MakeInvalidOpcode(0x3B) \
  MakeInvalidOpcode(0x3C) \
  MakeInvalidOpcode(0x3D) \
  MakeInvalidOpcode(0x3E) \
  MakeInvalidOpcode(0x3F)

#define EnumX87DERegOpcodes() \
  MakeTwoOperands(0x00, Operation_FIADD, ST(0), Md) \
  MakeTwoOperands(0x01, Operation_FIMUL, ST(0), Md) \
  MakeTwoOperands(0x02, Operation_FICOM, ST(0), Md) \
  MakeTwoOperands(0x03, Operation_FICOMP, ST(0), Md) \
  MakeTwoOperands(0x04, Operation_FISUB, ST(0), Md) \
  MakeTwoOperands(0x05, Operation_FISUBR, ST(0), Md) \
  MakeTwoOperands(0x06, Operation_FIDIV, ST(0), Md) \
  MakeTwoOperands(0x07, Operation_FIDIVR, ST(0), Md)

#define EnumX87DEMemOpcodes() \
  MakeTwoOperands(0x00, Operation_FADDP, ST(0), ST(0)) \
  MakeTwoOperands(0x01, Operation_FADDP, ST(1), ST(0)) \
  MakeTwoOperands(0x02, Operation_FADDP, ST(2), ST(0)) \
  MakeTwoOperands(0x03, Operation_FADDP, ST(3), ST(0)) \
  MakeTwoOperands(0x04, Operation_FADDP, ST(4), ST(0)) \
  MakeTwoOperands(0x05, Operation_FADDP, ST(5), ST(0)) \
  MakeTwoOperands(0x06, Operation_FADDP, ST(6), ST(0)) \
  MakeTwoOperands(0x07, Operation_FADDP, ST(7), ST(0)) \
  MakeTwoOperands(0x08, Operation_FMULP, ST(0), ST(0)) \
  MakeTwoOperands(0x09, Operation_FMULP, ST(1), ST(0)) \
  MakeTwoOperands(0x0A, Operation_FMULP, ST(2), ST(0)) \
  MakeTwoOperands(0x0B, Operation_FMULP, ST(3), ST(0)) \
  MakeTwoOperands(0x0C, Operation_FMULP, ST(4), ST(0)) \
  MakeTwoOperands(0x0D, Operation_FMULP, ST(5), ST(0)) \
  MakeTwoOperands(0x0E, Operation_FMULP, ST(6), ST(0)) \
  MakeTwoOperands(0x0F, Operation_FMULP, ST(7), ST(0)) \
  MakeInvalidOpcode(0x10) \
  MakeInvalidOpcode(0x11) \
  MakeInvalidOpcode(0x12) \
  MakeInvalidOpcode(0x13) \
  MakeInvalidOpcode(0x14) \
  MakeInvalidOpcode(0x15) \
  MakeInvalidOpcode(0x16) \
  MakeInvalidOpcode(0x17) \
  MakeInvalidOpcode(0x18) \
  MakeTwoOperands(0x19, Operation_FCOMPP, ST(0), ST(1)) \
  MakeInvalidOpcode(0x1A) \
  MakeInvalidOpcode(0x1B) \
  MakeInvalidOpcode(0x1C) \
  MakeInvalidOpcode(0x1D) \
  MakeInvalidOpcode(0x1E) \
  MakeInvalidOpcode(0x1F) \
  MakeTwoOperands(0x20, Operation_FSUBRP, ST(0), ST(0)) \
  MakeTwoOperands(0x21, Operation_FSUBRP, ST(1), ST(0)) \
  MakeTwoOperands(0x22, Operation_FSUBRP, ST(2), ST(0)) \
  MakeTwoOperands(0x23, Operation_FSUBRP, ST(3), ST(0)) \
  MakeTwoOperands(0x24, Operation_FSUBRP, ST(4), ST(0)) \
  MakeTwoOperands(0x25, Operation_FSUBRP, ST(5), ST(0)) \
  MakeTwoOperands(0x26, Operation_FSUBRP, ST(6), ST(0)) \
  MakeTwoOperands(0x27, Operation_FSUBRP, ST(7), ST(0)) \
  MakeTwoOperands(0x28, Operation_FSUBP, ST(0), ST(0)) \
  MakeTwoOperands(0x29, Operation_FSUBP, ST(1), ST(0)) \
  MakeTwoOperands(0x2A, Operation_FSUBP, ST(2), ST(0)) \
  MakeTwoOperands(0x2B, Operation_FSUBP, ST(3), ST(0)) \
  MakeTwoOperands(0x2C, Operation_FSUBP, ST(4), ST(0)) \
  MakeTwoOperands(0x2D, Operation_FSUBP, ST(5), ST(0)) \
  MakeTwoOperands(0x2E, Operation_FSUBP, ST(6), ST(0)) \
  MakeTwoOperands(0x2F, Operation_FSUBP, ST(7), ST(0)) \
  MakeTwoOperands(0x30, Operation_FDIVRP, ST(0), ST(0)) \
  MakeTwoOperands(0x31, Operation_FDIVRP, ST(1), ST(0)) \
  MakeTwoOperands(0x32, Operation_FDIVRP, ST(2), ST(0)) \
  MakeTwoOperands(0x33, Operation_FDIVRP, ST(3), ST(0)) \
  MakeTwoOperands(0x34, Operation_FDIVRP, ST(4), ST(0)) \
  MakeTwoOperands(0x35, Operation_FDIVRP, ST(5), ST(0)) \
  MakeTwoOperands(0x36, Operation_FDIVRP, ST(6), ST(0)) \
  MakeTwoOperands(0x37, Operation_FDIVRP, ST(7), ST(0)) \
  MakeTwoOperands(0x38, Operation_FDIVP, ST(0), ST(0)) \
  MakeTwoOperands(0x39, Operation_FDIVP, ST(1), ST(0)) \
  MakeTwoOperands(0x3A, Operation_FDIVP, ST(2), ST(0)) \
  MakeTwoOperands(0x3B, Operation_FDIVP, ST(3), ST(0)) \
  MakeTwoOperands(0x3C, Operation_FDIVP, ST(4), ST(0)) \
  MakeTwoOperands(0x3D, Operation_FDIVP, ST(5), ST(0)) \
  MakeTwoOperands(0x3E, Operation_FDIVP, ST(6), ST(0)) \
  MakeTwoOperands(0x3F, Operation_FDIVP, ST(7), ST(0))

#define EnumX87DFRegOpcodes() \
  MakeOneOperand(0x00, Operation_FILD, Md) \
  MakeInvalidOpcode(0x01) \
  MakeOneOperand(0x02, Operation_FIST, Md) \
  MakeOneOperand(0x03, Operation_FISTP, Md) \
  MakeOneOperand(0x04, Operation_FBLD, Mt) \
  MakeOneOperand(0x05, Operation_FILD, Mq) \
  MakeOneOperand(0x06, Operation_FBSTP, Mt) \
  MakeOneOperand(0x07, Operation_FISTP, Mq)

#define EnumX87DFMemOpcodes() \
  MakeInvalidOpcode(0x00) \
  MakeInvalidOpcode(0x01) \
  MakeInvalidOpcode(0x02) \
  MakeInvalidOpcode(0x03) \
  MakeInvalidOpcode(0x04) \
  MakeInvalidOpcode(0x05) \
  MakeInvalidOpcode(0x06) \
  MakeInvalidOpcode(0x07) \
  MakeInvalidOpcode(0x08) \
  MakeInvalidOpcode(0x09) \
  MakeInvalidOpcode(0x0A) \
  MakeInvalidOpcode(0x0B) \
  MakeInvalidOpcode(0x0C) \
  MakeInvalidOpcode(0x0D) \
  MakeInvalidOpcode(0x0E) \
  MakeInvalidOpcode(0x0F) \
  MakeInvalidOpcode(0x10) \
  MakeInvalidOpcode(0x11) \
  MakeInvalidOpcode(0x12) \
  MakeInvalidOpcode(0x13) \
  MakeInvalidOpcode(0x14) \
  MakeInvalidOpcode(0x15) \
  MakeInvalidOpcode(0x16) \
  MakeInvalidOpcode(0x17) \
  MakeInvalidOpcode(0x18) \
  MakeInvalidOpcode(0x19) \
  MakeInvalidOpcode(0x1A) \
  MakeInvalidOpcode(0x1B) \
  MakeInvalidOpcode(0x1C) \
  MakeInvalidOpcode(0x1D) \
  MakeInvalidOpcode(0x1E) \
  MakeInvalidOpcode(0x1F) \
  MakeOneOperand(0x20, Operation_FNSTSW, AX) \
  MakeInvalidOpcode(0x21) \
  MakeInvalidOpcode(0x22) \
  MakeInvalidOpcode(0x23) \
  MakeInvalidOpcode(0x24) \
  MakeInvalidOpcode(0x25) \
  MakeInvalidOpcode(0x26) \
  MakeInvalidOpcode(0x27) \
  MakeInvalidOpcode(0x28) \
  MakeInvalidOpcode(0x29) \
  MakeInvalidOpcode(0x2A) \
  MakeInvalidOpcode(0x2B) \
  MakeInvalidOpcode(0x2C) \
  MakeInvalidOpcode(0x2D) \
  MakeInvalidOpcode(0x2E) \
  MakeInvalidOpcode(0x2F) \
  MakeInvalidOpcode(0x30) \
  MakeInvalidOpcode(0x31) \
  MakeInvalidOpcode(0x32) \
  MakeInvalidOpcode(0x33) \
  MakeInvalidOpcode(0x34) \
  MakeInvalidOpcode(0x35) \
  MakeInvalidOpcode(0x36) \
  MakeInvalidOpcode(0x37) \
  MakeInvalidOpcode(0x38) \
  MakeInvalidOpcode(0x39) \
  MakeInvalidOpcode(0x3A) \
  MakeInvalidOpcode(0x3B) \
  MakeInvalidOpcode(0x3C) \
  MakeInvalidOpcode(0x3D) \
  MakeInvalidOpcode(0x3E) \
  MakeInvalidOpcode(0x3F)

// clang-format on
