#include "pce/systems/i430fx.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu.h"
Log_SetChannel(Systems::i430FX);

namespace Systems {

i430FX::i430FX(HostInterface* host_interface, CPU_X86::Model model /* = CPU_X86::MODEL_PENTIUM */,
               float cpu_frequency /* = 2000000.0f */, uint32 memory_size /* = 16 * 1024 * 1024 */)
  : PCIPC(host_interface, PCIPC::PCIConfigSpaceAccessType::Type1), m_bios_file_path("romimages/5ifw001.bin")
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
  if (!m_bus->CreateMMIOROMRegionFromFile(m_bios_file_path.c_str(), BIOS_ROM_ADDRESS, BIOS_ROM_SIZE) ||
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
  ISAPC::Reset();

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
  m_sb82437fx = new SB82437FX(this, m_bus);
  AddPCIDeviceToLocation(m_sb82437fx, 0, 0);

  AddComponent(m_interrupt_controller = new HW::i8259_PIC());
  AddComponent(m_dma_controller = new HW::i8237_DMA());
  AddComponent(m_timer = new HW::i8253_PIT());
  AddComponent(m_keyboard_controller = new HW::i8042_PS2());
  AddComponent(m_cmos = new HW::CMOS());

  AddComponent(m_fdd_controller = new HW::FDC(m_dma_controller));
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

i430FX::SB82437FX::SB82437FX(i430FX* system, Bus* bus) : PCIDevice(0x8086, 0x122D), m_system(system), m_bus(bus) {}

i430FX::SB82437FX::~SB82437FX() {}

bool i430FX::SB82437FX::InitializePCIDevice(uint32 pci_bus_number, uint32 pci_device_number)
{
  if (!PCIDevice::InitializePCIDevice(pci_bus_number, pci_device_number))
    return false;

  return true;
}

void i430FX::SB82437FX::Reset()
{
  PCIDevice::Reset();

  // Default values from pcem.
  m_config_space[0].bytes[0x04] = 0x06;
  m_config_space[0].bytes[0x05] = 0x00;
  m_config_space[0].bytes[0x06] = 0x00;
  m_config_space[0].bytes[0x07] = 0x82;
  m_config_space[0].bytes[0x08] = 0x00;
  m_config_space[0].bytes[0x09] = 0x00;
  m_config_space[0].bytes[0x0A] = 0x00;
  m_config_space[0].bytes[0x0B] = 0x06;
  m_config_space[0].bytes[0x52] = 0x40;
  // m_config_space[0].bytes[0x53] = 0x14;
  // m_config_space[0].bytes[0x56] = 0x52;
  m_config_space[0].bytes[0x57] = 0x01;
  m_config_space[0].bytes[0x60] = 0x02;
  m_config_space[0].bytes[0x61] = 0x02;
  m_config_space[0].bytes[0x62] = 0x02;
  m_config_space[0].bytes[0x63] = 0x02;
  m_config_space[0].bytes[0x64] = 0x02;
  // m_config_space[0].bytes[0x67] = 0x11;
  // m_config_space[0].bytes[0x69] = 0x03;
  // m_config_space[0].bytes[0x70] = 0x20;
  m_config_space[0].bytes[0x72] = 0x02;
  // m_config_space[0].bytes[0x74] = 0x0E;
  // m_config_space[0].bytes[0x78] = 0x23;

  for (uint8 i = 0; i < NUM_PAM_REGISTERS; i++)
    UpdatePAMMapping(PAM_BASE_OFFSET + i);
}

bool i430FX::SB82437FX::LoadState(BinaryReader& reader)
{
  if (!PCIDevice::LoadState(reader))
    return false;

  for (uint8 i = 0; i < NUM_PAM_REGISTERS; i++)
    UpdatePAMMapping(PAM_BASE_OFFSET + i);

  return true;
}

bool i430FX::SB82437FX::SaveState(BinaryWriter& writer)
{
  return PCIDevice::SaveState(writer);
}

uint8 i430FX::SB82437FX::HandleReadConfigRegister(uint32 function, uint8 offset)
{
  return PCIDevice::HandleReadConfigRegister(function, offset);
}

void i430FX::SB82437FX::HandleWriteConfigRegister(uint32 function, uint8 offset, uint8 value)
{
  if (offset >= 0x10 && offset < 0x4F)
    return;

  Log_DevPrintf("SB82437FX: 0x%08X 0x%02X", offset, value);

  switch (offset)
  {
    case 0x59: // PAM0
    case 0x5A: // PAM1
    case 0x5B: // PAM2
    case 0x5C: // PAM3
    case 0x5D: // PAM4
    case 0x5E: // PAM5
    case 0x5F: // PAM6
    {
      if (m_config_space[0].bytes[offset] != value)
      {
        m_config_space[0].bytes[offset] = value;
        UpdatePAMMapping(offset);
      }
    }
    break;

    default:
      PCIDevice::HandleWriteConfigRegister(function, offset, value);
      break;
  }
}

void i430FX::SB82437FX::SetPAMMapping(uint32 base, uint32 size, uint8 flag)
{
  const bool readable_memory = !!(flag & 1);
  const bool writable_memory = !!(flag & 2);

  Log_DevPrintf("Shadowing for 0x%08X-0x%08X - type %u, readable=%s, writable=%s", base, base + size - 1, flag,
                readable_memory ? "yes" : "no", writable_memory ? "yes" : "no");

  m_bus->SetPagesMemoryState(base, size, readable_memory, writable_memory);
}

void i430FX::SB82437FX::UpdatePAMMapping(uint8 offset)
{
  const uint8 value = m_config_space[0].bytes[offset];

  switch (offset)
  {
    case 0x59:
      SetPAMMapping(0xF0000, 0x10000, value >> 4);
      break;
    case 0x5A:
      SetPAMMapping(0xC0000, 0x04000, value & 0x0F);
      SetPAMMapping(0xC4000, 0x04000, value >> 4);
      break;
    case 0x5B:
      SetPAMMapping(0xC8000, 0x04000, value & 0x0F);
      SetPAMMapping(0xCC000, 0x04000, value >> 4);
      break;
    case 0x5C:
      SetPAMMapping(0xD0000, 0x04000, value & 0x0F);
      SetPAMMapping(0xD4000, 0x04000, value >> 4);
      break;
    case 0x5D:
      SetPAMMapping(0xD8000, 0x04000, value & 0x0F);
      SetPAMMapping(0xDC000, 0x04000, value >> 4);
      break;
    case 0x5E:
      SetPAMMapping(0xE0000, 0x04000, value & 0x0F);
      SetPAMMapping(0xE4000, 0x04000, value >> 4);
      break;
    case 0x5F:
      SetPAMMapping(0xE8000, 0x04000, value & 0x0F);
      SetPAMMapping(0xEC000, 0x04000, value >> 4);
      break;
  }
}

} // namespace Systems