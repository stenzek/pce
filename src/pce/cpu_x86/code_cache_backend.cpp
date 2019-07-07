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
Log_SetChannel(CPUX86::Interpreter);

namespace CPU_X86 {

static_assert(CPU::PAGE_SIZE == Bus::MEMORY_PAGE_SIZE, "CPU page size matches bus memory size");

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
  if (block->invalidated)
  {
    Bus::CodeHashType new_hash = GetBlockCodeHash(block);
    if (block->code_hash == new_hash)
    {
      Log_DebugPrintf("Block %08X is invalidated - hash matches, revalidating", block->key.eip_physical_address);
      block->invalidated = false;
      AddBlockPhysicalMappings(block);
    }
    else
    {
      // Block is invalid due to code change.
      Log_DebugPrintf("Block %08X is invalidated - hash mismatch, recompiling", block->key.eip_physical_address);
      ResetBlock(block);
      if (CompileBlock(block))
      {
        block->invalidated = false;
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
    if (!m_cpu->TranslateLinearAddress(&next_page_physical_address,
                                       ((eip_linear_address + CPU::PAGE_SIZE) & CPU::PAGE_MASK),
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

CodeCacheBackend::BlockBase* CodeCacheBackend::GetNextBlock()
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
    m_blocks.emplace(key, nullptr);
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
  block->linkable = false;
  block->destroy_pending = false;
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
  if (!block->invalidated)
    RemoveBlockPhysicalMappings(block);

  if (defer_destroy)
    block->destroy_pending = true;
  else
    DestroyBlock(block);
}

void CodeCacheBackend::InvalidateBlock(BlockBase* block)
{
  Log_DebugPrintf("Invalidating block %08X", block->key.eip_physical_address);
  block->invalidated = true;
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

bool CodeCacheBackend::IsExitBlockInstruction(const Instruction* instruction)
{
  switch (instruction->operation)
  {
    case Operation_JMP_Near:
    case Operation_JMP_Far:
    case Operation_LOOP:
    case Operation_Jcc:
    case Operation_JCXZ:
    case Operation_CALL_Near:
    case Operation_CALL_Far:
    case Operation_RET_Near:
    case Operation_RET_Far:
    case Operation_INT:
    case Operation_INT3:
    case Operation_INTO:
    case Operation_IRET:
    case Operation_HLT:
    case Operation_INVLPG:
    case Operation_BOUND:
      return true;

      // STI strictly shouldn't be an issue, but if a block has STI..CLI in the same block,
      // the interrupt flag will never be checked, resulting in hangs.
    case Operation_STI:
      return true;

    case Operation_MOV_CR:
    {
      // Changing CR0 changes processor behavior, and changing CR3 modifies page mappings.
      if (instruction->operands[0].mode == OperandMode_ModRM_ControlRegister)
      {
        const u8 cr_index = instruction->data.GetModRM_Reg();
        if (cr_index == 0 || cr_index == 3 || cr_index == 4)
          return true;
      }
    }
    break;

    case Operation_MOV_Sreg:
    {
      // Since we use SS as a block key, mov ss, <val> should exit the block.
      if (instruction->operands[0].mode == OperandMode_ModRM_SegmentReg &&
          instruction->data.GetModRM_Reg() == Segment_SS)
      {
        return true;
      }
    }
    break;

    case Operation_MOV_DR:
    {
      // Enabling debug registers should disable the code cache backend.
      if (instruction->operands[0].mode == OperandMode_ModRM_DebugRegister && instruction->data.GetModRM_Reg() >= 3)
        return true;
    }
    break;
  }

  return false;
}

bool CodeCacheBackend::IsLinkableExitInstruction(const Instruction* instruction)
{
  switch (instruction->operation)
  {
    case Operation_JMP_Near:
    case Operation_Jcc:
    case Operation_JCXZ:
    case Operation_LOOP:
    case Operation_CALL_Near:
    case Operation_RET_Near:
    case Operation_INVLPG:
      return true;

    default:
      break;
  }

  return false;
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
          u32 page_span = (physical_page - first_physical_page) >> 12;
          if (page_span > 1)
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
      block->linkable = IsLinkableExitInstruction(instruction);
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
  block->crosses_page =
    ((eip_linear_address & CPU::PAGE_MASK) != ((eip_linear_address + (block->code_length - 1)) & CPU::PAGE_MASK));
  if (block->CrossesPage())
  {
    if (!m_cpu->TranslateLinearAddress(&block->next_page_physical_address,
                                       ((eip_linear_address + CPU::PAGE_SIZE) & CPU::PAGE_MASK),
                                       AccessFlags::Normal | AccessFlags::NoPageFaults))
    {
      Panic("Failed to translate next page address of spanning block");
    }
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