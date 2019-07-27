#include "pce/cpu_x86/decoder.h"
#include "YBaseLib/BinaryReader.h"
// Log_SetChannel(Decoder);

namespace CPU_X86 {

bool Decoder::DecodeInstruction(Instruction* instruction, AddressSize address_size, OperandSize operand_size,
                                VirtualMemoryAddress eip_addr, ByteStream* stream)
{
  BinaryReader reader(stream, ENDIAN_TYPE_LITTLE);
  auto readb = [&reader](u8* val) { return reader.SafeReadUInt8(val); };
  auto readw = [&reader](u16* val) { return reader.SafeReadUInt16(val); };
  auto readd = [&reader](u32* val) { return reader.SafeReadUInt32(val); };
  return DecodeInstruction(instruction, address_size, operand_size, eip_addr, readb, readw, readd) &&
         !reader.GetErrorState();
}

static const char* operation_names[Operation_Count] = {"<invalid>",
                                                       "<extension>",
                                                       "<extension mod/rm reg>",
                                                       "<extension x87>",
                                                       "<segment prefix>",
                                                       "<repeat prefix>",
                                                       "<repeat ne prefix>",
                                                       "<lock prefix>",
                                                       "<operand size override prefix>",
                                                       "<address size override prefix>",

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
                                                       "int3",
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
                                                       "xor",

                                                       "bound",
                                                       "bsf",
                                                       "bsr",
                                                       "bt",
                                                       "bts",
                                                       "btr",
                                                       "btc",
                                                       "shld",
                                                       "shrd",
                                                       "ins",
                                                       "outs",
                                                       "clts",
                                                       "enter",
                                                       "leave",
                                                       "lss",
                                                       "lgdt",
                                                       "sgdt",
                                                       "lidt",
                                                       "sidt",
                                                       "lldt",
                                                       "sdlt",
                                                       "ltr",
                                                       "str",
                                                       "lmsw",
                                                       "smsw",
                                                       "verr",
                                                       "verw",
                                                       "arpl",
                                                       "lar",
                                                       "lsl",
                                                       "pusha",
                                                       "popa",

                                                       "lfs",
                                                       "lgs",
                                                       "movsx",
                                                       "movzx",
                                                       "mov",
                                                       "mov",
                                                       "mov",
                                                       "set",

                                                       "bswap",
                                                       "cmpxchg",
                                                       "cmov",
                                                       "invd",
                                                       "wbinvd",
                                                       "invlpg",
                                                       "xadd",

                                                       "cpuid",
                                                       "rdtsc",
                                                       "cmpxchg8b",

                                                       "f2xm1",
                                                       "fabs",
                                                       "fadd",
                                                       "faddp",
                                                       "fbld",
                                                       "fbstp",
                                                       "fchs",
                                                       "fcom",
                                                       "fcomp",
                                                       "fcompp",
                                                       "fdecstp",
                                                       "fdiv",
                                                       "fdivp",
                                                       "fdivr",
                                                       "fdivrp",
                                                       "ffree",
                                                       "fiadd",
                                                       "ficom",
                                                       "ficomp",
                                                       "fidiv",
                                                       "fidivr",
                                                       "fild",
                                                       "fimul",
                                                       "fincstp",
                                                       "fist",
                                                       "fistp",
                                                       "fisub",
                                                       "fisubr",
                                                       "fld",
                                                       "fld1",
                                                       "fldcw",
                                                       "fldenv",
                                                       "fldl2e",
                                                       "fldl2t",
                                                       "fldlg2",
                                                       "fldln2",
                                                       "fldpi",
                                                       "fldz",
                                                       "fmul",
                                                       "fmulp",
                                                       "fnclex",
                                                       "fndisi",
                                                       "fneni",
                                                       "fninit",
                                                       "fnop",
                                                       "fnsave",
                                                       "fnstcw",
                                                       "fnstenv",
                                                       "fnstsw",
                                                       "fpatan",
                                                       "fprem",
                                                       "fptan",
                                                       "frndint",
                                                       "frstor",
                                                       "fscale",
                                                       "fsqrt",
                                                       "fst",
                                                       "fstp",
                                                       "fsub",
                                                       "fsubp",
                                                       "fsubr",
                                                       "fsubrp",
                                                       "ftst",
                                                       "fxam",
                                                       "fxch",
                                                       "fxtract",
                                                       "fyl2x",
                                                       "fyl2xp1",

                                                       "fsetpm",

                                                       "fcos",
                                                       "fprem1",
                                                       "fsin",
                                                       "fsincos",
                                                       "fucom",
                                                       "fucomp",
                                                       "fucompp"};

static const char* reg8_names[Reg8_Count] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

static const char* reg16_names[Reg16_Count] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

static const char* reg32_names[Reg32_Count] = {"eax",    "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "eip",
                                               "eflags", "cr0", "cr2", "cr3", "cr4", "dr0", "dr1", "dr2", "dr3",
                                               "dr4",    "dr5", "dr6", "dr7", "tr3", "tr4", "tr5", "tr6", "tr7"};

static const char* segment_names[Segment_Count] = {"es", "cs", "ss", "ds", "fs", "gs"};

static const char* jump_condition_names[JumpCondition_Count] = {"",   "o", "no", "s",  "ns", "e", "ne", "b",  "ae",
                                                                "be", "a", "l",  "ge", "le", "g", "p",  "np", "cxz"};

static const char* operand_size_postfixes[OperandSize_32 + 1] = {"b", "w", "d"};

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

const char* Decoder::GetRegisterName(Reg32 reg)
{
  DebugAssert(reg < Reg32_Count);
  return reg32_names[reg];
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
    else if (size == OperandSize_32)
      out_string->AppendString("dword ptr");
    else if (size == OperandSize_64)
      out_string->AppendString("qword ptr");
    else if (size == OperandSize_80)
      out_string->AppendString("tword ptr");
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

  // print size suffixes for string operations, since they don't actually include operands
  if (operation == Operation_CMPS || operation == Operation_LODS || operation == Operation_MOVS ||
      operation == Operation_SCAS || operation == Operation_STOS || operation == Operation_INS ||
      operation == Operation_OUTS)
  {
    out_string->AppendString(
      operand_size_postfixes[instruction->operands[0].size == OperandSize_Count ? instruction->GetOperandSize() :
                                                                                  instruction->operands[0].size]);
  }

  // print the operation suffix. this is things like jump conditions, segment names, etc.
  u32 operand_index = 0;
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

    const AddressSize asize = instruction->GetAddressSize();
    const OperandSize size = operand.size;
    switch (operand.mode)
    {
      case OperandMode_Constant:
      {
        if (size == OperandSize_8)
          out_string->AppendFormattedString("%02xh", operand.constant);
        else if (size == OperandSize_16)
          out_string->AppendFormattedString("%04xh", operand.constant);
        else if (size == OperandSize_32)
          out_string->AppendFormattedString("%08xh", operand.constant);
      }
      break;
      case OperandMode_Register:
      {
        if (size == OperandSize_8)
          out_string->AppendString(reg8_names[operand.reg8]);
        else if (size == OperandSize_16)
          out_string->AppendString(reg16_names[operand.reg8]);
        else if (size == OperandSize_32)
          out_string->AppendString(reg32_names[operand.reg32]);
      }
      break;
      case OperandMode_RegisterIndirect:
      {
        PrintPtr(size);
        out_string->AppendFormattedString(" %s:[", segment_names[instruction->data.segment]);
        if (asize == AddressSize_16)
          out_string->AppendString(reg16_names[operand.reg16]);
        else
          out_string->AppendString(reg32_names[operand.reg32]);
        out_string->AppendString("]");
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
        else if (size == OperandSize_32)
          out_string->AppendFormattedString("%08xh", ZeroExtend32(instruction->data.imm32));
      }
      break;
      case OperandMode_Immediate2:
      {
        if (size == OperandSize_8)
          out_string->AppendFormattedString("%02xh", ZeroExtend32(instruction->data.imm2_8));
        else if (size == OperandSize_16)
          out_string->AppendFormattedString("%04xh", ZeroExtend32(instruction->data.imm2_16));
        else if (size == OperandSize_32)
          out_string->AppendFormattedString("%08xh", ZeroExtend32(instruction->data.imm2_32));
      }
      break;
      case OperandMode_Relative:
      {
        VirtualMemoryAddress dst_address = instruction->address + instruction->length;
        if (instruction->GetOperandSize() == OperandSize_16)
        {
          dst_address = ZeroExtend32(Truncate16(Truncate16(dst_address) + instruction->data.disp16));
          out_string->AppendFormattedString("%04xh", dst_address);
        }
        else
        {
          dst_address = dst_address + instruction->data.disp32;
          out_string->AppendFormattedString("%08xh", dst_address);
        }
      }
      break;
      case OperandMode_Memory:
      {
        PrintPtr(size);
        out_string->AppendFormattedString(" %s:[", segment_names[instruction->data.segment]);
        if (asize == AddressSize_16)
          out_string->AppendFormattedString("%04xh", instruction->data.disp32);
        else
          out_string->AppendFormattedString("%08xh", instruction->data.disp32);
        out_string->AppendString("]");
      }
      break;
      case OperandMode_FarAddress:
      {
        out_string->AppendFormattedString("%04x:", ZeroExtend32(instruction->data.imm16));
        if (asize == AddressSize_16)
          out_string->AppendFormattedString("%04xh", instruction->data.disp32);
        else
          out_string->AppendFormattedString("%08xh", instruction->data.disp32);
      }
      break;
      case OperandMode_ModRM_Reg:
      {
        u8 index = instruction->GetModRM_Reg();
        if (size == OperandSize_8)
          out_string->AppendString(reg8_names[index]);
        else if (size == OperandSize_16)
          out_string->AppendString(reg16_names[index]);
        else if (size == OperandSize_32)
          out_string->AppendString(reg32_names[index]);
      }
      break;
      case OperandMode_ModRM_RM:
      {
        const ModRMAddress* m = DecodeModRMAddress(instruction->GetAddressSize(), instruction->data.modrm);
        if (m->addressing_mode == ModRMAddressingMode::Register)
        {
          out_string->AppendString((size == OperandSize_8) ?
                                     reg8_names[instruction->data.modrm_rm] :
                                     ((size == OperandSize_16) ? reg16_names[instruction->data.modrm_rm] :
                                                                 reg32_names[instruction->data.modrm_rm]));
        }
        else
        {
          PrintPtr(size);
          out_string->AppendFormattedString(" [%s:", segment_names[instruction->data.segment]);

          const char** reg_names = (asize == AddressSize_16) ? reg16_names : reg32_names;
          switch (m->addressing_mode)
          {
            case ModRMAddressingMode::Direct:
              out_string->AppendFormattedString((asize == AddressSize_16) ? "%04xh" : "%08xh",
                                                instruction->data.disp32);
              break;
            case ModRMAddressingMode::Indirect:
              out_string->AppendString(reg_names[m->base_register]);
              break;
            case ModRMAddressingMode::BasedIndexed:
              out_string->AppendFormattedString("%s + %s", reg_names[m->base_register], reg_names[m->index_register]);
              break;
            case ModRMAddressingMode::Indexed:
            {
              s32 disp =
                s32((asize == AddressSize_16) ? SignExtend32(instruction->data.disp16) : instruction->data.disp32);
              out_string->AppendFormattedString("%s %s%xh", reg_names[m->base_register], (disp < 0) ? "- " : "+ ",
                                                (disp < 0) ? -disp : disp);
            }
            break;
            case ModRMAddressingMode::BasedIndexedDisplacement:
            {
              s32 disp =
                s32((asize == AddressSize_16) ? SignExtend32(instruction->data.disp16) : instruction->data.disp32);
              out_string->AppendFormattedString("%s + %s %s%xh", reg_names[m->base_register],
                                                reg_names[m->index_register], (disp < 0) ? "- " : "+ ",
                                                (disp < 0) ? -disp : disp);
            }
            break;
            case ModRMAddressingMode::SIB:
            {
              bool first = true;
              if (instruction->data.HasSIBBase())
              {
                out_string->AppendString(reg_names[instruction->data.GetSIBBaseRegister()]);
                first = false;
              }

              if (instruction->data.HasSIBIndex())
              {
                out_string->AppendFormattedString("%s%s", first ? "" : " + ",
                                                  reg_names[instruction->data.GetSIBIndexRegister()]);
                if (instruction->data.GetSIBScaling() != 0)
                  out_string->AppendFormattedString(" * %u", (1u << instruction->data.GetSIBScaling()));
                first = false;
              }

              if (instruction->data.disp32 != 0)
              {
                s32 disp = s32(instruction->data.disp32);
                out_string->AppendFormattedString("%s%xh", first ? "" : (disp < 0 ? " - " : " + "),
                                                  (disp < 0) ? -disp : disp);
              }
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
        u8 index = instruction->GetModRM_Reg();
        out_string->AppendString((index > Segment_Count) ? "<invalid>" : segment_names[index]);
      }
      break;
      case OperandMode_ModRM_ControlRegister:
        out_string->AppendFormattedString("cr%u", instruction->data.GetModRM_Reg());
        break;
      case OperandMode_ModRM_DebugRegister:
        out_string->AppendFormattedString("dr%u", instruction->data.GetModRM_Reg());
        break;
      case OperandMode_ModRM_TestRegister:
        out_string->AppendFormattedString("tr%u", instruction->data.GetModRM_Reg());
        break;
      case OperandMode_FPRegister:
        out_string->AppendFormattedString("st(%u)", operand.data);
        break;
      default:
        break;
    }
  }
}

const Decoder::ModRMAddress* Decoder::DecodeModRMAddress(AddressSize address_size, u8 modrm)
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

  // This could probably be implemented procedurally
  // http://www.sandpile.org/x86/opc_rm.htm / http://www.sandpile.org/x86/opc_sib.htm
  static const ModRMAddress modrm_table_32[32] = {
    /* 00 000 - [eAX]             */ {ModRMAddressingMode::Indirect, Reg32_EAX, 0, 0, Segment_DS},
    /* 00 001 - [eCX]             */ {ModRMAddressingMode::Indirect, Reg32_ECX, 0, 0, Segment_DS},
    /* 00 010 - [eDX]             */ {ModRMAddressingMode::Indirect, Reg32_EDX, 0, 0, Segment_DS},
    /* 00 011 - [eBX]             */ {ModRMAddressingMode::Indirect, Reg32_EBX, 0, 0, Segment_DS},
    /* 00 100 - [sib]             */ {ModRMAddressingMode::SIB, 0, 0, 0, Segment_DS},
    /* 00 101 - [dword]           */ {ModRMAddressingMode::Direct, 0, 0, 4, Segment_DS},
    /* 00 110 - [eSI]             */ {ModRMAddressingMode::Indirect, Reg32_ESI, 0, 0, Segment_DS},
    /* 00 111 - [eDI]             */ {ModRMAddressingMode::Indirect, Reg32_EDI, 0, 0, Segment_DS},
    /* 01 000 - [eAX + sbyte]     */ {ModRMAddressingMode::Indexed, Reg32_EAX, 0, 1, Segment_DS},
    /* 01 001 - [eCX + sbyte]     */ {ModRMAddressingMode::Indexed, Reg32_ECX, 0, 1, Segment_DS},
    /* 01 010 - [eDX + sbyte]     */ {ModRMAddressingMode::Indexed, Reg32_EDX, 0, 1, Segment_DS},
    /* 01 011 - [eBX + sbyte]     */ {ModRMAddressingMode::Indexed, Reg32_EBX, 0, 1, Segment_DS},
    /* 01 100 - [sib + sbyte]     */ {ModRMAddressingMode::SIB, 0, 0, 1, Segment_DS},
    /* 01 101 - [eBP + sbyte]     */ {ModRMAddressingMode::Indexed, Reg32_EBP, 0, 1, Segment_SS},
    /* 01 110 - [eSI + sbyte]     */ {ModRMAddressingMode::Indexed, Reg32_ESI, 0, 1, Segment_DS},
    /* 01 111 - [eDI + sbyte]     */ {ModRMAddressingMode::Indexed, Reg32_EDI, 0, 1, Segment_DS},
    /* 10 000 - [eAX + sdword]    */ {ModRMAddressingMode::Indexed, Reg32_EAX, 0, 4, Segment_DS},
    /* 10 001 - [eCX + sdword]    */ {ModRMAddressingMode::Indexed, Reg32_ECX, 0, 4, Segment_DS},
    /* 10 010 - [eDX + sdword]    */ {ModRMAddressingMode::Indexed, Reg32_EDX, 0, 4, Segment_DS},
    /* 10 011 - [eBX + sdword]    */ {ModRMAddressingMode::Indexed, Reg32_EBX, 0, 4, Segment_DS},
    /* 10 100 - [sib + sdword]    */ {ModRMAddressingMode::SIB, 0, 0, 4, Segment_DS},
    /* 10 101 - [eBP + sdword]    */ {ModRMAddressingMode::Indexed, Reg32_EBP, 0, 4, Segment_SS},
    /* 10 110 - [eSI + sdword]    */ {ModRMAddressingMode::Indexed, Reg32_ESI, 0, 4, Segment_DS},
    /* 10 111 - [eDI + sdword]    */ {ModRMAddressingMode::Indexed, Reg32_EDI, 0, 4, Segment_DS},
    /* 11 000 - AL/AX/EAX         */ {ModRMAddressingMode::Register, Reg32_EAX, 0, 0, Segment_DS},
    /* 11 001 - CL/CX/ECX         */ {ModRMAddressingMode::Register, Reg32_ECX, 0, 0, Segment_DS},
    /* 11 010 - DL/DX/EDX         */ {ModRMAddressingMode::Register, Reg32_EDX, 0, 0, Segment_DS},
    /* 11 011 - BL/BX/EBX         */ {ModRMAddressingMode::Register, Reg32_EBX, 0, 0, Segment_DS},
    /* 11 100 - AH/SP/ESP         */ {ModRMAddressingMode::Register, Reg32_ESP, 0, 0, Segment_DS},
    /* 11 101 - CH/BP/EBP         */ {ModRMAddressingMode::Register, Reg32_EBP, 0, 0, Segment_DS},
    /* 11 110 - DH/SI/ESI         */ {ModRMAddressingMode::Register, Reg32_ESI, 0, 0, Segment_DS},
    /* 11 111 - BH/DI/EDI         */ {ModRMAddressingMode::Register, Reg32_EDI, 0, 0, Segment_DS},
  };

  u8 index = ((modrm & 0b11000000) >> 3) | (modrm & 0b00000111);
  return (address_size == AddressSize_16) ? &modrm_table_16[index] : &modrm_table_32[index];
}

} // namespace CPU_X86

#include "pce/cpu_x86/decoder_tables.inl"
