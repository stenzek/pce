#include "pce/cpu_x86/code_cache_backend.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu_x86/debugger_interface.h"
#include "pce/cpu_x86/decoder.h"
#include "pce/cpu_x86/interpreter_backend.h"
#include "pce/system.h"
#include <array>
#include <cstdint>
#include <cstdio>
Log_SetChannel(CPUX86::Interpreter);

namespace CPU_X86 {

extern bool TRACE_EXECUTION;
extern uint32 TRACE_EXECUTION_LAST_EIP;

CodeCacheBackend::CodeCacheBackend(CPU* cpu) : m_cpu(cpu), m_system(cpu->GetSystem()), m_bus(cpu->GetBus())
{
  m_bus->SetCodeInvalidationCallback(
    std::bind(&CodeCacheBackend::InvalidateBlocksWithPhysicalPage, this, std::placeholders::_1));
}

CodeCacheBackend::~CodeCacheBackend()
{
  m_bus->ClearCodeInvalidationCallback();
}

void CodeCacheBackend::Reset()
{
  // When we reset, we assume we're not in a block.
  ClearPhysicalPageBlockMapping();
}

void CodeCacheBackend::OnControlRegisterLoaded(Reg32 reg, uint32 old_value, uint32 new_value) {}

void CodeCacheBackend::BranchTo(uint32 new_EIP)
{
  m_branched = true;
}

void CodeCacheBackend::BranchFromException(uint32 new_EIP)
{
  m_branched = true;
}

void CodeCacheBackend::FlushCodeCache()
{
  FlushAllBlocks();
}

bool CodeCacheBackend::BlockKey::operator==(const BlockKey& key) const
{
  // return (std::memcmp(this, &key, sizeof(key)) == 0);
  return (qword == key.qword);
}

bool CodeCacheBackend::BlockKey::operator!=(const BlockKey& key) const
{
  // return (std::memcmp(this, &key, sizeof(key)) != 0);
  return (qword != key.qword);
}

size_t CodeCacheBackend::BlockKeyHash::operator()(const BlockKey& key) const
{
  return std::hash<uint64>()(key.qword);
  // return size_t(key.qword);
}

void CodeCacheBackend::InvalidateBlocksWithPhysicalPage(PhysicalMemoryAddress physical_page_address)
{
  auto map_iter = m_physical_page_blocks.find(physical_page_address);
  if (map_iter == m_physical_page_blocks.end())
    return;

  for (BlockBase* block : map_iter->second)
  {
    Log_TracePrintf("Invalidating block 0x%08X", block->key.eip_physical_address);
    block->invalidated = true;
  }

  m_physical_page_blocks.erase(map_iter);
  m_bus->UnmarkPageAsCode(physical_page_address);
}

void CodeCacheBackend::ClearPhysicalPageBlockMapping()
{
  m_physical_page_blocks.clear();
  m_bus->ClearPageCodeFlags();
}

bool CodeCacheBackend::GetBlockKeyForCurrentState(BlockKey* key)
{
  // TODO: Disable when trap flag is enabled.
  // TODO: Real vs protected mode, instructions which can raise exceptions.
  // return false;

  // Fast path when paging isn't enabled.
  LinearMemoryAddress eip_linear_address = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP);
  if (!m_cpu->TranslateLinearAddress(&key->eip_physical_address, eip_linear_address, true, AccessType::Execute, false))
    return false;

  key->eip_physical_address &= m_bus->GetMemoryAddressMask();
  key->cs_size = BoolToUInt32(m_cpu->m_current_operand_size == OperandSize_32);
  key->cs_granularity = 0; // FIXME
  key->ss_size = BoolToUInt32(m_cpu->m_stack_address_size == AddressSize_32);
  key->v8086_mode = m_cpu->InVirtual8086Mode();
  key->pad = 0;
  return true;
}

bool CodeCacheBackend::RevalidateCachedBlockForCurrentState(BlockBase* block)
{
  // TOOD: This could break with A20...
  Bus::CodeHashType hash = m_bus->GetCodeHash(block->key.eip_physical_address, block->code_length);
  if (hash != block->code_hash)
  {
    // Block is invalid due to code change.
    Log_DevPrintf("Block %08X is invalidated - hash mismatch, recompiling", block->key.eip_physical_address);
    return false;
  }

  PhysicalMemoryAddress page_address = block->key.eip_physical_address & Bus::MEMORY_PAGE_MASK;
  m_physical_page_blocks[page_address].push_back(block);
  m_bus->MarkPageAsCode(page_address);

  if (block->crosses_page_boundary)
  {
    page_address = block->next_page_physical_address & Bus::MEMORY_PAGE_MASK;
    m_physical_page_blocks[page_address].push_back(block);
    m_bus->MarkPageAsCode(page_address);
  }

  // Re-validate the block.
  Log_TracePrintf("Block %08X is invalidated - hash matches, revalidating", block->key.eip_physical_address);
  block->invalidated = false;
  return true;
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
    case Operation_INTO:
    case Operation_IRET:
    case Operation_HLT:
    case Operation_LOADALL_286:
    case Operation_INVLPG:
      return true;

    case Operation_MOV:
    {
      // CR3 is an issue because it changes page mappings, CR0 as well, since this can toggle pading.
      if (instruction->operands[0].mode == OperandMode_ModRM_ControlRegister &&
          (instruction->operands[0].reg32 == Reg32_CR0 || instruction->operands[0].reg32 == Reg32_CR3))
      {
        return true;
      }
    }
    break;

    case Operation_MOV_Sreg:
    {
      // Since we use SS as a block key, mov ss, <val> should exit the block.
      if (instruction->operands[0].mode == OperandMode_ModRM_SegmentReg &&
          instruction->operands[0].segreg == Segment_SS)
      {
        return true;
      }
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

void CodeCacheBackend::PrintCPUStateAndInstruction(const Instruction* instruction)
{
#if 0
    std::fprintf(stdout, "EAX=%08X EBX=%08X ECX=%08X EDX=%08X ESI=%08X EDI=%08X\n",
                 m_cpu->m_registers.EAX, m_cpu->m_registers.EBX,
                 m_cpu->m_registers.ECX, m_cpu->m_registers.EDX,
                 m_cpu->m_registers.ESI, m_cpu->m_registers.EDI);
    std::fprintf(stdout, "ESP=%08X EBP=%08X EIP=%04X:%08X EFLAGS=%08X ES=%04X SS=%04X DS=%04X FS=%04X GS=%04X\n",
                 m_cpu->m_registers.ESP, m_cpu->m_registers.EBP,
                 ZeroExtend32(m_cpu->m_registers.CS), m_cpu->m_current_EIP,
                 m_cpu->m_registers.EFLAGS.bits,
                 ZeroExtend32(m_cpu->m_registers.ES), ZeroExtend32(m_cpu->m_registers.SS), ZeroExtend32(m_cpu->m_registers.DS),
                 ZeroExtend32(m_cpu->m_registers.FS), ZeroExtend32(m_cpu->m_registers.GS));
#endif

  SmallString instr_string;
  Decoder::DisassembleToString(instruction, &instr_string);

  LinearMemoryAddress linear_address = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_current_EIP);
  std::fprintf(stdout, "%04X:%08Xh (0x%08X) | %s\n", ZeroExtend32(m_cpu->m_registers.CS), m_cpu->m_current_EIP,
               linear_address, instr_string.GetCharArray());
}

bool CodeCacheBackend::CompileBlockBase(BlockBase* block)
{
  static constexpr uint32 BUFFER_SIZE = 64;

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

      uint32 available_size = segcache.limit_high - EIP;
      uint32 fetch_size = std::min(BUFFER_SIZE - buffer_size, available_size);
      while (fetch_size > 0)
      {
        PhysicalMemoryAddress physical_address;
        LinearMemoryAddress linear_address = segcache.base_address + EIP;
        uint32 fetch_size_in_page = std::min(CPU::PAGE_SIZE - (linear_address & CPU::PAGE_OFFSET_MASK), fetch_size);
        if (!cpu->TranslateLinearAddress(&physical_address, linear_address, true, AccessType::Execute, false))
          return;

        uint32 physical_page = (physical_address & cpu->m_bus->GetMemoryAddressMask()) & Bus::MEMORY_PAGE_MASK;
        if (first_physical_page == 0xFFFFFFFF)
          first_physical_page = physical_page;

        if (physical_page != last_physical_page)
        {
          // We shouldn't even read if it's not cachable.
          if (!cpu->m_bus->IsCachablePage(physical_page))
            return;

          // Can't span more than two pages.
          uint32 page_span = (physical_page - first_physical_page) >> 12;
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

    bool FetchByte(uint8* val)
    {
      uint32 buffer_remaining = buffer_size - buffer_pos;
      if (buffer_remaining < sizeof(uint8))
      {
        FillBuffer();
        buffer_remaining = buffer_size - buffer_pos;
      }

      if (buffer_remaining < sizeof(uint8))
        return false;

      *val = buffer[buffer_pos++];
      return true;
    }

    bool FetchWord(uint16* val)
    {
      uint32 buffer_remaining = buffer_size - buffer_pos;
      if (buffer_remaining < sizeof(uint16))
      {
        FillBuffer();
        buffer_remaining = buffer_size - buffer_pos;
      }

      if (buffer_remaining < sizeof(uint16))
        return false;

      std::memcpy(val, &buffer[buffer_pos], sizeof(uint16));
      buffer_pos += sizeof(uint16);
      return true;
    }

    uint32 FetchDWord(uint32* val)
    {
      uint32 buffer_remaining = buffer_size - buffer_pos;
      if (buffer_remaining < sizeof(uint32))
      {
        FillBuffer();
        buffer_remaining = buffer_size - buffer_pos;
      }

      if (buffer_remaining < sizeof(uint32))
        return false;

      std::memcpy(val, &buffer[buffer_pos], sizeof(uint32));
      buffer_pos += sizeof(uint32);
      return true;
    }

    FetchCallback(CodeCacheBackend* backend_, CPU* cpu_, uint32 EIP_, uint32 EIP_mask_)
      : backend(backend_), cpu(cpu_), EIP(EIP_), EIP_mask(EIP_mask_)
    {
    }

    CodeCacheBackend* backend;
    CPU* cpu;
    uint32 EIP;
    uint32 EIP_mask;
    uint32 first_physical_page = 0xFFFFFFFF;
    uint32 last_physical_page = 0xFFFFFFFF;
    bool is_32bit_code;

    std::array<uint8, BUFFER_SIZE> buffer;
    uint32 buffer_size = 0;
    uint32 buffer_pos = 0;
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

  Log_DevPrintf("-- COMPILED BLOCK AT %04X:%08X --", ZeroExtend32(m_cpu->m_registers.CS), m_cpu->m_registers.EIP);

  if (block->instructions.empty())
  {
    Log_DevPrintf("!!! EMPTY BLOCK !!!");
    return false;
  }

  for (const Instruction& instruction : block->instructions)
  {
    SmallString disasm;
    Decoder::DisassembleToString(&instruction, &disasm);
    Log_DevPrintf("  %08x: %s", instruction.address, disasm.GetCharArray());
  }

  Log_DevPrintf("-- END BLOCK --");

#else

  if (block->instructions.empty())
    return false;

#endif

  // Mark as code so we know when it is overwritten.
  PhysicalMemoryAddress page_address = block->key.eip_physical_address & Bus::MEMORY_PAGE_MASK;
  m_physical_page_blocks[page_address].push_back(block);
  m_bus->MarkPageAsCode(page_address);

  // Does this block cross a page boundary?
  block->crosses_page_boundary = (start_EIP & Bus::MEMORY_PAGE_MASK) != (next_EIP & Bus::MEMORY_PAGE_MASK);
  if (block->crosses_page_boundary)
  {
    block->next_page_physical_address = next_EIP & Bus::MEMORY_PAGE_MASK;
    m_physical_page_blocks[block->next_page_physical_address].push_back(block);
    m_bus->MarkPageAsCode(block->next_page_physical_address);
  }

  block->code_hash = m_bus->GetCodeHash(block->key.eip_physical_address, block->code_length);
  // Log_ErrorPrintf("Block %08X - %u inst, %u length", block->key.eip_physical_address,
  // unsigned(block->instructions.size()), block->code_length);
  return true;
}

void CodeCacheBackend::LinkBlockBase(BlockBase* from, BlockBase* to)
{
  Log_DevPrintf("Linking block %p(%08x) to %p(%08x)", from, from->key.eip_physical_address, to,
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

  auto iter = m_physical_page_blocks.find(block->key.eip_physical_address & Bus::MEMORY_PAGE_MASK);
  if (iter != m_physical_page_blocks.end())
  {
    auto viter = std::find(iter->second.begin(), iter->second.end(), block);
    if (viter != iter->second.end())
      iter->second.erase(viter);
  }
  if (block->crosses_page_boundary)
  {
    iter = m_physical_page_blocks.find(block->next_page_physical_address);
    if (iter != m_physical_page_blocks.end())
    {
      auto viter = std::find(iter->second.begin(), iter->second.end(), block);
      if (viter != iter->second.end())
        iter->second.erase(viter);
    }
  }
}

void CodeCacheBackend::InterpretCachedBlock(const BlockBase* block)
{
  for (const Instruction& instruction : block->instructions)
  {
    m_cpu->AddCycle();

    m_cpu->m_current_EIP = m_cpu->m_registers.EIP;
    m_cpu->m_current_ESP = m_cpu->m_registers.ESP;

    if (TRACE_EXECUTION && m_cpu->m_current_EIP != TRACE_EXECUTION_LAST_EIP)
    {
      m_cpu->PrintCurrentStateAndInstruction(nullptr);
      // PrintCPUStateAndInstruction(&instruction);
      TRACE_EXECUTION_LAST_EIP = m_cpu->m_current_EIP;
    }

    m_cpu->m_registers.EIP = (m_cpu->m_registers.EIP + instruction.length) & m_cpu->m_EIP_mask;
    std::memcpy(&m_cpu->idata, &instruction.data, sizeof(m_cpu->idata));
    instruction.interpreter_handler(m_cpu);
  }
}

void CodeCacheBackend::InterpretUncachedBlock()
{
  // The prefetch queue is an unknown state, and likely not in sync with our execution.
  m_cpu->FlushPrefetchQueue();

  // Execute until we hit a branch.
  // This isn't our "formal" block exit, but it's a point where we know we'll be in a good state.
  m_branched = false;
  while (!m_branched)
    InterpreterBackend::ExecuteInstruction(m_cpu);
}
} // namespace CPU_X86