#include "recompiler_register_cache.h"
#include "YBaseLib/Log.h"
#include "recompiler_code_generator.h"
#include <cinttypes>
Log_SetChannel(CPU_X86::Recompiler);

namespace CPU_X86::Recompiler {

Value::Value() = default;

Value::Value(RegisterCache* regcache_, u64 constant_, OperandSize size_, ValueFlags flags_)
  : regcache(regcache_), constant_value(constant_), size(size_), flags(flags_)
{
}

Value::Value(const Value& other)
  : regcache(other.regcache), constant_value(other.constant_value), host_reg(other.host_reg), size(other.size),
    flags(other.flags)
{
  AssertMsg(!other.IsScratch(), "Can't copy a temporary register");
}

Value::Value(Value&& other)
  : regcache(other.regcache), constant_value(other.constant_value), host_reg(other.host_reg), size(other.size),
    flags(other.flags)
{
  other.Clear();
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
  host_reg = other.host_reg;
  size = other.size;
  flags = other.flags;

  return *this;
}

Value& Value::operator=(Value&& other)
{
  Release();
  regcache = other.regcache;
  constant_value = other.constant_value;
  host_reg = other.host_reg;
  size = other.size;
  flags = other.flags;
  other.Clear();
  return *this;
}

void Value::Clear()
{
  regcache = nullptr;
  constant_value = 0;
  host_reg = {};
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

void Value::ReleaseAndClear()
{
  Release();
  Clear();
}

void Value::Discard()
{
  DebugAssert(IsInHostRegister());
  regcache->DiscardHostReg(host_reg);
}

void Value::Undiscard()
{
  DebugAssert(IsInHostRegister());
  regcache->UndiscardHostReg(host_reg);
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

bool RegisterCache::IsUsableHostReg(HostReg reg) const
{
  return (m_host_register_state[reg] & HostRegState::Usable) != HostRegState::None;
}

bool RegisterCache::IsHostRegInUse(HostReg reg) const
{
  return (m_host_register_state[reg] & HostRegState::InUse) != HostRegState::None;
}

bool RegisterCache::HasFreeHostRegister() const
{
  for (const HostRegState state : m_host_register_state)
  {
    if ((state & (HostRegState::Usable | HostRegState::InUse)) == (HostRegState::Usable))
      return true;
  }

  return false;
}

u32 RegisterCache::GetUsedHostRegisters() const
{
  u32 count = 0;
  for (const HostRegState state : m_host_register_state)
  {
    if ((state & (HostRegState::Usable | HostRegState::InUse)) == (HostRegState::Usable | HostRegState::InUse))
      count++;
  }

  return count;
}

u32 RegisterCache::GetFreeHostRegisters() const
{
  u32 count = 0;
  for (const HostRegState state : m_host_register_state)
  {
    if ((state & (HostRegState::Usable | HostRegState::InUse)) == (HostRegState::Usable))
      count++;
  }

  return count;
}

HostReg RegisterCache::AllocateHostReg(HostRegState state /* = HostRegState::InUse */)
{
  // try for a free register in allocation order
  for (u32 i = 0; i < m_host_register_available_count; i++)
  {
    const HostReg reg = m_host_register_allocation_order[i];
    if ((m_host_register_state[reg] & (HostRegState::Usable | HostRegState::InUse)) == HostRegState::Usable)
    {
      if (AllocateHostReg(reg, state))
        return reg;
    }
  }

  // evict one of the cached guest registers
  if (!EvictOneGuestRegister())
    Panic("Failed to evict guest register for new allocation");

  return AllocateHostReg(state);
}

bool RegisterCache::AllocateHostReg(HostReg reg, HostRegState state /*= HostRegState::InUse*/)
{
  if ((m_host_register_state[reg] & (HostRegState::Usable | HostRegState::InUse)) != HostRegState::Usable)
    return false;

  m_host_register_state[reg] |= state;

  if ((m_host_register_state[reg] & (HostRegState::CalleeSaved | HostRegState::CalleeSavedAllocated)) ==
      HostRegState::CalleeSaved)
  {
    // new register we need to save..
    DebugAssert(m_host_register_callee_saved_order_count < HostReg_Count);
    m_host_register_callee_saved_order[m_host_register_callee_saved_order_count++] = reg;
    m_host_register_state[reg] |= HostRegState::CalleeSavedAllocated;
    m_code_generator.EmitPushHostReg(reg);
  }

  return reg;
}

void RegisterCache::DiscardHostReg(HostReg reg)
{
  DebugAssert(IsHostRegInUse(reg));
  Log_DebugPrintf("Discarding host register %s", m_code_generator.GetHostRegName(reg));
  m_host_register_state[reg] |= HostRegState::Discarded;
}

void RegisterCache::UndiscardHostReg(HostReg reg)
{
  DebugAssert(IsHostRegInUse(reg));
  Log_DebugPrintf("Undiscarding host register %s", m_code_generator.GetHostRegName(reg));
  m_host_register_state[reg] &= ~HostRegState::Discarded;
}

void RegisterCache::FreeHostReg(HostReg reg)
{
  DebugAssert(IsHostRegInUse(reg));
  Log_DebugPrintf("Freeing host register %s", m_code_generator.GetHostRegName(reg));
  m_host_register_state[reg] &= ~HostRegState::InUse;
}

Value RegisterCache::GetCPUPtr()
{
  return Value::FromHostReg(this, m_cpu_ptr_host_register, HostPointerSize);
}

Value RegisterCache::AllocateScratch(OperandSize size)
{
  const HostReg reg = AllocateHostReg();
  Log_DebugPrintf("Allocating host register %s as scratch", m_code_generator.GetHostRegName(reg));
  return Value::FromScratch(this, reg, size);
}

u32 RegisterCache::PushCallerSavedRegisters() const
{
  u32 count = 0;
  for (u32 i = 0; i < HostReg_Count; i++)
  {
    if ((m_host_register_state[i] & (HostRegState::CallerSaved | HostRegState::InUse | HostRegState::Discarded)) ==
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
    if ((m_host_register_state[i] & (HostRegState::CallerSaved | HostRegState::InUse | HostRegState::Discarded)) ==
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
  if (m_host_register_callee_saved_order_count == 0)
    return 0;

  u32 count = 0;
  u32 i = m_host_register_callee_saved_order_count;
  do
  {
    const HostReg reg = m_host_register_callee_saved_order[i - 1];
    DebugAssert((m_host_register_state[reg] & (HostRegState::CalleeSaved | HostRegState::CalleeSavedAllocated)) ==
                (HostRegState::CalleeSaved | HostRegState::CalleeSavedAllocated));

    m_code_generator.EmitPopHostReg(reg);
    m_host_register_state[reg] &= ~HostRegState::CalleeSavedAllocated;
    count++;
    i--;
  } while (i > 0);
  return count;
}

void RegisterCache::FlushOverlappingGuestRegisters(OperandSize size, u8 guest_reg)
{
  switch (size)
  {
    case OperandSize_8:
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
    break;

    case OperandSize_16:
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
    break;

    case OperandSize_32:
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
          FlushGuestRegister(Reg16_SP, true);
          break;
        case Reg32_EBP:
          FlushGuestRegister(Reg16_BP, true);
          break;
        case Reg32_ESI:
          FlushGuestRegister(Reg16_SI, true);
          break;
        case Reg32_EDI:
          FlushGuestRegister(Reg16_DI, true);
          break;
        default:
          break;
      }
    }
    break;

    default:
      break;
  }
}

Value RegisterCache::ReadGuestRegister(Reg8 guest_reg, bool cache /* = true */, bool force_host_register /* = false */)
{
  return ReadGuestRegister(m_guest_reg8_cache[guest_reg], OperandSize_8, static_cast<u8>(guest_reg), cache,
                           force_host_register);
}

Value RegisterCache::ReadGuestRegister(Reg16 guest_reg, bool cache /* = true */, bool force_host_register /* = false */)
{
  return ReadGuestRegister(m_guest_reg16_cache[guest_reg], OperandSize_16, static_cast<u8>(guest_reg), cache,
                           force_host_register);
}

Value RegisterCache::ReadGuestRegister(Reg32 guest_reg, bool cache /* = true */, bool force_host_register /* = false */)
{
  return ReadGuestRegister(m_guest_reg32_cache[guest_reg], OperandSize_32, static_cast<u8>(guest_reg), cache,
                           force_host_register);
}

Value RegisterCache::ReadGuestRegister(Value& cache_value, OperandSize size, u8 guest_reg, bool cache,
                                       bool force_host_register)
{
  if (cache_value.IsValid())
  {
    if (cache_value.IsInHostRegister())
    {
      PushRegisterToOrder(size, guest_reg);
    }
    else if (force_host_register)
    {
      // if it's not in a register, it should be constant
      DebugAssert(cache_value.IsConstant());

      const HostReg host_reg = AllocateHostReg();
      Log_DebugPrintf("Allocated host register %s for constant guest register %s (%" PRIX64,
                      m_code_generator.GetHostRegName(host_reg), Decoder::GetRegisterName(size, guest_reg),
                      cache_value.constant_value);

      m_code_generator.EmitCopyValue(host_reg, cache_value);
      cache_value.AddHostReg(this, host_reg);
      AppendRegisterToOrder(size, guest_reg);
    }

    return cache_value;
  }

  FlushOverlappingGuestRegisters(size, guest_reg);

  const HostReg host_reg = AllocateHostReg();
  m_code_generator.EmitLoadGuestRegister(host_reg, size, guest_reg);

  Log_DebugPrintf("Loading guest register %s to host register %s%s", Decoder::GetRegisterName(size, guest_reg),
                  m_code_generator.GetHostRegName(host_reg, size), cache ? " (cached)" : "");

  if (cache)
  {
    // Now in cache.
    cache_value.SetHostReg(this, host_reg, size);
    AppendRegisterToOrder(size, guest_reg);
    return cache_value;
  }
  else
  {
    // Skip caching, return the register as a value.
    return Value::FromScratch(this, host_reg, size);
  }
}

Value RegisterCache::WriteGuestRegister(Reg8 guest_reg, Value&& value)
{
  return WriteGuestRegister(m_guest_reg8_cache[guest_reg], OperandSize_8, static_cast<u8>(guest_reg), std::move(value));
}

Value RegisterCache::WriteGuestRegister(Reg16 guest_reg, Value&& value)
{
  return WriteGuestRegister(m_guest_reg16_cache[guest_reg], OperandSize_16, static_cast<u8>(guest_reg),
                            std::move(value));
}

Value RegisterCache::WriteGuestRegister(Reg32 guest_reg, Value&& value)
{
  return WriteGuestRegister(m_guest_reg32_cache[guest_reg], OperandSize_32, static_cast<u8>(guest_reg),
                            std::move(value));
}

Value RegisterCache::WriteGuestRegister(Value& cache_value, OperandSize size, u8 guest_reg, Value&& value)
{
  DebugAssert(value.size == size);

  FlushOverlappingGuestRegisters(size, guest_reg);
  InvalidateGuestRegister(cache_value, size, guest_reg);
  DebugAssert(!cache_value.IsValid());

  if (value.IsConstant())
  {
    // No need to allocate a host register, and we can defer the store.
    cache_value = value;
    cache_value.SetDirty();
    return cache_value;
  }

  AppendRegisterToOrder(size, guest_reg);

  // If it's a temporary, we can bind that to the guest register.
  if (value.IsScratch())
  {
    Log_DebugPrintf("Binding scratch register %s to guest register %s",
                    m_code_generator.GetHostRegName(value.host_reg, size), Decoder::GetRegisterName(size, guest_reg));

    cache_value = std::move(value);
    cache_value.flags &= ~ValueFlags::Scratch;
    cache_value.SetDirty();
    return Value::FromHostReg(this, cache_value.host_reg, size);
  }

  // Allocate host register, and copy value to it.
  HostReg host_reg = AllocateHostReg();
  m_code_generator.EmitCopyValue(host_reg, value);
  cache_value.SetHostReg(this, host_reg, size);
  cache_value.SetDirty();

  Log_DebugPrintf("Copying non-scratch register %s to %s to guest register %s",
                  m_code_generator.GetHostRegName(value.host_reg, size),
                  m_code_generator.GetHostRegName(host_reg, size), Decoder::GetRegisterName(size, guest_reg));

  return Value::FromHostReg(this, cache_value.host_reg, size);
}

void RegisterCache::FlushGuestRegister(Reg8 guest_reg, bool invalidate)
{
  FlushGuestRegister(m_guest_reg8_cache[guest_reg], OperandSize_8, static_cast<u8>(guest_reg), invalidate);
}

void RegisterCache::FlushGuestRegister(Reg16 guest_reg, bool invalidate)
{
  FlushGuestRegister(m_guest_reg16_cache[guest_reg], OperandSize_16, static_cast<u8>(guest_reg), invalidate);
}

void RegisterCache::FlushGuestRegister(Reg32 guest_reg, bool invalidate)
{
  FlushGuestRegister(m_guest_reg32_cache[guest_reg], OperandSize_32, static_cast<u8>(guest_reg), invalidate);
}

void RegisterCache::FlushGuestRegister(Value& cache_value, OperandSize size, u8 guest_reg, bool invalidate)
{
  if (cache_value.IsDirty())
  {
    if (cache_value.IsInHostRegister())
    {
      Log_DebugPrintf("Flushing guest register %s from host register %s", Decoder::GetRegisterName(size, guest_reg),
                      m_code_generator.GetHostRegName(cache_value.host_reg, size));
    }
    else if (cache_value.IsConstant())
    {
      Log_DebugPrintf("Flushing guest register %s from constant 0x%" PRIX64, Decoder::GetRegisterName(size, guest_reg),
                      cache_value.constant_value);
    }
    m_code_generator.EmitStoreGuestRegister(size, guest_reg, cache_value);
    cache_value.ClearDirty();
  }

  if (invalidate)
    InvalidateGuestRegister(cache_value, size, guest_reg);
}

void RegisterCache::InvalidateGuestRegister(Reg8 guest_reg)
{
  InvalidateGuestRegister(m_guest_reg8_cache[guest_reg], OperandSize_8, static_cast<u8>(guest_reg));
}

void RegisterCache::InvalidateGuestRegister(Reg16 guest_reg)
{
  InvalidateGuestRegister(m_guest_reg16_cache[guest_reg], OperandSize_16, static_cast<u8>(guest_reg));
}

void RegisterCache::InvalidateGuestRegister(Reg32 guest_reg)
{
  InvalidateGuestRegister(m_guest_reg32_cache[guest_reg], OperandSize_32, static_cast<u8>(guest_reg));
}

void RegisterCache::InvalidateGuestRegister(Value& cache_value, OperandSize size, u8 guest_reg)
{
  if (!cache_value.IsValid())
    return;

  if (cache_value.IsInHostRegister())
  {
    FreeHostReg(cache_value.host_reg);
    ClearRegisterFromOrder(size, guest_reg);
  }

  Log_DebugPrintf("Invalidating guest register %s", Decoder::GetRegisterName(size, guest_reg));
  cache_value.Clear();
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
  if (m_guest_register_order_count == 0)
    return false;

  // evict the register used the longest time ago
  const auto [reg_size, reg_idx] = DecodeRegisterCode(m_guest_register_order[m_guest_register_order_count - 1]);
  if (reg_size == OperandSize_8)
  {
    Log_WarningPrintf("Evicting guest register %s", Decoder::GetRegisterName(static_cast<Reg8>(reg_idx)));
    FlushGuestRegister(static_cast<Reg8>(reg_idx), true);
  }
  else if (reg_size == OperandSize_16)
  {
    Log_WarningPrintf("Evicting guest register %s", Decoder::GetRegisterName(static_cast<Reg16>(reg_idx)));
    FlushGuestRegister(static_cast<Reg16>(reg_idx), true);
  }
  else if (reg_size == OperandSize_32)
  {
    Log_WarningPrintf("Evicting guest register %s", Decoder::GetRegisterName(static_cast<Reg32>(reg_idx)));
    FlushGuestRegister(static_cast<Reg32>(reg_idx), true);
  }

  return HasFreeHostRegister();
}

void RegisterCache::ClearRegisterFromOrder(u8 size, u8 reg)
{
  const u32 code = EncodeRegisterCode(size, reg);
  for (u32 i = 0; i < m_guest_register_order_count; i++)
  {
    if (m_guest_register_order[i] == code)
    {
      // move the registers after backwards into this spot
      const u32 count_after = m_guest_register_order_count - i - 1;
      if (count_after > 0)
        std::memmove(&m_guest_register_order[i], &m_guest_register_order[i + 1], sizeof(u32) * count_after);
      else
        m_guest_register_order[i] = INVALID_REGISTER_CODE;

      m_guest_register_order_count--;
      return;
    }
  }

  Panic("Clearing register from order not in order");
}

void RegisterCache::PushRegisterToOrder(u8 size, u8 reg)
{
  const u32 code = EncodeRegisterCode(size, reg);
  for (u32 i = 0; i < m_guest_register_order_count; i++)
  {
    if (m_guest_register_order[i] == code)
    {
      // move the registers after backwards into this spot
      const u32 count_before = i;
      if (count_before > 0)
        std::memmove(&m_guest_register_order[1], &m_guest_register_order[0], sizeof(u32) * count_before);

      m_guest_register_order[0] = code;
      return;
    }
  }

  Panic("Attempt to push register which is not ordered");
}

void RegisterCache::AppendRegisterToOrder(u8 size, u8 reg)
{
  const u32 code = EncodeRegisterCode(size, reg);
  DebugAssert(m_guest_register_order_count < HostReg_Count);
  if (m_guest_register_order_count > 0)
    std::memmove(&m_guest_register_order[1], &m_guest_register_order[0], sizeof(u32) * m_guest_register_order_count);
  m_guest_register_order[0] = code;
  m_guest_register_order_count++;
}

} // namespace CPU_X86::Recompiler
