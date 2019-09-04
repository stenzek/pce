#pragma once
#include "pce/bus.h"
#include "pce/cpu_x86/backend.h"
#include "pce/cpu_x86/cpu_x86.h"
#include "pce/cpu_x86/instruction.h"
#include "pce/cpu_x86/code_cache_types.h"
#include "YBaseLib/PODArray.h"
#include <unordered_map>

namespace CPU_X86 {

class CodeCacheBackend : public Backend
{
public:
  CodeCacheBackend(CPU* cpu);
  ~CodeCacheBackend();

  virtual void Reset() override;
  virtual void OnControlRegisterLoaded(Reg32 reg, u32 old_value, u32 new_value) override;
  virtual void BranchTo(u32 new_EIP) override;
  virtual void BranchFromException(u32 new_EIP) override;

  virtual size_t GetCodeBlockCount() const override;
  virtual void FlushCodeCache() override;

protected:
  /// Allocates storage for a block.
  virtual BlockBase* AllocateBlock(const BlockKey key) = 0;

  /// Uses the current state of the CPU to compile a block.
  virtual bool CompileBlock(BlockBase* block) = 0;

  /// Resets a block before recompiling it, which may be more efficient than completely destroying it.
  virtual void ResetBlock(BlockBase* block);

  /// Flushes a block. The destroying of the block may be deferred until later.
  virtual void FlushBlock(BlockBase* block, bool defer_destroy = false);

  /// Destroys a block, freeing all memory and code associated with it.
  virtual void DestroyBlock(BlockBase* block) = 0;

  /// Returns a code block ready for execution based on the current state, otherwise fall back to the interpreter.
  BlockBase* GetNextBlock();

  /// Compiles the base portion of a block (retrieves/decodes the instruction stream).
  bool CompileBlockBase(BlockBase* block);

  /// Inserts the block into the block map.
  void InsertBlock(BlockBase* block);

  /// Invalidates a single block of code, ensuring the code is re-hashed next execution.
  void InvalidateBlock(BlockBase* block);

  /// Invalidates any code blocks with a matching physical page.
  void InvalidateBlocksWithPhysicalPage(PhysicalMemoryAddress physical_page_address);

  /// Removes the physical page -> block mapping for block.
  void AddBlockPhysicalMapping(PhysicalMemoryAddress address, BlockBase* block);
  void AddBlockPhysicalMappings(BlockBase* block);
  void RemoveBlockPhysicalMapping(PhysicalMemoryAddress address, BlockBase* block);
  void RemoveBlockPhysicalMappings(BlockBase* block);

  /// Returns the hash of memory occupied by the block.
  Bus::CodeHashType GetBlockCodeHash(BlockBase* block);

  /// Returns the block key for the current execution state.
  /// Can raise general protection or page faults if the state is invalid.
  bool GetBlockKeyForCurrentState(BlockKey* key);

  /// Can the current block execute? This will re-validate the block if necessary.
  /// The block can also be flushed if recompilation failed, so ignore the pointer if false is returned.
  bool CanExecuteBlock(BlockBase* block);

  /// Link block from to to.
  void LinkBlockBase(BlockBase* from, BlockBase* to);

  /// Unlink all blocks which point to this block, and any that this block links to.
  void UnlinkBlockBase(BlockBase* block);

  /// Runs the interpreter until the emulated CPU branches.
  void InterpretUncachedBlock();

  CPU* m_cpu;
  System* m_system;
  Bus* m_bus;

  std::unordered_map<BlockKey, BlockBase*, BlockKeyHash> m_blocks;

  using BlockArray = PODArray<BlockBase*>;
  std::unique_ptr<BlockArray[]> m_physical_page_blocks;
  bool m_branched = false;
};
} // namespace CPU_X86
