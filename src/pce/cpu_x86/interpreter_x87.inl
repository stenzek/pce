#include "YBaseLib/Endian.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/String.h"
#include "pce/bus.h"
#include "pce/cpu_x86/interpreter.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"

#ifdef Y_COMPILER_MSVC
#include <intrin.h>

// The if constexpr warning is noise, the compiler eliminates the other branches anyway.
#pragma warning(push)
#pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#endif

namespace CPU_X86 {
void Interpreter::StartX87Instruction(CPU* cpu)
{
  if ((cpu->m_registers.CR0 & CR0Bit_EM) || (cpu->m_registers.CR0 & (CR0Bit_MP | CR0Bit_TS)) == (CR0Bit_MP | CR0Bit_TS))
  {
    cpu->RaiseException(Interrupt_CoprocessorNotAvailable);
    return;
  }

  // If no FPU
  // cpu->AbortCurrentInstruction();
}

template<OperandSize size, OperandMode mode, uint32 constant>
floatx80 Interpreter::ReadFloatOperand(CPU* cpu, float_status_t& fs)
{
  if constexpr (mode == OperandMode_FPRegister)
  {
    // These are all 80-bit already.
    static_assert(mode != OperandMode_FPRegister || size == OperandSize_80, "operand sizes are 80-bit FP");
    CheckFloatStackUnderflow(cpu, constant);
    return ReadFloatRegister(cpu, constant);
  }
  else
  {
    // Only memory-based addressing is valid.
    static_assert(mode == OperandMode_FPRegister || mode == OperandMode_Memory || mode == OperandMode_ModRM_RM,
                  "only can use memory-based addressing");
    static_assert(size == OperandSize_32 || size == OperandSize_64 || size == OperandSize_80,
                  "size is 32, 64 or 80-bit");
    if constexpr (size == OperandSize_32)
    {
      // Convert single precision -> extended precision.
      uint32 dword_val = cpu->ReadMemoryDWord(cpu->idata.segment, cpu->m_effective_address);
      return float32_to_floatx80(dword_val, fs);
    }
    else if constexpr (size == OperandSize_64)
    {
      // Convert double precision -> extended precision
      uint32 dword_val_low = cpu->ReadMemoryDWord(cpu->idata.segment, cpu->m_effective_address);
      uint32 dword_val_high =
        cpu->ReadMemoryDWord(cpu->idata.segment, (cpu->m_effective_address + 4) & cpu->idata.GetAddressMask());
      float64 qword_val = (ZeroExtend64(dword_val_high) << 32) | ZeroExtend64(dword_val_low);
      return float64_to_floatx80(qword_val, fs);
    }
    else if constexpr (size == OperandSize_80)
    {
      // Load 80-bit extended precision. Does not check for SNaNs.
      uint32 tword_val_low = cpu->ReadMemoryDWord(cpu->idata.segment, cpu->m_effective_address);
      uint32 tword_val_middle =
        cpu->ReadMemoryDWord(cpu->idata.segment, (cpu->m_effective_address + 4) & cpu->idata.GetAddressMask());
      uint16 tword_val_high =
        cpu->ReadMemoryWord(cpu->idata.segment, (cpu->m_effective_address + 8) & cpu->idata.GetAddressMask());
      floatx80 ret;
      ret.fraction = (ZeroExtend64(tword_val_middle) << 32) | ZeroExtend64(tword_val_low);
      ret.exp = tword_val_high;
      return ret;
    }
  }
}

template<OperandSize size, OperandMode mode, uint32 constant>
floatx80 Interpreter::ReadIntegerOperandAsFloat(CPU* cpu, float_status_t& fs)
{
  // Only memory-based addressing is valid.
  static_assert(mode == OperandMode_Memory || mode == OperandMode_ModRM_RM, "only can use memory-based addressing");
  static_assert(size == OperandSize_16 || size == OperandSize_32 || size == OperandSize_64, "size is 16, 32, 64-bit");
  if constexpr (size == OperandSize_16)
  {
    int64 int_value = SignExtend64(cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_effective_address));
    return int64_to_floatx80(int_value);
  }
  else if constexpr (size == OperandSize_32)
  {
    int64 int_value = SignExtend64(cpu->ReadMemoryDWord(cpu->idata.segment, cpu->m_effective_address));
    return int64_to_floatx80(int_value);
  }
  else if constexpr (size == OperandSize_64)
  {
    uint32 low = cpu->ReadMemoryDWord(cpu->idata.segment, cpu->m_effective_address);
    uint32 high =
      cpu->ReadMemoryDWord(cpu->idata.segment, (cpu->m_effective_address + 4) & cpu->idata.GetAddressMask());
    int64 int_value = int_value = int64((ZeroExtend64(high) << 32) | ZeroExtend64(low));
    return int64_to_floatx80(int_value);
  }
}

template<OperandSize size, OperandMode mode, uint32 constant>
void Interpreter::WriteFloatOperand(CPU* cpu, float_status_t& fs, const floatx80& value)
{
  if constexpr (mode == OperandMode_FPRegister)
  {
    // These are all 80-bit already.
    static_assert(mode != OperandMode_FPRegister || size == OperandSize_80, "operand sizes are 80-bit FP");
    WriteFloatRegister(cpu, constant, value, true);
  }
  else
  {
    // Only memory-based addressing is valid.
    static_assert(mode == OperandMode_FPRegister || mode == OperandMode_Memory || mode == OperandMode_ModRM_RM,
                  "only can use memory-based addressing");
    static_assert(size == OperandSize_32 || size == OperandSize_64 || size == OperandSize_80,
                  "size is 32, 64 or 80-bit");
    if constexpr (size == OperandSize_32)
    {
      // Convert extended precision -> single precision.
      uint32 dword_val = floatx80_to_float32(value, fs);
      cpu->WriteMemoryDWord(cpu->idata.segment, cpu->m_effective_address, dword_val);
    }
    if constexpr (size == OperandSize_64)
    {
      // Convert extended precision -> double precision
      uint64 qword_val = floatx80_to_float64(value, fs);
      cpu->WriteMemoryDWord(cpu->idata.segment, cpu->m_effective_address, Truncate32(qword_val));
      cpu->WriteMemoryDWord(cpu->idata.segment, (cpu->m_effective_address + 4) & cpu->idata.GetAddressMask(),
                            Truncate32(qword_val >> 32));
    }
    if constexpr (size == OperandSize_80)
    {
      // Store 80-bit extended precision.
      cpu->WriteMemoryDWord(cpu->idata.segment, cpu->m_effective_address, Truncate32(value.fraction));
      cpu->WriteMemoryDWord(cpu->idata.segment, (cpu->m_effective_address + 4) & cpu->idata.GetAddressMask(),
                            Truncate32(value.fraction >> 32));
      cpu->WriteMemoryWord(cpu->idata.segment, (cpu->m_effective_address + 8) & cpu->idata.GetAddressMask(), value.exp);
    }
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

void Interpreter::WriteFloatRegister(CPU* cpu, uint8 relative_index, const floatx80& value, bool update_tag /*= true*/)
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
    uint8 bits = floatx80_is_unsupported(fx80) ? 1 : 0;
    if (floatx80_is_nan(fx80) || floatx80_is_infinity(fx80) || floatx80_is_zero_or_denormal(fx80))
      bits |= 2;
    cpu->m_fpu_registers.TW.Set(index, bits);
  }
}

float_status_t Interpreter::GetFloatStatus(CPU* cpu)
{
  static const int precision_lut[] = {32, 64, 80, 80};

  float_status_t ret;
  ret.float_rounding_precision = precision_lut[cpu->m_fpu_registers.CW.PC];
  ret.float_rounding_mode = cpu->m_fpu_registers.CW.RC.GetValue();
  ret.float_exception_flags = 0;
  ret.float_exception_masks = Truncate8(cpu->m_fpu_registers.CW.bits) & 63;
  ret.float_suppress_exception = 0;
  ret.float_nan_handling_mode = float_larger_significand_nan;
  ret.flush_underflow_to_zero = 0;
  ret.denormals_are_zeros = 0;
  return ret;
}

void Interpreter::RaiseFloatExceptions(CPU* cpu, const float_status_t& fs)
{
  if (fs.float_exception_flags == 0)
    return;

  bool abort = false;

  if (fs.float_exception_flags & float_flag_invalid)
  {
    cpu->m_fpu_registers.SW.I = true;
    abort |= !cpu->m_fpu_registers.CW.IM;
  }

  if (fs.float_exception_flags & float_flag_denormal)
  {
    cpu->m_fpu_registers.SW.D = true;
    abort |= !cpu->m_fpu_registers.CW.DM;
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

  if (abort)
  {
    cpu->m_fpu_exception = true;
    cpu->AbortCurrentInstruction();
  }
}

void Interpreter::UpdateC1Status(CPU* cpu, const float_status_t& fs)
{
  // C1 <- 1 if rounded up, otherwise 0
  cpu->m_fpu_registers.SW.C1 = 0;
}

void Interpreter::Execute_Operation_FNINIT(CPU* cpu)
{
  StartX87Instruction(cpu);

  cpu->m_fpu_registers.CW.bits = 0x037F;
  cpu->m_fpu_registers.SW.bits = 0;
  cpu->m_fpu_registers.TW.bits = 0xFFFF;
  // cpu->m_fpu_registers.DP

  // TRACE_EXECUTION = true;
}

void Interpreter::Execute_Operation_FNCLEX(CPU* cpu)
{
  StartX87Instruction(cpu);

  // FPUStatusWord[0:7] <- 0;
  // FPUStatusWord[15] <- 0;
  cpu->m_fpu_registers.SW.bits &= ~uint16((1 << 15) | 0b11111111);
}

void Interpreter::Execute_Operation_FNOP(CPU* cpu)
{
  StartX87Instruction(cpu);
}

void Interpreter::Execute_Operation_FNENI(CPU* cpu)
{
  // No-op on 80287+.
  StartX87Instruction(cpu);
}

void Interpreter::Execute_Operation_FNDISI(CPU* cpu)
{
  // No-op on 80287+.
  StartX87Instruction(cpu);
}

void Interpreter::Execute_Operation_FSETPM(CPU* cpu)
{
  // No-op on 80387+.
  StartX87Instruction(cpu);
}

void Interpreter::Execute_Operation_FINCSTP(CPU* cpu)
{
  StartX87Instruction(cpu);
  cpu->m_fpu_registers.SW.TOP = (cpu->m_fpu_registers.SW.TOP + 1) & 7;
  cpu->m_fpu_registers.SW.C1 = 0;
}

void Interpreter::Execute_Operation_FDECSTP(CPU* cpu)
{
  StartX87Instruction(cpu);
  cpu->m_fpu_registers.SW.TOP = (cpu->m_fpu_registers.SW.TOP - 1) & 7;
  cpu->m_fpu_registers.SW.C1 = 0;
}

void Interpreter::Execute_Operation_FABS(CPU* cpu)
{
  StartX87Instruction(cpu);

  CheckFloatStackUnderflow(cpu, 0);

  floatx80 val = ReadFloatRegister(cpu, 0);
  val = floatx80_abs(val);
  WriteFloatRegister(cpu, 0, val, true);

  cpu->m_fpu_registers.SW.C1 = 0;
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FADD(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 rhs = ReadFloatOperand<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_add(lhs, rhs, fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FADDP(CPU* cpu)
{
  Execute_Operation_FADD<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
  PopFloatStack(cpu);
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void Interpreter::Execute_Operation_FBLD(CPU* cpu)
{
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  uint32 p1 = cpu->ReadMemoryDWord(cpu->idata.segment, cpu->m_effective_address);
  uint32 p2 = cpu->ReadMemoryDWord(cpu->idata.segment, (cpu->m_effective_address + 4) & cpu->idata.GetAddressMask());
  uint16 p3 = cpu->ReadMemoryWord(cpu->idata.segment, (cpu->m_effective_address + 8) & cpu->idata.GetAddressMask());

  // based on bochs fpu_load_store.cc
  uint16 hi2 = p3;
  uint64 lo8 = (ZeroExtend64(p2) << 32) | ZeroExtend64(p1);

  cpu->m_fpu_registers.SW.C1 = 0;
  CheckFloatStackOverflow(cpu);

  int64 scale = 1;
  int64 val64 = 0;
  for (uint32 n = 0; n < 16; n++)
  {
    val64 += (lo8 & 0x0f) * scale;
    lo8 >>= 4;
    scale *= 10;
  }

  val64 += (hi2 & 0x0f) * scale;
  val64 += ((hi2 >> 4) & 0x0f) * scale * 10;

  floatx80 result = int64_to_floatx80(val64);
  if (hi2 & 0x8000)
    result = floatx80_chs(result);

  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0, result);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FBSTP(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  StartX87Instruction(cpu);

  // The store can throw an exception, so we want to preserve the old FPU flags.
  uint16 SW = cpu->m_fpu_registers.SW.bits;

  cpu->m_fpu_registers.SW.C1 = 0;
  CheckFloatStackUnderflow(cpu, 0);

  // TODO: this shouldn't abort if underflow exceptions are masked
  uint16 save_reg_hi = 0xFFFF;
  uint64 save_reg_lo = UINT64_C(0xC000000000000000);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 reg = ReadFloatRegister(cpu, 0);

  int64 save_val = floatx80_to_int64(reg, fs);
  int sign = (reg.exp & 0x8000) != 0;
  if (sign)
    save_val = -save_val;

  if (save_val > INT64_C(999999999999999999))
    fs.float_exception_flags |= float_flag_invalid;

  RaiseFloatExceptions(cpu, fs);

  if ((fs.float_exception_flags & float_flag_invalid) == 0)
  {
    save_reg_hi = (sign) ? 0x8000 : 0;
    save_reg_lo = 0;

    for (uint32 i = 0; i < 16; i++)
    {
      save_reg_lo += uint64(save_val % 10) << (4 * i);
      save_val /= 10;
    }

    save_reg_hi += uint16(save_val % 10);
    save_val /= 10;
    save_reg_hi += uint16(save_val % 10) << 4;
  }

  uint16 new_SW = cpu->m_fpu_registers.SW.bits;
  cpu->m_fpu_registers.SW.bits = SW;

  cpu->WriteMemoryDWord(cpu->idata.segment, cpu->m_effective_address, Truncate32(save_reg_lo));
  cpu->WriteMemoryDWord(cpu->idata.segment, (cpu->m_effective_address + 4) & cpu->idata.GetAddressMask(),
                        Truncate32(save_reg_lo >> 32));
  cpu->WriteMemoryWord(cpu->idata.segment, (cpu->m_effective_address + 8) & cpu->idata.GetAddressMask(), save_reg_hi);

  cpu->m_fpu_registers.SW.bits = new_SW;
  PopFloatStack(cpu);
}

void Interpreter::Execute_Operation_FCHS(CPU* cpu)
{
  StartX87Instruction(cpu);

  CheckFloatStackUnderflow(cpu, 0);

  floatx80 val = ReadFloatRegister(cpu, 0);
  val = floatx80_chs(val);
  WriteFloatRegister(cpu, 0, val, true);

  cpu->m_fpu_registers.SW.C1 = 0;
}

void Interpreter::SetStatusWordFromCompare(CPU* cpu, const float_status_t& fs, int res)
{
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
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant, bool quiet>
void Interpreter::Execute_Operation_FCOM(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 rhs = ReadFloatOperand<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  int res;
  if constexpr (quiet)
    res = floatx80_compare_quiet(lhs, rhs, fs);
  else
    res = floatx80_compare(lhs, rhs, fs);

  SetStatusWordFromCompare(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant, bool quiet>
void Interpreter::Execute_Operation_FCOMP(CPU* cpu)
{
  Execute_Operation_FCOM<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant, quiet>(cpu);
  PopFloatStack(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant, bool quiet>
void Interpreter::Execute_Operation_FCOMPP(CPU* cpu)
{
  Execute_Operation_FCOM<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant, quiet>(cpu);
  PopFloatStack(cpu);
  PopFloatStack(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FUCOM(CPU* cpu)
{
  Execute_Operation_FCOM<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant, true>(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FUCOMP(CPU* cpu)
{
  Execute_Operation_FCOMP<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant, true>(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FUCOMPP(CPU* cpu)
{
  Execute_Operation_FCOMPP<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant, true>(cpu);
}

void Interpreter::Execute_Operation_FTST(CPU* cpu)
{
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 val = ReadFloatRegister(cpu, 0);

  int res = floatx80_compare(val, Const_Z, fs);
  SetStatusWordFromCompare(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FDIV(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 dividend = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 divisor = ReadFloatOperand<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_div(dividend, divisor, fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FDIVP(CPU* cpu)
{
  Execute_Operation_FDIV<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
  PopFloatStack(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FDIVR(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 divisor = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 dividend = ReadFloatOperand<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_div(dividend, divisor, fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FDIVRP(CPU* cpu)
{
  Execute_Operation_FDIVR<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
  PopFloatStack(cpu);
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant>
void Interpreter::Execute_Operation_FFREE(CPU* cpu)
{
  static_assert(val_mode == OperandMode_FPRegister && val_constant < countof(cpu->m_fpu_registers.ST),
                "mode is FP register");
  StartX87Instruction(cpu);
  cpu->m_fpu_registers.TW.SetEmpty(val_constant);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FIADD(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 rhs = ReadIntegerOperandAsFloat<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_add(lhs, rhs, fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FIMUL(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 rhs = ReadIntegerOperandAsFloat<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_mul(lhs, rhs, fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FIDIV(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 dividend = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 divisor = ReadIntegerOperandAsFloat<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_div(dividend, divisor, fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FIDIVR(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 divisor = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 dividend = ReadIntegerOperandAsFloat<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_div(dividend, divisor, fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant, bool quiet>
void Interpreter::Execute_Operation_FICOM(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 rhs = int32_to_floatx80(int32(ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu)));
  RaiseFloatExceptions(cpu, fs);

  int res;
  if constexpr (quiet)
    res = floatx80_compare_quiet(lhs, rhs, fs);
  else
    res = floatx80_compare(lhs, rhs, fs);

  SetStatusWordFromCompare(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FICOMP(CPU* cpu)
{
  Execute_Operation_FICOM<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
  PopFloatStack(cpu);
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void Interpreter::Execute_Operation_FILD(CPU* cpu)
{
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 value = ReadIntegerOperandAsFloat<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0, value, true);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FIST(CPU* cpu)
{
  static_assert(dst_size == OperandSize_16 || dst_size == OperandSize_32 || dst_size == OperandSize_64,
                "dst_size is 16/32/64 bits");
  CalculateEffectiveAddress<dst_mode>(cpu);
  StartX87Instruction(cpu);

  CheckFloatStackUnderflow(cpu, 0);
  floatx80 value = ReadFloatRegister(cpu, 0);

  float_status_t fs = GetFloatStatus(cpu);
  if constexpr (dst_size == OperandSize_16)
  {
    int32 int_value = floatx80_to_int32(value, fs);
    if (int_value < -32768 || int_value > 32767)
    {
      fs.float_exception_flags = float_flag_invalid;
      int_value = 0x8000;
    }
    RaiseFloatExceptions(cpu, fs);
    WriteWordOperand<dst_mode, dst_constant>(cpu, uint16(int16(int_value)));
  }
  else if constexpr (dst_size == OperandSize_32)
  {
    int32 int_value = floatx80_to_int32(value, fs);
    RaiseFloatExceptions(cpu, fs);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, uint32(int_value));
  }
  else if constexpr (dst_size == OperandSize_64)
  {
    int64 int_value = floatx80_to_int64(value, fs);
    RaiseFloatExceptions(cpu, fs);

    static_assert(dst_size != OperandSize_64 || (dst_mode == OperandMode_Memory || dst_mode == OperandMode_ModRM_RM),
                  "operand is correct type");
    Assert(!cpu->idata.modrm_rm_register);
    cpu->WriteMemoryDWord(cpu->idata.segment, cpu->m_effective_address, Truncate32(uint64(int_value)));
    cpu->WriteMemoryDWord(cpu->idata.segment, cpu->m_effective_address + 4, Truncate32(uint64(int_value) >> 32));
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FISTP(CPU* cpu)
{
  Execute_Operation_FIST<dst_size, dst_mode, dst_constant>(cpu);
  PopFloatStack(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FISUB(CPU* cpu)
{
  Panic("Not Implemented");
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FISUBR(CPU* cpu)
{
  Panic("Not Implemented");
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void Interpreter::Execute_Operation_FLD(CPU* cpu)
{
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);
  CheckFloatStackOverflow(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 value = ReadFloatOperand<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0, value, true);
}

void Interpreter::Execute_Operation_FLD1(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackOverflow(cpu);
  PushFloatStack(cpu);

  WriteFloatRegister(cpu, 0, Const_1, true);
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void Interpreter::Execute_Operation_FLDCW(CPU* cpu)
{
  static_assert(src_size == OperandSize_16, "src size is 16-bits");
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  uint16 cw = ReadWordOperand<src_mode, src_constant>(cpu);
  cpu->m_fpu_registers.CW.bits = cw;
}

void Interpreter::Execute_Operation_FLDL2E(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackOverflow(cpu);
  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0,
                     floatx80_round_const(Const_L2E, (cpu->m_fpu_registers.CW.RC & FPURoundingControl_Down) ? -1 : 0));
}

void Interpreter::Execute_Operation_FLDL2T(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackOverflow(cpu);
  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0,
                     floatx80_round_const(Const_L2T, (cpu->m_fpu_registers.CW.RC == FPURoundingControl_Up) ? 1 : 0));
}

void Interpreter::Execute_Operation_FLDLG2(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackOverflow(cpu);
  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0,
                     floatx80_round_const(Const_LG2, (cpu->m_fpu_registers.CW.RC & FPURoundingControl_Down) ? -1 : 0));
}

void Interpreter::Execute_Operation_FLDLN2(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackOverflow(cpu);
  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0,
                     floatx80_round_const(Const_LN2, (cpu->m_fpu_registers.CW.RC & FPURoundingControl_Down) ? -1 : 0));
}

void Interpreter::Execute_Operation_FLDPI(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackOverflow(cpu);
  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0,
                     floatx80_round_const(Const_PI, (cpu->m_fpu_registers.CW.RC & FPURoundingControl_Down) ? -1 : 0));
}

void Interpreter::Execute_Operation_FLDZ(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackOverflow(cpu);
  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0, Const_Z);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FMUL(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 rhs = ReadFloatOperand<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_mul(lhs, rhs, fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FMULP(CPU* cpu)
{
  Execute_Operation_FMUL<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
  PopFloatStack(cpu);
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void Interpreter::Execute_Operation_FRSTOR(CPU* cpu)
{
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  cpu->LoadFPUState(cpu->idata.segment, cpu->m_effective_address, cpu->idata.GetAddressMask(), cpu->idata.Is32Bit(),
                    true);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FNSAVE(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  StartX87Instruction(cpu);

  cpu->StoreFPUState(cpu->idata.segment, cpu->m_effective_address, cpu->idata.GetAddressMask(), cpu->idata.Is32Bit(),
                     true);

  // Reset state
  cpu->m_fpu_registers.CW.bits = 0x037F;
  cpu->m_fpu_registers.SW.bits = 0;
  cpu->m_fpu_registers.TW.bits = 0xFFFF;
  cpu->m_fpu_exception = false;
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void Interpreter::Execute_Operation_FLDENV(CPU* cpu)
{
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  cpu->LoadFPUState(cpu->idata.segment, cpu->m_effective_address, cpu->idata.GetAddressMask(), cpu->idata.Is32Bit(),
                    false);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FNSTENV(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  StartX87Instruction(cpu);

  cpu->StoreFPUState(cpu->idata.segment, cpu->m_effective_address, cpu->idata.GetAddressMask(), cpu->idata.Is32Bit(),
                     false);

  // Mask all exceptions.
  cpu->m_fpu_registers.CW.IM = false;
  cpu->m_fpu_registers.CW.DM = false;
  cpu->m_fpu_registers.CW.ZM = false;
  cpu->m_fpu_registers.CW.OM = false;
  cpu->m_fpu_registers.CW.UM = false;
  cpu->m_fpu_registers.CW.PM = false;
  cpu->m_fpu_registers.CW.IEM = false;
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FNSTCW(CPU* cpu)
{
  static_assert(dst_size == OperandSize_16, "dst_size is 16 bits");
  CalculateEffectiveAddress<dst_mode>(cpu);
  StartX87Instruction(cpu);

  WriteWordOperand<dst_mode, dst_constant>(cpu, cpu->m_fpu_registers.CW.bits);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FNSTSW(CPU* cpu)
{
  static_assert(dst_size == OperandSize_16, "dst_size is 16 bits");
  CalculateEffectiveAddress<dst_mode>(cpu);
  StartX87Instruction(cpu);

  WriteWordOperand<dst_mode, dst_constant>(cpu, cpu->m_fpu_registers.SW.bits);
}

template<bool ieee754>
void Interpreter::Execute_Operation_FPREM(CPU* cpu)
{
  StartX87Instruction(cpu);

  CheckFloatStackUnderflow(cpu, 0);
  CheckFloatStackUnderflow(cpu, 1);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 dividend = ReadFloatRegister(cpu, 0);
  floatx80 divisor = ReadFloatRegister(cpu, 1);

  floatx80 res;
  uint64 q;
  int flags = floatx80_remainder(dividend, divisor, res, q, fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatRegister(cpu, 0, res);
  if (flags == 0)
  {
    cpu->m_fpu_registers.SW.C2 = false;
    cpu->m_fpu_registers.SW.C1 = uint8(q >> 63);
    cpu->m_fpu_registers.SW.C3 = uint8(q >> 62) & uint8(1);
    cpu->m_fpu_registers.SW.C0 = uint8(q >> 61) & uint8(1);
  }
  else if (flags > 0)
  {
    cpu->m_fpu_registers.SW.C2 = true;
  }
}

void Interpreter::Execute_Operation_FPREM1(CPU* cpu)
{
  Execute_Operation_FPREM<true>(cpu);
}

void Interpreter::Execute_Operation_FPTAN(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackUnderflow(cpu, 0);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 val = ReadFloatRegister(cpu, 0);
  if (ftan(val, fs) != 0)
  {
    cpu->m_fpu_registers.SW.C2 = 1;
    return;
  }

  RaiseFloatExceptions(cpu, fs);
  CheckFloatStackOverflow(cpu);
  WriteFloatRegister(cpu, 0, val);
  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0, floatx80_is_nan(val) ? val : Const_1);
}

void Interpreter::Execute_Operation_FRNDINT(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackUnderflow(cpu, 0);

  // ST(0) <- RoundToIntegralValue(ST(0))
  float_status_t fs = GetFloatStatus(cpu);
  floatx80 val = ReadFloatRegister(cpu, 0);
  floatx80 res = floatx80_round_to_int(val, fs);
  RaiseFloatExceptions(cpu, fs);
  UpdateC1Status(cpu, fs);
  WriteFloatRegister(cpu, 0, res);
}

void Interpreter::Execute_Operation_FSCALE(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackUnderflow(cpu, 0);
  CheckFloatStackUnderflow(cpu, 1);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 st0 = ReadFloatRegister(cpu, 0);
  floatx80 st1 = ReadFloatRegister(cpu, 1);
  floatx80 res = floatx80_scale(st0, st1, fs);
  RaiseFloatExceptions(cpu, fs);
  UpdateC1Status(cpu, fs);
  WriteFloatRegister(cpu, 0, res);
}

void Interpreter::Execute_Operation_FSQRT(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackUnderflow(cpu, 0);

  // ST(0) <- SquareRoot(ST(0))
  float_status_t fs = GetFloatStatus(cpu);
  floatx80 val = ReadFloatRegister(cpu, 0);
  floatx80 res = floatx80_sqrt(val, fs);
  RaiseFloatExceptions(cpu, fs);
  UpdateC1Status(cpu, fs);
  WriteFloatRegister(cpu, 0, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FST(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  StartX87Instruction(cpu);

  CheckFloatStackUnderflow(cpu, 0);
  floatx80 value = ReadFloatRegister(cpu, 0);

  float_status_t fs = GetFloatStatus(cpu);
  WriteFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs, value);
  RaiseFloatExceptions(cpu, fs);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FSTCW(CPU* cpu)
{
  static_assert(dst_size == OperandSize_16, "dst is 16-bits");
  CalculateEffectiveAddress<dst_mode>(cpu);
  StartX87Instruction(cpu);

  WriteWordOperand<dst_mode, dst_constant>(cpu, cpu->m_fpu_registers.CW);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FSTENV(CPU* cpu)
{
  Panic("Not Implemented");
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FSTP(CPU* cpu)
{
  Execute_Operation_FST<dst_size, dst_mode, dst_constant>(cpu);
  PopFloatStack(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void Interpreter::Execute_Operation_FSTSW(CPU* cpu)
{
  static_assert(dst_size == OperandSize_16, "dst is 16-bits");
  CalculateEffectiveAddress<dst_mode>(cpu);
  StartX87Instruction(cpu);

  WriteWordOperand<dst_mode, dst_constant>(cpu, cpu->m_fpu_registers.SW);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FSUB(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 lhs = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 rhs = ReadFloatOperand<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_sub(lhs, rhs, fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FSUBP(CPU* cpu)
{
  Execute_Operation_FSUB<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
  PopFloatStack(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FSUBR(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  StartX87Instruction(cpu);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 rhs = ReadFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs);
  floatx80 lhs = ReadFloatOperand<src_size, src_mode, src_constant>(cpu, fs);
  RaiseFloatExceptions(cpu, fs);

  floatx80 res = floatx80_sub(lhs, rhs, fs);
  RaiseFloatExceptions(cpu, fs);

  WriteFloatOperand<dst_size, dst_mode, dst_constant>(cpu, fs, res);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void Interpreter::Execute_Operation_FSUBRP(CPU* cpu)
{
  Execute_Operation_FSUBR<dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
  PopFloatStack(cpu);
}

void Interpreter::Execute_Operation_FXAM(CPU* cpu)
{
  StartX87Instruction(cpu);

  // Empty values are accepted here.
  floatx80 val = ReadFloatRegister(cpu, 0);

  uint8 C0, C1, C2, C3;

  // C1 <- SignBit(ST(0))
  C1 = uint8(val.exp >> 15);

  // GetClass(ST(0))
  if (cpu->m_fpu_registers.TW.IsEmpty(cpu->m_fpu_registers.SW.TOP))
  {
    // Empty
    C3 = 1;
    C2 = 0;
    C0 = 1;
  }
  else if (floatx80_is_unsupported(val))
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

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant>
void Interpreter::Execute_Operation_FXCH(CPU* cpu)
{
  static_assert(val_mode == OperandMode_FPRegister && val_constant < countof(cpu->m_fpu_registers.ST),
                "val_mode is FP register");
  StartX87Instruction(cpu);

  uint8 index = val_constant;
  CheckFloatStackUnderflow(cpu, 0);
  CheckFloatStackUnderflow(cpu, index);

  floatx80 st0 = ReadFloatRegister(cpu, 0);
  floatx80 stn = ReadFloatRegister(cpu, index);

  WriteFloatRegister(cpu, index, st0, true);
  WriteFloatRegister(cpu, 0, stn, true);
}

void Interpreter::Execute_Operation_FXTRACT(CPU* cpu)
{
  Panic("Not Implemented");
}

void Interpreter::Execute_Operation_FPATAN(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackUnderflow(cpu, 0);
  CheckFloatStackUnderflow(cpu, 1);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 st0 = ReadFloatRegister(cpu, 0);
  floatx80 st1 = ReadFloatRegister(cpu, 1);
  floatx80 res = fpatan(st0, st1, fs);
  RaiseFloatExceptions(cpu, fs);
  UpdateC1Status(cpu, fs);
  WriteFloatRegister(cpu, 1, res);
  PopFloatStack(cpu);
}

void Interpreter::Execute_Operation_F2XM1(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackUnderflow(cpu, 0);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 val = ReadFloatRegister(cpu, 0);
  floatx80 res = f2xm1(val, fs);
  RaiseFloatExceptions(cpu, fs);
  UpdateC1Status(cpu, fs);
  WriteFloatRegister(cpu, 0, res);
}

void Interpreter::Execute_Operation_FYL2X(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackUnderflow(cpu, 0);
  CheckFloatStackUnderflow(cpu, 1);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 st0 = ReadFloatRegister(cpu, 0);
  floatx80 st1 = ReadFloatRegister(cpu, 1);
  floatx80 res = fyl2x(st0, st1, fs);
  RaiseFloatExceptions(cpu, fs);
  UpdateC1Status(cpu, fs);
  WriteFloatRegister(cpu, 1, res);
  PopFloatStack(cpu);
}

void Interpreter::Execute_Operation_FYL2XP1(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackUnderflow(cpu, 0);
  CheckFloatStackUnderflow(cpu, 1);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 st0 = ReadFloatRegister(cpu, 0);
  floatx80 st1 = ReadFloatRegister(cpu, 1);
  floatx80 res = fyl2xp1(st0, st1, fs);
  RaiseFloatExceptions(cpu, fs);
  UpdateC1Status(cpu, fs);
  WriteFloatRegister(cpu, 1, res);
}

void Interpreter::Execute_Operation_FCOS(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackUnderflow(cpu, 0);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 val = ReadFloatRegister(cpu, 0);
  if (fcos(val, fs) != 0)
  {
    cpu->m_fpu_registers.SW.C2 = 1;
    return;
  }

  RaiseFloatExceptions(cpu, fs);
  UpdateC1Status(cpu, fs);
  WriteFloatRegister(cpu, 0, val);
}

void Interpreter::Execute_Operation_FSIN(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackUnderflow(cpu, 0);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 val = ReadFloatRegister(cpu, 0);
  if (fsin(val, fs) != 0)
  {
    cpu->m_fpu_registers.SW.C2 = 1;
    return;
  }

  RaiseFloatExceptions(cpu, fs);
  UpdateC1Status(cpu, fs);
  WriteFloatRegister(cpu, 0, val);
}

void Interpreter::Execute_Operation_FSINCOS(CPU* cpu)
{
  StartX87Instruction(cpu);
  CheckFloatStackUnderflow(cpu, 0);

  float_status_t fs = GetFloatStatus(cpu);
  floatx80 val = ReadFloatRegister(cpu, 0);
  floatx80 sin_val, cos_val;
  if (fsincos(val, &sin_val, &cos_val, fs) != 0)
  {
    cpu->m_fpu_registers.SW.C2 = 1;
    return;
  }

  RaiseFloatExceptions(cpu, fs);
  CheckFloatStackOverflow(cpu);
  UpdateC1Status(cpu, fs);
  WriteFloatRegister(cpu, 0, sin_val);
  PushFloatStack(cpu);
  WriteFloatRegister(cpu, 0, cos_val);
}
} // namespace CPU_X86

#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif
