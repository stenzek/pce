#pragma once
#include "common/fastjmp.h"
#include "pce/cpu_x86/recompiler_types.h"
#include <unordered_map>

class JitCodeBuffer;

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
  BlockBase* AllocateBlock(const BlockKey key) override;
  bool CompileBlock(BlockBase* block) override;
  void ResetBlock(BlockBase* block) override;
  void FlushBlock(BlockBase* block, bool defer_destroy = false) override;
  void DestroyBlock(BlockBase* block) override;

private:
  /// Block execution dispatcher.
  void Dispatch();

#ifdef Y_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
  fastjmp_buf m_jmp_buf = {};
#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif

  CycleCount m_cycles_remaining = 0;

  std::unordered_map<BlockKey, Block*, BlockKeyHash> m_blocks;
  Block* m_current_block = nullptr;
  bool m_current_block_flushed = false;
  bool m_code_buffer_overflow = false;

  std::unique_ptr<JitCodeBuffer> m_code_space;
};
} // namespace CPU_X86