#include "pce/cpu_x86/code_cache_backend.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu_x86/debugger_interface.h"
#include "pce/cpu_x86/decoder.h"
#include "pce/cpu_x86/interpreter.h"
#include "pce/system.h"
#include <array>
#include <cstdint>
#include <cstdio>
Log_SetChannel(CPU_X86::CodeCacheBackend);

namespace CPU_X86 {

static_assert(CPU::PAGE_SIZE == Bus::MEMORY_PAGE_SIZE, "CPU page size matches bus memory size");

extern bool TRACE_EXECUTION;
extern u32 TRACE_EXECUTION_LAST_EIP;

CodeCacheBackend::CodeCacheBackend(CPU* cpu) : m_cpu(cpu), m_system(cpu->GetSystem()), m_bus(cpu->GetBus())
{
  m_physical_page_blocks = std::make_unique<BlockArray[]>(m_bus->GetMemoryPageCount());
  m_bus->SetCodeInvalidationCallback(
    std::bind(&CodeCacheBackend::InvalidateBlocksWithPhysicalPage, this, std::placeholders::_1));
}

CodeCacheBackend::~CodeCacheBackend()
{
  m_bus->ClearCodeInvalidationCallback();
  m_bus->ClearPageCodeFlags();
}

void CodeCacheBackend::Reset()
{
  // When we reset, we assume we're not in a block.
  FlushCodeCache();
}

void CodeCacheBackend::OnControlRegisterLoaded(Reg32 reg, u32 old_value, u32 new_value) {}

void CodeCacheBackend::BranchTo(u32 new_EIP)
{
  m_branched = true;
}

void CodeCacheBackend::BranchFromException(u32 new_EIP)
{
  m_branched = true;
}

void CodeCacheBackend::InvalidateBlocksWithPhysicalPage(PhysicalMemoryAddress physical_page_address)
{
  PODArray<BlockBase*>& block_list = m_physical_page_blocks[Bus::GetMemoryPageIndex(physical_page_address)];
  if (block_list.IsEmpty())
    return;

  // We unmark all pages as code, and invalidate the blocks.
  // When the blocks are next executed, they will be re-marked as code.
  m_bus->UnmarkPageAsCode(physical_page_address);

  // Move the list out, so we don't disturb it while iterating.
  PODArray<BlockBase*> temp_block_list;
  temp_block_list.Swap(block_list);

  for (BlockBase* block : temp_block_list)
    InvalidateBlock(block);

  temp_block_list.Swap(block_list);
  block_list.Clear();
}

Bus::CodeHashType CodeCacheBackend::GetBlockCodeHash(BlockBase* block)
{
  if (block->CrossesPage())
  {
    // Combine the hashes of both pages together.
    const u32 size_in_first_page = CPU::PAGE_SIZE - (block->key.eip_physical_address & CPU::PAGE_OFFSET_MASK);
    const u32 size_in_second_page = block->code_length - size_in_first_page;
    return (m_bus->GetCodeHash(block->key.eip_physical_address, size_in_first_page) +
            m_bus->GetCodeHash(block->next_page_physical_address, size_in_second_page));
  }
  else
  {
    // Doesn't cross a page boundary.
    return m_bus->GetCodeHash(block->key.eip_physical_address, block->code_length);
  }
}

bool CodeCacheBackend::GetBlockKeyForCurrentState(BlockKey* key)
{
  // Disable when trap flag is enabled.
  // TODO: Also debug registers, breakpoints active, etc.
  if (m_cpu->m_registers.EFLAGS.TF)
    return false;

  LinearMemoryAddress eip_linear_address = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP);
  if (!m_cpu->CheckSegmentAccess<sizeof(u8), AccessType::Execute>(Segment_CS, m_cpu->m_registers.EIP, false) ||
      !m_cpu->TranslateLinearAddress(
        &key->eip_physical_address, eip_linear_address,
        AddAccessTypeToFlags(AccessType::Execute, AccessFlags::Normal | AccessFlags::NoPageFaults)))
  {
    // Fall back to the interpreter, and let it handle the page fault.
    // Little slower, but more reliable.
    // TODO: Optimize this.
    return false;
  }

  key->eip_physical_address &= m_bus->GetMemoryAddressMask();
  key->cs_size = BoolToUInt32(m_cpu->m_current_operand_size == OperandSize_32);
  key->cs_granularity = 0; // FIXME
  key->ss_size = BoolToUInt32(m_cpu->m_stack_address_size == AddressSize_32);
  key->v8086_mode = m_cpu->InVirtual8086Mode();
  key->pad = 0;
  return true;
}

bool CodeCacheBackend::CanExecuteBlock(BlockBase* block)
{
  // If the block is invalidated, we should check if the code changed.
  if (block->IsInvalidated())
  {
    Bus::CodeHashType new_hash = GetBlockCodeHash(block);
    if (block->code_hash == new_hash)
    {
      Log_DebugPrintf("Block %08X is invalidated - hash matches, revalidating", block->key.eip_physical_address);
      block->flags &= ~BlockFlags::Invalidated;
      AddBlockPhysicalMappings(block);
    }
    else
    {
      // Block is invalid due to code change.
      Log_DebugPrintf("Block %08X is invalidated - hash mismatch, recompiling", block->key.eip_physical_address);
      ResetBlock(block);
      if (CompileBlock(block))
      {
        block->flags &= ~BlockFlags::Invalidated;
        AddBlockPhysicalMappings(block);
        return true;
      }

      Log_DebugPrintf("Block %08X failed recompile, flushing", block->key.eip_physical_address);
      FlushBlock(block);
      return false;
    }
  }

  // Need to check the second page for spanning blocks.
  if (block->CrossesPage())
  {
    const u32 eip_linear_address = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP);
    PhysicalMemoryAddress next_page_physical_address;
    if (!m_cpu->TranslateLinearAddress(
          &next_page_physical_address, ((eip_linear_address + CPU::PAGE_SIZE) & CPU::PAGE_MASK),
          AddAccessTypeToFlags(AccessType::Execute, AccessFlags::Normal | AccessFlags::NoPageFaults)) ||
        next_page_physical_address != block->next_page_physical_address)
    {
      // Can't execute this block, fallback to interpreter.
      Log_PerfPrintf("Cached block %08X incompatible with execution state", block->key.eip_physical_address);
      return false;
    }
  }

  return true;
}

BlockBase* CodeCacheBackend::GetNextBlock()
{
  BlockKey key;
  if (!GetBlockKeyForCurrentState(&key))
  {
    // Not possible to compile this block.
    return nullptr;
  }

  // Block lookup.
  BlockBase* block;
  auto iter = m_blocks.find(key);
  if (iter != m_blocks.end())
  {
    block = iter->second;
    if (!block)
    {
      // This block failed compilation previously, leave it.
      return nullptr;
    }

    // If CanExecuteBlock returns false, it means the block is incompatible with the current execution state.
    // In this case, fall back to the interpreter.
    if (!CanExecuteBlock(block))
      return nullptr;

    // Good to go.
    return block;
  }

  // Block doesn't exist, so compile it.
  Log_DebugPrintf("Attempting to compile block %08X", key.eip_physical_address);
  block = AllocateBlock(key);
  if (!CompileBlock(block))
  {
    Log_WarningPrintf("Failed to compile block %08X", key.eip_physical_address);
    DestroyBlock(block);
    return nullptr;
  }

  // Insert into tree.
  InsertBlock(block);
  return block;
}

void CodeCacheBackend::ResetBlock(BlockBase* block)
{
  UnlinkBlockBase(block);
  block->instructions.clear();
  block->total_cycles = 0;
  block->code_hash = 0;
  block->code_length = 0;
  block->next_page_physical_address = 0;
  block->flags = BlockFlags::None;
}

void CodeCacheBackend::FlushBlock(BlockBase* block, bool defer_destroy /* = false */)
{
  Log_DebugPrintf("Flushing block %08X", block->key.eip_physical_address);

  auto iter = m_blocks.find(block->key);
  if (iter == m_blocks.end())
  {
    Panic("Flushing untracked block");
    return;
  }

  m_blocks.erase(iter);
  UnlinkBlockBase(block);

  // This lookup may fail, if the block has been invalidated.
  if (block->IsValid())
    RemoveBlockPhysicalMappings(block);

  if (defer_destroy)
    block->flags |= BlockFlags::DestroyPending;
  else
    DestroyBlock(block);
}

void CodeCacheBackend::InvalidateBlock(BlockBase* block)
{
  Log_DebugPrintf("Invalidating block %08X", block->key.eip_physical_address);
  block->flags |= BlockFlags::Invalidated;
  RemoveBlockPhysicalMappings(block);
}

size_t CodeCacheBackend::GetCodeBlockCount() const
{
  return m_blocks.size();
}

void CodeCacheBackend::FlushCodeCache()
{
  for (u32 i = 0; i < m_bus->GetMemoryPageCount(); i++)
    m_physical_page_blocks[i].Clear();
  for (auto& iter : m_blocks)
    DestroyBlock(iter.second);
  m_blocks.clear();
  m_bus->ClearPageCodeFlags();
}

void CodeCacheBackend::AddBlockPhysicalMapping(PhysicalMemoryAddress address, BlockBase* block)
{
  const u32 page_index = Bus::GetMemoryPageIndex(address);
  m_physical_page_blocks[page_index].Add(block);
  m_bus->MarkPageAsCode(address);
}

void CodeCacheBackend::AddBlockPhysicalMappings(BlockBase* block)
{
  AddBlockPhysicalMapping(block->GetPhysicalPageAddress(), block);
  if (block->CrossesPage())
    AddBlockPhysicalMapping(block->GetNextPhysicalPageAddress(), block);
}

void CodeCacheBackend::RemoveBlockPhysicalMapping(PhysicalMemoryAddress address, BlockBase* block)
{
  const u32 page_index = Bus::GetMemoryPageIndex(address);
  auto& block_list = m_physical_page_blocks[page_index];
  block_list.FastRemoveItem(block);
  if (block_list.IsEmpty())
    m_bus->UnmarkPageAsCode(address);
}

void CodeCacheBackend::RemoveBlockPhysicalMappings(BlockBase* block)
{
  RemoveBlockPhysicalMapping(block->GetPhysicalPageAddress(), block);
  if (block->CrossesPage())
    RemoveBlockPhysicalMapping(block->GetNextPhysicalPageAddress(), block);
}

bool CodeCacheBackend::CompileBlockBase(BlockBase* block)
{
  static constexpr u32 BUFFER_SIZE = 64;
  DebugAssert(block != nullptr);

  struct FetchCallback
  {
    void FillBuffer()
    {
      if (buffer_pos < buffer_size)
      {
        std::memmove(&buffer[0], &buffer[buffer_pos], buffer_size - buffer_pos);
        buffer_size -= buffer_pos;
        buffer_pos = 0;
      }
      else
      {
        buffer_size = 0;
        buffer_pos = 0;
      }

      const CPU::SegmentCache& segcache = cpu->m_segment_cache[Segment_CS];
      if (EIP < segcache.limit_low || EIP >= segcache.limit_high)
        return;

      u32 available_size = segcache.limit_high - EIP;
      u32 fetch_size = std::min(BUFFER_SIZE - buffer_size, available_size);
      while (fetch_size > 0)
      {
        PhysicalMemoryAddress physical_address;
        LinearMemoryAddress linear_address = segcache.base_address + EIP;
        u32 fetch_size_in_page = std::min(CPU::PAGE_SIZE - (linear_address & CPU::PAGE_OFFSET_MASK), fetch_size);
        if (!cpu->TranslateLinearAddress(
              &physical_address, linear_address,
              AddAccessTypeToFlags(AccessType::Execute, AccessFlags::Normal | AccessFlags::NoPageFaults)))
        {
          return;
        }

        u32 physical_page = (physical_address & cpu->m_bus->GetMemoryAddressMask()) & CPU::PAGE_MASK;
        if (first_physical_page == 0xFFFFFFFF)
        {
          first_physical_page = physical_page;
          last_physical_page = physical_page;
        }

        if (physical_page != last_physical_page)
        {
          // We shouldn't even read if it's not cachable.
          if (!cpu->m_bus->IsCachablePage(physical_page))
            return;

          // Can't span more than two pages.
          if (first_physical_page != last_physical_page)
            return;

          last_physical_page = physical_page;
        }

        cpu->m_bus->ReadMemoryBlock(physical_address, fetch_size_in_page, &buffer[buffer_size]);
        buffer_size += fetch_size_in_page;
        fetch_size -= fetch_size_in_page;
        EIP = (EIP + fetch_size_in_page) & EIP_mask;
      }
    }

    bool FetchByte(u8* val)
    {
      u32 buffer_remaining = buffer_size - buffer_pos;
      if (buffer_remaining < sizeof(u8))
      {
        FillBuffer();
        buffer_remaining = buffer_size - buffer_pos;
      }

      if (buffer_remaining < sizeof(u8))
        return false;

      *val = buffer[buffer_pos++];
      return true;
    }

    bool FetchWord(u16* val)
    {
      u32 buffer_remaining = buffer_size - buffer_pos;
      if (buffer_remaining < sizeof(u16))
      {
        FillBuffer();
        buffer_remaining = buffer_size - buffer_pos;
      }

      if (buffer_remaining < sizeof(u16))
        return false;

      std::memcpy(val, &buffer[buffer_pos], sizeof(u16));
      buffer_pos += sizeof(u16);
      return true;
    }

    u32 FetchDWord(u32* val)
    {
      u32 buffer_remaining = buffer_size - buffer_pos;
      if (buffer_remaining < sizeof(u32))
      {
        FillBuffer();
        buffer_remaining = buffer_size - buffer_pos;
      }

      if (buffer_remaining < sizeof(u32))
        return false;

      std::memcpy(val, &buffer[buffer_pos], sizeof(u32));
      buffer_pos += sizeof(u32);
      return true;
    }

    FetchCallback(CodeCacheBackend* backend_, CPU* cpu_, u32 EIP_, u32 EIP_mask_)
      : backend(backend_), cpu(cpu_), EIP(EIP_), EIP_mask(EIP_mask_)
    {
    }

    CodeCacheBackend* backend;
    CPU* cpu;
    u32 EIP;
    u32 EIP_mask;
    u32 first_physical_page = 0xFFFFFFFF;
    u32 last_physical_page = 0xFFFFFFFF;

    std::array<u8, BUFFER_SIZE> buffer;
    u32 buffer_size = 0;
    u32 buffer_pos = 0;
  };

  FetchCallback callback(this, m_cpu, m_cpu->m_registers.EIP, m_cpu->m_EIP_mask);
  auto fetchb = std::bind(&FetchCallback::FetchByte, &callback, std::placeholders::_1);
  auto fetchw = std::bind(&FetchCallback::FetchWord, &callback, std::placeholders::_1);
  auto fetchd = std::bind(&FetchCallback::FetchDWord, &callback, std::placeholders::_1);
  size_t instruction_count = 0;
  VirtualMemoryAddress start_EIP = m_cpu->m_registers.EIP;
  VirtualMemoryAddress next_EIP = start_EIP;
  block->instructions.reserve(16);

  for (;;)
  {
    block->instructions.emplace_back();
    Instruction* instruction = &block->instructions.back();
    if (!Decoder::DecodeInstruction(instruction, m_cpu->m_current_address_size, m_cpu->m_current_operand_size, next_EIP,
                                    fetchb, fetchw, fetchd))
    {
      block->instructions.pop_back();
      break;
    }

    instruction_count++;
    block->total_cycles++;
    block->code_length += instruction->length;
    next_EIP = (next_EIP + instruction->length) & m_cpu->m_EIP_mask;

    if (IsExitBlockInstruction(instruction))
    {
      if (IsLinkableExitInstruction(instruction))
        block->flags |= BlockFlags::Linkable;

      break;
    }
  }

  block->instructions.shrink_to_fit();

#if !defined(Y_BUILD_CONFIG_RELEASE)

  Log_DebugPrintf("-- COMPILED BLOCK AT %04X:%08X --", ZeroExtend32(m_cpu->m_registers.CS), m_cpu->m_registers.EIP);

  if (block->instructions.empty())
  {
    Log_DebugPrintf("!!! EMPTY BLOCK !!!");
    return false;
  }

  for (const Instruction& instruction : block->instructions)
  {
    SmallString disasm;
    Decoder::DisassembleToString(&instruction, &disasm);
    Log_DebugPrintf("  %08x: %s", instruction.address, disasm.GetCharArray());
  }

  Log_DebugPrintf("-- END BLOCK --");

#else

  if (block->instructions.empty())
    return false;

#endif

  // Does this block cross a page boundary?
  const VirtualMemoryAddress eip_linear_address = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP);
  if ((eip_linear_address & CPU::PAGE_MASK) != ((eip_linear_address + (block->code_length - 1)) & CPU::PAGE_MASK))
    block->flags |= BlockFlags::CrossesPage;

  if (block->CrossesPage())
  {
    DebugAssert(callback.first_physical_page == (block->key.eip_physical_address & CPU::PAGE_MASK));
    block->next_page_physical_address = callback.last_physical_page;
  }

  // Hash the code block to check invalidation.
  block->code_hash = GetBlockCodeHash(block);

  // Log_ErrorPrintf("Block %08X - %u inst, %u length", block->key.eip_physical_address,
  // unsigned(block->instructions.size()), block->code_length);

  return true;
}

void CodeCacheBackend::InsertBlock(BlockBase* block)
{
  m_blocks.emplace(block->key, block);
  AddBlockPhysicalMappings(block);
}

void CodeCacheBackend::LinkBlockBase(BlockBase* from, BlockBase* to)
{
  Log_DebugPrintf("Linking block %p(%08x) to %p(%08x)", from, from->key.eip_physical_address, to,
                  to->key.eip_physical_address);
  from->link_successors.push_back(to);
  to->link_predecessors.push_back(from);
}

void CodeCacheBackend::UnlinkBlockBase(BlockBase* block)
{
  for (BlockBase* predecessor : block->link_predecessors)
  {
    auto iter = std::find(predecessor->link_successors.begin(), predecessor->link_successors.end(), block);
    Assert(iter != predecessor->link_successors.end());
    predecessor->link_successors.erase(iter);
  }
  block->link_predecessors.clear();

  for (BlockBase* successor : block->link_successors)
  {
    auto iter = std::find(successor->link_predecessors.begin(), successor->link_predecessors.end(), block);
    Assert(iter != successor->link_predecessors.end());
    successor->link_predecessors.erase(iter);
  }
  block->link_successors.clear();
}

void CodeCacheBackend::InterpretUncachedBlock()
{
  // The prefetch queue is an unknown state, and likely not in sync with our execution.
  m_cpu->FlushPrefetchQueue();

#if 1
  // Execute until we hit a branch.
  // This isn't our "formal" block exit, but it's a point where we know we'll be in a good state.
  m_branched = false;
  while (!m_branched && !m_cpu->IsHalted() && m_cpu->m_execution_downcount > 0)
  {
    if (m_cpu->HasExternalInterrupt())
    {
      m_cpu->DispatchExternalInterrupt();
      break;
    }

    if (TRACE_EXECUTION && m_cpu->m_registers.EIP != TRACE_EXECUTION_LAST_EIP)
    {
      m_cpu->PrintCurrentStateAndInstruction(m_cpu->m_registers.EIP, nullptr);
      TRACE_EXECUTION_LAST_EIP = m_cpu->m_registers.EIP;
    }

    Interpreter::ExecuteInstruction(m_cpu);
  }

  m_cpu->CommitPendingCycles();

#else
  // This is slower, but the trace output will match the cached variant.
  for (;;)
  {
    if (TRACE_EXECUTION && m_cpu->m_registers.EIP != TRACE_EXECUTION_LAST_EIP)
    {
      m_cpu->PrintCurrentStateAndInstruction(m_cpu->m_registers.EIP, nullptr);
      TRACE_EXECUTION_LAST_EIP = m_cpu->m_registers.EIP;
    }

    u32 fetch_EIP = m_cpu->m_registers.EIP;
    auto fetchb = [this, &fetch_EIP](u8* val) {
      m_cpu->SafeReadMemoryByte(m_cpu->CalculateLinearAddress(Segment_CS, fetch_EIP), val, AccessFlags::Debugger);
      fetch_EIP = (fetch_EIP + sizeof(u8)) & m_cpu->m_EIP_mask;
      return true;
    };
    auto fetchw = [this, &fetch_EIP](u16* val) {
      m_cpu->SafeReadMemoryWord(m_cpu->CalculateLinearAddress(Segment_CS, fetch_EIP), val, AccessFlags::Debugger);
      fetch_EIP = (fetch_EIP + sizeof(u16)) & m_cpu->m_EIP_mask;
      return true;
    };
    auto fetchd = [this, &fetch_EIP](u32* val) {
      m_cpu->SafeReadMemoryDWord(m_cpu->CalculateLinearAddress(Segment_CS, fetch_EIP), val, AccessFlags::Debugger);
      fetch_EIP = (fetch_EIP + sizeof(u32)) & m_cpu->m_EIP_mask;
      return true;
    };

    // Try to decode the instruction first.
    Instruction instruction;
    bool instruction_valid = Decoder::DecodeInstruction(
      &instruction, m_cpu->m_current_address_size, m_cpu->m_current_operand_size, fetch_EIP, fetchb, fetchw, fetchd);

    Interpreter::ExecuteInstruction(m_cpu);

    m_cpu->CommitPendingCycles();

    if (!instruction_valid || IsExitBlockInstruction(&instruction))
      return;
  }
#endif
}

} // namespace CPU_X86
