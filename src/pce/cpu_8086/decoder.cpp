#include "pce/cpu_8086/decoder.h"
#include "YBaseLib/BinaryReader.h"
// Log_SetChannel(Decoder);

namespace CPU_8086 {

bool Decoder::DecodeInstruction(Instruction* instruction, VirtualMemoryAddress eip_addr, ByteStream* stream)
{
  BinaryReader reader(stream, ENDIAN_TYPE_LITTLE);
  if (!DecodeInstruction(instruction, eip_addr, [&reader](uint8* val) { return reader.SafeReadUInt8(val); },
                         [&reader](uint16* val) { return reader.SafeReadUInt16(val); }))
  {
    return false;
  }

  return !reader.GetErrorState();
}

static const char* operation_names[Operation_Count] = {"<invalid>",
                                                       "<extension>",
                                                       "<extension mod/rm reg>",
                                                       "<segment prefix>",
                                                       "<repeat prefix>",
                                                       "<repeat ne prefix>",
                                                       "<lock prefix>",
                                                       "<coprocessor escape>",

                                                       "aaa",
                                                       "aad",
                                                       "aam",
                                                       "aas",
                                                       "adc",
                                                       "add",
                                                       "and",
                                                       "call near",
                                                       "call far",
                                                       "cbw",
                                                       "clc",
                                                       "cld",
                                                       "cli",
                                                       "cmc",
                                                       "cmp",
                                                       "cmps",
                                                       "cwd",
                                                       "daa",
                                                       "das",
                                                       "dec",
                                                       "div",
                                                       "esc",
                                                       "hlt",
                                                       "idiv",
                                                       "imul",
                                                       "in",
                                                       "inc",
                                                       "int",
                                                       "into",
                                                       "iret",
                                                       "j",
                                                       "jcxz",
                                                       "jmp near",
                                                       "jmp far",
                                                       "lahf",
                                                       "lds",
                                                       "lea",
                                                       "les",
                                                       "lock",
                                                       "lods",
                                                       "loop",
                                                       "l",
                                                       "mov",
                                                       "movs",
                                                       "mov",
                                                       "mul",
                                                       "neg",
                                                       "nop",
                                                       "not",
                                                       "or",
                                                       "out",
                                                       "pop",
                                                       "pop",
                                                       "popf",
                                                       "push",
                                                       "push",
                                                       "pushf",
                                                       "rcl",
                                                       "rcr",
                                                       "rep",
                                                       "ret near",
                                                       "ret far",
                                                       "rol",
                                                       "ror",
                                                       "sahf",
                                                       "sal",
                                                       "salc",
                                                       "sar",
                                                       "sbb",
                                                       "scas",
                                                       "shl",
                                                       "shr",
                                                       "stc",
                                                       "std",
                                                       "sti",
                                                       "stos",
                                                       "sub",
                                                       "test",
                                                       "wait",
                                                       "xchg",
                                                       "xlat",
                                                       "xor"};

static const char* reg8_names[Reg8_Count] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

static const char* reg16_names[Reg16_Count] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

static const char* segment_names[Segment_Count] = {"es", "cs", "ss", "ds"};

static const char* jump_condition_names[JumpCondition_Count] = {"",   "o", "no", "s",  "ns", "e", "ne", "b",  "ae",
                                                                "be", "a", "l",  "ge", "le", "g", "p",  "np", "cxz"};

static const char* operand_size_postfixes[OperandSize_16 + 1] = {"b", "w"};

const char* Decoder::GetOperationName(Operation op)
{
  DebugAssert(op < Operation_Count);
  return operation_names[op];
}

const char* Decoder::GetRegisterName(Reg8 reg)
{
  DebugAssert(reg < Reg8_Count);
  return reg8_names[reg];
}

const char* Decoder::GetRegisterName(Reg16 reg)
{
  DebugAssert(reg < Reg16_Count);
  return reg16_names[reg];
}

const char* Decoder::GetSegmentName(Segment reg)
{
  DebugAssert(reg < Segment_Count);
  return segment_names[reg];
}

void Decoder::DisassembleToString(const Instruction* instruction, String* out_string)
{
  auto PrintPtr = [out_string](OperandSize size) {
    if (size == OperandSize_8)
      out_string->AppendString("byte ptr");
    else if (size == OperandSize_16)
      out_string->AppendString("word ptr");
  };

  const Operation operation = instruction->operation;
  out_string->Clear();

  // print any prefixes.
  if (instruction->data.has_lock)
    out_string->AppendString("lock ");
  if (instruction->data.has_rep)
  {
    // repe/repne versions only apply to scas/cmps
    if (operation == Operation_CMPS || operation == Operation_SCAS)
      out_string->AppendString(instruction->data.has_repne ? "repne " : "repe ");
    else
      out_string->AppendString("rep ");
  }

  // print operation name.
  out_string->AppendString(operation_names[operation]);

  // print the operation suffix. this is things like jump conditions, segment names, etc.
  uint32 operand_index = 0;
  if (instruction->operands[0].mode == OperandMode_JumpCondition)
  {
    out_string->AppendString(jump_condition_names[instruction->operands[0].jump_condition]);
    operand_index++;
  }
  else if (operation == Operation_LXS)
  {
    out_string->AppendString(segment_names[instruction->operands[0].segreg]);
    operand_index++;
  }

  // space between memonic and operands
  out_string->AppendString(" ");

  // print remaining operands
  bool first_operand = true;
  for (; operand_index < countof(instruction->operands); operand_index++)
  {
    const Instruction::Operand& operand = instruction->operands[operand_index];
    if (operand.mode == OperandMode_None)
      continue;

    if (!first_operand)
      out_string->AppendString(", ");
    else
      first_operand = false;

    const OperandSize size = operand.size;
    switch (operand.mode)
    {
      case OperandMode_Constant:
      {
        if (size == OperandSize_8)
          out_string->AppendFormattedString("%02xh", operand.constant);
        else if (size == OperandSize_16)
          out_string->AppendFormattedString("%04xh", operand.constant);
      }
      break;
      case OperandMode_Register:
      {
        if (size == OperandSize_8)
          out_string->AppendString(reg8_names[operand.reg8]);
        else if (size == OperandSize_16)
          out_string->AppendString(reg16_names[operand.reg8]);
      }
      break;
      case OperandMode_RegisterIndirect:
      {
        PrintPtr(size);
        out_string->AppendFormattedString(" %s:[%s]", segment_names[instruction->data.segment],
                                          reg16_names[operand.reg16]);
      }
      break;
      case OperandMode_SegmentRegister:
      {
        out_string->AppendString(segment_names[operand.segreg]);
      }
      break;
      case OperandMode_Immediate:
      {
        if (size == OperandSize_8)
          out_string->AppendFormattedString("%02xh", ZeroExtend32(instruction->data.imm8));
        else if (size == OperandSize_16)
          out_string->AppendFormattedString("%04xh", ZeroExtend32(instruction->data.imm16));
      }
      break;
      case OperandMode_Immediate2:
      {
        if (size == OperandSize_8)
          out_string->AppendFormattedString("%02xh", ZeroExtend32(instruction->data.imm2_8));
        else if (size == OperandSize_16)
          out_string->AppendFormattedString("%04xh", ZeroExtend32(instruction->data.imm2_16));
      }
      break;
      case OperandMode_Relative:
      {
        VirtualMemoryAddress dst_address =
          instruction->address + Truncate16(instruction->length) + instruction->data.disp16;
        out_string->AppendFormattedString("%04xh", dst_address);
      }
      break;
      case OperandMode_Memory:
      {
        PrintPtr(size);
        out_string->AppendFormattedString(" %s:[%04xh]", segment_names[instruction->data.segment],
                                          instruction->data.disp16);
      }
      break;
      case OperandMode_FarAddress:
      {
        out_string->AppendFormattedString("%04x:%04xh", instruction->data.imm16, instruction->data.disp16);
      }
      break;
      case OperandMode_ModRM_Reg:
      {
        uint8 index = instruction->GetModRM_Reg();
        if (size == OperandSize_8)
          out_string->AppendString(reg8_names[index]);
        else if (size == OperandSize_16)
          out_string->AppendString(reg16_names[index]);
      }
      break;
      case OperandMode_ModRM_RM:
      {
        const ModRMAddress* m = DecodeModRMAddress(instruction->data.modrm);
        if (m->addressing_mode == ModRMAddressingMode::Register)
        {
          out_string->AppendString((size == OperandSize_8) ? reg8_names[instruction->data.modrm_rm] :
                                                             reg16_names[instruction->data.modrm_rm]);
        }
        else
        {
          PrintPtr(size);
          out_string->AppendFormattedString(" [%s:", segment_names[instruction->data.segment]);

          switch (m->addressing_mode)
          {
            case ModRMAddressingMode::Direct:
              out_string->AppendFormattedString("%04xh", instruction->data.disp16);
              break;
            case ModRMAddressingMode::Indirect:
              out_string->AppendString(reg16_names[m->base_register]);
              break;
            case ModRMAddressingMode::BasedIndexed:
              out_string->AppendFormattedString("%s + %s", reg16_names[m->base_register],
                                                reg16_names[m->index_register]);
              break;
            case ModRMAddressingMode::Indexed:
            {
              int32 disp = SignExtend32(instruction->data.disp16);
              out_string->AppendFormattedString("%s %s%xh", reg16_names[m->base_register], (disp < 0) ? "- " : "+ ",
                                                (disp < 0) ? -disp : disp);
            }
            break;
            case ModRMAddressingMode::BasedIndexedDisplacement:
            {
              int32 disp = SignExtend32(instruction->data.disp16);
              out_string->AppendFormattedString("%s + %s %s%xh", reg16_names[m->base_register],
                                                reg16_names[m->index_register], (disp < 0) ? "- " : "+ ",
                                                (disp < 0) ? -disp : disp);
            }
            break;
            default:
              break;
          }
          out_string->AppendString("]");
        }
      }
      break;
      case OperandMode_ModRM_SegmentReg:
      {
        uint8 index = instruction->GetModRM_Reg();
        out_string->AppendString((index > Segment_Count) ? "<invalid>" : segment_names[index]);
      }
      break;
      default:
        break;
    }
  }
}

const Decoder::ModRMAddress* Decoder::DecodeModRMAddress(uint8 modrm)
{
  // This could probably be implemented procedurally
  // http://www.sandpile.org/x86/opc_rm16.htm
  static const ModRMAddress modrm_table_16[32] = {
    /* 00 000 - [BX + SI]         */ {ModRMAddressingMode::BasedIndexed, Reg16_BX, Reg16_SI, 0, Segment_DS},
    /* 00 001 - [BX + DI]         */ {ModRMAddressingMode::BasedIndexed, Reg16_BX, Reg16_DI, 0, Segment_DS},
    /* 00 010 - [BP + SI]         */ {ModRMAddressingMode::BasedIndexed, Reg16_BP, Reg16_SI, 0, Segment_SS},
    /* 00 011 - [BP + DI]         */ {ModRMAddressingMode::BasedIndexed, Reg16_BP, Reg16_DI, 0, Segment_SS},
    /* 00 100 - [SI]              */ {ModRMAddressingMode::Indirect, Reg16_SI, 0, 0, Segment_DS},
    /* 00 101 - [DI]              */ {ModRMAddressingMode::Indirect, Reg16_DI, 0, 0, Segment_DS},
    /* 00 110 - [sword]           */ {ModRMAddressingMode::Direct, 0, 0, 2, Segment_DS},
    /* 00 111 - [BX]              */ {ModRMAddressingMode::Indirect, Reg16_BX, 0, 0, Segment_DS},
    /* 01 000 - [BX + SI + sbyte] */ {ModRMAddressingMode::BasedIndexedDisplacement, Reg16_BX, Reg16_SI, 1, Segment_DS},
    /* 01 001 - [BX + DI + sbyte] */ {ModRMAddressingMode::BasedIndexedDisplacement, Reg16_BX, Reg16_DI, 1, Segment_DS},
    /* 01 010 - [BP + SI + sbyte] */ {ModRMAddressingMode::BasedIndexedDisplacement, Reg16_BP, Reg16_SI, 1, Segment_SS},
    /* 01 011 - [BP + DI + sbyte] */ {ModRMAddressingMode::BasedIndexedDisplacement, Reg16_BP, Reg16_DI, 1, Segment_SS},
    /* 01 100 - [SI + sbyte]      */ {ModRMAddressingMode::Indexed, Reg16_SI, 0, 1, Segment_DS},
    /* 01 101 - [DI + sbyte]      */ {ModRMAddressingMode::Indexed, Reg16_DI, 0, 1, Segment_DS},
    /* 01 110 - [BP + sbyte]      */ {ModRMAddressingMode::Indexed, Reg16_BP, 0, 1, Segment_SS},
    /* 01 111 - [BX + sbyte]      */ {ModRMAddressingMode::Indexed, Reg16_BX, 0, 1, Segment_DS},
    /* 10 000 - [BX + SI + sword] */ {ModRMAddressingMode::BasedIndexedDisplacement, Reg16_BX, Reg16_SI, 2, Segment_DS},
    /* 10 001 - [BX + DI + sword] */ {ModRMAddressingMode::BasedIndexedDisplacement, Reg16_BX, Reg16_DI, 2, Segment_DS},
    /* 10 010 - [BP + SI + sword] */ {ModRMAddressingMode::BasedIndexedDisplacement, Reg16_BP, Reg16_SI, 2, Segment_SS},
    /* 10 011 - [BP + DI + sword] */ {ModRMAddressingMode::BasedIndexedDisplacement, Reg16_BP, Reg16_DI, 2, Segment_SS},
    /* 10 100 - [SI + sword]      */ {ModRMAddressingMode::Indexed, Reg16_SI, 0, 2, Segment_DS},
    /* 10 101 - [DI + sword]      */ {ModRMAddressingMode::Indexed, Reg16_DI, 0, 2, Segment_DS},
    /* 10 110 - [BP + sword]      */ {ModRMAddressingMode::Indexed, Reg16_BP, 0, 2, Segment_SS},
    /* 10 111 - [BX + sword]      */ {ModRMAddressingMode::Indexed, Reg16_BX, 0, 2, Segment_DS},
    /* 11 000 - AL/AX             */ {ModRMAddressingMode::Register, Reg16_AX, 0, 0, Segment_DS},
    /* 11 001 - CL/CX             */ {ModRMAddressingMode::Register, Reg16_CX, 0, 0, Segment_DS},
    /* 11 010 - DL/DX             */ {ModRMAddressingMode::Register, Reg16_DX, 0, 0, Segment_DS},
    /* 11 011 - BL/BX             */ {ModRMAddressingMode::Register, Reg16_BX, 0, 0, Segment_DS},
    /* 11 100 - AH/SP             */ {ModRMAddressingMode::Register, Reg16_SP, 0, 0, Segment_DS},
    /* 11 101 - CH/BP             */ {ModRMAddressingMode::Register, Reg16_BP, 0, 0, Segment_DS},
    /* 11 110 - DH/SI             */ {ModRMAddressingMode::Register, Reg16_SI, 0, 0, Segment_DS},
    /* 11 111 - BH/DI             */ {ModRMAddressingMode::Register, Reg16_DI, 0, 0, Segment_DS},
  };

  uint8 index = ((modrm & 0b11000000) >> 3) | (modrm & 0b00000111);
  return &modrm_table_16[index];
}

bool Decoder::DecodeInstruction(Instruction* instruction, VirtualMemoryAddress eip_addr,
                                std::function<bool(u8*)> fetchb, std::function<bool(u16*)> fetchw)
{
  std::memset(instruction, 0, sizeof(*instruction));
  instruction->data.segment = Segment_DS;
  instruction->address = eip_addr;

  const TableEntry* table = base;
  bool has_modrm = false;
  for (;;)
  {
    // Fetch first byte.
    uint8 opcode;
    if (!fetchb(&opcode))
      return false;

    instruction->length += sizeof(uint8);
    const TableEntry* te = &table[opcode];

    // Handle special cases (prefixes, overrides).
    switch (te->operation)
    {
      case Operation_Segment_Prefix:
        instruction->data.segment = te->operands[0].segreg;
        instruction->data.has_segment_override = true;
        continue;
      case Operation_Lock_Prefix:
        instruction->data.has_lock = true;
        continue;
      case Operation_Rep_Prefix:
        instruction->data.has_rep = true;
        continue;
      case Operation_RepNE_Prefix:
        instruction->data.has_rep = true;
        instruction->data.has_repne = true;
        continue;
      case Operation_Extension:
        table = te->next_table;
        continue;
      case Operation_Extension_ModRM_Reg:
      {
        has_modrm = true;
        if (!fetchb(&instruction->data.modrm))
          return false;
        instruction->length += sizeof(uint8);
        te = &te->next_table[instruction->data.GetModRM_Reg()];
      }
      break;
      default:
        break;
    }

    // Check for invalid opcodes.
    if (te->operation == Operation_Invalid)
      return false;

    // Copy operands from this table entry, if any.
    instruction->operation = te->operation;
    for (uint32 i = 0; i < countof(te->operands); i++)
    {
      const Instruction::Operand& table_operand = te->operands[i];
      if (table_operand.mode == OperandMode_None)
        continue;

      Instruction::Operand& operand = instruction->operands[i];
      std::memcpy(&operand, &table_operand, sizeof(Instruction::Operand));

      // Fetch modrm and immediates.
      switch (operand.mode)
      {
        case OperandMode_ModRM_Reg:
        {
          if (!has_modrm)
          {
            has_modrm = true;
            if (!fetchb(&instruction->data.modrm))
              return false;
            instruction->length += sizeof(uint8);
          }
        }
        break;

        case OperandMode_ModRM_RM:
        {
          if (!has_modrm)
          {
            has_modrm = true;
            if (!fetchb(&instruction->data.modrm))
              return false;
            instruction->length += sizeof(uint8);
          }

          const Decoder::ModRMAddress* addr = Decoder::DecodeModRMAddress(instruction->data.modrm);
          if (addr->addressing_mode != ModRMAddressingMode::Register)
          {
            uint8 displacement_size = addr->displacement_size;
            if (!instruction->data.has_segment_override)
              instruction->data.segment = addr->default_segment;

            switch (displacement_size)
            {
              case 1:
              {
                uint8 tmpbyte;
                if (!fetchb(&tmpbyte))
                  return false;
                instruction->data.disp16 = SignExtend16(tmpbyte);
                instruction->length += sizeof(uint8);
              }
              break;
              case 2:
              {
                if (!fetchw(&instruction->data.disp16))
                  return false;
                instruction->length += sizeof(uint16);
              }
              break;
            }
          }
        }
        break;

        case OperandMode_Immediate:
        {
          switch (operand.size)
          {
            case OperandSize_8:
            {
              if (!fetchb(&instruction->data.imm8))
                return false;
              instruction->length += sizeof(uint8);
            }
            break;
            case OperandSize_16:
            {
              if (!fetchw(&instruction->data.imm16))
                return false;
              instruction->length += sizeof(uint16);
            }
            break;
          }
        }
        break;

        case OperandMode_Immediate2:
        {
          switch (operand.size)
          {
            case OperandSize_8:
            {
              if (!fetchb(&instruction->data.imm2_8))
                return false;
              instruction->length += sizeof(uint8);
            }
            break;
            case OperandSize_16:
            {
              if (!fetchw(&instruction->data.imm2_16))
                return false;
              instruction->length += sizeof(uint16);
            }
            break;
          }
        }
        break;

        case OperandMode_Relative:
        {
          switch (operand.size)
          {
            case OperandSize_8:
            {
              uint8 tmpbyte;
              if (!fetchb(&tmpbyte))
                return false;
              instruction->data.disp16 = SignExtend16(tmpbyte);
              instruction->length += sizeof(uint8);
            }
            break;
            case OperandSize_16:
            {
              if (!fetchw(&instruction->data.disp16))
                return false;
              instruction->length += sizeof(uint16);
            }
            break;
          }
        }
        break;

        case OperandMode_Memory:
        {
          if (!fetchw(&instruction->data.disp16))
            return false;
          instruction->length += sizeof(uint16);
        }
        break;

        case OperandMode_FarAddress:
        {
          if (!fetchw(&instruction->data.disp16))
            return false;
          instruction->length += sizeof(uint16);
          if (!fetchw(&instruction->data.imm16))
            return false;
          instruction->length += sizeof(uint16);
        }
        break;

        default:
          break;
      }
    }

    // Done!
    return true;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// clang-format off
#define MakeInvalidOpcode(opcode) { Operation_Invalid, {}, nullptr },
#define MakeSegmentPrefix(opcode, seg) { Operation_Segment_Prefix, { { OperandSize_Count, OperandMode_SegmentRegister, seg } }, nullptr },
#define MakeOperandSizePrefix(opcode) { Operation_OperandSize_Prefix, {}, nullptr },
#define MakeAddressSizePrefix(opcode) { Operation_AddressSize_Prefix, {}, nullptr },
#define MakeLockPrefix(opcode) { Operation_Lock_Prefix, {}, nullptr },
#define MakeRepPrefix(opcode) { Operation_Rep_Prefix, {}, nullptr },
#define MakeRepNEPrefix(opcode) { Operation_RepNE_Prefix, {}, nullptr },
#define MakeNop(opcode) { Operation_NOP, {}, nullptr },
#define MakeNoOperands(opcode, inst) { inst, {}, nullptr },
#define MakeOneOperand(opcode, inst, op1) { inst, { {op1} }, nullptr },
#define MakeOneOperandCC(opcode, inst, cc, op1) { inst, { { OperandSize_Count, OperandMode_JumpCondition, cc}, {op1} }, nullptr },
#define MakeTwoOperands(opcode, inst, op1, op2) { inst, { {op1}, {op2} }, nullptr },
#define MakeTwoOperandsCC(opcode, inst, cc, op1, op2) { inst, { { OperandSize_Count, OperandMode_JumpCondition, cc}, {op1}, {op2} }, nullptr },
#define MakeThreeOperands(opcode, inst, op1, op2, op3) { inst, { {op1}, {op2}, {op3} }, nullptr },
#define MakeExtension(opcode, prefix) { Operation_Extension, {}, prefix_##prefix },
#define MakeModRMRegExtension(opcode, prefix) { Operation_Extension_ModRM_Reg, {}, prefix_##prefix },

#include "pce/cpu_8086/opcodes.h"

// Instruction table definitions
const Decoder::TableEntry Decoder::base[OPCODE_TABLE_SIZE] = { EnumBaseOpcodes() };
const Decoder::TableEntry Decoder::prefix_80[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp1Opcodes(Eb, Ib) };
const Decoder::TableEntry Decoder::prefix_81[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp1Opcodes(Ew, Iw) };
const Decoder::TableEntry Decoder::prefix_82[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp1Opcodes(Eb, Ib) };
const Decoder::TableEntry Decoder::prefix_83[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp1Opcodes(Ew, Ib) };
const Decoder::TableEntry Decoder::prefix_d0[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp2Opcodes(Eb, Cb(1)) };
const Decoder::TableEntry Decoder::prefix_d1[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp2Opcodes(Ew, Cb(1)) };
const Decoder::TableEntry Decoder::prefix_d2[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp2Opcodes(Eb, CL) };
const Decoder::TableEntry Decoder::prefix_d3[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp2Opcodes(Ew, CL) };
const Decoder::TableEntry Decoder::prefix_f6[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp3aOpcodes(Eb) };
const Decoder::TableEntry Decoder::prefix_f7[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp3bOpcodes(Ew) };
const Decoder::TableEntry Decoder::prefix_fe[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp4Opcodes(Eb) };
const Decoder::TableEntry Decoder::prefix_ff[MODRM_EXTENSION_OPCODE_TABLE_SIZE] = { EnumGrp5Opcodes(Ew) };
// clang-format on

} // namespace CPU_8086