// clang-format off

void CPU_8086::Instructions::DispatchInstruction(CPU* cpu)
{
  for (;;)
  {
    u8 opcode = cpu->FetchInstructionByte();
    switch (opcode)
    {
      case 0x00: // ADD Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_ADD<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x01: // ADD Ew, Gw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_ADD<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x02: // ADD Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_ADD<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x03: // ADD Gw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_ADD<OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x04: // ADD AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_ADD<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x05: // ADD AX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_ADD<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x06: // PUSH_Sreg ES
        Execute_Operation_PUSH_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_ES>(cpu);
        return;
      case 0x07: // POP_Sreg ES
        Execute_Operation_POP_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_ES>(cpu);
        return;
      case 0x08: // OR Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_OR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x09: // OR Ew, Gw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_OR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x0A: // OR Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_OR<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x0B: // OR Gw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_OR<OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x0C: // OR AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_OR<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x0D: // OR AX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_OR<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x0E: // PUSH_Sreg CS
        Execute_Operation_PUSH_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_CS>(cpu);
        return;
      case 0x0F: // POP_Sreg CS
        Execute_Operation_POP_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_CS>(cpu);
        return;
      case 0x10: // ADC Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_ADC<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x11: // ADC Ew, Gw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_ADC<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x12: // ADC Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_ADC<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x13: // ADC Gw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_ADC<OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x14: // ADC AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_ADC<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x15: // ADC AX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_ADC<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x16: // PUSH_Sreg SS
        Execute_Operation_PUSH_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_SS>(cpu);
        return;
      case 0x17: // POP_Sreg SS
        Execute_Operation_POP_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_SS>(cpu);
        return;
      case 0x18: // SBB Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_SBB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x19: // SBB Ew, Gw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_SBB<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x1A: // SBB Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_SBB<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x1B: // SBB Gw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_SBB<OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x1C: // SBB AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_SBB<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x1D: // SBB AX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_SBB<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x1E: // PUSH_Sreg DS
        Execute_Operation_PUSH_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_DS>(cpu);
        return;
      case 0x1F: // POP_Sreg DS
        Execute_Operation_POP_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_DS>(cpu);
        return;
      case 0x20: // AND Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_AND<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x21: // AND Ew, Gw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_AND<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x22: // AND Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_AND<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x23: // AND Gw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_AND<OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x24: // AND AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_AND<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x25: // AND AX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_AND<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x26: // Prefix Segment.ES
        cpu->idata.segment = Segment_ES;
        cpu->idata.has_segment_override = true;
        continue;
      case 0x27: // DAA
        Execute_Operation_DAA(cpu);
        return;
      case 0x28: // SUB Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_SUB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x29: // SUB Ew, Gw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_SUB<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x2A: // SUB Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_SUB<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x2B: // SUB Gw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_SUB<OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x2C: // SUB AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_SUB<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x2D: // SUB AX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_SUB<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x2E: // Prefix Segment.CS
        cpu->idata.segment = Segment_CS;
        cpu->idata.has_segment_override = true;
        continue;
      case 0x2F: // DAS
        Execute_Operation_DAS(cpu);
        return;
      case 0x30: // XOR Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_XOR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x31: // XOR Ew, Gw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_XOR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x32: // XOR Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_XOR<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x33: // XOR Gw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_XOR<OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x34: // XOR AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_XOR<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x35: // XOR AX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_XOR<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x36: // Prefix Segment.SS
        cpu->idata.segment = Segment_SS;
        cpu->idata.has_segment_override = true;
        continue;
      case 0x37: // AAA
        Execute_Operation_AAA(cpu);
        return;
      case 0x38: // CMP Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_CMP<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x39: // CMP Ew, Gw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_CMP<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x3A: // CMP Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_CMP<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x3B: // CMP Gw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_CMP<OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x3C: // CMP AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_CMP<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x3D: // CMP AX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_CMP<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x3E: // Prefix Segment.DS
        cpu->idata.segment = Segment_DS;
        cpu->idata.has_segment_override = true;
        continue;
      case 0x3F: // AAS
        Execute_Operation_AAS(cpu);
        return;
      case 0x40: // INC AX
        Execute_Operation_INC<OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0x41: // INC CX
        Execute_Operation_INC<OperandSize_16, OperandMode_Register, Reg16_CX>(cpu);
        return;
      case 0x42: // INC DX
        Execute_Operation_INC<OperandSize_16, OperandMode_Register, Reg16_DX>(cpu);
        return;
      case 0x43: // INC BX
        Execute_Operation_INC<OperandSize_16, OperandMode_Register, Reg16_BX>(cpu);
        return;
      case 0x44: // INC SP
        Execute_Operation_INC<OperandSize_16, OperandMode_Register, Reg16_SP>(cpu);
        return;
      case 0x45: // INC BP
        Execute_Operation_INC<OperandSize_16, OperandMode_Register, Reg16_BP>(cpu);
        return;
      case 0x46: // INC SI
        Execute_Operation_INC<OperandSize_16, OperandMode_Register, Reg16_SI>(cpu);
        return;
      case 0x47: // INC DI
        Execute_Operation_INC<OperandSize_16, OperandMode_Register, Reg16_DI>(cpu);
        return;
      case 0x48: // DEC AX
        Execute_Operation_DEC<OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0x49: // DEC CX
        Execute_Operation_DEC<OperandSize_16, OperandMode_Register, Reg16_CX>(cpu);
        return;
      case 0x4A: // DEC DX
        Execute_Operation_DEC<OperandSize_16, OperandMode_Register, Reg16_DX>(cpu);
        return;
      case 0x4B: // DEC BX
        Execute_Operation_DEC<OperandSize_16, OperandMode_Register, Reg16_BX>(cpu);
        return;
      case 0x4C: // DEC SP
        Execute_Operation_DEC<OperandSize_16, OperandMode_Register, Reg16_SP>(cpu);
        return;
      case 0x4D: // DEC BP
        Execute_Operation_DEC<OperandSize_16, OperandMode_Register, Reg16_BP>(cpu);
        return;
      case 0x4E: // DEC SI
        Execute_Operation_DEC<OperandSize_16, OperandMode_Register, Reg16_SI>(cpu);
        return;
      case 0x4F: // DEC DI
        Execute_Operation_DEC<OperandSize_16, OperandMode_Register, Reg16_DI>(cpu);
        return;
      case 0x50: // PUSH AX
        Execute_Operation_PUSH<OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0x51: // PUSH CX
        Execute_Operation_PUSH<OperandSize_16, OperandMode_Register, Reg16_CX>(cpu);
        return;
      case 0x52: // PUSH DX
        Execute_Operation_PUSH<OperandSize_16, OperandMode_Register, Reg16_DX>(cpu);
        return;
      case 0x53: // PUSH BX
        Execute_Operation_PUSH<OperandSize_16, OperandMode_Register, Reg16_BX>(cpu);
        return;
      case 0x54: // PUSH SP
        Execute_Operation_PUSH<OperandSize_16, OperandMode_Register, Reg16_SP>(cpu);
        return;
      case 0x55: // PUSH BP
        Execute_Operation_PUSH<OperandSize_16, OperandMode_Register, Reg16_BP>(cpu);
        return;
      case 0x56: // PUSH SI
        Execute_Operation_PUSH<OperandSize_16, OperandMode_Register, Reg16_SI>(cpu);
        return;
      case 0x57: // PUSH DI
        Execute_Operation_PUSH<OperandSize_16, OperandMode_Register, Reg16_DI>(cpu);
        return;
      case 0x58: // POP AX
        Execute_Operation_POP<OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0x59: // POP CX
        Execute_Operation_POP<OperandSize_16, OperandMode_Register, Reg16_CX>(cpu);
        return;
      case 0x5A: // POP DX
        Execute_Operation_POP<OperandSize_16, OperandMode_Register, Reg16_DX>(cpu);
        return;
      case 0x5B: // POP BX
        Execute_Operation_POP<OperandSize_16, OperandMode_Register, Reg16_BX>(cpu);
        return;
      case 0x5C: // POP SP
        Execute_Operation_POP<OperandSize_16, OperandMode_Register, Reg16_SP>(cpu);
        return;
      case 0x5D: // POP BP
        Execute_Operation_POP<OperandSize_16, OperandMode_Register, Reg16_BP>(cpu);
        return;
      case 0x5E: // POP SI
        Execute_Operation_POP<OperandSize_16, OperandMode_Register, Reg16_SI>(cpu);
        return;
      case 0x5F: // POP DI
        Execute_Operation_POP<OperandSize_16, OperandMode_Register, Reg16_DI>(cpu);
        return;
      case 0x60: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Overflow, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x61: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_NotOverflow, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x62: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Below, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x63: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_AboveOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x64: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Equal, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x65: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_NotEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x66: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_BelowOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x67: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Above, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x68: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Sign, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x69: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_NotSign, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x6A: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Parity, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x6B: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_NotParity, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x6C: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Less, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x6D: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_GreaterOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x6E: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_LessOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x6F: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Greater, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x70: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Overflow, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x71: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_NotOverflow, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x72: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Below, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x73: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_AboveOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x74: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Equal, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x75: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_NotEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x76: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_BelowOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x77: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Above, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x78: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Sign, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x79: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_NotSign, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7A: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Parity, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7B: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_NotParity, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7C: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Less, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7D: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_GreaterOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7E: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_LessOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7F: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_Greater, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x80: // ModRM-Reg-Extension 0x80
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ADD Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_ADD<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x01: // OR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_OR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // ADC Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_ADC<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x03: // SBB Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_SBB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x04: // AND Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_AND<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x05: // SUB Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_SUB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x06: // XOR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_XOR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x07: // CMP Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_CMP<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
        }
      }
      break;
      case 0x81: // ModRM-Reg-Extension 0x81
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ADD Ew, Iw
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_ADD<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x01: // OR Ew, Iw
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_OR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // ADC Ew, Iw
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_ADC<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x03: // SBB Ew, Iw
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_SBB<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x04: // AND Ew, Iw
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_AND<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x05: // SUB Ew, Iw
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_SUB<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x06: // XOR Ew, Iw
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_XOR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x07: // CMP Ew, Iw
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_CMP<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0>(cpu);
            return;
        }
      }
      break;
      case 0x82: // ModRM-Reg-Extension 0x82
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ADD Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_ADD<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x01: // OR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_OR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // ADC Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_ADC<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x03: // SBB Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_SBB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x04: // AND Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_AND<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x05: // SUB Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_SUB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x06: // XOR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_XOR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x07: // CMP Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_CMP<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
        }
      }
      break;
      case 0x83: // ModRM-Reg-Extension 0x83
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ADD Ew, Ib
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_ADD<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x01: // OR Ew, Ib
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_OR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // ADC Ew, Ib
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_ADC<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x03: // SBB Ew, Ib
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_SBB<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x04: // AND Ew, Ib
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_AND<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x05: // SUB Ew, Ib
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_SUB<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x06: // XOR Ew, Ib
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_XOR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x07: // CMP Ew, Ib
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_CMP<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
        }
      }
      break;
      case 0x84: // TEST Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_TEST<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x85: // TEST Gw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_TEST<OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x86: // XCHG Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_XCHG<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x87: // XCHG Ew, Gw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_XCHG<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x88: // MOV Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_MOV<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x89: // MOV Ew, Gw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_MOV<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x8A: // MOV Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_MOV<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x8B: // MOV Gw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_MOV<OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x8C: // MOV_Sreg Ew, Sw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_MOV_Sreg<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_SegmentReg, 0>(cpu);
        return;
      case 0x8D: // LEA Gw, M
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_LEA<OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x8E: // MOV_Sreg Sw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_SegmentReg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Execute_Operation_MOV_Sreg<OperandSize_16, OperandMode_ModRM_SegmentReg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x8F: // POP Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Execute_Operation_POP<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x90: // NOP
        Execute_Operation_NOP(cpu);
        return;
      case 0x91: // XCHG CX, AX
        Execute_Operation_XCHG<OperandSize_16, OperandMode_Register, Reg16_CX, OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0x92: // XCHG DX, AX
        Execute_Operation_XCHG<OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0x93: // XCHG BX, AX
        Execute_Operation_XCHG<OperandSize_16, OperandMode_Register, Reg16_BX, OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0x94: // XCHG SP, AX
        Execute_Operation_XCHG<OperandSize_16, OperandMode_Register, Reg16_SP, OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0x95: // XCHG BP, AX
        Execute_Operation_XCHG<OperandSize_16, OperandMode_Register, Reg16_BP, OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0x96: // XCHG SI, AX
        Execute_Operation_XCHG<OperandSize_16, OperandMode_Register, Reg16_SI, OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0x97: // XCHG DI, AX
        Execute_Operation_XCHG<OperandSize_16, OperandMode_Register, Reg16_DI, OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0x98: // CBW
        Execute_Operation_CBW(cpu);
        return;
      case 0x99: // CWD
        Execute_Operation_CWD(cpu);
        return;
      case 0x9A: // CALL_Far Ap
        FetchImmediate<OperandSize_Count, OperandMode_FarAddress, 0>(cpu); // fetch immediate for operand 0 (OperandMode_FarAddress)
        Execute_Operation_CALL_Far<OperandSize_Count, OperandMode_FarAddress, 0>(cpu);
        return;
      case 0x9B: // WAIT
        Execute_Operation_WAIT(cpu);
        return;
      case 0x9C: // PUSHF
        Execute_Operation_PUSHF(cpu);
        return;
      case 0x9D: // POPF
        Execute_Operation_POPF(cpu);
        return;
      case 0x9E: // SAHF
        Execute_Operation_SAHF(cpu);
        return;
      case 0x9F: // LAHF
        Execute_Operation_LAHF(cpu);
        return;
      case 0xA0: // MOV AL, Ob
        FetchImmediate<OperandSize_8, OperandMode_Memory, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Memory)
        Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Memory, 0>(cpu);
        return;
      case 0xA1: // MOV AX, Ow
        FetchImmediate<OperandSize_16, OperandMode_Memory, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Memory)
        Execute_Operation_MOV<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Memory, 0>(cpu);
        return;
      case 0xA2: // MOV Ob, AL
        FetchImmediate<OperandSize_8, OperandMode_Memory, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Memory)
        Execute_Operation_MOV<OperandSize_8, OperandMode_Memory, 0, OperandSize_8, OperandMode_Register, Reg8_AL>(cpu);
        return;
      case 0xA3: // MOV Ow, AX
        FetchImmediate<OperandSize_16, OperandMode_Memory, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Memory)
        Execute_Operation_MOV<OperandSize_16, OperandMode_Memory, 0, OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0xA4: // MOVS Yb, Xb
        Execute_Operation_MOVS<OperandSize_8, OperandMode_RegisterIndirect, Reg16_DI, OperandSize_8, OperandMode_RegisterIndirect, Reg16_SI>(cpu);
        return;
      case 0xA5: // MOVS Yw, Xw
        Execute_Operation_MOVS<OperandSize_16, OperandMode_RegisterIndirect, Reg16_DI, OperandSize_16, OperandMode_RegisterIndirect, Reg16_SI>(cpu);
        return;
      case 0xA6: // CMPS Xb, Yb
        Execute_Operation_CMPS<OperandSize_8, OperandMode_RegisterIndirect, Reg16_SI, OperandSize_8, OperandMode_RegisterIndirect, Reg16_DI>(cpu);
        return;
      case 0xA7: // CMPS Xw, Yw
        Execute_Operation_CMPS<OperandSize_16, OperandMode_RegisterIndirect, Reg16_SI, OperandSize_16, OperandMode_RegisterIndirect, Reg16_DI>(cpu);
        return;
      case 0xA8: // TEST AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_TEST<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xA9: // TEST AX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_TEST<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xAA: // STOS Yb, AL
        Execute_Operation_STOS<OperandSize_8, OperandMode_RegisterIndirect, Reg16_DI, OperandSize_8, OperandMode_Register, Reg8_AL>(cpu);
        return;
      case 0xAB: // STOS Yw, AX
        Execute_Operation_STOS<OperandSize_16, OperandMode_RegisterIndirect, Reg16_DI, OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0xAC: // LODS AL, Xb
        Execute_Operation_LODS<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_RegisterIndirect, Reg16_SI>(cpu);
        return;
      case 0xAD: // LODS AX, Xw
        Execute_Operation_LODS<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_RegisterIndirect, Reg16_SI>(cpu);
        return;
      case 0xAE: // SCAS AL, Xb
        Execute_Operation_SCAS<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_RegisterIndirect, Reg16_SI>(cpu);
        return;
      case 0xAF: // SCAS AX, Xw
        Execute_Operation_SCAS<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_RegisterIndirect, Reg16_SI>(cpu);
        return;
      case 0xB0: // MOV AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB1: // MOV CL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_CL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB2: // MOV DL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_DL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB3: // MOV BL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_BL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB4: // MOV AH, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_AH, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB5: // MOV CH, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_CH, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB6: // MOV DH, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_DH, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB7: // MOV BH, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_BH, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB8: // MOV AX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB9: // MOV CX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_16, OperandMode_Register, Reg16_CX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBA: // MOV DX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBB: // MOV BX, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_16, OperandMode_Register, Reg16_BX, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBC: // MOV SP, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_16, OperandMode_Register, Reg16_SP, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBD: // MOV BP, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_16, OperandMode_Register, Reg16_BP, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBE: // MOV SI, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_16, OperandMode_Register, Reg16_SI, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBF: // MOV DI, Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_16, OperandMode_Register, Reg16_DI, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xC0: // RET_Near Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Execute_Operation_RET_Near<OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xC1: // RET_Near
        Execute_Operation_RET_Near(cpu);
        return;
      case 0xC2: // RET_Near Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Execute_Operation_RET_Near<OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xC3: // RET_Near
        Execute_Operation_RET_Near(cpu);
        return;
      case 0xC4: // LXS ES, Gw, Mp
        FetchModRM(cpu); // fetch modrm for operand 1 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 2 (OperandMode_ModRM_RM)
        Execute_Operation_LXS<OperandSize_16, OperandMode_SegmentRegister, Segment_ES, OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0xC5: // LXS DS, Gw, Mp
        FetchModRM(cpu); // fetch modrm for operand 1 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 2 (OperandMode_ModRM_RM)
        Execute_Operation_LXS<OperandSize_16, OperandMode_SegmentRegister, Segment_DS, OperandSize_16, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0xC6: // MOV Eb, Ib
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xC7: // MOV Ew, Iw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_MOV<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xC8: // RET_Far Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Execute_Operation_RET_Far<OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xC9: // RET_Far
        Execute_Operation_RET_Far(cpu);
        return;
      case 0xCA: // RET_Far Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Execute_Operation_RET_Far<OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xCB: // RET_Far
        Execute_Operation_RET_Far(cpu);
        return;
      case 0xCC: // INT Cb(3)
        Execute_Operation_INT<OperandSize_8, OperandMode_Constant, 3>(cpu);
        return;
      case 0xCD: // INT Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Execute_Operation_INT<OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xCE: // INTO
        Execute_Operation_INTO(cpu);
        return;
      case 0xCF: // IRET
        Execute_Operation_IRET(cpu);
        return;
      case 0xD0: // ModRM-Reg-Extension 0xD0
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ROL Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_ROL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x01: // ROR Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_ROR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x02: // RCL Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_RCL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x03: // RCR Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_RCR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x04: // SHL Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SHL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x05: // SHR Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SHR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x07: // SAR Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SAR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
        }
      }
      break;
      case 0xD1: // ModRM-Reg-Extension 0xD1
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ROL Ew, Cb(1)
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_ROL<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x01: // ROR Ew, Cb(1)
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_ROR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x02: // RCL Ew, Cb(1)
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_RCL<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x03: // RCR Ew, Cb(1)
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_RCR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x04: // SHL Ew, Cb(1)
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SHL<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x05: // SHR Ew, Cb(1)
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SHR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x07: // SAR Ew, Cb(1)
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SAR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
        }
      }
      break;
      case 0xD2: // ModRM-Reg-Extension 0xD2
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ROL Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_ROL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x01: // ROR Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_ROR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x02: // RCL Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_RCL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x03: // RCR Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_RCR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x04: // SHL Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SHL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x05: // SHR Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SHR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x07: // SAR Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SAR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
        }
      }
      break;
      case 0xD3: // ModRM-Reg-Extension 0xD3
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ROL Ew, CL
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_ROL<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x01: // ROR Ew, CL
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_ROR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x02: // RCL Ew, CL
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_RCL<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x03: // RCR Ew, CL
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_RCR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x04: // SHL Ew, CL
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SHL<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x05: // SHR Ew, CL
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SHR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x07: // SAR Ew, CL
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_SAR<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
        }
      }
      break;
      case 0xD4: // AAM Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Execute_Operation_AAM<OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xD5: // AAD Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Execute_Operation_AAD<OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xD6: // SALC
        Execute_Operation_SALC(cpu);
        return;
      case 0xD7: // XLAT
        Execute_Operation_XLAT(cpu);
        return;
      case 0xD8: // Escape Eb
      case 0xD9: // Escape Eb
      case 0xDA: // Escape Eb
      case 0xDB: // Escape Eb
      case 0xDC: // Escape Eb
      case 0xDD: // Escape Eb
      case 0xDE: // Escape Eb
      case 0xDF: // Escape Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        return;
      case 0xE0: // LOOP Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_LOOP<JumpCondition_NotEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0xE1: // LOOP Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_LOOP<JumpCondition_Equal, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0xE2: // LOOP Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_LOOP<JumpCondition_Always, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0xE3: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_Jcc<JumpCondition_CXZero, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0xE4: // IN AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_IN<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xE5: // IN AX, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Execute_Operation_IN<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xE6: // OUT Ib, AL
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Execute_Operation_OUT<OperandSize_8, OperandMode_Immediate, 0, OperandSize_8, OperandMode_Register, Reg8_AL>(cpu);
        return;
      case 0xE7: // OUT Ib, AX
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Execute_Operation_OUT<OperandSize_8, OperandMode_Immediate, 0, OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0xE8: // CALL_Near Jw
        FetchImmediate<OperandSize_16, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_CALL_Near<OperandSize_16, OperandMode_Relative, 0>(cpu);
        return;
      case 0xE9: // JMP_Near Jw
        FetchImmediate<OperandSize_16, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_JMP_Near<OperandSize_16, OperandMode_Relative, 0>(cpu);
        return;
      case 0xEA: // JMP_Far Ap
        FetchImmediate<OperandSize_Count, OperandMode_FarAddress, 0>(cpu); // fetch immediate for operand 0 (OperandMode_FarAddress)
        Execute_Operation_JMP_Far<OperandSize_Count, OperandMode_FarAddress, 0>(cpu);
        return;
      case 0xEB: // JMP_Near Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Execute_Operation_JMP_Near<OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0xEC: // IN AL, DX
        Execute_Operation_IN<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_16, OperandMode_Register, Reg16_DX>(cpu);
        return;
      case 0xED: // IN AX, DX
        Execute_Operation_IN<OperandSize_16, OperandMode_Register, Reg16_AX, OperandSize_16, OperandMode_Register, Reg16_DX>(cpu);
        return;
      case 0xEE: // OUT DX, AL
        Execute_Operation_OUT<OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_8, OperandMode_Register, Reg8_AL>(cpu);
        return;
      case 0xEF: // OUT DX, AX
        Execute_Operation_OUT<OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
        return;
      case 0xF0: // Lock Prefix
        cpu->idata.has_lock = true;
        continue;
      case 0xF1: // Lock Prefix
        cpu->idata.has_lock = true;
        continue;
      case 0xF2: // RepNE Prefix
        cpu->idata.has_rep = true;
        cpu->idata.has_repne = true;
        continue;
      case 0xF3: // Rep Prefix
        cpu->idata.has_rep = true;
        continue;
      case 0xF4: // HLT
        Execute_Operation_HLT(cpu);
        return;
      case 0xF5: // CMC
        Execute_Operation_CMC(cpu);
        return;
      case 0xF6: // ModRM-Reg-Extension 0xF6
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // TEST Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_TEST<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // NOT Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_NOT<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x03: // NEG Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_NEG<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x04: // MUL Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_MUL<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x05: // IMUL Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_IMUL<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x06: // DIV Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_DIV<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x07: // IDIV Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_IDIV<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
        }
      }
      break;
      case 0xF7: // ModRM-Reg-Extension 0xF7
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // TEST Ew, Iw
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Execute_Operation_TEST<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // NOT Ew
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_NOT<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x03: // NEG Ew
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_NEG<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x04: // MUL Ew
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_MUL<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x05: // IMUL Ew
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_IMUL<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x06: // DIV Ew
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_DIV<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x07: // IDIV Ew
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_IDIV<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
        }
      }
      break;
      case 0xF8: // CLC
        Execute_Operation_CLC(cpu);
        return;
      case 0xF9: // STC
        Execute_Operation_STC(cpu);
        return;
      case 0xFA: // CLI
        Execute_Operation_CLI(cpu);
        return;
      case 0xFB: // STI
        Execute_Operation_STI(cpu);
        return;
      case 0xFC: // CLD
        Execute_Operation_CLD(cpu);
        return;
      case 0xFD: // STD
        Execute_Operation_STD(cpu);
        return;
      case 0xFE: // ModRM-Reg-Extension 0xFE
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // INC Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_INC<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x01: // DEC Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_DEC<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
        }
      }
      break;
      case 0xFF: // ModRM-Reg-Extension 0xFF
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // INC Ew
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_INC<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x01: // DEC Ew
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_DEC<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x02: // CALL_Near Ew
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_CALL_Near<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x03: // CALL_Far Mp
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_CALL_Far<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x04: // JMP_Near Ew
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_JMP_Near<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x05: // JMP_Far Mw
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_JMP_Far<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x06: // PUSH Ew
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Execute_Operation_PUSH<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
        }
      }
      break;
    }
    // If we hit here, it means the opcode is invalid, as all other switch cases continue
    RaiseInvalidOpcode(cpu);
  }
}

// clang-format on
