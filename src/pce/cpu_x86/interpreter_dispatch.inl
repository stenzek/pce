// clang-format off

void CPU_X86::InterpreterBackend::Dispatch(CPU* cpu)
{
  for (;;)
  {
    u8 opcode = cpu->FetchInstructionByte();
    switch (opcode)
    {
      case 0x00: // ADD Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_ADD<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x01: // ADD Ev, Gv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_ADD<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x02: // ADD Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_ADD<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x03: // ADD Gv, Ev
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_ADD<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x04: // ADD AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_ADD<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x05: // ADD eAX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_ADD<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x06: // PUSH_Sreg ES
        Interpreter::Execute_Operation_PUSH_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_ES>(cpu);
        return;
      case 0x07: // POP_Sreg ES
        Interpreter::Execute_Operation_POP_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_ES>(cpu);
        return;
      case 0x08: // OR Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_OR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x09: // OR Ev, Gv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_OR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x0A: // OR Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_OR<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x0B: // OR Gv, Ev
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_OR<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x0C: // OR AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_OR<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x0D: // OR eAX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_OR<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x0E: // PUSH_Sreg CS
        Interpreter::Execute_Operation_PUSH_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_CS>(cpu);
        return;
      case 0x0F: // Extension 0x0F
      {
        opcode = cpu->FetchInstructionByte();
        switch (opcode)
        {
          case 0x00: // ModRM-Reg-Extension 0x00
          {
            FetchModRM(cpu); // fetch modrm for extension
            switch (cpu->idata.GetModRM_Reg() & 0x07)
            {
              case 0x00: // SLDT Ew
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_SLDT<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
              case 0x01: // STR Ew
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_STR<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
              case 0x02: // LLDT Ew
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_LLDT<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
              case 0x03: // LTR Ew
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_LTR<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
              case 0x04: // VERR Ew
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_VERR<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
              case 0x05: // VERW Ew
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_VERW<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
            }
          }
          break;
          case 0x01: // ModRM-Reg-Extension 0x01
          {
            FetchModRM(cpu); // fetch modrm for extension
            switch (cpu->idata.GetModRM_Reg() & 0x07)
            {
              case 0x00: // SGDT Ms
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_SGDT<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
              case 0x01: // SIDT Ms
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_SIDT<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
              case 0x02: // LGDT Ms
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_LGDT<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
              case 0x03: // LIDT Ms
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_LIDT<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
              case 0x04: // SMSW Ew
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_SMSW<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
              case 0x06: // LMSW Ew
                FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_LMSW<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
                return;
              case 0x07: // INVLPG Ev
                FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                Interpreter::Execute_Operation_INVLPG<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
                return;
            }
          }
          break;
          case 0x02: // LAR Gv, Ew
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_LAR<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x03: // LSL Gv, Ew
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_LSL<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x06: // CLTS
            Interpreter::Execute_Operation_CLTS(cpu);
            return;
          case 0x08: // INVD
            Interpreter::Execute_Operation_INVD(cpu);
            return;
          case 0x09: // WBINVD
            Interpreter::Execute_Operation_WBINVD(cpu);
            return;
          case 0x20: // MOV_CR Rd, Cd
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MOV_CR<OperandSize_32, OperandMode_ModRM_RM, 0, OperandSize_32, OperandMode_ModRM_ControlRegister, 0>(cpu);
            return;
          case 0x21: // MOV_DR Rd, Dd
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MOV_DR<OperandSize_32, OperandMode_ModRM_RM, 0, OperandSize_32, OperandMode_ModRM_DebugRegister, 0>(cpu);
            return;
          case 0x22: // MOV_CR Cd, Rd
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_ControlRegister)
            FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MOV_CR<OperandSize_32, OperandMode_ModRM_ControlRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x23: // MOV_DR Dd, Rd
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_DebugRegister)
            FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MOV_DR<OperandSize_32, OperandMode_ModRM_DebugRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x24: // MOV_TR Rd, Td
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MOV_TR<OperandSize_32, OperandMode_ModRM_RM, 0, OperandSize_32, OperandMode_ModRM_TestRegister, 0>(cpu);
            return;
          case 0x26: // MOV_TR Td, Rd
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_TestRegister)
            FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MOV_TR<OperandSize_32, OperandMode_ModRM_TestRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x31: // RDTSC
            Interpreter::Execute_Operation_RDTSC(cpu);
            return;
          case 0x40: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_Overflow, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x41: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_NotOverflow, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x42: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_Below, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x43: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_AboveOrEqual, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x44: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_Equal, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x45: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_NotEqual, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x46: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_BelowOrEqual, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x47: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_Above, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x48: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_Sign, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x49: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_NotSign, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x4A: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_Parity, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x4B: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_NotParity, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x4C: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_Less, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x4D: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_GreaterOrEqual, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x4E: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_LessOrEqual, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x4F: // CMOVcc Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMOVcc<JumpCondition_Greater, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x80: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_Overflow, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x81: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_NotOverflow, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x82: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_Below, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x83: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_AboveOrEqual, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x84: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_Equal, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x85: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_NotEqual, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x86: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_BelowOrEqual, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x87: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_Above, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x88: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_Sign, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x89: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_NotSign, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x8A: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_Parity, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x8B: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_NotParity, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x8C: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_Less, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x8D: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_GreaterOrEqual, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x8E: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_LessOrEqual, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x8F: // Jcc Jv
            FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
            Interpreter::Execute_Operation_Jcc<JumpCondition_Greater, OperandSize_Count, OperandMode_Relative, 0>(cpu);
            return;
          case 0x90: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_Overflow, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x91: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_NotOverflow, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x92: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_Below, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x93: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_AboveOrEqual, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x94: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_Equal, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x95: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_NotEqual, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x96: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_BelowOrEqual, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x97: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_Above, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x98: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_Sign, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x99: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_NotSign, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x9A: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_Parity, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x9B: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_NotParity, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x9C: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_Less, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x9D: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_GreaterOrEqual, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x9E: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_LessOrEqual, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x9F: // SETcc Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SETcc<JumpCondition_Greater, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xA0: // PUSH_Sreg FS
            Interpreter::Execute_Operation_PUSH_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_FS>(cpu);
            return;
          case 0xA1: // POP_Sreg FS
            Interpreter::Execute_Operation_POP_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_FS>(cpu);
            return;
          case 0xA2: // CPUID
            Interpreter::Execute_Operation_CPUID(cpu);
            return;
          case 0xA3: // BT Ev, Gv
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_BT<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
            return;
          case 0xA4: // SHLD Ev, Gv, Ib
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 2 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SHLD<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0xA5: // SHLD Ev, Gv, CL
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SHLD<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0xA8: // PUSH_Sreg GS
            Interpreter::Execute_Operation_PUSH_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_GS>(cpu);
            return;
          case 0xA9: // POP_Sreg GS
            Interpreter::Execute_Operation_POP_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_GS>(cpu);
            return;
          case 0xAB: // BTS Ev, Gv
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_BTS<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
            return;
          case 0xAC: // SHRD Ev, Gv, Ib
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 2 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SHRD<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0xAD: // SHRD Ev, Gv, CL
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SHRD<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0xAF: // IMUL Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_IMUL<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xB0: // CMPXCHG Eb, Gb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMPXCHG<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
            return;
          case 0xB1: // CMPXCHG Ev, Gv
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMPXCHG<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
            return;
          case 0xB2: // LXS SS, Gv, Mp
            FetchModRM(cpu); // fetch modrm for operand 1 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 2 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_LXS<OperandSize_16, OperandMode_SegmentRegister, Segment_SS, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xB3: // BTR Ev, Gv
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_BTR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
            return;
          case 0xB4: // LXS FS, Gv, Mp
            FetchModRM(cpu); // fetch modrm for operand 1 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 2 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_LXS<OperandSize_16, OperandMode_SegmentRegister, Segment_FS, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xB5: // LXS GS, Gv, Mp
            FetchModRM(cpu); // fetch modrm for operand 1 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 2 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_LXS<OperandSize_16, OperandMode_SegmentRegister, Segment_GS, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xB6: // MOVZX Gv, Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MOVZX<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xB7: // MOVZX Gv, Ew
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MOVZX<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xBA: // ModRM-Reg-Extension 0xBA
          {
            FetchModRM(cpu); // fetch modrm for extension
            switch (cpu->idata.GetModRM_Reg() & 0x07)
            {
              case 0x04: // BT Ev, Ib
                FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
                Interpreter::Execute_Operation_BT<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
                return;
              case 0x05: // BTS Ev, Ib
                FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
                Interpreter::Execute_Operation_BTS<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
                return;
              case 0x06: // BTR Ev, Ib
                FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
                Interpreter::Execute_Operation_BTR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
                return;
              case 0x07: // BTC Ev, Ib
                FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
                FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
                Interpreter::Execute_Operation_BTC<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
                return;
            }
          }
          break;
          case 0xBB: // BTC Ev, Gv
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_BTC<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
            return;
          case 0xBC: // BSF Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_BSF<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xBD: // BSR Gv, Ev
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_BSR<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xBE: // MOVSX Gv, Eb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MOVSX<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xBF: // MOVSX Gv, Ew
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
            FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MOVSX<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xC0: // XADD Eb, Gb
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_XADD<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
            return;
          case 0xC1: // XADD Ev, Gv
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_XADD<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
            return;
          case 0xC7: // CMPXCHG8B Mq
            FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CMPXCHG8B<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0xC8: // BSWAP EAX
            Interpreter::Execute_Operation_BSWAP<OperandSize_32, OperandMode_Register, Reg32_EAX>(cpu);
            return;
          case 0xC9: // BSWAP ECX
            Interpreter::Execute_Operation_BSWAP<OperandSize_32, OperandMode_Register, Reg32_ECX>(cpu);
            return;
          case 0xCA: // BSWAP EDX
            Interpreter::Execute_Operation_BSWAP<OperandSize_32, OperandMode_Register, Reg32_EDX>(cpu);
            return;
          case 0xCB: // BSWAP EBX
            Interpreter::Execute_Operation_BSWAP<OperandSize_32, OperandMode_Register, Reg32_EBX>(cpu);
            return;
          case 0xCC: // BSWAP ESP
            Interpreter::Execute_Operation_BSWAP<OperandSize_32, OperandMode_Register, Reg32_ESP>(cpu);
            return;
          case 0xCD: // BSWAP EBP
            Interpreter::Execute_Operation_BSWAP<OperandSize_32, OperandMode_Register, Reg32_EBP>(cpu);
            return;
          case 0xCE: // BSWAP ESI
            Interpreter::Execute_Operation_BSWAP<OperandSize_32, OperandMode_Register, Reg32_ESI>(cpu);
            return;
          case 0xCF: // BSWAP EDI
            Interpreter::Execute_Operation_BSWAP<OperandSize_32, OperandMode_Register, Reg32_EDI>(cpu);
            return;
        }
      }
      break;
      case 0x10: // ADC Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_ADC<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x11: // ADC Ev, Gv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_ADC<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x12: // ADC Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_ADC<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x13: // ADC Gv, Ev
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_ADC<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x14: // ADC AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_ADC<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x15: // ADC eAX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_ADC<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x16: // PUSH_Sreg SS
        Interpreter::Execute_Operation_PUSH_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_SS>(cpu);
        return;
      case 0x17: // POP_Sreg SS
        Interpreter::Execute_Operation_POP_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_SS>(cpu);
        return;
      case 0x18: // SBB Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_SBB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x19: // SBB Ev, Gv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_SBB<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x1A: // SBB Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_SBB<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x1B: // SBB Gv, Ev
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_SBB<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x1C: // SBB AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_SBB<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x1D: // SBB eAX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_SBB<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x1E: // PUSH_Sreg DS
        Interpreter::Execute_Operation_PUSH_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_DS>(cpu);
        return;
      case 0x1F: // POP_Sreg DS
        Interpreter::Execute_Operation_POP_Sreg<OperandSize_16, OperandMode_SegmentRegister, Segment_DS>(cpu);
        return;
      case 0x20: // AND Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_AND<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x21: // AND Ev, Gv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_AND<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x22: // AND Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_AND<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x23: // AND Gv, Ev
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_AND<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x24: // AND AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_AND<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x25: // AND eAX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_AND<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x26: // Prefix Segment.ES
        cpu->idata.segment = Segment_ES;
        cpu->idata.has_segment_override = true;
        continue;
      case 0x27: // DAA
        Interpreter::Execute_Operation_DAA(cpu);
        return;
      case 0x28: // SUB Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_SUB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x29: // SUB Ev, Gv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_SUB<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x2A: // SUB Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_SUB<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x2B: // SUB Gv, Ev
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_SUB<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x2C: // SUB AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_SUB<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x2D: // SUB eAX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_SUB<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x2E: // Prefix Segment.CS
        cpu->idata.segment = Segment_CS;
        cpu->idata.has_segment_override = true;
        continue;
      case 0x2F: // DAS
        Interpreter::Execute_Operation_DAS(cpu);
        return;
      case 0x30: // XOR Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_XOR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x31: // XOR Ev, Gv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_XOR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x32: // XOR Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_XOR<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x33: // XOR Gv, Ev
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_XOR<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x34: // XOR AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_XOR<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x35: // XOR eAX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_XOR<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x36: // Prefix Segment.SS
        cpu->idata.segment = Segment_SS;
        cpu->idata.has_segment_override = true;
        continue;
      case 0x37: // AAA
        Interpreter::Execute_Operation_AAA(cpu);
        return;
      case 0x38: // CMP Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_CMP<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x39: // CMP Ev, Gv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_CMP<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x3A: // CMP Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_CMP<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x3B: // CMP Gv, Ev
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_CMP<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x3C: // CMP AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_CMP<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x3D: // CMP eAX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_CMP<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x3E: // Prefix Segment.DS
        cpu->idata.segment = Segment_DS;
        cpu->idata.has_segment_override = true;
        continue;
      case 0x3F: // AAS
        Interpreter::Execute_Operation_AAS(cpu);
        return;
      case 0x40: // INC eAX
        Interpreter::Execute_Operation_INC<OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0x41: // INC eCX
        Interpreter::Execute_Operation_INC<OperandSize_Count, OperandMode_Register, Reg32_ECX>(cpu);
        return;
      case 0x42: // INC eDX
        Interpreter::Execute_Operation_INC<OperandSize_Count, OperandMode_Register, Reg32_EDX>(cpu);
        return;
      case 0x43: // INC eBX
        Interpreter::Execute_Operation_INC<OperandSize_Count, OperandMode_Register, Reg32_EBX>(cpu);
        return;
      case 0x44: // INC eSP
        Interpreter::Execute_Operation_INC<OperandSize_Count, OperandMode_Register, Reg32_ESP>(cpu);
        return;
      case 0x45: // INC eBP
        Interpreter::Execute_Operation_INC<OperandSize_Count, OperandMode_Register, Reg32_EBP>(cpu);
        return;
      case 0x46: // INC eSI
        Interpreter::Execute_Operation_INC<OperandSize_Count, OperandMode_Register, Reg32_ESI>(cpu);
        return;
      case 0x47: // INC eDI
        Interpreter::Execute_Operation_INC<OperandSize_Count, OperandMode_Register, Reg32_EDI>(cpu);
        return;
      case 0x48: // DEC eAX
        Interpreter::Execute_Operation_DEC<OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0x49: // DEC eCX
        Interpreter::Execute_Operation_DEC<OperandSize_Count, OperandMode_Register, Reg32_ECX>(cpu);
        return;
      case 0x4A: // DEC eDX
        Interpreter::Execute_Operation_DEC<OperandSize_Count, OperandMode_Register, Reg32_EDX>(cpu);
        return;
      case 0x4B: // DEC eBX
        Interpreter::Execute_Operation_DEC<OperandSize_Count, OperandMode_Register, Reg32_EBX>(cpu);
        return;
      case 0x4C: // DEC eSP
        Interpreter::Execute_Operation_DEC<OperandSize_Count, OperandMode_Register, Reg32_ESP>(cpu);
        return;
      case 0x4D: // DEC eBP
        Interpreter::Execute_Operation_DEC<OperandSize_Count, OperandMode_Register, Reg32_EBP>(cpu);
        return;
      case 0x4E: // DEC eSI
        Interpreter::Execute_Operation_DEC<OperandSize_Count, OperandMode_Register, Reg32_ESI>(cpu);
        return;
      case 0x4F: // DEC eDI
        Interpreter::Execute_Operation_DEC<OperandSize_Count, OperandMode_Register, Reg32_EDI>(cpu);
        return;
      case 0x50: // PUSH eAX
        Interpreter::Execute_Operation_PUSH<OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0x51: // PUSH eCX
        Interpreter::Execute_Operation_PUSH<OperandSize_Count, OperandMode_Register, Reg32_ECX>(cpu);
        return;
      case 0x52: // PUSH eDX
        Interpreter::Execute_Operation_PUSH<OperandSize_Count, OperandMode_Register, Reg32_EDX>(cpu);
        return;
      case 0x53: // PUSH eBX
        Interpreter::Execute_Operation_PUSH<OperandSize_Count, OperandMode_Register, Reg32_EBX>(cpu);
        return;
      case 0x54: // PUSH eSP
        Interpreter::Execute_Operation_PUSH<OperandSize_Count, OperandMode_Register, Reg32_ESP>(cpu);
        return;
      case 0x55: // PUSH eBP
        Interpreter::Execute_Operation_PUSH<OperandSize_Count, OperandMode_Register, Reg32_EBP>(cpu);
        return;
      case 0x56: // PUSH eSI
        Interpreter::Execute_Operation_PUSH<OperandSize_Count, OperandMode_Register, Reg32_ESI>(cpu);
        return;
      case 0x57: // PUSH eDI
        Interpreter::Execute_Operation_PUSH<OperandSize_Count, OperandMode_Register, Reg32_EDI>(cpu);
        return;
      case 0x58: // POP eAX
        Interpreter::Execute_Operation_POP<OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0x59: // POP eCX
        Interpreter::Execute_Operation_POP<OperandSize_Count, OperandMode_Register, Reg32_ECX>(cpu);
        return;
      case 0x5A: // POP eDX
        Interpreter::Execute_Operation_POP<OperandSize_Count, OperandMode_Register, Reg32_EDX>(cpu);
        return;
      case 0x5B: // POP eBX
        Interpreter::Execute_Operation_POP<OperandSize_Count, OperandMode_Register, Reg32_EBX>(cpu);
        return;
      case 0x5C: // POP eSP
        Interpreter::Execute_Operation_POP<OperandSize_Count, OperandMode_Register, Reg32_ESP>(cpu);
        return;
      case 0x5D: // POP eBP
        Interpreter::Execute_Operation_POP<OperandSize_Count, OperandMode_Register, Reg32_EBP>(cpu);
        return;
      case 0x5E: // POP eSI
        Interpreter::Execute_Operation_POP<OperandSize_Count, OperandMode_Register, Reg32_ESI>(cpu);
        return;
      case 0x5F: // POP eDI
        Interpreter::Execute_Operation_POP<OperandSize_Count, OperandMode_Register, Reg32_EDI>(cpu);
        return;
      case 0x60: // PUSHA
        Interpreter::Execute_Operation_PUSHA(cpu);
        return;
      case 0x61: // POPA
        Interpreter::Execute_Operation_POPA(cpu);
        return;
      case 0x62: // BOUND Gv, Ma
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_BOUND<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x63: // ARPL Ew, Gw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_ARPL<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x64: // Prefix Segment.FS
        cpu->idata.segment = Segment_FS;
        cpu->idata.has_segment_override = true;
        continue;
      case 0x65: // Prefix Segment.GS
        cpu->idata.segment = Segment_GS;
        cpu->idata.has_segment_override = true;
        continue;
      case 0x66: // Operand-Size Prefix
        cpu->idata.operand_size = (cpu->m_current_operand_size == OperandSize_16) ? OperandSize_32 : OperandSize_16;
        continue;
      case 0x67: // Address-Size Prefix
        cpu->idata.address_size = (cpu->m_current_address_size == AddressSize_16) ? AddressSize_32 : AddressSize_16;
        continue;
      case 0x68: // PUSH Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Interpreter::Execute_Operation_PUSH<OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x69: // IMUL Gv, Ev, Iv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 2 (OperandMode_Immediate)
        Interpreter::Execute_Operation_IMUL<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x6A: // PUSH Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Interpreter::Execute_Operation_PUSH<OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x6B: // IMUL Gv, Ev, Ib
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 2 (OperandMode_Immediate)
        Interpreter::Execute_Operation_IMUL<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0x6C: // INS Yb, DX
        Interpreter::Execute_Operation_INS<OperandSize_8, OperandMode_RegisterIndirect, Reg32_EDI, OperandSize_16, OperandMode_Register, Reg16_DX>(cpu);
        return;
      case 0x6D: // INS Yv, DX
        Interpreter::Execute_Operation_INS<OperandSize_Count, OperandMode_RegisterIndirect, Reg32_EDI, OperandSize_16, OperandMode_Register, Reg16_DX>(cpu);
        return;
      case 0x6E: // OUTS DX, Xb
        Interpreter::Execute_Operation_OUTS<OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_8, OperandMode_RegisterIndirect, Reg32_ESI>(cpu);
        return;
      case 0x6F: // OUTS DX, Yv
        Interpreter::Execute_Operation_OUTS<OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_Count, OperandMode_RegisterIndirect, Reg32_EDI>(cpu);
        return;
      case 0x70: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_Overflow, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x71: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_NotOverflow, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x72: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_Below, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x73: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_AboveOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x74: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_Equal, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x75: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_NotEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x76: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_BelowOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x77: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_Above, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x78: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_Sign, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x79: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_NotSign, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7A: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_Parity, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7B: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_NotParity, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7C: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_Less, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7D: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_GreaterOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7E: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_LessOrEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x7F: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_Greater, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0x80: // ModRM-Reg-Extension 0x80
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ADD Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_ADD<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x01: // OR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_OR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // ADC Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_ADC<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x03: // SBB Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SBB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x04: // AND Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_AND<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x05: // SUB Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SUB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x06: // XOR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_XOR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x07: // CMP Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_CMP<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
        }
      }
      break;
      case 0x81: // ModRM-Reg-Extension 0x81
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ADD Ev, Iv
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_ADD<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x01: // OR Ev, Iv
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_OR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // ADC Ev, Iv
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_ADC<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x03: // SBB Ev, Iv
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SBB<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x04: // AND Ev, Iv
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_AND<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x05: // SUB Ev, Iv
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SUB<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x06: // XOR Ev, Iv
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_XOR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x07: // CMP Ev, Iv
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_CMP<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
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
            Interpreter::Execute_Operation_ADD<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x01: // OR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_OR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // ADC Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_ADC<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x03: // SBB Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SBB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x04: // AND Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_AND<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x05: // SUB Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SUB<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x06: // XOR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_XOR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x07: // CMP Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_CMP<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
        }
      }
      break;
      case 0x83: // ModRM-Reg-Extension 0x83
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ADD Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_ADD<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x01: // OR Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_OR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // ADC Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_ADC<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x03: // SBB Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SBB<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x04: // AND Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_AND<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x05: // SUB Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SUB<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x06: // XOR Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_XOR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x07: // CMP Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_CMP<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
        }
      }
      break;
      case 0x84: // TEST Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_TEST<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x85: // TEST Gv, Ev
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_TEST<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x86: // XCHG Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_XCHG<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x87: // XCHG Ev, Gv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_XCHG<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x88: // MOV Eb, Gb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x89: // MOV Ev, Gv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_ModRM_Reg, 0>(cpu);
        return;
      case 0x8A: // MOV Gb, Eb
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_ModRM_Reg, 0, OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x8B: // MOV Gv, Ev
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x8C: // MOV_Sreg Ew, Sw
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_MOV_Sreg<OperandSize_16, OperandMode_ModRM_RM, 0, OperandSize_16, OperandMode_ModRM_SegmentReg, 0>(cpu);
        return;
      case 0x8D: // LEA Gv, M
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_LEA<OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x8E: // MOV_Sreg Sw, Ew
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_SegmentReg)
        FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_MOV_Sreg<OperandSize_16, OperandMode_ModRM_SegmentReg, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x8F: // POP Ev
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_POP<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0x90: // NOP
        Interpreter::Execute_Operation_NOP(cpu);
        return;
      case 0x91: // XCHG eCX, eAX
        Interpreter::Execute_Operation_XCHG<OperandSize_Count, OperandMode_Register, Reg32_ECX, OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0x92: // XCHG eDX, eAX
        Interpreter::Execute_Operation_XCHG<OperandSize_Count, OperandMode_Register, Reg32_EDX, OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0x93: // XCHG eBX, eAX
        Interpreter::Execute_Operation_XCHG<OperandSize_Count, OperandMode_Register, Reg32_EBX, OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0x94: // XCHG eSP, eAX
        Interpreter::Execute_Operation_XCHG<OperandSize_Count, OperandMode_Register, Reg32_ESP, OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0x95: // XCHG eBP, eAX
        Interpreter::Execute_Operation_XCHG<OperandSize_Count, OperandMode_Register, Reg32_EBP, OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0x96: // XCHG eSI, eAX
        Interpreter::Execute_Operation_XCHG<OperandSize_Count, OperandMode_Register, Reg32_ESI, OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0x97: // XCHG eDI, eAX
        Interpreter::Execute_Operation_XCHG<OperandSize_Count, OperandMode_Register, Reg32_EDI, OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0x98: // CBW
        Interpreter::Execute_Operation_CBW(cpu);
        return;
      case 0x99: // CWD
        Interpreter::Execute_Operation_CWD(cpu);
        return;
      case 0x9A: // CALL_Far Ap
        FetchImmediate<OperandSize_Count, OperandMode_FarAddress, 0>(cpu); // fetch immediate for operand 0 (OperandMode_FarAddress)
        Interpreter::Execute_Operation_CALL_Far<OperandSize_Count, OperandMode_FarAddress, 0>(cpu);
        return;
      case 0x9B: // WAIT
        Interpreter::Execute_Operation_WAIT(cpu);
        return;
      case 0x9C: // PUSHF
        Interpreter::Execute_Operation_PUSHF(cpu);
        return;
      case 0x9D: // POPF
        Interpreter::Execute_Operation_POPF(cpu);
        return;
      case 0x9E: // SAHF
        Interpreter::Execute_Operation_SAHF(cpu);
        return;
      case 0x9F: // LAHF
        Interpreter::Execute_Operation_LAHF(cpu);
        return;
      case 0xA0: // MOV AL, Ob
        FetchImmediate<OperandSize_8, OperandMode_Memory, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Memory)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Memory, 0>(cpu);
        return;
      case 0xA1: // MOV eAX, Ov
        FetchImmediate<OperandSize_Count, OperandMode_Memory, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Memory)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_Memory, 0>(cpu);
        return;
      case 0xA2: // MOV Ob, AL
        FetchImmediate<OperandSize_8, OperandMode_Memory, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Memory)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_Memory, 0, OperandSize_8, OperandMode_Register, Reg8_AL>(cpu);
        return;
      case 0xA3: // MOV Ov, eAX
        FetchImmediate<OperandSize_Count, OperandMode_Memory, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Memory)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_Memory, 0, OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0xA4: // MOVS Yb, Xb
        Interpreter::Execute_Operation_MOVS<OperandSize_8, OperandMode_RegisterIndirect, Reg32_EDI, OperandSize_8, OperandMode_RegisterIndirect, Reg32_ESI>(cpu);
        return;
      case 0xA5: // MOVS Yv, Xv
        Interpreter::Execute_Operation_MOVS<OperandSize_Count, OperandMode_RegisterIndirect, Reg32_EDI, OperandSize_Count, OperandMode_RegisterIndirect, Reg32_ESI>(cpu);
        return;
      case 0xA6: // CMPS Xb, Yb
        Interpreter::Execute_Operation_CMPS<OperandSize_8, OperandMode_RegisterIndirect, Reg32_ESI, OperandSize_8, OperandMode_RegisterIndirect, Reg32_EDI>(cpu);
        return;
      case 0xA7: // CMPS Xv, Yv
        Interpreter::Execute_Operation_CMPS<OperandSize_Count, OperandMode_RegisterIndirect, Reg32_ESI, OperandSize_Count, OperandMode_RegisterIndirect, Reg32_EDI>(cpu);
        return;
      case 0xA8: // TEST AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_TEST<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xA9: // TEST eAX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_TEST<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xAA: // STOS Yb, AL
        Interpreter::Execute_Operation_STOS<OperandSize_8, OperandMode_RegisterIndirect, Reg32_EDI, OperandSize_8, OperandMode_Register, Reg8_AL>(cpu);
        return;
      case 0xAB: // STOS Yv, eAX
        Interpreter::Execute_Operation_STOS<OperandSize_Count, OperandMode_RegisterIndirect, Reg32_EDI, OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0xAC: // LODS AL, Xb
        Interpreter::Execute_Operation_LODS<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_RegisterIndirect, Reg32_ESI>(cpu);
        return;
      case 0xAD: // LODS eAX, Xv
        Interpreter::Execute_Operation_LODS<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_RegisterIndirect, Reg32_ESI>(cpu);
        return;
      case 0xAE: // SCAS AL, Yb
        Interpreter::Execute_Operation_SCAS<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_RegisterIndirect, Reg32_EDI>(cpu);
        return;
      case 0xAF: // SCAS eAX, Yv
        Interpreter::Execute_Operation_SCAS<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_RegisterIndirect, Reg32_EDI>(cpu);
        return;
      case 0xB0: // MOV AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB1: // MOV CL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_CL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB2: // MOV DL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_DL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB3: // MOV BL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_BL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB4: // MOV AH, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_AH, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB5: // MOV CH, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_CH, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB6: // MOV DH, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_DH, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB7: // MOV BH, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_Register, Reg8_BH, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB8: // MOV eAX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xB9: // MOV eCX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_Register, Reg32_ECX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBA: // MOV eDX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_Register, Reg32_EDX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBB: // MOV eBX, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_Register, Reg32_EBX, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBC: // MOV eSP, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_Register, Reg32_ESP, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBD: // MOV eBP, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_Register, Reg32_EBP, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBE: // MOV eSI, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_Register, Reg32_ESI, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xBF: // MOV eDI, Iv
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_Register, Reg32_EDI, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xC0: // ModRM-Reg-Extension 0xC0
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ROL Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_ROL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x01: // ROR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_ROR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // RCL Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_RCL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x03: // RCR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_RCR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x04: // SHL Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SHL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x05: // SHR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SHR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x07: // SAR Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SAR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
        }
      }
      break;
      case 0xC1: // ModRM-Reg-Extension 0xC1
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ROL Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_ROL<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x01: // ROR Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_ROR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // RCL Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_RCL<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x03: // RCR Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_RCR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x04: // SHL Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SHL<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x05: // SHR Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SHR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x07: // SAR Ev, Ib
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_SAR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
        }
      }
      break;
      case 0xC2: // RET_Near Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Interpreter::Execute_Operation_RET_Near<OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xC3: // RET_Near
        Interpreter::Execute_Operation_RET_Near(cpu);
        return;
      case 0xC4: // LXS ES, Gv, Mp
        FetchModRM(cpu); // fetch modrm for operand 1 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 2 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_LXS<OperandSize_16, OperandMode_SegmentRegister, Segment_ES, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0xC5: // LXS DS, Gv, Mp
        FetchModRM(cpu); // fetch modrm for operand 1 (OperandMode_ModRM_Reg)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 2 (OperandMode_ModRM_RM)
        Interpreter::Execute_Operation_LXS<OperandSize_16, OperandMode_SegmentRegister, Segment_DS, OperandSize_Count, OperandMode_ModRM_Reg, 0, OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
        return;
      case 0xC6: // MOV Eb, Ib
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xC7: // MOV Ev, Iv
        FetchModRM(cpu); // fetch modrm for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
        FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_MOV<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xC8: // ENTER Iw, Ib2
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        FetchImmediate<OperandSize_8, OperandMode_Immediate2, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate2)
        Interpreter::Execute_Operation_ENTER<OperandSize_16, OperandMode_Immediate, 0, OperandSize_8, OperandMode_Immediate2, 0>(cpu);
        return;
      case 0xC9: // LEAVE
        Interpreter::Execute_Operation_LEAVE(cpu);
        return;
      case 0xCA: // RET_Far Iw
        FetchImmediate<OperandSize_16, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Interpreter::Execute_Operation_RET_Far<OperandSize_16, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xCB: // RET_Far
        Interpreter::Execute_Operation_RET_Far(cpu);
        return;
      case 0xCC: // INT3
        Interpreter::Execute_Operation_INT3(cpu);
        return;
      case 0xCD: // INT Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Interpreter::Execute_Operation_INT<OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xCE: // INTO
        Interpreter::Execute_Operation_INTO(cpu);
        return;
      case 0xCF: // IRET
        Interpreter::Execute_Operation_IRET(cpu);
        return;
      case 0xD0: // ModRM-Reg-Extension 0xD0
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ROL Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_ROL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x01: // ROR Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_ROR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x02: // RCL Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_RCL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x03: // RCR Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_RCR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x04: // SHL Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SHL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x05: // SHR Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SHR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x07: // SAR Eb, Cb(1)
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SAR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
        }
      }
      break;
      case 0xD1: // ModRM-Reg-Extension 0xD1
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ROL Ev, Cb(1)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_ROL<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x01: // ROR Ev, Cb(1)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_ROR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x02: // RCL Ev, Cb(1)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_RCL<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x03: // RCR Ev, Cb(1)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_RCR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x04: // SHL Ev, Cb(1)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SHL<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x05: // SHR Ev, Cb(1)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SHR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
            return;
          case 0x07: // SAR Ev, Cb(1)
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SAR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Constant, 1>(cpu);
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
            Interpreter::Execute_Operation_ROL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x01: // ROR Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_ROR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x02: // RCL Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_RCL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x03: // RCR Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_RCR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x04: // SHL Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SHL<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x05: // SHR Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SHR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x07: // SAR Eb, CL
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SAR<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
        }
      }
      break;
      case 0xD3: // ModRM-Reg-Extension 0xD3
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // ROL Ev, CL
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_ROL<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x01: // ROR Ev, CL
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_ROR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x02: // RCL Ev, CL
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_RCL<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x03: // RCR Ev, CL
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_RCR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x04: // SHL Ev, CL
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SHL<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x05: // SHR Ev, CL
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SHR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
          case 0x07: // SAR Ev, CL
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_SAR<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Register, Reg8_CL>(cpu);
            return;
        }
      }
      break;
      case 0xD4: // AAM Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Interpreter::Execute_Operation_AAM<OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xD5: // AAD Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Interpreter::Execute_Operation_AAD<OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xD6: // SALC
        Interpreter::Execute_Operation_SALC(cpu);
        return;
      case 0xD7: // XLAT
        Interpreter::Execute_Operation_XLAT(cpu);
        return;
      case 0xD8: // X87 Extension 0xD8
      {
        FetchModRM(cpu); // fetch modrm for X87 extension
        if (!cpu->idata.ModRM_RM_IsReg())
        {
          // prefix_D8_reg
          switch (cpu->idata.GetModRM_Reg() & 0x07) // reg
          {
            case 0x00: // FADD ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x01: // FMUL ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x02: // FCOM ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x03: // FCOMP ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x04: // FSUB ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x05: // FSUBR ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x06: // FDIV ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x07: // FDIVR ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
        else
        {
          // prefix_D8_mem
          switch (cpu->idata.modrm & 0x3F) // mem
          {
            case 0x00: // FADD ST(0), ST(0)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x01: // FADD ST(0), ST(1)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x02: // FADD ST(0), ST(2)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x03: // FADD ST(0), ST(3)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x04: // FADD ST(0), ST(4)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x05: // FADD ST(0), ST(5)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x06: // FADD ST(0), ST(6)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x07: // FADD ST(0), ST(7)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x08: // FMUL ST(0), ST(0)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x09: // FMUL ST(0), ST(1)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x0A: // FMUL ST(0), ST(2)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x0B: // FMUL ST(0), ST(3)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x0C: // FMUL ST(0), ST(4)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x0D: // FMUL ST(0), ST(5)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x0E: // FMUL ST(0), ST(6)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x0F: // FMUL ST(0), ST(7)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x10: // FCOM ST(0), ST(0)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x11: // FCOM ST(0), ST(1)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x12: // FCOM ST(0), ST(2)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x13: // FCOM ST(0), ST(3)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x14: // FCOM ST(0), ST(4)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x15: // FCOM ST(0), ST(5)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x16: // FCOM ST(0), ST(6)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x17: // FCOM ST(0), ST(7)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x18: // FCOMP ST(0), ST(0)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x19: // FCOMP ST(0), ST(1)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x1A: // FCOMP ST(0), ST(2)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x1B: // FCOMP ST(0), ST(3)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x1C: // FCOMP ST(0), ST(4)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x1D: // FCOMP ST(0), ST(5)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x1E: // FCOMP ST(0), ST(6)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x1F: // FCOMP ST(0), ST(7)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x20: // FSUB ST(0), ST(0)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x21: // FSUB ST(0), ST(1)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x22: // FSUB ST(0), ST(2)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x23: // FSUB ST(0), ST(3)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x24: // FSUB ST(0), ST(4)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x25: // FSUB ST(0), ST(5)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x26: // FSUB ST(0), ST(6)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x27: // FSUB ST(0), ST(7)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x28: // FSUBR ST(0), ST(0)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x29: // FSUBR ST(0), ST(1)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x2A: // FSUBR ST(0), ST(2)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x2B: // FSUBR ST(0), ST(3)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x2C: // FSUBR ST(0), ST(4)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x2D: // FSUBR ST(0), ST(5)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x2E: // FSUBR ST(0), ST(6)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x2F: // FSUBR ST(0), ST(7)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x30: // FDIV ST(0), ST(0)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x31: // FDIV ST(0), ST(1)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x32: // FDIV ST(0), ST(2)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x33: // FDIV ST(0), ST(3)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x34: // FDIV ST(0), ST(4)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x35: // FDIV ST(0), ST(5)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x36: // FDIV ST(0), ST(6)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x37: // FDIV ST(0), ST(7)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x38: // FDIVR ST(0), ST(0)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x39: // FDIVR ST(0), ST(1)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x3A: // FDIVR ST(0), ST(2)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x3B: // FDIVR ST(0), ST(3)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x3C: // FDIVR ST(0), ST(4)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x3D: // FDIVR ST(0), ST(5)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x3E: // FDIVR ST(0), ST(6)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x3F: // FDIVR ST(0), ST(7)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
      }
      case 0xD9: // X87 Extension 0xD9
      {
        FetchModRM(cpu); // fetch modrm for X87 extension
        if (!cpu->idata.ModRM_RM_IsReg())
        {
          // prefix_D9_reg
          switch (cpu->idata.GetModRM_Reg() & 0x07) // reg
          {
            case 0x00: // FLD Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FLD<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x02: // FST Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FST<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x03: // FSTP Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FSTP<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x04: // FLDENV M
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FLDENV<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x05: // FLDCW Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FLDCW<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x06: // FNSTENV M
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FNSTENV<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x07: // FNSTCW Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FNSTCW<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
        else
        {
          // prefix_D9_mem
          switch (cpu->idata.modrm & 0x3F) // mem
          {
            case 0x00: // FLD ST(0)
              Interpreter::Execute_Operation_FLD<OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x01: // FLD ST(1)
              Interpreter::Execute_Operation_FLD<OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x02: // FLD ST(2)
              Interpreter::Execute_Operation_FLD<OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x03: // FLD ST(3)
              Interpreter::Execute_Operation_FLD<OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x04: // FLD ST(4)
              Interpreter::Execute_Operation_FLD<OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x05: // FLD ST(5)
              Interpreter::Execute_Operation_FLD<OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x06: // FLD ST(6)
              Interpreter::Execute_Operation_FLD<OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x07: // FLD ST(7)
              Interpreter::Execute_Operation_FLD<OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x08: // FXCH ST(0)
              Interpreter::Execute_Operation_FXCH<OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x09: // FXCH ST(1)
              Interpreter::Execute_Operation_FXCH<OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x0A: // FXCH ST(2)
              Interpreter::Execute_Operation_FXCH<OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x0B: // FXCH ST(3)
              Interpreter::Execute_Operation_FXCH<OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x0C: // FXCH ST(4)
              Interpreter::Execute_Operation_FXCH<OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x0D: // FXCH ST(5)
              Interpreter::Execute_Operation_FXCH<OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x0E: // FXCH ST(6)
              Interpreter::Execute_Operation_FXCH<OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x0F: // FXCH ST(7)
              Interpreter::Execute_Operation_FXCH<OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x10: // FNOP
              Interpreter::Execute_Operation_FNOP(cpu);
              return;
            case 0x20: // FCHS
              Interpreter::Execute_Operation_FCHS(cpu);
              return;
            case 0x21: // FABS
              Interpreter::Execute_Operation_FABS(cpu);
              return;
            case 0x24: // FTST
              Interpreter::Execute_Operation_FTST(cpu);
              return;
            case 0x25: // FXAM
              Interpreter::Execute_Operation_FXAM(cpu);
              return;
            case 0x28: // FLD1
              Interpreter::Execute_Operation_FLD1(cpu);
              return;
            case 0x29: // FLDL2T
              Interpreter::Execute_Operation_FLDL2T(cpu);
              return;
            case 0x2A: // FLDL2E
              Interpreter::Execute_Operation_FLDL2E(cpu);
              return;
            case 0x2B: // FLDPI
              Interpreter::Execute_Operation_FLDPI(cpu);
              return;
            case 0x2C: // FLDLG2
              Interpreter::Execute_Operation_FLDLG2(cpu);
              return;
            case 0x2D: // FLDLN2
              Interpreter::Execute_Operation_FLDLN2(cpu);
              return;
            case 0x2E: // FLDZ
              Interpreter::Execute_Operation_FLDZ(cpu);
              return;
            case 0x30: // F2XM1
              Interpreter::Execute_Operation_F2XM1(cpu);
              return;
            case 0x31: // FYL2X
              Interpreter::Execute_Operation_FYL2X(cpu);
              return;
            case 0x32: // FPTAN
              Interpreter::Execute_Operation_FPTAN(cpu);
              return;
            case 0x33: // FPATAN
              Interpreter::Execute_Operation_FPATAN(cpu);
              return;
            case 0x34: // FXTRACT
              Interpreter::Execute_Operation_FXTRACT(cpu);
              return;
            case 0x35: // FPREM1
              Interpreter::Execute_Operation_FPREM1(cpu);
              return;
            case 0x36: // FDECSTP
              Interpreter::Execute_Operation_FDECSTP(cpu);
              return;
            case 0x37: // FINCSTP
              Interpreter::Execute_Operation_FINCSTP(cpu);
              return;
            case 0x38: // FPREM
              Interpreter::Execute_Operation_FPREM(cpu);
              return;
            case 0x39: // FYL2XP1
              Interpreter::Execute_Operation_FYL2XP1(cpu);
              return;
            case 0x3A: // FSQRT
              Interpreter::Execute_Operation_FSQRT(cpu);
              return;
            case 0x3B: // FSINCOS
              Interpreter::Execute_Operation_FSINCOS(cpu);
              return;
            case 0x3C: // FRNDINT
              Interpreter::Execute_Operation_FRNDINT(cpu);
              return;
            case 0x3D: // FSCALE
              Interpreter::Execute_Operation_FSCALE(cpu);
              return;
            case 0x3E: // FSIN
              Interpreter::Execute_Operation_FSIN(cpu);
              return;
            case 0x3F: // FCOS
              Interpreter::Execute_Operation_FCOS(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
      }
      case 0xDA: // X87 Extension 0xDA
      {
        FetchModRM(cpu); // fetch modrm for X87 extension
        if (!cpu->idata.ModRM_RM_IsReg())
        {
          // prefix_DA_reg
          switch (cpu->idata.GetModRM_Reg() & 0x07) // reg
          {
            case 0x00: // FIADD ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FIADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x01: // FIMUL ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FIMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x02: // FICOM ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FICOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x03: // FICOMP ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FICOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x04: // FISUB ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FISUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x05: // FISUBR ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FISUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x06: // FIDIV ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FIDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x07: // FIDIVR ST(0), Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FIDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
        else
        {
          // prefix_DA_mem
          switch (cpu->idata.modrm & 0x3F) // mem
          {
            case 0x29: // FUCOMPP ST(0), ST(1)
              Interpreter::Execute_Operation_FUCOMPP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
      }
      case 0xDB: // X87 Extension 0xDB
      {
        FetchModRM(cpu); // fetch modrm for X87 extension
        if (!cpu->idata.ModRM_RM_IsReg())
        {
          // prefix_DB_reg
          switch (cpu->idata.GetModRM_Reg() & 0x07) // reg
          {
            case 0x00: // FILD Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FILD<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x02: // FIST Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FIST<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x03: // FISTP Md
              FetchImmediate<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FISTP<OperandSize_32, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x05: // FLD Mt
              FetchImmediate<OperandSize_80, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FLD<OperandSize_80, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x07: // FSTP Mt
              FetchImmediate<OperandSize_80, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FSTP<OperandSize_80, OperandMode_ModRM_RM, 0>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
        else
        {
          // prefix_DB_mem
          switch (cpu->idata.modrm & 0x3F) // mem
          {
            case 0x20: // FNENI
              Interpreter::Execute_Operation_FNENI(cpu);
              return;
            case 0x21: // FNDISI
              Interpreter::Execute_Operation_FNDISI(cpu);
              return;
            case 0x22: // FNCLEX
              Interpreter::Execute_Operation_FNCLEX(cpu);
              return;
            case 0x23: // FNINIT
              Interpreter::Execute_Operation_FNINIT(cpu);
              return;
            case 0x24: // FSETPM
              Interpreter::Execute_Operation_FSETPM(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
      }
      case 0xDC: // X87 Extension 0xDC
      {
        FetchModRM(cpu); // fetch modrm for X87 extension
        if (!cpu->idata.ModRM_RM_IsReg())
        {
          // prefix_DC_reg
          switch (cpu->idata.GetModRM_Reg() & 0x07) // reg
          {
            case 0x00: // FADD ST(0), Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x01: // FMUL ST(0), Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x02: // FCOM ST(0), Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x03: // FCOMP ST(0), Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x04: // FSUB ST(0), Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x05: // FSUBR ST(0), Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x06: // FDIV ST(0), Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x07: // FDIVR ST(0), Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
        else
        {
          // prefix_DC_mem
          switch (cpu->idata.modrm & 0x3F) // mem
          {
            case 0x00: // FADD ST(0), ST(0)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x01: // FADD ST(1), ST(0)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x02: // FADD ST(2), ST(0)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x03: // FADD ST(3), ST(0)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x04: // FADD ST(4), ST(0)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x05: // FADD ST(5), ST(0)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x06: // FADD ST(6), ST(0)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x07: // FADD ST(7), ST(0)
              Interpreter::Execute_Operation_FADD<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x08: // FMUL ST(0), ST(0)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x09: // FMUL ST(1), ST(0)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0A: // FMUL ST(2), ST(0)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0B: // FMUL ST(3), ST(0)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0C: // FMUL ST(4), ST(0)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0D: // FMUL ST(5), ST(0)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0E: // FMUL ST(6), ST(0)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0F: // FMUL ST(7), ST(0)
              Interpreter::Execute_Operation_FMUL<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x10: // FCOM ST(0), ST(0)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x11: // FCOM ST(1), ST(0)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x12: // FCOM ST(2), ST(0)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x13: // FCOM ST(3), ST(0)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x14: // FCOM ST(4), ST(0)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x15: // FCOM ST(5), ST(0)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x16: // FCOM ST(6), ST(0)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x17: // FCOM ST(7), ST(0)
              Interpreter::Execute_Operation_FCOM<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x18: // FCOMP ST(0), ST(0)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x19: // FCOMP ST(1), ST(0)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x1A: // FCOMP ST(2), ST(0)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x1B: // FCOMP ST(3), ST(0)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x1C: // FCOMP ST(4), ST(0)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x1D: // FCOMP ST(5), ST(0)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x1E: // FCOMP ST(6), ST(0)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x1F: // FCOMP ST(7), ST(0)
              Interpreter::Execute_Operation_FCOMP<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x20: // FSUBR ST(0), ST(0)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x21: // FSUBR ST(1), ST(0)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x22: // FSUBR ST(2), ST(0)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x23: // FSUBR ST(3), ST(0)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x24: // FSUBR ST(4), ST(0)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x25: // FSUBR ST(5), ST(0)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x26: // FSUBR ST(6), ST(0)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x27: // FSUBR ST(7), ST(0)
              Interpreter::Execute_Operation_FSUBR<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x28: // FSUB ST(0), ST(0)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x29: // FSUB ST(1), ST(0)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2A: // FSUB ST(2), ST(0)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2B: // FSUB ST(3), ST(0)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2C: // FSUB ST(4), ST(0)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2D: // FSUB ST(5), ST(0)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2E: // FSUB ST(6), ST(0)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2F: // FSUB ST(7), ST(0)
              Interpreter::Execute_Operation_FSUB<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x30: // FDIVR ST(0), ST(0)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x31: // FDIVR ST(1), ST(0)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x32: // FDIVR ST(2), ST(0)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x33: // FDIVR ST(3), ST(0)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x34: // FDIVR ST(4), ST(0)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x35: // FDIVR ST(5), ST(0)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x36: // FDIVR ST(6), ST(0)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x37: // FDIVR ST(7), ST(0)
              Interpreter::Execute_Operation_FDIVR<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x38: // FDIV ST(0), ST(0)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x39: // FDIV ST(1), ST(0)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3A: // FDIV ST(2), ST(0)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3B: // FDIV ST(3), ST(0)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3C: // FDIV ST(4), ST(0)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3D: // FDIV ST(5), ST(0)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3E: // FDIV ST(6), ST(0)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3F: // FDIV ST(7), ST(0)
              Interpreter::Execute_Operation_FDIV<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
      }
      case 0xDD: // X87 Extension 0xDD
      {
        FetchModRM(cpu); // fetch modrm for X87 extension
        if (!cpu->idata.ModRM_RM_IsReg())
        {
          // prefix_DD_reg
          switch (cpu->idata.GetModRM_Reg() & 0x07) // reg
          {
            case 0x00: // FLD Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FLD<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x02: // FST Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FST<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x03: // FSTP Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FSTP<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x04: // FRSTOR M
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FRSTOR<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x06: // FNSAVE M
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FNSAVE<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x07: // FNSTSW Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FNSTSW<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
        else
        {
          // prefix_DD_mem
          switch (cpu->idata.modrm & 0x3F) // mem
          {
            case 0x00: // FFREE ST(0)
              Interpreter::Execute_Operation_FFREE<OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x01: // FFREE ST(1)
              Interpreter::Execute_Operation_FFREE<OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x02: // FFREE ST(2)
              Interpreter::Execute_Operation_FFREE<OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x03: // FFREE ST(3)
              Interpreter::Execute_Operation_FFREE<OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x04: // FFREE ST(4)
              Interpreter::Execute_Operation_FFREE<OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x05: // FFREE ST(5)
              Interpreter::Execute_Operation_FFREE<OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x06: // FFREE ST(6)
              Interpreter::Execute_Operation_FFREE<OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x07: // FFREE ST(7)
              Interpreter::Execute_Operation_FFREE<OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x10: // FST ST(0)
              Interpreter::Execute_Operation_FST<OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x11: // FST ST(1)
              Interpreter::Execute_Operation_FST<OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x12: // FST ST(2)
              Interpreter::Execute_Operation_FST<OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x13: // FST ST(3)
              Interpreter::Execute_Operation_FST<OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x14: // FST ST(4)
              Interpreter::Execute_Operation_FST<OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x15: // FST ST(5)
              Interpreter::Execute_Operation_FST<OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x16: // FST ST(6)
              Interpreter::Execute_Operation_FST<OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x17: // FST ST(7)
              Interpreter::Execute_Operation_FST<OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x18: // FSTP ST(0)
              Interpreter::Execute_Operation_FSTP<OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x19: // FSTP ST(1)
              Interpreter::Execute_Operation_FSTP<OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x1A: // FSTP ST(2)
              Interpreter::Execute_Operation_FSTP<OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x1B: // FSTP ST(3)
              Interpreter::Execute_Operation_FSTP<OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x1C: // FSTP ST(4)
              Interpreter::Execute_Operation_FSTP<OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x1D: // FSTP ST(5)
              Interpreter::Execute_Operation_FSTP<OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x1E: // FSTP ST(6)
              Interpreter::Execute_Operation_FSTP<OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x1F: // FSTP ST(7)
              Interpreter::Execute_Operation_FSTP<OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x20: // FUCOM ST(0), ST(0)
              Interpreter::Execute_Operation_FUCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x21: // FUCOM ST(0), ST(1)
              Interpreter::Execute_Operation_FUCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x22: // FUCOM ST(0), ST(2)
              Interpreter::Execute_Operation_FUCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x23: // FUCOM ST(0), ST(3)
              Interpreter::Execute_Operation_FUCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x24: // FUCOM ST(0), ST(4)
              Interpreter::Execute_Operation_FUCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x25: // FUCOM ST(0), ST(5)
              Interpreter::Execute_Operation_FUCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x26: // FUCOM ST(0), ST(6)
              Interpreter::Execute_Operation_FUCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x27: // FUCOM ST(0), ST(7)
              Interpreter::Execute_Operation_FUCOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            case 0x28: // FUCOMP ST(0), ST(0)
              Interpreter::Execute_Operation_FUCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x29: // FUCOMP ST(0), ST(1)
              Interpreter::Execute_Operation_FUCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x2A: // FUCOMP ST(0), ST(2)
              Interpreter::Execute_Operation_FUCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 2>(cpu);
              return;
            case 0x2B: // FUCOMP ST(0), ST(3)
              Interpreter::Execute_Operation_FUCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 3>(cpu);
              return;
            case 0x2C: // FUCOMP ST(0), ST(4)
              Interpreter::Execute_Operation_FUCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 4>(cpu);
              return;
            case 0x2D: // FUCOMP ST(0), ST(5)
              Interpreter::Execute_Operation_FUCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 5>(cpu);
              return;
            case 0x2E: // FUCOMP ST(0), ST(6)
              Interpreter::Execute_Operation_FUCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 6>(cpu);
              return;
            case 0x2F: // FUCOMP ST(0), ST(7)
              Interpreter::Execute_Operation_FUCOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 7>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
      }
      case 0xDE: // X87 Extension 0xDE
      {
        FetchModRM(cpu); // fetch modrm for X87 extension
        if (!cpu->idata.ModRM_RM_IsReg())
        {
          // prefix_DE_reg
          switch (cpu->idata.GetModRM_Reg() & 0x07) // reg
          {
            case 0x00: // FIADD ST(0), Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FIADD<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x01: // FIMUL ST(0), Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FIMUL<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x02: // FICOM ST(0), Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FICOM<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x03: // FICOMP ST(0), Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FICOMP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x04: // FISUB ST(0), Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FISUB<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x05: // FISUBR ST(0), Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FISUBR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x06: // FIDIV ST(0), Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FIDIV<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x07: // FIDIVR ST(0), Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 1 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FIDIVR<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
        else
        {
          // prefix_DE_mem
          switch (cpu->idata.modrm & 0x3F) // mem
          {
            case 0x00: // FADDP ST(0), ST(0)
              Interpreter::Execute_Operation_FADDP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x01: // FADDP ST(1), ST(0)
              Interpreter::Execute_Operation_FADDP<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x02: // FADDP ST(2), ST(0)
              Interpreter::Execute_Operation_FADDP<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x03: // FADDP ST(3), ST(0)
              Interpreter::Execute_Operation_FADDP<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x04: // FADDP ST(4), ST(0)
              Interpreter::Execute_Operation_FADDP<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x05: // FADDP ST(5), ST(0)
              Interpreter::Execute_Operation_FADDP<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x06: // FADDP ST(6), ST(0)
              Interpreter::Execute_Operation_FADDP<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x07: // FADDP ST(7), ST(0)
              Interpreter::Execute_Operation_FADDP<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x08: // FMULP ST(0), ST(0)
              Interpreter::Execute_Operation_FMULP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x09: // FMULP ST(1), ST(0)
              Interpreter::Execute_Operation_FMULP<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0A: // FMULP ST(2), ST(0)
              Interpreter::Execute_Operation_FMULP<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0B: // FMULP ST(3), ST(0)
              Interpreter::Execute_Operation_FMULP<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0C: // FMULP ST(4), ST(0)
              Interpreter::Execute_Operation_FMULP<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0D: // FMULP ST(5), ST(0)
              Interpreter::Execute_Operation_FMULP<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0E: // FMULP ST(6), ST(0)
              Interpreter::Execute_Operation_FMULP<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x0F: // FMULP ST(7), ST(0)
              Interpreter::Execute_Operation_FMULP<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x19: // FCOMPP ST(0), ST(1)
              Interpreter::Execute_Operation_FCOMPP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 1>(cpu);
              return;
            case 0x20: // FSUBRP ST(0), ST(0)
              Interpreter::Execute_Operation_FSUBRP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x21: // FSUBRP ST(1), ST(0)
              Interpreter::Execute_Operation_FSUBRP<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x22: // FSUBRP ST(2), ST(0)
              Interpreter::Execute_Operation_FSUBRP<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x23: // FSUBRP ST(3), ST(0)
              Interpreter::Execute_Operation_FSUBRP<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x24: // FSUBRP ST(4), ST(0)
              Interpreter::Execute_Operation_FSUBRP<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x25: // FSUBRP ST(5), ST(0)
              Interpreter::Execute_Operation_FSUBRP<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x26: // FSUBRP ST(6), ST(0)
              Interpreter::Execute_Operation_FSUBRP<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x27: // FSUBRP ST(7), ST(0)
              Interpreter::Execute_Operation_FSUBRP<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x28: // FSUBP ST(0), ST(0)
              Interpreter::Execute_Operation_FSUBP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x29: // FSUBP ST(1), ST(0)
              Interpreter::Execute_Operation_FSUBP<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2A: // FSUBP ST(2), ST(0)
              Interpreter::Execute_Operation_FSUBP<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2B: // FSUBP ST(3), ST(0)
              Interpreter::Execute_Operation_FSUBP<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2C: // FSUBP ST(4), ST(0)
              Interpreter::Execute_Operation_FSUBP<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2D: // FSUBP ST(5), ST(0)
              Interpreter::Execute_Operation_FSUBP<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2E: // FSUBP ST(6), ST(0)
              Interpreter::Execute_Operation_FSUBP<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x2F: // FSUBP ST(7), ST(0)
              Interpreter::Execute_Operation_FSUBP<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x30: // FDIVRP ST(0), ST(0)
              Interpreter::Execute_Operation_FDIVRP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x31: // FDIVRP ST(1), ST(0)
              Interpreter::Execute_Operation_FDIVRP<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x32: // FDIVRP ST(2), ST(0)
              Interpreter::Execute_Operation_FDIVRP<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x33: // FDIVRP ST(3), ST(0)
              Interpreter::Execute_Operation_FDIVRP<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x34: // FDIVRP ST(4), ST(0)
              Interpreter::Execute_Operation_FDIVRP<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x35: // FDIVRP ST(5), ST(0)
              Interpreter::Execute_Operation_FDIVRP<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x36: // FDIVRP ST(6), ST(0)
              Interpreter::Execute_Operation_FDIVRP<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x37: // FDIVRP ST(7), ST(0)
              Interpreter::Execute_Operation_FDIVRP<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x38: // FDIVP ST(0), ST(0)
              Interpreter::Execute_Operation_FDIVP<OperandSize_80, OperandMode_FPRegister, 0, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x39: // FDIVP ST(1), ST(0)
              Interpreter::Execute_Operation_FDIVP<OperandSize_80, OperandMode_FPRegister, 1, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3A: // FDIVP ST(2), ST(0)
              Interpreter::Execute_Operation_FDIVP<OperandSize_80, OperandMode_FPRegister, 2, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3B: // FDIVP ST(3), ST(0)
              Interpreter::Execute_Operation_FDIVP<OperandSize_80, OperandMode_FPRegister, 3, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3C: // FDIVP ST(4), ST(0)
              Interpreter::Execute_Operation_FDIVP<OperandSize_80, OperandMode_FPRegister, 4, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3D: // FDIVP ST(5), ST(0)
              Interpreter::Execute_Operation_FDIVP<OperandSize_80, OperandMode_FPRegister, 5, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3E: // FDIVP ST(6), ST(0)
              Interpreter::Execute_Operation_FDIVP<OperandSize_80, OperandMode_FPRegister, 6, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            case 0x3F: // FDIVP ST(7), ST(0)
              Interpreter::Execute_Operation_FDIVP<OperandSize_80, OperandMode_FPRegister, 7, OperandSize_80, OperandMode_FPRegister, 0>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
      }
      case 0xDF: // X87 Extension 0xDF
      {
        FetchModRM(cpu); // fetch modrm for X87 extension
        if (!cpu->idata.ModRM_RM_IsReg())
        {
          // prefix_DF_reg
          switch (cpu->idata.GetModRM_Reg() & 0x07) // reg
          {
            case 0x00: // FILD Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FILD<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x02: // FIST Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FIST<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x03: // FISTP Mw
              FetchImmediate<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FISTP<OperandSize_16, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x04: // FBLD Mt
              FetchImmediate<OperandSize_80, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FBLD<OperandSize_80, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x05: // FILD Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FILD<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x06: // FBSTP Mt
              FetchImmediate<OperandSize_80, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FBSTP<OperandSize_80, OperandMode_ModRM_RM, 0>(cpu);
              return;
            case 0x07: // FISTP Mq
              FetchImmediate<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
              Interpreter::Execute_Operation_FISTP<OperandSize_64, OperandMode_ModRM_RM, 0>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
        else
        {
          // prefix_DF_mem
          switch (cpu->idata.modrm & 0x3F) // mem
          {
            case 0x20: // FNSTSW AX
              Interpreter::Execute_Operation_FNSTSW<OperandSize_16, OperandMode_Register, Reg16_AX>(cpu);
              return;
            default:
              FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
              Interpreter::StartX87Instruction(cpu);
              return;
          }
        }
      }
      case 0xE0: // LOOP Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_LOOP<JumpCondition_NotEqual, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0xE1: // LOOP Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_LOOP<JumpCondition_Equal, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0xE2: // LOOP Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_LOOP<JumpCondition_Always, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0xE3: // Jcc Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_Jcc<JumpCondition_CXZero, OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0xE4: // IN AL, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_IN<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xE5: // IN eAX, Ib
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
        Interpreter::Execute_Operation_IN<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_8, OperandMode_Immediate, 0>(cpu);
        return;
      case 0xE6: // OUT Ib, AL
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Interpreter::Execute_Operation_OUT<OperandSize_8, OperandMode_Immediate, 0, OperandSize_8, OperandMode_Register, Reg8_AL>(cpu);
        return;
      case 0xE7: // OUT Ib, eAX
        FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Immediate)
        Interpreter::Execute_Operation_OUT<OperandSize_8, OperandMode_Immediate, 0, OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0xE8: // CALL_Near Jv
        FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_CALL_Near<OperandSize_Count, OperandMode_Relative, 0>(cpu);
        return;
      case 0xE9: // JMP_Near Jv
        FetchImmediate<OperandSize_Count, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_JMP_Near<OperandSize_Count, OperandMode_Relative, 0>(cpu);
        return;
      case 0xEA: // JMP_Far Ap
        FetchImmediate<OperandSize_Count, OperandMode_FarAddress, 0>(cpu); // fetch immediate for operand 0 (OperandMode_FarAddress)
        Interpreter::Execute_Operation_JMP_Far<OperandSize_Count, OperandMode_FarAddress, 0>(cpu);
        return;
      case 0xEB: // JMP_Near Jb
        FetchImmediate<OperandSize_8, OperandMode_Relative, 0>(cpu); // fetch immediate for operand 0 (OperandMode_Relative)
        Interpreter::Execute_Operation_JMP_Near<OperandSize_8, OperandMode_Relative, 0>(cpu);
        return;
      case 0xEC: // IN AL, DX
        Interpreter::Execute_Operation_IN<OperandSize_8, OperandMode_Register, Reg8_AL, OperandSize_16, OperandMode_Register, Reg16_DX>(cpu);
        return;
      case 0xED: // IN eAX, DX
        Interpreter::Execute_Operation_IN<OperandSize_Count, OperandMode_Register, Reg32_EAX, OperandSize_16, OperandMode_Register, Reg16_DX>(cpu);
        return;
      case 0xEE: // OUT DX, AL
        Interpreter::Execute_Operation_OUT<OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_8, OperandMode_Register, Reg8_AL>(cpu);
        return;
      case 0xEF: // OUT DX, eAX
        Interpreter::Execute_Operation_OUT<OperandSize_16, OperandMode_Register, Reg16_DX, OperandSize_Count, OperandMode_Register, Reg32_EAX>(cpu);
        return;
      case 0xF0: // Lock Prefix
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
        Interpreter::Execute_Operation_HLT(cpu);
        return;
      case 0xF5: // CMC
        Interpreter::Execute_Operation_CMC(cpu);
        return;
      case 0xF6: // ModRM-Reg-Extension 0xF6
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // TEST Eb, Ib
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_8, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_TEST<OperandSize_8, OperandMode_ModRM_RM, 0, OperandSize_8, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // NOT Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_NOT<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x03: // NEG Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_NEG<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x04: // MUL Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MUL<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x05: // IMUL Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_IMUL<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x06: // DIV Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_DIV<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x07: // IDIV Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_IDIV<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
        }
      }
      break;
      case 0xF7: // ModRM-Reg-Extension 0xF7
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // TEST Ev, Iv
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            FetchImmediate<OperandSize_Count, OperandMode_Immediate, 0>(cpu); // fetch immediate for operand 1 (OperandMode_Immediate)
            Interpreter::Execute_Operation_TEST<OperandSize_Count, OperandMode_ModRM_RM, 0, OperandSize_Count, OperandMode_Immediate, 0>(cpu);
            return;
          case 0x02: // NOT Ev
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_NOT<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x03: // NEG Ev
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_NEG<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x04: // MUL Ev
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_MUL<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x05: // IMUL Ev
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_IMUL<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x06: // DIV Ev
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_DIV<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x07: // IDIV Ev
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_IDIV<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
        }
      }
      break;
      case 0xF8: // CLC
        Interpreter::Execute_Operation_CLC(cpu);
        return;
      case 0xF9: // STC
        Interpreter::Execute_Operation_STC(cpu);
        return;
      case 0xFA: // CLI
        Interpreter::Execute_Operation_CLI(cpu);
        return;
      case 0xFB: // STI
        Interpreter::Execute_Operation_STI(cpu);
        return;
      case 0xFC: // CLD
        Interpreter::Execute_Operation_CLD(cpu);
        return;
      case 0xFD: // STD
        Interpreter::Execute_Operation_STD(cpu);
        return;
      case 0xFE: // ModRM-Reg-Extension 0xFE
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // INC Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_INC<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x01: // DEC Eb
            FetchImmediate<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_DEC<OperandSize_8, OperandMode_ModRM_RM, 0>(cpu);
            return;
        }
      }
      break;
      case 0xFF: // ModRM-Reg-Extension 0xFF
      {
        FetchModRM(cpu); // fetch modrm for extension
        switch (cpu->idata.GetModRM_Reg() & 0x07)
        {
          case 0x00: // INC Ev
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_INC<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x01: // DEC Ev
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_DEC<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x02: // CALL_Near Ev
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CALL_Near<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x03: // CALL_Far Mp
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_CALL_Far<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x04: // JMP_Near Ev
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_JMP_Near<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x05: // JMP_Far Mp
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_JMP_Far<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
          case 0x06: // PUSH Ev
            FetchImmediate<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu); // fetch immediate for operand 0 (OperandMode_ModRM_RM)
            Interpreter::Execute_Operation_PUSH<OperandSize_Count, OperandMode_ModRM_RM, 0>(cpu);
            return;
        }
      }
      break;
    }
    // If we hit here, it means the opcode is invalid, as all other switch cases continue
    RaiseInvalidOpcode(cpu);
    return;
  }
}

// clang-format on
