#pragma once
#include "pce/cpu_x86/code_cache_backend.h"
#include "pce/cpu_x86/code_cache_types.h"
#include "pce/cpu_x86/cpu_x86.h"
#include "pce/cpu_x86/recompiler_types.h"
#include "pce/types.h"

#include <array>
#include <tuple>

namespace CPU_X86::Recompiler {

enum class HostRegState : u8
{
  None = 0,
  Usable = (1 << 1),               // Can be allocated
  CallerSaved = (1 << 2),          // Register is caller-saved, and should be saved/restored after calling a function.
  CalleeSaved = (1 << 3),          // Register is callee-saved, and should be restored after leaving the block.
  InUse = (1 << 4),                // In-use, must be saved/restored across function call.
  CalleeSavedAllocated = (1 << 5), // Register was callee-saved and allocated, so should be restored before returning.
};
IMPLEMENT_ENUM_CLASS_BITWISE_OPERATORS(HostRegState);

enum class ValueFlags : u8
{
  None = 0,
  Valid = (1 << 0),
  Constant = (1 << 1),       // The value itself is constant, and not in a register.
  InHostRegister = (1 << 2), // The value itself is located in a host register.
  Scratch = (1 << 3)         // The value is temporary, and will be released after the Value is destroyed.
};
IMPLEMENT_ENUM_CLASS_BITWISE_OPERATORS(ValueFlags);

struct Value
{
  RegisterCache* regcache = nullptr;
  union
  {
    u64 constant_value = 0;
    HostReg host_reg;
  };

  OperandSize size = OperandSize_8;
  ValueFlags flags = ValueFlags::None;

  Value();
  Value(RegisterCache* regcache_, u64 constant_, OperandSize size_, ValueFlags flags_);
  Value(RegisterCache* regcache_, HostReg reg_, OperandSize size_, ValueFlags flags_);
  Value(const Value& other);
  Value(Value&& other);
  ~Value();

  Value& operator=(const Value& other);
  Value& operator=(Value&& other);

  bool IsConstant() const { return (flags & ValueFlags::Constant) != ValueFlags::None; }
  bool IsValid() const { return (flags & ValueFlags::Valid) != ValueFlags::None; }
  bool IsInHostRegister() const { return (flags & ValueFlags::InHostRegister) != ValueFlags::None; }
  bool IsScratch() const { return (flags & ValueFlags::Scratch) != ValueFlags::None; }

  /// Removes the contents of this value. Use with care, as scratch/temporaries are not released.
  void Clear();

  /// Releases the host register if needed, and clears the contents.
  void ReleaseAndClear();

  static Value FromHostReg(RegisterCache* regcache, HostReg reg, OperandSize size)
  {
    return Value(regcache, static_cast<u32>(reg), size, ValueFlags::Valid | ValueFlags::InHostRegister);
  }
  static Value FromScratch(RegisterCache* regcache, HostReg reg, OperandSize size)
  {
    return Value(regcache, static_cast<u32>(reg), size,
                 ValueFlags::Valid | ValueFlags::InHostRegister | ValueFlags::Scratch);
  }
  static Value FromConstant(u64 cv, OperandSize size)
  {
    return Value(nullptr, cv, size, ValueFlags::Valid | ValueFlags::Constant);
  }
  static Value FromConstantU8(u8 value) { return FromConstant(ZeroExtend64(value), OperandSize_8); }
  static Value FromConstantU16(u16 value) { return FromConstant(ZeroExtend64(value), OperandSize_16); }
  static Value FromConstantU32(u32 value) { return FromConstant(ZeroExtend64(value), OperandSize_32); }
  static Value FromConstantU64(u64 value) { return FromConstant(value, OperandSize_64); }

private:
  void Release();
};

enum class GuestRegState : u8
{
  None = 0,            // Unknown, must be loaded.
  Cached = (1 << 1),   // Present in register cache.
  Constant = (1 << 2), // Has a constant value.
  Dirty = (1 << 3),    // Implies cached or constant, but the value differs from the CPU struct.
};
IMPLEMENT_ENUM_CLASS_BITWISE_OPERATORS(GuestRegState);

struct GuestRegData
{
  union
  {
    u32 constant_value;
    HostReg host_reg;
  };
  GuestRegState state;

  bool IsConstant() const { return (state & GuestRegState::Constant) != GuestRegState::None; }
  bool IsDirty() const { return (state & GuestRegState::Dirty) != GuestRegState::None; }
  bool IsCached() const { return (state != GuestRegState::None); }
  bool IsInHostRegister() const
  {
    return (state & (GuestRegState::Cached | GuestRegState::Constant)) == (GuestRegState::Cached);
  }

  void SetConstant(u32 cv)
  {
    constant_value = cv;
    state = GuestRegState::Constant | GuestRegState::Dirty;
  }
  void SetConstantU8(u8 value) { SetConstant(ZeroExtend32(value)); }
  void SetConstantU16(u16 value) { SetConstant(ZeroExtend32(value)); }
  void SetConstantU32(u32 value) { SetConstant(ZeroExtend32(value)); }

  void SetHostReg(HostReg hr)
  {
    constant_value = 0;
    host_reg = hr;
    state = GuestRegState::Cached;
  }

  void SetDirty() { state |= GuestRegState::Dirty; }
  void ClearDirty() { state &= ~GuestRegState::Dirty; }
  void Invalidate() { *this = {}; }

  Value ToValue(RegisterCache* regcache, OperandSize size) const
  {
    if (IsConstant())
      return Value::FromConstant(ZeroExtend64(constant_value), size);
    else
      return Value::FromHostReg(regcache, host_reg, size);
  }
};

class RegisterCache
{
public:
  RegisterCache(CodeGenerator& code_generator);
  ~RegisterCache();

  u32 GetActiveCalleeSavedRegisterCount() const { return m_active_callee_saved_register_count; }

  //////////////////////////////////////////////////////////////////////////
  // Register Allocation
  //////////////////////////////////////////////////////////////////////////
  void SetHostRegAllocationOrder(std::initializer_list<HostReg> regs);
  void SetCallerSavedHostRegs(std::initializer_list<HostReg> regs);
  void SetCalleeSavedHostRegs(std::initializer_list<HostReg> regs);
  void SetCPUPtrHostReg(HostReg reg);

  /// Returns true if the register is permitted to be used in the register cache.
  bool IsUsableHostReg(HostReg reg) const;
  bool IsHostRegInUse(HostReg reg) const;
  bool HasFreeHostRegister() const;
  u32 GetUsedHostRegisters() const;
  u32 GetFreeHostRegisters() const;

  HostReg AllocateHostReg(HostRegState state = HostRegState::InUse);
  void FreeHostReg(HostReg reg);

  // Push/pop volatile host registers. Returns the number of registers pushed/popped.
  u32 PushCallerSavedRegisters() const;
  u32 PopCallerSavedRegisters() const;

  // Restore callee-saved registers. Call at the end of the function.
  u32 PopCalleeSavedRegisters();

  //////////////////////////////////////////////////////////////////////////
  // Scratch Register Allocation
  //////////////////////////////////////////////////////////////////////////
  Value GetCPUPtr();
  Value AllocateScratch(OperandSize size);

  //////////////////////////////////////////////////////////////////////////
  // Guest Register Caching
  //////////////////////////////////////////////////////////////////////////
  void FlushOverlappingGuestRegisters(Reg8 guest_reg);
  void FlushOverlappingGuestRegisters(Reg16 guest_reg);
  void FlushOverlappingGuestRegisters(Reg32 guest_reg);

  Value ReadGuestRegister(Reg8 guest_reg, bool cache = true);
  Value ReadGuestRegister(Reg16 guest_reg, bool cache = true);
  Value ReadGuestRegister(Reg32 guest_reg, bool cache = true);
  Value ReadGuestRegister(OperandSize guest_size, u8 guest_reg, bool cache = true);

  /// Creates a copy of value, and stores it to guest_reg.
  void WriteGuestRegister(Reg8 guest_reg, Value&& value);
  void WriteGuestRegister(Reg16 guest_reg, Value&& value);
  void WriteGuestRegister(Reg32 guest_reg, Value&& value);

  void FlushGuestRegister(Reg8 guest_reg, bool invalidate);
  void FlushGuestRegister(Reg16 guest_reg, bool invalidate);
  void FlushGuestRegister(Reg32 guest_reg, bool invalidate);
  void InvalidateGuestRegister(Reg8 guest_reg);
  void InvalidateGuestRegister(Reg16 guest_reg);
  void InvalidateGuestRegister(Reg32 guest_reg);

  void FlushAllGuestRegisters(bool invalidate);
  bool EvictOneGuestRegister();

private:
  static constexpr u32 EncodeRegisterCode(u8 size, u8 reg) { return ((ZeroExtend16(size) << 8) | ZeroExtend16(reg)); }
  static constexpr std::tuple<u8, u8> DecodeRegisterCode(u32 code)
  {
    return std::tuple<u8, u8>(Truncate8(code >> 8), Truncate8(code));
  }
  static constexpr u32 INVALID_REGISTER_CODE = UINT32_C(0xFFFFFFFF);

  void ClearRegisterFromOrder(u8 size, u8 reg);
  void PushRegisterToOrder(u8 size, u8 reg);
  void AppendRegisterToOrder(u8 size, u8 reg);

  CodeGenerator& m_code_generator;

  HostReg m_cpu_ptr_host_register = {};
  std::array<HostRegState, HostReg_Count> m_host_register_state{};
  std::array<HostReg, HostReg_Count> m_host_register_allocation_order{};
  u32 m_host_register_available_count = 0;
  u32 m_active_callee_saved_register_count = 0;

  std::array<GuestRegData, Reg8_Count> m_guest_reg8_state{};
  std::array<GuestRegData, Reg16_Count> m_guest_reg16_state{};
  std::array<GuestRegData, Reg32_Count> m_guest_reg32_state{};

  std::array<u32, HostReg_Count> m_guest_register_order{};
  u32 m_guest_register_order_count = 0;
};

} // namespace CPU_X86::Recompiler