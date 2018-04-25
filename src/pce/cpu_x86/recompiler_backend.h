#pragma once
#include "pce/cpu_x86/code_cache_backend.h"
#include "pce/cpu_x86/cpu.h"
#include <csetjmp>
#include <unordered_map>
#include <utility>

namespace llvm {
class ExecutionEngine;
class Function;
class FunctionType;
class LLVMContext;
class Type;
} // namespace llvm

namespace CPU_X86 {

class RecompilerCodeSpace;

class RecompilerBackend : public CodeCacheBackend
{
  friend class RecompilerTranslator;

public:
  RecompilerBackend(CPU* cpu);
  ~RecompilerBackend();

  RecompilerCodeSpace* GetCodeSpace() const { return m_code_space.get(); }
  llvm::LLVMContext& GetLLVMContext() const { return *m_llvm_context.get(); }

  void Reset() override;
  void Execute() override;
  void AbortCurrentInstruction() override;
  void BranchTo(uint32 new_EIP) override;
  void BranchFromException(uint32 new_EIP) override;

private:
  struct Block : public BlockBase
  {
    using CodePointer = void (*)(uint8*);

    llvm::Function* function = nullptr;
    CodePointer code_pointer = nullptr;
    size_t code_size = 0;
  };

  static TinyString GetBlockModuleName(const Block* block);
  void CreateExecutionEngine();

  // Block flush handling.
  void FlushAllBlocks() override;
  void FlushBlock(const BlockKey& key, bool was_invalidated = false) override;

  // Compile block using current state.
  Block* LookupBlock();
  Block* LookupBlock(const BlockKey& key);
  Block* CompileBlock(const BlockKey& key);
  void DestroyBlock(Block* block);

  // Block execution dispatcher.
  void Dispatch();

#ifdef Y_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
  std::jmp_buf m_jmp_buf = {};
#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif

  std::unordered_map<BlockKey, Block*, BlockKeyHash> m_blocks;
  Block* m_current_block = nullptr;
  bool m_current_block_flushed = false;
  bool m_code_buffer_overflow = false;

  std::unique_ptr<RecompilerCodeSpace> m_code_space;

  //////////////////////////////////////////////////////////////////////////
  // LLVM stuff
  //////////////////////////////////////////////////////////////////////////
  std::unique_ptr<llvm::LLVMContext> m_llvm_context;
  llvm::FunctionType* m_translation_block_function_type;
  std::unique_ptr<llvm::ExecutionEngine> m_execution_engine;
};
} // namespace CPU_X86