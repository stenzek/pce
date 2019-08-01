#pragma once
#include "common/fastjmp.h"
#include "common/jit_code_buffer.h"
#include "pce/cpu_x86/code_cache_backend.h"
#include "pce/cpu_x86/cpu_x86.h"
#include "pce/cpu_x86/recompiler_types.h"
#include <unordered_map>
#include <utility>

namespace CPU_X86::Recompiler {

class Backend : public CodeCacheBackend
{
public:
  Backend(CPU* cpu);
  ~Backend();

  void Reset() override;
  void Execute() override;
  void AbortCurrentInstruction() override;
  void BranchTo(u32 new_EIP) override;
  void BranchFromException(u32 new_EIP) override;
  void FlushCodeCache() override;

protected:
  struct Block : public BlockBase
  {
    Block(const BlockKey key_) : BlockBase(key_) {}

    BlockFunctionType code_pointer = nullptr;
    size_t code_size = 0;
  };

  BlockBase* AllocateBlock(const BlockKey key) override;
  bool CompileBlock(BlockBase* block) override;
  void ResetBlock(BlockBase* block) override;
  void FlushBlock(BlockBase* block, bool defer_destroy = false) override;
  void DestroyBlock(BlockBase* block) override;

  void ExecuteBlock();

#ifdef Y_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
  fastjmp_buf m_jmp_buf = {};
#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif

  Block* m_current_block = nullptr;
  std::unique_ptr<JitCodeBuffer> m_code_space;
  bool m_code_buffer_overflow = false;
};
} // namespace CPU_X86::Recompiler