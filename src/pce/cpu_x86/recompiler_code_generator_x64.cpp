#include "interpreter.h"
#include "recompiler_code_generator.h"

namespace CPU_X86::Recompiler {

#if defined(ABI_WIN64)
constexpr HostReg RCPUPTR = Xbyak::Operand::RBP;
constexpr HostReg RRETURN = Xbyak::Operand::RAX;
constexpr HostReg RARG1 = Xbyak::Operand::RCX;
constexpr HostReg RARG2 = Xbyak::Operand::RDX;
constexpr HostReg RARG3 = Xbyak::Operand::R8;
constexpr HostReg RARG4 = Xbyak::Operand::R9;
const u32 FUNCTION_CALL_SHADOW_SPACE = 32;
#else
const u32 FUNCTION_CALL_SHADOW_SPACE = 0;
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
#else
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

void CodeGenerator::EmitShl(HostReg to_reg, const Value& value)
{
  DebugAssert(value.IsConstant() || value.IsInHostRegister());

  // We have to use CL for the shift amount :(
  const bool save_cl = (!value.IsConstant() && m_register_cache.IsHostRegInUse(Xbyak::Operand::RCX) &&
                        (!value.IsInHostRegister() || value.host_reg != Xbyak::Operand::RCX));
  if (save_cl)
    m_emit.push(m_emit.cl);

  if (!value.IsConstant())
    m_emit.mov(m_emit.cl, value.host_reg);

  switch (value.size)
  {
    case OperandSize_8:
    {
      if (value.IsConstant())
        m_emit.shl(GetHostReg8(to_reg), Truncate8(value.constant_value));
      else
        m_emit.shl(GetHostReg8(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant())
        m_emit.shl(GetHostReg16(to_reg), Truncate8(value.constant_value));
      else
        m_emit.shl(GetHostReg16(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant())
        m_emit.shl(GetHostReg32(to_reg), Truncate32(value.constant_value));
      else
        m_emit.shl(GetHostReg32(to_reg), m_emit.cl);
    }
    break;

    case OperandSize_64:
    {
      if (value.IsConstant())
        m_emit.shl(GetHostReg64(to_reg), Truncate32(value.constant_value));
      else
        m_emit.shl(GetHostReg64(to_reg), m_emit.cl);
    }
    break;
  }

  if (save_cl)
    m_emit.pop(m_emit.cl);
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

void CodeGenerator::EmitFunctionCall(Value* return_value, const void* ptr, const Value& arg1)
{
  if (return_value)
    return_value->Discard();

  // shadow space allocate
  const u32 adjust_size = PrepareStackForCall();

  // push arguments
  EmitCopyValue(RARG1, arg1);

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

void CodeGenerator::EmitFunctionCall(Value* return_value, const void* ptr, const Value& arg1, const Value& arg2)
{
  if (return_value)
    return_value->Discard();

  // shadow space allocate
  const u32 adjust_size = PrepareStackForCall();

  // push arguments
  EmitCopyValue(RARG1, arg1);
  EmitCopyValue(RARG2, arg2);

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

void CodeGenerator::EmitFunctionCall(Value* return_value, const void* ptr, const Value& arg1, const Value& arg2,
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

void CodeGenerator::EmitFunctionCall(Value* return_value, const void* ptr, const Value& arg1, const Value& arg2,
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

void CodeGenerator::EmitPushHostReg(HostReg reg)
{
  m_emit.push(GetHostReg64(reg));
}

void CodeGenerator::EmitPopHostReg(HostReg reg)
{
  m_emit.pop(GetHostReg64(reg));
}

Value CodeGenerator::ReadFlagsFromHost()
{
  // this is a 64-bit push/pop, we ignore the upper 32 bits
  Value temp = m_register_cache.AllocateScratch(OperandSize_32);
  DebugAssert(temp.IsInHostRegister());
  m_emit.pushf();
  m_emit.pop(GetHostReg64(temp.host_reg));
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
      break;
  }

  Value host_flags = ReadFlagsFromHost();

  if (instruction.operation != Operation_CMP)
    WriteOperand(instruction, 0, std::move(lhs));

  const u32 eflags_mask = Flag_OF | Flag_CF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF;
  UpdateEFLAGS(std::move(host_flags), 0, eflags_mask, 0);
  return true;
}

} // namespace CPU_X86::Recompiler