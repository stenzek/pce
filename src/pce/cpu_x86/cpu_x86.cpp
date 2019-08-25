#include "pce/cpu_x86/cpu_x86.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "pce/bus.h"
#include "pce/cpu_x86/backend.h"
#include "pce/cpu_x86/cached_interpreter_backend.h"
#include "pce/cpu_x86/debugger_interface.h"
#include "pce/cpu_x86/decoder.h"
#include "pce/cpu_x86/interpreter_backend.h"
#include "pce/cpu_x86/recompiler_backend.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cctype>
Log_SetChannel(CPU_X86::CPU);

namespace CPU_X86 {
DEFINE_NAMED_OBJECT_TYPE_INFO(CPU, "CPU_X86");
BEGIN_OBJECT_PROPERTY_MAP(CPU)
END_OBJECT_PROPERTY_MAP()

// Used by backends to enable tracing feature.
#ifdef Y_BUILD_CONFIG_RELEASE
bool TRACE_EXECUTION = false;
#else
bool TRACE_EXECUTION = false;
#endif
u32 TRACE_EXECUTION_LAST_EIP = 0;

static u32 GetCPUIDModel(Model model)
{
  // 386SX - 2308, 386DX - 308
  switch (model)
  {
    case MODEL_386:
      return 0x308;

    case MODEL_486:
      // 486SX - 42A, 486SX2 - 45B, 486DX - 404, 486DX2 - 430, 486DX4 - 481
      return 0x430;

    case MODEL_PENTIUM:
      // P75/P90 - 524, P100 - 525, P120 - 526, P133-200 - 52C, PMMX - 543
      return 0x524;

    default:
      return 0;
  }
}

CPU::CPU(const String& identifier, Model model, float frequency,
         BackendType backend_type /* = BackendType::Interpreter */,
         const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, frequency, backend_type, type_info), m_model(model)
{
}

CPU::~CPU() {}

const char* CPU::GetModelString() const
{
  static const char* model_name_strings[NUM_MODELS] = {"386", "486", "Pentium"};
  return model_name_strings[m_model];
}

bool CPU::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_interrupt_controller = system->GetComponentByType<InterruptController>();
  if (!m_interrupt_controller)
  {
    Log_ErrorPrintf("Failed to locate interrupt controller");
    return false;
  }

  // Copy cycle timings in.
  for (u32 i = 0; i < NUM_CYCLE_GROUPS; i++)
    m_cycle_group_timings[i] = Truncate16(g_cycle_group_timings[i][m_model]);

#ifdef ENABLE_TLB_EMULATION
  InvalidateAllTLBEntries(true);
#endif

  CreateBackend();
  return true;
}

void CPU::Reset()
{
  BaseClass::Reset();

  m_pending_cycles = 0;
  m_tsc_cycles = 0;

  Y_memzero(&m_registers, sizeof(m_registers));
  Y_memzero(&m_fpu_registers, sizeof(m_fpu_registers));
  Y_memzero(&m_msr_registers, sizeof(m_msr_registers));

  // IOPL NT, reserved are 1 on 8086
  m_registers.EFLAGS.bits = 0;
  m_registers.EFLAGS.bits |= Flag_Reserved;
  if (m_model >= MODEL_PENTIUM)
  {
    // Pentium+ supports CPUID.
    m_registers.EFLAGS.bits |= Flag_ID;
  }

  // Load default GDT/IDT locations
  m_idt_location.base_address = 0x0000;
  m_idt_location.limit = 0x3FFF;
  m_gdt_location.base_address = 0;
  m_gdt_location.limit = 0;
  m_ldt_location.base_address = 0;
  m_ldt_location.limit = 0;
  m_tss_location.base_address = 0;
  m_tss_location.limit = 0;

  // Set real mode limits for segment cache
  m_current_address_size = AddressSize_16;
  m_current_operand_size = OperandSize_16;
  m_stack_address_size = AddressSize_16;
  m_EIP_mask = 0xFFFF;
  for (u32 i = 0; i < Segment_Count; i++)
  {
    SegmentCache* segment = &m_segment_cache[i];
    segment->base_address = 0;
    segment->limit_low = 0;
    segment->limit_high = 0xFFFF;
    segment->access.bits = 0;
    segment->access.present = true;
    segment->access.dpl = 0;
    segment->access_mask = AccessTypeMask::All;
    if (i == Segment_CS)
    {
      segment->access.is_code = true;
      segment->access.code_readable = true;
      segment->access.code_confirming = false;
    }
    else
    {
      segment->access.is_code = false;
      segment->access.data_writable = true;
    }
  }

  // Execution begins at F000:FFF0
  // CS is F000 but the base is FFFF0000
  m_registers.CS = 0xF000;
  m_segment_cache[Segment_CS].base_address = 0xFFFF0000u;
  m_registers.EIP = 0xFFF0;

  // Protected mode off, FPU not present, cache disabled.
  m_registers.CR0 = 0;
  if (m_model >= MODEL_386)
    m_registers.CR0 |= CR0Bit_ET; // 80387 FPU
  if (m_model >= MODEL_486)
    m_registers.CR0 |= CR0Bit_CD | CR0Bit_NW | CR0Bit_ET;

  // Initial values of EDX.
  if (m_model >= MODEL_486)
    m_registers.EDX = GetCPUIDModel(m_model);

  // Start at privilege level 0
  SetCPL(0);
  m_halted = false;
  m_nmi_state = false;

  m_current_EIP = m_registers.EIP;
  m_current_ESP = m_registers.ESP;
  m_current_exception = Interrupt_Count;
  m_trap_after_instruction = false;

  // x87 state
  m_fpu_registers.CW.bits = 0x0040;
  m_fpu_registers.SW.bits = 0x0000;
  m_fpu_registers.TW.bits = 0x5555;

  InvalidateAllTLBEntries(true);
  FlushPrefetchQueue();

  m_effective_address = 0;
  std::memset(&idata, 0, sizeof(idata));
  m_execution_stats = {};

  m_backend->Reset();
}

bool CPU::LoadState(BinaryReader& reader)
{
  if (!BaseClass::LoadState(reader))
    return false;

  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  Model model = static_cast<Model>(reader.ReadUInt8());
  if (model != m_model)
  {
    Log_ErrorPrintf("Incompatible CPU models");
    return false;
  }

  reader.SafeReadInt64(&m_pending_cycles);
  reader.SafeReadInt64(&m_execution_downcount);
  reader.SafeReadInt64(&m_tsc_cycles);
  reader.SafeReadUInt32(&m_current_EIP);
  reader.SafeReadUInt32(&m_current_ESP);

  // reader.SafeReadBytes(&m_registers, sizeof(m_registers));
  for (u32 i = 0; i < Reg32_Count; i++)
    reader.SafeReadUInt32(&m_registers.reg32[i]);
  for (u32 i = 0; i < Segment_Count; i++)
    reader.SafeReadUInt16(&m_registers.segment_selectors[i]);
  reader.SafeReadUInt16(&m_registers.LDTR);
  reader.SafeReadUInt16(&m_registers.TR);

  for (u32 i = 0; i < countof(m_fpu_registers.ST); i++)
  {
    reader.SafeReadUInt64(&m_fpu_registers.ST[i].low);
    reader.SafeReadUInt16(&m_fpu_registers.ST[i].high);
  }
  reader.SafeReadUInt16(&m_fpu_registers.CW.bits);
  reader.SafeReadUInt16(&m_fpu_registers.SW.bits);
  reader.SafeReadUInt16(&m_fpu_registers.TW.bits);

  u8 current_address_size = 0;
  u8 current_operand_size = 0;
  u8 stack_address_size = 0;
  reader.SafeReadUInt8(&current_address_size);
  reader.SafeReadUInt8(&current_operand_size);
  reader.SafeReadUInt8(&stack_address_size);
  m_current_address_size = static_cast<AddressSize>(current_address_size);
  m_current_operand_size = static_cast<OperandSize>(current_operand_size);
  m_stack_address_size = static_cast<AddressSize>(stack_address_size);
  reader.SafeReadUInt32(&m_EIP_mask);

  auto ReadDescriptorTablePointer = [&reader](DescriptorTablePointer* ptr) {
    reader.SafeReadUInt32(&ptr->base_address);
    reader.SafeReadUInt32(&ptr->limit);
  };
  ReadDescriptorTablePointer(&m_idt_location);
  ReadDescriptorTablePointer(&m_gdt_location);
  ReadDescriptorTablePointer(&m_ldt_location);

  reader.SafeReadUInt32(&m_tss_location.base_address);
  reader.SafeReadUInt32(&m_tss_location.limit);
  u8 ts_type = 0;
  reader.SafeReadUInt8(&ts_type);
  m_tss_location.type = static_cast<DESCRIPTOR_TYPE>(ts_type);

  auto ReadSegmentCache = [&reader](SegmentCache* ptr) {
    reader.SafeReadUInt32(&ptr->base_address);
    reader.SafeReadUInt32(&ptr->limit_low);
    reader.SafeReadUInt32(&ptr->limit_high);
    reader.SafeReadUInt8(&ptr->access.bits);
    reader.SafeReadUInt8(reinterpret_cast<u8*>(&ptr->access_mask));
  };
  for (u32 i = 0; i < Segment_Count; i++)
    ReadSegmentCache(&m_segment_cache[i]);

  for (u32 i = 0; i < countof(m_msr_registers.raw_regs); i++)
    reader.SafeReadUInt32(&m_msr_registers.raw_regs[i]);

  reader.SafeReadUInt8(&m_cpl);
  reader.SafeReadUInt8(&m_tlb_user_bit);
  reader.SafeReadBool(&m_alignment_check_enabled);
  reader.SafeReadBool(&m_halted);

  reader.SafeReadBool(&m_nmi_state);
  reader.SafeReadBool(&m_irq_state);

#ifdef ENABLE_TLB_EMULATION
  u32 tlb_entry_count;
  if (!reader.SafeReadUInt32(&tlb_entry_count) || tlb_entry_count != Truncate32(TLB_ENTRY_COUNT))
    return false;
  for (u32 user_supervisor = 0; user_supervisor < 2; user_supervisor++)
  {
    for (u32 write_read = 0; write_read < 2; write_read++)
    {
      for (auto& entry : m_tlb_entries[user_supervisor][write_read])
      {
        reader.SafeReadUInt32(&entry.linear_address);
        reader.SafeReadUInt32(&entry.physical_address);
      }
    }
  }
  reader.SafeReadUInt32(&m_tlb_counter_bits);
#endif

#ifdef ENABLE_PREFETCH_EMULATION
  u32 prefetch_queue_size;
  if (!reader.SafeReadUInt32(&prefetch_queue_size) || prefetch_queue_size != PREFETCH_QUEUE_SIZE)
    return false;
  reader.SafeReadBytes(m_prefetch_queue, PREFETCH_QUEUE_SIZE);
  reader.SafeReadUInt32(&m_prefetch_queue_position);
  reader.SafeReadUInt32(&m_prefetch_queue_size);
#endif

  m_effective_address = 0;
  std::memset(&idata, 0, sizeof(idata));
  m_execution_stats = {};

  m_backend->FlushCodeCache();

  return !reader.GetErrorState();
}

bool CPU::SaveState(BinaryWriter& writer)
{
  if (!BaseClass::SaveState(writer))
    return false;

  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteUInt8(static_cast<u8>(m_model));

  writer.WriteInt64(m_pending_cycles);
  writer.WriteInt64(m_execution_downcount);
  writer.WriteInt64(m_tsc_cycles);
  writer.WriteUInt32(m_current_EIP);
  writer.WriteUInt32(m_current_ESP);

  // writer.WriteBytes(&m_registers, sizeof(m_registers));
  for (u32 i = 0; i < Reg32_Count; i++)
    writer.WriteUInt32(m_registers.reg32[i]);
  for (u32 i = 0; i < Segment_Count; i++)
    writer.WriteUInt16(m_registers.segment_selectors[i]);
  writer.WriteUInt16(m_registers.LDTR);
  writer.WriteUInt16(m_registers.TR);

  for (u32 i = 0; i < countof(m_fpu_registers.ST); i++)
  {
    writer.WriteUInt64(m_fpu_registers.ST[i].low);
    writer.WriteUInt16(m_fpu_registers.ST[i].high);
  }
  writer.WriteUInt16(m_fpu_registers.CW.bits);
  writer.WriteUInt16(m_fpu_registers.SW.bits);
  writer.WriteUInt16(m_fpu_registers.TW.bits);

  writer.WriteUInt8(static_cast<u8>(m_current_address_size));
  writer.WriteUInt8(static_cast<u8>(m_current_operand_size));
  writer.WriteUInt8(static_cast<u8>(m_stack_address_size));
  writer.WriteUInt32(m_EIP_mask);

  auto WriteDescriptorTablePointer = [&writer](DescriptorTablePointer* ptr) {
    writer.WriteUInt32(ptr->base_address);
    writer.WriteUInt32(ptr->limit);
  };
  WriteDescriptorTablePointer(&m_idt_location);
  WriteDescriptorTablePointer(&m_gdt_location);
  WriteDescriptorTablePointer(&m_ldt_location);

  writer.WriteUInt32(m_tss_location.base_address);
  writer.WriteUInt32(m_tss_location.limit);
  writer.WriteUInt8(static_cast<u8>(m_tss_location.type));

  auto WriteSegmentCache = [&writer](SegmentCache* ptr) {
    writer.WriteUInt32(ptr->base_address);
    writer.WriteUInt32(ptr->limit_low);
    writer.WriteUInt32(ptr->limit_high);
    writer.WriteUInt8(ptr->access.bits);
    writer.WriteUInt8(static_cast<u8>(ptr->access_mask));
  };
  for (u32 i = 0; i < Segment_Count; i++)
    WriteSegmentCache(&m_segment_cache[i]);

  for (u32 i = 0; i < countof(m_msr_registers.raw_regs); i++)
    writer.WriteUInt32(m_msr_registers.raw_regs[i]);

  writer.WriteUInt8(m_cpl);
  writer.WriteUInt8(m_tlb_user_bit);
  writer.WriteBool(m_alignment_check_enabled);
  writer.WriteBool(m_halted);

  writer.WriteBool(m_nmi_state);
  writer.WriteBool(m_irq_state);

#ifdef ENABLE_TLB_EMULATION
  writer.WriteUInt32(Truncate32(TLB_ENTRY_COUNT));
  for (u32 user_supervisor = 0; user_supervisor < 2; user_supervisor++)
  {
    for (u32 write_read = 0; write_read < 2; write_read++)
    {
      for (const auto& entry : m_tlb_entries[user_supervisor][write_read])
      {
        writer.WriteUInt32(entry.linear_address);
        writer.WriteUInt32(entry.physical_address);
      }
    }
  }
  writer.WriteUInt32(m_tlb_counter_bits);
#endif

#ifdef ENABLE_PREFETCH_EMULATION
  writer.WriteUInt32(PREFETCH_QUEUE_SIZE);
  writer.WriteBytes(m_prefetch_queue, PREFETCH_QUEUE_SIZE);
  writer.WriteUInt32(m_prefetch_queue_position);
  writer.WriteUInt32(m_prefetch_queue_size);
#endif

  return !writer.InErrorState();
}

void CPU::SetIRQState(bool state)
{
  if (state)
    Log_TracePrintf("IRQ line signaled");

  m_irq_state = state;

  if (state && m_halted && m_registers.EFLAGS.IF)
  {
    Log_TracePrintf("Bringing CPU up from halt due to IRQ");
    m_halted = false;
  }
}

void CPU::SignalNMI()
{
  Log_DebugPrintf("NMI line signaled");
  m_nmi_state = true;

  if (m_halted)
  {
    Log_DebugPrintf("Bringing CPU up from halt due to NMI");
    m_halted = false;
  }
}

::DebuggerInterface* CPU::GetDebuggerInterface()
{
  if (m_debugger_interface)
    return m_debugger_interface.get();

  Log_DevPrintf("Debugger enabled");
  m_debugger_interface = std::make_unique<DebuggerInterface>(this, m_system);
  return m_debugger_interface.get();
}

bool CPU::SupportsBackend(CPU::BackendType mode)
{
  return (mode == CPU::BackendType::Interpreter || mode == CPU::BackendType::CachedInterpreter ||
          mode == CPU::BackendType::Recompiler);
}

void CPU::SetBackend(CPU::BackendType mode)
{
  Assert(SupportsBackend(mode));
  if (m_backend_type == mode)
    return;

  m_backend_type = mode;

  // If we're initialized, switch backends now, otherwise wait until we have a system.
  if (m_system)
  {
    m_backend.reset();
    CreateBackend();
  }
}

void CPU::Execute()
{
  m_backend->Execute();
}

void CPU::FlushCodeCache()
{
  m_backend->FlushCodeCache();
}

void CPU::GetExecutionStats(ExecutionStats* stats) const
{
  std::memcpy(stats, &m_execution_stats, sizeof(*stats));
  stats->cycles_executed = m_tsc_cycles + m_pending_cycles;
  stats->num_code_cache_blocks = m_backend->GetCodeBlockCount();
}

void CPU::CreateBackend()
{
  switch (m_backend_type)
  {
    case BackendType::Interpreter:
      m_backend = std::make_unique<InterpreterBackend>(this);
      break;

    case BackendType::CachedInterpreter:
      m_backend = std::make_unique<CachedInterpreterBackend>(this);
      break;

#if defined(Y_CPU_X64)
    case BackendType::Recompiler:
      m_backend = std::make_unique<Recompiler::Backend>(this);
      break;
#endif

    default:
      Log_ErrorPrintf("Unsupported backend type %s, falling back to interpreter.", BackendTypeToString(m_backend_type));
      m_backend_type = BackendType::Interpreter;
      m_backend = std::make_unique<InterpreterBackend>(this);
      break;
  }
}

u8 CPU::FetchInstructionByte()
{
#ifdef ENABLE_PREFETCH_EMULATION
  // It's possible this will still fail if we're at the end of the segment.
  u8 value;
  if ((m_prefetch_queue_size - m_prefetch_queue_position) >= sizeof(u8) || FillPrefetchQueue())
    value = m_prefetch_queue[m_prefetch_queue_position++];
  else
    value = FetchDirectInstructionByte(m_registers.EIP);

  m_registers.EIP = (m_registers.EIP + sizeof(u8)) & m_EIP_mask;
  return value;
#else
  u8 value = FetchDirectInstructionByte(m_registers.EIP);
  m_registers.EIP = (m_registers.EIP + sizeof(u8)) & m_EIP_mask;
  return value;
#endif
}

u16 CPU::FetchInstructionWord()
{
#ifdef ENABLE_PREFETCH_EMULATION
  // It's possible this will still fail if we're at the end of the segment.
  u16 value;
  if ((m_prefetch_queue_size - m_prefetch_queue_position) >= sizeof(u16) || FillPrefetchQueue())
  {
    std::memcpy(&value, &m_prefetch_queue[m_prefetch_queue_position], sizeof(u16));
    m_prefetch_queue_position += sizeof(u16);
  }
  else
  {
    value = FetchDirectInstructionWord(m_registers.EIP);
  }

  m_registers.EIP = (m_registers.EIP + sizeof(u16)) & m_EIP_mask;
  return value;
#else
  u16 value = FetchDirectInstructionWord(m_registers.EIP);
  m_registers.EIP = (m_registers.EIP + sizeof(u16)) & m_EIP_mask;
  return value;
#endif
}

u32 CPU::FetchInstructionDWord()
{
#ifdef ENABLE_PREFETCH_EMULATION
  // It's possible this will still fail if we're at the end of the segment.
  u32 value;
  if ((m_prefetch_queue_size - m_prefetch_queue_position) >= sizeof(u32) || FillPrefetchQueue())
  {
    std::memcpy(&value, &m_prefetch_queue[m_prefetch_queue_position], sizeof(u32));
    m_prefetch_queue_position += sizeof(u32);
  }
  else
  {
    value = FetchDirectInstructionDWord(m_registers.EIP);
  }

  m_registers.EIP = (m_registers.EIP + sizeof(u32)) & m_EIP_mask;
  return value;
#else
  u32 value = FetchDirectInstructionDWord(m_registers.EIP);
  m_registers.EIP = (m_registers.EIP + sizeof(u32)) & m_EIP_mask;
  return value;
#endif
}

u8 CPU::FetchDirectInstructionByte(u32 address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, address);
  CheckSegmentAccess<sizeof(u8), AccessType::Execute>(Segment_CS, address, true);

  PhysicalMemoryAddress physical_address;
  TranslateLinearAddress(&physical_address, linear_address,
                         AddAccessTypeToFlags(AccessType::Execute, AccessFlags::Normal));

  return m_bus->ReadMemoryByte(physical_address);
}

u16 CPU::FetchDirectInstructionWord(u32 address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, address);
  CheckSegmentAccess<sizeof(u16), AccessType::Execute>(Segment_CS, address, true);

  // If it crosses a page, we have to fetch bytes instead.
  if ((linear_address & CPU::PAGE_MASK) != ((linear_address + sizeof(u16) - 1) & CPU::PAGE_MASK))
  {
    u32 mask = (m_current_address_size == AddressSize_16) ? 0xFFFF : 0xFFFFFFFF;
    u8 lsb = FetchDirectInstructionByte(address);
    u8 msb = FetchDirectInstructionByte((address + 1) & mask);
    return ZeroExtend16(lsb) | (ZeroExtend16(msb) << 8);
  }

  PhysicalMemoryAddress physical_address;
  TranslateLinearAddress(&physical_address, linear_address,
                         AddAccessTypeToFlags(AccessType::Execute, AccessFlags::Normal));
  return m_bus->ReadMemoryWord(physical_address);
}

u32 CPU::FetchDirectInstructionDWord(u32 address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, address);
  CheckSegmentAccess<sizeof(u32), AccessType::Execute>(Segment_CS, address, true);

  // If it crosses a page, we have to fetch words instead.
  if ((linear_address & CPU::PAGE_MASK) != ((linear_address + sizeof(u32) - 1) & CPU::PAGE_MASK))
  {
    u32 mask = (m_current_address_size == AddressSize_16) ? 0xFFFF : 0xFFFFFFFF;
    u16 lsb = FetchDirectInstructionWord(address);
    u16 msb = FetchDirectInstructionWord((address + 2) & mask);
    return ZeroExtend32(lsb) | (ZeroExtend32(msb) << 16);
  }

  PhysicalMemoryAddress physical_address;
  TranslateLinearAddress(&physical_address, linear_address,
                         AddAccessTypeToFlags(AccessType::Execute, AccessFlags::Normal));
  return m_bus->ReadMemoryDWord(physical_address);
}

void CPU::PushWord(u16 value)
{
  // TODO: Same optimization here with EIP mask - ESP mask
  LinearMemoryAddress linear_address;
  if (m_stack_address_size == AddressSize_16)
  {
    u16 new_SP = m_registers.SP - sizeof(u16);
    CheckSegmentAccess<sizeof(u16), AccessType::Write>(Segment_SS, ZeroExtend32(new_SP), true);
    linear_address = CalculateLinearAddress(Segment_SS, ZeroExtend32(new_SP));
    m_registers.SP = new_SP;
  }
  else
  {
    u32 new_ESP = m_registers.ESP - sizeof(u16);
    CheckSegmentAccess<sizeof(u16), AccessType::Write>(Segment_SS, new_ESP, true);
    linear_address = CalculateLinearAddress(Segment_SS, new_ESP);
    m_registers.ESP = new_ESP;
  }

  WriteMemoryWord(linear_address, value);
}

void CPU::PushWord32(u16 value)
{
  PhysicalMemoryAddress linear_address;
  if (m_stack_address_size == AddressSize_16)
  {
    u16 new_SP = m_registers.SP - sizeof(u32);
    CheckSegmentAccess<sizeof(u32), AccessType::Write>(Segment_SS, ZeroExtend32(new_SP), true);
    linear_address = CalculateLinearAddress(Segment_SS, ZeroExtend32(new_SP));
    m_registers.SP = new_SP;
  }
  else
  {
    u32 new_ESP = m_registers.ESP - sizeof(u32);
    CheckSegmentAccess<sizeof(u32), AccessType::Write>(Segment_SS, new_ESP, true);
    linear_address = CalculateLinearAddress(Segment_SS, new_ESP);
    m_registers.ESP = new_ESP;
  }

  WriteMemoryWord(linear_address, value);
}

void CPU::PushDWord(u32 value)
{
  PhysicalMemoryAddress linear_address;
  if (m_stack_address_size == AddressSize_16)
  {
    u16 new_SP = m_registers.SP - sizeof(u32);
    CheckSegmentAccess<sizeof(u32), AccessType::Write>(Segment_SS, ZeroExtend32(new_SP), true);
    linear_address = CalculateLinearAddress(Segment_SS, ZeroExtend32(new_SP));
    m_registers.SP = new_SP;
  }
  else
  {
    u32 new_ESP = m_registers.ESP - sizeof(u32);
    CheckSegmentAccess<sizeof(u32), AccessType::Write>(Segment_SS, new_ESP, true);
    linear_address = CalculateLinearAddress(Segment_SS, new_ESP);
    m_registers.ESP = new_ESP;
  }

  WriteMemoryDWord(linear_address, value);
}

u16 CPU::PopWord()
{
  PhysicalMemoryAddress linear_address;
  if (m_stack_address_size == AddressSize_16)
  {
    CheckSegmentAccess<sizeof(u16), AccessType::Read>(Segment_SS, ZeroExtend32(m_registers.SP), true);
    linear_address = CalculateLinearAddress(Segment_SS, m_registers.SP);
    m_registers.SP += sizeof(u16);
  }
  else
  {
    CheckSegmentAccess<sizeof(u16), AccessType::Read>(Segment_SS, m_registers.ESP, true);
    linear_address = CalculateLinearAddress(Segment_SS, m_registers.ESP);
    m_registers.ESP += sizeof(u16);
  }

  return ReadMemoryWord(linear_address);
}

u32 CPU::PopDWord()
{
  PhysicalMemoryAddress linear_address;
  if (m_stack_address_size == AddressSize_16)
  {
    CheckSegmentAccess<sizeof(u32), AccessType::Read>(Segment_SS, ZeroExtend32(m_registers.SP), true);
    linear_address = CalculateLinearAddress(Segment_SS, m_registers.SP);
    m_registers.SP += sizeof(u32);
  }
  else
  {
    CheckSegmentAccess<sizeof(u32), AccessType::Read>(Segment_SS, m_registers.ESP, true);
    linear_address = CalculateLinearAddress(Segment_SS, m_registers.ESP);
    m_registers.ESP += sizeof(u32);
  }

  return ReadMemoryDWord(linear_address);
}
CPU::TemporaryStack::TemporaryStack(CPU* cpu_, u32 ESP_, u16 SS_, u32 base_address_, u32 limit_low_, u32 limit_high_,
                                    AddressSize address_size_)
  : cpu(cpu_), ESP(ESP_), base_address(base_address_), limit_low(limit_low_), limit_high(limit_high_),
    address_size(address_size_), SS(SS_)
{
}

CPU::TemporaryStack::TemporaryStack(CPU* cpu_, u32 ESP_, u16 SS_, const DESCRIPTOR_ENTRY& dentry)
  : cpu(cpu_), ESP(ESP_), base_address(dentry.memory.GetBase()), limit_low(dentry.memory.GetLimitLow()),
    limit_high(dentry.memory.GetLimitHigh()), address_size(dentry.memory.GetAddressSize()), SS(SS_)
{
}

CPU::TemporaryStack::TemporaryStack(CPU* cpu_, u32 ESP_, u16 SS_) : cpu(cpu_), ESP(ESP_), SS(SS_)
{
  SEGMENT_SELECTOR_VALUE selector = {SS};
  DESCRIPTOR_ENTRY dentry = {};
  cpu->ReadDescriptorEntry(&dentry, selector);
  base_address = dentry.memory.GetBase();
  limit_low = dentry.memory.GetLimitLow();
  limit_high = dentry.memory.GetLimitHigh();
  address_size = dentry.memory.GetAddressSize();
}

bool CPU::TemporaryStack::CanPushBytes(u32 num_bytes) const
{
  return (ESP >= num_bytes && (ESP - num_bytes) >= limit_low && (ESP - 1) <= limit_high);
}

bool CPU::TemporaryStack::CanPushWords(u32 num_words) const
{
  return CanPushBytes(num_words * sizeof(u16));
}

bool CPU::TemporaryStack::CanPushDWords(u32 num_dwords) const
{
  return CanPushBytes(num_dwords * sizeof(u32));
}

void CPU::TemporaryStack::PushWord(u16 value)
{
  ESP = (address_size == AddressSize_16) ? ((ESP - sizeof(u16)) & UINT32_C(0xFFFF)) : (ESP - sizeof(u16));
  cpu->WriteMemoryWord(base_address + ESP, value);
}

void CPU::TemporaryStack::PushDWord(u32 value)
{
  ESP = (address_size == AddressSize_16) ? ((ESP - sizeof(u32)) & UINT32_C(0xFFFF)) : (ESP - sizeof(u32));
  cpu->WriteMemoryDWord(base_address + ESP, value);
}

u16 CPU::TemporaryStack::PopWord()
{
  u16 value = cpu->ReadMemoryWord(base_address + ESP);
  ESP = (address_size == AddressSize_16) ? ((ESP + sizeof(u16)) & UINT32_C(0xFFFF)) : (ESP + sizeof(u16));
  return value;
}

u32 CPU::TemporaryStack::PopDWord()
{
  u32 value = cpu->ReadMemoryDWord(base_address + ESP);
  ESP = (address_size == AddressSize_16) ? ((ESP + sizeof(u32)) & UINT32_C(0xFFFF)) : (ESP + sizeof(u32));
  return value;
}

void CPU::TemporaryStack::SwitchTo()
{
  cpu->LoadSegmentRegister(Segment_SS, SS);
  cpu->m_registers.ESP = ESP;
}

void CPU::SetFlags(u32 value)
{
  // Don't clear/set all flags, only those allowed
  u32 MASK =
    Flag_IOPL | Flag_NT | Flag_CF | Flag_PF | Flag_AF | Flag_ZF | Flag_SF | Flag_TF | Flag_IF | Flag_DF | Flag_OF;
  if (m_model >= MODEL_PENTIUM)
  {
    // ID is Pentium+
    MASK |= Flag_VIP | Flag_VIF | Flag_AC | Flag_ID;
  }
  else if (m_model == MODEL_486)
  {
    // AC is 486+
    MASK |= Flag_AC;
  }

  m_registers.EFLAGS.bits = (value & MASK) | (m_registers.EFLAGS.bits & ~MASK);
  UpdateAlignmentCheckMask();
}

void CPU::UpdateAlignmentCheckMask()
{
  m_alignment_check_enabled = InUserMode() && !!(m_registers.CR0 & CR0Bit_AM) && m_registers.EFLAGS.AC;
}

void CPU::SetCPL(u8 cpl)
{
  m_cpl = cpl;
  m_tlb_user_bit = BoolToUInt8(InUserMode());
  UpdateAlignmentCheckMask();
}

void CPU::Halt()
{
  Log_TracePrintf("CPU Halt");
  m_halted = true;

  // Eat all cycles until the next event.
  m_pending_cycles += std::max(m_execution_downcount - m_pending_cycles, CycleCount(0));

  // The downcount must be zeroed, otherwise we'll execute the next instruction. This will result in the downcount going
  // negative once the cycles are committed, but the next event will reset it anyway.
  m_execution_downcount = 0;
}

void CPU::LoadSpecialRegister(Reg32 reg, u32 value)
{
  switch (reg)
  {
    case Reg32_CR0:
    {
      u32 CHANGE_MASK = CR0Bit_PE | CR0Bit_NW | CR0Bit_CD | CR0Bit_EM | CR0Bit_PG;
      u32 old_value = m_registers.CR0;

      // 486 introduced WP bit
      if (m_model >= MODEL_486)
        CHANGE_MASK |= CR0Bit_AM | CR0Bit_WP;

      if (m_registers.CR0 != ((m_registers.CR0 & ~CHANGE_MASK) | (value & CHANGE_MASK)))
        Log_DebugPrintf("CR0 <- 0x%08X", value);

      value &= CHANGE_MASK;

      if ((value & (CR0Bit_PE | CR0Bit_PG)) != (m_registers.CR0 & (CR0Bit_PE | CR0Bit_PG)))
      {
        Log_DebugPrintf("Switching to %s mode%s", ((value & CR0Bit_PE) != 0) ? "protected" : "real",
                        ((value & CR0Bit_PG) != 0) ? " (paging)" : "");
      }

      if ((value & CR0Bit_CD) != (m_registers.CR0 & CR0Bit_CD))
        Log_ErrorPrintf("CPU read cache is now %s", ((value & CR0Bit_CD) != 0) ? "disabled" : "enabled");
      if ((value & CR0Bit_NW) != (m_registers.CR0 & CR0Bit_NW))
        Log_ErrorPrintf("CPU cache is now %s", ((value & CR0Bit_NW) != 0) ? "write-back" : "write-through");

      // We must flush the TLB when WP changes, because it changes the cached access masks.
      u32 new_value = (m_registers.CR0 & ~CHANGE_MASK) | value;
      if (((m_registers.CR0 & CR0Bit_WP) != (new_value & CR0Bit_WP)) ||
          (!(m_registers.CR0 & CR0Bit_PG) && (new_value & CR0Bit_PG)))
      {
        InvalidateAllTLBEntries();
      }

      m_registers.CR0 = new_value;
      UpdateAlignmentCheckMask();

      m_backend->OnControlRegisterLoaded(Reg32_CR0, old_value, m_registers.CR0);
    }
    break;

    case Reg32_CR2:
    {
      // Page fault linear address
      if (m_registers.CR2 != value)
        Log_DebugPrintf("CR2 <- 0x%08X", value);

      u32 old_value = m_registers.CR2;
      m_registers.CR2 = value;
      m_backend->OnControlRegisterLoaded(Reg32_CR2, old_value, value);
    }
    break;

    case Reg32_CR3:
    {
      if (m_registers.CR3 != value)
        Log_DebugPrintf("CR3 <- 0x%08X", value);

      u32 old_value = m_registers.CR3;
      m_registers.CR3 = value;
      InvalidateAllTLBEntries();
      FlushPrefetchQueue();
      m_backend->OnControlRegisterLoaded(Reg32_CR3, old_value, value);
    }
    break;

    case Reg32_CR4:
    {
      // TODO: Test for invalid bits on 386/486.
      if (m_registers.CR4.bits != value)
        Log_DebugPrintf("CR4 <- 0x%08X", value);

      u32 old_value = m_registers.CR4.bits;
      m_registers.CR4.bits = value;
      m_backend->OnControlRegisterLoaded(Reg32_CR4, old_value, value);
    }
    break;

    case Reg32_DR0:
    case Reg32_DR1:
    case Reg32_DR2:
    case Reg32_DR3:
    case Reg32_DR4:
    case Reg32_DR5:
    case Reg32_DR6:
    case Reg32_DR7:
    {
      if (m_registers.reg32[reg] != value)
        Log_DebugPrintf("DR%u <- 0x%08X", u32(reg - Reg32_DR0), value);

      m_registers.reg32[reg] = value;
    }
    break;

    case Reg32_TR3:
    case Reg32_TR4:
    case Reg32_TR5:
    case Reg32_TR6:
    case Reg32_TR7:
    {
      if (m_registers.reg32[reg] != value)
        Log_DebugPrintf("TR%u <- 0x%08X", u32(reg - Reg32_TR3), value);

      m_registers.reg32[reg] = value;
    }
    break;

    default:
      UnreachableCode();
      break;
  }
}

bool CPU::LookupPageTable(PhysicalMemoryAddress* out_physical_address, LinearMemoryAddress linear_address,
                          AccessFlags flags)
{
  // TODO: Large (4MB) pages
  const bool user_mode = (InUserMode() && !HasAccessFlagBit(flags, AccessFlags::UseSupervisorPrivileges));

  // Obtain the address of the page directory. Bits 22-31 index the page directory.
  LinearMemoryAddress dir_base_address = (m_registers.CR3 & 0xFFFFF000);
  LinearMemoryAddress dir_entry_address =
    dir_base_address + (((linear_address >> 22) & 0x3FF) * sizeof(PAGE_DIRECTORY_ENTRY));

  // Read the page directory entry.
  PAGE_DIRECTORY_ENTRY directory_entry;
  directory_entry.bits = m_bus->ReadMemoryDWord(dir_entry_address);
  AddMemoryCycle();

  // Check for present bits.
  // TODO: Permissions
  if (!directory_entry.present)
  {
    // Page not present.
    if (!HasAccessFlagBit(flags, AccessFlags::NoPageFaults))
      RaisePageFault(linear_address, flags, false);
    return false;
  }

  // Obtain the address of the page table. Address in the directory is 4KB aligned. Bits 12-21 index the page table.
  LinearMemoryAddress table_base_address = (directory_entry.page_table_address << 12);
  LinearMemoryAddress table_entry_address =
    table_base_address + (((linear_address >> 12) & 0x3FF) * sizeof(PAGE_TABLE_ENTRY));

  // Read the page table entry.
  PAGE_TABLE_ENTRY table_entry;
  table_entry.bits = m_bus->ReadMemoryDWord(table_entry_address);
  AddMemoryCycle();

  // Check for present bits.
  if (!table_entry.present)
  {
    // Page not present.
    if (!HasAccessFlagBit(flags, AccessFlags::NoPageFaults))
      RaisePageFault(linear_address, flags, false);
    return false;
  }

  // Check access, requires both directory and page access
  // Permission checks only apply in usermode, except if WP bit of CR0 is set
  const bool do_access_check =
    (!HasAccessFlagBit(flags, AccessFlags::NoPageProtectionCheck) && ((m_registers.CR0 & CR0Bit_WP) || user_mode));
  if (do_access_check)
  {
    // Permissions for directory
    // U bit set implies userspace can access it
    // R bit implies userspace can write to it
    u8 directory_permissions = (0x05 << 3);                               // supervisor=read,execute
    directory_permissions |= (directory_entry.bits << 3) & 0x10;          // supervisor=write from R/W bit
    directory_permissions |= (directory_entry.bits >> 2) & 0x01;          // user=read from U/S bit
    directory_permissions |= (directory_entry.bits) & 0x04;               // user=execute from U/S bit
    directory_permissions |= (directory_entry.bits) & 0x02;               // user=write from R/W bit
    directory_permissions &= 0x3D | ((directory_entry.bits >> 1) & 0x02); // user=write from U/S bit

    // Check for table permissions
    // U bit set implies userspace can access it
    // R bit implies userspace can write to it
    u8 table_permissions = (0x05 << 3);                           // supervisor=read,execute
    table_permissions |= (table_entry.bits << 3) & 0x10;          // supervisor=write from R/W bit
    table_permissions |= (table_entry.bits >> 2) & 0x01;          // user=read from U/S bit
    table_permissions |= (table_entry.bits) & 0x04;               // user=execute from U/S bit
    table_permissions |= (table_entry.bits) & 0x02;               // user=write from R/W bit
    table_permissions &= 0x3D | ((table_entry.bits >> 1) & 0x02); // user=write from U/S bit

    u8 access_mask = (1 << static_cast<u8>(GetAccessTypeFromFlags(flags))) << (user_mode ? 0 : 3);

    if ((access_mask & directory_permissions & table_permissions) == 0)
    {
      if (!HasAccessFlagBit(flags, AccessFlags::NoPageFaults))
        RaisePageFault(linear_address, flags, true);
      return false;
    }
  }

  // Calculate the physical address from the page table entry. Pages are 4KB aligned.
  PhysicalMemoryAddress page_base_address = (table_entry.physical_address << PAGE_SHIFT);
  PhysicalMemoryAddress translated_address = page_base_address + (linear_address & 0xFFF);

  // Updating of accessed/dirty bits is only done with access checks are enabled (=> normal usage)
  if (!HasAccessFlagBit(flags, AccessFlags::NoTLBUpdate))
  {
    // Update accessed bits on directory and table entries
    if (!directory_entry.accessed)
    {
      directory_entry.accessed = true;
      m_bus->WriteMemoryDWord(dir_entry_address, directory_entry.bits);
    }
    if (!table_entry.accessed)
    {
      table_entry.accessed = true;
      m_bus->WriteMemoryDWord(table_entry_address, table_entry.bits);
    }

    // Update dirty bit on table entry
    if (GetAccessTypeFromFlags(flags) == AccessType::Write && !table_entry.dirty)
    {
      table_entry.dirty = true;
      m_bus->WriteMemoryDWord(table_entry_address, table_entry.bits);
    }

#ifdef ENABLE_TLB_EMULATION
    const size_t tlb_index = GetTLBEntryIndex(linear_address);
    const u8 tlb_user_bit = BoolToUInt8(user_mode);
    const u8 tlb_type = static_cast<u8>(GetAccessTypeFromFlags(flags));
    TLBEntry& tlb_entry = m_tlb_entries[tlb_user_bit][tlb_type][tlb_index];
    tlb_entry.linear_address = (linear_address & PAGE_MASK) | m_tlb_counter_bits;
    tlb_entry.physical_address = page_base_address;
#endif
  }

  *out_physical_address = translated_address;
  return true;
}

void CPU::RaisePageFault(LinearMemoryAddress linear_address, AccessFlags flags, bool page_present)
{
  const bool is_write = (GetAccessTypeFromFlags(flags) == AccessType::Write);
  const bool user_mode = (InUserMode() && !HasAccessFlagBit(flags, AccessFlags::UseSupervisorPrivileges));
  Log_DebugPrintf("Page fault at linear address 0x%08X: %s,%s,%s", linear_address,
                  page_present ? "Present" : "Not Present", is_write ? "Write" : "Read",
                  user_mode ? "User Mode" : "Supervisor Mode");

  // Determine bits of error code
  const u32 error_code = (BoolToUInt8(page_present) << 0) | // P
                         (BoolToUInt8(is_write) << 1) |     // W/R
                         (BoolToUInt8(user_mode) << 2);     // U/S

  // Update CR2 with the linear address that triggered the fault
  m_registers.CR2 = linear_address;
  RaiseException(Interrupt_PageFault, error_code);
}

u8 CPU::ReadMemoryByte(CPU* cpu, LinearMemoryAddress address)
{
  cpu->AddMemoryCycle();

  PhysicalMemoryAddress physical_address;
  cpu->TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Read, AccessFlags::Normal));

  // TODO: Optimize Bus
  return cpu->m_bus->ReadMemoryByte(physical_address);
}

u16 CPU::ReadMemoryWord(CPU* cpu, LinearMemoryAddress address)
{
  cpu->AddMemoryCycle();

  // Unaligned access?
  if ((address & (sizeof(u16) - 1)) != 0)
  {
    // Alignment access exception.
    if (cpu->m_alignment_check_enabled)
    {
      cpu->RaiseException(Interrupt_AlignmentCheck, 0);
      return 0;
    }

    // If the address falls within the same page we can still skip doing byte reads.
    if ((address & PAGE_MASK) != ((address + sizeof(u16) - 1) & PAGE_MASK))
    {
      // Fall back to byte reads.
      u8 b0 = ReadMemoryByte(cpu, address + 0);
      u8 b1 = ReadMemoryByte(cpu, address + 1);
      return ZeroExtend16(b0) | (ZeroExtend16(b1) << 8);
    }
  }

  PhysicalMemoryAddress physical_address;
  cpu->TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Read, AccessFlags::Normal));
  return cpu->m_bus->ReadMemoryWord(physical_address);
}

u32 CPU::ReadMemoryDWord(CPU* cpu, LinearMemoryAddress address)
{
  cpu->AddMemoryCycle();

  // Unaligned access?
  if ((address & (sizeof(u32) - 1)) != 0)
  {
    // Alignment access exception.
    if (cpu->m_alignment_check_enabled)
    {
      cpu->RaiseException(Interrupt_AlignmentCheck, 0);
      return 0;
    }

    // If the address falls within the same page we can still skip doing byte reads.
    if ((address & PAGE_MASK) != ((address + sizeof(u32) - 1) & PAGE_MASK))
    {
      // Fallback to word reads when it's split across pages.
      u16 w0 = ReadMemoryWord(cpu, address + 0);
      u16 w1 = ReadMemoryWord(cpu, address + 2);
      return ZeroExtend32(w0) | (ZeroExtend32(w1) << 16);
    }
  }

  PhysicalMemoryAddress physical_address;
  cpu->TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Read, AccessFlags::Normal));
  return cpu->m_bus->ReadMemoryDWord(physical_address);
}

void CPU::WriteMemoryByte(CPU* cpu, LinearMemoryAddress address, u8 value)
{
  cpu->AddMemoryCycle();
  PhysicalMemoryAddress physical_address;
  cpu->TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Write, AccessFlags::Normal));
  cpu->m_bus->WriteMemoryByte(physical_address, value);
}

void CPU::WriteMemoryWord(CPU* cpu, LinearMemoryAddress address, u16 value)
{
  cpu->AddMemoryCycle();

  // Unaligned access?
  if ((address & (sizeof(u16) - 1)) != 0)
  {
    // Alignment access exception.
    if (cpu->m_alignment_check_enabled)
    {
      cpu->RaiseException(Interrupt_AlignmentCheck, 0);
      return;
    }

    // If the address falls within the same page we can still skip doing byte reads.
    if ((address & PAGE_MASK) != ((address + sizeof(u16) - 1) & PAGE_MASK))
    {
      // Slowest path here.
      WriteMemoryByte(cpu, (address + 0), Truncate8(value));
      WriteMemoryByte(cpu, (address + 1), Truncate8(value >> 8));
      return;
    }
  }

  PhysicalMemoryAddress physical_address;
  cpu->TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Write, AccessFlags::Normal));
  cpu->m_bus->WriteMemoryWord(physical_address, value);
}

void CPU::WriteMemoryDWord(CPU* cpu, LinearMemoryAddress address, u32 value)
{
  cpu->AddMemoryCycle();

  // Unaligned access?
  if ((address & (sizeof(u32) - 1)) != 0)
  {
    // Alignment access exception.
    if (cpu->m_alignment_check_enabled)
    {
      cpu->RaiseException(Interrupt_AlignmentCheck, 0);
      return;
    }

    // If the address falls within the same page we can still skip doing byte reads.
    if ((address & PAGE_MASK) != ((address + sizeof(u32) - 1) & PAGE_MASK))
    {
      // Fallback to word writes when it's split across pages.
      WriteMemoryWord(cpu, (address + 0), Truncate16(value));
      WriteMemoryWord(cpu, (address + 2), Truncate16(value >> 16));
      return;
    }
  }

  PhysicalMemoryAddress physical_address;
  cpu->TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Write, AccessFlags::Normal));
  cpu->m_bus->WriteMemoryDWord(physical_address, value);
}

u8 CPU::ReadSegmentMemoryByte(Segment segment, VirtualMemoryAddress address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(u8), AccessType::Read>(segment, address, true);
  return ReadMemoryByte(linear_address);
}

u16 CPU::ReadSegmentMemoryWord(Segment segment, VirtualMemoryAddress address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(u16), AccessType::Read>(segment, address, true);
  return ReadMemoryWord(linear_address);
}

u32 CPU::ReadSegmentMemoryDWord(Segment segment, VirtualMemoryAddress address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(u32), AccessType::Read>(segment, address, true);
  return ReadMemoryDWord(linear_address);
}

void CPU::WriteSegmentMemoryByte(Segment segment, VirtualMemoryAddress address, u8 value)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(u8), AccessType::Write>(segment, address, true);
  WriteMemoryByte(linear_address, value);
}

void CPU::WriteSegmentMemoryWord(Segment segment, VirtualMemoryAddress address, u16 value)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(u16), AccessType::Write>(segment, address, true);
  WriteMemoryWord(linear_address, value);
}

void CPU::WriteSegmentMemoryDWord(Segment segment, VirtualMemoryAddress address, u32 value)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(u32), AccessType::Write>(segment, address, true);
  WriteMemoryDWord(linear_address, value);
}

bool CPU::SafeReadMemoryByte(LinearMemoryAddress address, u8* value, AccessFlags access_flags)
{
  PhysicalMemoryAddress physical_address;
  if (!TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Read, access_flags)))
  {
    *value = UINT8_C(0xFF);
    return false;
  }

  return m_bus->CheckedReadMemoryByte(physical_address, value);
}

bool CPU::SafeReadMemoryWord(LinearMemoryAddress address, u16* value, AccessFlags access_flags)
{
  PhysicalMemoryAddress physical_address;

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(u16) - 1) & PAGE_MASK))
  {
    if (!TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Read, access_flags)))
    {
      *value = UINT16_C(0xFFFF);
      return false;
    }

    return m_bus->CheckedReadMemoryWord(physical_address, value);
  }

  // Fall back to byte reads.
  u8 b0, b1;
  bool result = SafeReadMemoryByte(address + 0, &b0, access_flags) & SafeReadMemoryByte(address + 1, &b1, access_flags);

  *value = ZeroExtend16(b0) | (ZeroExtend16(b1) << 8);
  return result;
}

bool CPU::SafeReadMemoryDWord(LinearMemoryAddress address, u32* value, AccessFlags access_flags)
{
  PhysicalMemoryAddress physical_address;

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(u32) - 1) & PAGE_MASK))
  {
    if (!TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Read, access_flags)))
      return false;

    return m_bus->CheckedReadMemoryDWord(physical_address, value);
  }

  // Fallback to word reads when it's split across pages.
  u16 w0 = 0, w1 = 0;
  bool result = SafeReadMemoryWord(address + 0, &w0, access_flags) & SafeReadMemoryWord(address + 2, &w1, access_flags);

  *value = ZeroExtend32(w0) | (ZeroExtend32(w1) << 16);
  return result;
}

bool CPU::SafeWriteMemoryByte(VirtualMemoryAddress address, u8 value, AccessFlags access_flags)
{
  PhysicalMemoryAddress physical_address;
  if (!TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Write, access_flags)))
    return false;

  return m_bus->CheckedWriteMemoryByte(physical_address, value);
}

bool CPU::SafeWriteMemoryWord(VirtualMemoryAddress address, u16 value, AccessFlags access_flags)
{
  PhysicalMemoryAddress physical_address;

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(u16) - 1) & PAGE_MASK))
  {
    if (!TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Write, access_flags)))
      return false;

    return m_bus->CheckedWriteMemoryWord(physical_address, value);
  }

  // Slowest path here.
  return SafeWriteMemoryByte((address + 0), Truncate8(value), access_flags) &
         SafeWriteMemoryByte((address + 1), Truncate8(value >> 8), access_flags);
}

bool CPU::SafeWriteMemoryDWord(VirtualMemoryAddress address, u32 value, AccessFlags access_flags)
{
  PhysicalMemoryAddress physical_address;

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(u32) - 1) & PAGE_MASK))
  {
    if (!TranslateLinearAddress(&physical_address, address, AddAccessTypeToFlags(AccessType::Write, access_flags)))
      return false;

    return m_bus->CheckedWriteMemoryDWord(physical_address, value);
  }

  // Fallback to word writes when it's split across pages.
  return SafeWriteMemoryWord((address + 0), Truncate16(value), access_flags) &
         SafeWriteMemoryWord((address + 2), Truncate16(value >> 16), access_flags);
}

void CPU::PrintCurrentStateAndInstruction(u32 EIP, const char* prefix_message /* = nullptr */)
{
  if (prefix_message)
  {
    std::fprintf(stdout, "%s at EIP = %04X:%08Xh (0x%08X)\n", prefix_message, m_registers.CS, EIP,
                 CalculateLinearAddress(Segment_CS, EIP));
  }

  //#define COMMON_LOGGING_FORMAT 1

#ifndef COMMON_LOGGING_FORMAT
#if 1
  std::fprintf(stdout, "EAX=%08X EBX=%08X ECX=%08X EDX=%08X ESI=%08X EDI=%08X ESP=%08X EBP=%08X\n", m_registers.EAX,
               m_registers.EBX, m_registers.ECX, m_registers.EDX, m_registers.ESI, m_registers.EDI, m_registers.ESP,
               m_registers.EBP);
  std::fprintf(stdout,
               "EFLAGS=%08X ES=%04X SS=%04X DS=%04X FS=%04X GS=%04X CR0=%08X CR2=%08X CR3=%08X TSC=%" PRIX64 "\n",
               m_registers.EFLAGS.bits, ZeroExtend32(m_registers.ES), ZeroExtend32(m_registers.SS),
               ZeroExtend32(m_registers.DS), ZeroExtend32(m_registers.FS), ZeroExtend32(m_registers.GS),
               m_registers.CR0, m_registers.CR2, m_registers.CR3, ReadTSC());
#endif
#endif

  u32 fetch_EIP = EIP;
  auto fetchb = [this, &fetch_EIP](u8* val) {
    if (!SafeReadMemoryByte(CalculateLinearAddress(Segment_CS, fetch_EIP), val, AccessFlags::Debugger))
      return false;
    fetch_EIP = (fetch_EIP + sizeof(u8)) & m_EIP_mask;
    return true;
  };
  auto fetchw = [this, &fetch_EIP](u16* val) {
    if (!SafeReadMemoryWord(CalculateLinearAddress(Segment_CS, fetch_EIP), val, AccessFlags::Debugger))
      return false;
    fetch_EIP = (fetch_EIP + sizeof(u16)) & m_EIP_mask;
    return true;
  };
  auto fetchd = [this, &fetch_EIP](u32* val) {
    if (!SafeReadMemoryDWord(CalculateLinearAddress(Segment_CS, fetch_EIP), val, AccessFlags::Debugger))
      return false;
    fetch_EIP = (fetch_EIP + sizeof(u32)) & m_EIP_mask;
    return true;
  };

  // Try to decode the instruction first.
  Instruction instruction;
  bool instruction_valid = Decoder::DecodeInstruction(&instruction, m_current_address_size, m_current_operand_size,
                                                      fetch_EIP, fetchb, fetchw, fetchd);

  // TODO: Handle 16 vs 32-bit operating mode clamp on address
  SmallString hex_string;
  u32 instruction_length = instruction_valid ? instruction.length : 16;
  for (u32 i = 0; i < instruction_length; i++)
  {
    u8 value = 0;
    if (!SafeReadMemoryByte(CalculateLinearAddress(Segment_CS, EIP + i), &value, AccessFlags::Debugger))
    {
      hex_string.AppendFormattedString(" <memory read failed at 0x%08X>", CalculateLinearAddress(Segment_CS, EIP + i));
      i = instruction_length;
      break;
    }

    hex_string.AppendFormattedString("%02X ", ZeroExtend32(value));
  }

  if (instruction_valid)
  {
    SmallString instr_string;
    Decoder::DisassembleToString(&instruction, &instr_string);

#ifndef COMMON_LOGGING_FORMAT
    for (u32 i = instruction_length; i < 10; i++)
      hex_string.AppendString("   ");

    LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, EIP);
    std::fprintf(stdout, "%04X:%08Xh (0x%08X) | %s | %s\n", ZeroExtend32(m_registers.CS), EIP, linear_address,
                 hex_string.GetCharArray(), instr_string.GetCharArray());
#else
    std::fprintf(stdout, "%04x:%08x %s%s\n", ZeroExtend32(m_registers.CS), EIP, hex_string.GetCharArray(),
                 instr_string.GetCharArray());
#endif
  }
  else
  {
#ifndef COMMON_LOGGING_FORMAT
    LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, EIP);
    std::fprintf(stdout, "%04X:%08Xh (0x%08X) Decoding failed, bytes at failure point: %s\n",
                 ZeroExtend32(m_registers.CS), EIP, linear_address, hex_string.GetCharArray());
#else
    std::fprintf(stdout, "%04x:%08x %s??????\n", ZeroExtend32(m_registers.CS), EIP, hex_string.GetCharArray());
#endif
  }

#ifdef COMMON_LOGGING_FORMAT
  std::fprintf(stdout, "Registers:\n");
  std::fprintf(stdout, "EAX: %08x EBX: %08x ECX: %08x EDX: %08x\n", m_registers.EAX, m_registers.EBX, m_registers.ECX,
               m_registers.EDX);
  std::fprintf(stdout, "ESP: %08x EBP: %08x ESI: %08x EDI: %08x\n", m_registers.ESP, m_registers.EBP, m_registers.ESI,
               m_registers.EDI);
  std::fprintf(stdout, "CS: %04x DS: %04x ES: %04x FS: %04x GS: %04x SS: %04x TR: %04x LDTR: %04x\n",
               ZeroExtend32(m_registers.CS), ZeroExtend32(m_registers.DS), ZeroExtend32(m_registers.ES),
               ZeroExtend32(m_registers.FS), ZeroExtend32(m_registers.GS), ZeroExtend32(m_registers.SS),
               ZeroExtend32(m_registers.TR), ZeroExtend32(m_registers.LDTR));
  std::fprintf(stdout, "EIP: %08x EFLAGS: %08x\n", EIP, m_registers.EFLAGS.bits);
  std::fprintf(stdout, "CR0: %08x CR2: %08x CR3: %08x\n", m_registers.CR0, m_registers.CR2, m_registers.CR3);
  std::fprintf(stdout, "DR0: %08x DR1: %08x DR2: %08x DR3: %08x\n", m_registers.DR0, m_registers.DR1, m_registers.DR2,
               m_registers.DR3);
  std::fprintf(stdout, "DR6: %08x DR7: %08x\n", m_registers.DR6, m_registers.DR7);
  std::fprintf(stdout, "GDTR: %08x%08x IDTR: %08x%08x\n", m_gdt_location.base_address, m_gdt_location.limit,
               m_idt_location.base_address, m_idt_location.limit);
  std::fprintf(stdout, "FLAGSINFO: %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
               ((m_registers.EFLAGS.bits >> 31) & 1) + '0', ((m_registers.EFLAGS.bits >> 30) & 1) + '0',
               ((m_registers.EFLAGS.bits >> 29) & 1) + '0', ((m_registers.EFLAGS.bits >> 28) & 1) + '0',
               ((m_registers.EFLAGS.bits >> 27) & 1) + '0', ((m_registers.EFLAGS.bits >> 26) & 1) + '0',
               ((m_registers.EFLAGS.bits >> 25) & 1) + '0', ((m_registers.EFLAGS.bits >> 24) & 1) + '0',
               ((m_registers.EFLAGS.bits >> 23) & 1) + '0', ((m_registers.EFLAGS.bits >> 22) & 1) + '0',
               ((m_registers.EFLAGS.bits >> 21) & 1) + '0', ((m_registers.EFLAGS.bits >> 20) & 1) + '0',
               ((m_registers.EFLAGS.bits >> 19) & 1) + '0', ((m_registers.EFLAGS.bits >> 18) & 1) + '0',
               ((m_registers.EFLAGS.bits >> 17) & 1) ? 'V' : 'v', ((m_registers.EFLAGS.bits >> 16) & 1) ? 'R' : 'r',
               ((m_registers.EFLAGS.bits >> 15) & 1) + '0', ((m_registers.EFLAGS.bits >> 14) & 1) ? 'N' : 'n',
               ((m_registers.EFLAGS.bits >> 13) & 1) + '0', ((m_registers.EFLAGS.bits >> 12) & 1) + '0',
               ((m_registers.EFLAGS.bits >> 11) & 1) ? 'O' : 'o', ((m_registers.EFLAGS.bits >> 10) & 1) ? 'D' : 'd',
               ((m_registers.EFLAGS.bits >> 9) & 1) ? 'I' : 'i', ((m_registers.EFLAGS.bits >> 8) & 1) ? 'T' : 't',
               ((m_registers.EFLAGS.bits >> 7) & 1) ? 'S' : 's', ((m_registers.EFLAGS.bits >> 6) & 1) ? 'Z' : 'z',
               ((m_registers.EFLAGS.bits >> 5) & 1) + '0', ((m_registers.EFLAGS.bits >> 4) & 1) ? 'A' : 'a',
               ((m_registers.EFLAGS.bits >> 3) & 1) + '0', ((m_registers.EFLAGS.bits >> 2) & 1) ? 'P' : 'p',
               ((m_registers.EFLAGS.bits >> 1) & 1) + '0', ((m_registers.EFLAGS.bits) & 1) ? 'C' : 'c');
#endif
}

bool CPU::ReadDescriptorEntry(DESCRIPTOR_ENTRY* entry, const DescriptorTablePointer& table, u32 index)
{
  u32 offset = index * 8;
  if ((offset + 7) > table.limit)
    return false;

  // TODO: Should this use supervisor privileges since it's reading the GDT?
  SafeReadMemoryDWord(table.base_address + offset + 0, &entry->bits0, AccessFlags::UseSupervisorPrivileges);
  SafeReadMemoryDWord(table.base_address + offset + 4, &entry->bits1, AccessFlags::UseSupervisorPrivileges);
  return true;
}

bool CPU::ReadDescriptorEntry(DESCRIPTOR_ENTRY* entry, const SEGMENT_SELECTOR_VALUE& selector)
{
  return ReadDescriptorEntry(entry, selector.ti ? m_ldt_location : m_gdt_location, selector.index);
}

bool CPU::WriteDescriptorEntry(const DESCRIPTOR_ENTRY& entry, const DescriptorTablePointer& table, u32 index)
{
  u32 offset = index * 8;
  if ((offset + 7) > table.limit)
    return false;

  // TODO: Should this use supervisor privileges since it's reading the GDT?
  LinearMemoryAddress descriptor_address = table.base_address + offset + 0;
  SafeWriteMemoryDWord(descriptor_address + 0, entry.bits0, AccessFlags::UseSupervisorPrivileges);
  SafeWriteMemoryDWord(descriptor_address + 4, entry.bits1, AccessFlags::UseSupervisorPrivileges);
  return true;
}

bool CPU::WriteDescriptorEntry(const DESCRIPTOR_ENTRY& entry, const SEGMENT_SELECTOR_VALUE& selector)
{
  return WriteDescriptorEntry(entry, selector.ti ? m_ldt_location : m_gdt_location, selector.index);
}

void CPU::SetDescriptorAccessedBit(DESCRIPTOR_ENTRY& entry, const DescriptorTablePointer& table, u32 index)
{
  DebugAssert(entry.is_memory_descriptor);

  // 486+ does not write to memory if the accessed bit is already set. 386 always does.
  if (m_model >= MODEL_486 && entry.memory.access.accessed)
    return;

  entry.memory.access.accessed = true;
  SafeWriteMemoryByte(table.base_address + index * 8 + 5, Truncate8(entry.bits1 >> 8),
                      AccessFlags::UseSupervisorPrivileges);
}

void CPU::SetDescriptorAccessedBit(DESCRIPTOR_ENTRY& entry, const SEGMENT_SELECTOR_VALUE& selector)
{
  SetDescriptorAccessedBit(entry, selector.ti ? m_ldt_location : m_gdt_location, selector.index);
}

bool CPU::CheckTargetCodeSegment(u16 raw_selector, u8 check_rpl, u8 check_cpl, bool raise_exceptions)
{
  // Check for null selector
  SEGMENT_SELECTOR_VALUE selector = {raw_selector};
  if (selector.index == 0)
  {
    if (raise_exceptions)
      RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return false;
  }

  // Read/check descriptor
  DESCRIPTOR_ENTRY descriptor;
  if (!ReadDescriptorEntry(&descriptor, selector.ti ? m_ldt_location : m_gdt_location, selector.index))
  {
    if (raise_exceptions)
      RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return false;
  }

  // Check for non-code segments
  if (!descriptor.IsCodeSegment())
  {
    if (raise_exceptions)
      RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return false;
  }

  if (!descriptor.memory.access.code_conforming)
  {
    // Non-conforming code segments must have DPL=CPL
    if (descriptor.dpl != check_cpl)
    {
      if (raise_exceptions)
        RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
      return false;
    }

    // RPL must be <= CPL
    if (check_rpl > check_cpl)
    {
      if (raise_exceptions)
        RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
      return false;
    }
  }
  else
  {
    // Conforming code segment must have DPL <= CPL
    if (descriptor.dpl > check_cpl)
    {
      if (raise_exceptions)
        RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
      return false;
    }
  }

  // Segment must be present
  // TODO: Is the order important (GPF before NP)?
  if (!descriptor.IsPresent())
  {
    if (raise_exceptions)
      RaiseException(Interrupt_SegmentNotPresent, selector.GetExceptionErrorCode(false));
    return false;
  }

  return true;
}

void CPU::LoadGlobalDescriptorTable(LinearMemoryAddress table_base_address, u32 table_limit)
{
  m_gdt_location.base_address = table_base_address;
  m_gdt_location.limit = table_limit;

  Log_DebugPrintf("Load GDT: Base 0x%08X limit 0x%04X", table_base_address, table_limit);
}

void CPU::LoadInterruptDescriptorTable(LinearMemoryAddress table_base_address, u32 table_limit)
{
  m_idt_location.base_address = table_base_address;
  m_idt_location.limit = table_limit;

  Log_DebugPrintf("Load IDT: Base 0x%08X limit 0x%04X", table_base_address, table_limit);
}

void CPU::LoadSegmentRegister(Segment segment, u16 value)
{
  static const char* segment_names[Segment_Count] = {"ES", "CS", "SS", "DS", "FS", "GS"};
  SegmentCache* segment_cache = &m_segment_cache[segment];

  // In real mode, base is 0x10 * value, with the limit at 64KiB
  if (InRealMode() || InVirtual8086Mode())
  {
    // The limit can be modified in protected mode by loading a descriptor.
    // When the register is reloaded in real mode, this limit must be preserved.
    // TODO: What about stack address size?
    m_registers.segment_selectors[segment] = value;

    segment_cache->base_address = PhysicalMemoryAddress(value) * 0x10;
    if (InRealMode())
    {
      // Real mode uses DPL=0, but maintains validity and limits of existing segment
      segment_cache->access.dpl = 0;

      // Only CS has the limits/access reset.
      if (segment == Segment_CS)
      {
        segment_cache->access.present = true;
        segment_cache->access.code_confirming = true;
        segment_cache->access.code_readable = true;
        segment_cache->access_mask = AccessTypeMask::All;
        SetCPL(0);
      }
    }
    else
    {
      // V8086 mode uses DPL=3, and makes the segment valid
      segment_cache->access.dpl = 3;
      segment_cache->access.present = true;
      if (segment == Segment_CS)
      {
        segment_cache->access.is_code = true;
        segment_cache->access.code_readable = true;
        segment_cache->access.code_confirming = true;
        SetCPL(3);
      }
      else
      {
        segment_cache->access.is_code = false;
        segment_cache->access.data_writable = true;
        segment_cache->access.data_expand_down = false;
      }
      segment_cache->limit_low = 0x0000;
      segment_cache->limit_high = 0xFFFF;
      segment_cache->access_mask = AccessTypeMask::All;
    }

    if (segment == Segment_CS)
    {
      m_current_address_size = AddressSize_16;
      m_current_operand_size = OperandSize_16;
      FlushPrefetchQueue();
    }
    else if (segment == Segment_SS)
    {
      m_stack_address_size = AddressSize_16;
    }

    return;
  }

  // Handle null descriptor separately.
  SEGMENT_SELECTOR_VALUE selector = {value};
  if (selector.index == 0 && !selector.ti)
  {
    // SS can't be a null selector.
    if (segment == Segment_SS)
    {
      RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    segment_cache->base_address = 0;
    segment_cache->limit_low = 0;
    segment_cache->limit_high = 0;
    segment_cache->access.bits = 0;
    segment_cache->access.dpl = 0;
    segment_cache->access_mask = AccessTypeMask::None;
    m_registers.segment_selectors[segment] = value;
    Log_TracePrintf("Loaded null selector for %s", segment_names[segment]);
    return;
  }

  // Read descriptor entry. If this fails, it's because it's outside the limit.
  DESCRIPTOR_ENTRY descriptor;
  if (!ReadDescriptorEntry(&descriptor, selector))
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return;
  }

  // SS has to be handled separately due to stack fault exception.
  if (segment == Segment_SS)
  {
    if (selector.rpl != GetCPL() || descriptor.dpl != GetCPL() || !descriptor.IsWritableDataSegment())
    {
      RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
      return;
    }
    if (!descriptor.IsPresent())
    {
      RaiseException(Interrupt_StackFault, selector.GetExceptionErrorCode(false));
      return;
    }
  }
  else if (segment == Segment_CS)
  {
    // Should've happened in the JMP part
    Assert(descriptor.IsCodeSegment() && descriptor.IsPresent());
  }
  else // DS,ES,FS,GS
  {
    if (!descriptor.IsDataSegment() && !descriptor.IsReadableCodeSegment())
    {
      RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
      return;
    }
    if (descriptor.IsDataSegment() || !descriptor.IsConformingCodeSegment())
    {
      if (selector.rpl > descriptor.dpl || GetCPL() > descriptor.dpl)
      {
        RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
        return;
      }
    }
    if (!descriptor.IsPresent())
    {
      RaiseException(Interrupt_SegmentNotPresent, selector.GetExceptionErrorCode(false));
      return;
    }
  }

  // Set accessed bit now that it's safe to load.
  SetDescriptorAccessedBit(descriptor, selector);

  // Extract information from descriptor
  const bool is_32bit_segment = (m_model >= MODEL_386 && descriptor.memory.flags.size);
  segment_cache->base_address = descriptor.memory.GetBase();
  segment_cache->limit_low = descriptor.memory.GetLimitLow();
  segment_cache->limit_high = descriptor.memory.GetLimitHigh();
  segment_cache->access.bits = descriptor.memory.access_bits;
  segment_cache->access.dpl = descriptor.dpl;
  m_registers.segment_selectors[segment] = value;

  // Access bits for code/data segments differ
  if (descriptor.memory.access.is_code)
  {
    segment_cache->access_mask = AccessTypeMask::Execute;
    if (descriptor.memory.access.code_readable)
      segment_cache->access_mask |= AccessTypeMask::Read;
  }
  else
  {
    segment_cache->access_mask = AccessTypeMask::Read;
    if (descriptor.memory.access.data_writable)
      segment_cache->access_mask |= AccessTypeMask::Write;
  }

  Log_TracePrintf("Load segment register %s = %04X: %s index %u base 0x%08X limit 0x%08X->0x%08X",
                  segment_names[segment], ZeroExtend32(value), selector.ti ? "LDT" : "GDT", u32(selector.index),
                  segment_cache->base_address, segment_cache->limit_low, segment_cache->limit_high);

  if (segment == Segment_CS)
  {
    // Code segment determines default address/operand sizes
    AddressSize new_address_size = (is_32bit_segment) ? AddressSize_32 : AddressSize_16;
    OperandSize new_operand_size = (is_32bit_segment) ? OperandSize_32 : OperandSize_16;
    if (new_address_size != m_current_address_size)
    {
      Log_DebugPrintf("Switching to %s %s execution%s", (new_address_size == AddressSize_32) ? "32-bit" : "16-bit",
                      InProtectedMode() ? "protected mode" : "real mode", IsPagingEnabled() ? " (paging enabled)" : "");

      m_current_address_size = new_address_size;
      m_current_operand_size = new_operand_size;
      m_EIP_mask = (is_32bit_segment) ? 0xFFFFFFFF : 0xFFFF;
    }

    // CPL is the selector's RPL
    if (GetCPL() != selector.rpl)
      Log_DebugPrintf("Privilege change: %u -> %u", ZeroExtend32(GetCPL()), ZeroExtend32(selector.rpl.GetValue()));
    SetCPL(selector.rpl);
    FlushPrefetchQueue();
  }
  else if (segment == Segment_SS)
  {
    // Stack segment determines stack address size
    AddressSize new_address_size = (is_32bit_segment) ? AddressSize_32 : AddressSize_16;
    if (new_address_size != m_stack_address_size)
    {
      Log_DebugPrintf("Switching to %s stack", (new_address_size == AddressSize_32) ? "32-bit" : "16-bit");
      m_stack_address_size = new_address_size;
    }
  }
}

void CPU::LoadLocalDescriptorTable(u16 value)
{
  // If it's a null descriptor, just clear out the fields. LDT entries can't be used after this.
  SEGMENT_SELECTOR_VALUE selector = {value};
  if (selector.IsNullSelector())
  {
    m_ldt_location.base_address = 0;
    m_ldt_location.limit = 0;
    m_registers.LDTR = selector.bits;
    return;
  }

  // Has to be a GDT selector
  if (selector.ti)
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return;
  }

  // Read descriptor entry. If this fails, it's because it's outside the limit.
  DESCRIPTOR_ENTRY descriptor;
  if (!ReadDescriptorEntry(&descriptor, m_gdt_location, selector.index))
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return;
  }
  if (!descriptor.present)
  {
    RaiseException(Interrupt_SegmentNotPresent, selector.GetExceptionErrorCode(false));
    return;
  }

  // Has to be a LDT descriptor
  if (!descriptor.IsSystemSegment() || descriptor.type != DESCRIPTOR_TYPE_LDT)
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return;
  }

  // Update descriptor cache
  m_ldt_location.base_address = descriptor.ldt.GetBase();
  m_ldt_location.limit = descriptor.ldt.GetLimit();
  m_registers.LDTR = selector.bits;

  Log_DebugPrintf("Load local descriptor table: %04X index %u base 0x%08X limit 0x%08X", ZeroExtend32(selector.bits),
                  ZeroExtend32(selector.index.GetValue()), m_tss_location.base_address, m_tss_location.limit);
}

void CPU::LoadTaskSegment(u16 value)
{
  // Has to be a GDT selector, and not a null selector
  SEGMENT_SELECTOR_VALUE selector = {value};
  if (selector.ti || selector.index == 0)
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return;
  }

  // Read descriptor entry. If this fails, it's because it's outside the limit.
  DESCRIPTOR_ENTRY descriptor;
  if (!ReadDescriptorEntry(&descriptor, m_gdt_location, selector.index))
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return;
  }
  if (!descriptor.present)
  {
    RaiseException(Interrupt_SegmentNotPresent, selector.GetExceptionErrorCode(false));
    return;
  }

  // Segment has to be a TSS, and has to be available
  if (!descriptor.IsSystemSegment() || (descriptor.type != DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16 &&
                                        descriptor.type != DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32))
  {
    RaiseException(Interrupt_SegmentNotPresent, selector.GetExceptionErrorCode(false));
    return;
  }

  // Mark segment as busy, this could be done with a bitwise OR
  descriptor.type = (descriptor.type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16) ?
                      DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16 :
                      DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32;
  if (!WriteDescriptorEntry(descriptor, m_gdt_location, selector.index))
  {
    // This shouldn't fail
    Panic("Failed to re-write active task segment descriptor");
  }

  // Update descriptor cache
  m_tss_location.base_address = descriptor.tss.GetBase();
  m_tss_location.limit = descriptor.tss.GetLimit();
  m_tss_location.type = static_cast<DESCRIPTOR_TYPE>(descriptor.type.GetValue());

  // Update the register copy of it
  m_registers.TR = selector.bits;

  Log_DebugPrintf("Load task register %04X: index %u base 0x%08X limit 0x%08X", ZeroExtend32(selector.bits),
                  ZeroExtend32(selector.index.GetValue()), m_tss_location.base_address, m_tss_location.limit);
}

void CPU::ClearInaccessibleSegmentSelectors()
{
  // Validate segments ES,FS,GS,DS so the kernel doesn't leak them
  static const Segment validate_segments[] = {Segment_ES, Segment_FS, Segment_GS, Segment_DS};
  for (u32 i = 0; i < countof(validate_segments); i++)
  {
    Segment validate_segment = validate_segments[i];
    const SegmentCache* validate_segment_cache = &m_segment_cache[validate_segment];
    if ((!validate_segment_cache->access.is_code || !validate_segment_cache->access.code_confirming) &&
        validate_segment_cache->access.dpl < GetCPL())
    {
      // If data or non-conforming code, set null selector
      LoadSegmentRegister(validate_segment, 0);
    }
  }
}

bool CPU::HasExternalInterrupt() const
{
  // TODO: NMI interrupts.
  // If there is a pending external interrupt and IF is set, jump to the interrupt handler.
  return (m_registers.EFLAGS.IF & m_irq_state) != 0;
}

void CPU::DispatchExternalInterrupt()
{
  DebugAssert(HasExternalInterrupt());

  // m_current_EIP/ESP must match the current state in case setting up the interrupt throws an exception.
  m_current_EIP = m_registers.EIP;
  m_current_ESP = m_registers.ESP;
  m_execution_stats.interrupts_serviced++;

  // Request interrupt number from the PIC.
  const u32 interrupt_number = m_interrupt_controller->GetInterruptNumber();
  Log_TracePrintf("Hardware interrupt %u", interrupt_number);
  SetupInterruptCall(interrupt_number, false, false, 0, m_registers.EIP);
}

void CPU::RaiseException(CPU* cpu, u32 interrupt, u32 error_code /* = 0 */)
{
  if (interrupt == Interrupt_PageFault)
  {
    Log_DebugPrintf("Raise exception %u error code 0x%08X EIP 0x%08X address 0x%08X", interrupt, error_code,
                    cpu->m_current_EIP, cpu->m_registers.CR2);
  }
  else
  {
    Log_DebugPrintf("Raise exception %u error code 0x%08X EIP 0x%08X", interrupt, error_code, cpu->m_current_EIP);
  }

  // If we're throwing an exception on a double-fault, this is a triple fault, and the CPU should reset.
  if (cpu->m_current_exception == Interrupt_DoubleFault)
  {
    // Failed double-fault, issue a triple fault.
    Log_WarningPrintf("Triple fault");
    cpu->Reset();
    cpu->AbortCurrentInstruction();
    return;
  }
  // If this is a nested exception, we issue a double-fault.
  else if (cpu->m_current_exception != Interrupt_Count)
  {
    // Change exception to double-fault.
    interrupt = Interrupt_DoubleFault;
    error_code = 0;
  }

  // Exceptions that include error codes
  bool push_error_code = (interrupt == Interrupt_InvalidTaskStateSegment || interrupt == Interrupt_SegmentNotPresent ||
                          interrupt == Interrupt_StackFault || interrupt == Interrupt_GeneralProtectionFault ||
                          interrupt == Interrupt_DoubleFault || interrupt == Interrupt_PageFault);

  // Restore stack before entering the interrupt handler, in case of
  // partially-executed instructions, or a nested exception.
  cpu->m_registers.ESP = cpu->m_current_ESP;
  cpu->m_current_exception = interrupt;
  cpu->m_execution_stats.exceptions_raised++;

  // Set up the call to the corresponding interrupt vector.
  cpu->SetupInterruptCall(interrupt, false, push_error_code, error_code, cpu->m_current_EIP);

  // Abort the current instruction that is executing.
  cpu->AbortCurrentInstruction();
}

void CPU::SoftwareInterrupt(u8 interrupt)
{
  if (InVirtual8086Mode())
  {
    // Check the bit in the interrupt bitmap, to see whether we should do a real-mode call.
    if (m_registers.CR4.VME && IsVMEInterruptBitSet(Truncate8(interrupt)))
    {
      // Invoke real-mode interrupt from within v8086 task.
      SetupV86ModeInterruptCall(interrupt, m_registers.EIP);
      return;
    }

    // Interrupts in V8086 mode with IOPL != 3 trap to monitor
    if (GetIOPL() < 3)
    {
      RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }
  }

  SetupInterruptCall(interrupt, true, false, 0, m_registers.EIP);
}

void CPU::RaiseSoftwareException(u32 interrupt)
{
  // TODO: Should this double fault on permission check fail? If so, uncomment the line below.
  // m_current_exception = interrupt;
  SetupInterruptCall(interrupt, true, false, 0, m_registers.EIP);
}

void CPU::RaiseDebugException()
{
  // We should push the next instruction pointer, not the instruction that's trapping,
  // since it has already executed. We also can't use RaiseException() since this would
  // reset the stack pointer too (and it could be a stack-modifying instruction). We
  // also don't need to abort the current instruction since we're looping anyway.
  SetupInterruptCall(Interrupt_Debugger, false, false, 0, m_registers.EIP);
}

void CPU::BranchTo(CPU* cpu, u32 new_EIP)
{
  cpu->FlushPrefetchQueue();
  cpu->m_registers.EIP = new_EIP;
  cpu->m_backend->BranchTo(new_EIP);
}

void CPU::BranchTo(u32 new_EIP)
{
  BranchTo(this, new_EIP);
}

void CPU::BranchFromException(u32 new_EIP)
{
  // Control was successfully transferred to the exception handler, so
  // we can clear the exception-in-progress now.
  m_current_exception = Interrupt_Count;

  // Prevent debug interrupt for this instruction.
  m_trap_after_instruction = false;

  FlushPrefetchQueue();
  m_registers.EIP = new_EIP;
  m_backend->BranchFromException(new_EIP);
}

void CPU::AbortCurrentInstruction()
{
  FlushPrefetchQueue();

  Log_TracePrintf("Aborting instruction at %04X:%08X", ZeroExtend32(m_registers.CS), m_registers.EIP);
  m_backend->AbortCurrentInstruction();
}

void CPU::RestartCurrentInstruction()
{
// Reset EIP, so that we start fetching from the beginning of the instruction again.
#ifdef ENABLE_PREFETCH_EMULATION
  u32 current_fetch_length = m_registers.EIP - m_current_EIP;
  if (m_prefetch_queue_position >= current_fetch_length)
    m_prefetch_queue_position -= current_fetch_length;
  else
    FlushPrefetchQueue();
#endif
  m_registers.EIP = m_current_EIP;
}

void CPU::FarJump(u16 segment_selector, u32 offset, OperandSize operand_size)
{
  // Really simple in real/V8086 mode.
  if (InRealMode() || InVirtual8086Mode())
  {
    LoadSegmentRegister(Segment_CS, segment_selector);
    BranchTo((operand_size == OperandSize_16) ? (offset & 0xFFFF) : (offset));
    return;
  }

  // Read descriptor, which can be a memory segment or a gate.
  SEGMENT_SELECTOR_VALUE selector = {segment_selector};
  DESCRIPTOR_ENTRY descriptor;
  if (selector.index == 0 ||
      !ReadDescriptorEntry(&descriptor, selector.ti ? m_ldt_location : m_gdt_location, selector.index))
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return;
  }
  if (!descriptor.present)
  {
    RaiseException(Interrupt_SegmentNotPresent, selector.GetExceptionErrorCode(false));
    return;
  }

  // Check for a simple jump to a code selector.
  if (descriptor.IsCodeSegment())
  {
    // Check for a conforming code segment
    if (descriptor.memory.IsConformingCodeSegment())
    {
      // Can't jump to a segment of lower privilege (DPL > CPL)
      if (descriptor.dpl > GetCPL())
      {
        RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
        return;
      }

      // RPL is replaced with CPL, call is allowed
      selector.rpl = GetCPL();
    }
    // Non-conforming code segment
    else
    {
      // Can't lower privilege levels via RPL, must have matching privilege
      if (selector.rpl > GetCPL() || descriptor.dpl != GetCPL())
      {
        RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
        return;
      }

      // RPL is replace with CPL, call is allowed
      selector.rpl = GetCPL();
    }

    LoadSegmentRegister(Segment_CS, selector.bits);
    BranchTo((operand_size == OperandSize_16) ? (offset & 0xFFFF) : (offset));
  }
  else if (descriptor.IsDataSegment())
  {
    // We can't jump to a non-code segment
    RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return;
  }
  else if (descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_16 || descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_32)
  {
    const bool is_32bit_gate = (descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_32);
    if (descriptor.dpl < GetCPL() || descriptor.dpl < selector.rpl)
    {
      RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
      return;
    }

    // Check the target of the gate is not a null selector, else GPF(0)
    SEGMENT_SELECTOR_VALUE target_selector = {descriptor.call_gate.selector};
    DESCRIPTOR_ENTRY target_descriptor;
    if (target_selector.index == 0 ||
        !ReadDescriptorEntry(&target_descriptor, target_selector.ti ? m_ldt_location : m_gdt_location,
                             target_selector.index) ||
        !target_descriptor.IsCodeSegment())
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(false));
      return;
    }
    if (!target_descriptor.IsPresent())
    {
      RaiseException(Interrupt_SegmentNotPresent, target_selector.GetExceptionErrorCode(false));
      return;
    }

    // Can't lower privilege or change privilege via far jumps
    if ((target_descriptor.memory.IsConformingCodeSegment() && target_descriptor.dpl > GetCPL()) ||
        (!target_descriptor.memory.IsConformingCodeSegment() && target_descriptor.dpl != GetCPL()))
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(false));
      return;
    }

    // All good to jump through it
    target_selector.rpl = GetCPL();
    LoadSegmentRegister(Segment_CS, target_selector.bits);
    BranchTo(is_32bit_gate ? (descriptor.call_gate.GetOffset()) : (descriptor.call_gate.GetOffset() & 0xFFFF));
  }
  else if (descriptor.type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16 ||
           descriptor.type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32 ||
           descriptor.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16 ||
           descriptor.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32 || descriptor.type == DESCRIPTOR_TYPE_TASK_GATE)
  {
    if (descriptor.dpl < GetCPL() || selector.rpl > descriptor.dpl)
    {
      RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
      return;
    }

    if (descriptor.type == DESCRIPTOR_TYPE_TASK_GATE)
    {
      // Switch to new task with nesting
      Log_DebugPrintf("Jump task gate -> 0x%04X", ZeroExtend32(descriptor.task_gate.selector.GetValue()));
      SwitchToTask(descriptor.task_gate.selector, false, false, false, 0);
    }
    else
    {
      // Jumping straight to a task segment without a task gate
      Log_DebugPrintf("Jump task segment -> 0x%04X", ZeroExtend32(segment_selector));
      SwitchToTask(segment_selector, false, false, false, 0);
    }
  }
  else
  {
    Panic("Unhandled far jump target");
  }
}

void CPU::FarCall(u16 segment_selector, u32 offset, OperandSize operand_size)
{
  auto MemorySegmentCall = [this](u16 real_selector, u32 real_offset, OperandSize real_operand_size) {
    // TODO: Should push(word) be done as a push(zeroextend32(word))?
    if (real_operand_size == OperandSize_16)
    {
      PushWord(m_registers.CS);
      PushWord(Truncate16(m_registers.EIP));
    }
    else
    {
      PushDWord(ZeroExtend32(m_registers.CS));
      PushDWord(m_registers.EIP);
    }

    // Far jump to 16-bit code.
    LoadSegmentRegister(Segment_CS, real_selector);
    BranchTo((real_operand_size == OperandSize_16) ? (real_offset & 0xFFFF) : (real_offset));
  };

  // Really simple in real/V8086 mode.
  if (InRealMode() || InVirtual8086Mode())
  {
    MemorySegmentCall(segment_selector, offset, operand_size);
    return;
  }

  // Read descriptor, which can be a memory segment or a gate.
  SEGMENT_SELECTOR_VALUE selector = {segment_selector};
  DESCRIPTOR_ENTRY descriptor;
  if (selector.index == 0 ||
      !ReadDescriptorEntry(&descriptor, selector.ti ? m_ldt_location : m_gdt_location, selector.index))
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return;
  }
  if (!descriptor.present)
  {
    RaiseException(Interrupt_SegmentNotPresent, selector.GetExceptionErrorCode(false));
    return;
  }

  // Check for a simple jump to a code selector.
  if (descriptor.IsCodeSegment())
  {
    // Check for a conforming code segment
    if (descriptor.memory.IsConformingCodeSegment())
    {
      // Can't jump to a segment of lower privilege (DPL > CPL)
      if (descriptor.dpl > GetCPL())
      {
        RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
        return;
      }

      // RPL is replaced with CPL, call is allowed
      selector.rpl = GetCPL();
    }
    // Non-conforming code segment
    else
    {
      // Can't lower privilege levels via RPL, must have matching privilege
      if (selector.rpl > GetCPL() || descriptor.dpl != GetCPL())
      {
        RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
        return;
      }

      // RPL is replace with CPL, call is allowed
      selector.rpl = GetCPL();
    }

    MemorySegmentCall(selector.bits, offset, operand_size);
  }
  else if (descriptor.IsDataSegment())
  {
    // We can't jump to a non-code segment
    RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
    return;
  }
  else if (descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_16 || descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_32)
  {
    const bool is_32bit_gate = (descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_32);
    if (descriptor.dpl < GetCPL() || selector.rpl > descriptor.dpl)
    {
      RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
      return;
    }

    // Check the target of the gate is not a null selector, else GPF(0)
    SEGMENT_SELECTOR_VALUE target_selector = {descriptor.call_gate.selector};
    DESCRIPTOR_ENTRY target_descriptor;
    if (target_selector.IsNullSelector() ||
        !ReadDescriptorEntry(&target_descriptor, target_selector.ti ? m_ldt_location : m_gdt_location,
                             target_selector.index) ||
        !target_descriptor.IsCodeSegment())
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(false));
      return;
    }
    if (!target_descriptor.IsPresent())
    {
      RaiseException(Interrupt_SegmentNotPresent, target_selector.GetExceptionErrorCode(false));
      return;
    }

    // Can't lower privilege?
    if (target_descriptor.dpl > GetCPL())
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(false));
      return;
    }

    // Changing privilege?
    if (!target_descriptor.memory.IsConformingCodeSegment() && target_descriptor.dpl < GetCPL())
    {
      // Call gate to lower privilege
      target_selector.rpl = target_descriptor.dpl;
      Log_DebugPrintf("Privilege raised via call gate, %u -> %u", ZeroExtend32(GetCPL()),
                      ZeroExtend32(target_selector.rpl.GetValue()));

      // We need to look at the current TSS to determine the stack pointer to change to
      u32 new_ESP;
      u16 new_SS;
      if (m_tss_location.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16)
      {
        LinearMemoryAddress tss_stack_offset =
          Truncate32(offsetof(TASK_STATE_SEGMENT_16, stacks[0]) + target_selector.rpl * 4);
        if ((tss_stack_offset + 3) > m_tss_location.limit)
        {
          RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
          return;
        }

        // Shouldn't fail, since we're bypassing access checks
        u16 temp = 0;
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset, &temp, AccessFlags::UseSupervisorPrivileges);
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset + 2, &new_SS,
                           AccessFlags::UseSupervisorPrivileges);
        new_ESP = ZeroExtend32(temp);
      }
      else
      {
        LinearMemoryAddress tss_stack_offset =
          Truncate32(offsetof(TASK_STATE_SEGMENT_32, stacks[0]) + target_selector.rpl * 8);
        if ((tss_stack_offset + 5) > m_tss_location.limit)
        {
          RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
          return;
        }

        // Shouldn't fail, since we're bypassing access checks
        SafeReadMemoryDWord(m_tss_location.base_address + tss_stack_offset, &new_ESP,
                            AccessFlags::UseSupervisorPrivileges);
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset + 4, &new_SS,
                           AccessFlags::UseSupervisorPrivileges);
      }

      // Read stack segment descriptor.
      // Inner SS selector must match DPL of code segment.
      // Inner SS DPL must match DPL of code segment.
      // SS must be data/writable and present.
      DESCRIPTOR_ENTRY inner_ss_descriptor;
      SEGMENT_SELECTOR_VALUE inner_ss_selector = {new_SS};
      if (inner_ss_selector.IsNullSelector() || !ReadDescriptorEntry(&inner_ss_descriptor, inner_ss_selector) ||
          inner_ss_descriptor.dpl != target_descriptor.dpl || !inner_ss_descriptor.IsWritableDataSegment())
      {
        RaiseException(Interrupt_InvalidTaskStateSegment, inner_ss_selector.GetExceptionErrorCode(false));
        return;
      }

      // Must be present. This triggers a different exception.
      if (!inner_ss_descriptor.IsPresent())
      {
        RaiseException(Interrupt_StackFault, inner_ss_selector.GetExceptionErrorCode(false));
        return;
      }

      // Save the old (outer) ESP/SS before we pop the parameters off?
      u32 outer_EIP = m_registers.EIP;
      u32 outer_ESP = m_registers.ESP;
      u16 outer_CS = m_registers.CS;
      u16 outer_SS = m_registers.SS;

      // Read parameters from caller before changing anything
      // We can pop here safely because the ESP will be restored afterwards
      u32 parameter_count = descriptor.call_gate.parameter_count & 0x1F;
      u32 caller_parameters[32];
      if (!is_32bit_gate)
      {
        for (u32 i = 0; i < parameter_count; i++)
          caller_parameters[(parameter_count - 1) - i] = ZeroExtend32(PopWord());
      }
      else
      {
        for (u32 i = 0; i < parameter_count; i++)
          caller_parameters[(parameter_count - 1) - i] = PopDWord();
      }

      // Make sure we have space in the new stack for the parameters, and outer SS/ESP/CS/EIP.
      TemporaryStack inner_stack(this, new_ESP, new_SS, inner_ss_descriptor);
      if (!(is_32bit_gate ? inner_stack.CanPushDWords(parameter_count + 4) :
                            inner_stack.CanPushWords(parameter_count + 4)))
      {
        RaiseException(Interrupt_StackFault, 0);
        return;
      }

      // The stack writes should be done with supervisor privileges.
      SetCPL(target_selector.rpl);

      // Write values to the new stack. This can still page fault, which will raise an exception in the calling context.
      if (!is_32bit_gate)
      {
        inner_stack.PushWord(outer_SS);
        inner_stack.PushWord(Truncate16(outer_ESP));
        for (u32 i = 0; i < parameter_count; i++)
          inner_stack.PushWord(Truncate16(caller_parameters[i]));
        inner_stack.PushWord(outer_CS);
        inner_stack.PushWord(Truncate16(outer_EIP));
      }
      else
      {
        inner_stack.PushDWord(ZeroExtend32(outer_SS));
        inner_stack.PushDWord(outer_ESP);
        for (u32 i = 0; i < parameter_count; i++)
          inner_stack.PushDWord(caller_parameters[i]);
        inner_stack.PushDWord(ZeroExtend32(outer_CS));
        inner_stack.PushDWord(outer_EIP);
      }

      // Load the new code segment early, since this can fail without side-effects
      LoadSegmentRegister(Segment_CS, target_selector.bits);

      // Load the new stack segment. This should succeed because we checked everything before.
      inner_stack.SwitchTo();

      // Finally transfer control.
      BranchTo(is_32bit_gate ? descriptor.call_gate.GetOffset() : (descriptor.call_gate.GetOffset() & 0xFFFF));
    }
    else
    {
      // Call gate to same privilege
      target_selector.rpl = GetCPL();
      if (!is_32bit_gate)
        MemorySegmentCall(target_selector.bits, descriptor.call_gate.GetOffset(), OperandSize_16);
      else
        MemorySegmentCall(target_selector.bits, descriptor.call_gate.GetOffset(), OperandSize_32);
    }
  }
  else if (descriptor.type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16 ||
           descriptor.type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32 ||
           descriptor.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16 ||
           descriptor.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32 || descriptor.type == DESCRIPTOR_TYPE_TASK_GATE)
  {
    if (descriptor.dpl < GetCPL() || selector.rpl > descriptor.dpl)
    {
      RaiseException(Interrupt_GeneralProtectionFault, selector.GetExceptionErrorCode(false));
      return;
    }

    if (descriptor.type == DESCRIPTOR_TYPE_TASK_GATE)
    {
      // Switch to new task with nesting
      Log_DebugPrintf("Call task gate -> 0x%04X", ZeroExtend32(descriptor.task_gate.selector.GetValue()));
      SwitchToTask(descriptor.task_gate.selector, true, false, false, 0);
    }
    else
    {
      // Jumping straight to a task segment without a task gate
      Log_DebugPrintf("Call task segment -> 0x%04X", ZeroExtend32(segment_selector));
      SwitchToTask(segment_selector, true, false, false, 0);
    }
  }
  else
  {
    Panic("Unhandled far call type");
  }
}

void CPU::FarReturn(OperandSize operand_size, u32 pop_byte_count)
{
  u32 return_EIP;
  u16 return_CS;
  if (operand_size == OperandSize_16)
  {
    return_EIP = ZeroExtend32(PopWord());
    return_CS = PopWord();
  }
  else
  {
    return_EIP = PopDWord();
    return_CS = Truncate16(PopDWord());
  }

  // Subtract extra bytes from operand.
  if (m_stack_address_size == AddressSize_16)
    m_registers.SP += Truncate16(pop_byte_count);
  else
    m_registers.ESP += pop_byte_count;

  // V8086 and real mode are the same here
  if (InRealMode() || InVirtual8086Mode())
  {
    LoadSegmentRegister(Segment_CS, return_CS);
    BranchTo((operand_size == OperandSize_16) ? (return_EIP & 0xFFFF) : (return_EIP));
  }
  else
  {
    SEGMENT_SELECTOR_VALUE target_selector = {return_CS};
    DESCRIPTOR_ENTRY target_descriptor;
    if (target_selector.IsNullSelector()) // Check for non-null segment
    {
      RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }
    if (!ReadDescriptorEntry(&target_descriptor, target_selector.ti ? m_ldt_location : m_gdt_location,
                             target_selector.index) || // Check table limits
        !target_descriptor.IsCodeSegment())            // Check for code segment
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(false));
      return;
    }
    if (target_selector.rpl < GetCPL()) // Check RPL<CPL
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(false));
      return;
    }
    if (target_descriptor.IsConformingCodeSegment() &&
        target_descriptor.dpl > target_selector.rpl) // conforming and DPL>selector RPL
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(false));
      return;
    }
    if (!target_descriptor.IsConformingCodeSegment() &&
        target_descriptor.dpl != target_selector.rpl) // non-conforming and DPL!=RPL
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(false));
      return;
    }
    if (!target_descriptor.IsPresent())
    {
      RaiseException(Interrupt_SegmentNotPresent, target_selector.GetExceptionErrorCode(false));
      return;
    }
    if (target_selector.rpl > GetCPL())
    {
      // Returning to outer privilege level
      Log_DebugPrintf("Privilege lowered via RETF: %u -> %u", ZeroExtend32(GetCPL()),
                      ZeroExtend32(target_selector.rpl.GetValue()));

      u32 return_ESP;
      u16 return_SS;
      if (operand_size == OperandSize_16)
      {
        return_ESP = ZeroExtend32(PopWord());
        return_SS = PopWord();
      }
      else
      {
        return_ESP = PopDWord();
        return_SS = Truncate16(PopDWord());
      }

      // Update privilege level for SS check, see InterruptReturn.
      SetCPL(target_selector.rpl);
      LoadSegmentRegister(Segment_SS, return_SS);
      LoadSegmentRegister(Segment_CS, return_CS);
      ClearInaccessibleSegmentSelectors();

      // Release parameters from caller's stack
      m_registers.ESP = return_ESP;
      if (m_stack_address_size == AddressSize_16)
        m_registers.SP += Truncate16(pop_byte_count);
      else
        m_registers.ESP += pop_byte_count;

      // Transfer instruction pointer finally
      BranchTo((operand_size == OperandSize_16) ? (return_EIP & 0xFFFF) : (return_EIP));
    }
    else
    {
      // Returning to same privilege level
      LoadSegmentRegister(Segment_CS, return_CS);
      BranchTo((operand_size == OperandSize_16) ? (return_EIP & 0xFFFF) : (return_EIP));
    }
  }
}

void CPU::InterruptReturn(OperandSize operand_size)
{
  if (InRealMode())
  {
    // Pull EIP/CS/FLAGS off the stack
    u32 return_EIP;
    u16 return_CS;
    u32 return_EFLAGS;
    if (operand_size == OperandSize_16)
    {
      return_EIP = ZeroExtend32(PopWord());
      return_CS = PopWord();
      return_EFLAGS = (m_registers.EFLAGS.bits & 0xFFFF0000) | ZeroExtend32(PopWord());
    }
    else
    {
      return_EIP = PopDWord();
      return_CS = Truncate16(PopDWord());
      return_EFLAGS = PopDWord();
    }

    // Simple IRET when in real mode, just change code segment
    LoadSegmentRegister(Segment_CS, return_CS);
    SetFlags(return_EFLAGS);
    BranchTo(return_EIP);
  }
  else if (InVirtual8086Mode())
  {
    // V8086 return and IOPL != 3 traps to monitor.
    if (GetIOPL() != 3)
    {
      // VME only kicks in on 16-bit IRET.
      if (!m_registers.CR4.VME || operand_size != OperandSize_16)
      {
        RaiseException(Interrupt_GeneralProtectionFault, 0);
        return;
      }
    }

    // Pull EIP/CS/FLAGS off the stack
    // VM, IOPL, VIP, VIF not modified by flags change
    u32 return_EIP;
    u16 return_CS;
    u32 return_EFLAGS;
    if (operand_size == OperandSize_16)
    {
      return_EIP = ZeroExtend32(PopWord());
      return_CS = PopWord();
      return_EFLAGS = (m_registers.EFLAGS.bits & 0xFFFF0000) | ZeroExtend32(PopWord());

      // In all V8086 mode returns, mask away flags it can't change.
      return_EFLAGS = (return_EFLAGS & ~(Flag_VM | Flag_IOPL)) | (m_registers.EFLAGS.bits & (Flag_VM | Flag_IOPL));

      if (m_registers.CR4.VME)
      {
        // If the stack image IF is set, and VIP is 1, #GP.
        if (m_registers.EFLAGS.VIP && (return_EFLAGS & Flag_IF) != 0)
        {
          RaiseException(Interrupt_GeneralProtectionFault, 0);
          return;
        }

        // If VME is enabled, VIP = IF unless IOPL = 0.
        if (GetIOPL() < 3)
          return_EFLAGS = (return_EFLAGS & ~Flag_VIF) | ((return_EFLAGS & Flag_IF) << 10);
      }
    }
    else
    {
      return_EIP = PopDWord();
      return_CS = Truncate16(PopDWord());
      return_EFLAGS = PopDWord();
      return_EFLAGS &= ~(Flag_VM | Flag_IOPL | Flag_VIP | Flag_VIF);
      return_EFLAGS |= m_registers.EFLAGS.bits & (Flag_VM | Flag_IOPL | Flag_VIP | Flag_VIF);

      // If VME is enabled, VIF = IF.
      if (m_registers.CR4.VME)
        return_EFLAGS = (return_EFLAGS & ~Flag_VIF) | ((return_EFLAGS & Flag_IF) << 10);
    }

    // IRET within V8086 mode, handle the same as real mode
    LoadSegmentRegister(Segment_CS, return_CS);
    SetFlags(return_EFLAGS);
    BranchTo(return_EIP);
  }
  else if (m_registers.EFLAGS.NT)
  {
    // Nested task return should not pop anything off stack
    // Link field is always two bytes at offset zero, in both 16 and 32-bit TSS
    u16 link_field;
    if ((sizeof(link_field) - 1) > m_tss_location.limit)
    {
      RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
      return;
    }
    SafeReadMemoryWord(m_tss_location.base_address, &link_field, AccessFlags::UseSupervisorPrivileges);

    // Switch tasks without nesting
    SwitchToTask(link_field, false, true, false, 0);
  }
  else
  {
    // Protected mode
    // Pull EIP/CS/FLAGS off the stack
    u32 return_EIP;
    u16 return_CS;
    u32 return_EFLAGS;
    if (operand_size == OperandSize_16)
    {
      return_EIP = ZeroExtend32(PopWord());
      return_CS = PopWord();
      return_EFLAGS = (m_registers.EFLAGS.bits & 0xFFFF0000) | ZeroExtend32(PopWord());
    }
    else
    {
      return_EIP = PopDWord();
      return_CS = Truncate16(PopDWord());
      return_EFLAGS = PopDWord();
    }

    // CPL must be zero to change V8086 state
    if (GetCPL() == 0 && operand_size == OperandSize_32 && (return_EFLAGS & Flag_VM))
    {
      // EIP is masked, ESP is not.
      return_EIP &= 0xFFFF;

      // Entering V8086 mode
      Log_DebugPrintf("Entering V8086 mode, EFLAGS = %08X, CS:IP = %04X:%04X", return_EFLAGS, ZeroExtend32(return_CS),
                      return_EIP);

      // TODO: Check EIP lies within CS limits.
      u32 v86_ESP = PopDWord();
      u16 v86_SS = Truncate16(PopDWord());
      u16 v86_ES = Truncate16(PopDWord());
      u16 v86_DS = Truncate16(PopDWord());
      u16 v86_FS = Truncate16(PopDWord());
      u16 v86_GS = Truncate16(PopDWord());

      // Enter v8086 mode
      // TODO: Validate segment registers
      m_registers.EFLAGS.VM = true;
      LoadSegmentRegister(Segment_CS, return_CS);
      LoadSegmentRegister(Segment_SS, v86_SS);
      LoadSegmentRegister(Segment_ES, v86_ES);
      LoadSegmentRegister(Segment_DS, v86_DS);
      LoadSegmentRegister(Segment_FS, v86_FS);
      LoadSegmentRegister(Segment_GS, v86_GS);
      SetFlags(return_EFLAGS);

      m_registers.ESP = v86_ESP;

      BranchTo(return_EIP);
      return;
    }

    // We can't raise privileges by IRET
    SEGMENT_SELECTOR_VALUE target_selector = {return_CS};
    if (target_selector.rpl < GetCPL())
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(false));
      return;
    }

    // Validate we can jump to this segment from here
    if (!CheckTargetCodeSegment(return_CS, 0, target_selector.rpl, true))
      return;

    // Some flags can't be changed if we're not in CPL=0.
    u32 change_mask = Flag_CF | Flag_PF | Flag_AF | Flag_ZF | Flag_SF | Flag_TF | Flag_DF | Flag_OF | Flag_NT |
                      Flag_RF | Flag_AC | Flag_ID;
    if (GetCPL() <= GetIOPL())
      change_mask |= Flag_IF;
    if (GetCPL() == 0)
      change_mask |= Flag_VIP | Flag_VIF | Flag_IOPL;
    return_EFLAGS = (return_EFLAGS & change_mask) | (m_registers.EFLAGS.bits & ~change_mask);

    // Are we changing privilege levels?
    if (target_selector.rpl > GetCPL())
    {
      // Returning to a outer/lower privilege level
      Log_DebugPrintf("Privilege lowered via IRET, %u -> %u", ZeroExtend32(GetCPL()),
                      ZeroExtend32(target_selector.rpl.GetValue()));

      // Grab ESP/SS from stack
      u32 outer_ESP;
      u16 outer_SS;
      if (operand_size == OperandSize_16)
      {
        outer_ESP = ZeroExtend32(PopWord());
        outer_SS = PopWord();
      }
      else
      {
        outer_ESP = PopDWord();
        outer_SS = Truncate16(PopDWord());
      }

      // Switch CPL to outer level, so the privilege of SS will be checked.
      SetCPL(target_selector.rpl);

      // Switch outer stack segment before switching CS, otherwise CS:IP will be invalid when it raises exceptions.
      LoadSegmentRegister(Segment_SS, outer_SS);
      LoadSegmentRegister(Segment_CS, return_CS);

      // Finally now that we can't fail, sort out the registers
      // Higher-order bits of ESP leak, undocumented
      if (m_stack_address_size == AddressSize_16)
        m_registers.SP = Truncate16(outer_ESP);
      else
        m_registers.ESP = outer_ESP;

      SetFlags(return_EFLAGS);
      ClearInaccessibleSegmentSelectors();
      BranchTo(return_EIP);
    }
    else
    {
      // Returning to the same privilege level
      LoadSegmentRegister(Segment_CS, return_CS);
      SetFlags(return_EFLAGS);
      BranchTo(return_EIP);
    }
  }
}

void CPU::SetupInterruptCall(u32 interrupt, bool software_interrupt, bool push_error_code, u32 error_code,
                             u32 return_EIP)
{
  if (InRealMode())
    SetupRealModeInterruptCall(interrupt, return_EIP);
  else
    SetupProtectedModeInterruptCall(interrupt, software_interrupt, push_error_code, error_code, return_EIP);
}

void CPU::SetupRealModeInterruptCall(u32 interrupt, u32 return_EIP)
{
  // Check IVT limit.
  const PhysicalMemoryAddress table_offset = static_cast<PhysicalMemoryAddress>(interrupt) * 4;
  if (table_offset > m_idt_location.limit)
  {
    RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // Read IVT.
  u32 ivt_entry = 0;
  SafeReadMemoryDWord(m_idt_location.base_address + table_offset, &ivt_entry, AccessFlags::UseSupervisorPrivileges);

  // Extract segment/instruction pointer from IDT entry
  const u16 isr_segment_selector = Truncate16(ivt_entry >> 16);
  const u32 isr_EIP = ZeroExtend32(ivt_entry & 0xFFFF);

  // Push FLAGS, CS, IP
  PushWord(Truncate16(m_registers.EFLAGS.bits));
  PushWord(m_registers.CS);
  PushWord(Truncate16(return_EIP));

  // Clear interrupt flag if set (stop recursive interrupts)
  m_registers.EFLAGS.IF = false;
  m_registers.EFLAGS.TF = false;
  m_registers.EFLAGS.AC = false;

  // Resume code execution at interrupt entry point
  LoadSegmentRegister(Segment_CS, isr_segment_selector);
  BranchFromException(isr_EIP);
}

void CPU::SetupV86ModeInterruptCall(u8 interrupt, u32 return_EIP)
{
  // Read virtual IVT.
  u32 ivt_entry = 0;
  SafeReadMemoryDWord(static_cast<PhysicalMemoryAddress>(interrupt) * 4, &ivt_entry,
                      AccessFlags::UseSupervisorPrivileges);

  // Extract segment/instruction pointer from IDT entry
  const u16 isr_segment_selector = Truncate16(ivt_entry >> 16);
  const u32 isr_EIP = ZeroExtend32(ivt_entry & 0xFFFF);

  // VIF -> IF in flags if IOPL < 3
  u16 push_FLAGS = Truncate16(m_registers.EFLAGS.bits);
  if (GetIOPL() < 3)
    push_FLAGS = (push_FLAGS & ~Flag_IF) | Truncate16((m_registers.EFLAGS.bits & Flag_VIF) >> 10) | Flag_IOPL;

  // Push FLAGS, CS, IP
  PushWord(push_FLAGS);
  PushWord(m_registers.CS);
  PushWord(Truncate16(return_EIP));

  // Clear interrupt flag if set (stop recursive interrupts)
  m_registers.EFLAGS.TF = false;
  m_registers.EFLAGS.RF = false;
  if (GetIOPL() == 3)
    m_registers.EFLAGS.IF = false;
  else
    m_registers.EFLAGS.VIF = false;

  // Resume code execution at interrupt entry point
  LoadSegmentRegister(Segment_CS, isr_segment_selector);
  BranchFromException(isr_EIP);
}

constexpr u32 MakeIDTErrorCode(u32 num, bool software_interrupt)
{
  return ((num << 3) | u32(2) | BoolToUInt32(software_interrupt));
}

void CPU::SetupProtectedModeInterruptCall(u32 interrupt, bool software_interrupt, bool push_error_code, u32 error_code,
                                          u32 return_EIP)
{
  // Check against bounds of the IDT
  DESCRIPTOR_ENTRY descriptor;
  if (!ReadDescriptorEntry(&descriptor, m_idt_location, interrupt))
  {
    // Raise GPF for out-of-range.
    Log_DebugPrintf("Interrupt out of range: %u (limit %u)", interrupt, u32(m_idt_location.limit));
    RaiseException(Interrupt_GeneralProtectionFault, MakeIDTErrorCode(interrupt, software_interrupt));
    return;
  }

  // Check type
  if (descriptor.type != DESCRIPTOR_TYPE_INTERRUPT_GATE_16 && descriptor.type != DESCRIPTOR_TYPE_INTERRUPT_GATE_32 &&
      descriptor.type != DESCRIPTOR_TYPE_TRAP_GATE_16 && descriptor.type != DESCRIPTOR_TYPE_TRAP_GATE_32 &&
      descriptor.type != DESCRIPTOR_TYPE_TASK_GATE)
  {
    Log_DebugPrintf("Invalid IDT gate type");
    RaiseException(Interrupt_GeneralProtectionFault, MakeIDTErrorCode(interrupt, software_interrupt));
    return;
  }

  // Software interrupts have to check that CPL <= DPL to access privileged interrupts
  if (software_interrupt && descriptor.dpl < GetCPL())
  {
    RaiseException(Interrupt_GeneralProtectionFault, MakeIDTErrorCode(interrupt, software_interrupt));
    return;
  }

  // Check present flag
  if (!descriptor.IsPresent())
  {
    RaiseException(Interrupt_GeneralProtectionFault, MakeIDTErrorCode(interrupt, software_interrupt));
    return;
  }

  if (descriptor.type == DESCRIPTOR_TYPE_INTERRUPT_GATE_16 || descriptor.type == DESCRIPTOR_TYPE_INTERRUPT_GATE_32 ||
      descriptor.type == DESCRIPTOR_TYPE_TRAP_GATE_16 || descriptor.type == DESCRIPTOR_TYPE_TRAP_GATE_32)
  {
    // Load target segment selector
    SEGMENT_SELECTOR_VALUE target_selector = {descriptor.interrupt_gate.selector};
    DESCRIPTOR_ENTRY target_descriptor;
    if (target_selector.index == 0)
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(!software_interrupt));
      return;
    }
    if (!ReadDescriptorEntry(&target_descriptor, target_selector.ti ? m_ldt_location : m_gdt_location,
                             target_selector.index) ||
        !target_descriptor.IsCodeSegment() || target_descriptor.dpl > GetCPL())
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(!software_interrupt));
      return;
    }
    if (!target_descriptor.IsPresent())
    {
      RaiseException(Interrupt_SegmentNotPresent, target_selector.GetExceptionErrorCode(!software_interrupt));
      return;
    }

    // Does this result in a privilege change?
    if (!target_descriptor.memory.IsConformingCodeSegment() && target_descriptor.dpl < GetCPL())
    {
      bool is_virtual_8086_exit = InVirtual8086Mode();
      if (is_virtual_8086_exit)
      {
        // If new code segment DPL != 0, then #GP(
        if (target_descriptor.dpl != 0)
        {
          RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(!software_interrupt));
          return;
        }

        // Leaving V8086 mode via trap
        Log_DebugPrintf("Leaving V8086 mode via gate %u", interrupt);
      }
      else
      {
        Log_DebugPrintf("Privilege raised via interrupt gate, %u -> %u", ZeroExtend32(GetCPL()),
                        ZeroExtend32(target_descriptor.dpl.GetValue()));
        target_selector.rpl = target_descriptor.dpl;
      }

      // Check TSS validity
      if (m_tss_location.limit == 0)
      {
        RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
        return;
      }

      // We need to look at the current TSS to determine the stack pointer to change to
      u32 new_ESP = 0;
      u16 new_SS = 0;
      if (m_tss_location.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16)
      {
        LinearMemoryAddress tss_stack_offset =
          Truncate32(offsetof(TASK_STATE_SEGMENT_16, stacks[0]) + target_selector.rpl * 4);
        if ((tss_stack_offset + 3) > m_tss_location.limit)
        {
          RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
          return;
        }

        // Shouldn't fail, since we're bypassing access checks
        u16 temp = 0;
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset, &temp, AccessFlags::UseSupervisorPrivileges);
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset + 2, &new_SS,
                           AccessFlags::UseSupervisorPrivileges);
        new_ESP = ZeroExtend32(temp);
      }
      else
      {
        LinearMemoryAddress tss_stack_offset =
          Truncate32(offsetof(TASK_STATE_SEGMENT_32, stacks[0]) + target_selector.rpl * 8);
        if ((tss_stack_offset + 5) > m_tss_location.limit)
        {
          RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
          return;
        }

        // Shouldn't fail, since we're bypassing access checks
        SafeReadMemoryDWord(m_tss_location.base_address + tss_stack_offset, &new_ESP,
                            AccessFlags::UseSupervisorPrivileges);
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset + 4, &new_SS,
                           AccessFlags::UseSupervisorPrivileges);
      }

      // Save the old (outer) ESP/SS before we pop the parameters off?
      u32 return_EFLAGS = m_registers.EFLAGS.bits;
      u32 return_ESP = m_registers.ESP;
      u16 return_CS = m_registers.CS;
      u16 return_SS = m_registers.SS;

      // SS should have the same privilege level as CS
      // TODO: Check this on the descriptor as well
      SEGMENT_SELECTOR_VALUE return_SS_selector = {new_SS};
      if (return_SS_selector.rpl != target_selector.rpl)
      {
        RaiseException(Interrupt_StackFault, 0);
        return;
      }

      // If loading this segment fails, we'll get pushed out of V8086 when we shouldn't..
      // TODO: We really need a ValidateSegmentRegister
      m_registers.EFLAGS.bits &= ~(Flag_VM | Flag_TF | Flag_RF | Flag_NT);

      // Load the new code segment early, since this can fail without side-effects
      LoadSegmentRegister(Segment_CS, target_selector.bits);

      // Load the new stack segment. If any of the pushes following this fail, we are in trouble.
      // TODO: Perhaps we should save the SS as well as the ESP in case of exceptions?
      LoadSegmentRegister(Segment_SS, new_SS);
      if (m_stack_address_size == AddressSize_16)
        m_registers.SP = Truncate16(new_ESP);
      else
        m_registers.ESP = new_ESP;

      // V8086 returns have different parameters
      if (is_virtual_8086_exit)
      {
        if (descriptor.type == DESCRIPTOR_TYPE_INTERRUPT_GATE_16 || descriptor.type == DESCRIPTOR_TYPE_TRAP_GATE_16)
        {
          PushWord(m_registers.GS);
          PushWord(m_registers.FS);
          PushWord(m_registers.DS);
          PushWord(m_registers.ES);
          PushWord(return_SS);
          PushWord(Truncate16(return_ESP));
          PushWord(Truncate16(return_EFLAGS));
          PushWord(return_CS);
          PushWord(Truncate16(return_EIP));
          if (push_error_code)
            PushWord(Truncate16(error_code));
        }
        else
        {
          PushDWord(ZeroExtend32(m_registers.GS));
          PushDWord(ZeroExtend32(m_registers.FS));
          PushDWord(ZeroExtend32(m_registers.DS));
          PushDWord(ZeroExtend32(m_registers.ES));
          PushDWord(ZeroExtend32(return_SS));
          PushDWord(return_ESP);
          PushDWord(return_EFLAGS);
          PushDWord(ZeroExtend32(return_CS));
          PushDWord(return_EIP);
          if (push_error_code)
            PushDWord(error_code);
        }

        // Clear high flags
        LoadSegmentRegister(Segment_GS, 0);
        LoadSegmentRegister(Segment_FS, 0);
        LoadSegmentRegister(Segment_DS, 0);
        LoadSegmentRegister(Segment_ES, 0);
      }
      else
      {
        // Push parameters to target procedure
        if (descriptor.type == DESCRIPTOR_TYPE_INTERRUPT_GATE_16 || descriptor.type == DESCRIPTOR_TYPE_TRAP_GATE_16)
        {
          PushWord(return_SS);
          PushWord(Truncate16(return_ESP));
          PushWord(Truncate16(return_EFLAGS));
          PushWord(return_CS);
          PushWord(Truncate16(return_EIP));
          if (push_error_code)
            PushWord(Truncate16(error_code));
        }
        else
        {
          PushDWord(ZeroExtend32(return_SS));
          PushDWord(return_ESP);
          PushDWord(return_EFLAGS);
          PushDWord(ZeroExtend32(return_CS));
          PushDWord(return_EIP);
          if (push_error_code)
            PushDWord(error_code);
        }
      }

      // Finally transfer control
      u32 new_EIP = descriptor.call_gate.GetOffset();
      if (descriptor.type == DESCRIPTOR_TYPE_INTERRUPT_GATE_16)
        new_EIP &= 0xFFFF;
      BranchFromException(new_EIP);
    }
    else
    {
      // Trap to V8086 monitor
      if (InVirtual8086Mode())
      {
        RaiseException(Interrupt_GeneralProtectionFault, target_selector.GetExceptionErrorCode(!software_interrupt));
        return;
      }

      // Push FLAGS, CS, IP
      if (descriptor.type == DESCRIPTOR_TYPE_INTERRUPT_GATE_16 || descriptor.type == DESCRIPTOR_TYPE_TRAP_GATE_16)
      {
        PushWord(Truncate16(m_registers.EFLAGS.bits));
        PushWord(m_registers.CS);
        PushWord(Truncate16(return_EIP));
        if (push_error_code)
          PushWord(Truncate16(error_code));

        // Far jump to 16-bit code. RPL=CPL
        target_selector.rpl = GetCPL();
        LoadSegmentRegister(Segment_CS, target_selector.bits);
        BranchFromException(descriptor.interrupt_gate.GetOffset() & 0xFFFF);
      }
      else
      {
        PushDWord(m_registers.EFLAGS.bits);
        PushDWord(m_registers.CS);
        PushDWord(return_EIP);
        if (push_error_code)
          PushDWord(error_code);

        // Far jump to 32-bit code. RPL=CPL
        target_selector.rpl = GetCPL();
        LoadSegmentRegister(Segment_CS, target_selector.bits);
        BranchFromException(descriptor.interrupt_gate.GetOffset());
      }
    }

    // Clear interrupt flag if set (stop recursive interrupts)
    m_registers.EFLAGS.TF = false;
    m_registers.EFLAGS.RF = false;
    if (descriptor.type == DESCRIPTOR_TYPE_INTERRUPT_GATE_16 || descriptor.type == DESCRIPTOR_TYPE_INTERRUPT_GATE_32)
    {
      m_registers.EFLAGS.IF = false;
    }
  }
  else if (descriptor.type == DESCRIPTOR_TYPE_TASK_GATE)
  {
    // When we eventually switch back to the faulting task, we want EIP to point to the instruction
    // which faulted, not the next instruction (which is what EIP currently contains).
    m_registers.EIP = return_EIP;

    // Switch to new task with nesting
    DebugAssert(!m_registers.EFLAGS.VM);
    Log_DebugPrintf("Task gate -> 0x%04X", ZeroExtend32(descriptor.task_gate.selector.GetValue()));
    SwitchToTask(descriptor.task_gate.selector, true, false, push_error_code, error_code);
  }
  else
  {
    Panic("Unhandled gate type");
  }
}

inline constexpr bool IsAvailableTaskDescriptorType(u8 type)
{
  return (type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16 || type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32);
}

inline constexpr bool IsBusyTaskDescriptorType(u8 type)
{
  return (type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16 || type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32);
}

void CPU::SwitchToTask(u16 new_task, bool nested_task, bool from_iret, bool push_error_code, u32 error_code)
{
  Log_DevPrintf("Switching from task %02X to task %02X%s", ZeroExtend32(m_registers.TR), ZeroExtend32(new_task),
                nested_task ? " (nested)" : "");

  // Read the current task descriptor. This should never fail.
  SEGMENT_SELECTOR_VALUE current_task_selector = {m_registers.TR};
  DESCRIPTOR_ENTRY current_task_descriptor;
  if (current_task_selector.index == 0 || current_task_selector.ti ||
      !ReadDescriptorEntry(&current_task_descriptor, m_gdt_location, current_task_selector.index))
  {
    RaiseException(Interrupt_SegmentNotPresent, current_task_selector.GetExceptionErrorCode(false));
    return;
  }

  // The current task should be busy, but it's not fatal if it isn't.
  if (current_task_descriptor.is_memory_descriptor || (!IsAvailableTaskDescriptorType(current_task_descriptor.type) &&
                                                       !IsBusyTaskDescriptorType(current_task_descriptor.type)))
  {
    Log_WarningPrintf("Outgoing task descriptor is not valid - type %u",
                      ZeroExtend32(current_task_descriptor.type.GetValue()));

    // TODO: Is this correct?
    RaiseException(Interrupt_GeneralProtectionFault, current_task_selector.GetExceptionErrorCode(false));
    return;
  }

  // The limit should be enough for the task state
  bool current_task_is_32bit = ((current_task_descriptor.type & 8) != 0);
  u32 current_tss_min_size = (current_task_is_32bit) ? sizeof(TASK_STATE_SEGMENT_32) : sizeof(TASK_STATE_SEGMENT_16);
  if (current_task_descriptor.tss.GetLimit() < (current_tss_min_size - 1))
  {
    // TODO: Is this correct?
    Log_WarningPrintf("Outgoing task segment is too small - %u required %u", current_task_descriptor.tss.GetLimit() + 1,
                      current_tss_min_size);
    RaiseException(Interrupt_InvalidTaskStateSegment, current_task_selector.GetExceptionErrorCode(false));
    return;
  }

  // Clear the busy bit in the current task descriptor before loading the new task descriptor.
  // This allows us to switch to the same task (which is legal).
  if (!nested_task)
  {
    // Otherwise clear the busy flag on the current task
    current_task_descriptor.type =
      (current_task_is_32bit) ? DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32 : DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16;
    if (!WriteDescriptorEntry(current_task_descriptor, m_gdt_location, current_task_selector.index))
      Panic("Failed to update descriptor entry that was successfully read");
  }

  // Validate the new task before switching out.
  SEGMENT_SELECTOR_VALUE new_task_selector = {new_task};
  DESCRIPTOR_ENTRY new_task_descriptor;
  if (new_task_selector.index == 0 || new_task_selector.ti ||
      !ReadDescriptorEntry(&new_task_descriptor, m_gdt_location, new_task_selector.index))
  {
    Log_WarningPrintf("Incoming task descriptor is not valid");
    RaiseException(Interrupt_GeneralProtectionFault, new_task_selector.GetExceptionErrorCode(false));
    return;
  }

  // The new task must not be busy, except if this is a task switch from an IRET, the task we're switching to must be
  // busy. http://www.logix.cz/michal/doc/i386/chp07-06.htm
  if (new_task_descriptor.is_memory_descriptor || (from_iret && !IsBusyTaskDescriptorType(new_task_descriptor.type)) ||
      (!from_iret && !IsAvailableTaskDescriptorType(new_task_descriptor.type)))
  {
    // TODO: Is this correct?
    Log_WarningPrintf("Incoming task descriptor is incorrect type - %u",
                      ZeroExtend32(new_task_descriptor.type.GetValue()));
    RaiseException(from_iret ? Interrupt_InvalidTaskStateSegment : Interrupt_GeneralProtectionFault,
                   new_task_selector.GetExceptionErrorCode(false));
    return;
  }

  // Check the limit on the new task state
  bool new_task_is_32bit = ((new_task_descriptor.type & 8) != 0);
  u32 new_tss_min_size = (new_task_is_32bit) ? sizeof(TASK_STATE_SEGMENT_32) : sizeof(TASK_STATE_SEGMENT_16);
  if (new_task_descriptor.tss.GetLimit() < (new_tss_min_size - 1))
  {
    // TODO: Is this correct?
    Log_WarningPrintf("Incoming task segment is too small - %u required %u", new_task_descriptor.tss.GetLimit() + 1,
                      new_tss_min_size);
    RaiseException(Interrupt_InvalidTaskStateSegment, new_task_selector.GetExceptionErrorCode(false));
    return;
  }

  // Calculate linear addresses of task segments
  LinearMemoryAddress current_tss_address = current_task_descriptor.tss.GetBase();
  LinearMemoryAddress new_tss_address = new_task_descriptor.tss.GetBase();
  union
  {
    TASK_STATE_SEGMENT_16 ts16;
    TASK_STATE_SEGMENT_32 ts32;
    u16 words[sizeof(TASK_STATE_SEGMENT_16) / sizeof(u16)];
    u32 dwords[sizeof(TASK_STATE_SEGMENT_32) / sizeof(u32)];
    u16 link;
  } current_task_state = {}, new_task_state = {};

  // Read the current TSS in, this could cause a page fault
  if (current_task_is_32bit)
  {
    for (u32 i = 0; i < countof(current_task_state.dwords); i++)
    {
      SafeReadMemoryDWord(current_tss_address + i * sizeof(u32), &current_task_state.dwords[i],
                          AccessFlags::UseSupervisorPrivileges);
    }
  }
  else
  {
    for (u32 i = 0; i < countof(current_task_state.words); i++)
    {
      SafeReadMemoryWord(current_tss_address + i * sizeof(u16), &current_task_state.words[i],
                         AccessFlags::UseSupervisorPrivileges);
    }
  }

  // Read the new TSS in, this could cause a page fault
  if (new_task_is_32bit)
  {
    for (u32 i = 0; i < countof(new_task_state.dwords); i++)
    {
      SafeReadMemoryDWord(new_tss_address + i * sizeof(u32), &new_task_state.dwords[i],
                          AccessFlags::UseSupervisorPrivileges);
    }
  }
  else
  {
    for (u32 i = 0; i < countof(new_task_state.words); i++)
    {
      SafeReadMemoryWord(new_tss_address + i * sizeof(u16), &new_task_state.words[i],
                         AccessFlags::UseSupervisorPrivileges);
    }
  }

  // The outgoing task should have the NT bit cleared when switching from IRET
  if (from_iret)
    m_registers.EFLAGS.NT = false;

  // Update the current task's TSS, and write it back to memory
  if (current_task_is_32bit)
  {
    current_task_state.ts32.CR3 = m_registers.CR3;
    current_task_state.ts32.EIP = m_registers.EIP;
    current_task_state.ts32.EFLAGS = m_registers.EFLAGS.bits;
    current_task_state.ts32.EAX = m_registers.EAX;
    current_task_state.ts32.ECX = m_registers.ECX;
    current_task_state.ts32.EDX = m_registers.EDX;
    current_task_state.ts32.EBX = m_registers.EBX;
    current_task_state.ts32.ESP = m_registers.ESP;
    current_task_state.ts32.EBP = m_registers.EBP;
    current_task_state.ts32.ESI = m_registers.ESI;
    current_task_state.ts32.EDI = m_registers.EDI;
    current_task_state.ts32.ES = m_registers.ES;
    current_task_state.ts32.CS = m_registers.CS;
    current_task_state.ts32.SS = m_registers.SS;
    current_task_state.ts32.DS = m_registers.DS;
    current_task_state.ts32.FS = m_registers.FS;
    current_task_state.ts32.GS = m_registers.GS;
    current_task_state.ts32.LDTR = m_registers.LDTR;
    for (u32 i = 0; i < countof(current_task_state.dwords); i++)
    {
      SafeWriteMemoryDWord(current_tss_address + i * sizeof(u32), current_task_state.dwords[i],
                           AccessFlags::UseSupervisorPrivileges);
    }
  }
  else
  {
    current_task_state.ts16.IP = m_registers.IP;
    current_task_state.ts16.FLAGS = m_registers.FLAGS;
    current_task_state.ts16.AX = m_registers.AX;
    current_task_state.ts16.CX = m_registers.CX;
    current_task_state.ts16.DX = m_registers.DX;
    current_task_state.ts16.BX = m_registers.BX;
    current_task_state.ts16.SP = m_registers.SP;
    current_task_state.ts16.BP = m_registers.BP;
    current_task_state.ts16.SI = m_registers.SI;
    current_task_state.ts16.DI = m_registers.DI;
    current_task_state.ts16.ES = m_registers.ES;
    current_task_state.ts16.CS = m_registers.CS;
    current_task_state.ts16.SS = m_registers.SS;
    current_task_state.ts16.DS = m_registers.DS;
    current_task_state.ts16.LDTR = m_registers.LDTR;
    for (u32 i = 0; i < countof(current_task_state.words); i++)
    {
      SafeWriteMemoryWord(current_tss_address + i * sizeof(u16), current_task_state.words[i],
                          AccessFlags::UseSupervisorPrivileges);
    }
  }

  // If we're a nested task, set the backlink field in the new TSS
  if (nested_task)
  {
    new_task_state.link = current_task_selector.bits;
    SafeWriteMemoryWord(new_tss_address, new_task_state.words[0], AccessFlags::UseSupervisorPrivileges);
  }

  // New task is now busy
  if (!from_iret)
  {
    new_task_descriptor.type =
      (new_task_is_32bit) ? DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32 : DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16;
    if (!WriteDescriptorEntry(new_task_descriptor, m_gdt_location, new_task_selector.index))
      Panic("Failed to update descriptor entry that was successfully read");
  }

  // Update TR with the new task
  m_registers.TR = new_task;
  m_tss_location.base_address = new_task_descriptor.tss.GetBase();
  m_tss_location.limit = new_task_descriptor.tss.GetLimit();
  m_tss_location.type = static_cast<DESCRIPTOR_TYPE>(new_task_descriptor.type.GetValue());

  // Load registers from TSS.
  u32 new_EIP;
  if (new_task_is_32bit)
  {
    // CR3 is only loaded when paging is enabled.
    if (IsPagingEnabled())
      LoadSpecialRegister(Reg32_CR3, new_task_state.ts32.CR3);

    m_registers.EAX = new_task_state.ts32.EAX;
    m_registers.ECX = new_task_state.ts32.ECX;
    m_registers.EDX = new_task_state.ts32.EDX;
    m_registers.EBX = new_task_state.ts32.EBX;
    m_registers.ESP = new_task_state.ts32.ESP;
    m_registers.EBP = new_task_state.ts32.EBP;
    m_registers.ESI = new_task_state.ts32.ESI;
    m_registers.EDI = new_task_state.ts32.EDI;
    m_registers.LDTR = new_task_state.ts32.LDTR;
    m_registers.CS = new_task_state.ts32.CS;
    m_registers.ES = new_task_state.ts32.ES;
    m_registers.SS = new_task_state.ts32.SS;
    m_registers.DS = new_task_state.ts32.DS;
    m_registers.FS = new_task_state.ts32.FS;
    m_registers.GS = new_task_state.ts32.GS;
    new_EIP = m_registers.EIP = new_task_state.ts32.EIP;

    // We have to bring in the V8086 flag here.
    m_registers.EFLAGS.VM = (new_task_state.ts32.EFLAGS & Flag_VM) != 0;
    SetFlags(new_task_state.ts32.EFLAGS);
  }
  else
  {
    m_registers.EAX = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.AX);
    m_registers.ECX = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.CX);
    m_registers.EDX = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.DX);
    m_registers.EBX = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.BX);
    m_registers.ESP = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.SP);
    m_registers.EBP = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.BP);
    m_registers.ESI = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.SI);
    m_registers.EDI = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.DI);
    m_registers.LDTR = new_task_state.ts16.LDTR;
    m_registers.CS = new_task_state.ts16.CS;
    m_registers.ES = new_task_state.ts16.ES;
    m_registers.SS = new_task_state.ts16.SS;
    m_registers.DS = new_task_state.ts16.DS;
    m_registers.FS = 0;
    m_registers.GS = 0;
    new_EIP = m_registers.EIP = ZeroExtend32(new_task_state.ts16.IP);

    // Keep upper bits of flags. TODO: Is this correct?
    SetFlags((m_registers.EFLAGS.bits & UINT32_C(0xFFFF0000)) | ZeroExtend32(new_task_state.ts16.FLAGS));
  }

  // Set NT flag if we're nesting
  if (nested_task)
    m_registers.EFLAGS.NT = true;
  else if (!from_iret)
    m_registers.EFLAGS.NT = false;

  // TS flag in CR0 should always be set.
  m_registers.CR0 |= CR0Bit_TS;

  // Update the previous EIP/ESP values.
  // If the push below results in an exception, it should use the register values from the incoming
  // task, as the outgoing task's registers are meaningless now.
  m_current_EIP = new_EIP;
  m_current_ESP = m_registers.ESP;

  // Checks that should be performed:
  // http://www.intel.com/design/pentium/MANUALS/24143004.pdf page 342.
  // Also note:
  //   Any errors detected in this step occur in the context of the new task. To an exception handler, the first
  //   instruction of the new task appears not to have executed.
  //
  // We reorder some of the steps here for simplicity, and it is implementation-defined anyway.
  // If we hit an exception here, the system will be in a pretty bad state anyway. "If an unrecoverable error occurs in
  // step 12, architectural state may be corrupted, but an attempt will be made to handle the error in the prior
  // execution environment." (Intel SDM Vol. 3A, 7-10)

  // (5) LDTR is the first which is loaded/validated, since the others depend on it.
  // (10) LDT of new task is present in memory.
  SEGMENT_SELECTOR_VALUE ldt_selector = {m_registers.LDTR};
  DESCRIPTOR_ENTRY ldt_descriptor;
  if (!ldt_selector.IsNullSelector() &&
      (ldt_selector.ti || !ReadDescriptorEntry(&ldt_descriptor, m_gdt_location, ldt_selector.index) ||
       !ldt_descriptor.IsPresent() || !ldt_descriptor.IsSystemSegment() || ldt_descriptor.type != DESCRIPTOR_TYPE_LDT))
  {
    RaiseException(Interrupt_InvalidTaskStateSegment, ldt_selector.GetExceptionErrorCode(false));
    return;
  }
  m_ldt_location.base_address = ldt_descriptor.ldt.GetBase();
  m_ldt_location.limit = ldt_descriptor.ldt.GetLimit();

  // None of this applies in V86 mode.
  if (!InVirtual8086Mode())
  {
    // (6) Code segment DPL matches selector RPL.
    // NOTE: This doesn't mention conforming code segments. We'll handle them anyway.
    SEGMENT_SELECTOR_VALUE segment_selectors[Segment_Count] = {{m_registers.ES}, {m_registers.CS}, {m_registers.SS},
                                                               {m_registers.DS}, {m_registers.FS}, {m_registers.GS}};
    DESCRIPTOR_ENTRY segment_descriptors[Segment_Count];
    const SEGMENT_SELECTOR_VALUE& cs_selector = segment_selectors[Segment_CS];
    DESCRIPTOR_ENTRY& cs_descriptor = segment_descriptors[Segment_CS];
    if (cs_selector.IsNullSelector() || !ReadDescriptorEntry(&cs_descriptor, cs_selector) ||
        (!cs_descriptor.IsConformingCodeSegment() && cs_selector.rpl != cs_descriptor.dpl))
    {
      RaiseException(Interrupt_InvalidTaskStateSegment, cs_selector.GetExceptionErrorCode(false));
      return;
    }

    // (7) SS segment is valid.
    const SEGMENT_SELECTOR_VALUE& ss_selector = segment_selectors[Segment_SS];
    DESCRIPTOR_ENTRY& ss_descriptor = segment_descriptors[Segment_SS];
    if (ss_selector.IsNullSelector() || !ReadDescriptorEntry(&ss_descriptor, ss_selector) ||
        !ss_descriptor.IsWritableDataSegment())
    {
      RaiseException(Interrupt_InvalidTaskStateSegment, ss_selector.GetExceptionErrorCode(false));
      return;
    }

    // (8) Stack segment is present in memory.
    if (!ss_descriptor.IsPresent())
    {
      RaiseException(Interrupt_StackFault, ss_selector.GetExceptionErrorCode(false));
      return;
    }

    // (9) Stack segment DPL matches CPL.
    if (ss_descriptor.dpl != cs_selector.rpl)
    {
      RaiseException(Interrupt_InvalidTaskStateSegment, ss_selector.GetExceptionErrorCode(false));
      return;
    }

    // (11) CS selector is valid.
    if (!cs_descriptor.IsCodeSegment())
    {
      RaiseException(Interrupt_InvalidTaskStateSegment, cs_selector.GetExceptionErrorCode(false));
      return;
    }

    // (12) Code segment is present in memory.
    if (!cs_descriptor.IsPresent())
    {
      RaiseException(Interrupt_SegmentNotPresent, cs_selector.GetExceptionErrorCode(false));
      return;
    }

    // (13) Stack segment DPL matches selector RPL.
    if (ss_descriptor.dpl != ss_selector.rpl)
    {
      RaiseException(Interrupt_InvalidTaskStateSegment, ss_selector.GetExceptionErrorCode(false));
      return;
    }

    // At this point, CS and SS are safe to load. This way we can get the updated CPL.
    LoadSegmentRegister(Segment_CS, cs_selector.bits);
    LoadSegmentRegister(Segment_SS, ss_selector.bits);

    // (14) DS, ES, FS, GS selectors are valid.
    static const Segment data_segments[] = {Segment_DS, Segment_ES, Segment_FS, Segment_GS};
    for (Segment segment : data_segments)
    {
      const SEGMENT_SELECTOR_VALUE& selector = segment_selectors[segment];
      DESCRIPTOR_ENTRY& descriptor = segment_descriptors[segment];
      if (!selector.IsNullSelector() && !ReadDescriptorEntry(&descriptor, selector))
      {
        RaiseException(Interrupt_InvalidTaskStateSegment, selector.GetExceptionErrorCode(false));
        return;
      }
    }

    // (14) DS, ES, FS, GS segments are readable.
    for (Segment segment : data_segments)
    {
      const SEGMENT_SELECTOR_VALUE& selector = segment_selectors[segment];
      const DESCRIPTOR_ENTRY& descriptor = segment_descriptors[segment];
      if (!selector.IsNullSelector() && !descriptor.IsReadableSegment())
      {
        RaiseException(Interrupt_InvalidTaskStateSegment, selector.GetExceptionErrorCode(false));
        return;
      }
    }

    // (15) DS, ES, FS, GS segments are present in memory.
    for (Segment segment : data_segments)
    {
      const SEGMENT_SELECTOR_VALUE& selector = segment_selectors[segment];
      const DESCRIPTOR_ENTRY& descriptor = segment_descriptors[segment];
      if (!selector.IsNullSelector() && !descriptor.IsPresent())
      {
        RaiseException(Interrupt_SegmentNotPresent, selector.GetExceptionErrorCode(false));
        return;
      }
    }

    // (15) DS, ES, FS, GS segment DPL greater than or equal to CPL (unless conforming).
    for (Segment segment : data_segments)
    {
      const SEGMENT_SELECTOR_VALUE& selector = segment_selectors[segment];
      const DESCRIPTOR_ENTRY& descriptor = segment_descriptors[segment];
      if (!selector.IsNullSelector() && descriptor.dpl < cs_selector.rpl && !descriptor.IsConformingCodeSegment())
      {
        RaiseException(Interrupt_InvalidTaskStateSegment, selector.GetExceptionErrorCode(false));
        return;
      }
    }

    // Now we can load the data segments. This should not fail.
    LoadSegmentRegister(Segment_DS, m_registers.DS);
    LoadSegmentRegister(Segment_ES, m_registers.ES);
    LoadSegmentRegister(Segment_FS, m_registers.FS);
    LoadSegmentRegister(Segment_GS, m_registers.GS);
  }
  else
  {
    // Switching to a V86 mode task.
    LoadSegmentRegister(Segment_CS, m_registers.CS);
    LoadSegmentRegister(Segment_SS, m_registers.SS);
    LoadSegmentRegister(Segment_DS, m_registers.DS);
    LoadSegmentRegister(Segment_ES, m_registers.ES);
    LoadSegmentRegister(Segment_FS, m_registers.FS);
    LoadSegmentRegister(Segment_GS, m_registers.GS);
  }

  // Push error codes for task switches on exceptions.
  if (push_error_code)
  {
    if (new_task_is_32bit)
      PushDWord(error_code);
    else
      PushWord(Truncate16(error_code));
  }

  // A task switch can result from an interrupt/exception.
  BranchFromException(new_EIP);
}

bool CPU::HasIOPermissions(u32 port_number, u32 port_count, bool raise_exceptions)
{
  // All access is permitted in real mode
  if (InRealMode())
    return true;

  // CPL>IOPL or V8086 must check IO permission map
  if (GetCPL() <= GetIOPL() && !InVirtual8086Mode())
    return true;

  // Check TSS validity
  if ((offsetof(TASK_STATE_SEGMENT_32, IOPB_offset) + (sizeof(u16) - 1)) > m_tss_location.limit)
    return false;

  // Get IOPB offset
  u16 iopb_offset;
  LinearMemoryAddress iopb_offset_address = m_tss_location.base_address + offsetof(TASK_STATE_SEGMENT_32, IOPB_offset);
  SafeReadMemoryWord(iopb_offset_address, &iopb_offset, AccessFlags::UseSupervisorPrivileges);

  // Find the offset in the IO bitmap
  u32 bitmap_byte_offset = port_number / 8;
  u32 bitmap_bit_offset = port_number % 8;

  // Check that it's not over a byte boundary
  bool spanning_byte = ((bitmap_bit_offset + port_count) > 8);
  u32 read_byte_count = spanning_byte ? 2 : 1;

  // Check against TSS limits
  if ((ZeroExtend32(iopb_offset) + (read_byte_count - 1)) > m_tss_location.limit)
    return false;

  // A value of 1 in the bitmap means no access
  u8 mask = ((1 << port_count) - 1);

  // Spanning a byte boundary?
  if (spanning_byte)
  {
    // Need to test against a word
    u16 permissions;
    SafeReadMemoryWord(m_tss_location.base_address + ZeroExtend32(iopb_offset) + bitmap_byte_offset, &permissions,
                       AccessFlags::UseSupervisorPrivileges);
    if (((permissions >> bitmap_bit_offset) & ZeroExtend16(mask)) != 0)
      return false;
  }
  else
  {
    // Test against the single byte
    u8 permissions;
    SafeReadMemoryByte(m_tss_location.base_address + ZeroExtend32(iopb_offset) + bitmap_byte_offset, &permissions,
                       AccessFlags::UseSupervisorPrivileges);
    if (((permissions >> bitmap_bit_offset) & mask) != 0)
      return false;
  }

  // IO operation is allowed
  return true;
}

bool CPU::IsVMEInterruptBitSet(u8 interrupt)
{
  // Check TSS validity
  if ((offsetof(TASK_STATE_SEGMENT_32, IOPB_offset) + (sizeof(u16) - 1)) > m_tss_location.limit)
    return false;

  // Get IOPB offset
  u16 iopb_offset;
  SafeReadMemoryWord(m_tss_location.base_address + offsetof(TASK_STATE_SEGMENT_32, IOPB_offset), &iopb_offset,
                     AccessFlags::UseSupervisorPrivileges);
  if (iopb_offset < 32)
    return false;

  // Compute offset to permission byte
  const u8 byte_number = interrupt / 8;
  const u8 bit_mask = (1u << (interrupt % 8));
  const u32 offset = ZeroExtend32(iopb_offset) - 32 + byte_number;
  if (offset > m_tss_location.limit)
    return false;

  u8 permission_bit;
  SafeReadMemoryByte(m_tss_location.base_address + offset, &permission_bit, AccessFlags::UseSupervisorPrivileges);

  // The bit *not* being set enables VME behavior.
  return ((permission_bit & bit_mask) == 0);
}

void CPU::CheckFloatingPointException()
{
  // Check and handle any pending floating point exceptions
  if (!m_fpu_registers.SW.IR)
    return;

  if (m_registers.CR0 & CR0Bit_NE)
  {
    RaiseException(Interrupt_MathFault);
    return;
  }

  // Compatibility mode via the PIC.
  m_interrupt_controller->TriggerInterrupt(13);
}

void CPU::LoadFPUState(Segment seg, VirtualMemoryAddress addr, VirtualMemoryAddress addr_mask, bool is_32bit,
                       bool load_registers)
{
  u16 cw = 0;
  u16 sw = 0;
  u16 tw = 0;
  u32 fip = 0;
  u16 fcs = 0;
  u32 fdp = 0;
  u16 fds = 0;
  u32 fop = 0;

  if (is_32bit)
  {
    cw = ReadSegmentMemoryWord(seg, (addr + 0) & addr_mask);
    sw = ReadSegmentMemoryWord(seg, (addr + 4) & addr_mask);
    tw = ReadSegmentMemoryWord(seg, (addr + 8) & addr_mask);
    if (InProtectedMode())
    {
      fip = ReadSegmentMemoryDWord(seg, (addr + 12) & addr_mask);
      u32 temp = ReadSegmentMemoryDWord(seg, (addr + 16) & addr_mask);
      fop = (temp >> 16) & 0x7FF;
      fcs = Truncate16(temp & 0xFFFF);
      fdp = ReadSegmentMemoryDWord(seg, (addr + 20) & addr_mask);
      fds = ReadSegmentMemoryWord(seg, (addr + 24) & addr_mask);
    }
    else
    {
      fip = ZeroExtend32(ReadSegmentMemoryWord(seg, (addr + 12) & addr_mask));
      u32 temp = ReadSegmentMemoryDWord(seg, (addr + 16) & addr_mask);
      fop = Truncate16(temp) & 0x7FF;
      fip |= ZeroExtend32(Truncate16(temp >> 11)) << 16;
      fdp = ZeroExtend32(ReadSegmentMemoryWord(seg, (addr + 20) & addr_mask));
      fdp |= ZeroExtend32(Truncate16(ReadSegmentMemoryWord(seg, (addr + 24) & addr_mask) >> 11)) << 16;
    }
    addr += 28;
  }
  else
  {
    cw = ReadSegmentMemoryWord(seg, (addr + 0) & addr_mask);
    sw = ReadSegmentMemoryWord(seg, (addr + 2) & addr_mask);
    tw = ReadSegmentMemoryWord(seg, (addr + 4) & addr_mask);
    if (InProtectedMode() && !InVirtual8086Mode())
    {
      fip = ZeroExtend32(ReadSegmentMemoryWord(seg, (addr + 6) & addr_mask));
      fcs = ReadSegmentMemoryWord(seg, (addr + 8) & addr_mask);
      fdp = ZeroExtend32(ReadSegmentMemoryWord(seg, (addr + 10) & addr_mask));
      fds = ReadSegmentMemoryWord(seg, (addr + 12) & addr_mask);
    }
    else
    {
      fip = ZeroExtend32(ReadSegmentMemoryWord(seg, (addr + 6) & addr_mask));
      u16 temp = ReadSegmentMemoryWord(seg, (addr + 8) & addr_mask);
      fop = temp & 0x7FF;
      fip |= (ZeroExtend32(temp >> 12) & 0xF) << 16;
      fdp = ZeroExtend32(ReadSegmentMemoryWord(seg, (addr + 10) & addr_mask));
      fdp |= (ZeroExtend32(ReadSegmentMemoryWord(seg, (addr + 12) & addr_mask) >> 12) & 0xF) << 16;
    }
    addr += 14;
  }

  m_fpu_registers.CW.bits = (cw & ~FPUControlWord::ReservedBits) | FPUControlWord::FixedBits;
  m_fpu_registers.SW.bits = sw;
  m_fpu_registers.TW.bits = tw;
  UpdateFPUSummaryException();

  if (!load_registers)
    return;

  for (u32 i = 0; i < 8; i++)
  {
    m_fpu_registers.ST[i].low = ZeroExtend64(ReadSegmentMemoryDWord(seg, (addr + 0) & addr_mask));
    m_fpu_registers.ST[i].low |= ZeroExtend64(ReadSegmentMemoryDWord(seg, (addr + 4) & addr_mask)) << 32;
    m_fpu_registers.ST[i].high = ReadSegmentMemoryWord(seg, (addr + 8) & addr_mask);
    addr += 10;
  }
}

void CPU::StoreFPUState(Segment seg, VirtualMemoryAddress addr, VirtualMemoryAddress addr_mask, bool is_32bit,
                        bool store_registers)
{
  const u16 cw = m_fpu_registers.CW.bits;
  const u16 sw = m_fpu_registers.SW.bits;
  const u16 tw = m_fpu_registers.TW.bits;
  const u32 fip = 0;
  const u16 fcs = 0;
  const u32 fdp = 0;
  const u16 fds = 0;
  const u32 fop = 0;

  if (is_32bit)
  {
    WriteSegmentMemoryWord(seg, (addr + 0) & addr_mask, cw);
    WriteSegmentMemoryWord(seg, (addr + 4) & addr_mask, sw);
    WriteSegmentMemoryWord(seg, (addr + 8) & addr_mask, tw);
    if (InProtectedMode())
    {
      WriteSegmentMemoryDWord(seg, (addr + 12) & addr_mask, fip);
      WriteSegmentMemoryDWord(seg, (addr + 16) & addr_mask, ZeroExtend32(fcs) | ((fop & 0x7FF) << 16));
      WriteSegmentMemoryDWord(seg, (addr + 20) & addr_mask, fdp);
      WriteSegmentMemoryWord(seg, (addr + 24) & addr_mask, fds);
    }
    else
    {
      WriteSegmentMemoryWord(seg, (addr + 12) & addr_mask, Truncate16(fip));
      WriteSegmentMemoryDWord(seg, (addr + 16) & addr_mask, (fop & 0x7FF) | ((fip >> 16) << 11));
      WriteSegmentMemoryWord(seg, (addr + 20) & addr_mask, Truncate16(fdp));
      WriteSegmentMemoryWord(seg, (addr + 24) & addr_mask, ((fdp >> 16) << 11));
    }

    addr += 28;
  }
  else
  {
    WriteSegmentMemoryWord(seg, (addr + 0) & addr_mask, cw);
    WriteSegmentMemoryWord(seg, (addr + 2) & addr_mask, sw);
    WriteSegmentMemoryWord(seg, (addr + 4) & addr_mask, tw);
    if (InProtectedMode() && !InVirtual8086Mode())
    {
      WriteSegmentMemoryWord(seg, (addr + 6) & addr_mask, Truncate16(fip));
      WriteSegmentMemoryWord(seg, (addr + 8) & addr_mask, fcs);
      WriteSegmentMemoryWord(seg, (addr + 10) & addr_mask, Truncate16(fdp));
      WriteSegmentMemoryWord(seg, (addr + 12) & addr_mask, fds);
    }
    else
    {
      WriteSegmentMemoryWord(seg, (addr + 6) & addr_mask, Truncate16(fip));
      WriteSegmentMemoryWord(seg, (addr + 8) & addr_mask, (fop & 0x7FF) | (((fip >> 16) & 0xF) << 12));
      WriteSegmentMemoryWord(seg, (addr + 10) & addr_mask, Truncate16(fdp));
      WriteSegmentMemoryWord(seg, (addr + 12) & addr_mask, (((fdp >> 16) & 0xF) << 12));
    }

    addr += 14;
  }

  if (!store_registers)
    return;

  // Save each of the registers out
  for (u32 i = 0; i < 8; i++)
  {
    WriteSegmentMemoryDWord(seg, (addr + 0) & addr_mask, Truncate32(m_fpu_registers.ST[i].low));
    WriteSegmentMemoryDWord(seg, (addr + 4) & addr_mask, Truncate32(m_fpu_registers.ST[i].low >> 32));
    WriteSegmentMemoryWord(seg, (addr + 8) & addr_mask, m_fpu_registers.ST[i].high);
    addr += 10;
  }
}

void CPU::UpdateFPUSummaryException()
{
  u16 unmasked_exceptions = (m_fpu_registers.SW.bits & 0x3F) & (m_fpu_registers.CW.bits ^ u16(0x3F));
  if (unmasked_exceptions != 0)
    m_fpu_registers.SW.IR = m_fpu_registers.SW.B = true;
  else
    m_fpu_registers.SW.IR = m_fpu_registers.SW.B = false;
}

void CPU::ExecuteCPUIDInstruction()
{
  enum CPUID_FLAG : u32
  {
    CPUID_FLAG_FPU = (1 << 0),
    CPUID_FLAG_VME = (1 << 1),
    CPUID_FLAG_DE = (1 << 2), // Debug extensions (I/O Breakpoints)
    CPUID_FLAG_PSE = (1 << 3),
    CPUID_FLAG_TSC = (1 << 4),
    CPUID_FLAG_MSR = (1 << 5),
    CPUID_FLAG_PAE = (1 << 6),
    CPUID_FLAG_CX8 = (1 << 8),
    CPUID_FLAG_APIC = (1 << 9),
    CPUID_FLAG_SYSENTER = (1 << 11),
    CPUID_FLAG_MTRR = (1 << 12),
    CPUID_FLAG_PTE_GLOBAL = (1 << 13),
    CPUID_FLAG_MCA = (1 << 14), // Machine check architecture
    CPUID_FLAG_CMOV = (1 << 15),
    CPUID_FLAG_PAT = (1 << 16), // Page attribute table
    CPUID_FLAG_PSE36 = (1 << 17),
    CPUID_FLAG_PSN = (1 << 18),
    CPUID_FLAG_CLFLUSH = (1 << 19),
    CPUID_FLAG_DS = (1 << 21), // Debug store
    CPUID_FLAG_ACPI = (1 << 22),
    CPUID_FLAG_MMX = (1 << 23),
    CPUID_FLAG_FXSR = (1 << 24),
    CPUID_FLAG_SSE = (1 << 25),
    CPUID_FLAG_SSE2 = (1 << 26)
  };

  Log_DebugPrintf("Executing CPUID with EAX=%08X", m_registers.EAX);

  switch (m_registers.EAX)
  {
    case 0:
    {
      m_registers.EAX = UINT32_C(0x00000001); // Maximum CPUID leaf supported.
      m_registers.EBX = UINT32_C(0x756E6547); // Genu
      m_registers.EDX = UINT32_C(0x49656E69); // ineI
      m_registers.ECX = UINT32_C(0x6C65746E); // ntel
    }
    break;

    case 1:
    {
      m_registers.EAX = GetCPUIDModel(m_model);
      m_registers.EBX = 0;
      m_registers.ECX = 0;

      switch (m_model)
      {
        case MODEL_486:
          m_registers.EDX = CPUID_FLAG_FPU /* | CPUID_FLAG_VME*/; // TODO: DX4 should support VME.
          break;

        case MODEL_PENTIUM:
          m_registers.EDX = CPUID_FLAG_FPU /*| CPUID_FLAG_DE */ | CPUID_FLAG_VME /*| CPUID_FLAG_PSE*/
                            | CPUID_FLAG_TSC | CPUID_FLAG_MSR |
                            /*| CPUID_FLAG_MCE */ CPUID_FLAG_CX8;
          break;
      }
    }
    break;

      //     case 2:
      //       {
      //         // Dummy cache info.
      //         m_registers.EAX = UINT32_C(0x03020101);
      //         m_registers.EBX = UINT32_C(0x00000000);
      //         m_registers.ECX = UINT32_C(0x00000000);
      //         m_registers.EDX = UINT32_C(0x0C040843);
      //       }
      //       break;

    default:
    {
      m_registers.EAX = 0;
      m_registers.EBX = 0;
      m_registers.ECX = 0;
      m_registers.EDX = 0;
    }
    break;
  }
}

u64 CPU::ReadMSR(u32 index)
{
  if (m_model == MODEL_PENTIUM)
  {
    switch (index)
    {
      case MSR_TR1:
        return ZeroExtend64(m_msr_registers.pentium.tr1);

      case MSR_TR12:
        return ZeroExtend64(m_msr_registers.pentium.tr12);

      case MSR_TSC:
        return ReadTSC();

      default:
        break;
    }
  }

  Log_WarningPrintf("Unhandled MSR read: 0x%08X", index);
  RaiseException(Interrupt_GeneralProtectionFault, 0);
  return 0;
}

void CPU::WriteMSR(u32 index, u64 value)
{
  if (m_model == MODEL_PENTIUM)
  {
    switch (index)
    {
      case MSR_TR1:
        m_msr_registers.pentium.tr1 = Truncate32(value);
        return;

      case MSR_TR12:
        m_msr_registers.pentium.tr12 = Truncate32(value);
        return;

      case MSR_TSC:
        CommitPendingCycles();
        m_tsc_cycles = value;
        return;

      default:
        break;
    }
  }

  Log_WarningPrintf("Unhandled MSR write: 0x%08X <- %" PRIx64, index, value);
  RaiseException(Interrupt_GeneralProtectionFault, 0);
}

void CPU::DumpPageTable()
{
  std::fprintf(stderr, "Page table\n");
  if (!InProtectedMode() && !(m_registers.CR0 & CR0Bit_PE))
    return;

  PhysicalMemoryAddress directory_address = (m_registers.CR3 & 0xFFFFF000);
  for (u32 i = 0; i < 1024; i++)
  {
    PhysicalMemoryAddress dir_entry_address = directory_address + (i * sizeof(PAGE_DIRECTORY_ENTRY));
    PAGE_DIRECTORY_ENTRY dir_entry;
    dir_entry.bits = m_bus->ReadMemoryDWord(dir_entry_address);
    if (!dir_entry.present)
      continue;

    LinearMemoryAddress table_start_linaddr = (i << 22);
    PhysicalMemoryAddress table_address = (dir_entry.page_table_address << 12);

    LinearMemoryAddress page_start_linaddr = 0;
    PhysicalMemoryAddress page_start_physaddr = 0;
    PhysicalMemoryAddress page_next_physaddr = 0;
    u32 page_count = 0;
    for (u32 j = 0; j < 1024; j++)
    {
      PhysicalMemoryAddress table_entry_address = table_address + (j * sizeof(PAGE_TABLE_ENTRY));
      PAGE_TABLE_ENTRY table_entry;
      table_entry.bits = m_bus->ReadMemoryDWord(table_entry_address);

      if (!table_entry.present || table_entry.physical_address != page_next_physaddr)
      {
        if (page_count > 0)
        {
          std::fprintf(stderr, "Linear 0x%08X - 0x%08X -> Physical 0x%08X - 0x%08X\n", page_start_linaddr,
                       page_start_linaddr + (page_count * PAGE_SIZE) - 1, page_start_physaddr,
                       page_start_physaddr + (page_count * PAGE_SIZE) - 1);
          page_count = 0;
        }
      }

      if (!table_entry.present)
        continue;

      if (page_count == 0)
      {
        page_start_linaddr = table_start_linaddr + (j << 12);
        page_start_physaddr = (table_entry.physical_address << 12);
        page_next_physaddr = page_start_physaddr;
      }
      page_next_physaddr += PAGE_SIZE;
      page_count++;
    }

    if (page_count > 0)
    {
      std::fprintf(stderr, "Linear 0x%08X - 0x%08X -> Physical 0x%08X - 0x%08X\n", page_start_linaddr,
                   page_start_linaddr + (page_count * PAGE_SIZE) - 1, page_start_physaddr,
                   page_start_physaddr + (page_count * PAGE_SIZE) - 1);
    }
  }
}

void CPU::DumpMemory(LinearMemoryAddress start_address, u32 size)
{
  DebugAssert(size > 0);
  LinearMemoryAddress end_address = start_address + size - 1;
  std::fprintf(stderr, "Memory dump from 0x%08X - 0x%08X\n", start_address, end_address);

  LinearMemoryAddress current_address = start_address;
  while (current_address <= end_address)
  {
    static constexpr u32 COLUMNS = 16;
    LinearMemoryAddress row_address = current_address;

    SmallString hex;
    TinyString printable;

    for (u32 i = 0; i < COLUMNS; i++)
    {
      u8 value;
      if (!SafeReadMemoryByte(current_address++, &value, AccessFlags::Debugger))
      {
        std::fprintf(stderr, "0x%08X | %s| %s |", row_address, hex.GetCharArray(), printable.GetCharArray());
        std::fprintf(stderr, "Failed memory read at 0x%08X\n", current_address);
        return;
      }

      hex.AppendFormattedString("%02X ", ZeroExtend32(value));
      if (std::isprint(value))
        printable.AppendCharacter(value);
      else
        printable.AppendCharacter('.');
    }

    std::fprintf(stderr, "0x%08X | %s| %s |\n", row_address, hex.GetCharArray(), printable.GetCharArray());
  }
}

void CPU::DumpStack()
{
  LinearMemoryAddress stack_top = m_current_ESP;
  LinearMemoryAddress stack_bottom = 0xFFFFFFFF;
  if (m_stack_address_size == AddressSize_16)
  {
    stack_top &= 0xFFFF;
    stack_bottom &= 0xFFFF;
  }

  std::fprintf(stderr, "Stack dump, ESP = 0x%08X (linear 0x%08X)\n", stack_top,
               CalculateLinearAddress(Segment_SS, stack_top));

  stack_top = CalculateLinearAddress(Segment_SS, stack_top);
  stack_bottom = CalculateLinearAddress(Segment_SS, stack_bottom);

  LinearMemoryAddress stack_address = stack_top;
  for (u32 count = 0; count < 128; count++)
  {
    if (m_stack_address_size == AddressSize_16)
    {
      u16 value;
      if (!SafeReadMemoryWord(stack_address, &value, AccessFlags::Debugger))
        break;
      std::fprintf(stderr, "  0x%04X: 0x%04X\n", stack_address, ZeroExtend32(value));
    }
    else
    {
      u32 value;
      if (!SafeReadMemoryDWord(stack_address, &value, AccessFlags::Debugger))
        break;
      std::fprintf(stderr, "  0x%08X: 0x%08X\n", stack_address, value);
    }

    if (stack_address == stack_bottom)
      break;

    if (m_stack_address_size == AddressSize_16)
      stack_address += sizeof(u16);
    else
      stack_address += sizeof(u32);
  }
}

size_t CPU::GetTLBEntryIndex(u32 linear_address)
{
  // Maybe a better hash function should be used here?
  return size_t(linear_address >> 12) % TLB_ENTRY_COUNT;
}

void CPU::InvalidateAllTLBEntries(bool force_clear /* = false */)
{
#ifdef ENABLE_TLB_EMULATION
  m_tlb_counter_bits++;
  Log_DebugPrintf("Invaliding TLB entries, counter=0x%03X", m_tlb_counter_bits);
  if (m_tlb_counter_bits == 0xFFF || force_clear)
  {
    std::memset(m_tlb_entries, 0xFF, sizeof(m_tlb_entries));
    m_tlb_counter_bits = 0;
  }
#endif
}

void CPU::InvalidateTLBEntry(u32 linear_address)
{
#ifdef ENABLE_TLB_EMULATION
  const u32 compare_linear_address = (linear_address & PAGE_MASK) | m_tlb_counter_bits;
  const size_t index = GetTLBEntryIndex(linear_address);
  for (u32 user_supervisor = 0; user_supervisor < 2; user_supervisor++)
  {
    for (u32 write_read = 0; write_read < 2; write_read++)
    {
      TLBEntry& entry = m_tlb_entries[user_supervisor][write_read][index];
      if (entry.linear_address == compare_linear_address)
        entry.linear_address = 0xFFFFFFFF;
    }
  }
#endif
}

void CPU::FlushPrefetchQueue()
{
#ifdef ENABLE_PREFETCH_EMULATION
  m_prefetch_queue_position = 0;
  m_prefetch_queue_size = 0;
#endif
}

bool CPU::FillPrefetchQueue()
{
#ifdef ENABLE_PREFETCH_EMULATION
  m_prefetch_queue_position = 0;
  m_prefetch_queue_size = 0;

  // Currently can't handle wrap-around.
  if ((m_registers.EIP & ~m_EIP_mask) != ((m_registers.EIP + PREFETCH_QUEUE_SIZE - 1) & ~m_EIP_mask))
    return false;

  // If fetching the whole queue puts us outside the segment limits, don't bother with prefetching.
  if (!CheckSegmentAccess<PREFETCH_QUEUE_SIZE, AccessType::Execute>(Segment_CS, m_registers.EIP, false))
    return false;

  // Currently we can't handle crossing pages here.
  LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, m_registers.EIP);
  if ((linear_address & PAGE_MASK) != ((linear_address + PREFETCH_QUEUE_SIZE - 1) & PAGE_MASK))
    return false;

  // Translate away.
  PhysicalMemoryAddress physical_address;
  if (!TranslateLinearAddress(&physical_address, linear_address, AccessFlags::Normal | AccessFlags::NoPageFaults))
  {
    // Use direct fetch and page fault when it fails.
    return false;
  }

  m_bus->ReadMemoryBlock(physical_address, PREFETCH_QUEUE_SIZE, m_prefetch_queue);
  m_prefetch_queue_size = PREFETCH_QUEUE_SIZE;
  return true;
#else
  return false;
#endif
}

#if 0
float80* CPU::GetFloatRegister(uint8 relative_index)
{
  uint8 index = (m_fpu_registers.SW.TOP + relative_index) % 8;
  return &m_fpu_registers.ST[index];
}

void CPU::PushFloat(const float80& value, bool update_tag)
{
  uint8 top = m_fpu_registers.SW.TOP;
  if (top == 7)
  {
    m_fpu_registers.SW.C1 = 1;
    m_fpu_registers.SW.SF = true;
    m_fpu_registers.SW.I = true;
    AbortCurrentInstruction();
    return;
  }

  std::memcpy(&m_fpu_registers.ST[top], &value, sizeof(float80));
  if (update_tag)
    UpdateFloatTag(top);

  m_fpu_registers.SW.TOP = (top - 1) & 7;
}

void CPU::UpdateFloatTag(uint8 index)
{
  if ((value.high & 0x7fff) == 0 && value.low == 0)
  {
    m_fpu_registers.TW.se
  }
}

void CPU::PopFloat()
{
  uint8 top = m_fpu_registers.SW.TOP;
  if (top == 0)
  {
    m_fpu_registers.SW.C1 = 0;
    m_fpu_registers.SW.SF = true;
    m_fpu_registers.SW.I = true;
    AbortCurrentInstruction();
  }

  m_fpu_registers.SW.TOP = top - 1;
}
#endif

} // namespace CPU_X86
