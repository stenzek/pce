#include "recompiler_register_cache.h"
#include "YBaseLib/Log.h"
#include "recompiler_code_generator.h"
Log_SetChannel(CPU_X86::Recompiler);

namespace CPU_X86::Recompiler {

Value::Value() = default;

Value::Value(RegisterCache* regcache_, u64 constant_, OperandSize size_, ValueFlags flags_)
  : regcache(regcache_), constant_value(constant_), size(size_), flags(flags_)
{
}

Value::Value(const Value& other)
  : regcache(other.regcache), constant_value(other.constant_value), size(other.size), flags(other.flags)
{
  AssertMsg(!other.IsScratch(), "Can't copy a temporary register");
}

Value::Value(Value&& other)
  : regcache(other.regcache), constant_value(other.constant_value), size(other.size), flags(other.flags)
{
  other.regcache = nullptr;
  other.constant_value = 0;
  other.size = OperandSize_8;
  other.flags = ValueFlags::None;
}

Value::Value(RegisterCache* regcache_, HostReg reg_, OperandSize size_, ValueFlags flags_)
  : regcache(regcache_), host_reg(reg_), size(size_), flags(flags_)
{
}

Value::~Value()
{
  Release();
}

Value& Value::operator=(const Value& other)
{
  AssertMsg(!other.IsScratch(), "Can't copy a temporary register");

  Release();
  regcache = other.regcache;
  constant_value = other.constant_value;
  size = other.size;
  flags = other.flags;

  return *this;
}

Value& Value::operator=(Value&& other)
{
  Release();
  regcache = other.regcache;
  constant_value = other.constant_value;
  size = other.size;
  flags = other.flags;
  other.regcache = nullptr;
  other.constant_value = 0;
  other.size = OperandSize_8;
  other.flags = ValueFlags::None;
  return *this;
}

void Value::Clear()
{
  Release();
  regcache = nullptr;
  constant_value = 0;
  size = OperandSize_8;
  flags = ValueFlags::None;
}

void Value::Release()
{
  if (IsScratch())
  {
    DebugAssert(IsInHostRegister() && regcache);
    regcache->FreeHostReg(host_reg);
  }
}

RegisterCache::RegisterCache(CodeGenerator& code_generator) : m_code_generator(code_generator) {}

RegisterCache::~RegisterCache() = default;

void RegisterCache::SetHostRegAllocationOrder(std::initializer_list<HostReg> regs)
{
  size_t index = 0;
  for (HostReg reg : regs)
  {
    m_host_register_state[reg] = HostRegState::Usable;
    m_host_register_allocation_order[index++] = reg;
  }
  m_host_register_available_count = static_cast<u32>(index);
}

void RegisterCache::SetCallerSavedHostRegs(std::initializer_list<HostReg> regs)
{
  for (HostReg reg : regs)
    m_host_register_state[reg] |= HostRegState::CallerSaved;
}

void RegisterCache::SetCalleeSavedHostRegs(std::initializer_list<HostReg> regs)
{
  for (HostReg reg : regs)
    m_host_register_state[reg] |= HostRegState::CalleeSaved;
}

void RegisterCache::SetCPUPtrHostReg(HostReg reg)
{
  m_cpu_ptr_host_register = reg;
}

bool RegisterCache::IsUsableHostRegister(HostReg reg) const
{
  return (m_host_register_state[reg] & HostRegState::Usable) != HostRegState::None;
}

HostReg RegisterCache::AllocateHostReg(HostRegState state /* = HostRegState::InUse */)
{
  // try for a free register in allocation order
  for (u32 i = 0; i < m_host_register_available_count; i++)
  {
    const HostReg reg = m_host_register_allocation_order[i];
    if ((m_host_register_state[reg] & (HostRegState::Usable | HostRegState::InUse)) == HostRegState::Usable)
    {
      m_host_register_state[reg] |= state;

      if ((m_host_register_state[reg] & (HostRegState::CalleeSaved | HostRegState::CalleeSavedAllocated)) ==
          HostRegState::CalleeSaved)
      {
        // new register we need to save..
        m_host_register_state[reg] |= HostRegState::CalleeSavedAllocated;
        m_active_callee_saved_register_count++;
        m_code_generator.EmitPushHostReg(reg);
      }

      return reg;
    }
  }

  // evict one of the cached guest registers
  if (!EvictOneGuestRegister())
    Panic("Failed to evict guest register for new allocation");

  return AllocateHostReg(state);
}

Value RegisterCache::GetCPUPtr()
{
  return Value::FromHostReg(this, m_cpu_ptr_host_register, HostPointerSize);
}

Value RegisterCache::AllocateScratch(OperandSize size)
{
  HostReg reg = AllocateHostReg();
  return Value::FromScratch(this, reg, size);
}

void RegisterCache::FreeHostReg(HostReg reg)
{
  m_host_register_state[reg] &= ~HostRegState::InUse;
}

u32 RegisterCache::PushCallerSavedRegisters() const
{
  u32 count = 0;
  for (u32 i = 0; i < HostReg_Count; i++)
  {
    if ((m_host_register_state[i] & (HostRegState::CallerSaved | HostRegState::InUse)) ==
        (HostRegState::CallerSaved | HostRegState::InUse))
    {
      m_code_generator.EmitPushHostReg(static_cast<HostReg>(i));
      count++;
    }
  }

  return count;
}

u32 RegisterCache::PopCallerSavedRegisters() const
{
  u32 count = 0;
  u32 i = (HostReg_Count - 1);
  do
  {
    if ((m_host_register_state[i] & (HostRegState::CallerSaved | HostRegState::InUse)) ==
        (HostRegState::CallerSaved | HostRegState::InUse))
    {
      m_code_generator.EmitPopHostReg(static_cast<HostReg>(i));
      count++;
    }
    i--;
  } while (i > 0);
  return count;
}

u32 RegisterCache::PopCalleeSavedRegisters()
{
  u32 count = 0;
  u32 i = (HostReg_Count - 1);
  do
  {
    if ((m_host_register_state[i] & (HostRegState::CalleeSaved | HostRegState::CalleeSavedAllocated)) ==
        (HostRegState::CalleeSaved | HostRegState::CalleeSavedAllocated))
    {
      m_code_generator.EmitPopHostReg(static_cast<HostReg>(i));
      m_host_register_state[i] &= ~HostRegState::CalleeSavedAllocated;
      count++;
    }
    i--;
  } while (i > 0);
  return count;
}

void RegisterCache::FlushOverlappingGuestRegisters(Reg8 guest_reg)
{
  switch (guest_reg)
  {
    case Reg8_AL:
    case Reg8_AH:
      FlushGuestRegister(Reg16_AX, true);
      FlushGuestRegister(Reg32_EAX, true);
      break;
    case Reg8_CL:
    case Reg8_CH:
      FlushGuestRegister(Reg16_CX, true);
      FlushGuestRegister(Reg32_ECX, true);
      break;
    case Reg8_BL:
    case Reg8_BH:
      FlushGuestRegister(Reg16_BX, true);
      FlushGuestRegister(Reg32_EBX, true);
      break;
    case Reg8_DL:
    case Reg8_DH:
      FlushGuestRegister(Reg16_DX, true);
      FlushGuestRegister(Reg32_EDX, true);
      break;
    default:
      break;
  }
}

void RegisterCache::FlushOverlappingGuestRegisters(Reg16 guest_reg)
{
  switch (guest_reg)
  {
    case Reg16_AX:
      FlushGuestRegister(Reg8_AL, true);
      FlushGuestRegister(Reg8_AH, true);
      FlushGuestRegister(Reg32_EAX, true);
      break;
    case Reg16_BX:
      FlushGuestRegister(Reg8_BL, true);
      FlushGuestRegister(Reg8_BH, true);
      FlushGuestRegister(Reg32_EBX, true);
      break;
    case Reg16_CX:
      FlushGuestRegister(Reg8_CL, true);
      FlushGuestRegister(Reg8_CH, true);
      FlushGuestRegister(Reg16_CX, true);
      FlushGuestRegister(Reg32_ECX, true);
      break;
    case Reg16_DX:
      FlushGuestRegister(Reg8_DL, true);
      FlushGuestRegister(Reg8_DH, true);
      FlushGuestRegister(Reg16_DX, true);
      FlushGuestRegister(Reg32_EDX, true);
      break;
    case Reg16_SP:
      FlushGuestRegister(Reg32_ESP, true);
      break;
    case Reg16_BP:
      FlushGuestRegister(Reg32_EBP, true);
      break;
    case Reg16_SI:
      FlushGuestRegister(Reg32_ESI, true);
      break;
    case Reg16_DI:
      FlushGuestRegister(Reg32_EDI, true);
      break;
    default:
      break;
  }
}

void RegisterCache::FlushOverlappingGuestRegisters(Reg32 guest_reg)
{
  switch (guest_reg)
  {
    case Reg32_EAX:
      FlushGuestRegister(Reg8_AL, true);
      FlushGuestRegister(Reg8_AH, true);
      FlushGuestRegister(Reg16_AX, true);
      break;
    case Reg32_EBX:
      FlushGuestRegister(Reg8_BL, true);
      FlushGuestRegister(Reg8_BH, true);
      FlushGuestRegister(Reg16_BX, true);
      break;
    case Reg32_ECX:
      FlushGuestRegister(Reg8_CL, true);
      FlushGuestRegister(Reg8_CH, true);
      FlushGuestRegister(Reg16_CX, true);
      FlushGuestRegister(Reg32_ECX, true);
      break;
    case Reg32_EDX:
      FlushGuestRegister(Reg8_DL, true);
      FlushGuestRegister(Reg8_DH, true);
      FlushGuestRegister(Reg16_DX, true);
      FlushGuestRegister(Reg32_EDX, true);
      break;
    case Reg32_ESP:
      FlushGuestRegister(Reg32_ESP, true);
      break;
    case Reg32_EBP:
      FlushGuestRegister(Reg32_EBP, true);
      break;
    case Reg32_ESI:
      FlushGuestRegister(Reg32_ESI, true);
      break;
    case Reg32_EDI:
      FlushGuestRegister(Reg32_EDI, true);
      break;
    default:
      break;
  }
}

Value RegisterCache::ReadGuestRegister(Reg8 guest_reg, bool cache /* = true */)
{
  if (m_guest_reg8_state[guest_reg].IsCached())
    return m_guest_reg8_state[guest_reg].ToValue(this, OperandSize_8);

  FlushOverlappingGuestRegisters(guest_reg);

  const HostReg host_reg = AllocateHostReg();
  m_code_generator.EmitLoadGuestRegister(host_reg, OperandSize_8, guest_reg);

  // Now in cache.
  if (cache)
  {
    m_guest_reg8_state[guest_reg].SetHostReg(host_reg);
    return Value::FromHostReg(this, host_reg, OperandSize_8);
  }
  else
  {
    return Value::FromScratch(this, host_reg, OperandSize_8);
  }
}

Value RegisterCache::ReadGuestRegister(Reg16 guest_reg, bool cache /* = true */)
{
  if (m_guest_reg16_state[guest_reg].IsCached())
    return m_guest_reg16_state[guest_reg].ToValue(this, OperandSize_16);

  FlushOverlappingGuestRegisters(guest_reg);

  const HostReg host_reg = AllocateHostReg();
  m_code_generator.EmitLoadGuestRegister(host_reg, OperandSize_16, guest_reg);

  // Now in cache.
  if (cache)
  {
    m_guest_reg16_state[guest_reg].SetHostReg(host_reg);
    return Value::FromHostReg(this, host_reg, OperandSize_16);
  }
  else
  {
    return Value::FromScratch(this, host_reg, OperandSize_16);
  }
}

Value RegisterCache::ReadGuestRegister(Reg32 guest_reg, bool cache /* = true */)
{
  if (m_guest_reg32_state[guest_reg].IsCached())
    return m_guest_reg32_state[guest_reg].ToValue(this, OperandSize_32);

  FlushOverlappingGuestRegisters(guest_reg);

  const HostReg host_reg = AllocateHostReg();
  m_code_generator.EmitLoadGuestRegister(host_reg, OperandSize_32, guest_reg);

  // Now in cache.
  if (cache)
  {
    m_guest_reg32_state[guest_reg].SetHostReg(host_reg);
    return Value::FromHostReg(this, host_reg, OperandSize_32);
  }
  else
  {
    return Value::FromScratch(this, host_reg, OperandSize_32);
  }
}

void RegisterCache::WriteGuestRegister(Reg8 guest_reg, Value&& value)
{
  FlushOverlappingGuestRegisters(guest_reg);
  InvalidateGuestRegister(guest_reg);

  GuestRegData& guest_reg_state = m_guest_reg8_state[guest_reg];
  if (value.IsConstant())
  {
    // No need to allocate a host register, and we can defer the store.
    guest_reg_state.SetConstant(Truncate32(value.constant_value));
    guest_reg_state.SetDirty();
    return;
  }

  // If it's a temporary, we can bind that to the guest register.
  if (value.IsScratch())
  {
    guest_reg_state.SetHostReg(value.host_reg);
    guest_reg_state.SetDirty();
    value.Clear();
    return;
  }

  // Allocate host register, and copy value to it.
  HostReg host_reg = AllocateHostReg();
  m_code_generator.EmitCopyValue(host_reg, value);
  guest_reg_state.SetHostReg(host_reg);
  guest_reg_state.SetDirty();
}

void RegisterCache::WriteGuestRegister(Reg16 guest_reg, Value&& value)
{
  FlushOverlappingGuestRegisters(guest_reg);
  InvalidateGuestRegister(guest_reg);

  GuestRegData& guest_reg_state = m_guest_reg16_state[guest_reg];
  if (value.IsConstant())
  {
    // No need to allocate a host register, and we can defer the store.
    guest_reg_state.SetConstant(Truncate32(value.constant_value));
    guest_reg_state.SetDirty();
    return;
  }

  // If it's a temporary, we can bind that to the guest register.
  if (value.IsScratch())
  {
    guest_reg_state.SetHostReg(value.host_reg);
    guest_reg_state.SetDirty();
    value.Clear();
    return;
  }

  // Allocate host register, and copy value to it.
  HostReg host_reg = AllocateHostReg();
  m_code_generator.EmitCopyValue(host_reg, value);
  guest_reg_state.SetHostReg(host_reg);
  guest_reg_state.SetDirty();
}

void RegisterCache::WriteGuestRegister(Reg32 guest_reg, Value&& value)
{
  FlushOverlappingGuestRegisters(guest_reg);
  InvalidateGuestRegister(guest_reg);

  GuestRegData& guest_reg_state = m_guest_reg32_state[guest_reg];
  if (value.IsConstant())
  {
    // No need to allocate a host register, and we can defer the store.
    guest_reg_state.SetConstant(Truncate32(value.constant_value));
    guest_reg_state.SetDirty();
    return;
  }

  // If it's a temporary, we can bind that to the guest register.
  if (value.IsScratch())
  {
    guest_reg_state.SetHostReg(value.host_reg);
    guest_reg_state.SetDirty();
    value.Clear();
    return;
  }

  // Allocate host register, and copy value to it.
  HostReg host_reg = AllocateHostReg();
  m_code_generator.EmitCopyValue(host_reg, value);
  guest_reg_state.SetHostReg(host_reg);
  guest_reg_state.SetDirty();
}

void RegisterCache::FlushGuestRegister(Reg8 guest_reg, bool invalidate)
{
  GuestRegData& guest_reg_state = m_guest_reg8_state[guest_reg];
  if (guest_reg_state.IsDirty())
    m_code_generator.EmitStoreGuestRegister(OperandSize_8, guest_reg, guest_reg_state.ToValue(this, OperandSize_8));

  if (invalidate)
    InvalidateGuestRegister(guest_reg);
}

void RegisterCache::InvalidateGuestRegister(Reg8 guest_reg)
{
  GuestRegData& guest_reg_state = m_guest_reg8_state[guest_reg];
  if (guest_reg_state.IsInHostRegister())
    FreeHostReg(guest_reg_state.host_reg);

  guest_reg_state.Invalidate();
}

void RegisterCache::FlushGuestRegister(Reg16 guest_reg, bool invalidate)
{
  GuestRegData& guest_reg_state = m_guest_reg16_state[guest_reg];
  if (guest_reg_state.IsDirty())
    m_code_generator.EmitStoreGuestRegister(OperandSize_16, guest_reg, guest_reg_state.ToValue(this, OperandSize_16));

  if (invalidate)
    InvalidateGuestRegister(guest_reg);
}

void RegisterCache::InvalidateGuestRegister(Reg16 guest_reg)
{
  GuestRegData& guest_reg_state = m_guest_reg16_state[guest_reg];
  if (guest_reg_state.IsInHostRegister())
    FreeHostReg(guest_reg_state.host_reg);

  guest_reg_state.Invalidate();
}

void RegisterCache::FlushGuestRegister(Reg32 guest_reg, bool invalidate)
{
  GuestRegData& guest_reg_state = m_guest_reg32_state[guest_reg];
  if (guest_reg_state.IsDirty())
    m_code_generator.EmitStoreGuestRegister(OperandSize_32, guest_reg, guest_reg_state.ToValue(this, OperandSize_32));

  if (invalidate)
    InvalidateGuestRegister(guest_reg);
}

void RegisterCache::InvalidateGuestRegister(Reg32 guest_reg)
{
  GuestRegData& guest_reg_state = m_guest_reg32_state[guest_reg];
  if (guest_reg_state.IsInHostRegister())
    FreeHostReg(guest_reg_state.host_reg);

  guest_reg_state.Invalidate();
}

void RegisterCache::FlushAllGuestRegisters(bool invalidate)
{
  for (u8 reg = 0; reg < Reg8_Count; reg++)
    FlushGuestRegister(static_cast<Reg8>(reg), invalidate);
  for (u8 reg = 0; reg < Reg16_Count; reg++)
    FlushGuestRegister(static_cast<Reg16>(reg), invalidate);
  for (u8 reg = 0; reg < Reg32_Count; reg++)
    FlushGuestRegister(static_cast<Reg32>(reg), invalidate);
}

bool RegisterCache::EvictOneGuestRegister()
{
  return false;
}


} // namespace CPU_X86::Recompiler
