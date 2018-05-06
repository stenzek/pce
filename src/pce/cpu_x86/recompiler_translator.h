#pragma once
// clang-format off
#include "pce/cpu_x86/recompiler_llvm_headers.h"
// clang-format on
#include "pce/cpu_x86/recompiler_backend.h"
#include <utility>

// TODO: Block leaking on invalidation
// TODO: Remove physical references when block is destroyed
// TODO: block linking
// TODO: memcpy-like stuff from bus for validation

namespace llvm {
class AllocaInst;
class LLVMContext;
class Constant;
class Function;
class Value;
} // namespace llvm

namespace CPU_X86 {
class RecompilerTranslator
{
public:
  RecompilerTranslator(RecompilerBackend* backend, RecompilerBackend::Block* block, llvm::Module* module,
                       llvm::Function* function);
  ~RecompilerTranslator();

  bool TranslateBlock();

private:
  RecompilerBackend* m_backend;
  RecompilerBackend::Block* m_block;
  CPU* m_cpu;

  uint32 m_delayed_eip_add = 0;
  uint32 m_delayed_current_eip_add = 0;
  uint32 m_delayed_cycles_add = 0;
  bool m_update_current_esp = false;

  struct
  {
    llvm::Constant* interpret_instruction;
  } m_trampoline_functions = {};

  struct
  {
    llvm::Value* reg8[Reg8_Count];
    llvm::Value* reg16[Reg16_Count];
    llvm::Value* reg32[Reg32_Count];
    bool reg8_dirty[Reg8_Count];
    bool reg16_dirty[Reg16_Count];
    bool reg32_dirty[Reg32_Count];
  } m_register_cache = {};

  llvm::Module* m_module;
  llvm::Function* m_function;
  llvm::Value* m_function_cpu_ptr;

  llvm::BasicBlock* m_basic_block;
  llvm::IRBuilder<> m_builder;

  llvm::BasicBlock* m_rep_start_block = nullptr;
  llvm::BasicBlock* m_rep_end_block = nullptr;

  //////////////////////////////////////////////////////////////////////////
  // Helpers
  //////////////////////////////////////////////////////////////////////////
  llvm::LLVMContext& GetLLVMContext() const { return m_backend->GetLLVMContext(); }
  llvm::Value* GetPtrValue(const void* ptr);
  llvm::Value* GetCPUInt8Ptr(uint32 offset);
  llvm::Value* GetCPUInt16Ptr(uint32 offset);
  llvm::Value* GetCPUInt32Ptr(uint32 offset);
  llvm::Value* GetCPUInt64Ptr(uint32 offset);

  //////////////////////////////////////////////////////////////////////////
  // Trampoline functions
  //////////////////////////////////////////////////////////////////////////
  llvm::Constant* GetInterpretInstructionFunction();

  //////////////////////////////////////////////////////////////////////////
  // Utility functions
  //////////////////////////////////////////////////////////////////////////
  bool OperandIsESP(const Instruction::Operand& operand);
  bool CanInstructionFault(const Instruction* instruction);
  uint32 CalculateRegisterOffset(Reg8 reg);
  uint32 CalculateRegisterOffset(Reg16 reg);
  uint32 CalculateRegisterOffset(Reg32 reg);
  uint32 CalculateSegmentRegisterOffset(Segment segment);
  llvm::Value* ReadRegister(Reg8 reg);
  llvm::Value* ReadRegister(Reg16 reg);
  llvm::Value* ReadRegister(Reg32 reg);
  void WriteRegister(Reg8 reg, llvm::Value* value);
  void WriteRegister(Reg16 reg, llvm::Value* value);
  void WriteRegister(Reg32 reg, llvm::Value* value);
  void FlushRegister(Reg8 reg);
  void FlushRegister(Reg16 reg);
  void FlushRegister(Reg32 reg);
  void FlushOverlappingRegisters(Reg8 reg);
  void FlushOverlappingRegisters(Reg16 reg);
  void FlushOverlappingRegisters(Reg32 reg);
  llvm::Value* CalculateEffectiveAddress(const Instruction* instruction);
  bool IsConstantOperand(const Instruction* instruction, size_t index);
  llvm::Constant* GetConstantOperand(const Instruction* instruction, size_t index, bool sign_extend);
  llvm::Value* ReadOperand(const Instruction* instruction, size_t index, OperandSize size, bool sign_extend);
  void WriteOperand(const Instruction* instruction, size_t index, llvm::Value* value);
  std::pair<llvm::Value*, llvm::Value*> ReadFarAddressOperand(const Instruction* instruction, size_t index);
  void UpdateFlags(uint32 clear_mask, uint32 set_mask, uint32 host_mask);

  void SyncInstructionPointers();
  void FlushRegisterCache(bool clear_cache);
  void StartInstruction(const Instruction* instruction);
  void EndInstruction(const Instruction* instruction, bool update_esp = false);

  bool CompileInstruction(const Instruction* instruction);

  bool Compile_Fallback(const Instruction* instruction);

  bool Compile_NOP(const Instruction* instruction);
  //   bool Compile_LEA(const Instruction* instruction);
  bool Compile_MOV(const Instruction* instruction);
  //   bool Compile_MOV_Extended(const Instruction* instruction);
  //   bool Compile_ALU_Binary_Update(const Instruction* instruction);
  //   bool Compile_ALU_Binary_Test(const Instruction* instruction);
  //   bool Compile_ALU_Unary_Update(const Instruction* instruction);
  //   bool Compile_ShiftRotate(const Instruction* instruction);
  //   bool Compile_DoublePrecisionShift(const Instruction* instruction);
  //   bool Compile_JumpConditional(const Instruction* instruction);
  //   bool Compile_JumpCallReturn(const Instruction* instruction);
  //   bool Compile_Stack(const Instruction* instruction);
  //   bool Compile_Flags(const Instruction* instruction);
};

} // namespace CPU_X86
