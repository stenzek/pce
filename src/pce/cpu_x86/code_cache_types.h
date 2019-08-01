#pragma once
#include "pce/bus.h"
#include "pce/cpu_x86/cpu_x86.h"
#include "pce/cpu_x86/instruction.h"
#include <optional>
#include <unordered_map>

namespace CPU_X86 {

struct BlockKey
{
  union
  {
    struct
    {
      u32 eip_physical_address;
      u32 cs_size : 1;
      u32 cs_granularity : 1;
      u32 ss_size : 1;
      u32 v8086_mode : 1;
      u32 pad : 28;
    };

    u64 qword;
  };

  bool operator==(const BlockKey& key) const
  {
    // return (std::memcmp(this, &key, sizeof(key)) == 0);
    return (qword == key.qword);
  }

  bool operator!=(const BlockKey& key) const
  {
    // return (std::memcmp(this, &key, sizeof(key)) != 0);
    return (qword != key.qword);
  }

  bool Is16BitCode() const { return (cs_size == 0); }
  bool Is32BitCode() const { return (cs_size != 0); }
  bool IsV8086Code() const { return (v8086_mode != 0); }
};

struct BlockKeyHash
{
  size_t operator()(const BlockKey& key) const
  {
    return std::hash<u64>()(key.qword);
    // return size_t(key.qword);
  }

  size_t operator()(const BlockKey& lhs, const BlockKey& rhs) const { return lhs.qword < rhs.qword; }
};

enum class BlockFlags : u32
{
  None = 0,

  Linkable = (1 << 1),
  CrossesPage = (1 << 2),

  Decoding = (1 << 3),
  BackgroundCompiling = (1 << 4), // Only used by recompiler backends.
  Invalidated = (1 << 5),
  DestroyPending = (1 << 6),
};
IMPLEMENT_ENUM_CLASS_BITWISE_OPERATORS(BlockFlags);

struct BlockBase
{
  BlockBase(const BlockKey key_);

  std::vector<Instruction> instructions;
  std::vector<BlockBase*> link_predecessors;
  std::vector<BlockBase*> link_successors;
  CycleCount total_cycles = 0;
  Bus::CodeHashType code_hash;
  BlockKey key = {};
  u32 code_length = 0;
  u32 next_page_physical_address = 0;
  BlockFlags flags = BlockFlags::None;

  PhysicalMemoryAddress GetPhysicalAddress() const { return key.eip_physical_address; }
  PhysicalMemoryAddress GetPhysicalPageAddress() const { return (key.eip_physical_address & CPU::PAGE_MASK); }
  PhysicalMemoryAddress GetNextPhysicalPageAddress() const { return next_page_physical_address; }

  bool IsValid() const { return (flags & BlockFlags::Invalidated) == BlockFlags::None; }

  bool IsInvalidated() const { return (flags & BlockFlags::Invalidated) != BlockFlags::None; }

  bool IsLinkable() const { return (flags & BlockFlags::Linkable) != BlockFlags::None; }

  // Returns true if a block crosses a virtual memory page.
  bool CrossesPage() const { return (flags & BlockFlags::CrossesPage) != BlockFlags::None; }

  bool IsDestroyPending() const { return (flags & BlockFlags::DestroyPending) != BlockFlags::None; }

  bool Is16BitCode() const { return key.Is16BitCode(); }
  bool Is32BitCode() const { return key.Is32BitCode(); }
  bool IsV8086Code() const { return key.IsV8086Code(); }
};

bool IsExitBlockInstruction(const Instruction* instruction);
bool IsLinkableExitInstruction(const Instruction* instruction);
bool CanInstructionFault(const Instruction* instruction);
bool OperandIsESP(const Instruction& instruction, u32 index);

std::optional<u8> GetOperandRegister(const Instruction& instruction, u32 index);
bool OperandRegistersMatch(const Instruction& instruction, u32 index1, u32 index2);

// TODO: Model
bool IsInvalidInstruction(const Instruction& instruction);

} // namespace CPU_X86
