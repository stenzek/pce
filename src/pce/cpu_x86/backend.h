#pragma once
#include "pce/cpu_x86/types.h"

namespace CPU_X86 {

class Backend
{
public:
  virtual ~Backend() = default;

  virtual void Execute() = 0;
  virtual void AbortCurrentInstruction() = 0;

  virtual size_t GetCodeBlockCount() const = 0;
  virtual void FlushCodeCache() = 0;
};

} // namespace CPU_X86