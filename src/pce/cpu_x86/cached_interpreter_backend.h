#pragma once
#include "common/fastjmp.h"
#include "pce/cpu_x86/code_cache_backend.h"
#include "pce/cpu_x86/cpu_x86.h"
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
  void FlushCodeCache() override;

protected:
  // We don't need to store any additional information, only the instruction stream.
  struct Block : public BlockBase
  {
    Block(const BlockKey key_) : BlockBase(key_) {}

    struct Entry
    {
      void (*handler)(CPU*);
      InstructionData data;
      u8 length;
    };
    std::vector<Entry> entries;
  };

  BlockBase* AllocateBlock(const BlockKey key) override;
  bool CompileBlock(BlockBase* block) override;
  void ResetBlock(BlockBase* block) override;
  void FlushBlock(BlockBase* block, bool defer_destroy = false) override;
  void DestroyBlock(BlockBase* block) override;

  void ExecuteBlock(BlockBase* block);

#ifdef Y_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
  fastjmp_buf m_jmp_buf = {};
#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif

  Block* m_current_block = nullptr;
};
} // namespace CPU_X86