#include "interpreter.h"
#include "recompiler_code_generator.h"
#include "recompiler_thunks.h"

namespace CPU_X86::Recompiler {

#if defined(ABI_WIN64)
constexpr HostReg RCPUPTR = Xbyak::Operand::RBP;
constexpr HostReg RRETURN = Xbyak::Operand::RAX;
constexpr HostReg RARG1 = Xbyak::Operand::RCX;
constexpr HostReg RARG2 = Xbyak::Operand::RDX;
constexpr HostReg RARG3 = Xbyak::Operand::R8;
constexpr HostReg RARG4 = Xbyak::Operand::R9;
constexpr u32 FUNCTION_CALL_SHADOW_SPACE = 32;
constexpr u64 FUNCTION_CALL_STACK_ALIGNMENT = 16;
#elif defined(ABI_SYSV)
constexpr HostReg RCPUPTR = Xbyak::Operand::RBP;
constexpr HostReg RRETURN = Xbyak::Operand::RAX;
constexpr HostReg RARG1 = Xbyak::Operand::RDI;
constexpr HostReg RARG2 = Xbyak::Operand::RSI;
constexpr HostReg RARG3 = Xbyak::Operand::RDX;
constexpr HostReg RARG4 = Xbyak::Operand::RCX;
constexpr u32 FUNCTION_CALL_SHADOW_SPACE = 0;
constexpr u64 FUNCTION_CALL_STACK_ALIGNMENT = 16;
#endif

static const Xbyak::Reg8 GetHostReg8(HostReg reg)
{
  return Xbyak::Reg8(reg, reg >= Xbyak::Operand::SPL);
}

static const Xbyak::Reg8 GetHostReg8(const Value& value)
{
  DebugAssert(value.size == OperandSize_8 && value.IsInHostRegister());
  return Xbyak::Reg8(value.host_reg, value.host_reg >= Xbyak::Operand::SPL);
}

static const Xbyak::Reg16 GetHostReg16(HostReg reg)
{
  return Xbyak::Reg16(reg);
}

static const Xbyak::Reg16 GetHostReg16(const Value& value)
{
  DebugAssert(value.size == OperandSize_16 && value.IsInHostRegister());
  return Xbyak::Reg16(value.host_reg);
}

static const Xbyak::Reg32 GetHostReg32(HostReg reg)
{
  return Xbyak::Reg32(reg);
}

static const Xbyak::Reg32 GetHostReg32(const Value& value)
{
  DebugAssert(value.size == OperandSize_32 && value.IsInHostRegister());
  return Xbyak::Reg32(value.host_reg);
}

static const Xbyak::Reg64 GetHostReg64(HostReg reg)
{
  return Xbyak::Reg64(reg);
}

static const Xbyak::Reg64 GetHostReg64(const Value& value)
{
  DebugAssert(value.size == OperandSize_64 && value.IsInHostRegister());
  return Xbyak::Reg64(value.host_reg);
}

static const Xbyak::Reg64 GetCPUPtrReg()
{
  return GetHostReg64(RCPUPTR);
}

const char* CodeGenerator::GetHostRegName(HostReg reg, OperandSize size /*= HostPointerSize*/)
{
  static constexpr std::array<const char*, HostReg_Count> reg8_names = {
    {"al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"}};
  static constexpr std::array<const char*, HostReg_Count> reg16_names = {
    {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"}};
  static constexpr std::array<const char*, HostReg_Count> reg32_names = {{"eax", "ecx", "edx", "ebx", "esp", "ebp",
                                                                          "esi", "edi", "r8d", "r9d", "r10d", "r11d",
                                                                          "r12d", "r13d", "r14d", "r15d"}};
  static constexpr std::array<const char*, HostReg_Count> reg64_names = {
    {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"}};
  if (reg >= static_cast<HostReg>(HostReg_Count))
    return "";

  switch (size)
  {
    case OperandSize_8:
      return reg8_names[reg];
    case OperandSize_16:
      return reg16_names[reg];
    case OperandSize_32:
      return reg32_names[reg];
    case OperandSize_64:
      return reg64_names[reg];
    default:
      return "";
  }
}

void CodeGenerator::InitHostRegs()
{
#if defined(ABI_WIN64)
  // TODO: function calls mess up the parameter registers if we use them.. fix it
  // allocate nonvolatile before volatile
  m_register_cache.SetHostRegAllocationOrder(
    {Xbyak::Operand::RBX, Xbyak::Operand::RBP, Xbyak::Operand::RDI, Xbyak::Operand::RSI, /*Xbyak::Operand::RSP, */
     Xbyak::Operand::R12, Xbyak::Operand::R13, Xbyak::Operand::R14, Xbyak::Operand::R15, /*Xbyak::Operand::RCX,
     Xbyak::Operand::RDX, Xbyak::Operand::R8, Xbyak::Operand::R9, */
     Xbyak::Operand::R10, Xbyak::Operand::R11,
     /*Xbyak::Operand::RAX*/});
  m_register_cache.SetCallerSavedHostRegs({Xbyak::Operand::RAX, Xbyak::Operand::RCX, Xbyak::Operand::RDX,
                                           Xbyak::Operand::R8, Xbyak::Operand::R9, Xbyak::Operand::R10,
                                           Xbyak::Operand::R11});
  m_register_cache.SetCalleeSavedHostRegs({Xbyak::Operand::RBX, Xbyak::Operand::RBP, Xbyak::Operand::RDI,
                                           Xbyak::Operand::RSI, Xbyak::Operand::RSP, Xbyak::Operand::R12,
                                           Xbyak::Operand::R13, Xbyak::Operand::R14, Xbyak::Operand::R15});
  m_register_cache.SetCPUPtrHostReg(RCPUPTR);
#elif defined(ABI_SYSV)
  m_register_cache.SetHostRegAllocationOrder(
    {Xbyak::Operand::RBX, /*Xbyak::Operand::RSP, */ Xbyak::Operand::RBP, Xbyak::Operand::R12, Xbyak::Operand::R13,
     Xbyak::Operand::R14, Xbyak::Operand::R15,
     /*Xbyak::Operand::RAX, */ /*Xbyak::Operand::RDI, */ /*Xbyak::Operand::RSI, */
     /*Xbyak::Operand::RDX, */ /*Xbyak::Operand::RCX, */ Xbyak::Operand::R8, Xbyak::Operand::R9, Xbyak::Operand::R10,
     Xbyak::Operand::R11});
  m_register_cache.SetCallerSavedHostRegs({Xbyak::Operand::RAX, Xbyak::Operand::RDI, Xbyak::Operand::RSI,
                                           Xbyak::Operand::RDX, Xbyak::Operand::RCX, Xbyak::Operand::R8,
                                           Xbyak::Operand::R9, Xbyak::Operand::R10, Xbyak::Operand::R11});
  m_register_cache.SetCalleeSavedHostRegs({Xbyak::Operand::RBX, Xbyak::Operand::RSP, Xbyak::Operand::RBP,
                                           Xbyak::Operand::R12, Xbyak::Operand::R13, Xbyak::Operand::R14,
                                           Xbyak::Operand::R15});
  m_register_cache.SetCPUPtrHostReg(RCPUPTR);
#endif
}

void CodeGenerator::EmitBeginBlock()
{
  // Store the CPU struct pointer.
  const bool cpu_reg_allocated = m_register_cache.AllocateHostReg(RCPUPTR);
  DebugAssert(cpu_reg_allocated);
  m_emit.mov(GetCPUPtrReg(), GetHostReg64(RARG1));

  // Copy {EIP,ESP} to m_current_{EIP,ESP}
  SyncCurrentEIP();
  SyncCurrentESP();
}

void CodeGenerator::EmitEndBlock()
{
  m_register_cache.FreeHostReg(RCPUPTR);
  m_register_cache.PopCalleeSavedRegisters();

  m_emit.ret();
}

void CodeGenerator::FinalizeBlock(BlockFunctionType* out_function_ptr, size_t* out_code_size)
{
  m_emit.ready();

  const size_t code_size = m_emit.getSize();
  *out_function_ptr = m_emit.getCode<BlockFunctionType>();
  *out_code_size = code_size;
  m_code_buffer->CommitCode(code_size);
  m_emit.reset();
}

void CodeGenerator::EmitSignExtend(HostReg to_reg, OperandSize to_size, HostReg from_reg, OperandSize from_size)
{
  switch (to_size)
  {
    case OperandSize_16:
    {
      switch (from_size)
      {
        case OperandSize_8:
          m_emit.movsx(GetHostReg16(to_reg), GetHostReg8(from_reg));
          return;
      }
    }
    break;

    case OperandSize_32:
    {
      switch (from_size)
      {
        case OperandSize_8:
          m_emit.movsx(GetHostReg32(to_reg), GetHostReg8(from_reg));
          return;
        case OperandSize_16:
          m_emit.movsx(GetHostReg32(to_reg), GetHostReg16(from_reg));
          return;
      }
    }
    break;
  }

  Panic("Unknown sign-extend combination");
}

void CodeGenerator::EmitZeroExtend(HostReg to_reg, OperandSize to_size, HostReg from_reg, OperandSize from_size)
{
  switch (to_size)
  {
    case OperandSize_16:
    {
      switch (from_size)
      {
        case OperandSize_8:
          m_emit.movzx(GetHostReg16(to_reg), GetHostReg8(from_reg));
          return;
      }
    }
    break;

    case OperandSize_32:
    {
      switch (from_size)
      {
        case OperandSize_8:
          m_emit.movzx(GetHostReg32(to_reg), GetHostReg8(from_reg));
          return;
        case OperandSize_16:
          m_emit.movzx(GetHostReg32(to_reg), GetHostReg16(from_reg));
          return;
      }
    }
    break;
  }

  Panic("Unknown sign-extend combination");
}

void CodeGenerator::EmitCopyValue(HostReg to_reg, const Value& value)
{
  // TODO: mov x, 0 -> xor x, x
  DebugAssert(value.IsConstant() || value.IsInHostRegister());

  switch (value.size)
  {
    case OperandSize_8:
    {
      if (value.HasConstantValue(0))
        m_emit.xor_(GetHostReg8(to_reg), GetHostReg8(to_reg));
      else if (value.IsConstant())
        m_emit.mov(GetHostReg8(to_reg), value.constant_value);
      else
        m_emit.mov(GetHostReg8(to_reg), GetHostReg8(value.host_reg));
    }
    break;

    case OperandSize_16:
    {
      if (value.HasConstantValue(0))
        m_emit.xor_(GetHostReg16(to_reg), GetHostReg16(to_reg));
      else if (value.IsConstant())
        m_emit.mov(GetHostReg16(to_reg), value.constant_value);
      else
        m_emit.mov(GetHostReg16(to_reg), GetHostReg16(value.host_reg));
    }
    break;

    case OperandSize_32:
    {
      if (value.HasConstantValue(0))
        m_emit.xor_(GetHostReg32(to_reg), GetHostReg32(to_reg));
      else if (value.IsConstant())
        m_emit.mov(GetHostReg32(to_reg), value.constant_value);
      else
        m_emit.mov(GetHostReg32(to_reg), GetHostReg32(value.host_reg));
    }
    break;

    case OperandSize_64:
    {
      if (value.HasConstantValue(0))
        m_emit.xor_(GetHostReg64(to_reg), GetHostReg64(to_reg));
      else if (value.IsConstant())
        m_emit.mov(GetHostReg64(to_reg), value.constant_value);
      else
        m_emit.mov(GetHostReg64(to_reg), GetHostReg64(value.host_reg));
    }
    break;
  }
}

void CodeGenerator::EmitAdd(HostReg to_reg, const Value& value)
{
  DebugAssert(value.IsConstant() || value.IsInHostRegister());

  switch (value.size)
  {
    case OperandSize_8:
    {
      if (value.IsConstant())
        m_emit.add(GetHostReg8(to_reg), SignExtend32(Truncate8(value.constant_value)));
      else
        m_emit.add(GetHostReg8(to_reg), GetHostReg8(value.host_reg));
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant())
        m_emit.add(GetHostReg16(to_reg), SignExtend32(Truncate16(value.constant_value)));
      else
        m_emit.add(GetHostReg16(to_reg), GetHostReg16(value.host_reg));
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant())
        m_emit.add(GetHostReg32(to_reg), Truncate32(value.constant_value));
      else
        m_emit.add(GetHostReg32(to_reg), GetHostReg32(value.host_reg));
    }
    break;

    case OperandSize_64:
    {
      if (value.IsConstant())
      {
        if (!Xbyak::inner::IsInInt32(value.constant_value))
        {
          Value temp = m_register_cache.AllocateScratch(OperandSize_64);
          m_emit.mov(GetHostReg64(temp.host_reg), value.constant_value);
          m_emit.add(GetHostReg64(to_reg), GetHostReg64(temp.host_reg));
        }
        else
        {
          m_emit.add(GetHostReg64(to_reg), Truncate32(value.constant_value));
        }
      }
      else
      {
        m_emit.add(GetHostReg64(to_reg), GetHostReg64(value.host_reg));
      }
    }
    break;
  }
}

void CodeGenerator::EmitSub(HostReg to_reg, const Value& value)
{
  DebugAssert(value.IsConstant() || value.IsInHostRegister());

  switch (value.size)
  {
    case OperandSize_8:
    {
      if (value.IsConstant())
        m_emit.sub(GetHostReg8(to_reg), SignExtend32(Truncate8(value.constant_value)));
      else
        m_emit.sub(GetHostReg8(to_reg), GetHostReg8(value.host_reg));
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant())
        m_emit.sub(GetHostReg16(to_reg), SignExtend32(Truncate16(value.constant_value)));
      else
        m_emit.sub(GetHostReg16(to_reg), GetHostReg16(value.host_reg));
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant())
        m_emit.sub(GetHostReg32(to_reg), Truncate32(value.constant_value));
      else
        m_emit.sub(GetHostReg32(to_reg), GetHostReg32(value.host_reg));
    }
    break;

    case OperandSize_64:
    {
      if (value.IsConstant())
      {
        if (!Xbyak::inner::IsInInt32(value.constant_value))
        {
          Value temp = m_register_cache.AllocateScratch(OperandSize_64);
          m_emit.mov(GetHostReg64(temp.host_reg), value.constant_value);
          m_emit.sub(GetHostReg64(to_reg), GetHostReg64(temp.host_reg));
        }
        else
        {
          m_emit.sub(GetHostReg64(to_reg), Truncate32(value.constant_value));
        }
      }
      else
      {
        m_emit.sub(GetHostReg64(to_reg), GetHostReg64(value.host_reg));
      }
    }
    break;
  }
}

void CodeGenerator::EmitCmp(HostReg to_reg, const Value& value)
{
  DebugAssert(value.IsConstant() || value.IsInHostRegister());

  switch (value.size)
  {
    case OperandSize_8:
    {
      if (value.IsConstant())
        m_emit.cmp(GetHostReg8(to_reg), SignExtend32(Truncate8(value.constant_value)));
      else
        m_emit.cmp(GetHostReg8(to_reg), GetHostReg8(value.host_reg));
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant())
        m_emit.cmp(GetHostReg16(to_reg), SignExtend32(Truncate16(value.constant_value)));
      else
        m_emit.cmp(GetHostReg16(to_reg), GetHostReg16(value.host_reg));
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant())
        m_emit.cmp(GetHostReg32(to_reg), Truncate32(value.constant_value));
      else
        m_emit.cmp(GetHostReg32(to_reg), GetHostReg32(value.host_reg));
    }
    break;

    case OperandSize_64:
    {
      if (value.IsConstant())
      {
        if (!Xbyak::inner::IsInInt32(value.constant_value))
        {
          Value temp = m_register_cache.AllocateScratch(OperandSize_64);
          m_emit.mov(GetHostReg64(temp.host_reg), value.constant_value);
          m_emit.cmp(GetHostReg64(to_reg), GetHostReg64(temp.host_reg));
        }
        else
        {
          m_emit.cmp(GetHostReg64(to_reg), Truncate32(value.constant_value));
        }
      }
      else
      {
        m_emit.cmp(GetHostReg64(to_reg), GetHostReg64(value.host_reg));
      }
    }
    break;
  }
}

void CodeGenerator::EmitInc(HostReg to_reg, OperandSize size)
{
  switch (size)
  {
    case OperandSize_8:
      m_emit.inc(GetHostReg8(to_reg));
      break;
    case OperandSize_16:
      m_emit.inc(GetHostReg16(to_reg));
      break;
    case OperandSize_32:
      m_emit.inc(GetHostReg32(to_reg));
      break;
    default:
      UnreachableCode();
      break;
  }
}

void CodeGenerator::EmitDec(HostReg to_reg, OperandSize size)
{
  switch (size)
  {
    case OperandSize_8:
      m_emit.dec(GetHostReg8(to_reg));
      break;
    case OperandSize_16:
      m_emit.dec(GetHostReg16(to_reg));
      break;
    case OperandSize_32:
      m_emit.dec(GetHostReg32(to_reg));
      break;
    default:
      UnreachableCode();
      break;
  }
}

void CodeGenerator::EmitShl(HostReg to_reg, OperandSize size, const Value& amount_value)
{
  DebugAssert(amount_value.IsConstant() || amount_value.IsInHostRegister());

  // We have to use CL for the shift amount :(
  const bool save_cl = (!amount_value.IsConstant() && m_register_cache.IsHostRegInUse(Xbyak::Operand::RCX) &&
                        (!amount_value.IsInHostRegister() || amount_value.host_reg != Xbyak::Operand::RCX));
  if (save_cl)
    m_emit.push(m_emit.rcx);

  if (!amount_value.IsConstant())
    m_emit.mov(m_emit.cl, GetHostReg8(amount_value.host_reg));

  switch (size)
  {
    case OperandSize_8:
    {
      if (amount_value.IsConstant())
        m_emit.shl(GetHostReg8(to_reg), Truncate8(amount_value.constant_value));
      else
        m_emit.shl(GetHostReg8(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_16:
    {
      if (amount_value.IsConstant())
        m_emit.shl(GetHostReg16(to_reg), Truncate8(amount_value.constant_value));
      else
        m_emit.shl(GetHostReg16(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_32:
    {
      if (amount_value.IsConstant())
        m_emit.shl(GetHostReg32(to_reg), Truncate32(amount_value.constant_value));
      else
        m_emit.shl(GetHostReg32(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_64:
    {
      if (amount_value.IsConstant())
        m_emit.shl(GetHostReg64(to_reg), Truncate32(amount_value.constant_value));
      else
        m_emit.shl(GetHostReg64(to_reg), m_emit.cl);
    }
    break;
  }

  if (save_cl)
    m_emit.pop(m_emit.rcx);
}

void CodeGenerator::EmitShr(HostReg to_reg, OperandSize size, const Value& amount_value)
{
  DebugAssert(amount_value.IsConstant() || amount_value.IsInHostRegister());

  // We have to use CL for the shift amount :(
  const bool save_cl = (!amount_value.IsConstant() && m_register_cache.IsHostRegInUse(Xbyak::Operand::RCX) &&
                        (!amount_value.IsInHostRegister() || amount_value.host_reg != Xbyak::Operand::RCX));
  if (save_cl)
    m_emit.push(m_emit.rcx);

  if (!amount_value.IsConstant())
    m_emit.mov(m_emit.cl, GetHostReg8(amount_value.host_reg));

  switch (size)
  {
    case OperandSize_8:
    {
      if (amount_value.IsConstant())
        m_emit.shr(GetHostReg8(to_reg), Truncate8(amount_value.constant_value));
      else
        m_emit.shr(GetHostReg8(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_16:
    {
      if (amount_value.IsConstant())
        m_emit.shr(GetHostReg16(to_reg), Truncate8(amount_value.constant_value));
      else
        m_emit.shr(GetHostReg16(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_32:
    {
      if (amount_value.IsConstant())
        m_emit.shr(GetHostReg32(to_reg), Truncate32(amount_value.constant_value));
      else
        m_emit.shr(GetHostReg32(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_64:
    {
      if (amount_value.IsConstant())
        m_emit.shr(GetHostReg64(to_reg), Truncate32(amount_value.constant_value));
      else
        m_emit.shr(GetHostReg64(to_reg), m_emit.cl);
    }
    break;
  }

  if (save_cl)
    m_emit.pop(m_emit.rcx);
}

void CodeGenerator::EmitSar(HostReg to_reg, OperandSize size, const Value& amount_value)
{
  DebugAssert(amount_value.IsConstant() || amount_value.IsInHostRegister());

  // We have to use CL for the shift amount :(
  const bool save_cl = (!amount_value.IsConstant() && m_register_cache.IsHostRegInUse(Xbyak::Operand::RCX) &&
                        (!amount_value.IsInHostRegister() || amount_value.host_reg != Xbyak::Operand::RCX));
  if (save_cl)
    m_emit.push(m_emit.rcx);

  if (!amount_value.IsConstant())
    m_emit.mov(m_emit.cl, GetHostReg8(amount_value.host_reg));

  switch (size)
  {
    case OperandSize_8:
    {
      if (amount_value.IsConstant())
        m_emit.sar(GetHostReg8(to_reg), Truncate8(amount_value.constant_value));
      else
        m_emit.sar(GetHostReg8(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_16:
    {
      if (amount_value.IsConstant())
        m_emit.sar(GetHostReg16(to_reg), Truncate8(amount_value.constant_value));
      else
        m_emit.sar(GetHostReg16(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_32:
    {
      if (amount_value.IsConstant())
        m_emit.sar(GetHostReg32(to_reg), Truncate32(amount_value.constant_value));
      else
        m_emit.sar(GetHostReg32(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_64:
    {
      if (amount_value.IsConstant())
        m_emit.sar(GetHostReg64(to_reg), Truncate32(amount_value.constant_value));
      else
        m_emit.sar(GetHostReg64(to_reg), m_emit.cl);
    }
    break;
  }

  if (save_cl)
    m_emit.pop(m_emit.rcx);
}

void CodeGenerator::EmitAnd(HostReg to_reg, const Value& value)
{
  DebugAssert(value.IsConstant() || value.IsInHostRegister());
  switch (value.size)
  {
    case OperandSize_8:
    {
      if (value.IsConstant())
        m_emit.and_(GetHostReg8(to_reg), Truncate32(value.constant_value & UINT32_C(0xFF)));
      else
        m_emit.and_(GetHostReg8(to_reg), GetHostReg8(value));
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant())
        m_emit.and_(GetHostReg16(to_reg), Truncate32(value.constant_value & UINT32_C(0xFFFF)));
      else
        m_emit.and_(GetHostReg16(to_reg), GetHostReg16(value));
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant())
        m_emit.and_(GetHostReg32(to_reg), Truncate32(value.constant_value));
      else
        m_emit.and_(GetHostReg32(to_reg), GetHostReg32(value));
    }
    break;

    case OperandSize_64:
    {
      if (value.IsConstant())
      {
        if (!Xbyak::inner::IsInInt32(value.constant_value))
        {
          Value temp = m_register_cache.AllocateScratch(OperandSize_64);
          m_emit.mov(GetHostReg64(temp), value.constant_value);
          m_emit.and_(GetHostReg64(to_reg), GetHostReg64(temp));
        }
        else
        {
          m_emit.and_(GetHostReg64(to_reg), Truncate32(value.constant_value));
        }
      }
      else
      {
        m_emit.and_(GetHostReg64(to_reg), GetHostReg64(value));
      }
    }
    break;
  }
}

void CodeGenerator::EmitOr(HostReg to_reg, const Value& value)
{
  DebugAssert(value.IsConstant() || value.IsInHostRegister());
  switch (value.size)
  {
    case OperandSize_8:
    {
      if (value.IsConstant())
        m_emit.or_(GetHostReg8(to_reg), Truncate32(value.constant_value & UINT32_C(0xFF)));
      else
        m_emit.or_(GetHostReg8(to_reg), GetHostReg8(value));
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant())
        m_emit.or_(GetHostReg16(to_reg), Truncate32(value.constant_value & UINT32_C(0xFFFF)));
      else
        m_emit.or_(GetHostReg16(to_reg), GetHostReg16(value));
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant())
        m_emit.or_(GetHostReg32(to_reg), Truncate32(value.constant_value));
      else
        m_emit.or_(GetHostReg32(to_reg), GetHostReg32(value));
    }
    break;

    case OperandSize_64:
    {
      if (value.IsConstant())
      {
        if (!Xbyak::inner::IsInInt32(value.constant_value))
        {
          Value temp = m_register_cache.AllocateScratch(OperandSize_64);
          m_emit.mov(GetHostReg64(temp), value.constant_value);
          m_emit.or_(GetHostReg64(to_reg), GetHostReg64(temp));
        }
        else
        {
          m_emit.or_(GetHostReg64(to_reg), Truncate32(value.constant_value));
        }
      }
      else
      {
        m_emit.or_(GetHostReg64(to_reg), GetHostReg64(value));
      }
    }
    break;
  }
}

void CodeGenerator::EmitXor(HostReg to_reg, const Value& value)
{
  DebugAssert(value.IsConstant() || value.IsInHostRegister());
  switch (value.size)
  {
    case OperandSize_8:
    {
      if (value.IsConstant())
        m_emit.xor_(GetHostReg8(to_reg), Truncate32(value.constant_value & UINT32_C(0xFF)));
      else
        m_emit.xor_(GetHostReg8(to_reg), GetHostReg8(value));
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant())
        m_emit.xor_(GetHostReg16(to_reg), Truncate32(value.constant_value & UINT32_C(0xFFFF)));
      else
        m_emit.xor_(GetHostReg16(to_reg), GetHostReg16(value));
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant())
        m_emit.xor_(GetHostReg32(to_reg), Truncate32(value.constant_value));
      else
        m_emit.xor_(GetHostReg32(to_reg), GetHostReg32(value));
    }
    break;

    case OperandSize_64:
    {
      if (value.IsConstant())
      {
        if (!Xbyak::inner::IsInInt32(value.constant_value))
        {
          Value temp = m_register_cache.AllocateScratch(OperandSize_64);
          m_emit.mov(GetHostReg64(temp), value.constant_value);
          m_emit.xor_(GetHostReg64(to_reg), GetHostReg64(temp));
        }
        else
        {
          m_emit.xor_(GetHostReg64(to_reg), Truncate32(value.constant_value));
        }
      }
      else
      {
        m_emit.xor_(GetHostReg64(to_reg), GetHostReg64(value));
      }
    }
    break;
  }
}

void CodeGenerator::EmitTest(HostReg to_reg, const Value& value)
{
  DebugAssert(value.IsConstant() || value.IsInHostRegister());
  switch (value.size)
  {
    case OperandSize_8:
    {
      if (value.IsConstant())
        m_emit.test(GetHostReg8(to_reg), Truncate32(value.constant_value & UINT32_C(0xFF)));
      else
        m_emit.test(GetHostReg8(to_reg), GetHostReg8(value));
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant())
        m_emit.test(GetHostReg16(to_reg), Truncate32(value.constant_value & UINT32_C(0xFFFF)));
      else
        m_emit.test(GetHostReg16(to_reg), GetHostReg16(value));
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant())
        m_emit.test(GetHostReg32(to_reg), Truncate32(value.constant_value));
      else
        m_emit.test(GetHostReg32(to_reg), GetHostReg32(value));
    }
    break;

    case OperandSize_64:
    {
      if (value.IsConstant())
      {
        if (!Xbyak::inner::IsInInt32(value.constant_value))
        {
          Value temp = m_register_cache.AllocateScratch(OperandSize_64);
          m_emit.mov(GetHostReg64(temp), value.constant_value);
          m_emit.test(GetHostReg64(to_reg), GetHostReg64(temp));
        }
        else
        {
          m_emit.test(GetHostReg64(to_reg), Truncate32(value.constant_value));
        }
      }
      else
      {
        m_emit.test(GetHostReg64(to_reg), GetHostReg64(value));
      }
    }
    break;
  }
}

void CodeGenerator::EmitNot(HostReg to_reg, OperandSize size)
{
  switch (size)
  {
    case OperandSize_8:
      m_emit.not(GetHostReg8(to_reg));
      break;

    case OperandSize_16:
      m_emit.not(GetHostReg16(to_reg));
      break;

    case OperandSize_32:
      m_emit.not(GetHostReg32(to_reg));
      break;

    case OperandSize_64:
      m_emit.not(GetHostReg64(to_reg));
      break;

    default:
      break;
  }
}

u32 CodeGenerator::PrepareStackForCall()
{
  // we assume that the stack is unaligned at this point
  const u32 num_callee_saved = m_register_cache.GetActiveCalleeSavedRegisterCount();
  const u32 num_caller_saved = m_register_cache.PushCallerSavedRegisters();
  const u32 current_offset = 8 + (num_callee_saved + num_caller_saved) * 8;
  const u32 aligned_offset = Common::AlignUp(current_offset + FUNCTION_CALL_SHADOW_SPACE, 16);
  const u32 adjust_size = aligned_offset - current_offset;
  if (adjust_size > 0)
    m_emit.sub(m_emit.rsp, adjust_size);

  return adjust_size;
}

void CodeGenerator::RestoreStackAfterCall(u32 adjust_size)
{
  if (adjust_size > 0)
    m_emit.add(m_emit.rsp, adjust_size);

  m_register_cache.PopCallerSavedRegisters();
}

void CodeGenerator::EmitFunctionCallPtr(Value* return_value, const void* ptr)
{
  if (return_value)
    return_value->Discard();

  // shadow space allocate
  const u32 adjust_size = PrepareStackForCall();

  // actually call the function
  m_emit.mov(GetHostReg64(RRETURN), reinterpret_cast<size_t>(ptr));
  m_emit.call(GetHostReg64(RRETURN));

  // shadow space release
  RestoreStackAfterCall(adjust_size);

  // copy out return value if requested
  if (return_value)
  {
    return_value->Undiscard();
    EmitCopyValue(return_value->GetHostRegister(), Value::FromHostReg(&m_register_cache, RRETURN, return_value->size));
  }
}

void CodeGenerator::EmitFunctionCallPtr(Value* return_value, const void* ptr, const Value& arg1)
{
  if (return_value)
    return_value->Discard();

  // shadow space allocate
  const u32 adjust_size = PrepareStackForCall();

  // push arguments
  EmitCopyValue(RARG1, arg1);

  // actually call the function
  if (Xbyak::inner::IsInInt32(reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(m_emit.getCurr())))
  {
    m_emit.call(ptr);
  }
  else
  {
    m_emit.mov(GetHostReg64(RRETURN), reinterpret_cast<size_t>(ptr));
    m_emit.call(GetHostReg64(RRETURN));
  }

  // shadow space release
  RestoreStackAfterCall(adjust_size);

  // copy out return value if requested
  if (return_value)
  {
    return_value->Undiscard();
    EmitCopyValue(return_value->GetHostRegister(), Value::FromHostReg(&m_register_cache, RRETURN, return_value->size));
  }
}

void CodeGenerator::EmitFunctionCallPtr(Value* return_value, const void* ptr, const Value& arg1, const Value& arg2)
{
  if (return_value)
    return_value->Discard();

  // shadow space allocate
  const u32 adjust_size = PrepareStackForCall();

  // push arguments
  EmitCopyValue(RARG1, arg1);
  EmitCopyValue(RARG2, arg2);

  // actually call the function
  if (Xbyak::inner::IsInInt32(reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(m_emit.getCurr())))
  {
    m_emit.call(ptr);
  }
  else
  {
    m_emit.mov(GetHostReg64(RRETURN), reinterpret_cast<size_t>(ptr));
    m_emit.call(GetHostReg64(RRETURN));
  }

  // shadow space release
  RestoreStackAfterCall(adjust_size);

  // copy out return value if requested
  if (return_value)
  {
    return_value->Undiscard();
    EmitCopyValue(return_value->GetHostRegister(), Value::FromHostReg(&m_register_cache, RRETURN, return_value->size));
  }
}

void CodeGenerator::EmitFunctionCallPtr(Value* return_value, const void* ptr, const Value& arg1, const Value& arg2,
                                        const Value& arg3)
{
  if (return_value)
    m_register_cache.DiscardHostReg(return_value->GetHostRegister());

  // shadow space allocate
  const u32 adjust_size = PrepareStackForCall();

  // push arguments
  EmitCopyValue(RARG1, arg1);
  EmitCopyValue(RARG2, arg2);
  EmitCopyValue(RARG3, arg3);

  // actually call the function
  if (Xbyak::inner::IsInInt32(reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(m_emit.getCurr())))
  {
    m_emit.call(ptr);
  }
  else
  {
    m_emit.mov(GetHostReg64(RRETURN), reinterpret_cast<size_t>(ptr));
    m_emit.call(GetHostReg64(RRETURN));
  }

  // shadow space release
  RestoreStackAfterCall(adjust_size);

  // copy out return value if requested
  if (return_value)
  {
    return_value->Undiscard();
    EmitCopyValue(return_value->GetHostRegister(), Value::FromHostReg(&m_register_cache, RRETURN, return_value->size));
  }
}

void CodeGenerator::EmitFunctionCallPtr(Value* return_value, const void* ptr, const Value& arg1, const Value& arg2,
                                        const Value& arg3, const Value& arg4)
{
  if (return_value)
    return_value->Discard();

  // shadow space allocate
  const u32 adjust_size = PrepareStackForCall();

  // push arguments
  EmitCopyValue(RARG1, arg1);
  EmitCopyValue(RARG2, arg2);
  EmitCopyValue(RARG3, arg3);
  EmitCopyValue(RARG4, arg4);

  // actually call the function
  if (Xbyak::inner::IsInInt32(reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(m_emit.getCurr())))
  {
    m_emit.call(ptr);
  }
  else
  {
    m_emit.mov(GetHostReg64(RRETURN), reinterpret_cast<size_t>(ptr));
    m_emit.call(GetHostReg64(RRETURN));
  }

  // shadow space release
  RestoreStackAfterCall(adjust_size);

  // copy out return value if requested
  if (return_value)
  {
    return_value->Undiscard();
    EmitCopyValue(return_value->GetHostRegister(), Value::FromHostReg(&m_register_cache, RRETURN, return_value->size));
  }
}

void CodeGenerator::EmitPushHostReg(HostReg reg)
{
  m_emit.push(GetHostReg64(reg));
}

void CodeGenerator::EmitPopHostReg(HostReg reg)
{
  m_emit.pop(GetHostReg64(reg));
}

void CodeGenerator::ReadFlagsFromHost(Value* value)
{
  // this is a 64-bit push/pop, we ignore the upper 32 bits
  DebugAssert(value->IsInHostRegister());
  m_emit.pushf();
  m_emit.pop(GetHostReg64(value->host_reg));
}

Value CodeGenerator::ReadFlagsFromHost()
{
  Value temp = m_register_cache.AllocateScratch(OperandSize_32);
  ReadFlagsFromHost(&temp);
  return temp;
}

void CodeGenerator::EmitLoadGuestRegister(HostReg host_reg, OperandSize guest_size, u8 guest_reg)
{
  switch (guest_size)
  {
    case OperandSize_8:
      EmitLoadCPUStructField(host_reg, OperandSize_8, CalculateRegisterOffset(static_cast<Reg8>(guest_reg)));
      break;

    case OperandSize_16:
      EmitLoadCPUStructField(host_reg, OperandSize_16, CalculateRegisterOffset(static_cast<Reg16>(guest_reg)));
      break;

    case OperandSize_32:
      EmitLoadCPUStructField(host_reg, OperandSize_32, CalculateRegisterOffset(static_cast<Reg32>(guest_reg)));
      break;

    default:
      UnreachableCode();
      break;
  }
}

void CodeGenerator::EmitStoreGuestRegister(OperandSize guest_size, u8 guest_reg, const Value& value)
{
  DebugAssert(guest_size == value.size);
  switch (guest_size)
  {
    case OperandSize_8:
      EmitStoreCPUStructField(CalculateRegisterOffset(static_cast<Reg8>(guest_reg)), value);
      break;

    case OperandSize_16:
      EmitStoreCPUStructField(CalculateRegisterOffset(static_cast<Reg16>(guest_reg)), value);
      break;

    case OperandSize_32:
      EmitStoreCPUStructField(CalculateRegisterOffset(static_cast<Reg32>(guest_reg)), value);
      break;

    default:
      UnreachableCode();
      break;
  }
}

void CodeGenerator::EmitLoadCPUStructField(HostReg host_reg, OperandSize guest_size, u32 offset)
{
  switch (guest_size)
  {
    case OperandSize_8:
      m_emit.mov(GetHostReg8(host_reg), m_emit.byte[GetCPUPtrReg() + offset]);
      break;

    case OperandSize_16:
      m_emit.mov(GetHostReg16(host_reg), m_emit.word[GetCPUPtrReg() + offset]);
      break;

    case OperandSize_32:
      m_emit.mov(GetHostReg32(host_reg), m_emit.dword[GetCPUPtrReg() + offset]);
      break;

    case OperandSize_64:
      m_emit.mov(GetHostReg64(host_reg), m_emit.qword[GetCPUPtrReg() + offset]);
      break;

    default:
    {
      UnreachableCode();
    }
    break;
  }
}

void CodeGenerator::EmitStoreCPUStructField(u32 offset, const Value& value)
{
  DebugAssert(value.IsInHostRegister() || value.IsConstant());
  switch (value.size)
  {
    case OperandSize_8:
    {
      if (value.IsConstant())
        m_emit.mov(m_emit.byte[GetCPUPtrReg() + offset], value.constant_value);
      else
        m_emit.mov(m_emit.byte[GetCPUPtrReg() + offset], GetHostReg8(value.host_reg));
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant())
        m_emit.mov(m_emit.word[GetCPUPtrReg() + offset], value.constant_value);
      else
        m_emit.mov(m_emit.word[GetCPUPtrReg() + offset], GetHostReg16(value.host_reg));
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant())
        m_emit.mov(m_emit.dword[GetCPUPtrReg() + offset], value.constant_value);
      else
        m_emit.mov(m_emit.dword[GetCPUPtrReg() + offset], GetHostReg32(value.host_reg));
    }
    break;

    case OperandSize_64:
    {
      if (value.IsConstant())
      {
        // we need a temporary to load the value if it doesn't fit in 32-bits
        if (!Xbyak::inner::IsInInt32(value.constant_value))
        {
          Value temp = m_register_cache.AllocateScratch(OperandSize_64);
          EmitCopyValue(temp.host_reg, value);
          m_emit.mov(m_emit.qword[GetCPUPtrReg() + offset], GetHostReg64(temp.host_reg));
        }
        else
        {
          m_emit.mov(m_emit.qword[GetCPUPtrReg() + offset], value.constant_value);
        }
      }
      else
      {
        m_emit.mov(m_emit.qword[GetCPUPtrReg() + offset], GetHostReg64(value.host_reg));
      }
    }
    break;

    default:
    {
      UnreachableCode();
    }
    break;
  }
}

void CodeGenerator::EmitAddCPUStructField(u32 offset, const Value& value)
{
  DebugAssert(value.IsInHostRegister() || value.IsConstant());
  switch (value.size)
  {
    case OperandSize_8:
    {
      if (value.IsConstant() && value.constant_value == 1)
        m_emit.inc(m_emit.byte[GetCPUPtrReg() + offset]);
      else if (value.IsConstant())
        m_emit.add(m_emit.byte[GetCPUPtrReg() + offset], Truncate32(value.constant_value));
      else
        m_emit.add(m_emit.byte[GetCPUPtrReg() + offset], GetHostReg8(value.host_reg));
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant() && value.constant_value == 1)
        m_emit.inc(m_emit.word[GetCPUPtrReg() + offset]);
      else if (value.IsConstant())
        m_emit.add(m_emit.word[GetCPUPtrReg() + offset], Truncate32(value.constant_value));
      else
        m_emit.add(m_emit.word[GetCPUPtrReg() + offset], GetHostReg16(value.host_reg));
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant() && value.constant_value == 1)
        m_emit.inc(m_emit.dword[GetCPUPtrReg() + offset]);
      else if (value.IsConstant())
        m_emit.add(m_emit.dword[GetCPUPtrReg() + offset], Truncate32(value.constant_value));
      else
        m_emit.add(m_emit.dword[GetCPUPtrReg() + offset], GetHostReg32(value.host_reg));
    }
    break;

    case OperandSize_64:
    {
      if (value.IsConstant() && value.constant_value == 1)
      {
        m_emit.inc(m_emit.qword[GetCPUPtrReg() + offset]);
      }
      else if (value.IsConstant())
      {
        // we need a temporary to load the value if it doesn't fit in 32-bits
        if (!Xbyak::inner::IsInInt32(value.constant_value))
        {
          Value temp = m_register_cache.AllocateScratch(OperandSize_64);
          EmitCopyValue(temp.host_reg, value);
          m_emit.add(m_emit.qword[GetCPUPtrReg() + offset], GetHostReg64(temp.host_reg));
        }
        else
        {
          m_emit.add(m_emit.qword[GetCPUPtrReg() + offset], Truncate32(value.constant_value));
        }
      }
      else
      {
        m_emit.add(m_emit.qword[GetCPUPtrReg() + offset], GetHostReg64(value.host_reg));
      }
    }
    break;

    default:
    {
      UnreachableCode();
    }
    break;
  }
}

Value CodeGenerator::GetSignFlag(const Value& value)
{
  Value ret = m_register_cache.AllocateScratch(OperandSize_32);
  Xbyak::Reg32 ret_reg = GetHostReg32(ret);

  switch (value.size)
  {
    case OperandSize_8:
      m_emit.movzx(ret_reg, GetHostReg8(value));
      break;

    case OperandSize_16:
      m_emit.movzx(ret_reg, GetHostReg16(value));
      m_emit.shr(ret_reg, 8);
      break;

    case OperandSize_32:
      m_emit.mov(ret_reg, GetHostReg32(value));
      m_emit.shr(ret_reg, 24);
      break;

    default:
      UnreachableCode();
      return {};
  }

  m_emit.and_(ret_reg, Flag_SF);
  return ret;
}

Value CodeGenerator::GetZeroFlag(const Value& value)
{
  switch (value.size)
  {
    case OperandSize_8:
      m_emit.cmp(GetHostReg8(value), 0);
      break;

    case OperandSize_16:
      m_emit.cmp(GetHostReg16(value), 0);
      break;

    case OperandSize_32:
      m_emit.cmp(GetHostReg32(value), 0);
      break;

    default:
      UnreachableCode();
      return {};
  }

  Value ret = m_register_cache.AllocateScratch(OperandSize_32);
  Xbyak::Reg32 ret_reg = GetHostReg32(ret);
  m_emit.setz(ret_reg.cvt8());
  m_emit.movzx(ret_reg, ret_reg.cvt8());
  m_emit.shl(ret_reg, 6); // Flag_ZF
  return ret;
}

Value CodeGenerator::GetParityFlag(const Value& value)
{
  Value ret = m_register_cache.AllocateScratch(OperandSize_32);
  Xbyak::Reg32 ret_reg = GetHostReg32(ret);

  // movzx takes the place of AND'ing with 0xFF.
  DebugAssert(value.IsInHostRegister());
  m_emit.movzx(ret_reg, GetHostReg8(value.host_reg));
  m_emit.popcnt(ret_reg, ret_reg);
  m_emit.not_(ret_reg);
  m_emit.shl(ret_reg, 2);
  m_emit.and_(ret_reg, Flag_PF);
  return ret;
}

bool CodeGenerator::Compile_Bitwise_Impl(const Instruction& instruction, CycleCount cycles)
{
  InstructionPrologue(instruction, cycles);
  CalculateEffectiveAddress(instruction);

  const OperandSize size = instruction.operands[0].size;
  Value lhs = ReadOperand(instruction, 0, size, false, true);
  Value rhs = ReadOperand(instruction, 1, size, true, false);

  switch (instruction.operation)
  {
    case Operation_AND:
      EmitAnd(lhs.GetHostRegister(), rhs);
      break;

    case Operation_OR:
      EmitOr(lhs.GetHostRegister(), rhs);
      break;

    case Operation_XOR:
      EmitXor(lhs.GetHostRegister(), rhs);
      break;

    case Operation_TEST:
      EmitTest(lhs.GetHostRegister(), rhs);
      break;

    default:
      break;
  }

  Value host_flags = ReadFlagsFromHost();

  if (instruction.operation != Operation_TEST)
    WriteOperand(instruction, 0, std::move(lhs));

  const u32 clear_flags_mask = Flag_OF | Flag_CF | Flag_AF;
  const u32 copy_flags_mask = Flag_SF | Flag_ZF | Flag_PF;
  UpdateEFLAGS(std::move(host_flags), clear_flags_mask, copy_flags_mask, 0);
  return true;
}

bool CodeGenerator::Compile_Shift_Impl(const Instruction& instruction, CycleCount cycles)
{
  InstructionPrologue(instruction, cycles);
  CalculateEffectiveAddress(instruction);

  bool is_unary_version = instruction.operands[1].mode == OperandMode_Constant;
  const OperandSize size = instruction.operands[0].size;
  Value val = ReadOperand(instruction, 0, size, false, true);
  Value shift_amount = ReadOperand(instruction, 1, OperandSize_8, true, false);
  if (shift_amount.HasConstantValue(0))
  {
    // shift zero does nothing and effects no flags.
    return true;
  }

  if (shift_amount.IsInHostRegister())
    m_emit.and_(GetHostReg8(shift_amount), 0x1F);
  else
    shift_amount.constant_value &= 0x1F;

  // allocate storage for the host flags before the jump, because it can save
  // a callee-saved register. also read/cache the flags for the same reason
  Value host_flags = m_register_cache.AllocateScratch(OperandSize_32);
  m_register_cache.ReadGuestRegister(Reg32_EFLAGS, true, true);

  Xbyak::Label noop_label;
  if (!shift_amount.IsConstant())
  {
    m_emit.test(GetHostReg8(shift_amount), GetHostReg8(shift_amount));
    m_emit.jz(noop_label);
  }

  switch (instruction.operation)
  {
    case Operation_SHL:
      EmitShl(val.GetHostRegister(), val.size, shift_amount);
      break;

    case Operation_SHR:
      EmitShr(val.GetHostRegister(), val.size, shift_amount);
      break;

    case Operation_SAR:
      EmitSar(val.GetHostRegister(), val.size, shift_amount);
      break;

    default:
      UnreachableCode();
      break;
  }

  ReadFlagsFromHost(&host_flags);
  WriteOperand(instruction, 0, std::move(val));
  if (instruction.operation == Operation_SAR || !is_unary_version)
    UpdateEFLAGS(std::move(host_flags), Flag_OF, Flag_CF | Flag_SF | Flag_ZF | Flag_PF, 0);
  else
    UpdateEFLAGS(std::move(host_flags), 0, Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF, 0);

  if (!shift_amount.IsConstant())
    m_emit.L(noop_label);

  return true;
}

bool CodeGenerator::Compile_AddSub_Impl(const Instruction& instruction, CycleCount cycles)
{
  InstructionPrologue(instruction, cycles);
  CalculateEffectiveAddress(instruction);

  const OperandSize size = instruction.operands[0].size;
  Value lhs = ReadOperand(instruction, 0, size, false, true);
  Value rhs = ReadOperand(instruction, 1, size, true, false);

  switch (instruction.operation)
  {
    case Operation_ADD:
      EmitAdd(lhs.GetHostRegister(), rhs);
      break;

    case Operation_SUB:
      EmitSub(lhs.GetHostRegister(), rhs);
      break;

    case Operation_CMP:
      EmitCmp(lhs.GetHostRegister(), rhs);
      break;

    default:
      UnreachableCode();
      break;
  }

  Value host_flags = ReadFlagsFromHost();

  if (instruction.operation != Operation_CMP)
    WriteOperand(instruction, 0, std::move(lhs));

  const u32 eflags_mask = Flag_OF | Flag_CF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF;
  UpdateEFLAGS(std::move(host_flags), 0, eflags_mask, 0);
  return true;
}

bool CodeGenerator::Compile_IncDec_Impl(const Instruction& instruction, CycleCount cycles)
{
  InstructionPrologue(instruction, cycles);
  CalculateEffectiveAddress(instruction);

  Value val = ReadOperand(instruction, 0, instruction.operands[0].size, false, true);
  if (instruction.operation == Operation_INC)
    EmitInc(val.GetHostRegister(), val.size);
  else if (instruction.operation == Operation_DEC)
    EmitDec(val.GetHostRegister(), val.size);
  else
    Panic("Unknown operation");

  Value host_flags = ReadFlagsFromHost();
  WriteOperand(instruction, 0, std::move(val));

  const u32 eflags_mask = Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF;
  UpdateEFLAGS(std::move(host_flags), 0, eflags_mask, 0);
  return true;
}

bool CodeGenerator::Compile_String(const Instruction& instruction)
{
  const CycleCount cycles_base = m_cpu->GetCycles(instruction.IsRep() ? CYCLES_REP_MOVS_BASE : CYCLES_MOVS);
  const CycleCount cycles_n = m_cpu->GetCycles(CYCLES_REP_MOVS_N) + 1;
  const u32 data_size = GetOperandSizeInBytes(instruction.operands[0].size);
  const OperandSize reg_address_size =
    ((instruction.GetAddressSize() == AddressSize_32) ? OperandSize_32 : OperandSize_16);
  const bool is_rep = instruction.IsRep();
  const bool is_32bit = (instruction.GetAddressSize() == AddressSize_32);
  const bool needs_eax = (instruction.operation == Operation_LODS || instruction.operation == Operation_STOS ||
                          instruction.operation == Operation_SCAS);
  const bool needs_esi = (instruction.operation == Operation_MOVS || instruction.operation == Operation_CMPS ||
                          instruction.operation == Operation_LODS);
  const bool needs_edi = (instruction.operation == Operation_MOVS || instruction.operation == Operation_CMPS ||
                          instruction.operation == Operation_STOS || instruction.operation == Operation_SCAS);
  const bool needs_lhs = (instruction.operation == Operation_MOVS || instruction.operation == Operation_CMPS);
  const bool needs_rhs = (instruction.operation == Operation_CMPS);

  InstructionPrologue(instruction, cycles_base, true);

  // work out the increment (positive or negative)
  Value increment = m_register_cache.AllocateScratch(reg_address_size);
  {
    Value eflags = m_register_cache.ReadGuestRegister(Reg32_EFLAGS, true, true);
    Value increment_temp = m_register_cache.AllocateScratch(reg_address_size);
    if (is_32bit)
    {
      EmitCopyValue(increment.GetHostRegister(), Value::FromConstantU32(data_size));
      EmitCopyValue(increment_temp.GetHostRegister(), Value::FromConstantU32(u32(-s32(data_size))));
      m_emit.test(GetHostReg32(eflags), Flag_DF);
      m_emit.cmovnz(GetHostReg32(increment), GetHostReg32(increment_temp));
    }
    else
    {
      EmitCopyValue(increment.GetHostRegister(), Value::FromConstantU16(u16(data_size)));
      EmitCopyValue(increment_temp.GetHostRegister(), Value::FromConstantU16(u16(-s16(data_size))));
      m_emit.test(GetHostReg32(eflags), Flag_DF);
      m_emit.cmovnz(GetHostReg16(increment), GetHostReg16(increment_temp));
    }
  }

  // temporary for the value read, and zero-extended address (if in 16-bit mode)
  Value lhs;
  Value rhs;
  Value address;
  Value eax;
  Value ecx;
  Value esi;
  Value edi;
  if (!is_32bit)
    address = m_register_cache.AllocateScratch(OperandSize_32);
  if (needs_lhs)
    lhs = m_register_cache.AllocateScratch(instruction.operands[0].size);
  if (needs_rhs)
    rhs = m_register_cache.AllocateScratch(instruction.operands[0].size);
  if (needs_eax)
    eax = m_register_cache.ReadGuestRegister(instruction.operands[0].size, Reg32_EAX, true, true);
  if (needs_esi)
    esi = m_register_cache.ReadGuestRegister(reg_address_size, Reg32_ESI, true, true);
  if (needs_edi)
    edi = m_register_cache.ReadGuestRegister(reg_address_size, Reg32_EDI, true, true);
  if (is_rep)
    ecx = m_register_cache.ReadGuestRegister(reg_address_size, Reg32_ECX, true, true);

  // everything should be in the register cache still
  Assert((!needs_eax || m_register_cache.IsGuestRegisterInHostReg(instruction.operands[0].size, Reg32_EAX)) &&
         (!needs_esi || m_register_cache.IsGuestRegisterInHostReg(reg_address_size, Reg32_ESI)) &&
         (!needs_edi || m_register_cache.IsGuestRegisterInHostReg(reg_address_size, Reg32_EDI)) &&
         (!is_rep || m_register_cache.IsGuestRegisterInHostReg(reg_address_size, Reg32_ECX)));

  Xbyak::Label rep_label;
  Xbyak::Label done_label;
  if (is_rep)
  {
    m_emit.L(rep_label);

    // add n cycles
    EmitAddCPUStructField(offsetof(CPU, m_pending_cycles), Value::FromConstantU64(cycles_n));

    // compare ecx against zero
    EmitTest(ecx.GetHostRegister(), ecx);
    m_emit.jz(done_label);
  }

  switch (instruction.operation)
  {
    case Operation_MOVS:
    {
      // read from esi, write to es:edi
      if (is_32bit)
      {
        LoadSegmentMemory(&lhs, lhs.size, esi, instruction.GetMemorySegment());
        StoreSegmentMemory(lhs, edi, Segment_ES);
      }
      else
      {
        EmitZeroExtend(address.GetHostRegister(), OperandSize_32, esi.GetHostRegister(), OperandSize_16);
        LoadSegmentMemory(&lhs, lhs.size, address, instruction.GetMemorySegment());
        EmitZeroExtend(address.GetHostRegister(), OperandSize_32, edi.GetHostRegister(), OperandSize_16);
        StoreSegmentMemory(lhs, address, Segment_ES);
      }
    }
    break;

    case Operation_LODS:
    {
      // read from esi, write to (e)ax
      if (is_32bit)
      {
        LoadSegmentMemory(&eax, eax.size, esi, instruction.GetMemorySegment());
      }
      else
      {
        EmitZeroExtend(address.GetHostRegister(), OperandSize_32, esi.GetHostRegister(), OperandSize_16);
        LoadSegmentMemory(&eax, eax.size, address, instruction.GetMemorySegment());
      }
      eax = m_register_cache.WriteGuestRegister(eax.size, Reg32_EAX, std::move(eax));
    }
    break;

    case Operation_STOS:
    {
      // read from (e)ax, write to [es:edi]
      if (is_32bit)
      {
        StoreSegmentMemory(eax, edi, Segment_ES);
      }
      else
      {
        EmitZeroExtend(address.GetHostRegister(), OperandSize_32, edi.GetHostRegister(), OperandSize_16);
        StoreSegmentMemory(eax, address, Segment_ES);
      }
    }
    break;

    default:
      UnreachableCode();
      break;
  }

  // increment edi/esi
  if (needs_esi)
  {
    EmitAdd(esi.GetHostRegister(), increment);
    esi = m_register_cache.WriteGuestRegister(reg_address_size, Reg32_ESI, std::move(esi));
  }
  if (needs_edi)
  {
    EmitAdd(edi.GetHostRegister(), increment);
    edi = m_register_cache.WriteGuestRegister(reg_address_size, Reg32_EDI, std::move(edi));
  }

  if (is_rep)
  {
    // reduce ecx
    EmitDec(ecx.GetHostRegister(), ecx.size);
    ecx = m_register_cache.WriteGuestRegister(reg_address_size, Reg32_ECX, std::move(ecx));

    // flush new esi/edi/ecx values, since we can fault after the next loop
    if (needs_esi)
      m_register_cache.FlushGuestRegister(reg_address_size, Reg32_ESI, false);
    if (needs_edi)
      m_register_cache.FlushGuestRegister(reg_address_size, Reg32_EDI, false);
    m_register_cache.FlushGuestRegister(reg_address_size, Reg32_ECX, false);

    // jump if at the end with condition, the flushes shouldn't change the flags
    // m_emit.jz(done_label);

    m_emit.jmp(rep_label);
  }

  m_emit.L(done_label);
  return true;
}

class ThunkGenerator
{
public:
  template<typename DataType>
  static DataType (*CompileMemoryReadFunction(JitCodeBuffer* code_buffer))(u8, u32)
  {
    using FunctionType = DataType (*)(u8, u32);
    const auto rret = GetHostReg64(RRETURN);
    const auto rcpuptr = GetHostReg64(RCPUPTR);
    const auto rarg1 = GetHostReg32(RARG1);
    const auto rarg2 = GetHostReg32(RARG2);
    const auto rarg3 = GetHostReg32(RARG3);
    const auto scratch = GetHostReg64(RARG3);

    Xbyak::CodeGenerator emitter(code_buffer->GetFreeCodeSpace(), code_buffer->GetFreeCodePointer());

    // ensure function starts at aligned 16 bytes
    emitter.align();
    FunctionType ret = emitter.getCurr<FunctionType>();

    // TODO: We can skip these if the base address is zero and the size is 4GB.
    Xbyak::Label raise_gpf_label;

    static_assert(sizeof(CPU::SegmentCache) == 16);
    emitter.movzx(rarg1, rarg1.cvt8());
    emitter.shl(rarg1, 4);
    emitter.lea(rret, emitter.byte[rcpuptr + rarg1.cvt64() + offsetof(CPU, m_segment_cache[0])]);

    // if segcache->access_mask & Read == 0
    emitter.test(emitter.byte[rret + offsetof(CPU::SegmentCache, access_mask)], static_cast<u32>(AccessTypeMask::Read));
    emitter.jz(raise_gpf_label);

    // if offset < limit_low
    emitter.cmp(rarg2, emitter.dword[rret + offsetof(CPU::SegmentCache, limit_low)]);
    emitter.jb(raise_gpf_label);

    // if offset + (size - 1) > limit_high
    // offset += segcache->base_address
    if constexpr (sizeof(DataType) > 1)
    {
      emitter.lea(scratch, emitter.qword[rarg2.cvt64() + (sizeof(DataType) - 1)]);
      emitter.add(rarg2, emitter.dword[rret + offsetof(CPU::SegmentCache, base_address)]);
      emitter.mov(rret.cvt32(), emitter.dword[rret + offsetof(CPU::SegmentCache, limit_high)]);
      emitter.cmp(scratch, rret);
      emitter.ja(raise_gpf_label);
    }
    else
    {
      emitter.cmp(rarg2, emitter.dword[rret + offsetof(CPU::SegmentCache, limit_high)]);
      emitter.ja(raise_gpf_label);
      emitter.add(rarg2, emitter.dword[rret + offsetof(CPU::SegmentCache, base_address)]);
    }

    // swap segment with CPU
    emitter.mov(rarg1, rcpuptr);

    // go ahead with the memory read
    if constexpr (std::is_same_v<DataType, u8>)
    {
      emitter.mov(rret, reinterpret_cast<size_t>(static_cast<u8 (*)(CPU*, LinearMemoryAddress)>(&CPU::ReadMemoryByte)));
    }
    else if constexpr (std::is_same_v<DataType, u16>)
    {
      emitter.mov(rret,
                  reinterpret_cast<size_t>(static_cast<u16 (*)(CPU*, LinearMemoryAddress)>(&CPU::ReadMemoryWord)));
    }
    else
    {
      emitter.mov(rret,
                  reinterpret_cast<size_t>(static_cast<u32 (*)(CPU*, LinearMemoryAddress)>(&CPU::ReadMemoryDWord)));
    }

    emitter.jmp(rret);

    // RAISE GPF BRANCH
    emitter.L(raise_gpf_label);

    // register swap since the CPU has to come first
    emitter.cmp(rarg1, (Segment_SS << 4));
    emitter.mov(rarg1, Interrupt_StackFault);
    emitter.mov(rarg2, Interrupt_GeneralProtectionFault);
    emitter.cmove(rarg2, rarg1);
    emitter.xor_(rarg3, rarg3);
    emitter.mov(rarg1, rcpuptr);

    // cpu->RaiseException(ss ? Interrupt_StackFault : Interrupt_GeneralProtectionFault, 0)
    emitter.mov(rret, reinterpret_cast<size_t>(static_cast<void (*)(CPU*, u32, u32)>(&CPU::RaiseException)));
    emitter.jmp(rret);

    emitter.ready();
    code_buffer->CommitCode(emitter.getSize());
    return ret;
  }

  template<typename DataType>
  static void (*CompileMemoryWriteFunction(JitCodeBuffer* code_buffer))(u8, u32, DataType)
  {
    using FunctionType = void (*)(u8, u32, DataType);
    const auto rret = GetHostReg64(RRETURN);
    const auto rcpuptr = GetHostReg64(RCPUPTR);
    const auto rarg1 = GetHostReg32(RARG1);
    const auto rarg2 = GetHostReg32(RARG2);
    const auto rarg3 = GetHostReg32(RARG3);
    const auto scratch = GetHostReg64(RARG4);

    Xbyak::CodeGenerator emitter(code_buffer->GetFreeCodeSpace(), code_buffer->GetFreeCodePointer());

    // ensure function starts at aligned 16 bytes
    emitter.align();
    FunctionType ret = emitter.getCurr<FunctionType>();

    // TODO: We can skip these if the base address is zero and the size is 4GB.
    Xbyak::Label raise_gpf_label;

    static_assert(sizeof(CPU::SegmentCache) == 16);
    emitter.movzx(rarg1, rarg1.cvt8());
    emitter.shl(rarg1, 4);
    emitter.lea(rret, emitter.byte[rcpuptr + rarg1.cvt64() + offsetof(CPU, m_segment_cache[0])]);

    // if segcache->access_mask & Read == 0
    emitter.test(emitter.byte[rret + offsetof(CPU::SegmentCache, access_mask)],
                 static_cast<u32>(AccessTypeMask::Write));
    emitter.jz(raise_gpf_label);

    // if offset < limit_low
    emitter.cmp(rarg2, emitter.dword[rret + offsetof(CPU::SegmentCache, limit_low)]);
    emitter.jb(raise_gpf_label);

    // if offset + (size - 1) > limit_high
    // offset += segcache->base_address
    if constexpr (sizeof(DataType) > 1)
    {
      emitter.lea(scratch, emitter.qword[rarg2.cvt64() + (sizeof(DataType) - 1)]);
      emitter.add(rarg2, emitter.dword[rret + offsetof(CPU::SegmentCache, base_address)]);
      emitter.mov(rret.cvt32(), emitter.dword[rret + offsetof(CPU::SegmentCache, limit_high)]);
      emitter.cmp(scratch, rret.cvt64());
      emitter.ja(raise_gpf_label);
    }
    else
    {
      emitter.cmp(rarg2, emitter.dword[rret + offsetof(CPU::SegmentCache, limit_high)]);
      emitter.ja(raise_gpf_label);
      emitter.add(rarg2, emitter.dword[rret + offsetof(CPU::SegmentCache, base_address)]);
    }

    // swap segment with CPU
    emitter.mov(rarg1, rcpuptr);

    // go ahead with the memory read
    if constexpr (std::is_same_v<DataType, u8>)
    {
      emitter.mov(
        rret, reinterpret_cast<size_t>(static_cast<void (*)(CPU*, LinearMemoryAddress, u8)>(&CPU::WriteMemoryByte)));
    }
    else if constexpr (std::is_same_v<DataType, u16>)
    {
      emitter.mov(
        rret, reinterpret_cast<size_t>(static_cast<void (*)(CPU*, LinearMemoryAddress, u16)>(&CPU::WriteMemoryWord)));
    }
    else
    {
      emitter.mov(
        rret, reinterpret_cast<size_t>(static_cast<void (*)(CPU*, LinearMemoryAddress, u32)>(&CPU::WriteMemoryDWord)));
    }

    emitter.jmp(rret);

    // RAISE GPF BRANCH
    emitter.L(raise_gpf_label);

    // register swap since the CPU has to come first
    emitter.cmp(rarg1, (Segment_SS << 4));
    emitter.mov(rarg1, Interrupt_StackFault);
    emitter.mov(rarg2, Interrupt_GeneralProtectionFault);
    emitter.cmove(rarg2, rarg1);
    emitter.xor_(rarg3, rarg3);
    emitter.mov(rarg1, rcpuptr);

    // cpu->RaiseException(ss ? Interrupt_StackFault : Interrupt_GeneralProtectionFault, 0)
    emitter.mov(rret, reinterpret_cast<size_t>(static_cast<void (*)(CPU*, u32, u32)>(&CPU::RaiseException)));
    emitter.jmp(rret);

    emitter.ready();
    code_buffer->CommitCode(emitter.getSize());
    return ret;
  }
};

ASMFunctions ASMFunctions::Generate(JitCodeBuffer* code_buffer)
{
  ASMFunctions functions = {};
  functions.read_memory_byte = ThunkGenerator::CompileMemoryReadFunction<u8>(code_buffer);
  functions.read_memory_word = ThunkGenerator::CompileMemoryReadFunction<u16>(code_buffer);
  functions.read_memory_dword = ThunkGenerator::CompileMemoryReadFunction<u32>(code_buffer);
  functions.write_memory_byte = ThunkGenerator::CompileMemoryWriteFunction<u8>(code_buffer);
  functions.write_memory_word = ThunkGenerator::CompileMemoryWriteFunction<u16>(code_buffer);
  functions.write_memory_dword = ThunkGenerator::CompileMemoryWriteFunction<u32>(code_buffer);
  return functions;
}

} // namespace CPU_X86::Recompiler
