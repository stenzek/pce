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
  virtual void BranchTo(uint32 new_EIP) = 0;
  virtual void BranchFromException(uint32 new_EIP) = 0;

  virtual void OnControlRegisterLoaded(Reg32 reg, uint32 old_CR3, uint32 new_CR3) = 0;

  virtual void FlushCodeCache() = 0;
};

} // namespace CPU_X86