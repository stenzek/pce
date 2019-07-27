#include "interpreter.h"
#include "recompiler_code_generator.h"

namespace CPU_X86::Recompiler {

#if defined(ABI_WIN64)
constexpr HostReg RCPUPTR = Xbyak::Operand::RBP;
constexpr HostReg RRETURN = Xbyak::Operand::RAX;
constexpr HostReg RARG1 = Xbyak::Operand::RCX;
const u32 FUNCTION_CALL_SHADOW_SPACE = 16;
#else
const u32 FUNCTION_CALL_SHADOW_SPACE = 0;
#endif

static const Xbyak::Reg64 GetCPUPtrReg()
{
  return Xbyak::Reg64(RCPUPTR);
}

void CodeGenerator::InitHostRegs()
{
#if defined(ABI_WIN64)
  // allocate nonvolatile before volatile
  m_register_cache.SetHostRegAllocationOrder(
    {Xbyak::Operand::RBX, /*Xbyak::Operand::RBP, */ Xbyak::Operand::RDI, Xbyak::Operand::RSI, Xbyak::Operand::RSP,
     Xbyak::Operand::R12, Xbyak::Operand::R13, Xbyak::Operand::R14, Xbyak::Operand::R15, Xbyak::Operand::RCX,
     Xbyak::Operand::RDX, Xbyak::Operand::R8, Xbyak::Operand::R9, Xbyak::Operand::R10, Xbyak::Operand::R11,
     /*Xbyak::Operand::RAX*/});
  m_register_cache.SetCallerSavedHostRegs({/*Xbyak::Operand::RAX,*/ Xbyak::Operand::RCX, Xbyak::Operand::RDX,
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
#if defined(ABI_WIN64)
  m_emit.push(GetCPUPtrReg());
  m_emit.mov(GetCPUPtrReg(), Xbyak::Reg64(RARG1));
#else

#endif

  // Copy {EIP,ESP} to m_current_{EIP,ESP}
  SyncCurrentEIP();
  SyncCurrentESP();
}

void CodeGenerator::EmitEndBlock()
{
  m_register_cache.PopCalleeSavedRegisters();

#if defined(ABI_WIN64)
  m_emit.pop(GetCPUPtrReg());
#endif

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
      const Xbyak::Reg16 dest(to_reg);
      switch (from_size)
      {
        case OperandSize_8:
          m_emit.movsx(dest, Xbyak::Reg8(from_reg));
          return;
      }
    }
    break;

    case OperandSize_32:
    {
      const Xbyak::Reg32 dest(to_reg);
      switch (from_size)
      {
        case OperandSize_8:
          m_emit.movsx(dest, Xbyak::Reg8(from_reg));
          return;
        case OperandSize_16:
          m_emit.movsx(dest, Xbyak::Reg16(from_reg));
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
      const Xbyak::Reg16 dest(to_reg);
      switch (from_size)
      {
        case OperandSize_8:
          m_emit.movzx(dest, Xbyak::Reg8(from_reg));
          return;
      }
    }
    break;

    case OperandSize_32:
    {
      const Xbyak::Reg32 dest(to_reg);
      switch (from_size)
      {
        case OperandSize_8:
          m_emit.movzx(dest, Xbyak::Reg8(from_reg));
          return;
        case OperandSize_16:
          m_emit.movzx(dest, Xbyak::Reg16(from_reg));
          return;
      }
    }
    break;
  }

  Panic("Unknown sign-extend combination");
}

void CodeGenerator::EmitCopyValue(HostReg to_reg, const Value& value)
{
  DebugAssert(value.IsConstant() || value.IsInHostRegister());

  switch (value.size)
  {
    case OperandSize_8:
    {
      const Xbyak::Reg8 dest(to_reg);
      if (value.IsConstant())
        m_emit.mov(dest, value.constant_value);
      else
        m_emit.mov(dest, Xbyak::Reg8(value.host_reg));
    }
    break;

    case OperandSize_16:
    {
      const Xbyak::Reg16 dest(to_reg);
      if (value.IsConstant())
        m_emit.mov(dest, value.constant_value);
      else
        m_emit.mov(dest, Xbyak::Reg16(value.host_reg));
    }
    break;

    case OperandSize_32:
    {
      const Xbyak::Reg32 dest(to_reg);
      if (value.IsConstant())
        m_emit.mov(dest, value.constant_value);
      else
        m_emit.mov(dest, Xbyak::Reg32(value.host_reg));
    }
    break;

    case OperandSize_64:
    {
      const Xbyak::Reg64 dest(to_reg);
      if (value.IsConstant())
        m_emit.mov(dest, value.constant_value);
      else
        m_emit.mov(dest, Xbyak::Reg64(value.host_reg));
    }
    break;
  }
}

void CodeGenerator::PrepareStackForCall(u32 num_parameters)
{
  // we assume that the stack is unaligned at this point
  const u32 num_callee_saved = m_register_cache.GetActiveCalleeSavedRegisterCount();
  const u32 num_caller_saved = m_register_cache.PushCallerSavedRegisters();
  const u32 current_offset = 8 + (num_callee_saved + num_caller_saved + num_parameters) * 8;
  const u32 aligned_offset = Common::AlignUp(current_offset + FUNCTION_CALL_SHADOW_SPACE, 16);
  const u32 adjust_size = aligned_offset - current_offset;
  if (adjust_size > 0)
    m_emit.sub(m_emit.rsp, adjust_size);
}

void CodeGenerator::RestoreStackAfterCall(u32 num_parameters)
{
  const u32 num_callee_saved = m_register_cache.GetActiveCalleeSavedRegisterCount();
  const u32 num_caller_saved = m_register_cache.PushCallerSavedRegisters();
  const u32 current_offset = 8 + (num_callee_saved + num_caller_saved + num_parameters) * 8;
  const u32 aligned_offset = Common::AlignUp(current_offset + FUNCTION_CALL_SHADOW_SPACE, 16);
  const u32 adjust_size = aligned_offset - current_offset;
  if (adjust_size > 0)
    m_emit.add(m_emit.rsp, adjust_size);
}

void CodeGenerator::EmitFunctionCall(const void* ptr, const Value& arg1)
{
  // we need a temporary for the function pointer
  const Value function_addr = m_register_cache.AllocateScratch(OperandSize_64);
  const Xbyak::Reg64 function_addr_reg(function_addr.host_reg);
  m_emit.mov(function_addr_reg, reinterpret_cast<size_t>(ptr));

  // shadow space allocate
  PrepareStackForCall(1);

  // push arguments
  EmitCopyValue(RARG1, arg1);

  // actually call the function
  m_emit.call(function_addr_reg);

  // shadow space release
  RestoreStackAfterCall(1);

  m_register_cache.PopCallerSavedRegisters();
}

void CodeGenerator::EmitPushHostReg(HostReg reg)
{
  m_emit.push(Xbyak::Reg64(reg));
}

void CodeGenerator::EmitPopHostReg(HostReg reg)
{
  m_emit.pop(Xbyak::Reg64(reg));
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
    {
      const Xbyak::Reg8 dest_host_reg(host_reg);
      m_emit.mov(dest_host_reg, m_emit.byte[GetCPUPtrReg() + offset]);
    }
    break;

    case OperandSize_16:
    {
      const Xbyak::Reg16 dest_host_reg(host_reg);
      m_emit.mov(dest_host_reg, m_emit.word[GetCPUPtrReg() + offset]);
    }
    break;

    case OperandSize_32:
    {
      const Xbyak::Reg32 dest_host_reg(host_reg);
      m_emit.mov(dest_host_reg, m_emit.dword[GetCPUPtrReg() + offset]);
    }
    break;

    case OperandSize_64:
    {
      const Xbyak::Reg64 dest_host_reg(host_reg);
      m_emit.mov(dest_host_reg, m_emit.qword[GetCPUPtrReg() + offset]);
    }
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
        m_emit.mov(m_emit.byte[GetCPUPtrReg() + offset], Xbyak::Reg8(value.host_reg));
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant())
        m_emit.mov(m_emit.word[GetCPUPtrReg() + offset], value.constant_value);
      else
        m_emit.mov(m_emit.word[GetCPUPtrReg() + offset], Xbyak::Reg16(value.host_reg));
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant())
        m_emit.mov(m_emit.dword[GetCPUPtrReg() + offset], value.constant_value);
      else
        m_emit.mov(m_emit.dword[GetCPUPtrReg() + offset], Xbyak::Reg32(value.host_reg));
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
          m_emit.mov(m_emit.qword[GetCPUPtrReg() + offset], Xbyak::Reg64(temp.host_reg));
        }
        else
        {
          m_emit.mov(m_emit.qword[GetCPUPtrReg() + offset], value.constant_value);
        }
      }
      else
      {
        m_emit.mov(m_emit.qword[GetCPUPtrReg() + offset], Xbyak::Reg64(value.host_reg));
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
      if (value.IsConstant())
        m_emit.add(m_emit.byte[GetCPUPtrReg() + offset], Truncate32(value.constant_value));
      else
        m_emit.add(m_emit.byte[GetCPUPtrReg() + offset], Xbyak::Reg8(value.host_reg));
    }
    break;

    case OperandSize_16:
    {
      if (value.IsConstant())
        m_emit.add(m_emit.word[GetCPUPtrReg() + offset], Truncate32(value.constant_value));
      else
        m_emit.add(m_emit.word[GetCPUPtrReg() + offset], Xbyak::Reg16(value.host_reg));
    }
    break;

    case OperandSize_32:
    {
      if (value.IsConstant())
        m_emit.add(m_emit.dword[GetCPUPtrReg() + offset], Truncate32(value.constant_value));
      else
        m_emit.add(m_emit.dword[GetCPUPtrReg() + offset], Xbyak::Reg32(value.host_reg));
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
          m_emit.add(m_emit.qword[GetCPUPtrReg() + offset], Xbyak::Reg64(temp.host_reg));
        }
        else
        {
          m_emit.add(m_emit.qword[GetCPUPtrReg() + offset], Truncate32(value.constant_value));
        }
      }
      else
      {
        m_emit.add(m_emit.qword[GetCPUPtrReg() + offset], Xbyak::Reg64(value.host_reg));
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

void CodeGenerator::EmitLoadGuestMemory(HostReg dest_reg, OperandSize size, const Value& address, Segment segment) {}

void CodeGenerator::EmitStoreGuestMemory(const Value& value, const Value& address, Segment segment) {}



} // namespace CPU_X86::Recompiler