#include "pce/systems/i430fx.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "pce/hw/pci_bus.h"
Log_SetChannel(Systems::i430FX);

namespace Systems {
DEFINE_OBJECT_TYPE_INFO(i430FX);
DEFINE_OBJECT_GENERIC_FACTORY(i430FX);
BEGIN_OBJECT_PROPERTY_MAP(i430FX)
PROPERTY_TABLE_MEMBER_UINT("RAMSize", 0, offsetof(i430FX, m_ram_size), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("BIOSPath", 0, offsetof(i430FX, m_bios_file_path), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

i430FX::i430FX(CPU_X86::Model model /* = CPU_X86::MODEL_PENTIUM */, float cpu_frequency /* = 75000000.0f */,
               uint32 memory_size /* = 16 * 1024 * 1024 */, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(PCIPC::PCIConfigSpaceAccessType::Type1, type_info), m_bios_file_path("romimages/5ifw001.bin"),
    m_ram_size(memory_size)
{
  m_bus = new PCIBus(PHYSICAL_MEMORY_BITS);
  m_cpu = CreateComponent<CPU_X86::CPU>("CPU", model, cpu_frequency);
  AddComponents();
}

i430FX::~i430FX() = default;

bool i430FX::Initialize()
{
  if (m_ram_size < 1 * 1024 * 1024)
  {
    Log_ErrorPrintf("Invalid RAM size: %u bytes", m_ram_size);
    return false;
  }

  AllocatePhysicalMemory(m_ram_size, false, false);

  if (!BaseClass::Initialize())
    return false;

  if (!m_bus->CreateROMRegionFromFile(m_bios_file_path, BIOS_ROM_ADDRESS, BIOS_ROM_SIZE))
    return false;

  m_bus->MirrorRegion(BIOS_ROM_ADDRESS + BIOS_ROM_MIRROR_START, BIOS_ROM_MIRROR_SIZE, BIOS_ROM_MIRROR_ADDRESS);

  ConnectSystemIOPorts();
  SetCMOSVariables();
  return true;
}

void i430FX::Reset()
{
  BaseClass::Reset();

  m_cmos_lock = false;

  // Set keyboard controller input port up.
  // b7 = Keyboard not inhibited, b5 = POST loop inactive
  m_keyboard_controller->SetInputPort(0xA0);

  // Start with A20 line on
  SetA20State(true);
  UpdateKeyboardControllerOutputPort();
}

bool i430FX::LoadSystemState(BinaryReader& reader)
{
  if (!BaseClass::LoadSystemState(reader))
    return false;

  reader.SafeReadBool(&m_cmos_lock);
  return !reader.GetErrorState();
}

bool i430FX::SaveSystemState(BinaryWriter& writer)
{
  if (!BaseClass::SaveSystemState(writer))
    return false;

  writer.SafeWriteBool(m_cmos_lock);
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
  Log_TracePrintf("Write system control port A: 0x%02X", ZeroExtend32(value));

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
  // http://qlibdos32.sourceforge.net/tutor/tutor-port61h.php
  // http://www.ee.hacettepe.edu.tr/~alkar/ELE336/w9-hacettepe[2016].pdf
  // Port 61h toggles every 15.085us.
  const SimulationTime num_refresh_cycles = m_timing_manager.GetTotalEmulatedTime() / 15085;
  const u8 refresh_bit = Truncate8(num_refresh_cycles & 1);

  *value = (BoolToUInt8(m_timer->GetChannelGateInput(2)) << 0) |  // Timer 2 gate input
           (BoolToUInt8(m_speaker->IsOutputEnabled()) << 1) |     // Speaker data status
           (BoolToUInt8(refresh_bit) << 4) |                      // Triggers with each memory refresh
           (BoolToUInt8(m_timer->GetChannelOutputState(2)) << 5); // Raw timer 2 output
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
  m_sb82437 = CreatePCIDevice<HW::i82437FX>(0, 0, "Southbridge");

  m_interrupt_controller = CreateComponent<HW::i8259_PIC>("InterruptController");
  m_dma_controller = CreateComponent<HW::i8237_DMA>("DMAController");
  m_timer = CreateComponent<HW::i8253_PIT>("PIT");
  m_keyboard_controller = CreateComponent<HW::i8042_PS2>("KeyboardController");
  m_cmos = CreateComponent<HW::CMOS>("CMOS");
  m_speaker = CreateComponent<HW::PCSpeaker>("Speaker");

  m_fdd_controller = CreateComponent<HW::FDC>("FDC", HW::FDC::Model_82077);
  m_hdd_controller = CreateComponent<HW::PCIIDE>("IDE Controller", HW::PCIIDE::Model::PIIX);

  // Connect channel 0 of the PIT to the interrupt controller
  m_timer->SetChannelOutputChangeCallback(0,
                                          [this](bool value) { m_interrupt_controller->SetInterruptState(0, value); });

  // Connect channel 2 of the PIT to the speaker
  m_timer->SetChannelOutputChangeCallback(2, [this](bool value) { m_speaker->SetLevel(value); });
}

void i430FX::SetCMOSVariables() {}

} // namespace Systems