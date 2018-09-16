// clang-format off
#define Eb OperandSize_8, OperandMode_ModRM_RM, 0
#define Ew OperandSize_16, OperandMode_ModRM_RM, 0
#define Gb OperandSize_8, OperandMode_ModRM_Reg, 0
#define Gw OperandSize_16, OperandMode_ModRM_Reg, 0
#define Sw OperandSize_16, OperandMode_ModRM_SegmentReg, 0
#define Ib OperandSize_8, OperandMode_Immediate, 0
#define Iw OperandSize_16, OperandMode_Immediate, 0
#define Ib2 OperandSize_8, OperandMode_Immediate2, 0
#define Iw2 OperandSize_16, OperandMode_Immediate2, 0
#define M OperandSize_Count, OperandMode_ModRM_RM, 0
#define Ap OperandSize_Count, OperandMode_FarAddress, 0
#define Mp OperandSize_Count, OperandMode_ModRM_RM, 0
#define Mw OperandSize_16, OperandMode_ModRM_RM, 0
#define Md OperandSize_32, OperandMode_ModRM_RM, 0
#define Mq OperandSize_64, OperandMode_ModRM_RM, 0
#define Mt OperandSize_80, OperandMode_ModRM_RM, 0
#define Ob OperandSize_8, OperandMode_Memory, 0
#define Ow OperandSize_16, OperandMode_Memory, 0
#define Jb OperandSize_8, OperandMode_Relative, 0
#define Jw OperandSize_16, OperandMode_Relative, 0
#define Cb(n) OperandSize_8, OperandMode_Constant, (n)
#define Cw(n) OperandSize_16, OperandMode_Constant, (n)
#define Ms OperandSize_16, OperandMode_ModRM_RM, 0
#define Ma OperandSize_16, OperandMode_ModRM_RM, 0
#define Xb OperandSize_8, OperandMode_RegisterIndirect, Reg16_SI
#define Xw OperandSize_16, OperandMode_RegisterIndirect, Reg16_SI
#define Yb OperandSize_8, OperandMode_RegisterIndirect, Reg16_DI
#define Yw OperandSize_16, OperandMode_RegisterIndirect, Reg16_DI
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
#define CS OperandSize_16, OperandMode_SegmentRegister, Segment_CS
#define DS OperandSize_16, OperandMode_SegmentRegister, Segment_DS
#define ES OperandSize_16, OperandMode_SegmentRegister, Segment_ES
#define SS OperandSize_16, OperandMode_SegmentRegister, Segment_SS

#define EnumBaseOpcodes() \
MakeTwoOperands(0x00, Operation_ADD, Eb, Gb) \
MakeTwoOperands(0x01, Operation_ADD, Ew, Gw) \
MakeTwoOperands(0x02, Operation_ADD, Gb, Eb) \
MakeTwoOperands(0x03, Operation_ADD, Gw, Ew) \
MakeTwoOperands(0x04, Operation_ADD, AL, Ib) \
MakeTwoOperands(0x05, Operation_ADD, AX, Iw) \
MakeOneOperand(0x06, Operation_PUSH_Sreg, ES) \
MakeOneOperand(0x07, Operation_POP_Sreg, ES) \
MakeTwoOperands(0x08, Operation_OR, Eb, Gb) \
MakeTwoOperands(0x09, Operation_OR, Ew, Gw) \
MakeTwoOperands(0x0A, Operation_OR, Gb, Eb) \
MakeTwoOperands(0x0B, Operation_OR, Gw, Ew) \
MakeTwoOperands(0x0C, Operation_OR, AL, Ib) \
MakeTwoOperands(0x0D, Operation_OR, AX, Iw) \
MakeOneOperand(0x0E, Operation_PUSH_Sreg, CS) \
MakeOneOperand(0x0F, Operation_POP_Sreg, CS) \
MakeTwoOperands(0x10, Operation_ADC, Eb, Gb) \
MakeTwoOperands(0x11, Operation_ADC, Ew, Gw) \
MakeTwoOperands(0x12, Operation_ADC, Gb, Eb) \
MakeTwoOperands(0x13, Operation_ADC, Gw, Ew) \
MakeTwoOperands(0x14, Operation_ADC, AL, Ib) \
MakeTwoOperands(0x15, Operation_ADC, AX, Iw) \
MakeOneOperand(0x16, Operation_PUSH_Sreg, SS) \
MakeOneOperand(0x17, Operation_POP_Sreg, SS) \
MakeTwoOperands(0x18, Operation_SBB, Eb, Gb) \
MakeTwoOperands(0x19, Operation_SBB, Ew, Gw) \
MakeTwoOperands(0x1A, Operation_SBB, Gb, Eb) \
MakeTwoOperands(0x1B, Operation_SBB, Gw, Ew) \
MakeTwoOperands(0x1C, Operation_SBB, AL, Ib) \
MakeTwoOperands(0x1D, Operation_SBB, AX, Iw) \
MakeOneOperand(0x1E, Operation_PUSH_Sreg, DS) \
MakeOneOperand(0x1F, Operation_POP_Sreg, DS) \
MakeTwoOperands(0x20, Operation_AND, Eb, Gb) \
MakeTwoOperands(0x21, Operation_AND, Ew, Gw) \
MakeTwoOperands(0x22, Operation_AND, Gb, Eb) \
MakeTwoOperands(0x23, Operation_AND, Gw, Ew) \
MakeTwoOperands(0x24, Operation_AND, AL, Ib) \
MakeTwoOperands(0x25, Operation_AND, AX, Iw) \
MakeSegmentPrefix(0x26, Segment_ES) \
MakeNoOperands(0x27, Operation_DAA) \
MakeTwoOperands(0x28, Operation_SUB, Eb, Gb) \
MakeTwoOperands(0x29, Operation_SUB, Ew, Gw) \
MakeTwoOperands(0x2A, Operation_SUB, Gb, Eb) \
MakeTwoOperands(0x2B, Operation_SUB, Gw, Ew) \
MakeTwoOperands(0x2C, Operation_SUB, AL, Ib) \
MakeTwoOperands(0x2D, Operation_SUB, AX, Iw) \
MakeSegmentPrefix(0x2E, Segment_CS) \
MakeNoOperands(0x2F, Operation_DAS) \
MakeTwoOperands(0x30, Operation_XOR, Eb, Gb) \
MakeTwoOperands(0x31, Operation_XOR, Ew, Gw) \
MakeTwoOperands(0x32, Operation_XOR, Gb, Eb) \
MakeTwoOperands(0x33, Operation_XOR, Gw, Ew) \
MakeTwoOperands(0x34, Operation_XOR, AL, Ib) \
MakeTwoOperands(0x35, Operation_XOR, AX, Iw) \
MakeSegmentPrefix(0x36, Segment_SS) \
MakeNoOperands(0x37, Operation_AAA) \
MakeTwoOperands(0x38, Operation_CMP, Eb, Gb) \
MakeTwoOperands(0x39, Operation_CMP, Ew, Gw) \
MakeTwoOperands(0x3A, Operation_CMP, Gb, Eb) \
MakeTwoOperands(0x3B, Operation_CMP, Gw, Ew) \
MakeTwoOperands(0x3C, Operation_CMP, AL, Ib) \
MakeTwoOperands(0x3D, Operation_CMP, AX, Iw) \
MakeSegmentPrefix(0x3E, Segment_DS) \
MakeNoOperands(0x3F, Operation_AAS) \
MakeOneOperand(0x40, Operation_INC, AX) \
MakeOneOperand(0x41, Operation_INC, CX) \
MakeOneOperand(0x42, Operation_INC, DX) \
MakeOneOperand(0x43, Operation_INC, BX) \
MakeOneOperand(0x44, Operation_INC, SP) \
MakeOneOperand(0x45, Operation_INC, BP) \
MakeOneOperand(0x46, Operation_INC, SI) \
MakeOneOperand(0x47, Operation_INC, DI) \
MakeOneOperand(0x48, Operation_DEC, AX) \
MakeOneOperand(0x49, Operation_DEC, CX) \
MakeOneOperand(0x4A, Operation_DEC, DX) \
MakeOneOperand(0x4B, Operation_DEC, BX) \
MakeOneOperand(0x4C, Operation_DEC, SP) \
MakeOneOperand(0x4D, Operation_DEC, BP) \
MakeOneOperand(0x4E, Operation_DEC, SI) \
MakeOneOperand(0x4F, Operation_DEC, DI) \
MakeOneOperand(0x50, Operation_PUSH, AX) \
MakeOneOperand(0x51, Operation_PUSH, CX) \
MakeOneOperand(0x52, Operation_PUSH, DX) \
MakeOneOperand(0x53, Operation_PUSH, BX) \
MakeOneOperand(0x54, Operation_PUSH, SP) \
MakeOneOperand(0x55, Operation_PUSH, BP) \
MakeOneOperand(0x56, Operation_PUSH, SI) \
MakeOneOperand(0x57, Operation_PUSH, DI) \
MakeOneOperand(0x58, Operation_POP, AX) \
MakeOneOperand(0x59, Operation_POP, CX) \
MakeOneOperand(0x5A, Operation_POP, DX) \
MakeOneOperand(0x5B, Operation_POP, BX) \
MakeOneOperand(0x5C, Operation_POP, SP) \
MakeOneOperand(0x5D, Operation_POP, BP) \
MakeOneOperand(0x5E, Operation_POP, SI) \
MakeOneOperand(0x5F, Operation_POP, DI) \
MakeOneOperandCC(0x60, Operation_Jcc, JumpCondition_Overflow, Jb) \
MakeOneOperandCC(0x61, Operation_Jcc, JumpCondition_NotOverflow, Jb) \
MakeOneOperandCC(0x62, Operation_Jcc, JumpCondition_Below, Jb) \
MakeOneOperandCC(0x63, Operation_Jcc, JumpCondition_AboveOrEqual, Jb) \
MakeOneOperandCC(0x64, Operation_Jcc, JumpCondition_Equal, Jb) \
MakeOneOperandCC(0x65, Operation_Jcc, JumpCondition_NotEqual, Jb) \
MakeOneOperandCC(0x66, Operation_Jcc, JumpCondition_BelowOrEqual, Jb) \
MakeOneOperandCC(0x67, Operation_Jcc, JumpCondition_Above, Jb) \
MakeOneOperandCC(0x68, Operation_Jcc, JumpCondition_Sign, Jb) \
MakeOneOperandCC(0x69, Operation_Jcc, JumpCondition_NotSign, Jb) \
MakeOneOperandCC(0x6A, Operation_Jcc, JumpCondition_Parity, Jb) \
MakeOneOperandCC(0x6B, Operation_Jcc, JumpCondition_NotParity, Jb) \
MakeOneOperandCC(0x6C, Operation_Jcc, JumpCondition_Less, Jb) \
MakeOneOperandCC(0x6D, Operation_Jcc, JumpCondition_GreaterOrEqual, Jb) \
MakeOneOperandCC(0x6E, Operation_Jcc, JumpCondition_LessOrEqual, Jb) \
MakeOneOperandCC(0x6F, Operation_Jcc, JumpCondition_Greater, Jb) \
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
MakeTwoOperands(0x85, Operation_TEST, Gw, Ew) \
MakeTwoOperands(0x86, Operation_XCHG, Eb, Gb) \
MakeTwoOperands(0x87, Operation_XCHG, Ew, Gw) \
MakeTwoOperands(0x88, Operation_MOV, Eb, Gb) \
MakeTwoOperands(0x89, Operation_MOV, Ew, Gw) \
MakeTwoOperands(0x8A, Operation_MOV, Gb, Eb) \
MakeTwoOperands(0x8B, Operation_MOV, Gw, Ew) \
MakeTwoOperands(0x8C, Operation_MOV_Sreg, Ew, Sw) \
MakeTwoOperands(0x8D, Operation_LEA, Gw, M) \
MakeTwoOperands(0x8E, Operation_MOV_Sreg, Sw, Ew) \
MakeOneOperand(0x8F, Operation_POP, Ew) \
MakeNop(0x90) \
MakeTwoOperands(0x91, Operation_XCHG, CX, AX) \
MakeTwoOperands(0x92, Operation_XCHG, DX, AX) \
MakeTwoOperands(0x93, Operation_XCHG, BX, AX) \
MakeTwoOperands(0x94, Operation_XCHG, SP, AX) \
MakeTwoOperands(0x95, Operation_XCHG, BP, AX) \
MakeTwoOperands(0x96, Operation_XCHG, SI, AX) \
MakeTwoOperands(0x97, Operation_XCHG, DI, AX) \
MakeNoOperands(0x98, Operation_CBW) \
MakeNoOperands(0x99, Operation_CWD) \
MakeOneOperand(0x9A, Operation_CALL_Far, Ap) \
MakeNoOperands(0x9B, Operation_WAIT) \
MakeNoOperands(0x9C, Operation_PUSHF) \
MakeNoOperands(0x9D, Operation_POPF) \
MakeNoOperands(0x9E, Operation_SAHF) \
MakeNoOperands(0x9F, Operation_LAHF) \
MakeTwoOperands(0xA0, Operation_MOV, AL, Ob) \
MakeTwoOperands(0xA1, Operation_MOV, AX, Ow) \
MakeTwoOperands(0xA2, Operation_MOV, Ob, AL) \
MakeTwoOperands(0xA3, Operation_MOV, Ow, AX) \
MakeTwoOperands(0xA4, Operation_MOVS, Yb, Xb) \
MakeTwoOperands(0xA5, Operation_MOVS, Yw, Xw) \
MakeTwoOperands(0xA6, Operation_CMPS, Xb, Yb) \
MakeTwoOperands(0xA7, Operation_CMPS, Xw, Yw) \
MakeTwoOperands(0xA8, Operation_TEST, AL, Ib) \
MakeTwoOperands(0xA9, Operation_TEST, AX, Iw) \
MakeTwoOperands(0xAA, Operation_STOS, Yb, AL) \
MakeTwoOperands(0xAB, Operation_STOS, Yw, AX) \
MakeTwoOperands(0xAC, Operation_LODS, AL, Xb) \
MakeTwoOperands(0xAD, Operation_LODS, AX, Xw) \
MakeTwoOperands(0xAE, Operation_SCAS, AL, Xb) \
MakeTwoOperands(0xAF, Operation_SCAS, AX, Xw) \
MakeTwoOperands(0xB0, Operation_MOV, AL, Ib) \
MakeTwoOperands(0xB1, Operation_MOV, CL, Ib) \
MakeTwoOperands(0xB2, Operation_MOV, DL, Ib) \
MakeTwoOperands(0xB3, Operation_MOV, BL, Ib) \
MakeTwoOperands(0xB4, Operation_MOV, AH, Ib) \
MakeTwoOperands(0xB5, Operation_MOV, CH, Ib) \
MakeTwoOperands(0xB6, Operation_MOV, DH, Ib) \
MakeTwoOperands(0xB7, Operation_MOV, BH, Ib) \
MakeTwoOperands(0xB8, Operation_MOV, AX, Iw) \
MakeTwoOperands(0xB9, Operation_MOV, CX, Iw) \
MakeTwoOperands(0xBA, Operation_MOV, DX, Iw) \
MakeTwoOperands(0xBB, Operation_MOV, BX, Iw) \
MakeTwoOperands(0xBC, Operation_MOV, SP, Iw) \
MakeTwoOperands(0xBD, Operation_MOV, BP, Iw) \
MakeTwoOperands(0xBE, Operation_MOV, SI, Iw) \
MakeTwoOperands(0xBF, Operation_MOV, DI, Iw) \
MakeOneOperand(0xC0, Operation_RET_Near, Iw) \
MakeNoOperands(0xC1, Operation_RET_Near) \
MakeOneOperand(0xC2, Operation_RET_Near, Iw) \
MakeNoOperands(0xC3, Operation_RET_Near) \
MakeThreeOperands(0xC4, Operation_LXS, ES, Gw, Mp) \
MakeThreeOperands(0xC5, Operation_LXS, DS, Gw, Mp) \
MakeTwoOperands(0xC6, Operation_MOV, Eb, Ib) \
MakeTwoOperands(0xC7, Operation_MOV, Ew, Iw) \
MakeOneOperand(0xC8, Operation_RET_Far, Iw) \
MakeNoOperands(0xC9, Operation_RET_Far) \
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
MakeOneOperand(0xD8, Operation_Escape, Eb) \
MakeOneOperand(0xD9, Operation_Escape, Eb) \
MakeOneOperand(0xDA, Operation_Escape, Eb) \
MakeOneOperand(0xDB, Operation_Escape, Eb) \
MakeOneOperand(0xDC, Operation_Escape, Eb) \
MakeOneOperand(0xDD, Operation_Escape, Eb) \
MakeOneOperand(0xDE, Operation_Escape, Eb) \
MakeOneOperand(0xDF, Operation_Escape, Eb) \
MakeOneOperandCC(0xE0, Operation_LOOP, JumpCondition_NotEqual, Jb) \
MakeOneOperandCC(0xE1, Operation_LOOP, JumpCondition_Equal, Jb) \
MakeOneOperandCC(0xE2, Operation_LOOP, JumpCondition_Always, Jb) \
MakeOneOperandCC(0xE3, Operation_Jcc, JumpCondition_CXZero, Jb) \
MakeTwoOperands(0xE4, Operation_IN, AL, Ib) \
MakeTwoOperands(0xE5, Operation_IN, AX, Ib) \
MakeTwoOperands(0xE6, Operation_OUT, Ib, AL) \
MakeTwoOperands(0xE7, Operation_OUT, Ib, AX) \
MakeOneOperand(0xE8, Operation_CALL_Near, Jw) \
MakeOneOperand(0xE9, Operation_JMP_Near, Jw) \
MakeOneOperand(0xEA, Operation_JMP_Far, Ap) \
MakeOneOperand(0xEB, Operation_JMP_Near, Jb) \
MakeTwoOperands(0xEC, Operation_IN, AL, DX) \
MakeTwoOperands(0xED, Operation_IN, AX, DX) \
MakeTwoOperands(0xEE, Operation_OUT, DX, AL) \
MakeTwoOperands(0xEF, Operation_OUT, DX, AX) \
MakeLockPrefix(0xF0) \
MakeLockPrefix(0xF1) \
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
MakeTwoOperands(0x00, Operation_TEST, Ew, Iw) \
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

// clang-format on
