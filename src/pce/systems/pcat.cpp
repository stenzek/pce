#include "pce/systems/pcat.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu.h"
#include "pce/cpu_x86/cpu.h"
Log_SetChannel(Systems::PCAT);

namespace Systems {

PCAT::PCAT(HostInterface* host_interface, float cpu_frequency /* = 2000000.0f */,
           uint32 memory_size /* = 1024 * 1024 */)
  : PCBase(host_interface)
{
  m_cpu = new CPU_X86::CPU(CPU_X86::MODEL_286, cpu_frequency);
  m_bus = new Bus(PHYSICAL_MEMORY_BITS);
  AllocatePhysicalMemory(memory_size, true);
  ConnectSystemIOPorts();
  AddComponents();
  SetCMOSVariables();
}

PCAT::~PCAT() {}

void PCAT::Reset()
{
  PCBase::Reset();

  // Default gate A20 to on
  IOWriteSystemControlPortA((1 << 1));
}

bool PCAT::LoadSystemState(BinaryReader& reader)
{
  if (!PCBase::LoadSystemState(reader))
    return false;

  reader.SafeReadUInt8(&m_system_control_port_a.raw);
  return true;
}

bool PCAT::SaveSystemState(BinaryWriter& writer)
{
  if (!PCBase::SaveSystemState(writer))
    return false;

  writer.SafeWriteUInt8(m_system_control_port_a.raw);
  return true;
}

void PCAT::ConnectSystemIOPorts()
{
  m_bus->ConnectIOPortReadToPointer(0x0092, this, &m_system_control_port_a.raw);
  m_bus->ConnectIOPortWrite(0x0092, this, std::bind(&PCAT::IOWriteSystemControlPortA, this, std::placeholders::_2));

  //     // NFI what this is...
  m_bus->ConnectIOPortRead(0x0061, this, [](uint32 port, uint8* value) {
    static uint8 refresh_request_bit = 0x00;
    *value = refresh_request_bit;
    refresh_request_bit ^= 0x10;
  });
}

void PCAT::IOWriteSystemControlPortA(uint8 value)
{
  m_system_control_port_a.raw = value;
  SetA20State(m_system_control_port_a.a20_gate);
  Log_DevPrintf("A20 gate is %s", m_system_control_port_a.a20_gate ? "on" : "off");

#if 0
if (!m_system_control_port_a.system_reset)
    {
        Log_DevPrintf("System reset via system control port");
        m_system_control_port_a.system_reset = true;
        m_cpu->Reset();
    }
#endif // 0
}

void PCAT::AddComponents()
{
  m_keyboard_controller = new HW::PS2Controller();
  m_dma_controller = new HW::i8237_DMA();
  m_timer = new HW::i8253_PIT();
  m_interrupt_controller = new HW::i8259_PIC();
  m_cmos = new HW::CMOS();

  AddComponent(m_interrupt_controller);
  AddComponent(m_dma_controller);
  AddComponent(m_timer);
  AddComponent(m_keyboard_controller);
  AddComponent(m_cmos);

  m_fdd_controller = new HW::FDC(m_dma_controller);
  m_hdd_controller = new HW::HDC(HW::HDC::CHANNEL_PRIMARY);

  AddComponent(m_fdd_controller);
  AddComponent(m_hdd_controller);
}

void PCAT::SetCMOSVariables()
{
  PhysicalMemoryAddress base_memory = GetBaseMemorySize();
  PhysicalMemoryAddress extended_memory = GetExtendedMemorySize();
  PhysicalMemoryAddress kb;

  // Base memory
  kb = base_memory / 1024;
  m_cmos->SetVariable(0x15, Truncate8(kb));
  m_cmos->SetVariable(0x16, Truncate8(kb >> 8));

  // Extended memory
  kb = extended_memory / 1024;
  m_cmos->SetVariable(0x17, Truncate8(kb));
  m_cmos->SetVariable(0x18, Truncate8(kb >> 8));
}

} // namespace Systems