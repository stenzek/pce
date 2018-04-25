#pragma once

#include "pce/cpu_x86/backend.h"
#include "pce/cpu_x86/cpu.h"
#include <csetjmp>

namespace CPU_X86 {
class NewInterpreterBackend : public Backend
{
public:
  NewInterpreterBackend(CPU* cpu);
  ~NewInterpreterBackend();

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
  std::jmp_buf m_jmp_buf = {};
#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif

  // Helper routines
  static void RaiseInvalidOpcode(CPU* cpu);
  static void FetchModRM(CPU* cpu);
  template<OperandSize op_size, OperandMode op_mode, uint32 op_constant>
  static void FetchImmediate(CPU* cpu);

  // Dispatcher routines
  static void Dispatch_Base(CPU* cpu);
  static void Dispatch_Prefix_0f(CPU* cpu);
  static void Dispatch_Prefix_0f00(CPU* cpu);
  static void Dispatch_Prefix_0f01(CPU* cpu);
  static void Dispatch_Prefix_0fba(CPU* cpu);
  static void Dispatch_Prefix_80(CPU* cpu);
  static void Dispatch_Prefix_81(CPU* cpu);
  static void Dispatch_Prefix_82(CPU* cpu);
  static void Dispatch_Prefix_83(CPU* cpu);
  static void Dispatch_Prefix_c0(CPU* cpu);
  static void Dispatch_Prefix_c1(CPU* cpu);
  static void Dispatch_Prefix_d0(CPU* cpu);
  static void Dispatch_Prefix_d1(CPU* cpu);
  static void Dispatch_Prefix_d2(CPU* cpu);
  static void Dispatch_Prefix_d3(CPU* cpu);
  static void Dispatch_Prefix_d8(CPU* cpu);
  static void Dispatch_Prefix_d9(CPU* cpu);
  static void Dispatch_Prefix_da(CPU* cpu);
  static void Dispatch_Prefix_db(CPU* cpu);
  static void Dispatch_Prefix_dc(CPU* cpu);
  static void Dispatch_Prefix_dd(CPU* cpu);
  static void Dispatch_Prefix_de(CPU* cpu);
  static void Dispatch_Prefix_df(CPU* cpu);
  static void Dispatch_Prefix_f6(CPU* cpu);
  static void Dispatch_Prefix_f7(CPU* cpu);
  static void Dispatch_Prefix_fe(CPU* cpu);
  static void Dispatch_Prefix_ff(CPU* cpu);
};

} // namespace CPU_X86