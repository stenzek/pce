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

extern bool TRACE_EXECUTION;
extern u32 TRACE_EXECUTION_LAST_EIP;

CodeCacheBackend::CodeCacheBackend(CPU* cpu) : m_cpu(cpu), m_system(cpu->GetSystem()), m_bus(cpu->GetBus())
{
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
  auto map_iter = m_physical_page_blocks.find(physical_page_address);
  if (map_iter == m_physical_page_blocks.end())
    return;

  // We unmark all pages as code, and invalidate the blocks.
  // When the blocks are next executed, they will be re-marked as code.
  m_bus->UnmarkPageAsCode(physical_page_address);

  // Move the list out, so we don't disturb it while iterating.
  auto blocks = std::move(map_iter->second);
  m_physical_page_blocks.erase(map_iter);

  for (BlockBase* block : blocks)
    InvalidateBlock(block);
}

Bus::CodeHashType CodeCacheBackend::GetBlockCodeHash(BlockBase* block)
{
  if (block->CrossesPage())
  {
    // Combine the hashes of both pages together.
    const u32 size_in_first_page = PAGE_SIZE - (block->key.eip_physical_address & PAGE_OFFSET_MASK);
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
    if (!block->IsCompiled() || block->code_hash != GetBlockCodeHash(block))
      return false;

    Log_DebugPrintf("Block %08X is invalidated - hash matches, revalidating", block->key.eip_physical_address);
    block->flags &= ~BlockFlags::Invalidated;
    AddBlockPhysicalMappings(block);
    return true;
  }

  // Need to check the second page for spanning blocks.
  if (block->CrossesPage())
  {
    const u32 eip_linear_address = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP);
    PhysicalMemoryAddress next_page_physical_address;
    if (!m_cpu->TranslateLinearAddress(&next_page_physical_address, ((eip_linear_address + PAGE_SIZE) & PAGE_MASK),
                                       AccessFlags::Normal | AccessFlags::NoPageFaults) ||
        next_page_physical_address != block->next_page_physical_address)
    {
      // Can't execute this block, fallback to interpreter.
      Log_PerfPrintf("Cached block %08X incompatible with execution state", block->key.eip_physical_address);
      return false;
    }
  }

  return true;
}


BlockBase* CodeCacheBackend::LookupBlock(const BlockKey key)
{
  // Block lookup.
  auto iter = m_blocks.find(key);
  if (iter == m_blocks.end())
    return nullptr;

  BlockBase* block = iter->second;
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

bool CodeCacheBackend::RecompileBlock(BlockBase* block)
{
  const bool was_compiled = block->IsCompiled();

  // Block is invalid due to code change.
  Log_DebugPrintf("Block %08X is invalidated - hash mismatch, recompiling", block->key.eip_physical_address);
  ResetBlock(block);

  // Set the previously failed compile flag if it wasn't compiled, so we don't try a third time.
  if (!was_compiled)
  {
    Log_DebugPrintf("Block %08X previously failed compilation", block->key.eip_physical_address);
    block->flags |= BlockFlags::PreviouslyFailedCompilation;
  }

  // Try recompiling it.
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
  m_physical_page_blocks.clear();
  for (auto& iter : m_blocks)
    DestroyBlock(iter.second);
  m_blocks.clear();
  m_bus->ClearPageCodeFlags();
}

void CodeCacheBackend::AddBlockPhysicalMappings(BlockBase* block)
{
#define ADD_PAGE(page_address)                                                                                         \
  do                                                                                                                   \
  {                                                                                                                    \
    auto iter = m_physical_page_blocks.find(page_address);                                                             \
    if (iter == m_physical_page_blocks.end())                                                                          \
    {                                                                                                                  \
      m_physical_page_blocks[page_address].push_back(block);                                                           \
      m_bus->MarkPageAsCode(page_address);                                                                             \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      iter->second.push_back(block);                                                                                   \
    }                                                                                                                  \
  } while (0)

  ADD_PAGE(block->GetPhysicalPageAddress());
  if (block->CrossesPage())
    ADD_PAGE(block->GetNextPhysicalPageAddress());

#undef ADD_PAGE
}

void CodeCacheBackend::RemoveBlockPhysicalMappings(BlockBase* block)
{
#define REMOVE_PAGE(page_address)                                                                                      \
  do                                                                                                                   \
  {                                                                                                                    \
    auto iter = m_physical_page_blocks.find(page_address);                                                             \
    if (iter != m_physical_page_blocks.end())                                                                          \
    {                                                                                                                  \
      auto& page_blocks = iter->second;                                                                                \
      auto iter2 = std::find(page_blocks.begin(), page_blocks.end(), block);                                           \
      if (iter2 != page_blocks.end())                                                                                  \
        page_blocks.erase(iter2);                                                                                      \
      if (page_blocks.empty())                                                                                         \
      {                                                                                                                \
        m_bus->UnmarkPageAsCode(page_address);                                                                         \
        m_physical_page_blocks.erase(iter);                                                                            \
      }                                                                                                                \
    }                                                                                                                  \
  } while (0)

  REMOVE_PAGE(block->GetPhysicalPageAddress());
  if (block->CrossesPage())
    REMOVE_PAGE(block->GetNextPhysicalPageAddress());

#undef REMOVE_PAGE
}

bool CodeCacheBackend::CompileBlockBase(BlockBase* block)
{
  // We need to check if the memory we're in is cacheable.
  // It's okay to pollute the TLB or fault here, since the instruction fetch would've done it anyway.
  const PhysicalMemoryAddress first_page_physical_address = block->key.eip_physical_address;
  if (!m_bus->IsCachablePage(first_page_physical_address))
    return false;

  // Instruction fetch callbacks.
  auto fetchb = [this](u8* val) {
    *val = m_cpu->FetchInstructionByte();
    return true;
  };
  auto fetchw = [this](u16* val) {
    *val = m_cpu->FetchInstructionWord();
    return true;
  };
  auto fetchd = [this](u32* val) {
    *val = m_cpu->FetchInstructionDWord();
    return true;
  };

  // Instruction decoding loop.
  const u16 block_CS = m_cpu->m_registers.CS;
  const u32 block_EIP = m_cpu->m_registers.EIP;
  const LinearMemoryAddress block_linear_address = m_cpu->CalculateLinearAddress(Segment_CS, block_EIP);
  PhysicalMemoryAddress last_page_physical_address = first_page_physical_address;
  LinearMemoryAddress last_page_linear_address = block_linear_address & PAGE_MASK;
  block->instructions.reserve(16);
  for (;;)
  {
    PhysicalMemoryAddress page_physical_address = last_page_physical_address;
    LinearMemoryAddress page_linear_address =
      m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP) & PAGE_MASK;
    if (page_linear_address != last_page_linear_address)
    {
      // The block can't span more than two pages.
      if (block->CrossesPage())
        break;

      m_cpu->TranslateLinearAddress(&page_physical_address, page_linear_address, AccessFlags::Normal);
      if (!m_bus->IsCachablePage(page_physical_address))
        break;
    }

    // Instruction executing setup.
    m_cpu->m_current_EIP = m_cpu->m_registers.EIP;
    m_cpu->m_current_ESP = m_cpu->m_registers.ESP;
    if (TRACE_EXECUTION && m_cpu->m_registers.EIP != TRACE_EXECUTION_LAST_EIP)
    {
      m_cpu->PrintCurrentStateAndInstruction(m_cpu->m_registers.EIP, nullptr);
      TRACE_EXECUTION_LAST_EIP = m_cpu->m_registers.EIP;
    }

    // Decode the instruction using the same fetch as the interpreter.
    block->instructions.emplace_back();
    BlockBase::InstructionEntry* entry = &block->instructions.back();
    if (!Decoder::DecodeInstruction(&entry->instruction, m_cpu->m_current_address_size, m_cpu->m_current_operand_size,
                                    m_cpu->m_registers.EIP, fetchb, fetchw, fetchd))
    {
      m_cpu->m_registers.EIP = m_cpu->m_current_EIP;
      block->instructions.pop_back();
      break;
    }

    // Find the interpreter handler for this instruction and save it.
    entry->interpreter_handler = Interpreter::GetInterpreterHandlerForInstruction(&entry->instruction);
    if (!entry->interpreter_handler)
    {
      String disassembled;
      Decoder::DisassembleToString(&entry->instruction, &disassembled);
      Log_ErrorPrintf("Failed to get handler for instruction '%s'", disassembled.GetCharArray());
      m_cpu->m_registers.EIP = m_cpu->m_current_EIP;
      block->instructions.pop_back();
      break;
    }

    // Metadata.
    block->total_cycles++;
    block->code_length += entry->instruction.length;
    if (page_linear_address != last_page_linear_address)
    {
      last_page_linear_address = page_linear_address;
      last_page_physical_address = page_physical_address;
      block->next_page_physical_address = page_physical_address;
      block->flags |= BlockFlags::CrossesPage;
    }

    // Is this the end of the block?
    const bool end_of_block = IsExitBlockInstruction(&entry->instruction);
    if (end_of_block && IsLinkableExitInstruction(&entry->instruction))
      block->flags |= BlockFlags::Linkable;

    // Execute the instruction. This can raise exceptions. If this happens, we'll flush the block in the abort handler.
    m_cpu->AddCycle();
    std::memcpy(&m_cpu->idata, &entry->instruction.data, sizeof(m_cpu->idata));
    entry->interpreter_handler(m_cpu);

    // Are we done?
    if (end_of_block)
      break;
  }

  block->instructions.shrink_to_fit();

  // Was the block modified while it was executed?
  if (block->IsInvalidated())
  {
    // These are the nastiest sorts of self-modifying code. Just ignore them.
    Log_PerfPrintf("Self-modifying code within block detected at %04X:%08X", ZeroExtend32(block_CS), block_EIP);
    return false;
  }

#if !defined(Y_BUILD_CONFIG_RELEASE)

  Log_DebugPrintf("-- COMPILED BLOCK AT %04X:%08X --", ZeroExtend32(block_CS), block_EIP);
  if (block->instructions.empty())
  {
    Log_DebugPrintf("!!! EMPTY BLOCK !!!");
    return false;
  }

  for (const BlockBase::InstructionEntry& entry : block->instructions)
  {
    SmallString disasm;
    Decoder::DisassembleToString(&entry.instruction, &disasm);
    Log_DebugPrintf("  %08x: %s", entry.instruction.address, disasm.GetCharArray());
  }

  Log_DebugPrintf("-- END BLOCK --");

#else

  if (block->instructions.empty())
    return false;

#endif

  // Hash the code block to check invalidation.
  // TODO: hash the block in place to avoid this call
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