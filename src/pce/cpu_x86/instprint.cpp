#include "YBaseLib/String.h"
#include "pce/cpu_x86/cpu.h"
#include "pce/types.h"

namespace CPU_X86 {

static const char* opcode_names[Operation_Count] = {"<invalid>",
                                                    "<extension>",
                                                    "<extension mod/rm reg>",
                                                    "<extension x87>",
                                                    "<segment prefix>",
                                                    "<repeat prefix>",
                                                    "<repeat ne prefix>",
                                                    "<lock prefix>",
                                                    "<operand size override prefix>",
                                                    "<address size override prefix>",

                                                    "AAA",
                                                    "AAD",
                                                    "AAM",
                                                    "AAS",
                                                    "ADC",
                                                    "ADD",
                                                    "AND",
                                                    "CALL NEAR",
                                                    "CALL FAR",
                                                    "CBW",
                                                    "CLC",
                                                    "CLD",
                                                    "CLI",
                                                    "CMC",
                                                    "CMP",
                                                    "CMPS",
                                                    "CWD",
                                                    "DAA",
                                                    "DAS",
                                                    "DEC",
                                                    "DIV",
                                                    "ESC",
                                                    "HLT",
                                                    "IDIV",
                                                    "IMUL",
                                                    "IN",
                                                    "INC",
                                                    "INT",
                                                    "INTO",
                                                    "IRET",
                                                    "J",
                                                    "JCXZ",
                                                    "JMP NEAR",
                                                    "JMP FAR",
                                                    "LAHF",
                                                    "LDS",
                                                    "LEA",
                                                    "LES",
                                                    "LOCK",
                                                    "LODS",
                                                    "LOOP",
                                                    "LXS",
                                                    "MOV",
                                                    "MOVS",
                                                    "MOV",
                                                    "MUL",
                                                    "NEG",
                                                    "NOP",
                                                    "NOT",
                                                    "OR",
                                                    "OUT",
                                                    "POP",
                                                    "POP",
                                                    "POPF",
                                                    "PUSH",
                                                    "PUSH",
                                                    "PUSHF",
                                                    "RCL",
                                                    "RCR",
                                                    "REPxx",
                                                    "RET NEAR",
                                                    "RET FAR",
                                                    "ROL",
                                                    "ROR",
                                                    "SAHF",
                                                    "SAL",
                                                    "SAR",
                                                    "SBB",
                                                    "SCAS",
                                                    "SHL",
                                                    "SHR",
                                                    "STC",
                                                    "STD",
                                                    "STI",
                                                    "STOS",
                                                    "SUB",
                                                    "TEST",
                                                    "WAIT",
                                                    "XCHG",
                                                    "XLAT",
                                                    "XOR",

                                                    "BOUND",
                                                    "BSF",
                                                    "BSR",
                                                    "BT",
                                                    "BTS",
                                                    "BTR",
                                                    "BTC",
                                                    "SHLD",
                                                    "SHRD",
                                                    "INS",
                                                    "OUTS",
                                                    "CLTS",
                                                    "ENTER",
                                                    "LEAVE",
                                                    "LSS",
                                                    "LGDT",
                                                    "SGDT",
                                                    "LIDT",
                                                    "SIDT",
                                                    "LLDT",
                                                    "SDLT",
                                                    "LTR",
                                                    "STR",
                                                    "LMSW",
                                                    "SMSW",
                                                    "VERR",
                                                    "VERW",
                                                    "ARPL",
                                                    "LAR",
                                                    "LSL",
                                                    "PUSHA",
                                                    "POPA",
                                                    "LOADALL_286",

                                                    "LFS",
                                                    "LGS",
                                                    "MOVSX",
                                                    "MOVZX",
                                                    "MOV",
                                                    "MOV",
                                                    "MOV",
                                                    "SET",

                                                    "BSWAP",
                                                    "CMPXCHG",
                                                    "CMOV",
                                                    "WBINVD",
                                                    "INVLPG",
                                                    "XADD",

                                                    "RDTSC",

                                                    "F2XM1",
                                                    "FABS",
                                                    "FADD",
                                                    "FADDP",
                                                    "FBLD",
                                                    "FBSTP",
                                                    "FCHS",
                                                    "FCLEX",
                                                    "FCOM",
                                                    "FCOMP",
                                                    "FCOMPP",
                                                    "FDECSTP",
                                                    "FDISI",
                                                    "FDIV",
                                                    "FDIVP",
                                                    "FDIVR",
                                                    "FDIVRP",
                                                    "FENI",
                                                    "FFREE",
                                                    "FIADD",
                                                    "FICOM",
                                                    "FICOMP",
                                                    "FIDIV",
                                                    "FIDIVR",
                                                    "FILD",
                                                    "FIMUL",
                                                    "FINCSTP",
                                                    "FINIT",
                                                    "FIST",
                                                    "FISTP",
                                                    "FISUB",
                                                    "FISUBR",
                                                    "FLD",
                                                    "FLD1",
                                                    "FLDCW",
                                                    "FLDENV",
                                                    "FLDL2E",
                                                    "FLDL2T",
                                                    "FLDLG2",
                                                    "FLDLN2",
                                                    "FLDPI",
                                                    "FLDZ",
                                                    "FMUL",
                                                    "FMULP",
                                                    "FNCLEX",
                                                    "FNDISI",
                                                    "FNENI",
                                                    "FNINIT",
                                                    "FNOP",
                                                    "FNSAVE",
                                                    "FNSTCW",
                                                    "FNSTENV",
                                                    "FNSTSW",
                                                    "FPATAN",
                                                    "FPREM",
                                                    "FPTAN",
                                                    "FRNDINT",
                                                    "FRSTOR",
                                                    "FSCALE",
                                                    "FSQRT",
                                                    "FST",
                                                    "FSTCW",
                                                    "FSTENV",
                                                    "FSTP",
                                                    "FSTSW",
                                                    "FSUB",
                                                    "FSUBP",
                                                    "FSUBR",
                                                    "FSUBRP",
                                                    "FTST",
                                                    "FXAM",
                                                    "FXCH",
                                                    "FXTRACT",
                                                    "FYL2X",
                                                    "FYL2XP1",

                                                    "FSETPM",

                                                    "FCOS",
                                                    "FPREM1",
                                                    "FSIN",
                                                    "FSINCOS",
                                                    "FUCOM",
                                                    "FUCOMP",
                                                    "FUCOMPP"};

static const char* reg8_names[Reg8_Count] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};

static const char* reg16_names[Reg16_Count] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};

static const char* reg32_names[Reg32_Count] = {"EAX",    "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI", "EIP",
                                               "EFLAGS", "CR0", "CR2", "CR3", "CR4", "DR0", "DR1", "DR2", "DR3",
                                               "DR4",    "DR5", "DR6", "DR7", "TR3", "TR4", "TR5", "TR6", "TR7"};

static const char* segment_names[Segment_Count] = {"ES", "CS", "SS", "DS", "FS", "GS"};

static const char* jump_condition_names[JumpCondition_Count] = {"",   "O", "NO", "S",  "NS", "E", "NE", "B",  "AE",
                                                                "BE", "A", "L",  "GE", "LE", "G", "P",  "NP", "CXZ"};

static const char* operand_size_postfixes[OperandSize_32 + 1] = {"B", "W", "D"};

static bool ShouldHaveTrailingSize(Operation operation)
{
  // Some operations need trailing sizes to determine the operand size
  switch (operation)
  {
    case Operation_CMPS:
    case Operation_LODS:
    case Operation_MOVS:
    case Operation_SCAS:
    case Operation_STOS:
    case Operation_INS:
    case Operation_OUTS:
      return true;

    default:
      return false;
  }
}

static void PrintPtr(OperandSize mode, String* out_string)
{
  if (mode == OperandSize_8)
    out_string->AppendString("BYTE PTR");
  else if (mode == OperandSize_16)
    out_string->AppendString("WORD PTR");
  else if (mode == OperandSize_32)
    out_string->AppendString("DWORD PTR");
  else if (mode == OperandSize_64)
    out_string->AppendString("QWORD PTR");
  else if (mode == OperandSize_80)
    out_string->AppendString("TWORD PTR");
}

static void PrintSegment(Segment segment, String* out_string)
{
  if (segment == Segment_Count)
    return;

  out_string->AppendFormattedString("%s:", segment_names[segment]);
}

static void PrintRegister(OperandSize mode, uint8 index, String* out_string)
{
  if (mode == OperandSize_8)
  {
    DebugAssert(index < countof(reg8_names));
    out_string->AppendString(reg8_names[index]);
  }
  else if (mode == OperandSize_16)
  {
    DebugAssert(index < countof(reg16_names));
    out_string->AppendString(reg16_names[index]);
  }
  else if (mode == OperandSize_32)
  {
    DebugAssert(index < countof(reg32_names));
    out_string->AppendString(reg32_names[index]);
  }
}

static void PrintIndirectRegister(AddressSize address_size, uint8 index, String* out_string)
{
  if (address_size == AddressSize_16)
  {
    DebugAssert(index < countof(reg16_names));
    out_string->AppendString(reg16_names[index]);
  }
  else if (address_size == AddressSize_32)
  {
    DebugAssert(index < countof(reg32_names));
    out_string->AppendString(reg32_names[index]);
  }
}

static void PrintOperand(const OldInstruction* instruction, const OldInstruction::Operand* operand,
                         VirtualMemoryAddress physical_address, String* out_string)
{
  switch (operand->mode)
  {
    case AddressingMode_Immediate:
    {
      if (operand->size == OperandSize_8)
        out_string->AppendFormattedString("%02Xh", operand->immediate.value8);
      else if (operand->size == OperandSize_16)
        out_string->AppendFormattedString("%04Xh", operand->immediate.value16);
      else if (operand->size == OperandSize_32)
        out_string->AppendFormattedString("%08Xh", operand->immediate.value32);
    }
    break;

    case AddressingMode_Register:
    {
      PrintRegister(operand->size, operand->reg.raw, out_string);
    }
    break;

    case AddressingMode_SegmentRegister:
    {
      DebugAssert(operand->reg.sreg < Segment_Count);
      out_string->AppendString(segment_names[operand->reg.sreg]);
    }
    break;

    case AddressingMode_ControlRegister:
      out_string->AppendFormattedString("CR%u", ZeroExtend32(operand->reg.raw));
      break;

    case AddressingMode_DebugRegister:
      out_string->AppendFormattedString("DR%u", ZeroExtend32(operand->reg.raw));
      break;

    case AddressingMode_TestRegister:
      out_string->AppendFormattedString("TR%u", ZeroExtend32(operand->reg.raw));
      break;

    case AddressingMode_RegisterIndirect:
    {
      PrintPtr(operand->size, out_string);
      out_string->AppendString(" [");
      PrintSegment(instruction->segment, out_string);
      PrintIndirectRegister(instruction->address_size, operand->reg.raw, out_string);
      out_string->AppendString("]");
    }
    break;

    case AddressingMode_FarAddress:
    {
      out_string->AppendFormattedString("%04X:", operand->far_address.segment_selector);
      if (operand->size == OperandSize_16)
        out_string->AppendFormattedString("%04X", operand->far_address.address);
      else if (operand->size == OperandSize_32)
        out_string->AppendFormattedString("%08X", operand->far_address.address);
    }
    break;

    case AddressingMode_Direct:
    {
      PrintPtr(operand->size, out_string);
      out_string->AppendString(" [");
      PrintSegment(instruction->segment, out_string);
      if (instruction->address_size == AddressSize_16)
        out_string->AppendFormattedString("%04Xh", operand->direct.address);
      else
        out_string->AppendFormattedString("%08Xh", operand->direct.address);
      out_string->AppendString("]");
    }
    break;

    case AddressingMode_Indexed:
    {
      PrintPtr(operand->size, out_string);
      out_string->AppendString(" [");
      PrintSegment(instruction->segment, out_string);
      out_string->AppendFormattedString("%X + ", operand->indexed.displacement); // signed??
      PrintIndirectRegister(instruction->address_size, operand->indexed.reg.raw, out_string);
      out_string->AppendString("]");
    }
    break;

    case AddressingMode_BasedIndexed:
    {
      PrintPtr(operand->size, out_string);
      out_string->AppendString(" [");
      PrintSegment(instruction->segment, out_string);
      PrintIndirectRegister(instruction->address_size, operand->based_indexed.base.raw, out_string);
      out_string->AppendString(" + ");
      PrintIndirectRegister(instruction->address_size, operand->based_indexed.index.raw, out_string);
      out_string->AppendString("]");
    }
    break;

    case AddressingMode_BasedIndexedDisplacement:
    {
      PrintPtr(operand->size, out_string);
      out_string->AppendString(" [");
      PrintSegment(instruction->segment, out_string);
      out_string->AppendFormattedString("%Xh + ", operand->indexed.displacement); // signed??
      PrintIndirectRegister(instruction->address_size, operand->based_indexed.base.raw, out_string);
      out_string->AppendString(" + ");
      PrintIndirectRegister(instruction->address_size, operand->based_indexed.index.raw, out_string);
      out_string->AppendString("]");
    }
    break;

    case AddressingMode_Relative:
    {
      VirtualMemoryAddress dst_address = physical_address + instruction->length;
      if (instruction->operand_size == OperandSize_16)
      {
        dst_address = ZeroExtend32(Truncate16(dst_address) + Truncate16(uint32(operand->relative.displacement)));
        out_string->AppendFormattedString("%04Xh", dst_address);
      }
      else
      {
        dst_address = dst_address + uint32(operand->relative.displacement);
        out_string->AppendFormattedString("%08Xh", dst_address);
      }
    }
    break;

    case AddressingMode_SIB:
    {
      PrintPtr(operand->size, out_string);
      out_string->AppendString(" [");
      PrintSegment(instruction->segment, out_string);
      out_string->AppendFormattedString("%Xh", operand->sib.displacement); // signed??
      if (operand->sib.base.reg32 != Reg32_Count)
      {
        out_string->AppendString(" + ");
        PrintIndirectRegister(instruction->address_size, operand->sib.base.reg32, out_string);
      }
      if (operand->sib.index.reg32 != Reg32_Count)
      {
        out_string->AppendString(" + ");
        PrintIndirectRegister(instruction->address_size, operand->sib.index.reg32, out_string);
        out_string->AppendFormattedString(" * %u", UINT32_C(1) << operand->sib.scale_shift);
      }
      out_string->AppendString("]");
    }
    break;

    case AddressingMode_ST:
      out_string->AppendFormattedString("ST(%u)", ZeroExtend32(operand->st.index));
      break;
  }
}

bool DisassembleToString(const OldInstruction* instruction, VirtualMemoryAddress physical_address, String* out_string)
{
  out_string->Clear();

  if (instruction->flags & InstructionFlag_Lock)
    out_string->AppendString("LOCK ");

  if (instruction->flags & InstructionFlag_RepEqual)
    out_string->AppendString("REPE ");
  else if (instruction->flags & InstructionFlag_RepNotEqual)
    out_string->AppendString("REPNE ");
  else if (instruction->flags & InstructionFlag_Rep)
    out_string->AppendString("REP ");

  out_string->AppendString(opcode_names[instruction->operation]);

  if (ShouldHaveTrailingSize(instruction->operation))
    out_string->AppendString(operand_size_postfixes[instruction->operands[0].size]);

  if (instruction->jump_condition != JumpCondition_Always)
    out_string->AppendString(jump_condition_names[instruction->jump_condition]);

  out_string->AppendCharacter(' ');

  for (size_t i = 0; i < countof(instruction->operands); i++)
  {
    if (instruction->operands[i].mode != AddressingMode_None)
    {
      if (i > 0)
        out_string->AppendString(", ");

      PrintOperand(instruction, &instruction->operands[i], physical_address, out_string);
    }
  }

  return true;
}

} // namespace CPU_X86