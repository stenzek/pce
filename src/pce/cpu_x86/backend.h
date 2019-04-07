#pragma once
#include "pce/cpu_x86/types.h"

namespace CPU_X86 {

class Backend
{
public:
  virtual ~Backend() = default;

  virtual void Reset() = 0;
  virtual void Execute() = 0;
  virtual void AbortCurrentInstruction() = 0;
  virtual void BranchTo(u32 new_EIP) = 0;
  virtual void BranchFromException(u32 new_EIP) = 0;

  virtual void OnControlRegisterLoaded(Reg32 reg, u32 old_CR3, u32 new_CR3) = 0;

  virtual void FlushCodeCache() = 0;
};

} // namespace CPU_X86