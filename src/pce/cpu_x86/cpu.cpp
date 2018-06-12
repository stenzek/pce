#include "pce/cpu_x86/cpu.h"
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
#include "pce/cpu_x86/jitx64_backend.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cctype>
Log_SetChannel(CPU_X86::CPU);

namespace CPU_X86 {

// Used by backends to enable tracing feature.
#ifdef Y_BUILD_CONFIG_RELEASE
bool TRACE_EXECUTION = false;
#else
bool TRACE_EXECUTION = false;
#endif
uint32 TRACE_EXECUTION_LAST_EIP = 0;

CPU::CPU(Model model, float frequency, CPUBackendType backend_type) : CPUBase(frequency, backend_type), m_model(model)
{
#ifdef ENABLE_TLB_EMULATION
  InvalidateAllTLBEntries();
#endif
}

CPU::~CPU() {}

void CPU::Initialize(System* system, Bus* bus)
{
  m_system = system;
  m_bus = bus;
  CreateBackend();
  Reset();
}

void CPU::Reset()
{
  m_pending_cycles = 0;
  m_execution_downcount = 0;

  Y_memzero(&m_registers, sizeof(m_registers));
  Y_memzero(&m_fpu_registers, sizeof(m_fpu_registers));

  // IOPL NT, reserved are 1 on 8086
  m_registers.EFLAGS.bits = 0;
  if (m_model == MODEL_8086)
  {
    m_registers.EFLAGS.bits |= Flag_IOPL;
    m_registers.EFLAGS.bits |= Flag_NT;
    m_registers.EFLAGS.bits |= (1 << 15);
  }
  m_registers.EFLAGS.bits |= Flag_Reserved;

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
  for (uint32 i = 0; i < Segment_Count; i++)
  {
    SegmentCache* segment = &m_segment_cache[i];
    segment->base_address = 0;
    segment->limit_low = 0;
    segment->limit_high = 0xFFFF;
    segment->limit_raw = 0xFFFF;
    segment->access.bits = 0;
    segment->access.present = true;
    segment->access.privilege = 0;
    segment->dpl = 0;
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
  LoadSegmentRegister(Segment_CS, 0xF000);
  m_registers.EIP = 0xFFF0;
  if (m_model >= MODEL_386)
  {
    // CS is F000 but the base is FFFF0000
    m_segment_cache[Segment_CS].base_address = 0xFFFF0000u;
  }

  // Protected mode off, FPU not present, cache disabled.
  m_registers.CR0 = 0;
  if (m_model >= MODEL_386)
    m_registers.CR0 |= CR0Bit_ET; // 80387 FPU
  if (m_model >= MODEL_486)
    m_registers.CR0 |= CR0Bit_CD | CR0Bit_NW | CR0Bit_ET;

  // Start at privilege level 0
  m_cpl = 0;
  m_tlb_user_supervisor_bit = 0;

  m_current_EIP = m_registers.EIP;
  m_current_ESP = m_registers.ESP;
  m_current_exception = Interrupt_Count;
  m_nmi_state = false;
  m_fpu_exception = false;
  m_halted = false;
  m_trap_after_instruction = false;

  // x87 state
  m_fpu_registers.CW.bits = 0x0040;
  m_fpu_registers.SW.bits = 0x0000;
  m_fpu_registers.TW.bits = 0x5555;

  InvalidateAllTLBEntries();
  FlushPrefetchQueue();

  m_effective_address = 0;
  std::memset(&idata, 0, sizeof(idata));

  m_backend->Reset();
}

bool CPU::LoadState(BinaryReader& reader)
{
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
  reader.SafeReadUInt32(&m_current_EIP);
  reader.SafeReadUInt32(&m_current_ESP);

  // reader.SafeReadBytes(&m_registers, sizeof(m_registers));
  for (uint32 i = 0; i < Reg32_Count; i++)
    reader.SafeReadUInt32(&m_registers.reg32[i]);
  for (uint32 i = 0; i < Segment_Count; i++)
    reader.SafeReadUInt16(&m_registers.segment_selectors[i]);
  reader.SafeReadUInt16(&m_registers.LDTR);
  reader.SafeReadUInt16(&m_registers.TR);

  for (uint32 i = 0; i < countof(m_fpu_registers.ST); i++)
  {
    reader.SafeReadUInt64(&m_fpu_registers.ST[i].low);
    reader.SafeReadUInt16(&m_fpu_registers.ST[i].high);
  }
  reader.SafeReadUInt16(&m_fpu_registers.CW.bits);
  reader.SafeReadUInt16(&m_fpu_registers.SW.bits);
  reader.SafeReadUInt16(&m_fpu_registers.TW.bits);

  uint8 current_address_size = 0;
  uint8 current_operand_size = 0;
  uint8 stack_address_size = 0;
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
  uint8 ts_type = 0;
  reader.SafeReadUInt8(&ts_type);
  m_tss_location.type = static_cast<DESCRIPTOR_TYPE>(ts_type);

  auto ReadSegmentCache = [&reader](SegmentCache* ptr) {
    reader.SafeReadUInt32(&ptr->base_address);
    reader.SafeReadUInt32(&ptr->limit_low);
    reader.SafeReadUInt32(&ptr->limit_high);
    reader.SafeReadUInt32(&ptr->limit_raw);
    reader.SafeReadUInt8(&ptr->access.bits);
    reader.SafeReadUInt8(reinterpret_cast<uint8*>(&ptr->access_mask));
    reader.SafeReadUInt8(&ptr->dpl);
  };
  for (uint32 i = 0; i < Segment_Count; i++)
    ReadSegmentCache(&m_segment_cache[i]);

  reader.SafeReadUInt8(&m_cpl);
  reader.SafeReadUInt8(&m_tlb_user_supervisor_bit);

  reader.SafeReadBool(&m_nmi_state);
  reader.SafeReadBool(&m_irq_state);
  reader.SafeReadBool(&m_fpu_exception);
  reader.SafeReadBool(&m_halted);

#ifdef ENABLE_TLB_EMULATION
  uint32 tlb_entry_count;
  if (!reader.SafeReadUInt32(&tlb_entry_count) || tlb_entry_count != Truncate32(TLB_ENTRY_COUNT))
    return false;
  for (uint32 user_supervisor = 0; user_supervisor < 2; user_supervisor++)
  {
    for (uint32 write_read = 0; write_read < 2; write_read++)
    {
      for (auto& entry : m_tlb_entries[user_supervisor][write_read])
      {
        reader.SafeReadUInt32(&entry.linear_address);
        reader.SafeReadUInt32(&entry.physical_address);
      }
    }
  }
#endif

#ifdef ENABLE_PREFETCH_EMULATION
  uint32 prefetch_queue_size;
  if (!reader.SafeReadUInt32(&prefetch_queue_size) || prefetch_queue_size != PREFETCH_QUEUE_SIZE)
    return false;
  reader.SafeReadBytes(m_prefetch_queue, PREFETCH_QUEUE_SIZE);
  reader.SafeReadUInt32(&m_prefetch_queue_position);
  reader.SafeReadUInt32(&m_prefetch_queue_size);
#endif

  reader.SafeReadUInt32(&m_effective_address);
  std::memset(&idata, 0, sizeof(idata));

  return !reader.GetErrorState();
}

bool CPU::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteUInt8(static_cast<uint8>(m_model));

  writer.WriteInt64(m_pending_cycles);
  writer.WriteInt64(m_execution_downcount);
  writer.WriteUInt32(m_current_EIP);
  writer.WriteUInt32(m_current_ESP);

  // writer.WriteBytes(&m_registers, sizeof(m_registers));
  for (uint32 i = 0; i < Reg32_Count; i++)
    writer.WriteUInt32(m_registers.reg32[i]);
  for (uint32 i = 0; i < Segment_Count; i++)
    writer.WriteUInt16(m_registers.segment_selectors[i]);
  writer.WriteUInt16(m_registers.LDTR);
  writer.WriteUInt16(m_registers.TR);

  for (uint32 i = 0; i < countof(m_fpu_registers.ST); i++)
  {
    writer.WriteUInt64(m_fpu_registers.ST[i].low);
    writer.WriteUInt16(m_fpu_registers.ST[i].high);
  }
  writer.WriteUInt16(m_fpu_registers.CW.bits);
  writer.WriteUInt16(m_fpu_registers.SW.bits);
  writer.WriteUInt16(m_fpu_registers.TW.bits);

  writer.WriteUInt8(static_cast<uint8>(m_current_address_size));
  writer.WriteUInt8(static_cast<uint8>(m_current_operand_size));
  writer.WriteUInt8(static_cast<uint8>(m_stack_address_size));
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
  writer.WriteUInt8(static_cast<uint8>(m_tss_location.type));

  auto WriteSegmentCache = [&writer](SegmentCache* ptr) {
    writer.WriteUInt32(ptr->base_address);
    writer.WriteUInt32(ptr->limit_low);
    writer.WriteUInt32(ptr->limit_high);
    writer.WriteUInt32(ptr->limit_raw);
    writer.WriteUInt8(ptr->access.bits);
    writer.WriteUInt8(static_cast<uint8>(ptr->access_mask));
    writer.WriteUInt8(ptr->dpl);
  };
  for (uint32 i = 0; i < Segment_Count; i++)
    WriteSegmentCache(&m_segment_cache[i]);

  writer.WriteUInt8(m_cpl);
  writer.WriteUInt8(m_tlb_user_supervisor_bit);

  writer.WriteBool(m_nmi_state);
  writer.WriteBool(m_irq_state);
  writer.WriteBool(m_fpu_exception);
  writer.WriteBool(m_halted);

#ifdef ENABLE_TLB_EMULATION
  writer.WriteUInt32(Truncate32(TLB_ENTRY_COUNT));
  for (uint32 user_supervisor = 0; user_supervisor < 2; user_supervisor++)
  {
    for (uint32 write_read = 0; write_read < 2; write_read++)
    {
      for (const auto& entry : m_tlb_entries[user_supervisor][write_read])
      {
        writer.WriteUInt32(entry.linear_address);
        writer.WriteUInt32(entry.physical_address);
      }
    }
  }
#endif

#ifdef ENABLE_PREFETCH_EMULATION
  writer.WriteUInt32(PREFETCH_QUEUE_SIZE);
  writer.WriteBytes(m_prefetch_queue, PREFETCH_QUEUE_SIZE);
  writer.WriteUInt32(m_prefetch_queue_position);
  writer.WriteUInt32(m_prefetch_queue_size);
#endif

  writer.WriteUInt32(m_effective_address);
  std::memset(&idata, 0, sizeof(idata));

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
  Log_DevPrintf("NMI line signaled");
  m_nmi_state = true;

  if (m_halted)
  {
    Log_DevPrintf("Bringing CPU up from halt due to NMI");
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

bool CPU::SupportsBackend(CPUBackendType mode)
{
  return (mode == CPUBackendType::Interpreter || mode == CPUBackendType::CachedInterpreter ||
          mode == CPUBackendType::Recompiler);
}

void CPU::SetBackend(CPUBackendType mode)
{
  m_backend_type = mode;

  // If we're initialized, switch backends now, otherwise wait until we have a system.
  if (m_system)
    CreateBackend();
}

void CPU::ExecuteCycles(CycleCount cycles)
{
  m_execution_downcount += cycles;

  while (m_system->GetState() == System::State::Running && !m_system->HasExternalEvents() && m_execution_downcount > 0)
  {
    // If we're halted, don't even bother calling into the backend.
    if (m_halted)
    {
      // Run as many ticks until we hit the downcount.
      SimulationTime time_to_execute =
        std::max(m_system->GetTimingManager()->GetNextEventTime() - m_system->GetTimingManager()->GetPendingTime(),
                 SimulationTime(0));
      time_to_execute = std::min(cycles * m_cycle_period, time_to_execute);

      // Align the execution time to the cycle period, this way we don't drift due to halt.
      time_to_execute += (m_cycle_period - (time_to_execute % m_cycle_period));
      if (time_to_execute > 0)
      {
        m_system->GetTimingManager()->AddPendingTime(time_to_execute);
        cycles -= time_to_execute / m_cycle_period;
      }

      m_execution_downcount = 0;
      continue;
    }

    // Execute instructions in the backend.
    m_backend->Execute();
  }
}

void CPU::CommitPendingCycles()
{
  m_execution_downcount -= m_pending_cycles;
  m_system->GetTimingManager()->AddPendingTime(m_pending_cycles * GetCyclePeriod());
  m_pending_cycles = 0;
}

void CPU::FlushCodeCache()
{
  m_backend->FlushCodeCache();
}

void CPU::CreateBackend()
{
  switch (m_backend_type)
  {
    case CPUBackendType::Interpreter:
      Log_InfoPrintf("Switching to interpreter backend.");
      m_backend = std::make_unique<InterpreterBackend>(this);
      break;

    case CPUBackendType::CachedInterpreter:
      Log_InfoPrintf("Switching to cached interpreter backend.");
      m_backend = std::make_unique<CachedInterpreterBackend>(this);
      break;

    case CPUBackendType::Recompiler:
      Log_InfoPrintf("Switching to recompiler backend.");
      m_backend = std::make_unique<JitX64Backend>(this);
      break;

    default:
      Log_ErrorPrintf("Unsupported backend type, falling back to interpreter.");
      m_backend_type = CPUBackendType::Interpreter;
      CreateBackend();
      break;
  }
}

uint8 CPU::GetCPL() const
{
  return m_cpl;
}

bool CPU::InSupervisorMode() const
{
  return (GetCPL() != 3);
}

bool CPU::InUserMode() const
{
  return (GetCPL() == 3);
}

bool CPU::IsPagingEnabled() const
{
  return ((m_registers.CR0 & CR0Bit_PG) != 0);
}

uint16 CPU::GetIOPL() const
{
  return Truncate16((m_registers.EFLAGS.bits & Flag_IOPL) >> 12);
}

bool CPU::InRealMode() const
{
  return ((m_registers.CR0 & CR0Bit_PE) == 0);
}

bool CPU::InProtectedMode() const
{
  return ((m_registers.CR0 & CR0Bit_PE) != 0);
}

bool CPU::InVirtual8086Mode() const
{
  return m_registers.EFLAGS.VM;
}

uint8 CPU::FetchInstructionByte()
{
#ifdef ENABLE_PREFETCH_EMULATION
  // It's possible this will still fail if we're at the end of the segment.
  uint8 value;
  if ((m_prefetch_queue_size - m_prefetch_queue_position) >= sizeof(uint8) || FillPrefetchQueue())
    value = m_prefetch_queue[m_prefetch_queue_position++];
  else
    value = FetchDirectInstructionByte(m_registers.EIP, true);

  m_registers.EIP = (m_registers.EIP + sizeof(uint8)) & m_EIP_mask;
  return value;
#else
  uint8 value = FetchDirectInstructionByte(m_registers.EIP, true);
  m_registers.EIP = (m_registers.EIP + sizeof(uint8)) & m_EIP_mask;
  return value;
#endif
}

uint16 CPU::FetchInstructionWord()
{
#ifdef ENABLE_PREFETCH_EMULATION
  // It's possible this will still fail if we're at the end of the segment.
  uint16 value;
  if ((m_prefetch_queue_size - m_prefetch_queue_position) >= sizeof(uint16) || FillPrefetchQueue())
  {
    std::memcpy(&value, &m_prefetch_queue[m_prefetch_queue_position], sizeof(uint16));
    m_prefetch_queue_position += sizeof(uint16);
  }
  else
  {
    value = FetchDirectInstructionWord(m_registers.EIP, true);
  }

  m_registers.EIP = (m_registers.EIP + sizeof(uint16)) & m_EIP_mask;
  return value;
#else
  uint16 value = FetchDirectInstructionWord(m_registers.EIP, true);
  m_registers.EIP = (m_registers.EIP + sizeof(uint16)) & m_EIP_mask;
  return value;
#endif
}

uint32 CPU::FetchInstructionDWord()
{
#ifdef ENABLE_PREFETCH_EMULATION
  // It's possible this will still fail if we're at the end of the segment.
  uint32 value;
  if ((m_prefetch_queue_size - m_prefetch_queue_position) >= sizeof(uint32) || FillPrefetchQueue())
  {
    std::memcpy(&value, &m_prefetch_queue[m_prefetch_queue_position], sizeof(uint32));
    m_prefetch_queue_position += sizeof(uint32);
  }
  else
  {
    value = FetchDirectInstructionDWord(m_registers.EIP, true);
  }

  m_registers.EIP = (m_registers.EIP + sizeof(uint32)) & m_EIP_mask;
  return value;
#else
  uint32 value = FetchDirectInstructionDWord(m_registers.EIP, true);
  m_registers.EIP = (m_registers.EIP + sizeof(uint32)) & m_EIP_mask;
  return value;
#endif
}

uint8 CPU::FetchDirectInstructionByte(uint32 address, bool raise_exceptions)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, address);
  if (raise_exceptions)
    CheckSegmentAccess<sizeof(uint8), AccessType::Execute>(Segment_CS, address, raise_exceptions);

  PhysicalMemoryAddress physical_address;
  TranslateLinearAddress(&physical_address, linear_address, raise_exceptions, AccessType::Execute, raise_exceptions);

  return m_bus->ReadMemoryByte(physical_address);
}

uint16 CPU::FetchDirectInstructionWord(uint32 address, bool raise_exceptions)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, address);

  // If it crosses a page, we have to fetch bytes instead.
  if ((linear_address & CPU::PAGE_MASK) != ((linear_address + sizeof(uint16) - 1) & CPU::PAGE_MASK))
  {
    uint32 mask = (m_current_address_size == AddressSize_16) ? 0xFFFF : 0xFFFFFFFF;
    uint8 lsb = FetchDirectInstructionByte(address, raise_exceptions);
    uint8 msb = FetchDirectInstructionByte((address + 1) & mask, raise_exceptions);
    return ZeroExtend16(lsb) | (ZeroExtend16(msb) << 8);
  }

  if (raise_exceptions)
    CheckSegmentAccess<sizeof(uint16), AccessType::Execute>(Segment_CS, address, raise_exceptions);

  PhysicalMemoryAddress physical_address;
  TranslateLinearAddress(&physical_address, linear_address, raise_exceptions, AccessType::Execute, raise_exceptions);
  return m_bus->ReadMemoryWord(physical_address);
}

uint32 CPU::FetchDirectInstructionDWord(uint32 address, bool raise_exceptions)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, address);

  // If it crosses a page, we have to fetch words instead.
  if ((linear_address & CPU::PAGE_MASK) != ((linear_address + sizeof(uint32) - 1) & CPU::PAGE_MASK))
  {
    uint32 mask = (m_current_address_size == AddressSize_16) ? 0xFFFF : 0xFFFFFFFF;
    uint16 lsb = FetchDirectInstructionWord(address, raise_exceptions);
    uint16 msb = FetchDirectInstructionWord((address + 2) & mask, raise_exceptions);
    return ZeroExtend32(lsb) | (ZeroExtend32(msb) << 16);
  }

  if (raise_exceptions)
    CheckSegmentAccess<sizeof(uint32), AccessType::Execute>(Segment_CS, address, raise_exceptions);

  PhysicalMemoryAddress physical_address;
  TranslateLinearAddress(&physical_address, linear_address, raise_exceptions, AccessType::Execute, raise_exceptions);
  return m_bus->ReadMemoryDWord(physical_address);
}

void CPU::PushWord(uint16 value)
{
  // TODO: Same optimization here with EIP mask - ESP mask
  LinearMemoryAddress linear_address;
  if (m_stack_address_size == AddressSize_16)
  {
    uint16 new_SP = m_registers.SP - sizeof(uint16);
    CheckSegmentAccess<sizeof(uint16), AccessType::Write>(Segment_SS, ZeroExtend32(new_SP), true);
    linear_address = CalculateLinearAddress(Segment_SS, ZeroExtend32(new_SP));
    m_registers.SP = new_SP;
  }
  else
  {
    uint32 new_ESP = m_registers.ESP - sizeof(uint16);
    CheckSegmentAccess<sizeof(uint16), AccessType::Write>(Segment_SS, new_ESP, true);
    linear_address = CalculateLinearAddress(Segment_SS, new_ESP);
    m_registers.ESP = new_ESP;
  }

  WriteMemoryWord(linear_address, value);
}

void CPU::PushDWord(uint32 value)
{
  PhysicalMemoryAddress linear_address;
  if (m_stack_address_size == AddressSize_16)
  {
    uint16 new_SP = m_registers.SP - sizeof(uint32);
    CheckSegmentAccess<sizeof(uint32), AccessType::Write>(Segment_SS, ZeroExtend32(new_SP), true);
    linear_address = CalculateLinearAddress(Segment_SS, ZeroExtend32(new_SP));
    m_registers.SP = new_SP;
  }
  else
  {
    uint32 new_ESP = m_registers.ESP - sizeof(uint32);
    CheckSegmentAccess<sizeof(uint32), AccessType::Write>(Segment_SS, new_ESP, true);
    linear_address = CalculateLinearAddress(Segment_SS, new_ESP);
    m_registers.ESP = new_ESP;
  }

  WriteMemoryDWord(linear_address, value);
}

uint16 CPU::PopWord()
{
  PhysicalMemoryAddress linear_address;
  if (m_stack_address_size == AddressSize_16)
  {
    CheckSegmentAccess<sizeof(uint16), AccessType::Read>(Segment_SS, ZeroExtend32(m_registers.SP), true);
    linear_address = CalculateLinearAddress(Segment_SS, m_registers.SP);
    m_registers.SP += sizeof(uint16);
  }
  else
  {
    CheckSegmentAccess<sizeof(uint16), AccessType::Read>(Segment_SS, m_registers.ESP, true);
    linear_address = CalculateLinearAddress(Segment_SS, m_registers.ESP);
    m_registers.ESP += sizeof(uint16);
  }

  return ReadMemoryWord(linear_address);
}

uint32 CPU::PopDWord()
{
  PhysicalMemoryAddress linear_address;
  if (m_stack_address_size == AddressSize_16)
  {
    CheckSegmentAccess<sizeof(uint32), AccessType::Read>(Segment_SS, ZeroExtend32(m_registers.SP), true);
    linear_address = CalculateLinearAddress(Segment_SS, m_registers.SP);
    m_registers.SP += sizeof(uint32);
  }
  else
  {
    CheckSegmentAccess<sizeof(uint32), AccessType::Read>(Segment_SS, m_registers.ESP, true);
    linear_address = CalculateLinearAddress(Segment_SS, m_registers.ESP);
    m_registers.ESP += sizeof(uint32);
  }

  return ReadMemoryDWord(linear_address);
}

void CPU::SetFlags(uint32 value)
{
  // Don't clear/set all flags, only those allowed
  uint32 MASK = Flag_CF | Flag_PF | Flag_AF | Flag_ZF | Flag_SF | Flag_TF | Flag_IF | Flag_DF | Flag_OF;

  if (m_model >= MODEL_PENTIUM)
  {
    // ID is Pentium+
    MASK |= Flag_IOPL | Flag_NT | Flag_AC | Flag_ID;
  }
  if (m_model >= MODEL_486)
  {
    // AC is 486+
    MASK |= Flag_IOPL | Flag_NT | Flag_AC;
  }
  else if (m_model >= MODEL_386)
  {
    MASK |= Flag_IOPL | Flag_NT;
  }
  else
  {
    // Clear upper bits on <386
    value &= 0x0000FFFF;

    if (m_model == MODEL_286)
    {
      // IOPL flag is 286+
      // Nested task flag can't be set in real mode on a 286
      MASK |= Flag_IOPL;
      if (InProtectedMode())
        MASK |= Flag_NT;
    }
  }

  m_registers.EFLAGS.bits = (value & MASK) | (m_registers.EFLAGS.bits & ~MASK);
}

void CPU::SetHalted(bool halt)
{
  if (halt)
    Log_TracePrintf("CPU Halt");

  m_halted = halt;
}

void CPU::LoadSpecialRegister(Reg32 reg, uint32 value)
{
  switch (reg)
  {
    case Reg32_CR0:
    {
      uint32 CHANGE_MASK = CR0Bit_PE | CR0Bit_NW | CR0Bit_CD | CR0Bit_EM | CR0Bit_PG;
      uint32 old_value = m_registers.CR0;

      // 486 introduced WP bit
      if (m_model >= MODEL_486)
        CHANGE_MASK |= CR0Bit_WP;

      Log_DevPrintf("CR0 <- 0x%08X", value);
      // Log_DevPrintf("  Protected mode: %s", ((value & CR0Bit_PE) != 0) ? "enabled" : "disabled");
      // Log_DevPrintf("  Paging: %s", ((value & CR0Bit_PG) != 0) ? "enabled" : "disabled");

      if ((value & CR0Bit_PE) != (m_registers.CR0 & CR0Bit_PE))
        Log_DevPrintf("Switching to %s mode", ((value & CR0Bit_PE) != 0) ? "protected" : "real");
      if ((value & CR0Bit_PG) != (m_registers.CR0 & CR0Bit_PG))
        Log_DevPrintf("Paging is now %s", ((value & CR0Bit_PG) != 0) ? "enabled" : "disabled");

      if ((value & CR0Bit_CD) != (m_registers.CR0 & CR0Bit_CD))
        Log_ErrorPrintf("CPU read cache is now %s", ((value & CR0Bit_CD) != 0) ? "disabled" : "enabled");
      if ((value & CR0Bit_NW) != (m_registers.CR0 & CR0Bit_NW))
        Log_ErrorPrintf("CPU cache is now %s", ((value & CR0Bit_NW) != 0) ? "write-back" : "write-through");

      // We must flush the TLB when WP changes, because it changes the cached access masks.
      constexpr uint32 flush_mask = CR0Bit_WP;
      if ((m_registers.CR0 & flush_mask) != (value & flush_mask))
        InvalidateAllTLBEntries();

      m_registers.CR0 &= ~CHANGE_MASK;
      m_registers.CR0 |= (value & CHANGE_MASK);

      if (InRealMode())
      {
        // CPL is always zero in real mode
        // CPL will be updated when CS is loaded after switching to protected mode
        m_cpl = 0;
        m_tlb_user_supervisor_bit = 0;
      }

      m_backend->OnControlRegisterLoaded(Reg32_CR0, old_value, m_registers.CR0);
    }
    break;

    case Reg32_CR2:
    {
      // Page fault linear address
      uint32 old_value = m_registers.CR2;
      Log_DevPrintf("CR2 <- 0x%08X", value);
      m_registers.CR2 = value;
      m_backend->OnControlRegisterLoaded(Reg32_CR2, old_value, value);
    }
    break;

    case Reg32_CR3:
    {
      // TODO: Invalidate TLB?
      uint32 old_value = m_registers.CR3;
      Log_DevPrintf("CR3 <- 0x%08X", value);
      m_registers.CR3 = value;
      InvalidateAllTLBEntries();
      FlushPrefetchQueue();
      m_backend->OnControlRegisterLoaded(Reg32_CR3, old_value, value);
    }
    break;

    case Reg32_CR4:
    {
      uint32 old_value = m_registers.CR4;
      Log_DevPrintf("CR4 <- 0x%08X", value);
      m_registers.CR4 = value;
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
      Log_WarningPrintf("DR%u <- 0x%08X", uint32(reg - Reg32_DR0), value);
      m_registers.reg32[reg] = value;
    }
    break;

    default:
      UnreachableCode();
      break;
  }
}

PhysicalMemoryAddress CPU::CalculateLinearAddress(Segment segment, VirtualMemoryAddress offset)
{
  DebugAssert(segment < Segment_Count);
  return m_segment_cache[segment].base_address + offset;
}

bool CPU::TranslateLinearAddress(PhysicalMemoryAddress* out_physical_address, LinearMemoryAddress linear_address,
                                 bool access_check, AccessType access_type, bool raise_page_fault)
{
  // Skip if paging is not enabled.
  if ((m_registers.CR0 & CR0Bit_PG) == 0)
  {
    *out_physical_address = linear_address;
    return true;
  }

#ifdef ENABLE_TLB_EMULATION
  // Check TLB.
  size_t tlb_index = GetTLBEntryIndex(linear_address);
  TLBEntry& tlb_entry = m_tlb_entries[m_tlb_user_supervisor_bit][static_cast<uint8>(access_type)][tlb_index];
  if (tlb_entry.linear_address == (linear_address & PAGE_MASK))
  {
    // TLB hit!
    *out_physical_address = tlb_entry.physical_address + (linear_address & PAGE_OFFSET_MASK);
    return true;
  }
#endif

  // TODO: Large (4MB) pages

  // Permission checks only apply in usermode, except if WP bit of CR0 is set
  bool do_access_check = (access_check && ((m_registers.CR0 & CR0Bit_WP) || InUserMode()));
  uint8 access_mask = (1 << (uint8)access_type) << (InUserMode() ? 0 : 3);

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
    if (raise_page_fault)
      RaisePageFault(linear_address, access_type, false);
    return false;
  }

  // Permissions for directory
  // U bit set implies userspace can access it
  // R bit implies userspace can write to it
  uint8 directory_permissions = (0x05 << 3);                   // supervisor=read,execute
  directory_permissions |= (directory_entry.bits << 3) & 0x10; // supervisor=write from R/W bit
  directory_permissions |= (directory_entry.bits >> 2) & 0x01; // user=read from U/S bit
  directory_permissions |= (directory_entry.bits) & 0x04;      // user=execute from U/S bit
  directory_permissions |= (directory_entry.bits) & 0x02;      // user=write from R/W bit
  directory_permissions |= (directory_entry.bits >> 1) & 0x02; // user=write from U/S bit

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
    if (raise_page_fault)
      RaisePageFault(linear_address, access_type, false);
    return false;
  }

  // Check for table permissions
  // U bit set implies userspace can access it
  // R bit implies userspace can write to it
  uint8 table_permissions = (0x05 << 3);               // supervisor=read,execute
  table_permissions |= (table_entry.bits << 3) & 0x10; // supervisor=write from R/W bit
  table_permissions |= (table_entry.bits >> 2) & 0x01; // user=read from U/S bit
  table_permissions |= (table_entry.bits) & 0x04;      // user=execute from U/S bit
  table_permissions |= (table_entry.bits) & 0x02;      // user=write from R/W bit
  table_permissions |= (table_entry.bits >> 1) & 0x02; // user=write from U/S bit

  // Check access, requires both directory and page access
  if (do_access_check && (access_mask & directory_permissions & table_permissions) == 0)
  {
    if (raise_page_fault)
      RaisePageFault(linear_address, access_type, true);
    return false;
  }

  // Updating of accessed/dirty bits is only done with access checks are enabled (=> normal usage)
  if (access_check)
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
    if (access_type == AccessType::Write && !table_entry.dirty)
    {
      table_entry.dirty = true;
      m_bus->WriteMemoryDWord(table_entry_address, table_entry.bits);
    }
  }

  // Calculate the physical address from the page table entry. Pages are 4KB aligned.
  PhysicalMemoryAddress page_base_address = (table_entry.physical_address << 12);
  PhysicalMemoryAddress translated_address = page_base_address + (linear_address & 0xFFF);

#ifdef ENABLE_TLB_EMULATION
  // Require both directory and page permissions to access in the TLB.
  // TODO: Is this the same on the 386 and 486?
  tlb_entry.linear_address = linear_address & PAGE_MASK;
  tlb_entry.physical_address = page_base_address;
#endif

  *out_physical_address = translated_address;
  return true;
}

void CPU::RaisePageFault(LinearMemoryAddress linear_address, AccessType access_type, bool page_present)
{
  Log_WarningPrintf("Page fault at linear address 0x%08X: %s,%s,%s", linear_address,
                    page_present ? "Present" : "Not Present", access_type == AccessType::Write ? "Write" : "Read",
                    InUserMode() ? "User Mode" : "Supervisor Mode");

  // Determine bits of error code
  uint32 error_code = (((page_present) ? (1u << 0) : 0) |                     // P
                       ((access_type == AccessType::Write) ? (1u << 1) : 0) | // W/R
                       ((InUserMode()) ? (1u << 2) : 0));                     // U/S

  // Update CR2 with the linear address that triggered the fault
  m_registers.CR2 = linear_address;
  RaiseException(Interrupt_PageFault, error_code);
}

uint8 CPU::ReadMemoryByte(LinearMemoryAddress address)
{
  AddMemoryCycle();

  // TODO: Inline TLB check
  PhysicalMemoryAddress physical_address;
  TranslateLinearAddress(&physical_address, address, true, AccessType::Read, true);

  // TODO: Optimize Bus
  return m_bus->ReadMemoryByte(physical_address);
}

uint16 CPU::ReadMemoryWord(LinearMemoryAddress address)
{
  // TODO: Alignment access exception.
  // if ((address & (sizeof(uint16) - 1)) != 0)

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(uint16) - 1) & PAGE_MASK))
  {
    AddMemoryCycle();
    PhysicalMemoryAddress physical_address;
    TranslateLinearAddress(&physical_address, address, true, AccessType::Read, true);
    return m_bus->ReadMemoryWord(physical_address);
  }

  // Fall back to byte reads.
  uint8 b0 = ReadMemoryByte(address + 0);
  uint8 b1 = ReadMemoryByte(address + 1);
  return ZeroExtend16(b0) | (ZeroExtend16(b1) << 8);
}

uint32 CPU::ReadMemoryDWord(LinearMemoryAddress address)
{
  // TODO: Alignment access exception.
  // if ((address & (sizeof(uint32) - 1)) != 0)

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(uint32) - 1) & PAGE_MASK))
  {
    AddMemoryCycle();
    PhysicalMemoryAddress physical_address;
    TranslateLinearAddress(&physical_address, address, true, AccessType::Read, true);
    return m_bus->ReadMemoryDWord(physical_address);
  }

  // Fallback to word reads when it's split across pages.
  uint16 w0 = ReadMemoryWord(address + 0);
  uint16 w1 = ReadMemoryWord(address + 2);
  return ZeroExtend32(w0) | (ZeroExtend32(w1) << 16);
}

void CPU::WriteMemoryByte(LinearMemoryAddress address, uint8 value)
{
  AddMemoryCycle();
  PhysicalMemoryAddress physical_address;
  TranslateLinearAddress(&physical_address, address, true, AccessType::Write, true);
  m_bus->WriteMemoryByte(physical_address, value);
}

void CPU::WriteMemoryWord(LinearMemoryAddress address, uint16 value)
{
  // TODO: Alignment check exception.
  // if ((address & (sizeof(uint16) - 1)) != 0)

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(uint16) - 1) & PAGE_MASK))
  {
    AddMemoryCycle();
    PhysicalMemoryAddress physical_address;
    TranslateLinearAddress(&physical_address, address, true, AccessType::Write, true);
    m_bus->WriteMemoryWord(physical_address, value);
    return;
  }

  // Slowest path here.
  WriteMemoryByte((address + 0), Truncate8(value));
  WriteMemoryByte((address + 1), Truncate8(value >> 8));
}

void CPU::WriteMemoryDWord(LinearMemoryAddress address, uint32 value)
{
  // TODO: Alignment access exception.
  // if ((address & (sizeof(uint32) - 1)) != 0)

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(uint32) - 1) & PAGE_MASK))
  {
    AddMemoryCycle();
    PhysicalMemoryAddress physical_address;
    TranslateLinearAddress(&physical_address, address, true, AccessType::Write, true);
    m_bus->WriteMemoryDWord(physical_address, value);
    return;
  }

  // Fallback to word writes when it's split across pages.
  WriteMemoryWord((address + 0), Truncate16(value));
  WriteMemoryWord((address + 2), Truncate16(value >> 16));
}

uint8 CPU::ReadMemoryByte(Segment segment, VirtualMemoryAddress address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(uint8), AccessType::Read>(segment, address, true);
  return ReadMemoryByte(linear_address);
}

uint16 CPU::ReadMemoryWord(Segment segment, VirtualMemoryAddress address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(uint16), AccessType::Read>(segment, address, true);
  return ReadMemoryWord(linear_address);
}

uint32 CPU::ReadMemoryDWord(Segment segment, VirtualMemoryAddress address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(uint32), AccessType::Read>(segment, address, true);
  return ReadMemoryDWord(linear_address);
}

void CPU::WriteMemoryByte(Segment segment, VirtualMemoryAddress address, uint8 value)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(uint8), AccessType::Write>(segment, address, true);
  WriteMemoryByte(linear_address, value);
}

void CPU::WriteMemoryWord(Segment segment, VirtualMemoryAddress address, uint16 value)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(uint16), AccessType::Write>(segment, address, true);
  WriteMemoryWord(linear_address, value);
}

void CPU::WriteMemoryDWord(Segment segment, VirtualMemoryAddress address, uint32 value)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  CheckSegmentAccess<sizeof(uint32), AccessType::Write>(segment, address, true);
  WriteMemoryDWord(linear_address, value);
}

bool CPU::SafeReadMemoryByte(LinearMemoryAddress address, uint8* value, bool access_check, bool raise_page_fault)
{
  PhysicalMemoryAddress physical_address;
  if (!TranslateLinearAddress(&physical_address, address, access_check, AccessType::Read, raise_page_fault))
  {
    *value = UINT8_C(0xFF);
    return false;
  }

  return m_bus->CheckedReadMemoryByte(physical_address, value);
}

bool CPU::SafeReadMemoryWord(LinearMemoryAddress address, uint16* value, bool access_check, bool raise_page_fault)
{
  PhysicalMemoryAddress physical_address;

  // TODO: Alignment access exception.
  // if ((address & (sizeof(uint16) - 1)) != 0)

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(uint16) - 1) & PAGE_MASK))
  {
    if (!TranslateLinearAddress(&physical_address, address, access_check, AccessType::Read, raise_page_fault))
    {
      *value = UINT16_C(0xFFFF);
      return false;
    }

    return m_bus->CheckedReadMemoryWord(physical_address, value);
  }

  // Fall back to byte reads.
  uint8 b0, b1;
  bool result = SafeReadMemoryByte(address + 0, &b0, access_check, raise_page_fault) &
                SafeReadMemoryByte(address + 1, &b1, access_check, raise_page_fault);

  *value = ZeroExtend16(b0) | (ZeroExtend16(b1) << 8);
  return result;
}

bool CPU::SafeReadMemoryDWord(LinearMemoryAddress address, uint32* value, bool access_check, bool raise_page_fault)
{
  PhysicalMemoryAddress physical_address;

  // TODO: Alignment access exception.
  // if ((address & (sizeof(uint32) - 1)) != 0)

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(uint32) - 1) & PAGE_MASK))
  {
    if (!TranslateLinearAddress(&physical_address, address, access_check, AccessType::Read, raise_page_fault))
      return false;

    return m_bus->CheckedReadMemoryDWord(physical_address, value);
  }

  // Fallback to word reads when it's split across pages.
  uint16 w0 = 0, w1 = 0;
  bool result = SafeReadMemoryWord(address + 0, &w0, access_check, raise_page_fault) &
                SafeReadMemoryWord(address + 2, &w1, access_check, raise_page_fault);

  *value = ZeroExtend32(w0) | (ZeroExtend32(w1) << 16);
  return result;
}

bool CPU::SafeWriteMemoryByte(VirtualMemoryAddress address, uint8 value, bool access_check, bool raise_page_fault)
{
  PhysicalMemoryAddress physical_address;
  if (!TranslateLinearAddress(&physical_address, address, access_check, AccessType::Write, raise_page_fault))
    return false;

  return m_bus->CheckedWriteMemoryByte(physical_address, value);
}

bool CPU::SafeWriteMemoryWord(VirtualMemoryAddress address, uint16 value, bool access_check, bool raise_page_fault)
{
  PhysicalMemoryAddress physical_address;

  // TODO: Alignment check exception.
  // if ((address & (sizeof(uint16) - 1)) != 0)

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(uint16) - 1) & PAGE_MASK))
  {
    if (!TranslateLinearAddress(&physical_address, address, access_check, AccessType::Write, raise_page_fault))
      return false;

    return m_bus->CheckedWriteMemoryWord(physical_address, value);
  }

  // Slowest path here.
  return SafeWriteMemoryByte((address + 0), Truncate8(value), access_check, raise_page_fault) &
         SafeWriteMemoryByte((address + 1), Truncate8(value >> 8), access_check, raise_page_fault);
}

bool CPU::SafeWriteMemoryDWord(VirtualMemoryAddress address, uint32 value, bool access_check, bool raise_page_fault)
{
  PhysicalMemoryAddress physical_address;

  // TODO: Alignment access exception.
  // if ((address & (sizeof(uint32) - 1)) != 0)

  // If the address falls within the same page we can still skip doing byte reads.
  if ((address & PAGE_MASK) == ((address + sizeof(uint32) - 1) & PAGE_MASK))
  {
    if (!TranslateLinearAddress(&physical_address, address, access_check, AccessType::Write, raise_page_fault))
      return false;

    return m_bus->CheckedWriteMemoryDWord(physical_address, value);
  }

  // Fallback to word writes when it's split across pages.
  return SafeWriteMemoryWord((address + 0), Truncate16(value), access_check, raise_page_fault) &
         SafeWriteMemoryWord((address + 2), Truncate16(value >> 16), access_check, raise_page_fault);
}

void CPU::PrintCurrentStateAndInstruction(const char* prefix_message /* = nullptr */)
{
  if (prefix_message)
  {
    std::fprintf(stdout, "%s at EIP = %04X:%08Xh (0x%08X)\n", prefix_message, m_registers.CS, m_current_EIP,
                 CalculateLinearAddress(Segment_CS, m_current_EIP));
  }

#if 1
  std::fprintf(stdout, "EAX=%08X EBX=%08X ECX=%08X EDX=%08X ESI=%08X EDI=%08X\n", m_registers.EAX, m_registers.EBX,
               m_registers.ECX, m_registers.EDX, m_registers.ESI, m_registers.EDI);
  std::fprintf(stdout, "ESP=%08X EBP=%08X EIP=%04X:%08X EFLAGS=%08X ES=%04X SS=%04X DS=%04X FS=%04X GS=%04X\n",
               m_registers.ESP, m_registers.EBP, ZeroExtend32(m_registers.CS), m_current_EIP, m_registers.EFLAGS.bits,
               ZeroExtend32(m_registers.ES), ZeroExtend32(m_registers.SS), ZeroExtend32(m_registers.DS),
               ZeroExtend32(m_registers.FS), ZeroExtend32(m_registers.GS));
#endif

  uint32 fetch_EIP = m_current_EIP;
  auto fetchb = [this, &fetch_EIP]() {
    uint8 value = FetchDirectInstructionByte(fetch_EIP, false);
    fetch_EIP = (fetch_EIP + sizeof(value)) & m_EIP_mask;
    return value;
  };
  auto fetchw = [this, &fetch_EIP]() {
    uint16 value = FetchDirectInstructionWord(fetch_EIP, false);
    fetch_EIP = (fetch_EIP + sizeof(value)) & m_EIP_mask;
    return value;
  };
  auto fetchd = [this, &fetch_EIP]() {
    uint32 value = FetchDirectInstructionDWord(fetch_EIP, false);
    fetch_EIP = (fetch_EIP + sizeof(value)) & m_EIP_mask;
    return value;
  };

  // Try to decode the instruction first.
  Instruction instruction;
  bool instruction_valid = Decoder::DecodeInstruction(&instruction, m_current_address_size, m_current_operand_size,
                                                      fetch_EIP, fetchb, fetchw, fetchd);

  // TODO: Handle 16 vs 32-bit operating mode clamp on address
  SmallString hex_string;
  uint32 instruction_length = instruction_valid ? instruction.length : 16;
  for (uint32 i = 0; i < instruction_length; i++)
  {
    uint8 value = 0;
    SafeReadMemoryByte(CalculateLinearAddress(Segment_CS, m_current_EIP + i), &value, false, false);
    hex_string.AppendFormattedString("%02X ", ZeroExtend32(value));
  }
  for (uint32 i = instruction_length; i < 10; i++)
    hex_string.AppendString("   ");

  if (instruction_valid)
  {
    SmallString instr_string;
    Decoder::DisassembleToString(&instruction, &instr_string);

    LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, m_current_EIP);
    std::fprintf(stdout, "%04X:%08Xh (0x%08X) | %s | %s\n", ZeroExtend32(m_registers.CS), m_current_EIP, linear_address,
                 hex_string.GetCharArray(), instr_string.GetCharArray());
  }
  else
  {
    std::fprintf(stdout, "Decoding failed, bytes at failure point: %s\n", hex_string.GetCharArray());
  }
}

bool CPU::ReadDescriptorEntry(DESCRIPTOR_ENTRY* entry, const DescriptorTablePointer& table, uint32 index)
{
  uint32 offset = index * 8;
  if ((offset + 7) > table.limit)
    return false;

  // TODO: Should this use supervisor privileges since it's reading the GDT?
  SafeReadMemoryDWord(table.base_address + offset + 0, &entry->bits0, false, true);
  SafeReadMemoryDWord(table.base_address + offset + 4, &entry->bits1, false, true);
  return true;
}

bool CPU::WriteDescriptorEntry(const DESCRIPTOR_ENTRY& entry, const DescriptorTablePointer& table, uint32 index)
{
  uint32 offset = index * 8;
  if ((offset + 7) > table.limit)
    return false;

  // TODO: Should this use supervisor privileges since it's reading the GDT?
  LinearMemoryAddress descriptor_address = table.base_address + offset + 0;
  SafeWriteMemoryDWord(descriptor_address + 0, entry.bits0, false, true);
  SafeWriteMemoryDWord(descriptor_address + 4, entry.bits1, false, true);
  return true;
}

bool CPU::CheckTargetCodeSegment(uint16 raw_selector, uint8 check_rpl, uint8 check_cpl, bool raise_exceptions)
{
  // Check for null selector
  SEGMENT_SELECTOR_VALUE selector = {raw_selector};
  if (selector.index == 0)
  {
    if (raise_exceptions)
      RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return false;
  }

  // Read/check descriptor
  DESCRIPTOR_ENTRY descriptor;
  if (!ReadDescriptorEntry(&descriptor, selector.ti ? m_ldt_location : m_gdt_location, selector.index))
  {
    if (raise_exceptions)
      RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return false;
  }

  // Check for non-code segments
  if (!descriptor.IsCodeSegment())
  {
    if (raise_exceptions)
      RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return false;
  }

  if (!descriptor.memory.access.code_conforming)
  {
    // Non-conforming code segments must have DPL=CPL
    if (descriptor.dpl != check_cpl)
    {
      if (raise_exceptions)
        RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
      return false;
    }

    // RPL must be <= CPL
    if (check_rpl > check_cpl)
    {
      if (raise_exceptions)
        RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
      return false;
    }
  }
  else
  {
    // Conforming code segment must have DPL <= CPL
    if (descriptor.dpl > check_cpl)
    {
      if (raise_exceptions)
        RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
      return false;
    }
  }

  // Segment must be present
  // TODO: Is the order important (GPF before NP)?
  if (!descriptor.IsPresent())
  {
    if (raise_exceptions)
      RaiseException(Interrupt_SegmentNotPresent, selector.ValueForException());
    return false;
  }

  return true;
}

void CPU::LoadGlobalDescriptorTable(LinearMemoryAddress table_base_address, uint32 table_limit)
{
  m_gdt_location.base_address = table_base_address;
  m_gdt_location.limit = table_limit;

  Log_DevPrintf("Load GDT: Base 0x%08X limit 0x%04X", table_base_address, table_limit);
}

void CPU::LoadInterruptDescriptorTable(LinearMemoryAddress table_base_address, uint32 table_limit)
{
  m_idt_location.base_address = table_base_address;
  m_idt_location.limit = table_limit;

  Log_DevPrintf("Load IDT: Base 0x%08X limit 0x%04X", table_base_address, table_limit);
}

void CPU::LoadSegmentRegister(Segment segment, uint16 value)
{
  static const char* segment_names[Segment_Count] = {"ES", "CS", "SS", "DS", "FS", "GS"};
  SegmentCache* segment_cache = &m_segment_cache[segment];

  // In real mode, base is 0x10 * value, with the limit at 64KiB
  if (InRealMode() || InVirtual8086Mode())
  {
    // The limit can be modified in protected mode by loading a descriptor.
    // When the register is reloaded in real mode, this limit must be preserved.
    // TODO: What about stack address size?
    segment_cache->base_address = PhysicalMemoryAddress(value) * 0x10;
    m_registers.segment_selectors[segment] = value;

    if (InRealMode())
    {
      // Real mode uses DPL=0, but maintains validity and limits of existing segment
      segment_cache->dpl = 0;
    }
    else
    {
      // V8086 mode uses DPL=3, and makes the segment valid
      segment_cache->dpl = 3;
      if (segment == Segment_CS)
      {
        segment_cache->access.is_code = true;
        segment_cache->access.code_readable = true;
      }
      else
      {
        segment_cache->access.is_code = false;
        segment_cache->access.data_writable = true;
        segment_cache->access.data_expand_down = false;
      }
      segment_cache->limit_low = 0x0000;
      segment_cache->limit_high = 0xFFFF;
      segment_cache->limit_raw = 0xFFFF;
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
  SEGMENT_SELECTOR_VALUE reg_value = {value};
  if (reg_value.index == 0 && !reg_value.ti)
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
    segment_cache->limit_raw = 0;
    segment_cache->access.bits = 0;
    segment_cache->dpl = 0;
    segment_cache->access_mask = AccessTypeMask::None;
    m_registers.segment_selectors[segment] = value;
    Log_TracePrintf("Loaded null selector for %s", segment_names[segment]);
    return;
  }

  // Read descriptor entry. If this fails, it's because it's outside the limit.
  DESCRIPTOR_ENTRY descriptor;
  if (!ReadDescriptorEntry(&descriptor, reg_value.ti ? m_ldt_location : m_gdt_location, reg_value.index))
  {
    RaiseException(Interrupt_GeneralProtectionFault, reg_value.ValueForException());
    return;
  }

  // SS has to be handled separately due to stack fault exception.
  if (segment == Segment_SS)
  {
    if (reg_value.rpl != GetCPL() || descriptor.dpl != GetCPL() || !descriptor.IsWritableDataSegment())
    {
      RaiseException(Interrupt_GeneralProtectionFault, reg_value.ValueForException());
      return;
    }
    if (!descriptor.IsPresent())
    {
      RaiseException(Interrupt_StackFault, reg_value.ValueForException());
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
    if ((!descriptor.IsDataSegment() && !descriptor.IsReadableCodeSegment()) ||
        ((descriptor.IsDataSegment() || descriptor.IsConformingCodeSegment() || (reg_value.rpl > descriptor.dpl)) &&
         (GetCPL() > descriptor.dpl)))
    {
      RaiseException(Interrupt_GeneralProtectionFault, reg_value.ValueForException());
      return;
    }
    if (!descriptor.IsPresent())
    {
      RaiseException(Interrupt_SegmentNotPresent, reg_value.ValueForException());
      return;
    }
  }

  // Extract information from descriptor
  segment_cache->base_address = descriptor.memory.GetBase();
  segment_cache->access.bits = descriptor.memory.access_bits;
  segment_cache->dpl = descriptor.dpl;
  m_registers.segment_selectors[segment] = value;

  // Handle page granularity.
  uint32 limit = descriptor.memory.GetLimit();
  bool is_32bit_segment = (m_model >= MODEL_386 && descriptor.memory.flags.size);
  segment_cache->limit_raw = limit;
  if (m_model >= MODEL_386 && descriptor.memory.flags.granularity)
    limit = (limit << 12) | 0xFFF;

  // Expand-up segment?
  if (descriptor.memory.access.is_code || !descriptor.memory.access.data_expand_down)
  {
    // limit=c000: 0 <= address <= c000.
    segment_cache->limit_low = 0;
    segment_cache->limit_high = limit;
  }
  else
  {
    // limit=c000: limit < address <= ffff/ffffffff
    segment_cache->limit_low = limit + 1;
    segment_cache->limit_high = is_32bit_segment ? 0xFFFFFFFF : 0xFFFF;
  }

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
                  segment_names[segment], ZeroExtend32(value), reg_value.ti ? "LDT" : "GDT", uint32(reg_value.index),
                  segment_cache->base_address, segment_cache->limit_low, segment_cache->limit_high);

  if (segment == Segment_CS)
  {
    // Code segment determines default address/operand sizes
    AddressSize new_address_size = (is_32bit_segment) ? AddressSize_32 : AddressSize_16;
    OperandSize new_operand_size = (is_32bit_segment) ? OperandSize_32 : OperandSize_16;
    if (new_address_size != m_current_address_size)
    {
      Log_DevPrintf("Switching to %s %s execution", (new_address_size == AddressSize_32) ? "32-bit" : "16-bit",
                    (InProtectedMode()) ? "protected mode" : "real mode");

      m_current_address_size = new_address_size;
      m_current_operand_size = new_operand_size;
      m_EIP_mask = (is_32bit_segment) ? 0xFFFFFFFF : 0xFFFF;
    }

    // CPL is the selector's RPL
    if (m_cpl != reg_value.rpl)
      Log_DevPrintf("Privilege change: %u -> %u", ZeroExtend32(m_cpl), ZeroExtend32(reg_value.rpl.GetValue()));
    m_cpl = reg_value.rpl;
    m_tlb_user_supervisor_bit = BoolToUInt8(InSupervisorMode());
    FlushPrefetchQueue();
  }
  else if (segment == Segment_SS)
  {
    // Stack segment determines stack address size
    AddressSize new_address_size = (is_32bit_segment) ? AddressSize_32 : AddressSize_16;
    if (new_address_size != m_stack_address_size)
    {
      Log_DevPrintf("Switching to %s stack", (new_address_size == AddressSize_32) ? "32-bit" : "16-bit");

      m_stack_address_size = new_address_size;
    }
  }
}

void CPU::LoadLocalDescriptorTable(uint16 value)
{
  //     // We can't change task segments when CPL != 0
  //     // TODO: This causes issues when nested in task switches
  //     if (GetCPL() != 0)
  //     {
  //         RaiseException(Interrupt_GeneralProtectionFault, 0);
  //         return;
  //     }

  // If it's a null descriptor, just clear out the fields. LDT entries can't be used after this.
  SEGMENT_SELECTOR_VALUE selector = {value};
  if (selector.index == 0)
  {
    m_ldt_location.base_address = 0;
    m_ldt_location.limit = 0;
    m_registers.LDTR = selector.bits;
    return;
  }

  // Has to be a GDT selector
  if (selector.ti)
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return;
  }

  // Read descriptor entry. If this fails, it's because it's outside the limit.
  DESCRIPTOR_ENTRY descriptor;
  if (!ReadDescriptorEntry(&descriptor, m_gdt_location, selector.index))
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return;
  }
  if (!descriptor.present)
  {
    RaiseException(Interrupt_SegmentNotPresent, selector.ValueForException());
    return;
  }

  // Has to be a LDT descriptor
  if (!descriptor.IsSystemSegment() || descriptor.type != DESCRIPTOR_TYPE_LDT)
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return;
  }

  // Update descriptor cache
  m_ldt_location.base_address = descriptor.ldt.GetBase();
  m_ldt_location.limit = descriptor.ldt.GetLimit();
  m_registers.LDTR = selector.bits;

  Log_DevPrintf("Load local descriptor table: %04X index %u base 0x%08X limit 0x%08X", ZeroExtend32(selector.bits),
                ZeroExtend32(selector.index.GetValue()), m_tss_location.base_address, m_tss_location.limit);
}

void CPU::LoadTaskSegment(uint16 value)
{
  //     // We can't change task segments when CPL != 0
  //     // TODO: This causes issues when nested in task switches
  //     if (GetCPL() != 0)
  //     {
  //         RaiseException(Interrupt_GeneralProtectionFault, 0);
  //         return;
  //     }

  // Has to be a GDT selector, and not a null selector
  SEGMENT_SELECTOR_VALUE selector = {value};
  if (selector.ti || selector.index == 0)
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return;
  }

  // Read descriptor entry. If this fails, it's because it's outside the limit.
  DESCRIPTOR_ENTRY descriptor;
  if (!ReadDescriptorEntry(&descriptor, m_gdt_location, selector.index))
  {
    RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return;
  }
  if (!descriptor.present)
  {
    RaiseException(Interrupt_SegmentNotPresent, selector.ValueForException());
    return;
  }

  // Segment has to be a TSS, and has to be available
  if (!descriptor.IsSystemSegment() || (descriptor.type != DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16 &&
                                        descriptor.type != DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32))
  {
    RaiseException(Interrupt_SegmentNotPresent, selector.ValueForException());
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

  Log_DevPrintf("Load task register %04X: index %u base 0x%08X limit 0x%08X", ZeroExtend32(selector.bits),
                ZeroExtend32(selector.index.GetValue()), m_tss_location.base_address, m_tss_location.limit);
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

  // Request interrupt number from the PIC. In reality this is done by the PIC placing a CALL instruction on the bus.
  uint32 interrupt_number = m_system->GetInterruptController()->GetPendingInterruptNumber();
  m_system->GetInterruptController()->AcknowledgeInterrupt(interrupt_number);
  SetupInterruptCall(interrupt_number, false, false, 0, m_registers.EIP);
}

void CPU::RaiseException(uint32 interrupt, uint32 error_code)
{
  if (interrupt == Interrupt_PageFault)
    Log_WarningPrintf("Raise exception %u error code 0x%08X EIP 0x%08X address 0x%08X", interrupt, error_code,
                      m_current_EIP, m_registers.CR2);
  else
    Log_WarningPrintf("Raise exception %u error code 0x%08X EIP 0x%08X", interrupt, error_code, m_current_EIP);

  // If we're throwing an exception on a double-fault, this is a triple fault, and the CPU should reset.
  if (m_current_exception == Interrupt_DoubleFault)
  {
    // Failed double-fault, issue a triple fault.
    Log_WarningPrintf("Triple fault");
    Reset();
    AbortCurrentInstruction();
    return;
  }
  // If this is a nested exception, we issue a double-fault.
  else if (m_current_exception != Interrupt_Count)
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
  m_registers.ESP = m_current_ESP;
  m_current_exception = interrupt;

  // Set up the call to the corresponding interrupt vector.
  SetupInterruptCall(interrupt, false, push_error_code, error_code, m_current_EIP);

  // Abort the current instruction that is executing.
  AbortCurrentInstruction();
}

void CPU::BranchTo(uint32 new_EIP)
{
  FlushPrefetchQueue();
  m_registers.EIP = new_EIP;
  m_backend->BranchTo(new_EIP);
}

void CPU::BranchFromException(uint32 new_EIP)
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
  uint32 current_fetch_length = m_registers.EIP - m_current_EIP;
  if (m_prefetch_queue_position >= current_fetch_length)
    m_prefetch_queue_position -= current_fetch_length;
  else
    FlushPrefetchQueue();
#endif
  m_registers.EIP = m_current_EIP;
}

void CPU::FarJump(uint16 segment_selector, uint32 offset, OperandSize operand_size)
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
    RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return;
  }
  if (!descriptor.present)
  {
    RaiseException(Interrupt_SegmentNotPresent, selector.ValueForException());
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
        RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
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
        RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
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
    RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return;
  }
  else if (descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_16 || descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_32)
  {
    if (descriptor.dpl < GetCPL() || descriptor.dpl < selector.rpl)
    {
      RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
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
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.ValueForException());
      return;
    }
    if (!target_descriptor.IsPresent())
    {
      RaiseException(Interrupt_SegmentNotPresent, target_selector.ValueForException());
      return;
    }

    // Can't lower privilege or change privilege via far jumps
    if ((target_descriptor.memory.IsConformingCodeSegment() && target_descriptor.dpl > GetCPL()) ||
        (!target_descriptor.memory.IsConformingCodeSegment() && target_descriptor.dpl != GetCPL()))
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.ValueForException());
      return;
    }

    // All good to jump through it
    target_selector.rpl = GetCPL();
    LoadSegmentRegister(Segment_CS, target_selector.bits);
    BranchTo((operand_size == OperandSize_16) ? (offset & 0xFFFF) : (offset));
  }
  else if (descriptor.type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16 ||
           descriptor.type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32 ||
           descriptor.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16 ||
           descriptor.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32)
  {
    // Jumping straight to a task segment without a task gate
    SwitchToTask(segment_selector, false, false, false, 0);
  }
  else if (descriptor.type == DESCRIPTOR_TYPE_TASK_GATE)
  {
    // Switch to new task with nesting
    DebugAssert(!m_registers.EFLAGS.VM);
    Log_DevPrintf("Task gate -> 0x%04X", ZeroExtend32(descriptor.task_gate.selector.GetValue()));
    SwitchToTask(descriptor.task_gate.selector, false, false, false, 0);
  }
  else
  {
    Panic("Unhandled far jump target");
  }
}

void CPU::FarCall(uint16 segment_selector, uint32 offset, OperandSize operand_size)
{
  auto MemorySegmentCall = [this](uint16 real_selector, uint32 real_offset, OperandSize real_operand_size) {
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
    RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return;
  }
  if (!descriptor.present)
  {
    RaiseException(Interrupt_SegmentNotPresent, selector.ValueForException());
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
        RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
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
        RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
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
    RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
    return;
  }
  else if (descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_16 || descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_32)
  {
    if (descriptor.dpl < GetCPL() || selector.rpl > descriptor.dpl)
    {
      RaiseException(Interrupt_GeneralProtectionFault, selector.ValueForException());
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
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.ValueForException());
      return;
    }
    if (!target_descriptor.IsPresent())
    {
      RaiseException(Interrupt_SegmentNotPresent, target_selector.ValueForException());
      return;
    }

    // Can't lower privilege?
    if (target_descriptor.dpl > GetCPL())
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.ValueForException());
      return;
    }

    // Changing privilege?
    if (!target_descriptor.memory.IsConformingCodeSegment() && target_descriptor.dpl < GetCPL())
    {
      // Call gate to lower privilege
      target_selector.rpl = target_descriptor.dpl;
      Log_DevPrintf("Privilege raised via call gate, %u -> %u", ZeroExtend32(GetCPL()),
                    ZeroExtend32(target_selector.rpl.GetValue()));

      // Check TSS validity
      if (m_tss_location.limit == 0)
      {
        RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
        return;
      }

      // We need to look at the current TSS to determine the stack pointer to change to
      uint32 new_ESP;
      uint16 new_SS;
      if (m_tss_location.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16)
      {
        LinearMemoryAddress tss_stack_offset =
          Truncate32(offsetof(TASK_STATE_SEGMENT_16, stacks[0]) + target_selector.rpl * sizeof(4));
        if ((tss_stack_offset + 3) > m_tss_location.limit)
        {
          RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
          return;
        }

        // Shouldn't fail, since we're bypassing access checks
        uint16 temp = 0;
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset, &temp, false, true);
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset + 2, &new_SS, false, true);
        new_ESP = ZeroExtend32(temp);
      }
      else
      {
        LinearMemoryAddress tss_stack_offset =
          Truncate32(offsetof(TASK_STATE_SEGMENT_32, stacks[0]) + target_selector.rpl * sizeof(8));
        if ((tss_stack_offset + 5) > m_tss_location.limit)
        {
          RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
          return;
        }

        // Shouldn't fail, since we're bypassing access checks
        SafeReadMemoryDWord(m_tss_location.base_address + tss_stack_offset, &new_ESP, false, true);
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset + 4, &new_SS, false, true);
      }

      // Save the old (outer) ESP/SS before we pop the parameters off?
      uint32 outer_EIP = m_registers.EIP;
      uint32 outer_ESP = m_registers.ESP;
      uint16 outer_CS = m_registers.CS;
      uint16 outer_SS = m_registers.SS;

      // Read parameters from caller before changing anything
      // We can pop here safely because the ESP will be restored afterwards
      uint32 parameter_count = descriptor.call_gate.parameter_count;
      uint32 caller_parameters[32];
      if (m_tss_location.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16)
      {
        for (uint32 i = 0; i < parameter_count; i++)
          caller_parameters[(parameter_count - 1) - i] = ZeroExtend32(PopWord());
      }
      else
      {
        for (uint32 i = 0; i < parameter_count; i++)
          caller_parameters[(parameter_count - 1) - i] = PopDWord();
      }

      // Load the new code segment early, since this can fail without side-effects
      LoadSegmentRegister(Segment_CS, target_selector.bits);

      // Load the new stack segment. If any of the pushes following this fail, we are in trouble.
      // TODO: Perhaps we should save the SS as well as the ESP in case of exceptions?
      LoadSegmentRegister(Segment_SS, new_SS);
      m_registers.ESP = new_ESP;

      // Push parameters to target procedure
      if (descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_16)
      {
        PushWord(outer_SS);
        PushWord(Truncate16(outer_ESP));
        for (uint32 i = 0; i < parameter_count; i++)
          PushWord(Truncate16(caller_parameters[i]));
        PushWord(outer_CS);
        PushWord(Truncate16(outer_EIP));
      }
      else
      {
        PushDWord(ZeroExtend32(outer_SS));
        PushDWord(outer_ESP);
        for (uint32 i = 0; i < parameter_count; i++)
          PushDWord(caller_parameters[i]);
        PushDWord(ZeroExtend32(outer_CS));
        PushDWord(outer_EIP);
      }

      // Finally transfer control
      uint32 new_EIP = descriptor.call_gate.GetOffset();
      if (descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_16)
        new_EIP &= 0xFFFF;
      BranchTo(new_EIP);
    }
    else
    {
      // Call gate to same privilege
      target_selector.rpl = GetCPL();
      if (descriptor.type == DESCRIPTOR_TYPE_CALL_GATE_16)
        MemorySegmentCall(target_selector.bits, descriptor.call_gate.GetOffset(), OperandSize_16);
      else
        MemorySegmentCall(target_selector.bits, descriptor.call_gate.GetOffset(), OperandSize_32);
    }
  }
  else if (descriptor.type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16 ||
           descriptor.type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32 ||
           descriptor.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16 ||
           descriptor.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32)
  {
    // Jumping straight to a task segment without a task gate
    SwitchToTask(segment_selector, true, false, false, 0);
  }
  else if (descriptor.type == DESCRIPTOR_TYPE_TASK_GATE)
  {
    // Switch to new task with nesting
    DebugAssert(!m_registers.EFLAGS.VM);
    Log_DevPrintf("Task gate -> 0x%04X", ZeroExtend32(descriptor.task_gate.selector.GetValue()));
    SwitchToTask(descriptor.task_gate.selector, true, false, false, 0);
  }
  else
  {
    Panic("Unhandled far call type");
  }
}

void CPU::FarReturn(OperandSize operand_size, uint32 pop_byte_count)
{
  uint32 return_EIP;
  uint16 return_CS;
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
    if (target_selector.index == 0) // Check for non-null segment
    {
      RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }
    if (!ReadDescriptorEntry(&target_descriptor, target_selector.ti ? m_ldt_location : m_gdt_location,
                             target_selector.index) || // Check table limits
        !target_descriptor.IsCodeSegment())            // Check for code segment
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.ValueForException());
      return;
    }
    if (target_selector.rpl < GetCPL()) // Check RPL<CPL
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.ValueForException());
      return;
    }
    if (target_descriptor.IsConformingCodeSegment() &&
        target_descriptor.dpl > target_selector.rpl) // conforming and DPL>selector RPL
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.ValueForException());
      return;
    }
    if (!target_descriptor.IsConformingCodeSegment() &&
        target_descriptor.dpl != target_selector.rpl) // non-conforming and DPL!=RPL
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.ValueForException());
      return;
    }
    if (!target_descriptor.IsPresent())
    {
      RaiseException(Interrupt_SegmentNotPresent, target_selector.ValueForException());
      return;
    }
    if (target_selector.rpl > GetCPL())
    {
      // Returning to outer privilege level
      Log_DevPrintf("Privilege lowered via RETF: %u -> %u", ZeroExtend32(GetCPL()),
                    ZeroExtend32(target_selector.rpl.GetValue()));

      uint32 return_ESP;
      uint16 return_SS;
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

      // TODO: We really should check the stack segment validity here rather than in Load..
      LoadSegmentRegister(Segment_CS, return_CS);
      LoadSegmentRegister(Segment_SS, return_SS);
      m_registers.ESP = return_ESP;

      // Validate segments ES,FS,GS,DS so the kernel doesn't leak them
      static const Segment validate_segments[] = {Segment_ES, Segment_FS, Segment_GS, Segment_DS};
      for (uint32 i = 0; i < countof(validate_segments); i++)
      {
        Segment validate_segment = validate_segments[i];
        const SegmentCache* validate_segment_cache = &m_segment_cache[validate_segment];
        if ((!validate_segment_cache->access.is_code || !validate_segment_cache->access.code_confirming) &&
            validate_segment_cache->dpl < target_selector.rpl)
        {
          // If data or non-conforming code, set null selector
          LoadSegmentRegister(validate_segment, 0);
        }
      }

      // Release parameters from caller's stack
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

void CPU::SoftwareInterrupt(OperandSize operand_size, uint32 interrupt)
{
  // In V8086 mode, IOPL has to be 3 otherwise GPF.
  // TODO: If it's set to 0, jumps to PL 0????
  if (InVirtual8086Mode() && GetIOPL() != 3)
  {
    RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // Return to IP after this instruction
  SetupInterruptCall(interrupt, true, false, 0, m_registers.EIP);
}

void CPU::InterruptReturn(OperandSize operand_size)
{
  if (InRealMode())
  {
    // Pull EIP/CS/FLAGS off the stack
    uint32 return_EIP;
    uint16 return_CS;
    uint32 return_EFLAGS;
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
    // V8086 return and IOPL != 3 traps to monitor
    if (GetIOPL() != 3)
    {
      RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    // Pull EIP/CS/FLAGS off the stack
    uint32 return_EIP;
    uint16 return_CS;
    uint32 return_EFLAGS;
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

    // VM, IOPL, VIP, VIF not modified by flags change
    return_EFLAGS &= ~(Flag_VM | Flag_IOPL | Flag_VIP | Flag_VIF);
    return_EFLAGS |= m_registers.EFLAGS.bits & (Flag_VM | Flag_IOPL | Flag_VIP | Flag_VIF);

    // IRET within V8086 mode, handle the same as real mode
    LoadSegmentRegister(Segment_CS, return_CS);
    SetFlags(return_EFLAGS);
    BranchTo(return_EIP);
  }
  else if (m_registers.EFLAGS.NT)
  {
    // Nested task return should not pop anything off stack
    // Link field is always two bytes at offset zero, in both 16 and 32-bit TSS
    uint16 link_field;
    if ((sizeof(link_field) - 1) > m_tss_location.limit)
    {
      RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
      return;
    }
    SafeReadMemoryWord(m_tss_location.base_address, &link_field, false, true);

    // Switch tasks without nesting
    SwitchToTask(link_field, false, true, false, 0);
  }
  else
  {
    // Protected mode
    // Pull EIP/CS/FLAGS off the stack
    uint32 return_EIP;
    uint16 return_CS;
    uint32 return_EFLAGS;
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
    if (GetCPL() == 0 && (return_EFLAGS & Flag_VM) != 0)
    {
      // Entering V8086 mode
      Log_DevPrintf("Entering V8086 mode, EFLAGS = %08X, CS:IP = %04X:%04X", return_EFLAGS, ZeroExtend32(return_CS),
                    return_EIP);

      // TODO: Check EIP lies within CS limits.
      uint32 v86_ESP = PopDWord();
      uint16 v86_SS = Truncate16(PopDWord());
      uint16 v86_ES = Truncate16(PopDWord());
      uint16 v86_DS = Truncate16(PopDWord());
      uint16 v86_FS = Truncate16(PopDWord());
      uint16 v86_GS = Truncate16(PopDWord());

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

      m_cpl = 3;
      m_tlb_user_supervisor_bit = BoolToUInt8(InSupervisorMode());
      m_registers.ESP = v86_ESP;

      BranchTo(return_EIP);
      return;
    }

    // We can't raise privileges by IRET
    SEGMENT_SELECTOR_VALUE target_selector = {return_CS};
    if (target_selector.rpl < GetCPL())
    {
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.ValueForException());
      return;
    }

    // Validate we can jump to this segment from here
    if (!CheckTargetCodeSegment(return_CS, 0, target_selector.rpl, true))
      return;

    // Some flags can't be changed if we're not in CPL=0.
    uint32 change_mask = Flag_CF | Flag_PF | Flag_AF | Flag_ZF | Flag_SF | Flag_TF | Flag_DF | Flag_OF | Flag_NT |
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
      Log_DevPrintf("Privilege lowered via IRET, %u -> %u", ZeroExtend32(GetCPL()),
                    ZeroExtend32(target_selector.rpl.GetValue()));

      // Grab ESP/SS from stack
      uint32 outer_ESP;
      uint16 outer_SS;
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

      // Change code segment and CPL
      LoadSegmentRegister(Segment_CS, return_CS);

      // Load the outer stack, we can do this here because an exception will reload CS
      LoadSegmentRegister(Segment_SS, outer_SS);

      // ESP leak, undocumented
      if (m_stack_address_size == AddressSize_16)
        m_registers.SP = Truncate16(outer_ESP);
      else
        m_registers.ESP = outer_ESP;
      // outer_ESP = (m_registers.ESP & 0xFFFF0000 | outer_ESP & 0xFFFF);

      // Finally now that we can't fail, sort out the registers
      // m_registers.ESP = outer_ESP;
      SetFlags(return_EFLAGS);

      // Validate segments ES,FS,GS,DS so the kernel doesn't leak them
      static const Segment validate_segments[] = {Segment_ES, Segment_FS, Segment_GS, Segment_DS};
      for (uint32 i = 0; i < countof(validate_segments); i++)
      {
        Segment validate_segment = validate_segments[i];
        const SegmentCache* validate_segment_cache = &m_segment_cache[validate_segment];
        if ((!validate_segment_cache->access.is_code || !validate_segment_cache->access.code_confirming) &&
            validate_segment_cache->dpl < target_selector.rpl)
        {
          // If data or non-conforming code, set null selector
          LoadSegmentRegister(validate_segment, 0);
        }
      }

      BranchTo(return_EIP);
    }
    else
    {
      Assert(target_selector.rpl == GetCPL());

      // Returning to the same privilege level
      LoadSegmentRegister(Segment_CS, return_CS);
      SetFlags(return_EFLAGS);
      BranchTo(return_EIP);
    }
  }
}

void CPU::SetupInterruptCall(uint32 interrupt, bool software_interrupt, bool push_error_code, uint32 error_code,
                             uint32 return_EIP)
{
  // Log_DevPrintf("Interrupt %02Xh", interrupt);
  if (!software_interrupt)
    Log_TracePrintf("Hardware interrupt %u", interrupt);

  // Interrupts in V8086 mode with IOPL != 3 trap to monitor
  if (InVirtual8086Mode() && GetIOPL() != 3 && software_interrupt)
  {
    RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  if (InRealMode())
    SetupRealModeInterruptCall(interrupt, return_EIP);
  else
    SetupProtectedModeInterruptCall(interrupt, software_interrupt, push_error_code, error_code, return_EIP);
}

void CPU::SetupRealModeInterruptCall(uint32 interrupt, uint32 return_EIP)
{
  // Read IVT
  // TODO: Check limit?
  PhysicalMemoryAddress address = m_idt_location.base_address;
  address += PhysicalMemoryAddress(interrupt) * 4;

  uint32 ivt_entry = 0;
  SafeReadMemoryDWord(address, &ivt_entry, false, true);

  // Extract segment/instruction pointer from IDT entry
  uint16 isr_segment_selector = uint16(ivt_entry >> 16);
  uint32 isr_EIP = ZeroExtend32(ivt_entry & 0xFFFF);

  // Load segment selector first in case it throws an exception
  uint16 old_CS = m_registers.CS;
  LoadSegmentRegister(Segment_CS, isr_segment_selector);

  // Push FLAGS, CS, IP
  PushWord(Truncate16(m_registers.EFLAGS.bits));
  PushWord(old_CS);
  PushWord(Truncate16(return_EIP));

  // Clear interrupt flag if set (stop recursive interrupts)
  m_registers.EFLAGS.IF = false;
  m_registers.EFLAGS.TF = false;
  m_registers.EFLAGS.AC = false;

  // Resume code execution at interrupt entry point
  BranchFromException(isr_EIP);
}

void CPU::SetupProtectedModeInterruptCall(uint32 interrupt, bool software_interrupt, bool push_error_code,
                                          uint32 error_code, uint32 return_EIP)
{
  auto MakeErrorCode = [](uint32 num, uint8 idt, bool software_interrupt) {
    if (idt == 0)
      return ((num & 0xFC) | BoolToUInt32(!software_interrupt));
    else
      return ((num << 3) | 2 | BoolToUInt32(!software_interrupt));
  };

  // Check against bounds of the IDT
  DESCRIPTOR_ENTRY descriptor;
  if (!ReadDescriptorEntry(&descriptor, m_idt_location, interrupt))
  {
    // Raise GPF for out-of-range.
    // TODO: Arguments
    Log_WarningPrintf("Interrupt out of range: %u (limit %u)", interrupt, uint32(m_idt_location.limit));
    RaiseException(Interrupt_GeneralProtectionFault, MakeErrorCode(interrupt, 1, software_interrupt));
    return;
  }

  // Check type
  if (descriptor.type != DESCRIPTOR_TYPE_INTERRUPT_GATE_16 && descriptor.type != DESCRIPTOR_TYPE_INTERRUPT_GATE_32 &&
      descriptor.type != DESCRIPTOR_TYPE_TRAP_GATE_16 && descriptor.type != DESCRIPTOR_TYPE_TRAP_GATE_32 &&
      descriptor.type != DESCRIPTOR_TYPE_TASK_GATE)
  {
    Log_WarningPrintf("Invalid IDT gate type");
    RaiseException(Interrupt_GeneralProtectionFault, MakeErrorCode(interrupt, 1, software_interrupt));
    return;
  }

  // Software interrupts have to check that CPL <= DPL to access privileged interrupts
  if (software_interrupt && descriptor.dpl < GetCPL())
  {
    RaiseException(Interrupt_GeneralProtectionFault, MakeErrorCode(interrupt, 1, software_interrupt));
    return;
  }

  // Check present flag
  if (!descriptor.IsPresent())
  {
    Log_WarningPrintf("IDT gate not present");
    RaiseException(Interrupt_GeneralProtectionFault, MakeErrorCode(interrupt, 1, software_interrupt));
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
      RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }
    if (!ReadDescriptorEntry(&target_descriptor, target_selector.ti ? m_ldt_location : m_gdt_location,
                             target_selector.index) ||
        !target_descriptor.IsCodeSegment())
    {
      // TODO: Correct error code here, should be error_code(num,int,ext)
      RaiseException(Interrupt_GeneralProtectionFault, target_selector.ValueForException());
      return;
    }
    if (!target_descriptor.IsPresent())
    {
      RaiseException(Interrupt_SegmentNotPresent, target_selector.ValueForException());
      return;
    }

    // Does this result in a privilege change?
    if (!target_descriptor.memory.IsConformingCodeSegment() && target_descriptor.dpl < m_cpl)
    {
      bool is_virtual_8086_exit = InVirtual8086Mode();
      if (is_virtual_8086_exit)
      {
        // Leaving V8086 mode via trap
        Log_DevPrintf("Leaving V8086 mode via gate %u", interrupt);
        target_selector.rpl = 0;
      }
      else
      {
        Log_DevPrintf("Privilege raised via interrupt gate, %u -> %u", ZeroExtend32(GetCPL()),
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
      uint32 new_ESP = 0;
      uint16 new_SS = 0;
      if (m_tss_location.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16)
      {
        LinearMemoryAddress tss_stack_offset =
          Truncate32(offsetof(TASK_STATE_SEGMENT_16, stacks[0]) + target_selector.rpl * sizeof(4));
        if ((tss_stack_offset + 3) > m_tss_location.limit)
        {
          RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
          return;
        }

        // Shouldn't fail, since we're bypassing access checks
        uint16 temp = 0;
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset, &temp, false, true);
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset + 2, &new_SS, false, true);
        new_ESP = ZeroExtend32(temp);
      }
      else
      {
        LinearMemoryAddress tss_stack_offset =
          Truncate32(offsetof(TASK_STATE_SEGMENT_32, stacks[0]) + target_selector.rpl * sizeof(8));
        if ((tss_stack_offset + 5) > m_tss_location.limit)
        {
          RaiseException(Interrupt_InvalidTaskStateSegment, m_registers.TR);
          return;
        }

        // Shouldn't fail, since we're bypassing access checks
        SafeReadMemoryDWord(m_tss_location.base_address + tss_stack_offset, &new_ESP, false, true);
        SafeReadMemoryWord(m_tss_location.base_address + tss_stack_offset + 4, &new_SS, false, true);
      }

      // Save the old (outer) ESP/SS before we pop the parameters off?
      uint32 return_EFLAGS = m_registers.EFLAGS.bits;
      uint32 return_ESP = m_registers.ESP;
      uint16 return_CS = m_registers.CS;
      uint16 return_SS = m_registers.SS;

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
      if (is_virtual_8086_exit)
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
      uint32 new_EIP = descriptor.call_gate.GetOffset();
      if (descriptor.type == DESCRIPTOR_TYPE_INTERRUPT_GATE_16)
        new_EIP &= 0xFFFF;
      BranchFromException(new_EIP);
    }
    else
    {
      // Trap to V8086 monitor
      if (InVirtual8086Mode())
      {
        RaiseException(Interrupt_GeneralProtectionFault, 0);
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
    Log_DevPrintf("Task gate -> 0x%04X", ZeroExtend32(descriptor.task_gate.selector.GetValue()));
    SwitchToTask(descriptor.task_gate.selector, true, false, push_error_code, error_code);
  }
  else
  {
    Panic("Unhandled gate type");
  }
}

void CPU::CheckFloatingPointException()
{
  if ((m_registers.CR0 & (CR0Bit_MP | CR0Bit_TS)) == (CR0Bit_MP | CR0Bit_TS))
  {
    RaiseException(Interrupt_CoprocessorNotAvailable, 0);
    return;
  }

  // Check and handle any pending floating point exceptions
  if (!m_fpu_exception)
    return;

  m_fpu_exception = false;
  if (m_registers.CR0 & CR0Bit_NE)
  {
    RaiseException(Interrupt_MathFault);
    return;
  }

  // Compatibility mode via the PIC.
  m_system->GetInterruptController()->TriggerInterrupt(13);
}

inline constexpr bool IsAvailableTaskDescriptorType(uint8 type)
{
  return (type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16 || type == DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32);
}

inline constexpr bool IsBusyTaskDescriptorType(uint8 type)
{
  return (type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16 || type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32);
}

void CPU::SwitchToTask(uint16 new_task, bool nested_task, bool from_iret, bool push_error_code, uint32 error_code)
{
  Log_DevPrintf("Switching to task %02X%s", ZeroExtend32(new_task), nested_task ? " (nested)" : "");

  // Read the current task descriptor. This should never fail.
  SEGMENT_SELECTOR_VALUE current_task_selector = {m_registers.TR};
  DESCRIPTOR_ENTRY current_task_descriptor;
  if (current_task_selector.index == 0 || current_task_selector.ti ||
      !ReadDescriptorEntry(&current_task_descriptor, m_gdt_location, current_task_selector.index))
  {
    RaiseException(Interrupt_SegmentNotPresent, current_task_selector.ValueForException());
    return;
  }

  // The current task should be busy, but it's not fatal if it isn't.
  if (current_task_descriptor.is_memory_descriptor || (!IsAvailableTaskDescriptorType(current_task_descriptor.type) &&
                                                       !IsBusyTaskDescriptorType(current_task_descriptor.type)))
  {
    Log_WarningPrintf("Outgoing task descriptor is not valid - type %u",
                      ZeroExtend32(current_task_descriptor.type.GetValue()));

    // TODO: Is this correct?
    RaiseException(Interrupt_GeneralProtectionFault, current_task_selector.ValueForException());
    return;
  }

  // The limit should be enough for the task state
  bool current_task_is_32bit = ((current_task_descriptor.type & 8) != 0);
  uint32 current_tss_min_size = (current_task_is_32bit) ? sizeof(TASK_STATE_SEGMENT_32) : sizeof(TASK_STATE_SEGMENT_16);
  if (current_task_descriptor.tss.GetLimit() < current_tss_min_size)
  {
    // TODO: Is this correct?
    Log_WarningPrintf("Outgoing task segment is too small - %u required %u", current_task_descriptor.tss.GetLimit(),
                      current_tss_min_size);
    RaiseException(Interrupt_InvalidTaskStateSegment, current_task_selector.ValueForException());
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
  // TODO: Privilege level check should not happen on interrupt or IRET.
  SEGMENT_SELECTOR_VALUE new_task_selector = {new_task};
  DESCRIPTOR_ENTRY new_task_descriptor;
  if (new_task_selector.index == 0 || new_task_selector.ti ||
      !ReadDescriptorEntry(&new_task_descriptor, m_gdt_location, new_task_selector.index) ||
      new_task_descriptor.dpl < GetCPL() || new_task_descriptor.dpl < new_task_selector.rpl)
  {
    Log_WarningPrintf("Incoming task descriptor is not valid or privilege error");
    RaiseException(Interrupt_GeneralProtectionFault, new_task_selector.ValueForException());
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
                   new_task_selector.ValueForException());
    return;
  }

  // Check the limit on the new task state
  bool new_task_is_32bit = ((new_task_descriptor.type & 8) != 0);
  uint32 new_tss_min_size = (new_task_is_32bit) ? sizeof(TASK_STATE_SEGMENT_32) : sizeof(TASK_STATE_SEGMENT_16);
  if (new_task_descriptor.tss.GetLimit() < new_tss_min_size)
  {
    // TODO: Is this correct?
    Log_WarningPrintf("Incoming task segment is too small - %u required %u", new_task_descriptor.tss.GetLimit(),
                      new_tss_min_size);
    RaiseException(Interrupt_InvalidTaskStateSegment, new_task_selector.ValueForException());
    return;
  }

  // Calculate linear addresses of task segments
  LinearMemoryAddress current_tss_address = current_task_descriptor.tss.GetBase();
  LinearMemoryAddress new_tss_address = new_task_descriptor.tss.GetBase();
  union
  {
    TASK_STATE_SEGMENT_16 ts16;
    TASK_STATE_SEGMENT_32 ts32;
    uint16 words[sizeof(TASK_STATE_SEGMENT_16) / sizeof(uint16)];
    uint32 dwords[sizeof(TASK_STATE_SEGMENT_32) / sizeof(uint32)];
    uint16 link;
  } current_task_state = {}, new_task_state = {};

  // Read the current TSS in, this could cause a page fault
  if (current_task_is_32bit)
  {
    for (uint32 i = 0; i < countof(current_task_state.dwords); i++)
      SafeReadMemoryDWord(current_tss_address + i * sizeof(uint32), &current_task_state.dwords[i], false, true);
  }
  else
  {
    for (uint32 i = 0; i < countof(current_task_state.words); i++)
      SafeReadMemoryWord(current_tss_address + i * sizeof(uint16), &current_task_state.words[i], false, true);
  }

  // Read the new TSS in, this could cause a page fault
  if (new_task_is_32bit)
  {
    for (uint32 i = 0; i < countof(new_task_state.dwords); i++)
      SafeReadMemoryDWord(new_tss_address + i * sizeof(uint32), &new_task_state.dwords[i], false, true);
  }
  else
  {
    for (uint32 i = 0; i < countof(new_task_state.words); i++)
      SafeReadMemoryWord(new_tss_address + i * sizeof(uint16), &new_task_state.words[i], false, true);
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
    for (uint32 i = 0; i < countof(current_task_state.dwords); i++)
      SafeWriteMemoryDWord(current_tss_address + i * sizeof(uint32), current_task_state.dwords[i], false, true);
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
    for (uint32 i = 0; i < countof(current_task_state.words); i++)
      SafeWriteMemoryWord(current_tss_address + i * sizeof(uint16), current_task_state.words[i], false, true);
  }

  // If we're a nested task, set the backlink field in the new TSS
  if (nested_task)
  {
    new_task_state.link = current_task_selector.bits;
    SafeWriteMemoryWord(new_tss_address, new_task_state.words[0], false, true);
  }

  // New task is now busy
  if (!from_iret)
  {
    new_task_descriptor.type =
      (new_task_is_32bit) ? DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32 : DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16;
    if (!WriteDescriptorEntry(new_task_descriptor, m_gdt_location, new_task_selector.index))
      Panic("Failed to update descriptor entry that was successfully read");
  }

  // Checks that should be performed:
  // http://www.intel.com/design/pentium/MANUALS/24143004.pdf page 342.
  // Also note:
  //   Any errors detected in this step occur in the context of the new task. To an
  //   exception handler, the first instruction of the new task appears not to have executed.

  // Now we can load the new task in
  // TODO: Validate segment descriptors before loading them
  // TODO: set all segments to zero first?
  // TODO: Loading LDT when not present should raise invalid TSS, not #NP.
  uint32 new_EIP;
  if (new_task_is_32bit)
  {
    // TODO: Flags should be loaded before segment registers because of V8086.
    // CS has to be loaded first, because the other segments descriptor levels depend on it.
    // This would cause a TLB flush.
    LoadSpecialRegister(Reg32_CR3, new_task_state.ts32.CR3);
    LoadLocalDescriptorTable(new_task_state.ts32.LDTR);
    LoadSegmentRegister(Segment_CS, new_task_state.ts32.CS);
    LoadSegmentRegister(Segment_ES, new_task_state.ts32.ES);
    LoadSegmentRegister(Segment_SS, new_task_state.ts32.SS);
    LoadSegmentRegister(Segment_DS, new_task_state.ts32.DS);
    LoadSegmentRegister(Segment_FS, new_task_state.ts32.FS);
    LoadSegmentRegister(Segment_GS, new_task_state.ts32.GS);
    SetFlags(new_task_state.ts32.EFLAGS);
    m_registers.EAX = new_task_state.ts32.EAX;
    m_registers.ECX = new_task_state.ts32.ECX;
    m_registers.EDX = new_task_state.ts32.EDX;
    m_registers.EBX = new_task_state.ts32.EBX;
    m_registers.ESP = new_task_state.ts32.ESP;
    m_registers.EBP = new_task_state.ts32.EBP;
    m_registers.ESI = new_task_state.ts32.ESI;
    m_registers.EDI = new_task_state.ts32.EDI;
    new_EIP = new_task_state.ts32.EIP;
  }
  else
  {
    LoadLocalDescriptorTable(new_task_state.ts16.LDTR);
    LoadSegmentRegister(Segment_CS, new_task_state.ts16.CS);
    LoadSegmentRegister(Segment_ES, new_task_state.ts16.ES);
    LoadSegmentRegister(Segment_SS, new_task_state.ts16.SS);
    LoadSegmentRegister(Segment_DS, new_task_state.ts16.DS);
    LoadSegmentRegister(Segment_FS, 0);
    LoadSegmentRegister(Segment_GS, 0);
    SetFlags(ZeroExtend32(new_task_state.ts16.FLAGS));
    m_registers.EAX = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.AX);
    m_registers.ECX = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.CX);
    m_registers.EDX = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.DX);
    m_registers.EBX = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.BX);
    m_registers.ESP = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.SP);
    m_registers.EBP = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.BP);
    m_registers.ESI = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.SI);
    m_registers.EDI = UINT32_C(0xFFFF0000) | ZeroExtend32(new_task_state.ts16.DI);
    new_EIP = ZeroExtend32(new_task_state.ts16.IP);
  }

  // Update TR with the new task
  m_registers.TR = new_task;
  m_tss_location.base_address = new_task_descriptor.tss.GetBase();
  m_tss_location.limit = new_task_descriptor.tss.GetLimit();
  m_tss_location.type = static_cast<DESCRIPTOR_TYPE>(new_task_descriptor.type.GetValue());

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

bool CPU::HasIOPermissions(uint32 port_number, uint32 port_count, bool raise_exceptions)
{
  // All access is permitted in real mode
  if (InRealMode())
    return true;

  // CPL>IOPL or V8086 must check IO permission map
  if (GetCPL() <= GetIOPL() && !InVirtual8086Mode())
    return true;

  // But this doesn't exist on the 286
  if (m_model == MODEL_286 || m_tss_location.type == DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16)
  {
    return false;
  }

  // Check TSS validity
  if ((offsetof(TASK_STATE_SEGMENT_32, IOPB_offset) + (sizeof(uint16) - 1)) > m_tss_location.limit)
    return false;

  // Get IOPB offset
  uint16 iopb_offset;
  LinearMemoryAddress iopb_offset_address = m_tss_location.base_address + offsetof(TASK_STATE_SEGMENT_32, IOPB_offset);
  SafeReadMemoryWord(iopb_offset_address, &iopb_offset, false, true);

  // Find the offset in the IO bitmap
  uint32 bitmap_byte_offset = port_number / 8;
  uint32 bitmap_bit_offset = port_number % 8;

  // Check that it's not over a byte boundary
  bool spanning_byte = ((bitmap_bit_offset + port_count) > 8);
  uint32 read_byte_count = spanning_byte ? 2 : 1;

  // Check against TSS limits
  if ((ZeroExtend32(iopb_offset) + (read_byte_count - 1)) > m_tss_location.limit)
    return false;

  // A value of 1 in the bitmap means no access
  uint8 mask = ((1 << port_count) - 1);

  // Spanning a byte boundary?
  if (spanning_byte)
  {
    // Need to test against a word
    uint16 permissions;
    SafeReadMemoryWord(m_tss_location.base_address + ZeroExtend32(iopb_offset) + bitmap_byte_offset, &permissions,
                       false, true);
    if (((permissions >> bitmap_bit_offset) & ZeroExtend16(mask)) != 0)
      return false;
  }
  else
  {
    // Test against the single byte
    uint8 permissions;
    SafeReadMemoryByte(m_tss_location.base_address + ZeroExtend32(iopb_offset) + bitmap_byte_offset, &permissions,
                       false, true);
    if (((permissions >> bitmap_bit_offset) & mask) != 0)
      return false;
  }

  // IO operation is allowed
  return true;
}

void CPU::DumpPageTable()
{
  std::fprintf(stderr, "Page table\n");
  if (!InProtectedMode() && !(m_registers.CR0 & CR0Bit_PE))
    return;

  PhysicalMemoryAddress directory_address = (m_registers.CR3 & 0xFFFFF000);
  for (uint32 i = 0; i < 1024; i++)
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
    uint32 page_count = 0;
    for (uint32 j = 0; j < 1024; j++)
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

void CPU::DumpMemory(LinearMemoryAddress start_address, uint32 size)
{
  DebugAssert(size > 0);
  LinearMemoryAddress end_address = start_address + size - 1;
  std::fprintf(stderr, "Memory dump from 0x%08X - 0x%08X\n", start_address, end_address);

  LinearMemoryAddress current_address = start_address;
  while (current_address <= end_address)
  {
    static constexpr uint32 COLUMNS = 16;
    LinearMemoryAddress row_address = current_address;

    SmallString hex;
    TinyString printable;

    for (uint32 i = 0; i < COLUMNS; i++)
    {
      uint8 value;
      if (!SafeReadMemoryByte(current_address++, &value, false, false))
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
  LinearMemoryAddress stack_top = m_registers.ESP;
  LinearMemoryAddress stack_bottom = 0xFFFFFFFF;
  if (m_stack_address_size == AddressSize_16)
  {
    stack_top &= 0xFFFF;
    stack_bottom &= 0xFFFF;
  }

  std::fprintf(stderr, "Stack dump, ESP = 0x%08X\n", m_registers.ESP);

  LinearMemoryAddress stack_address = stack_top;
  for (uint32 count = 0; count < 128; count++)
  {
    if (m_stack_address_size == AddressSize_16)
    {
      uint16 value;
      if (!SafeReadMemoryWord(stack_address, &value, false, false))
        break;
      std::fprintf(stderr, "  0x%04X: 0x%04X\n", stack_address, ZeroExtend32(value));
    }
    else
    {
      uint32 value;
      if (!SafeReadMemoryDWord(stack_address, &value, false, false))
        break;
      std::fprintf(stderr, "  0x%08X: 0x%08X\n", stack_address, value);
    }

    if (stack_address == stack_bottom)
      break;

    if (m_stack_address_size == AddressSize_16)
      stack_address += sizeof(uint16);
    else
      stack_address += sizeof(uint32);
  }
}

size_t CPU::GetTLBEntryIndex(uint32 linear_address)
{
  // Maybe a better hash function should be used here?
  return size_t(linear_address >> 12) % TLB_ENTRY_COUNT;
}

void CPU::InvalidateAllTLBEntries()
{
#ifdef ENABLE_TLB_EMULATION
  std::memset(m_tlb_entries, 0xFF, sizeof(m_tlb_entries));
#endif
}

void CPU::InvalidateTLBEntry(uint32 linear_address)
{
#ifdef ENABLE_TLB_EMULATION
  linear_address &= PAGE_MASK;

  size_t index = GetTLBEntryIndex(linear_address);
  for (uint32 user_supervisor = 0; user_supervisor < 2; user_supervisor++)
  {
    for (uint32 write_read = 0; write_read < 2; write_read++)
    {
      TLBEntry& entry = m_tlb_entries[user_supervisor][write_read][index];
      if (entry.linear_address == linear_address)
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
  if (!TranslateLinearAddress(&physical_address, linear_address, true, AccessType::Execute, false))
  {
    // Use direct fetch and page fault when it fails.
    return false;
  }

#if 1
  // Fast path: we're fetching from a RAM page.
  const byte* ram_ptr = m_bus->GetRAMPointer(physical_address);
  if (ram_ptr)
  {
    std::memcpy(m_prefetch_queue, ram_ptr, PREFETCH_QUEUE_SIZE);
    m_prefetch_queue_size = PREFETCH_QUEUE_SIZE;
    return true;
  }

  // Slow path: it's a MMIO page, or locked memory.
  while (m_prefetch_queue_size < PREFETCH_QUEUE_SIZE)
  {
    uint64 value = m_bus->ReadMemoryQWord(physical_address);
    std::memcpy(&m_prefetch_queue[m_prefetch_queue_size], &value, sizeof(uint64));
    m_prefetch_queue_size += sizeof(value);
    linear_address += sizeof(value);
    physical_address += sizeof(value);
  }
#else
  m_bus->ReadMemoryBlock(physical_address, PREFETCH_QUEUE_SIZE, m_prefetch_queue);
  m_prefetch_queue_size = PREFETCH_QUEUE_SIZE;
#endif

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
