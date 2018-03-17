#include "pce/cpu_x86/code_cache_backend.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu_x86/debugger_interface.h"
#include "pce/cpu_x86/new_decoder.h"
#include "pce/cpu_x86/new_interpreter_backend.h"
#include "pce/system.h"
#include <array>
#include <cstdint>
#include <cstdio>
Log_SetChannel(CPUX86::Interpreter);

#define XXH_PRIVATE_API 1
#define XXH_FORCE_NATIVE_FORMAT 1
#include "xxhash.h"

namespace CPU_X86 {

extern bool TRACE_EXECUTION;
extern uint32 TRACE_EXECUTION_LAST_EIP;

CodeCacheBackend::CodeCacheBackend(CPU* cpu) : m_cpu(cpu), m_system(cpu->GetSystem()), m_bus(cpu->GetBus()) {}

CodeCacheBackend::~CodeCacheBackend() {}

void CodeCacheBackend::Reset()
{
  // When we reset, we assume we're not in a block.
  ClearPhysicalPageBlockMapping();
}

void CodeCacheBackend::OnControlRegisterLoaded(Reg32 reg, uint32 old_value, uint32 new_value)
{
  //     if (reg == Reg32_CR3)
  //         Log_PerfPrintf("CR3 loaded, flushing blocks");
  //     else if (reg == Reg32_CR0 && ((old_value ^ new_value) & CR0Bit_PG))
  //         Log_PerfPrintf("Paging state changed, flushing blocks");
  //     else
  return;

  // FlushBlocks();
}

void CodeCacheBackend::BranchTo(uint32 new_EIP)
{
  m_branched = true;
}

void CodeCacheBackend::BranchFromException(uint32 new_EIP)
{
  m_branched = true;
}

void CodeCacheBackend::OnLockedMemoryAccess(PhysicalMemoryAddress address, PhysicalMemoryAddress range_start,
                                            PhysicalMemoryAddress range_end, MemoryLockAccess access)
{
  if ((access & MemoryLockAccess::Write) == MemoryLockAccess::None)
    return;

  // TODO: Only flush affected blocks
  Log_TracePrintf("Locked memory accessed (0x%08X - 0x%08X : 0x%08X)", range_start, range_end, address);

  for (PhysicalMemoryAddress page_address = range_start; page_address < range_end;
       page_address += Bus::MEMORY_PAGE_SIZE)
  {
    auto map_iter = m_physical_page_blocks.find(page_address);
    if (map_iter == m_physical_page_blocks.end())
      return;

    for (const BlockKey& block_key : map_iter->second)
    {
      Log_TracePrintf("  Block 0x%08X", block_key.eip_physical_address);
      FlushBlock(block_key, true);
    }

    m_physical_page_blocks.erase(map_iter);
  }
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
  // return std::hash<uint64>()(key.qword);
  return size_t(key.qword);
}

void CodeCacheBackend::ClearPhysicalPageBlockMapping()
{
  m_physical_page_blocks.clear();
}

bool CodeCacheBackend::GetBlockKeyForCurrentState(BlockKey* key)
{
  // TODO: Disable when trap flag is enabled.
  // return false;

  // Fast path when paging isn't enabled.
  if (m_cpu->IsPagingEnabled())
  {
    key->eip_physical_address = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP);
    key->eip_next_physical_page = 0;
    key->eip_next_physical_page_valid = false;
  }
  else
  {
    LinearMemoryAddress eip_linear_address = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP);
    PhysicalMemoryAddress eip_physical_address;

#ifdef ENABLE_TLB_EMULATION
    // Optimal path: The page we're looking for is already in the TLB.
    size_t tlb_index = m_cpu->GetTLBEntryIndex(eip_linear_address);
    const CPU::TLBEntry* tlb_entry = &m_cpu->m_tlb_entries[tlb_index];
    if (tlb_entry->linear_address == (eip_linear_address & CPU::PAGE_MASK))
      eip_physical_address = tlb_entry->physical_address + (eip_linear_address & CPU::PAGE_OFFSET_MASK);

    // Otherwise, fall back to the slow path. If this fails, it means page fault, so we shouldn't execute anyway.
    else if (!m_cpu->TranslateLinearAddress(&eip_physical_address, eip_linear_address, true, AccessType::Execute,
                                            false))
      return false;
#else
    if (!m_cpu->TranslateLinearAddress(&eip_physical_address, eip_linear_address, true, AccessType::Execute, false))
      return false;
#endif

    key->eip_physical_address = eip_physical_address;

    // The next page address may be valid, but if it isn't, that's fine.
    // TODO: Don't bother checking the next page when the code length is shorter.
    eip_linear_address = (eip_linear_address & CPU::PAGE_MASK) + CPU::PAGE_SIZE;
    key->eip_next_physical_page_valid = BoolToUInt32(false);
#ifdef ENABLE_TLB_EMULATION
    tlb_index = m_cpu->GetTLBEntryIndex(eip_linear_address);
    tlb_entry = &m_cpu->m_tlb_entries[tlb_index];
    if (tlb_entry->linear_address == eip_linear_address)
      eip_physical_address = tlb_entry->physical_address;
    else
      key->eip_next_physical_page_valid = BoolToUInt32(
        m_cpu->TranslateLinearAddress(&eip_physical_address, eip_linear_address, true, AccessType::Execute, false));
#else
    key->eip_next_physical_page_valid = BoolToUInt32(
      m_cpu->TranslateLinearAddress(&eip_physical_address, eip_linear_address, true, AccessType::Execute, false));
#endif

    key->eip_next_physical_page = eip_physical_address;
  }

  key->cs_size = BoolToUInt32(m_cpu->m_current_operand_size == OperandSize_32);
  key->cs_granularity = 0; // FIXME
  key->ss_size = BoolToUInt32(m_cpu->m_stack_address_size == AddressSize_32);
  key->v8086_mode = m_cpu->InVirtual8086Mode();
  key->pad = 0;
  return true;
}

bool CodeCacheBackend::CodeHash::operator!=(const CodeHash& rhs) const
{
  // return (crc32 != rhs.crc32 /* || (std::memcmp(md5, rhs.md5, sizeof(md5)) != 0)*/);
  return hash != rhs.hash;
}

void CodeCacheBackend::GetCodeHashForCurrentState(CodeHash* hash, uint32 code_length)
{
  // TODO: Use physaddr as seed?
  // CRC32 crc32;
  // MD5Digest md5;
  XXH64_state_t hash_state;
  XXH64_reset(&hash_state, 0);

  // TODO: Optimize to aligned reads and page stuff
  uint32 EIP = m_cpu->GetRegisters()->EIP;
  uint32 EIP_mask = m_cpu->m_EIP_mask;
  while (code_length > 0)
  {
    LinearMemoryAddress linear_address = m_cpu->CalculateLinearAddress(Segment_CS, EIP);
    PhysicalMemoryAddress physical_address;

    if ((code_length) >= sizeof(uint64) &&
        (linear_address & CPU::PAGE_MASK) == ((linear_address + (sizeof(uint64) - 1)) & CPU::PAGE_MASK) &&
        m_cpu->CheckSegmentAccess<sizeof(uint64), AccessType::Execute>(Segment_CS, EIP, false) &&
        m_cpu->TranslateLinearAddress(&physical_address, linear_address, true, AccessType::Execute, false))
    {
      uint64 fetch_qword = m_bus->ReadMemoryQWord(physical_address);
      // crc32.HashBytes(&fetch_qword, sizeof(fetch_qword));
      XXH64_update(&hash_state, &fetch_qword, sizeof(fetch_qword));
      code_length -= sizeof(fetch_qword);
      EIP = (EIP + sizeof(fetch_qword)) & EIP_mask;
      continue;
    }

    if ((code_length) >= sizeof(uint32) &&
        (linear_address & CPU::PAGE_MASK) == ((linear_address + (sizeof(uint32) - 1)) & CPU::PAGE_MASK) &&
        m_cpu->CheckSegmentAccess<sizeof(uint32), AccessType::Execute>(Segment_CS, EIP, false) &&
        m_cpu->TranslateLinearAddress(&physical_address, linear_address, true, AccessType::Execute, false))
    {
      uint32 fetch_dword = m_bus->ReadMemoryDWord(physical_address);
      // crc32.HashBytes(&fetch_dword, sizeof(fetch_dword));
      XXH64_update(&hash_state, &fetch_dword, sizeof(fetch_dword));
      code_length -= sizeof(fetch_dword);
      EIP = (EIP + sizeof(fetch_dword)) & EIP_mask;
      continue;
    }

    if ((code_length) >= sizeof(uint16) &&
        (linear_address & CPU::PAGE_MASK) == ((linear_address + (sizeof(uint16) - 1)) & CPU::PAGE_MASK) &&
        m_cpu->CheckSegmentAccess<sizeof(uint16), AccessType::Execute>(Segment_CS, EIP, false) &&
        m_cpu->TranslateLinearAddress(&physical_address, linear_address, true, AccessType::Execute, false))
    {
      uint16 fetch_word = m_bus->ReadMemoryWord(physical_address);
      // crc32.HashBytes(&fetch_word, sizeof(fetch_word));
      XXH64_update(&hash_state, &fetch_word, sizeof(fetch_word));
      code_length -= sizeof(fetch_word);
      EIP = (EIP + sizeof(fetch_word)) & EIP_mask;
      continue;
    }

    if (!m_cpu->CheckSegmentAccess<1, AccessType::Execute>(Segment_CS, EIP, false) ||
        !m_cpu->TranslateLinearAddress(&physical_address, linear_address, true, AccessType::Execute, false))
    {
      // This is okay, it might mean the CS limit was changed.
      // In which case, the hash will be different, and the block recompiled.
      break;
    }

    uint8 fetch_byte = m_bus->ReadMemoryByte(physical_address);
    // crc32.HashBytes(&fetch_byte, sizeof(fetch_byte));
    XXH64_update(&hash_state, &fetch_byte, sizeof(fetch_byte));
    code_length -= sizeof(fetch_byte);
    EIP = (EIP + 1) & EIP_mask;
  }

  // hash->crc32 = crc32.GetCRC();
  hash->hash = XXH64_digest(&hash_state);
}

bool CodeCacheBackend::RevalidateCachedBlockForCurrentState(const BlockKey* key, BlockBase* block)
{
  CodeHash hash;
  GetCodeHashForCurrentState(&hash, block->code_length);
  if (hash != block->code_hash)
  {
    // Block is invalid due to code change.
    Log_DevPrintf("Block %08X is invalidated - hash mismatch, recompiling", key->eip_physical_address);
    return false;
  }

  // Block is still valid. We need to re-lock the physical memory so that future writes are caught.
  LinearMemoryAddress eip_linear_address = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP);
  PhysicalMemoryAddress eip_physical_address;
  if (!m_cpu->TranslateLinearAddress(&eip_physical_address, eip_linear_address, true, AccessType::Execute, false))
  {
    // This one shouldn't fail, if it does, we need to raise a page fault, so fall back to the interpreter.
    return false;
  }

  // Re-validate the block.
  Log_TracePrintf("Block %08X is invalidated - hash matches, revalidating", key->eip_physical_address);
  block->invalidated = false;

  // Check for blocks that cross pages.
  uint32 physical_page;
  uint32 lock_length = block->code_length;
  if ((eip_linear_address & CPU::PAGE_MASK) == ((eip_linear_address + lock_length - 1) & CPU::PAGE_MASK))
  {
    // Simple, just a single page.
    physical_page = (eip_physical_address & m_bus->GetMemoryAddressMask()) & Bus::MEMORY_PAGE_MASK;
    m_bus->LockPhysicalMemory(eip_physical_address, lock_length, MemoryLockAccess::Write);
    m_physical_page_blocks[physical_page].push_back(*key);
    return true;
  }

  // Page crossing. Need to lock both pages.
  uint32 next_page_start = ((eip_linear_address & CPU::PAGE_MASK) + CPU::PAGE_SIZE);
  uint32 first_length = next_page_start - eip_linear_address;
  uint32 second_length = lock_length - first_length;

  physical_page = (eip_physical_address & m_bus->GetMemoryAddressMask()) & Bus::MEMORY_PAGE_MASK;
  m_bus->LockPhysicalMemory(physical_page, first_length, MemoryLockAccess::Write);
  m_physical_page_blocks[physical_page].push_back(*key);

  physical_page = (next_page_start & m_bus->GetMemoryAddressMask()) & Bus::MEMORY_PAGE_MASK;
  m_bus->LockPhysicalMemory(physical_page, second_length, MemoryLockAccess::Write);
  m_physical_page_blocks[physical_page].push_back(*key);

  // All done
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
  static constexpr size_t BUFFER_SIZE = 16;

  struct FetchCallback : public InstructionFetchCallback
  {
    inline void CheckPhysicalPage(PhysicalMemoryAddress address)
    {
      uint32 physical_page = (address & cpu->m_bus->GetMemoryAddressMask()) & Bus::MEMORY_PAGE_MASK;
      if (first_physical_page == 0xFFFFFFFF)
        first_physical_page = physical_page;

      if (physical_page != last_physical_page)
      {
        last_physical_page = physical_page;
        //                 if (physical_page >= 0x100000)
        //                     return;

        cpu->m_bus->LockPhysicalMemory(physical_page, Bus::MEMORY_PAGE_SIZE, MemoryLockAccess::Write);

        BlockKey key;
        if (backend->GetBlockKeyForCurrentState(&key))
          backend->m_physical_page_blocks[physical_page].push_back(key);

        // Can't span more than two pages.
        uint32 page_span = (last_physical_page - first_physical_page) >> 12;
        if (page_span > 1)
          failed = true;
      }
    }

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

      while (buffer_size < BUFFER_SIZE)
      {
        LinearMemoryAddress linear_address = cpu->CalculateLinearAddress(Segment_CS, EIP);
        PhysicalMemoryAddress physical_address;

        if ((BUFFER_SIZE - buffer_size) >= sizeof(uint64) &&
            (linear_address & CPU::PAGE_MASK) == ((linear_address + (sizeof(uint64) - 1)) & CPU::PAGE_MASK) &&
            cpu->CheckSegmentAccess<sizeof(uint64), AccessType::Execute>(Segment_CS, EIP, false) &&
            cpu->TranslateLinearAddress(&physical_address, linear_address, true, AccessType::Execute, false))
        {
          uint64 fetch_qword = cpu->m_bus->ReadMemoryQWord(physical_address);
          CheckPhysicalPage(physical_address);
          CheckPhysicalPage(physical_address + (sizeof(fetch_qword) - 1));
          std::memcpy(&buffer[buffer_size], &fetch_qword, sizeof(fetch_qword));
          buffer_size += sizeof(fetch_qword);
          EIP = (EIP + sizeof(fetch_qword)) & EIP_mask;
          continue;
        }

        if ((BUFFER_SIZE - buffer_size) >= sizeof(uint32) &&
            (linear_address & CPU::PAGE_MASK) == ((linear_address + (sizeof(uint32) - 1)) & CPU::PAGE_MASK) &&
            cpu->CheckSegmentAccess<sizeof(uint32), AccessType::Execute>(Segment_CS, EIP, false) &&
            cpu->TranslateLinearAddress(&physical_address, linear_address, true, AccessType::Execute, false))
        {
          uint32 fetch_dword = cpu->m_bus->ReadMemoryDWord(physical_address);
          CheckPhysicalPage(physical_address);
          CheckPhysicalPage(physical_address + (sizeof(fetch_dword) - 1));
          std::memcpy(&buffer[buffer_size], &fetch_dword, sizeof(fetch_dword));
          buffer_size += sizeof(fetch_dword);
          EIP = (EIP + sizeof(fetch_dword)) & EIP_mask;
          continue;
        }

        if ((BUFFER_SIZE - buffer_size) >= sizeof(uint16) &&
            (linear_address & CPU::PAGE_MASK) == ((linear_address + (sizeof(uint16) - 1)) & CPU::PAGE_MASK) &&
            cpu->CheckSegmentAccess<sizeof(uint16), AccessType::Execute>(Segment_CS, EIP, false) &&
            cpu->TranslateLinearAddress(&physical_address, linear_address, true, AccessType::Execute, false))
        {
          uint16 fetch_word = cpu->m_bus->ReadMemoryWord(physical_address);
          CheckPhysicalPage(physical_address);
          CheckPhysicalPage(physical_address + (sizeof(fetch_word) - 1));
          std::memcpy(&buffer[buffer_size], &fetch_word, sizeof(fetch_word));
          buffer_size += sizeof(fetch_word);
          EIP = (EIP + sizeof(fetch_word)) & EIP_mask;
          continue;
        }

        if (!cpu->CheckSegmentAccess<1, AccessType::Execute>(Segment_CS, EIP, false) ||
            !cpu->TranslateLinearAddress(&physical_address, linear_address, true, AccessType::Execute, false))
        {
          break;
        }

        uint8 fetch_byte = cpu->m_bus->ReadMemoryByte(physical_address);
        CheckPhysicalPage(physical_address);
        buffer[buffer_size++] = fetch_byte;
        EIP = (EIP + 1) & EIP_mask;
      }
    }

    uint8 FetchByte() override
    {
      uint32 buffer_remaining = buffer_size - buffer_pos;
      if (buffer_remaining < sizeof(uint8))
      {
        FillBuffer();
        buffer_remaining = buffer_size - buffer_pos;
      }

      if (buffer_remaining < sizeof(uint8))
      {
        failed = true;
        return 0;
      }

      return buffer[buffer_pos++];
    }

    uint16 FetchWord() override
    {
      uint32 buffer_remaining = buffer_size - buffer_pos;
      if (buffer_remaining < sizeof(uint16))
      {
        FillBuffer();
        buffer_remaining = buffer_size - buffer_pos;
      }

      if (buffer_remaining < sizeof(uint16))
      {
        failed = true;
        return 0;
      }

      uint16 value;
      std::memcpy(&value, &buffer[buffer_pos], sizeof(value));
      buffer_pos += sizeof(value);
      return value;
    }

    uint32 FetchDWord() override
    {
      uint32 buffer_remaining = buffer_size - buffer_pos;
      if (buffer_remaining < sizeof(uint32))
      {
        FillBuffer();
        buffer_remaining = buffer_size - buffer_pos;
      }

      if (buffer_remaining < sizeof(uint32))
      {
        failed = true;
        return 0;
      }

      uint32 value;
      std::memcpy(&value, &buffer[buffer_pos], sizeof(value));
      buffer_pos += sizeof(value);
      return value;
    }

    FetchCallback(CodeCacheBackend* backend_, CPU* cpu_, uint32 EIP_, uint32 EIP_mask_)
      : backend(backend_), cpu(cpu_), EIP(EIP_), EIP_mask(EIP_mask_), failed(false)
    {
    }

    CodeCacheBackend* backend;
    CPU* cpu;
    uint32 EIP;
    uint32 EIP_mask;
    uint32 first_physical_page = 0xFFFFFFFF;
    uint32 last_physical_page = 0xFFFFFFFF;
    bool is_32bit_code;
    bool failed;

    std::array<uint8, BUFFER_SIZE> buffer;
    uint32 buffer_size = 0;
    uint32 buffer_pos = 0;
  };

  FetchCallback callback(this, m_cpu, m_cpu->m_registers.EIP, m_cpu->m_EIP_mask);
  auto fetchb = [&callback]() -> uint8 { return callback.FetchByte(); };
  auto fetchw = [&callback]() -> uint16 { return callback.FetchWord(); };
  auto fetchd = [&callback]() -> uint32 { return callback.FetchDWord(); };
  size_t instruction_count = 0;
  VirtualMemoryAddress next_EIP = m_cpu->m_registers.EIP;
  block->instructions.reserve(16);

  for (;;)
  {
#if 0
    LinearMemoryAddress linear_addr = m_cpu->CalculateLinearAddress(Segment_CS, next_EIP);
    if (linear_addr == 0x00005B07)
      __debugbreak();
#endif

    block->instructions.emplace_back();
    Instruction* instruction = &block->instructions.back();
    if (!Decoder::DecodeInstruction(instruction, m_cpu->m_current_address_size, m_cpu->m_current_operand_size, next_EIP,
                                    fetchb, fetchw, fetchd) ||
        callback.failed)
    {
      block->instructions.pop_back();
      break;
    }

    instruction_count++;
    block->total_cycles++;
    block->code_length += instruction->length;
    next_EIP = (next_EIP + instruction->length) & m_cpu->m_EIP_mask;

    //     // Skip nops. Cycles are already added so this is fine.
    //     if (instruction->operation == Operation_NOP)
    //     {
    //       block->instructions.pop_back();
    //       continue;
    //     }

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

  GetCodeHashForCurrentState(&block->code_hash, block->code_length);
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
}

void CodeCacheBackend::InterpretCachedBlock(const BlockBase* block)
{
  for (const Instruction& instruction : block->instructions)
  {
#if 0
    // Check for external interrupts.
    if (m_cpu->HasExternalInterrupt())
    {
      m_cpu->DispatchExternalInterrupt();
      AbortCurrentInstruction();
    }
#endif
#if 0
    LinearMemoryAddress linear_addr = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP);
    if (linear_addr == 0x0040185B && m_cpu->m_registers.FS == 0x1A07)
      TRACE_EXECUTION = true;
#endif

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

#if 0
    // Run events if needed.
    m_cpu->CommitPendingCycles();
#endif
  }
}

void CodeCacheBackend::InterpretUncachedBlock()
{
#if 1
  // The prefetch queue is an unknown state, and likely not in sync with our execution.
  m_cpu->FlushPrefetchQueue();

  // Execute until we hit a branch.
  // This isn't our "formal" block exit, but it's a point where we know we'll be in a good state.
  m_branched = false;
  while (!m_branched)
    NewInterpreterBackend::ExecuteInstruction(m_cpu);
#else
  for (;;)
  {
    // Check for external interrupts.
    if (m_cpu->HasExternalInterrupt())
    {
      m_cpu->DispatchExternalInterrupt();
      AbortCurrentInstruction();
    }

    // if (m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP) == 0xC0050E3F)
    // TRACE_EXECUTION = true;

    m_cpu->m_current_EIP = m_cpu->m_registers.EIP;
    m_cpu->m_current_ESP = m_cpu->m_registers.ESP;

    if (TRACE_EXECUTION && m_cpu->m_current_EIP != TRACE_EXECUTION_LAST_EIP)
    {
      m_cpu->PrintCurrentStateAndInstruction(nullptr);
      // PrintCPUStateAndInstruction(&instruction);
      TRACE_EXECUTION_LAST_EIP = m_cpu->m_current_EIP;
    }

    Instruction instruction;
    auto fetchb = [this]() -> uint8 { return m_cpu->FetchInstructionByte(); };
    auto fetchw = [this]() -> uint16 { return m_cpu->FetchInstructionWord(); };
    auto fetchd = [this]() -> uint32 { return m_cpu->FetchInstructionDWord(); };

    if (!Decoder::DecodeInstruction(&instruction, m_cpu->m_current_address_size, m_cpu->m_current_operand_size,
                                    m_cpu->m_registers.EIP, fetchb, fetchw, fetchd))
    {
      m_cpu->RaiseException(Interrupt_InvalidOpcode, 0);
      AbortCurrentInstruction();
    }

    m_cpu->AddCycle();

    if (instruction.operation != Operation_NOP)
    {
      std::memcpy(&m_cpu->idata, &instruction.data, sizeof(m_cpu->idata));
      instruction.interpreter_handler(m_cpu);

      // m_cpu->m_registers.EIP = m_cpu->m_current_EIP;
      // m_cpu->FlushPrefetchQueue();
      // NewInterpreterBackend::ExecuteInstruction(m_cpu);
    }

    // Run events if needed.
    m_cpu->CommitPendingCycles();

    if (IsExitBlockInstruction(&instruction))
      break;
  }
#endif
}
} // namespace CPU_X86