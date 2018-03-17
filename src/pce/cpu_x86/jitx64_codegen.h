#pragma once
#include <utility>

#include "pce/cpu_x86/jitx64_backend.h"
#include "xbyak.h"

// ABI Selection
#include "YBaseLib/Common.h"
#if defined(Y_PLATFORM_WINDOWS)
#define ABI_WIN64 1
#elif defined(Y_PLATFORM_LINUX) || defined(Y_PLATFORM_ANDROID) || defined(Y_PLATFORM_OSX)
#define ABI_SYSV 1
#else
#error Unknown platform/ABI.
#endif

// TODO: Block leaking on invalidation
// TODO: Remove physical references when block is destroyed
// TODO: block linking
// TODO: memcpy-like stuff from bus for validation

#if 0

namespace CPU_X86 {

class JitX64CodeGenerator : private Xbyak::CodeGenerator
{
public:
  JitX64CodeGenerator(JitX64Backend* backend, void* code_ptr, size_t code_size);
  ~JitX64CodeGenerator();

  std::pair<const void*, size_t> FinishBlock();

  bool CompileInstruction(const Instruction* instruction, bool is_final);

private:
  JitX64Backend* m_backend;
  CPU* m_cpu;

  // Temp registers, destroyed on function call
  const Xbyak::Reg8 &RTEMP8A, RTEMP8B, RTEMP8C;
  const Xbyak::Reg16 &RTEMP16A, RTEMP16B, RTEMP16C;
  const Xbyak::Reg32 &RTEMP32A, RTEMP32B, RTEMP32C;
  const Xbyak::Reg64 &RTEMP64A, RTEMP64B, RTEMP64C;
  const Xbyak::Reg64& RTEMPADDR;

  // Store registers, saved on function call
  const Xbyak::Reg8 &RSTORE8A, RSTORE8B, RSTORE8C;
  const Xbyak::Reg16 &RSTORE16A, RSTORE16B, RSTORE16C;
  const Xbyak::Reg32 &RSTORE32A, RSTORE32B, RSTORE32C;
  const Xbyak::Reg64 &RSTORE64A, RSTORE64B, RSTORE64C;
  const Xbyak::Reg16& READDR16;
  const Xbyak::Reg32& READDR32;
  const Xbyak::Reg64& READDR64;

  // CPU structure pointer
  const Xbyak::Reg64& RCPUPTR;

  // Temp register used in internal stuff
  const Xbyak::Reg64& RSCRATCH64;
  const Xbyak::Reg32& RSCRATCH32;
  const Xbyak::Reg16& RSCRATCH16;
  const Xbyak::Reg8& RSCRATCH8;

  // Registers used for function calls.
  // NOTE: gcc expects high order bits to be zero?
  const Xbyak::Reg8 &RPARAM1_8, RPARAM2_8, RPARAM3_8, RPARAM4_8, RRET_8;
  const Xbyak::Reg16 &RPARAM1_16, RPARAM2_16, RPARAM3_16, RPARAM4_16, RRET_16;
  const Xbyak::Reg32 &RPARAM1_32, RPARAM2_32, RPARAM3_32, RPARAM4_32, RRET_32;
  const Xbyak::Reg64 &RPARAM1_64, RPARAM2_64, RPARAM3_64, RPARAM4_64, RRET_64;

  uint32 m_delayed_eip_add = 0;
  uint32 m_delayed_cycles_add = 0;

  // Calculate the offset relative to the module for a given function
  /*static void DummyFunction() {}
  template<typename T> uint32 CalcModuleRelativeOffset(T param)
  {
      const ptrdiff_t base = reinterpret_cast<ptrdiff_t>(&JitModuleDummyVariable);
      ptrdiff_t diff = reinterpret_cast<ptrdiff_t>(param) - base;
      DebugAssert(Xbyak::inner::IsInInt32(static_cast<uint64>(diff)));
      return static_cast<uint32>(static_cast<uint64>(diff));
  }*/
  template<typename T>
  void CallModuleFunction(T param)
  {
    // uint32 rel = CalcModuleRelativeOffset(param);
    // lea(RSCRATCH, qword[RMODULE + rel]);
    // call(RSCRATCH);
    mov(RSCRATCH64, reinterpret_cast<size_t>(param));
    call(RSCRATCH64);
  }

  // Can destroy temporary registers.
  uint32 CalculateRegisterOffset(Reg8 reg);
  uint32 CalculateRegisterOffset(Reg16 reg);
  uint32 CalculateRegisterOffset(Reg32 reg);
  uint32 CalculateSegmentRegisterOffset(Segment segment);
  void CalculateEffectiveAddress(const Instruction* instruction);
  bool IsConstantOperand(const Instruction* instruction, size_t index);
  uint32 GetConstantOperand(const Instruction* instruction, size_t index, bool sign_extend);
  void ReadOperand(const Instruction* instruction, size_t index, const Xbyak::Reg& dest, bool sign_extend);
  void WriteOperand(const Instruction* instruction, size_t index, const Xbyak::Reg& dest);
  void ReadFarAddressOperand(const Instruction* instruction, size_t index, const Xbyak::Reg& dest_segment,
                             const Xbyak::Reg& dest_offset);
  void UpdateFlags(uint32 clear_mask, uint32 set_mask, uint32 host_mask);

  void SyncInstructionPointers(const Instruction* next_instruction);
  void StartInstruction(const Instruction* instruction);
  void EndInstruction(const Instruction* instruction, bool update_eip = true, bool update_esp = false);

  bool Compile_NOP(const Instruction* instruction);
  bool Compile_LEA(const Instruction* instruction);
  bool Compile_MOV(const Instruction* instruction);
  bool Compile_MOV_Extended(const Instruction* instruction);
  bool Compile_ALU_Binary_Update(const Instruction* instruction);
  bool Compile_ALU_Binary_Test(const Instruction* instruction);
  bool Compile_ALU_Unary_Update(const Instruction* instruction);
  bool Compile_ShiftRotate(const Instruction* instruction);
  bool Compile_DoublePrecisionShift(const Instruction* instruction);
  bool Compile_JumpConditional(const Instruction* instruction);
  bool Compile_JumpCallReturn(const Instruction* instruction);
  bool Compile_Stack(const Instruction* instruction);
  bool Compile_Flags(const Instruction* instruction);
  bool Compile_Fallback(const Instruction* instruction);

  // Helper/wrapper methods.
  static void BranchToTrampoline(CPU* cpu, uint32 address);
  static void PushWordTrampoline(CPU* cpu, uint16 value);
  static void PushDWordTrampoline(CPU* cpu, uint32 value);
  static uint16 PopWordTrampoline(CPU* cpu);
  static uint32 PopDWordTrampoline(CPU* cpu);
  static void LoadSegmentRegisterTrampoline(CPU* cpu, uint32 segment, uint16 value);
  static void RaiseExceptionTrampoline(CPU* cpu, uint32 interrupt, uint32 error_code);
  static void SetFlagsTrampoline(CPU* cpu, uint32 flags);
  static void SetFlags16Trampoline(CPU* cpu, uint16 flags);
  static void FarJumpTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size);
  static void FarCallTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size);
  static void FarReturnTrampoline(CPU* cpu, uint32 op_size, uint32 pop_count);
};

} // namespace CPU_X86
#endif