#include "pce/cpu_8086/cpu.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "common/fastjmp.h"
#include "pce/bus.h"
#include "pce/cpu_8086/debugger_interface.h"
#include "pce/cpu_8086/decoder.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cctype>
Log_SetChannel(CPU_8086);

namespace CPU_8086 {
DEFINE_NAMED_OBJECT_TYPE_INFO(CPU, "CPU_8086");
BEGIN_OBJECT_PROPERTY_MAP(CPU)
END_OBJECT_PROPERTY_MAP()

CPU::CPU(const String& identifier, Model model, float frequency, const ObjectTypeInfo* type_info)
  : BaseClass(identifier, frequency, BackendType::Interpreter, type_info), m_model(model)
{
}

CPU::~CPU() {}

const char* CPU::GetModelString() const
{
  static const char* model_name_strings[NUM_MODELS] = {"8088", "8086", "V20", "V30", "80188", "80186"};
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

  m_data_bus_is_8bit = (m_model == MODEL_8088 || m_model == MODEL_V20 || m_model == MODEL_80188);
  return true;
}

void CPU::Reset()
{
  BaseClass::Reset();

  m_pending_cycles = 0;
  m_execution_downcount = 0;

  Y_memzero(&m_registers, sizeof(m_registers));

  m_registers.FLAGS.bits = 0;
  m_registers.FLAGS.bits |= Flag_Reserved;
  if (m_model < MODEL_80186)
  {
    // IOPL NT, reserved are 1 on 8086
    m_registers.FLAGS.bits |= (1u << 12) | (1u << 13) | (1u << 14);
  }

  // Execution begins at F000:FFF0
  m_registers.CS = 0xF000;
  m_registers.IP = 0xFFF0;

  m_halted = false;
  m_nmi_state = false;

  m_current_IP = m_registers.IP;

  FlushPrefetchQueue();

  m_effective_address = 0;
  std::memset(&idata, 0, sizeof(idata));
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
  reader.SafeReadUInt16(&m_current_IP);

  for (u32 i = 0; i < Reg16_Count; i++)
    reader.SafeReadUInt16(&m_registers.reg16[i]);

  reader.SafeReadBool(&m_halted);
  reader.SafeReadBool(&m_nmi_state);
  reader.SafeReadBool(&m_irq_state);

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
  writer.WriteUInt16(m_current_IP);

  for (u32 i = 0; i < Reg16_Count; i++)
    writer.WriteUInt16(m_registers.reg16[i]);

  writer.WriteBool(m_halted);
  writer.WriteBool(m_nmi_state);
  writer.WriteBool(m_irq_state);

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

  if (state && m_halted && m_registers.FLAGS.IF)
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

bool CPU::SupportsBackend(BackendType mode)
{
  return (mode == BackendType::Interpreter);
}

void CPU::SetBackend(BackendType mode)
{
  Assert(mode == BackendType::Interpreter);
}

void CPU::ExecuteSlice(SimulationTime time)
{
  const CycleCount cycles_in_slice = (time + (m_cycle_period - 1)) / m_cycle_period;
  m_execution_downcount += cycles_in_slice;
  if (m_execution_downcount <= 0)
    return;

  fastjmp_set(&m_jmp_buf);

  do
  {
    // If we're halted, don't even bother calling into the backend.
    if (m_halted)
    {
      CommitPendingCycles();

      // Run as many ticks until we hit the downcount.
      const SimulationTime time_to_execute =
        std::max(m_system->GetTimingManager()->GetNextEventTime() - m_system->GetTimingManager()->GetPendingTime(),
                 SimulationTime(0));

      // Align the execution time to the cycle period, this way we don't drift due to halt.
      const CycleCount cycles_to_next_event =
        std::max((time_to_execute + m_cycle_period - 1) / m_cycle_period, CycleCount(1));
      const CycleCount cycles_to_idle = std::min(m_execution_downcount, cycles_to_next_event);
      m_system->GetTimingManager()->AddPendingTime(cycles_to_idle * m_cycle_period);
      m_execution_downcount -= cycles_to_idle;
      continue;
    }

    // Check for external interrupts.
    if (HasExternalInterrupt())
      DispatchExternalInterrupt();

    ExecuteInstruction();

    // Run events if needed.
    CommitPendingCycles();
  } while (m_execution_downcount > 0);

  // If we had a long-running instruction (e.g. a long REP), set downcount to zero, as it'll likely be negative.
  // This is safe, as any time-dependent events occur during CommitPendingCycles();
  m_execution_downcount = std::max(m_execution_downcount, CycleCount(0));
}

void CPU::StallExecution(SimulationTime time)
{
  const CycleCount cycles_in_slice = (time + (m_cycle_period - 1)) / m_cycle_period;
  m_execution_downcount -= cycles_in_slice;
  m_system->GetTimingManager()->AddPendingTime(cycles_in_slice * m_cycle_period);
}

void CPU::StopExecution()
{
  // Zero the downcount, causing the above loop to exit early.
  m_execution_downcount = 0;
}

void CPU::FlushCodeCache()
{
}

void CPU::GetExecutionStats(ExecutionStats* stats) const
{
  std::memcpy(stats, &m_execution_stats, sizeof(m_execution_stats));
}

void CPU::CommitPendingCycles()
{
  m_execution_stats.cycles_executed += m_pending_cycles;
  m_execution_downcount -= m_pending_cycles;
  m_system->GetTimingManager()->AddPendingTime(m_pending_cycles * GetCyclePeriod());
  m_pending_cycles = 0;
}

void CPU::AbortCurrentInstruction()
{
  Log_TracePrintf("Aborting instruction at %04X:%04X", m_registers.CS, m_registers.IP);

#ifdef ENABLE_PREFETCH_EMULATION
  u32 current_fetch_length = m_registers.IP - m_current_IP;
  if (m_prefetch_queue_position >= current_fetch_length)
    m_prefetch_queue_position -= current_fetch_length;
  else
    FlushPrefetchQueue();
#endif

  fastjmp_jmp(&m_jmp_buf);
}

void CPU::RestartCurrentInstruction()
{
// Reset EIP, so that we start fetching from the beginning of the instruction again.
#ifdef ENABLE_PREFETCH_EMULATION
  u32 current_fetch_length = m_registers.IP - m_current_IP;
  if (m_prefetch_queue_position >= current_fetch_length)
    m_prefetch_queue_position -= current_fetch_length;
  else
    FlushPrefetchQueue();
#endif
  m_registers.IP = m_current_IP;
  fastjmp_jmp(&m_jmp_buf);
}

u8 CPU::FetchInstructionByte()
{
#ifdef ENABLE_PREFETCH_EMULATION
  // It's possible this will still fail if we're at the end of the segment.
  u8 value;
  if ((m_prefetch_queue_size - m_prefetch_queue_position) >= sizeof(u8) || FillPrefetchQueue())
    value = m_prefetch_queue[m_prefetch_queue_position++];
  else
    value = FetchDirectInstructionByte(m_registers.IP);

  m_registers.IP += sizeof(u8);
  return value;
#else
  u8 value = FetchDirectInstructionByte(m_registers.IP);
  m_registers.IP += sizeof(u8);
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
    value = FetchDirectInstructionWord(m_registers.IP);
  }

  m_registers.IP += sizeof(u16);
  return value;
#else
  u16 value = FetchDirectInstructionWord(m_registers.IP);
  m_registers.IP += sizeof(u16);
  return value;
#endif
}

u8 CPU::FetchDirectInstructionByte(u16 address)
{
  return m_bus->ReadMemoryByte(CalculateLinearAddress(Segment_CS, address));
}

u16 CPU::FetchDirectInstructionWord(u16 address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, address);

  // If it crosses a page, we have to fetch bytes instead.
  if ((linear_address & Bus::MEMORY_PAGE_MASK) != ((linear_address + sizeof(u16) - 1) & Bus::MEMORY_PAGE_MASK))
  {
    u8 lsb = FetchDirectInstructionByte(address);
    u8 msb = FetchDirectInstructionByte(address + 1);
    return ZeroExtend16(lsb) | (ZeroExtend16(msb) << 8);
  }

  return m_bus->ReadMemoryWord(linear_address);
}

void CPU::PushWord(u16 value)
{
  m_registers.SP -= sizeof(u16);
  LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_SS, m_registers.SP);
  WriteMemoryWord(linear_address, value);
}

u16 CPU::PopWord()
{
  PhysicalMemoryAddress linear_address = CalculateLinearAddress(Segment_SS, m_registers.SP);
  m_registers.SP += sizeof(u16);
  return ReadMemoryWord(linear_address);
}

void CPU::SetFlags(u16 value)
{
  // Don't clear/set all flags, only those allowed
  const u16 MASK = Flag_CF | Flag_PF | Flag_AF | Flag_ZF | Flag_SF | Flag_TF | Flag_IF | Flag_DF | Flag_OF;
  m_registers.FLAGS.bits = (value & MASK) | (m_registers.FLAGS.bits & ~MASK);
}

void CPU::SetHalted(bool halt)
{
  if (halt)
    Log_TracePrintf("CPU Halt");

  m_halted = halt;
}

PhysicalMemoryAddress CPU::CalculateLinearAddress(Segment segment, VirtualMemoryAddress offset)
{
  DebugAssert(segment < Segment_Count);
  return (ZeroExtend32(m_registers.segment_selectors[segment]) << 4) + offset;
}

u8 CPU::ReadMemoryByte(LinearMemoryAddress address)
{
  AddMemoryCycle();
  return m_bus->ReadMemoryByte(address);
}

u16 CPU::ReadMemoryWord(LinearMemoryAddress address)
{
  AddMemoryCycle();

  // Unaligned access?
  if ((address & (sizeof(u16) - 1)) != 0)
  {
    // If the address falls within the same page we can still skip doing byte reads.
    if ((address & Bus::MEMORY_PAGE_MASK) != ((address + sizeof(u16) - 1) & Bus::MEMORY_PAGE_MASK))
    {
      // Fall back to byte reads.
      u8 b0 = ReadMemoryByte(address + 0);
      u8 b1 = ReadMemoryByte(address + 1);
      return ZeroExtend16(b0) | (ZeroExtend16(b1) << 8);
    }
  }

  return m_bus->ReadMemoryWord(address);
}

void CPU::WriteMemoryByte(LinearMemoryAddress address, u8 value)
{
  AddMemoryCycle();
  m_bus->WriteMemoryByte(address, value);
}

void CPU::WriteMemoryWord(LinearMemoryAddress address, u16 value)
{
  AddMemoryCycle();

  // Unaligned access?
  if ((address & (sizeof(u16) - 1)) != 0)
  {
    // If the address falls within the same page we can still skip doing byte reads.
    if ((address & Bus::MEMORY_PAGE_MASK) != ((address + sizeof(u16) - 1) & Bus::MEMORY_PAGE_MASK))
    {
      // Slowest path here.
      WriteMemoryByte((address + 0), Truncate8(value));
      WriteMemoryByte((address + 1), Truncate8(value >> 8));
      return;
    }
  }

  m_bus->WriteMemoryWord(address, value);
}

bool CPU::SafeReadMemoryByte(LinearMemoryAddress address, u8* value)
{
  return m_bus->CheckedReadMemoryByte(address, value);
}

bool CPU::SafeReadMemoryWord(LinearMemoryAddress address, u16* value)
{
  return m_bus->CheckedReadMemoryWord(address, value);
}

bool CPU::SafeWriteMemoryByte(LinearMemoryAddress address, u8 value)
{
  return m_bus->CheckedWriteMemoryByte(address, value);
}

bool CPU::SafeWriteMemoryWord(LinearMemoryAddress address, u16 value)
{
  return m_bus->CheckedWriteMemoryWord(address, value);
}

u8 CPU::ReadMemoryByte(Segment segment, VirtualMemoryAddress address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  return ReadMemoryByte(linear_address);
}

u16 CPU::ReadMemoryWord(Segment segment, VirtualMemoryAddress address)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  return ReadMemoryWord(linear_address);
}

void CPU::WriteMemoryByte(Segment segment, VirtualMemoryAddress address, u8 value)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  WriteMemoryByte(linear_address, value);
}

void CPU::WriteMemoryWord(Segment segment, VirtualMemoryAddress address, u16 value)
{
  LinearMemoryAddress linear_address = CalculateLinearAddress(segment, address);
  WriteMemoryWord(linear_address, value);
}

void CPU::PrintCurrentStateAndInstruction(const char* prefix_message /* = nullptr */)
{
  if (prefix_message)
  {
    std::fprintf(stdout, "%s at EIP = %04X:%08Xh (0x%08X)\n", prefix_message, m_registers.CS, m_current_IP,
                 CalculateLinearAddress(Segment_CS, m_current_IP));
  }

  std::fprintf(stdout, "AX=%04X BX=%04X CX=%04X DX=%04X SI=%04X DI=%04X\n", m_registers.AX, m_registers.BX,
               m_registers.CX, m_registers.DX, m_registers.SI, m_registers.DI);
  std::fprintf(stdout, "SP=%04X BP=%04X IP=%04X:%04X FLAGS=%04X ES=%04X SS=%04X DS=%04X\n", m_registers.SP,
               m_registers.BP, m_registers.CS, m_current_IP, m_registers.FLAGS.bits, m_registers.ES, m_registers.SS,
               m_registers.DS);

  u16 fetch_IP = m_current_IP;
  auto fetchb = [this, &fetch_IP](u8* val) {
    if (!SafeReadMemoryByte(CalculateLinearAddress(Segment_CS, fetch_IP), val))
      return false;
    fetch_IP += sizeof(u8);
    return true;
  };
  auto fetchw = [this, &fetch_IP](u16* val) {
    if (!SafeReadMemoryWord(CalculateLinearAddress(Segment_CS, fetch_IP), val))
      return false;
    fetch_IP += sizeof(u16);
    return true;
  };

  // Try to decode the instruction first.
  Instruction instruction;
  bool instruction_valid = Decoder::DecodeInstruction(&instruction, fetch_IP, std::move(fetchb), std::move(fetchw));

  SmallString hex_string;
  u16 instruction_length = instruction_valid ? instruction.length : 16;
  for (u16 i = 0; i < instruction_length; i++)
  {
    u8 value = 0;
    if (!SafeReadMemoryByte(CalculateLinearAddress(Segment_CS, m_current_IP + i), &value))
    {
      hex_string.AppendFormattedString(" <memory read failed at 0x%08X>",
                                       CalculateLinearAddress(Segment_CS, m_current_IP + i));
      i = instruction_length;
      break;
    }

    hex_string.AppendFormattedString("%02X ", ZeroExtend32(value));
  }

  if (instruction_valid)
  {
    SmallString instr_string;
    Decoder::DisassembleToString(&instruction, &instr_string);

    for (u16 i = instruction_length; i < 10; i++)
      hex_string.AppendString("   ");

    LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, m_current_IP);
    std::fprintf(stdout, "%04X:%08Xh (0x%08X) | %s | %s\n", m_registers.CS, m_current_IP, linear_address,
                 hex_string.GetCharArray(), instr_string.GetCharArray());
  }
  else
  {
    std::fprintf(stdout, "Decoding failed, bytes at failure point: %s\n", hex_string.GetCharArray());
  }
}

bool CPU::HasExternalInterrupt() const
{
  // TODO: NMI interrupts.
  // If there is a pending external interrupt and IF is set, jump to the interrupt handler.
  return (m_registers.FLAGS.IF & m_irq_state) != 0;
}

void CPU::DispatchExternalInterrupt()
{
  DebugAssert(HasExternalInterrupt());

  // m_current_EIP/ESP must match the current state in case setting up the interrupt throws an exception.
  m_current_IP = m_registers.IP;
  m_execution_stats.interrupts_serviced++;

  // Request interrupt number from the PIC.
  const u32 interrupt_number = m_interrupt_controller->GetInterruptNumber();
  SetupInterruptCall(interrupt_number, m_registers.IP);
}

void CPU::RaiseException(u32 interrupt)
{
  Log_DebugPrintf("Raise exception %u IP %04x:%04x", interrupt, m_registers.CS, m_current_IP);
  m_execution_stats.exceptions_raised++;

  // Set up the call to the corresponding interrupt vector.
  SetupInterruptCall(interrupt, m_current_IP);

  // Abort the current instruction that is executing.
  AbortCurrentInstruction();
}

void CPU::BranchTo(u16 new_IP)
{
  FlushPrefetchQueue();
  m_registers.IP = new_IP;
}

void CPU::BranchTo(u16 new_CS, u16 new_IP)
{
  FlushPrefetchQueue();
  m_registers.CS = new_CS;
  m_registers.IP = new_IP;
}

void CPU::SetupInterruptCall(u32 interrupt, u16 return_IP)
{
  // Read IVT
  PhysicalMemoryAddress ivt_address = PhysicalMemoryAddress(interrupt) * 4;

  // Extract segment/instruction pointer from IDT entry
  u16 isr_IP = ReadMemoryWord(ivt_address + 0);
  u16 isr_CS = ReadMemoryWord(ivt_address + 2);

  // Push FLAGS, CS, IP
  PushWord(Truncate16(m_registers.FLAGS.bits));
  PushWord(m_registers.CS);
  PushWord(Truncate16(return_IP));

  // Clear interrupt flag if set (stop recursive interrupts)
  m_registers.FLAGS.IF = false;
  m_registers.FLAGS.TF = false;

  // Resume code execution at interrupt entry point
  BranchTo(isr_CS, isr_IP);
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

  // Currently we can't handle crossing pages here.
  LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, m_registers.IP);
  if ((linear_address & Bus::MEMORY_PAGE_MASK) != ((linear_address + PREFETCH_QUEUE_SIZE - 1) & Bus::MEMORY_PAGE_MASK))
    return false;

  m_bus->ReadMemoryBlock(linear_address, PREFETCH_QUEUE_SIZE, m_prefetch_queue);
  m_prefetch_queue_size = PREFETCH_QUEUE_SIZE;
  return true;
#else
  return false;
#endif
}
} // namespace CPU_8086
