#pragma once
#include "pce/cpu_x86/code_cache_backend.h"
#include "pce/cpu_x86/cpu.h"
#include <csetjmp>
#include <unordered_map>

namespace CPU_X86 {

class JitX64Code;

class JitX64Backend : public CodeCacheBackend
{
  friend class JitX64CodeGenerator;

public:
  JitX64Backend(CPU* cpu);
  ~JitX64Backend();

  void Reset() override;
  void Execute() override;
  void AbortCurrentInstruction() override;
  void BranchTo(uint32 new_EIP) override;
  void BranchFromException(uint32 new_EIP) override;

private:
  struct Block : public BlockBase
  {
    ~Block();

    static const size_t CODE_SIZE = 4096;
    using CodePointer = void (*)(CPU*);
    // void AllocCode(size_t size);

    CodePointer code_pointer = nullptr;
    size_t code_size = 0;
  };

  // Block flush handling.
  void FlushAllBlocks() override;
  void FlushBlock(const BlockKey& key, bool was_invalidated = false) override;

  // Compile block using current state.
  const Block* LookupBlock();
  Block* CompileBlock();

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

  CycleCount m_cycles_remaining = 0;

  std::unordered_map<BlockKey, Block*, BlockKeyHash> m_blocks;
  const Block* m_current_block = nullptr;
  bool m_current_block_flushed = false;
  bool m_code_buffer_overflow = false;

  std::unique_ptr<JitX64Code> m_code_space;
};
} // namespace CPU_X86