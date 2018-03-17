#include "YBaseLib/Endian.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu_x86/interpreter.h"
Log_SetChannel(CPU_X86::Interpreter);

namespace CPU_X86 {
void Interpreter::StartX87Instruction(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->m_registers.CR0 & (CR0Bit_EM | CR0Bit_TS))
  {
    cpu->RaiseException(Interrupt_CoprocessorNotAvailable);
    return;
  }

  // If no FPU
  cpu->AbortCurrentInstruction();
}

floatx80 Interpreter::ReadFloatOperand(CPU* cpu, const OldInstruction* instruction,
                                       const OldInstruction::Operand* operand, float_status* fs)
{
  if (operand->mode == AddressingMode_ST)
  {
    // These are all 80-bit already.
    CheckFloatStackUnderflow(cpu, operand->st.index);
    return ReadFloatRegister(cpu, operand->st.index);
  }

  // Only memory-based addressing is valid.
  DebugAssert(operand->mode == AddressingMode_RegisterIndirect ||
              (operand->mode >= AddressingMode_Direct && operand->mode <= AddressingMode_SIB));
  VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, operand);
  floatx80 ret;
  switch (operand->size)
  {
    case OperandSize_32:
    {
      // Convert single precision -> extended precision.
      uint32 dword_val = cpu->ReadMemoryDWord(instruction->segment, address);
      ret = float32_to_floatx80(dword_val, fs);
    }
    break;

    case OperandSize_64:
    {
      // Convert double precision -> extended precision
      uint32 dword_val_low = cpu->ReadMemoryDWord(instruction->segment, address);
      uint32 dword_val_high = cpu->ReadMemoryDWord(instruction->segment, address + 4);
      float64 qword_val = (ZeroExtend64(dword_val_high) << 32) | ZeroExtend64(dword_val_low);
      ret = float64_to_floatx80(qword_val, fs);
    }
    break;

    case OperandSize_80:
    {
      // Load 80-bit extended precision. Does not check for SNaNs.
      uint32 tword_val_low = cpu->ReadMemoryDWord(instruction->segment, address);
      uint32 tword_val_middle = cpu->ReadMemoryDWord(instruction->segment, address + 4);
      uint16 tword_val_high = cpu->ReadMemoryWord(instruction->segment, address + 8);
      ret.low = (ZeroExtend64(tword_val_middle) << 32) | ZeroExtend64(tword_val_low);
      ret.high = tword_val_high;
    }
    break;

    default:
      UnreachableCode();
      ret = {};
      break;
  }

  return ret;
}

floatx80 Interpreter::ReadIntegerOperandAsFloat(CPU* cpu, const OldInstruction* instruction,
                                                const OldInstruction::Operand* operand, float_status* fs)
{
  int64 int_value = 0;
  switch (operand->size)
  {
    case OperandSize_16:
      int_value = SignExtend64(ReadSignExtendedWordOperand(cpu, instruction, operand));
      break;

    case OperandSize_32:
      int_value = SignExtend64(ReadSignExtendedDWordOperand(cpu, instruction, operand));
      break;

    case OperandSize_64:
    {
      VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, operand);
      uint32 low = cpu->ReadMemoryDWord(instruction->segment, address);
      uint32 high = cpu->ReadMemoryDWord(instruction->segment, address + 4);
      int_value = int64((ZeroExtend64(high) << 32) | ZeroExtend64(low));
    }
    break;

    case OperandSize_Count:
      UnreachableCode();
      break;
  }

  return int64_to_floatx80(int_value, fs);
}

void Interpreter::WriteFloatOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand,
                                    float_status* fs, const floatx80& value)
{
  if (operand->mode == AddressingMode_ST)
  {
    // These are all 80-bit already.
    WriteFloatRegister(cpu, operand->st.index, value, true);
    return;
  }

  // Only memory-based addressing is valid.
  DebugAssert(operand->mode == AddressingMode_RegisterIndirect ||
              (operand->mode >= AddressingMode_Direct && operand->mode <= AddressingMode_SIB));
  VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, operand);
  switch (operand->size)
  {
    case OperandSize_32:
    {
      // Convert single precision -> extended precision.
      uint32 dword_val = floatx80_to_float32(value, fs);
      cpu->WriteMemoryDWord(instruction->segment, address, dword_val);
    }
    break;

    case OperandSize_64:
    {
      // Convert double precision -> extended precision
      uint64 qword_val = floatx80_to_float64(value, fs);
      cpu->WriteMemoryDWord(instruction->segment, address, Truncate32(qword_val));
      cpu->WriteMemoryDWord(instruction->segment, address + 4, Truncate32(qword_val >> 32));
    }
    break;

    case OperandSize_80:
    {
      // Load 80-bit extended precision.
      cpu->WriteMemoryDWord(instruction->segment, address, Truncate32(value.low));
      cpu->WriteMemoryDWord(instruction->segment, address + 4, Truncate32(value.low >> 32));
      cpu->WriteMemoryDWord(instruction->segment, address + 8, value.high);
    }
    break;

    default:
      UnreachableCode();
      break;
  }
}

void Interpreter::CheckFloatStackOverflow(CPU* cpu)
{
  // overflow = write, underflow = read
  uint8 index = (cpu->m_fpu_registers.SW.TOP - 1) & 7;
  if (!cpu->m_fpu_registers.TW.IsEmpty(index))
  {
    cpu->m_fpu_registers.SW.C1 = 1;
    cpu->m_fpu_registers.SW.SF = true;
    cpu->m_fpu_registers.SW.I = true;
    cpu->AbortCurrentInstruction();
  }
}

void Interpreter::CheckFloatStackUnderflow(CPU* cpu, uint8 relative_index)
{
  uint8 index = (cpu->m_fpu_registers.SW.TOP + relative_index) & 7;
  if (cpu->m_fpu_registers.TW.IsEmpty(index))
  {
    cpu->m_fpu_registers.SW.C1 = 0;
    cpu->m_fpu_registers.SW.SF = true;
    cpu->m_fpu_registers.SW.I = true;
    cpu->AbortCurrentInstruction();
  }
}

void Interpreter::PushFloatStack(CPU* cpu)
{
  uint8 top = (cpu->m_fpu_registers.SW.TOP - 1) & 7;
  cpu->m_fpu_registers.SW.TOP = top;
}

void Interpreter::PopFloatStack(CPU* cpu)
{
  uint8 top = cpu->m_fpu_registers.SW.TOP;
  cpu->m_fpu_registers.SW.TOP = (top + 1) & 7;
  cpu->m_fpu_registers.TW.SetEmpty(top);
}

floatx80 Interpreter::ReadFloatRegister(CPU* cpu, uint8 relative_index)
{
  uint8 index = (cpu->m_fpu_registers.SW.TOP + relative_index) & 7;
  floatx80 ret;
  std::memcpy(&ret, &cpu->m_fpu_registers.ST[index], sizeof(ret));
  return ret;
}

void Interpreter::WriteFloatRegister(CPU* cpu, uint8 relative_index, const floatx80& value,
                                     bool update_tag /* = true */)
{
  uint8 index = (cpu->m_fpu_registers.SW.TOP + relative_index) & 7;
  std::memcpy(&cpu->m_fpu_registers.ST[index], &value, sizeof(float80));
  if (update_tag)
    UpdateFloatTagRegister(cpu, index);
}

void Interpreter::UpdateFloatTagRegister(CPU* cpu, uint8 index)
{
  // TODO: Defer updating tag word until FXSAVE
  const floatx80& fx80 = reinterpret_cast<const floatx80&>(cpu->m_fpu_registers.ST[index]);
  if (floatx80_is_zero(fx80))
  {
    cpu->m_fpu_registers.TW.SetZero(index);
  }
  else
  {
    uint8 bits = floatx80_invalid_encoding(fx80) ? 1 : 0;
    if (floatx80_is_any_nan(fx80) || floatx80_is_infinity(fx80) || floatx80_is_zero_or_denormal(fx80))
      bits |= 2;
    cpu->m_fpu_registers.TW.Set(index, bits);
  }
}

float_status Interpreter::GetFloatStatus(CPU* cpu)
{
  float_status ret;
  ret.float_detect_tininess = float_tininess_after_rounding;
  ret.float_rounding_mode = float_round_nearest_even;
  ret.float_exception_flags = 0;
  ret.floatx80_rounding_precision = 0;
  ret.flush_to_zero = 0;
  ret.flush_inputs_to_zero = 0;
  ret.default_nan_mode = 0;
  ret.snan_bit_is_one = 0;
  return ret;
}

void Interpreter::RaiseFloatExceptions(CPU* cpu, const float_status& fs)
{
  if (fs.float_exception_flags == 0)
    return;

  bool abort = false;

  if (fs.float_exception_flags & float_flag_invalid)
  {
    cpu->m_fpu_registers.SW.I = true;
    abort |= !cpu->m_fpu_registers.CW.IM;
  }

  if (fs.float_exception_flags & float_flag_divbyzero)
  {
    cpu->m_fpu_registers.SW.Z = true;
    abort |= !cpu->m_fpu_registers.CW.ZM;
  }

  if (fs.float_exception_flags & float_flag_overflow)
  {
    cpu->m_fpu_registers.SW.O = true;
    abort |= !cpu->m_fpu_registers.CW.OM;
  }

  if (fs.float_exception_flags & float_flag_underflow)
  {
    cpu->m_fpu_registers.SW.U = true;
    abort |= !cpu->m_fpu_registers.CW.UM;
  }

  if (fs.float_exception_flags & float_flag_inexact)
  {
    cpu->m_fpu_registers.SW.P = true;
    abort |= !cpu->m_fpu_registers.CW.PM;
  }

  if (fs.float_exception_flags & (float_flag_input_denormal | float_flag_output_denormal))
  {
    cpu->m_fpu_registers.SW.D = true;
    abort |= !cpu->m_fpu_registers.CW.DM;
  }

  if (abort)
  {
    cpu->m_fpu_exception = true;
    cpu->AbortCurrentInstruction();
  }
}

extern bool TRACE_EXECUTION;

void Interpreter::Execute_FNINIT(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  cpu->m_fpu_registers.CW.bits = 0x037F;
  cpu->m_fpu_registers.SW.bits = 0;
  cpu->m_fpu_registers.TW.bits = 0xFFFF;
  // cpu->m_fpu_registers.DP

  // TRACE_EXECUTION = true;
}

void Interpreter::Execute_FSETPM(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);
}

void Interpreter::Execute_FNSTCW(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);
  WriteWordOperand(cpu, instruction, &instruction->operands[0], cpu->m_fpu_registers.CW.bits);
}

void Interpreter::Execute_FNSTSW(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);
  WriteWordOperand(cpu, instruction, &instruction->operands[0], cpu->m_fpu_registers.SW.bits);
}

void Interpreter::Execute_FNCLEX(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  // FPUStatusWord[0:7] <- 0;
  // FPUStatusWord[15] <- 0;
  cpu->m_fpu_registers.SW.bits &= ~uint16((1 << 15) | 0b11111111);
}

void Interpreter::Execute_FLDCW(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  uint16 cw = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
  cpu->m_fpu_registers.CW.bits = cw;
}

void Interpreter::Execute_FLD(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);
  CheckFloatStackOverflow(cpu);

  float_status fs = GetFloatStatus(cpu);
  floatx80 value = ReadFloatOperand(cpu, instruction, &instruction->operands[0], &fs);
  RaiseFloatExceptions(cpu, fs);

  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0, value, true);
}

void Interpreter::Execute_FLD1(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);
  CheckFloatStackOverflow(cpu);
  PushFloatStack(cpu);

  static constexpr floatx80 value = {0x3FFF800000000000ULL, 0};
  WriteFloatRegister(cpu, 0, value, true);
}

void Interpreter::Execute_FLDZ(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);
  CheckFloatStackOverflow(cpu);
  PushFloatStack(cpu);

  static constexpr floatx80 value = {0, 0};
  WriteFloatRegister(cpu, 0, value, true);
}

void Interpreter::Execute_FST(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  CheckFloatStackUnderflow(cpu, 0);
  floatx80 value = ReadFloatRegister(cpu, 0);

  float_status fs = GetFloatStatus(cpu);
  WriteFloatOperand(cpu, instruction, &instruction->operands[0], &fs, value);
  RaiseFloatExceptions(cpu, fs);

  if (instruction->operation == Operation_FSTP)
    PopFloatStack(cpu);
}

void Interpreter::Execute_FADD(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  float_status fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand(cpu, instruction, &instruction->operands[0], &fs);
  floatx80 rhs = ReadFloatOperand(cpu, instruction, &instruction->operands[1], &fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_add(lhs, rhs, &fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand(cpu, instruction, &instruction->operands[0], &fs, res);

  if (instruction->operation == Operation_FADDP)
    PopFloatStack(cpu);
}

void Interpreter::Execute_FSUB(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  float_status fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand(cpu, instruction, &instruction->operands[0], &fs);
  floatx80 rhs = ReadFloatOperand(cpu, instruction, &instruction->operands[1], &fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_sub(lhs, rhs, &fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand(cpu, instruction, &instruction->operands[0], &fs, res);

  if (instruction->operation == Operation_FSUBP)
    PopFloatStack(cpu);
}

void Interpreter::Execute_FSUBR(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  float_status fs = GetFloatStatus(cpu);
  floatx80 rhs = ReadFloatOperand(cpu, instruction, &instruction->operands[0], &fs);
  floatx80 lhs = ReadFloatOperand(cpu, instruction, &instruction->operands[1], &fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_sub(lhs, rhs, &fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand(cpu, instruction, &instruction->operands[0], &fs, res);

  if (instruction->operation == Operation_FSUBRP)
    PopFloatStack(cpu);
}

void Interpreter::Execute_FMUL(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  float_status fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand(cpu, instruction, &instruction->operands[0], &fs);
  floatx80 rhs = ReadFloatOperand(cpu, instruction, &instruction->operands[1], &fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_mul(lhs, rhs, &fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand(cpu, instruction, &instruction->operands[0], &fs, res);

  if (instruction->operation == Operation_FMULP)
    PopFloatStack(cpu);
}

void Interpreter::Execute_FDIV(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  float_status fs = GetFloatStatus(cpu);
  floatx80 dividend = ReadFloatOperand(cpu, instruction, &instruction->operands[0], &fs);
  floatx80 divisor = ReadFloatOperand(cpu, instruction, &instruction->operands[1], &fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_div(dividend, divisor, &fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand(cpu, instruction, &instruction->operands[0], &fs, res);

  if (instruction->operation == Operation_FDIVP)
    PopFloatStack(cpu);
}

void Interpreter::Execute_FDIVR(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  float_status fs = GetFloatStatus(cpu);
  floatx80 divisor = ReadFloatOperand(cpu, instruction, &instruction->operands[0], &fs);
  floatx80 dividend = ReadFloatOperand(cpu, instruction, &instruction->operands[1], &fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_div(dividend, divisor, &fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand(cpu, instruction, &instruction->operands[0], &fs, res);

  if (instruction->operation == Operation_FDIVRP)
    PopFloatStack(cpu);
}

void Interpreter::Execute_FCOM(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  float_status fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand(cpu, instruction, &instruction->operands[0], &fs);
  floatx80 rhs = ReadFloatOperand(cpu, instruction, &instruction->operands[1], &fs);
  RaiseFloatExceptions(cpu, fs);

  int res;

  // TODO: Abuse templates to generate smaller code here.
  if (instruction->operation >= Operation_FUCOM && instruction->operation <= Operation_FUCOMPP)
    res = floatx80_compare_quiet(lhs, rhs, &fs);
  else
    res = floatx80_compare(lhs, rhs, &fs);

  // Condition flags should be set to unordered if an unmasked exception occurs.
  cpu->m_fpu_registers.SW.C3 = 1;
  cpu->m_fpu_registers.SW.C2 = 1;
  cpu->m_fpu_registers.SW.C0 = 1;
  RaiseFloatExceptions(cpu, fs);

  if (res < 0)
  {
    // less
    cpu->m_fpu_registers.SW.C3 = 0;
    cpu->m_fpu_registers.SW.C2 = 0;
    cpu->m_fpu_registers.SW.C0 = 1;
  }
  else if (res == 0)
  {
    // equal
    cpu->m_fpu_registers.SW.C3 = 1;
    cpu->m_fpu_registers.SW.C2 = 0;
    cpu->m_fpu_registers.SW.C0 = 0;
  }
  else if (res != float_relation_unordered)
  {
    // greater
    cpu->m_fpu_registers.SW.C3 = 0;
    cpu->m_fpu_registers.SW.C2 = 0;
    cpu->m_fpu_registers.SW.C0 = 0;
  }

  if (instruction->operation == Operation_FCOMP || instruction->operation == Operation_FUCOMP)
  {
    PopFloatStack(cpu);
  }
  else if (instruction->operation == Operation_FCOMPP || instruction->operation == Operation_FUCOMPP)
  {
    PopFloatStack(cpu);
    PopFloatStack(cpu);
  }
}

void Interpreter::Execute_FILD(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);
  CheckFloatStackOverflow(cpu);

  float_status fs = GetFloatStatus(cpu);
  floatx80 value = ReadIntegerOperandAsFloat(cpu, instruction, &instruction->operands[0], &fs);
  RaiseFloatExceptions(cpu, fs);

  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0, value, true);
}

void Interpreter::Execute_FIST(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  CheckFloatStackUnderflow(cpu, 0);
  floatx80 value = ReadFloatRegister(cpu, 0);

  float_status fs = GetFloatStatus(cpu);
  switch (instruction->operands[0].size)
  {
    case OperandSize_16:
    {
      int32 int_value = floatx80_to_int32(value, &fs);
      if (int_value < -32768 || int_value > 32767)
      {
        fs.float_exception_flags = float_flag_invalid;
        int_value = 0x8000;
      }
      RaiseFloatExceptions(cpu, fs);
      WriteWordOperand(cpu, instruction, &instruction->operands[0], uint16(int16(int_value)));
    }
    break;

    case OperandSize_32:
    {
      int32 int_value = floatx80_to_int32(value, &fs);
      RaiseFloatExceptions(cpu, fs);
      WriteDWordOperand(cpu, instruction, &instruction->operands[0], uint32(int_value));
    }
    break;

    case OperandSize_64:
    {
      int64 int_value = floatx80_to_int64(value, &fs);
      RaiseFloatExceptions(cpu, fs);

      VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, &instruction->operands[0]);
      cpu->WriteMemoryDWord(instruction->segment, address, Truncate32(uint64(int_value)));
      cpu->WriteMemoryDWord(instruction->segment, address + 4, Truncate32(uint64(int_value) >> 32));
    }
    break;

    case OperandSize_Count:
      UnreachableCode();
      return;
  }

  if (instruction->operation == Operation_FISTP)
    PopFloatStack(cpu);
}

void Interpreter::Execute_FIDIV(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  float_status fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand(cpu, instruction, &instruction->operands[0], &fs);
  floatx80 rhs = ReadIntegerOperandAsFloat(cpu, instruction, &instruction->operands[1], &fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_div(lhs, rhs, &fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand(cpu, instruction, &instruction->operands[0], &fs, res);
}

void Interpreter::Execute_FABS(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  CheckFloatStackUnderflow(cpu, 0);

  floatx80 val = ReadFloatRegister(cpu, 0);
  val = floatx80_abs(val);
  WriteFloatRegister(cpu, 0, val, true);

  cpu->m_fpu_registers.SW.C1 = 0;
}

void Interpreter::Execute_FCHS(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  CheckFloatStackUnderflow(cpu, 0);

  floatx80 val = ReadFloatRegister(cpu, 0);
  val = floatx80_chs(val);
  WriteFloatRegister(cpu, 0, val, true);

  cpu->m_fpu_registers.SW.C1 = 0;
}

void Interpreter::Execute_FFREE(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  DebugAssert(instruction->operands[0].mode == AddressingMode_ST);
  uint8 index = instruction->operands[0].st.index;
  cpu->m_fpu_registers.TW.SetEmpty(index);
}

void Interpreter::Execute_FPREM(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  CheckFloatStackUnderflow(cpu, 0);
  CheckFloatStackUnderflow(cpu, 1);

  floatx80 dividend = ReadFloatRegister(cpu, 0);
  floatx80 divisor = ReadFloatRegister(cpu, 1);
}

void Interpreter::Execute_FXAM(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  // Empty values are accepted here.
  floatx80 val = ReadFloatRegister(cpu, 0);

  uint8 C0, C1, C2, C3;

  // C1 <- SignBit(ST(0))
  C1 = uint8(val.high >> 15);

  // GetClass(ST(0))
  if (cpu->m_fpu_registers.TW.IsEmpty(cpu->m_fpu_registers.SW.TOP))
  {
    // Empty
    C3 = 1;
    C2 = 0;
    C0 = 1;
  }
  else if (floatx80_invalid_encoding(val))
  {
    // Unsupported
    C3 = 0;
    C2 = 0;
    C0 = 0;
  }
  else if (floatx80_is_infinity(val))
  {
    // Infinity
    C3 = 1;
    C2 = 0;
    C0 = 0;
  }
  else if (floatx80_is_zero(val))
  {
    // Zero
    C3 = 1;
    C2 = 0;
    C0 = 0;
  }
  else if (floatx80_is_zero_or_denormal(val))
  {
    // Denormal
    C3 = 1;
    C2 = 1;
    C0 = 0;
  }
  else
  {
    // Normal
    C3 = 0;
    C2 = 1;
    C0 = 0;
  }

  cpu->m_fpu_registers.SW.C0 = C0;
  cpu->m_fpu_registers.SW.C1 = C1;
  cpu->m_fpu_registers.SW.C2 = C2;
  cpu->m_fpu_registers.SW.C3 = C3;
}

void Interpreter::Execute_FXCH(CPU* cpu, const OldInstruction* instruction)
{
  StartX87Instruction(cpu, instruction);

  DebugAssert(instruction->operands[0].mode == AddressingMode_ST);

  uint8 index = instruction->operands[0].st.index;
  CheckFloatStackUnderflow(cpu, 0);
  CheckFloatStackUnderflow(cpu, index);

  floatx80 st0 = ReadFloatRegister(cpu, 0);
  floatx80 stn = ReadFloatRegister(cpu, index);

  WriteFloatRegister(cpu, index, st0, true);
  WriteFloatRegister(cpu, index, stn, true);
}
} // namespace CPU_X86