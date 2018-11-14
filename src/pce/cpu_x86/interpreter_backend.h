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

  void Reset() override;
  void Execute() override;
  void AbortCurrentInstruction() override;
  void BranchTo(uint32 new_EIP) override;
  void BranchFromException(uint32 new_EIP) override;

  void OnControlRegisterLoaded(Reg32 reg, uint32 old_value, uint32 new_value) override;

  void FlushCodeCache() override;

  // Instruction executer, can be used by other backends.
  static void ExecuteInstruction(CPU* cpu);

private:
  CPU* m_cpu;
  Bus* m_bus;
  System* m_system;
#ifdef Y_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
  fastjmp_buf m_jmp_buf = {};
#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif

  // Helper routines
  static void RaiseInvalidOpcode(CPU* cpu);
  static void FetchModRM(CPU* cpu);
  template<OperandSize op_size, OperandMode op_mode, uint32 op_constant>
  static void FetchImmediate(CPU* cpu);

  // Dispatcher routines
  static void Dispatch(CPU* cpu);
};

} // namespace CPU_X86