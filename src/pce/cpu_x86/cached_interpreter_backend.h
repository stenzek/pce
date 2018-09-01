#pragma once
#include "common/fastjmp.h"
#include "pce/cpu_x86/code_cache_backend.h"
#include "pce/cpu_x86/cpu.h"
#include <unordered_map>
#include <utility>

namespace CPU_X86 {

class CachedInterpreterBackend : public CodeCacheBackend
{
public:
  CachedInterpreterBackend(CPU* cpu);
  ~CachedInterpreterBackend();

  void Reset() override;
  void Execute() override;
  void AbortCurrentInstruction() override;
  void BranchTo(uint32 new_EIP) override;
  void BranchFromException(uint32 new_EIP) override;

protected:
  // We don't need to store any additional information, only the instruction stream.
  struct Block : BlockBase
  {
  };

  // Block flush handling.
  void FlushAllBlocks() override;
  void FlushBlock(const BlockKey& key, bool was_invalidated = false) override;

  // Compile block using current state.
  Block* LookupBlock();
  Block* LookupBlock(const BlockKey& key);
  Block* CompileBlock(const BlockKey& key);
  void DestroyBlock(Block* block);

#ifdef Y_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
  fastjmp_buf m_jmp_buf = {};
#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif

  std::unordered_map<BlockKey, Block*, BlockKeyHash> m_blocks;
  Block* m_current_block = nullptr;
  bool m_current_block_flushed = false;
};
} // namespace CPU_X86