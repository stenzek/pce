#pragma once
#include "common/fastjmp.h"
#include "pce/cpu_x86/backend.h"
#include "pce/cpu_x86/cpu_x86.h"

namespace CPU_X86 {
class InterpreterBackend : public Backend
{
public:
  InterpreterBackend(CPU* cpu);
  ~InterpreterBackend();

  void Execute() override;
  void AbortCurrentInstruction() override;

  size_t GetCodeBlockCount() const override;
  void FlushCodeCache() override;

private:
  CPU* m_cpu;
  System* m_system;
  Bus* m_bus;
#ifdef Y_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
  fastjmp_buf m_jmp_buf = {};
#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif
};

} // namespace CPU_X86