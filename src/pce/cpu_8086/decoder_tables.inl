// clang-format off

const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::base[OPCODE_TABLE_SIZE] =
{
  { Operation_ADD, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0} },
  { Operation_ADD, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0} },
  { Operation_ADD, {OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_ADD, {OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_ADD, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_ADD, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_PUSH_Sreg, {OperandSize_16, OperandMode_SegmentRegister, Segment_ES} },
  { Operation_POP_Sreg, {OperandSize_16, OperandMode_SegmentRegister, Segment_ES} },
  { Operation_OR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0} },
  { Operation_OR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0} },
  { Operation_OR, {OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_OR, {OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_OR, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_OR, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_PUSH_Sreg, {OperandSize_16, OperandMode_SegmentRegister, Segment_CS} },
  { Operation_POP_Sreg, {OperandSize_16, OperandMode_SegmentRegister, Segment_CS} },
  { Operation_ADC, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0} },
  { Operation_ADC, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0} },
  { Operation_ADC, {OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_ADC, {OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_ADC, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_ADC, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_PUSH_Sreg, {OperandSize_16, OperandMode_SegmentRegister, Segment_SS} },
  { Operation_POP_Sreg, {OperandSize_16, OperandMode_SegmentRegister, Segment_SS} },
  { Operation_SBB, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0} },
  { Operation_SBB, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0} },
  { Operation_SBB, {OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_SBB, {OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_SBB, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_SBB, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_PUSH_Sreg, {OperandSize_16, OperandMode_SegmentRegister, Segment_DS} },
  { Operation_POP_Sreg, {OperandSize_16, OperandMode_SegmentRegister, Segment_DS} },
  { Operation_AND, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0} },
  { Operation_AND, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0} },
  { Operation_AND, {OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_AND, {OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_AND, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_AND, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_Segment_Prefix, { { OperandSize_Count, OperandMode_SegmentRegister, Segment_ES } } },
  { Operation_DAA, {} },
  { Operation_SUB, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0} },
  { Operation_SUB, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0} },
  { Operation_SUB, {OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_SUB, {OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_SUB, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_SUB, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_Segment_Prefix, { { OperandSize_Count, OperandMode_SegmentRegister, Segment_CS } } },
  { Operation_DAS, {} },
  { Operation_XOR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0} },
  { Operation_XOR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0} },
  { Operation_XOR, {OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_XOR, {OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_XOR, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_XOR, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_Segment_Prefix, { { OperandSize_Count, OperandMode_SegmentRegister, Segment_SS } } },
  { Operation_AAA, {} },
  { Operation_CMP, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0} },
  { Operation_CMP, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0} },
  { Operation_CMP, {OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_CMP, {OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_CMP, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_CMP, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_Segment_Prefix, { { OperandSize_Count, OperandMode_SegmentRegister, Segment_DS } } },
  { Operation_AAS, {} },
  { Operation_INC, {OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_INC, {OperandSize_16, OperandMode_Register, Reg16_CX} },
  { Operation_INC, {OperandSize_16, OperandMode_Register, Reg16_DX} },
  { Operation_INC, {OperandSize_16, OperandMode_Register, Reg16_BX} },
  { Operation_INC, {OperandSize_16, OperandMode_Register, Reg16_SP} },
  { Operation_INC, {OperandSize_16, OperandMode_Register, Reg16_BP} },
  { Operation_INC, {OperandSize_16, OperandMode_Register, Reg16_SI} },
  { Operation_INC, {OperandSize_16, OperandMode_Register, Reg16_DI} },
  { Operation_DEC, {OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_DEC, {OperandSize_16, OperandMode_Register, Reg16_CX} },
  { Operation_DEC, {OperandSize_16, OperandMode_Register, Reg16_DX} },
  { Operation_DEC, {OperandSize_16, OperandMode_Register, Reg16_BX} },
  { Operation_DEC, {OperandSize_16, OperandMode_Register, Reg16_SP} },
  { Operation_DEC, {OperandSize_16, OperandMode_Register, Reg16_BP} },
  { Operation_DEC, {OperandSize_16, OperandMode_Register, Reg16_SI} },
  { Operation_DEC, {OperandSize_16, OperandMode_Register, Reg16_DI} },
  { Operation_PUSH, {OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_PUSH, {OperandSize_16, OperandMode_Register, Reg16_CX} },
  { Operation_PUSH, {OperandSize_16, OperandMode_Register, Reg16_DX} },
  { Operation_PUSH, {OperandSize_16, OperandMode_Register, Reg16_BX} },
  { Operation_PUSH, {OperandSize_16, OperandMode_Register, Reg16_SP} },
  { Operation_PUSH, {OperandSize_16, OperandMode_Register, Reg16_BP} },
  { Operation_PUSH, {OperandSize_16, OperandMode_Register, Reg16_SI} },
  { Operation_PUSH, {OperandSize_16, OperandMode_Register, Reg16_DI} },
  { Operation_POP, {OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_POP, {OperandSize_16, OperandMode_Register, Reg16_CX} },
  { Operation_POP, {OperandSize_16, OperandMode_Register, Reg16_DX} },
  { Operation_POP, {OperandSize_16, OperandMode_Register, Reg16_BX} },
  { Operation_POP, {OperandSize_16, OperandMode_Register, Reg16_SP} },
  { Operation_POP, {OperandSize_16, OperandMode_Register, Reg16_BP} },
  { Operation_POP, {OperandSize_16, OperandMode_Register, Reg16_SI} },
  { Operation_POP, {OperandSize_16, OperandMode_Register, Reg16_DI} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Overflow}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_NotOverflow}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Below}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_AboveOrEqual}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Equal}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_NotEqual}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_BelowOrEqual}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Above}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Sign}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_NotSign}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Parity}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_NotParity}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Less}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_GreaterOrEqual}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_LessOrEqual}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Greater}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Overflow}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_NotOverflow}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Below}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_AboveOrEqual}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Equal}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_NotEqual}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_BelowOrEqual}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Above}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Sign}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_NotSign}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Parity}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_NotParity}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Less}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_GreaterOrEqual}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_LessOrEqual}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Greater}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Extension_ModRM_Reg, {}, prefix_80 },
  { Operation_Extension_ModRM_Reg, {}, prefix_81 },
  { Operation_Extension_ModRM_Reg, {}, prefix_82 },
  { Operation_Extension_ModRM_Reg, {}, prefix_83 },
  { Operation_TEST, {OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_TEST, {OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_XCHG, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0} },
  { Operation_XCHG, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0} },
  { Operation_MOV, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0} },
  { Operation_MOV, {OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_MOV_Sreg, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_SegmentReg, 0} },
  { Operation_LEA, {OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0} },
  { Operation_MOV_Sreg, {OperandSize_16, OperandMode_ModRM_SegmentReg, 0, OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_POP, {OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_NOP, {} },
  { Operation_XCHG, {OperandSize_16, OperandMode_Register, Reg16_CX, OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_XCHG, {OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_XCHG, {OperandSize_16, OperandMode_Register, Reg16_BX, OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_XCHG, {OperandSize_16, OperandMode_Register, Reg16_SP, OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_XCHG, {OperandSize_16, OperandMode_Register, Reg16_BP, OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_XCHG, {OperandSize_16, OperandMode_Register, Reg16_SI, OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_XCHG, {OperandSize_16, OperandMode_Register, Reg16_DI, OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_CBW, {} },
  { Operation_CWD, {} },
  { Operation_CALL_Far, {OperandSize_Count, OperandMode_FarAddress, 0} },
  { Operation_WAIT, {} },
  { Operation_PUSHF, {} },
  { Operation_POPF, {} },
  { Operation_SAHF, {} },
  { Operation_LAHF, {} },
  { Operation_MOV, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Memory, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Memory, 0} },
  { Operation_MOV, {OperandSize_8, OperandMode_Memory, 0, OperandSize_8, OperandMode_Register, Reg8_AL} },
  { Operation_MOV, {OperandSize_16, OperandMode_Memory, 0, OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_MOVS, {OperandSize_8, OperandMode_RegisterIndirect, Reg16_DI, OperandSize_8, OperandMode_RegisterIndirect, Reg16_SI} },
  { Operation_MOVS, {OperandSize_16, OperandMode_RegisterIndirect, Reg16_DI, OperandSize_16, OperandMode_RegisterIndirect, Reg16_SI} },
  { Operation_CMPS, {OperandSize_8, OperandMode_RegisterIndirect, Reg16_SI, OperandSize_8, OperandMode_RegisterIndirect, Reg16_DI} },
  { Operation_CMPS, {OperandSize_16, OperandMode_RegisterIndirect, Reg16_SI, OperandSize_16, OperandMode_RegisterIndirect, Reg16_DI} },
  { Operation_TEST, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_TEST, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_STOS, {OperandSize_8, OperandMode_RegisterIndirect, Reg16_DI, OperandSize_8, OperandMode_Register, Reg8_AL} },
  { Operation_STOS, {OperandSize_16, OperandMode_RegisterIndirect, Reg16_DI, OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_LODS, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_RegisterIndirect, Reg16_SI} },
  { Operation_LODS, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_RegisterIndirect, Reg16_SI} },
  { Operation_SCAS, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_RegisterIndirect, Reg16_SI} },
  { Operation_SCAS, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_RegisterIndirect, Reg16_SI} },
  { Operation_MOV, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_8, OperandMode_Register, Reg8_CL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_8, OperandMode_Register, Reg8_DL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_8, OperandMode_Register, Reg8_BL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_8, OperandMode_Register, Reg8_AH, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_8, OperandMode_Register, Reg8_CH, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_8, OperandMode_Register, Reg8_DH, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_8, OperandMode_Register, Reg8_BH, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_Register, Reg16_CX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_Register, Reg16_BX, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_Register, Reg16_SP, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_Register, Reg16_BP, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_Register, Reg16_SI, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_Register, Reg16_DI, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_RET_Near, {OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_RET_Near, {} },
  { Operation_RET_Near, {OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_RET_Near, {} },
  { Operation_LXS, {OperandSize_16, OperandMode_SegmentRegister, Segment_ES, OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0} },
  { Operation_LXS, {OperandSize_16, OperandMode_SegmentRegister, Segment_DS, OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0} },
  { Operation_MOV, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_MOV, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_RET_Far, {OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_RET_Far, {} },
  { Operation_RET_Far, {OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_RET_Far, {} },
  { Operation_INT, {OperandSize_8, OperandMode_Constant, 3} },
  { Operation_INT, {OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_INTO, {} },
  { Operation_IRET, {} },
  { Operation_Extension_ModRM_Reg, {}, prefix_D0 },
  { Operation_Extension_ModRM_Reg, {}, prefix_D1 },
  { Operation_Extension_ModRM_Reg, {}, prefix_D2 },
  { Operation_Extension_ModRM_Reg, {}, prefix_D3 },
  { Operation_AAM, {OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_AAD, {OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_SALC, {} },
  { Operation_XLAT, {} },
  { Operation_Escape, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_Escape, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_Escape, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_Escape, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_Escape, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_Escape, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_Escape, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_Escape, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_LOOP, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_NotEqual}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_LOOP, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Equal}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_LOOP, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_Always}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_Jcc, { { OperandSize_Count, OperandMode_JumpCondition, JumpCondition_CXZero}, OperandSize_8, OperandMode_Relative, 0} },
  { Operation_IN, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_IN, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_OUT, {OperandSize_8, OperandMode_Immediate, 0, OperandSize_8, OperandMode_Register, Reg8_AL} },
  { Operation_OUT, {OperandSize_8, OperandMode_Immediate, 0, OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_CALL_Near, {OperandSize_16, OperandMode_Relative, 0} },
  { Operation_JMP_Near, {OperandSize_16, OperandMode_Relative, 0} },
  { Operation_JMP_Far, {OperandSize_Count, OperandMode_FarAddress, 0} },
  { Operation_JMP_Near, {OperandSize_8, OperandMode_Relative, 0} },
  { Operation_IN, {OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_16, OperandMode_Register, Reg16_DX} },
  { Operation_IN, {OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Register, Reg16_DX} },
  { Operation_OUT, {OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_8, OperandMode_Register, Reg8_AL} },
  { Operation_OUT, {OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_16, OperandMode_Register, Reg16_AX} },
  { Operation_Lock_Prefix },
  { Operation_Lock_Prefix },
  { Operation_RepNE_Prefix },
  { Operation_Rep_Prefix },
  { Operation_HLT, {} },
  { Operation_CMC, {} },
  { Operation_Extension_ModRM_Reg, {}, prefix_F6 },
  { Operation_Extension_ModRM_Reg, {}, prefix_F7 },
  { Operation_CLC, {} },
  { Operation_STC, {} },
  { Operation_CLI, {} },
  { Operation_STI, {} },
  { Operation_CLD, {} },
  { Operation_STD, {} },
  { Operation_Extension_ModRM_Reg, {}, prefix_FE },
  { Operation_Extension_ModRM_Reg, {}, prefix_FF },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_80[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_ADD, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_OR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_ADC, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_SBB, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_AND, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_SUB, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_XOR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_CMP, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_81[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_ADD, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_OR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_ADC, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_SBB, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_AND, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_SUB, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_XOR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_CMP, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0} },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_82[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_ADD, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_OR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_ADC, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_SBB, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_AND, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_SUB, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_XOR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_CMP, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_83[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_ADD, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_OR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_ADC, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_SBB, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_AND, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_SUB, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_XOR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_CMP, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_D0[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_ROL, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_ROR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_RCL, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_RCR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_SHL, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_SHR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_Invalid },
  { Operation_SAR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_D1[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_ROL, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_ROR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_RCL, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_RCR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_SHL, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_SHR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
  { Operation_Invalid },
  { Operation_SAR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1} },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_D2[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_ROL, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_ROR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_RCL, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_RCR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_SHL, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_SHR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_Invalid },
  { Operation_SAR, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_D3[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_ROL, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_ROR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_RCL, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_RCR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_SHL, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_SHR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
  { Operation_Invalid },
  { Operation_SAR, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL} },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_F6[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_TEST, {OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0} },
  { Operation_Invalid },
  { Operation_NOT, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_NEG, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_MUL, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_IMUL, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_DIV, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_IDIV, {OperandSize_8, OperandMode_ModRM_RM, 0} },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_F7[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_TEST, {OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0} },
  { Operation_Invalid },
  { Operation_NOT, {OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_NEG, {OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_MUL, {OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_IMUL, {OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_DIV, {OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_IDIV, {OperandSize_16, OperandMode_ModRM_RM, 0} },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_FE[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_INC, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_DEC, {OperandSize_8, OperandMode_ModRM_RM, 0} },
  { Operation_Invalid },
  { Operation_Invalid },
  { Operation_Invalid },
  { Operation_Invalid },
  { Operation_Invalid },
  { Operation_Invalid },
};
const CPU_8086::Decoder::TableEntry CPU_8086::Decoder::prefix_FF[MODRM_EXTENSION_OPCODE_TABLE_SIZE] =
{
  { Operation_INC, {OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_DEC, {OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_CALL_Near, {OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_CALL_Far, {OperandSize_Count, OperandMode_ModRM_RM, 0} },
  { Operation_JMP_Near, {OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_JMP_Far, {OperandSize_Count, OperandMode_ModRM_RM, 0} },
  { Operation_PUSH, {OperandSize_16, OperandMode_ModRM_RM, 0} },
  { Operation_Invalid },
};

// clang-format on
