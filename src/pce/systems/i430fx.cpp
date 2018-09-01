#include "pce/systems/i430fx.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu.h"
Log_SetChannel(Systems::i430FX);

namespace Systems {
DEFINE_OBJECT_TYPE_INFO(i430FX);
DEFINE_OBJECT_GENERIC_FACTORY(i430FX);
BEGIN_OBJECT_PROPERTY_MAP(i430FX)
END_OBJECT_PROPERTY_MAP()

i430FX::i430FX(CPU_X86::Model model /* = CPU_X86::MODEL_PENTIUM */, float cpu_frequency /* = 2000000.0f */,
               uint32 memory_size /* = 16 * 1024 * 1024 */)
  : PCIPC(PCIPC::PCIConfigSpaceAccessType::Type1), m_bios_file_path("romimages/5ifw001.bin")
{
  m_cpu = new CPU_X86::CPU(model, cpu_frequency);
  m_bus = new Bus(PHYSICAL_MEMORY_BITS);
  AllocatePhysicalMemory(memory_size, false, false);
  AddComponents();
}

i430FX::~i430FX() {}

bool i430FX::Initialize()
{
  if (!PCIPC::Initialize())
    return false;

  // We have to use MMIO ROMs, because the shadowed region can only be RAM or ROM, not both.
  // The upper binding is okay to keep as a ROM region, though, since we don't shadow it.
  if (!m_bus->CreateROMRegionFromFile(m_bios_file_path.c_str(), BIOS_ROM_ADDRESS, BIOS_ROM_SIZE) ||
      !m_bus->CreateROMRegionFromFile(m_bios_file_path.c_str(), BIOS_ROM_MIRROR_ADDRESS, BIOS_ROM_SIZE))
  {
    return false;
  }

  ConnectSystemIOPorts();
  SetCMOSVariables();
  return true;
}

void i430FX::Reset()
{
  PCIPC::Reset();

  m_cmos_lock = false;
  m_refresh_bit = false;

  // Set keyboard controller input port up.
  // b7 = Keyboard not inhibited, b5 = POST loop inactive
  m_keyboard_controller->SetInputPort(0xA0);

  // Start with A20 line on
  SetA20State(true);
  UpdateKeyboardControllerOutputPort();
}

bool i430FX::LoadSystemState(BinaryReader& reader)
{
  if (!ISAPC::LoadSystemState(reader))
    return false;

  reader.SafeReadBool(&m_cmos_lock);
  reader.SafeReadBool(&m_refresh_bit);
  return !reader.GetErrorState();
}

bool i430FX::SaveSystemState(BinaryWriter& writer)
{
  if (!ISAPC::SaveSystemState(writer))
    return false;

  writer.SafeWriteBool(m_cmos_lock);
  writer.SafeWriteBool(m_refresh_bit);
  return !writer.InErrorState();
}

void i430FX::ConnectSystemIOPorts()
{
  // System control ports
  m_bus->ConnectIOPortRead(0x0092, this, std::bind(&i430FX::IOReadSystemControlPortA, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0092, this, std::bind(&i430FX::IOWriteSystemControlPortA, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x0061, this, std::bind(&i430FX::IOReadSystemControlPortB, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0061, this, std::bind(&i430FX::IOWriteSystemControlPortB, this, std::placeholders::_2));

  // Dummy I/O delay port
  m_bus->ConnectIOPortRead(0x00EB, this, [](uint32, uint8* value) { *value = 0xFF; });
  m_bus->ConnectIOPortWrite(0x00EB, this, [](uint32, uint8 value) {});

  // Connect the keyboard controller output port to the lower 2 bits of system control port A.
  m_keyboard_controller->SetOutputPortWrittenCallback([this](uint8 value, uint8 old_value, bool pulse) {
    if (!pulse)
      value &= ~uint8(0x01);
    IOWriteSystemControlPortA(value & 0x03);
    IOReadSystemControlPortA(&value);
    m_keyboard_controller->SetOutputPort(value);
  });
}

void i430FX::IOReadSystemControlPortA(uint8* value)
{
  *value = (BoolToUInt8(m_cmos_lock) << 3) | (BoolToUInt8(GetA20State()) << 1);
}

void i430FX::IOWriteSystemControlPortA(uint8 value)
{
  Log_DevPrintf("Write system control port A: 0x%02X", ZeroExtend32(value));

  // b7-6 - Activity Lights
  // b5 - Reserved
  // b4 - Watchdog Timeout
  // b3 - CMOS Security Lock
  // b2 - Reserved
  // b1 - A20 Active
  // b0 - System Reset

  bool cmos_security_lock = !!(value & (1 << 2));
  bool new_a20_state = !!(value & (1 << 1));
  bool system_reset = !!(value & (1 << 0));

  // Update A20 state
  if (GetA20State() != new_a20_state)
  {
    SetA20State(new_a20_state);
    UpdateKeyboardControllerOutputPort();
  }

  m_cmos_lock = cmos_security_lock;

  // System reset?
  // We do this last as it's going to destroy everything.
  // TODO: We should probably put it on an event though..
  if (system_reset)
  {
    Log_WarningPrintf("CPU reset via system control port");
    m_cpu->Reset();
  }
}

void i430FX::IOReadSystemControlPortB(uint8* value)
{
  *value = (BoolToUInt8(m_timer->GetChannelGateInput(2)) << 0) |  // Timer 2 gate input
           (BoolToUInt8(m_speaker->IsOutputEnabled()) << 1) |     // Speaker data status
           (BoolToUInt8(m_refresh_bit) << 4) |                    // Triggers with each memory refresh
           (BoolToUInt8(m_timer->GetChannelOutputState(2)) << 5); // Raw timer 2 output

  // Seems that we can get away with faking this every read.
  // The refresh controller steps one refresh address every 15 microseconds. Each refresh cycle
  // requires eight clock cycles to refresh all of the system's dynamic memory; 256 refresh cycles
  // are required every 4 milliseconds, but the system hardware refreshes every 3.84ms.
  m_refresh_bit ^= true;
}

void i430FX::IOWriteSystemControlPortB(uint8 value)
{
  Log_DevPrintf("Write system control port B: 0x%02X", ZeroExtend32(value));

  m_timer->SetChannelGateInput(2, !!(value & (1 << 0))); // Timer 2 gate input
  m_speaker->SetOutputEnabled(!!(value & (1 << 1)));     // Speaker data enable
}

void i430FX::UpdateKeyboardControllerOutputPort()
{
  uint8 value = m_keyboard_controller->GetOutputPort();
  value &= ~uint8(0x03);
  value |= (BoolToUInt8(GetA20State()) << 1);
  m_keyboard_controller->SetOutputPort(value);
}

void i430FX::AddComponents()
{
  AddPCIDeviceToLocation((m_sb82437 = new HW::i82437FX(this, m_bus)), 0, 0);

  AddComponent(m_interrupt_controller = new HW::i8259_PIC());
  AddComponent(m_dma_controller = new HW::i8237_DMA());
  AddComponent(m_timer = new HW::i8253_PIT());
  AddComponent(m_keyboard_controller = new HW::i8042_PS2());
  AddComponent(m_cmos = new HW::CMOS());

  AddComponent(m_fdd_controller = new HW::FDC(HW::FDC::Model_82077, m_dma_controller));
  AddComponent(m_primary_hdd_controller = new HW::HDC(HW::HDC::CHANNEL_PRIMARY));
  AddComponent(m_secondary_hdd_controller = new HW::HDC(HW::HDC::CHANNEL_SECONDARY));

  // Connect channel 0 of the PIT to the interrupt controller
  m_timer->SetChannelOutputChangeCallback(0,
                                          [this](bool value) { m_interrupt_controller->SetInterruptState(0, value); });

  AddComponent(m_speaker = new HW::PCSpeaker());

  // Connect channel 2 of the PIT to the speaker
  m_timer->SetChannelOutputChangeCallback(2, [this](bool value) { m_speaker->SetLevel(value); });
}

void i430FX::SetCMOSVariables() {}

} // namespace Systems