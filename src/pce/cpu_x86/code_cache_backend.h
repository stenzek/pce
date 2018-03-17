#pragma once
#include "pce/cpu_x86/backend.h"
#include "pce/cpu_x86/cpu.h"
#include "pce/cpu_x86/instruction.h"
#include <unordered_map>

namespace CPU_X86 {

class CodeCacheBackend : public Backend
{
public:
  CodeCacheBackend(CPU* cpu);
  ~CodeCacheBackend();

  virtual void Reset() override;
  virtual void OnControlRegisterLoaded(Reg32 reg, uint32 old_value, uint32 new_value) override;
  virtual void BranchTo(uint32 new_EIP) override;
  virtual void BranchFromException(uint32 new_EIP) override;

  void OnLockedMemoryAccess(PhysicalMemoryAddress address, PhysicalMemoryAddress range_start,
                            PhysicalMemoryAddress range_end, MemoryLockAccess access) override;

  void FlushCodeCache() override;

protected:
  struct BlockKey
  {
    union
    {
      struct
      {
        // Since a block can span a page, we store two page numbers.
        uint32 eip_physical_address;
        uint32 eip_next_physical_page : 20;
        uint32 eip_next_physical_page_valid : 1;
        uint32 cs_size : 1;
        uint32 cs_granularity : 1;
        uint32 ss_size : 1;
        uint32 v8086_mode : 1;
        uint32 pad : 7;
      };

      uint64 qword;
    };

    bool operator==(const BlockKey& key) const;
    bool operator!=(const BlockKey& key) const;
  };

  struct BlockKeyHash
  {
    size_t operator()(const BlockKey& key) const;
    size_t operator()(const BlockKey& lhs, const BlockKey& rhs) const { return lhs.qword < rhs.qword; }
  };

  struct CodeHash
  {
    // uint32 crc32;
    // uint8 md5[16];
    uint64 hash;

    bool operator!=(const CodeHash& rhs) const;
  };

  struct BlockBase
  {
    BlockKey key = {};
    std::vector<Instruction> instructions;
    std::vector<BlockBase*> link_predecessors;
    std::vector<BlockBase*> link_successors;
    CycleCount total_cycles = 0;
    uint32 code_length = 0;
    CodeHash code_hash;
    bool invalidated = false;
    bool linkable = false;

    bool IsLinkable() const { return (linkable); }
  };

  static bool IsExitBlockInstruction(const Instruction* instruction);
  static bool IsLinkableExitInstruction(const Instruction* instruction);

  virtual void FlushAllBlocks() = 0;
  virtual void FlushBlock(const BlockKey& key, bool was_invalidated = false) = 0;

  void ClearPhysicalPageBlockMapping();

  bool GetBlockKeyForCurrentState(BlockKey* key);
  void GetCodeHashForCurrentState(CodeHash* hash, uint32 code_length);
  bool RevalidateCachedBlockForCurrentState(const BlockKey* key, BlockBase* block);

  // Uses the current state of the CPU to compile a block.
  bool CompileBlockBase(BlockBase* block);

  // Link block from to to.
  void LinkBlockBase(BlockBase* from, BlockBase* to);

  // Unlink all blocks which point to this block, and any that this block links to.
  void UnlinkBlockBase(BlockBase* block);

  void PrintCPUStateAndInstruction(const Instruction* instruction);

  void InterpretCachedBlock(const BlockBase* block);
  void InterpretUncachedBlock();

  CPU* m_cpu;
  System* m_system;
  Bus* m_bus;

  std::unordered_map<PhysicalMemoryAddress, std::vector<BlockKey>> m_physical_page_blocks;
  bool m_branched = false;
};
} // namespace CPU_X86