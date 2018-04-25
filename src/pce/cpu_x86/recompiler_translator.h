#pragma once
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4267)
#pragma warning(disable : 4146)
#pragma warning(disable : 4141)
#pragma warning(disable : 4458)
#pragma warning(disable : 4624)
#pragma warning(disable : 4244)
#pragma warning(disable : 4291)
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#define _SCL_SECURE_NO_WARNINGS
#endif

#include "pce/cpu_x86/recompiler_backend.h"
#include <utility>

// Include all LLVM headers. We do this here since we have to mess with warning flags :(
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// TODO: Block leaking on invalidation
// TODO: Remove physical references when block is destroyed
// TODO: block linking
// TODO: memcpy-like stuff from bus for validation

namespace llvm {
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
  uint32 m_delayed_cycles_add = 0;

  struct
  {
    llvm::Function* interpret_instruction;
  } m_trampoline_functions = {};

  llvm::Module* m_module;
  llvm::Function* m_function;
  llvm::BasicBlock* m_basic_block;
  llvm::IRBuilder<> m_builder;

  llvm::BasicBlock* m_rep_start_block = nullptr;
  llvm::BasicBlock* m_rep_end_block = nullptr;

  //////////////////////////////////////////////////////////////////////////
  // Helpers
  //////////////////////////////////////////////////////////////////////////
  llvm::LLVMContext& GetLLVMContext() const { return m_backend->GetLLVMContext(); }

  //////////////////////////////////////////////////////////////////////////
  // Trampoline functions
  //////////////////////////////////////////////////////////////////////////
  llvm::Function* GetInterpretInstructionFunction();

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
  llvm::Value* CalculateEffectiveAddress(const Instruction* instruction);
  bool IsConstantOperand(const Instruction* instruction, size_t index);
  llvm::Constant* GetConstantOperand(const Instruction* instruction, size_t index, bool sign_extend);
  llvm::Value* ReadOperand(const Instruction* instruction, size_t index, OperandSize size, bool sign_extend);
  void WriteOperand(const Instruction* instruction, size_t index, const llvm::Value* value);
  std::pair<llvm::Value*, llvm::Value*> ReadFarAddressOperand(const Instruction* instruction, size_t index);
  void UpdateFlags(uint32 clear_mask, uint32 set_mask, uint32 host_mask);

  void SyncInstructionPointers();
  void StartInstruction(const Instruction* instruction);
  void EndInstruction(const Instruction* instruction, bool update_eip = true, bool update_esp = false);

  bool CompileInstruction(const Instruction* instruction);

  bool Compile_Fallback(const Instruction* instruction);

  bool Compile_NOP(const Instruction* instruction);
  //   bool Compile_LEA(const Instruction* instruction);
  //   bool Compile_MOV(const Instruction* instruction);
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
