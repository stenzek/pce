#pragma once
#include "pce/cpu_x86/code_cache_backend.h"
#include "pce/cpu_x86/code_cache_types.h"
#include "pce/cpu_x86/cpu_x86.h"
#include "pce/types.h"

namespace CPU_X86::Recompiler {

struct Block : public BlockBase
{
  friend CodeGenerator;

  Block(const BlockKey key);
  ~Block();

  static constexpr size_t CODE_SIZE = 4096;
  using CodePointer = void (*)(CPU*);
  // void AllocCode(size_t size);

  CodePointer code_pointer = nullptr;
  size_t code_size = 0;
};

enum class HostRegState : u8
{
  None = 0,
  InUse = (1 << 0), // Not - Reserved, therefore we don't need to save it on call.
  Free = (1 << 1),  // Free to use.
  // Temporary = (1 << 2),        // Cannot evict temporaries, until they're freed.
  // GuestCached = (1 << 3),      // Can be evicted.
  // GuestDirty = (1 << 4),       // Can be evicted, after flushing.
  CallerSaved = (1 << 5), // Register is caller-saved, and should be restored before returning.
  // CallerSavedDirty = (1 << 6), // Register is caller-saved, and dirty.
  // Locked = (1 << 7),           // Value must be preserved.
};
IMPLEMENT_ENUM_CLASS_BITWISE_OPERATORS(HostRegState);

#if defined(Y_CPU_X64)
enum HostReg : u8
{
#if 0
  HostReg_al = (static_cast<u16>(HostRegSize_8) << 8) | 0,
  HostReg_cl = (static_cast<u16>(HostRegSize_8) << 8) | 1,
  HostReg_dl = (static_cast<u16>(HostRegSize_8) << 8) | 2,
  HostReg_bl = (static_cast<u16>(HostRegSize_8) << 8) | 3,
  HostReg_spl = (static_cast<u16>(HostRegSize_8) << 8) | 4,
  HostReg_bpl = (static_cast<u16>(HostRegSize_8) << 8) | 5,
  HostReg_sil = (static_cast<u16>(HostRegSize_8) << 8) | 6,
  HostReg_dil = (static_cast<u16>(HostRegSize_8) << 8) | 7,
  HostReg_r8l = (static_cast<u16>(HostRegSize_8) << 8) | 8,
  HostReg_r9l = (static_cast<u16>(HostRegSize_8) << 8) | 9,
  HostReg_r10l = (static_cast<u16>(HostRegSize_8) << 8) | 10,
  HostReg_r11l = (static_cast<u16>(HostRegSize_8) << 8) | 11,
  HostReg_r12l = (static_cast<u16>(HostRegSize_8) << 8) | 12,
  HostReg_r13l = (static_cast<u16>(HostRegSize_8) << 8) | 13,
  HostReg_r14l = (static_cast<u16>(HostRegSize_8) << 8) | 14,
  HostReg_r15l = (static_cast<u16>(HostRegSize_8) << 8) | 15,

  HostReg_ax = (static_cast<u16>(HostRegSize_16) << 8) | 0,
  HostReg_cx = (static_cast<u16>(HostRegSize_16) << 8) | 1,
  HostReg_dx = (static_cast<u16>(HostRegSize_16) << 8) | 2,
  HostReg_bx = (static_cast<u16>(HostRegSize_16) << 8) | 3,
  HostReg_sp = (static_cast<u16>(HostRegSize_16) << 8) | 4,
  HostReg_bp = (static_cast<u16>(HostRegSize_16) << 8) | 5,
  HostReg_si = (static_cast<u16>(HostRegSize_16) << 8) | 6,
  HostReg_di = (static_cast<u16>(HostRegSize_16) << 8) | 7,
  HostReg_r8w = (static_cast<u16>(HostRegSize_16) << 8) | 8,
  HostReg_r9w = (static_cast<u16>(HostRegSize_16) << 8) | 9,
  HostReg_r10w = (static_cast<u16>(HostRegSize_16) << 8) | 10,
  HostReg_r11w = (static_cast<u16>(HostRegSize_16) << 8) | 11,
  HostReg_r12w = (static_cast<u16>(HostRegSize_16) << 8) | 12,
  HostReg_r13w = (static_cast<u16>(HostRegSize_16) << 8) | 13,
  HostReg_r14w = (static_cast<u16>(HostRegSize_16) << 8) | 14,
  HostReg_r15w = (static_cast<u16>(HostRegSize_16) << 8) | 15,

  HostReg_eax = (static_cast<u16>(HostRegSize_32) << 8) | 0,
  HostReg_ecx = (static_cast<u16>(HostRegSize_32) << 8) | 1,
  HostReg_edx = (static_cast<u16>(HostRegSize_32) << 8) | 2,
  HostReg_ebx = (static_cast<u16>(HostRegSize_32) << 8) | 3,
  HostReg_esp = (static_cast<u16>(HostRegSize_32) << 8) | 4,
  HostReg_ebp = (static_cast<u16>(HostRegSize_32) << 8) | 5,
  HostReg_esi = (static_cast<u16>(HostRegSize_32) << 8) | 6,
  HostReg_edi = (static_cast<u16>(HostRegSize_32) << 8) | 7,
  HostReg_r8d = (static_cast<u16>(HostRegSize_32) << 8) | 8,
  HostReg_r9d = (static_cast<u16>(HostRegSize_32) << 8) | 9,
  HostReg_r10d = (static_cast<u16>(HostRegSize_32) << 8) | 10,
  HostReg_r11d = (static_cast<u16>(HostRegSize_32) << 8) | 11,
  HostReg_r12d = (static_cast<u16>(HostRegSize_32) << 8) | 12,
  HostReg_r13d = (static_cast<u16>(HostRegSize_32) << 8) | 13,
  HostReg_r14d = (static_cast<u16>(HostRegSize_32) << 8) | 14,
  HostReg_r15d = (static_cast<u16>(HostRegSize_32) << 8) | 15,

  HostReg_rax = (static_cast<u16>(HostRegSize_64) << 8) | 0,
  HostReg_rcx = (static_cast<u16>(HostRegSize_64) << 8) | 1,
  HostReg_rdx = (static_cast<u16>(HostRegSize_64) << 8) | 2,
  HostReg_rbx = (static_cast<u16>(HostRegSize_64) << 8) | 3,
  HostReg_rsp = (static_cast<u16>(HostRegSize_64) << 8) | 4,
  HostReg_rbp = (static_cast<u16>(HostRegSize_64) << 8) | 5,
  HostReg_rsi = (static_cast<u16>(HostRegSize_64) << 8) | 6,
  HostReg_rdi = (static_cast<u16>(HostRegSize_64) << 8) | 7,
  HostReg_r8 = (static_cast<u16>(HostRegSize_64) << 8) | 8,
  HostReg_r9 = (static_cast<u16>(HostRegSize_64) << 8) | 9,
  HostReg_r10 = (static_cast<u16>(HostRegSize_64) << 8) | 10,
  HostReg_r11 = (static_cast<u16>(HostRegSize_64) << 8) | 11,
  HostReg_r12 = (static_cast<u16>(HostRegSize_64) << 8) | 12,
  HostReg_r13 = (static_cast<u16>(HostRegSize_64) << 8) | 13,
  HostReg_r14 = (static_cast<u16>(HostRegSize_64) << 8) | 14,
  HostReg_r15 = (static_cast<u16>(HostRegSize_64) << 8) | 15,
#endif

  HostReg_rax = 0,
  HostReg_rcx = 1,
  HostReg_rdx = 2,
  HostReg_rbx = 3,
  HostReg_rsp = 4,
  HostReg_rbp = 5,
  HostReg_rsi = 6,
  HostReg_rdi = 7,
  HostReg_r8 = 8,
  HostReg_r9 = 9,
  HostReg_r10 = 10,
  HostReg_r11 = 11,
  HostReg_r12 = 12,
  HostReg_r13 = 13,
  HostReg_r14 = 14,
  HostReg_r15 = 15,
  HostReg_Count = 16
};

#else
enum HostReg : u32
{

};
#endif

enum class ValueFlags : u8
{
  None = 0,
  Valid = (1 << 0),
  Constant = (1 << 1),       // The value itself is constant, and not in a register.
  InHostRegister = (1 << 2), // The value itself is located in a host register.
  Temporary = (1 << 3)       // The value is temporary, and must be released.
};
IMPLEMENT_ENUM_CLASS_BITWISE_OPERATORS(ValueFlags);

struct Value
{
  union
  {
    u32 constant_value;
    HostReg host_reg;
  };

  OperandSize size;
  ValueFlags flags;

  bool IsConstant() const { return (flags & ValueFlags::Constant) != ValueFlags::None; }
  bool IsValid() const { return (flags & ValueFlags::Constant) != ValueFlags::None; }
  bool IsInHostRegister() const { return (flags & ValueFlags::InHostRegister) != ValueFlags::None; }
  bool IsTemporary() const { return (flags & ValueFlags::Temporary) != ValueFlags::None; }
  void Clear() { *this = {}; }

  static Value FromHostReg(HostReg reg, OperandSize size)
  {
    return Value{static_cast<u32>(reg), size, ValueFlags::Valid | ValueFlags::InHostRegister};
  }
  static Value FromTemporary(HostReg reg, OperandSize size)
  {
    return Value{static_cast<u32>(reg), size, ValueFlags::Valid | ValueFlags::InHostRegister | ValueFlags::Temporary};
  }
  static Value FromConstant(u32 cv, OperandSize size)
  {
    return Value{cv, size, ValueFlags::Valid | ValueFlags::Constant};
  }
  static Value FromConstantU8(u8 value) { return FromConstant(ZeroExtend32(value), OperandSize_8); }
  static Value FromConstantU16(u16 value) { return FromConstant(ZeroExtend32(value), OperandSize_16); }
  static Value FromConstantU32(u32 value) { return FromConstant(ZeroExtend32(value), OperandSize_32); }
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
  void ClearDirty() { state &= GuestRegState::Dirty; }
  void Invalidate() { *this = {}; }

  Value ToValue(OperandSize size) const
  {
    if (IsConstant())
      return Value::FromConstant(constant_value, size);
    else
      return Value::FromHostReg(host_reg, size);
  }
};

} // namespace CPU_X86::Recompiler