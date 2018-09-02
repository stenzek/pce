#include "pce/systems/bochs.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "pce/hw/pci_bus.h"
#include <memory>
Log_SetChannel(Systems::Bochs);

namespace Systems {
DEFINE_OBJECT_TYPE_INFO(Bochs);
DEFINE_OBJECT_GENERIC_FACTORY(Bochs);
BEGIN_OBJECT_PROPERTY_MAP(Bochs)
PROPERTY_TABLE_MEMBER_UINT("RAMSize", 0, offsetof(Bochs, m_ram_size), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

Bochs::Bochs(CPU_X86::Model model /* = CPU_X86::MODEL_PENTIUM */, float cpu_frequency /* = 8000000.0f */,
             uint32 memory_size /* = 16 * 1024 * 1024 */, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(PCIPC::PCIConfigSpaceAccessType::Type1, type_info), m_bios_file_path("romimages/BIOS-bochs-latest"),
    m_ram_size(memory_size)
{
  m_bus = new PCIBus(PHYSICAL_MEMORY_BITS);
  m_cpu = CreateComponent<CPU_X86::CPU>("CPU", model, cpu_frequency);
  AddComponents();
}

Bochs::~Bochs() = default;

bool Bochs::Initialize()
{
  if (m_ram_size < 1 * 1024 * 1024)
  {
    Log_ErrorPrintf("Invalid RAM size: %u bytes", m_ram_size);
    return false;
  }

  AllocatePhysicalMemory(m_ram_size, false, false);

  if (!BaseClass::Initialize())
    return false;

  if (!m_bus->CreateROMRegionFromFile(m_bios_file_path.c_str(), BIOS_ROM_ADDRESS, BIOS_ROM_SIZE))
    return false;

  m_bus->MirrorRegion(BIOS_ROM_ADDRESS + BIOS_ROM_MIRROR_START, BIOS_ROM_MIRROR_SIZE, BIOS_ROM_MIRROR_ADDRESS);

  ConnectSystemIOPorts();
  return true;
}

void Bochs::Reset()
{
  BaseClass::Reset();

  // Hack: Set the CMOS variables on reset, so things like floppies are picked up.
  // We should probably set this last, or have a PostInitialize function or something.
  SetCMOSVariables();

  m_refresh_bit = false;

  // Default gate A20 to on
  SetA20State(true);
  UpdateKeyboardControllerOutputPort();
}

bool Bochs::LoadSystemState(BinaryReader& reader)
{
  if (!BaseClass::LoadSystemState(reader))
    return false;

  reader.SafeReadBool(&m_refresh_bit);
  return !reader.GetErrorState();
}

bool Bochs::SaveSystemState(BinaryWriter& writer)
{
  if (!BaseClass::SaveSystemState(writer))
    return false;

  writer.SafeWriteBool(m_refresh_bit);
  return !writer.InErrorState();
}

void Bochs::ConnectSystemIOPorts()
{
  // System control ports
  m_bus->ConnectIOPortRead(0x0092, this, std::bind(&Bochs::IOReadSystemControlPortA, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0092, this, std::bind(&Bochs::IOWriteSystemControlPortA, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x0061, this, std::bind(&Bochs::IOReadSystemControlPortB, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0061, this, std::bind(&Bochs::IOWriteSystemControlPortB, this, std::placeholders::_2));

  // Connect the keyboard controller output port to the lower 2 bits of system control port A.
  m_keyboard_controller->SetOutputPortWrittenCallback([this](uint8 value, uint8 old_value, bool pulse) {
    // We're doing something wrong here, the BIOS resets the CPU almost immediately after booting?
    value &= ~uint8(0x01);
    IOWriteSystemControlPortA(value & 0x03);
    IOReadSystemControlPortA(&value);
    m_keyboard_controller->SetOutputPort(value);
  });

  // Debug ports.
  static const char* debug_port_names[5] = {"panic", "panic2", "info", "debug", "unknown"};
  static const uint32 debug_port_numbers[5] = {0x0400, 0x0401, 0x0402, 0x0403, 0x0500};
  for (uint32 i = 0; i < countof(debug_port_names); i++)
  {
    const char* port_name = debug_port_names[i];
    std::shared_ptr<std::string> str = std::make_shared<std::string>();
    m_bus->ConnectIOPortWrite(debug_port_numbers[i], this, [port_name, str](uint32 port, uint8 value) {
      if (value == '\n')
      {
        Log_DevPrintf("%s port message (%04X): %s", port_name, port, str->c_str());
        str->clear();
      }
      else
      {
        *str += static_cast<char>(value);
      }
    });
  }
}

void Bochs::IOReadSystemControlPortA(uint8* value)
{
  *value = (BoolToUInt8(GetA20State()) << 1);
}

void Bochs::IOWriteSystemControlPortA(uint8 value)
{
  Log_DevPrintf("Write system control port A: 0x%02X", ZeroExtend32(value));

  // b7-6 - Activity Lights
  // b5 - Reserved
  // b4 - Watchdog Timeout
  // b3 - CMOS Security Lock
  // b2 - Reserved
  // b1 - A20 Active
  // b0 - System Reset

  bool new_a20_state = !!(value & (1 << 1));
  bool system_reset = !!(value & (1 << 0));

  // Update A20 state
  if (GetA20State() != new_a20_state)
  {
    SetA20State(new_a20_state);
    UpdateKeyboardControllerOutputPort();
  }

  // System reset?
  // We do this last as it's going to destroy everything.
  // TODO: We should probably put it on an event though..
  if (system_reset)
  {
    Log_WarningPrintf("CPU reset via system control port");
    m_cpu->Reset();
  }
}

void Bochs::IOReadSystemControlPortB(uint8* value)
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

void Bochs::IOWriteSystemControlPortB(uint8 value)
{
  Log_DevPrintf("Write system control port B: 0x%02X", ZeroExtend32(value));

  m_timer->SetChannelGateInput(2, !!(value & (1 << 0))); // Timer 2 gate input
  m_speaker->SetOutputEnabled(!!(value & (1 << 1)));     // Speaker data enable
}

void Bochs::UpdateKeyboardControllerOutputPort()
{
  uint8 value = m_keyboard_controller->GetOutputPort();
  value &= ~uint8(0x03);
  value |= (BoolToUInt8(GetA20State()) << 1);
  m_keyboard_controller->SetOutputPort(value);
}

void Bochs::AddComponents()
{
  m_sb82437 = CreatePCIDevice<HW::i82437FX>(0, 0, "Southbridge");

  m_interrupt_controller = CreateComponent<HW::i8259_PIC>("InterruptController");
  m_dma_controller = CreateComponent<HW::i8237_DMA>("DMAController");
  m_timer = CreateComponent<HW::i8253_PIT>("PIT");
  m_keyboard_controller = CreateComponent<HW::i8042_PS2>("KeyboardController");
  m_cmos = CreateComponent<HW::CMOS>("CMOS");
  m_speaker = CreateComponent<HW::PCSpeaker>("Speaker");

  m_fdd_controller = CreateComponent<HW::FDC>("FDC", HW::FDC::Model_82077);
  m_primary_hdd_controller = CreateComponent<HW::HDC>("PrimaryHDC", HW::HDC::CHANNEL_PRIMARY);
  m_secondary_hdd_controller = CreateComponent<HW::HDC>("SecondaryHDC", HW::HDC::CHANNEL_SECONDARY);

  // Connect channel 0 of the PIT to the interrupt controller
  m_timer->SetChannelOutputChangeCallback(0,
                                          [this](bool value) { m_interrupt_controller->SetInterruptState(0, value); });

  // Connect channel 2 of the PIT to the speaker
  m_timer->SetChannelOutputChangeCallback(2, [this](bool value) { m_speaker->SetLevel(value); });
}

void Bochs::SetCMOSVariables()
{
  // Bochs CMOS map
  //
  // Idx  Len   Description
  // 0x10   1   floppy drive types
  // 0x11   1   configuration bits
  // 0x12   1   harddisk types
  // 0x13   1   advanced configuration bits
  // 0x15   2   base memory in 1k
  // 0x17   2   memory size above 1M in 1k
  // 0x19   2   extended harddisk types
  // 0x1b   9   harddisk configuration (hd0)
  // 0x24   9   harddisk configuration (hd1)
  // 0x2d   1   boot sequence (fd/hd)
  // 0x30   2   memory size above 1M in 1k
  // 0x34   2   memory size above 16M in 64k
  // 0x38   1   eltorito boot sequence (#3) + bootsig check
  // 0x39   2   ata translation policy (ata0...ata3)
  // 0x3d   1   eltorito boot sequence (#1 + #2)
  //
  // Qemu CMOS map
  //
  // Idx  Len   Description
  // 0x5b   3   extra memory above 4GB
  // 0x5f   1   number of processors

  PhysicalMemoryAddress base_memory_in_k = GetBaseMemorySize() / 1024;
  m_cmos->SetWordVariable(0x15, Truncate16(base_memory_in_k));
  Log_DevPrintf("Base memory in KB: %u", Truncate32(base_memory_in_k));

  PhysicalMemoryAddress extended_memory_in_k = GetTotalMemorySize() / 1024 - 1024;
  if (extended_memory_in_k > 0xFC00)
    extended_memory_in_k = 0xFC00;
  m_cmos->SetWordVariable(0x17, Truncate16(extended_memory_in_k));
  m_cmos->SetWordVariable(0x30, Truncate16(extended_memory_in_k));
  Log_DevPrintf("Extended memory in KB: %u", Truncate32(extended_memory_in_k));

  PhysicalMemoryAddress extended_memory_in_64k = GetTotalMemorySize() / 1024; // TODO: Fix this
  if (extended_memory_in_64k > 16384)
    extended_memory_in_64k = (extended_memory_in_64k - 16384) / 64;
  else
    extended_memory_in_64k = 0;
  if (extended_memory_in_64k > 0xBF00)
    extended_memory_in_64k = 0xBF00;

  m_cmos->SetWordVariable(0x34, Truncate16(extended_memory_in_64k));
  Log_DevPrintf("Extended memory above 16MB in KB: %u", Truncate32(extended_memory_in_64k * 64));

  m_cmos->SetFloppyCount(m_fdd_controller->GetDriveCount());
  for (uint32 i = 0; i < HW::FDC::MAX_DRIVES; i++)
  {
    if (m_fdd_controller->IsDrivePresent(i))
      m_cmos->SetFloppyType(i, m_fdd_controller->GetDriveType_(i));
  }

  // Equipment byte
  uint8 equipment_byte = 0;
  equipment_byte |= (1 << 1); // coprocessor installed
  equipment_byte |= (1 << 2); // ps/2 device/mouse installed
  if (m_fdd_controller->GetDriveCount() > 0)
  {
    equipment_byte |= (1 << 0); // disk available for boot
    if (m_fdd_controller->GetDriveCount() > 1)
      equipment_byte |= (0b01 << 7); // 2 drives installed
  }
  m_cmos->SetVariable(0x14, equipment_byte);

  // Boot from HDD first
  // Legacy - 0 - C: -> A:, 1 - A: -> C:
  m_cmos->SetVariable(0x2D, (0 << 5));
  // 0x00 - undefined, 0x01 - first floppy, 0x02 - first HDD, 0x03 - first cdrom
  if (m_primary_hdd_controller->GetDriveCount() > 0)
    m_cmos->SetVariable(0x3D, 0x02);
  else
    m_cmos->SetVariable(0x3D, 0x01);

  // Skip IPL validity check
  m_cmos->SetVariable(0x38, 0x01);

  // HDD information
  m_cmos->SetVariable(0x12, 0);
  if (m_primary_hdd_controller->IsDrivePresent(0))
  {
    m_cmos->SetVariable(0x12, m_cmos->GetVariable(0x12) | 0xF0);
    m_cmos->SetVariable(0x19, 47); // user-defined type
    m_cmos->SetVariable(0x1B, Truncate8(m_primary_hdd_controller->GetDriveCylinders(0)));
    m_cmos->SetVariable(0x1C, Truncate8(m_primary_hdd_controller->GetDriveCylinders(0) >> 8));
    m_cmos->SetVariable(0x1D, Truncate8(m_primary_hdd_controller->GetDriveHeads(0)));
    m_cmos->SetVariable(0x1E, 0xFF);
    m_cmos->SetVariable(0x1F, 0xFF);
    m_cmos->SetVariable(0x20, 0xC0 | ((m_primary_hdd_controller->GetDriveHeads(0) > 8) ? 8 : 0));
    m_cmos->SetVariable(0x21, m_cmos->GetVariable(0x1B));
    m_cmos->SetVariable(0x22, m_cmos->GetVariable(0x1C));
    m_cmos->SetVariable(0x23, Truncate8(m_primary_hdd_controller->GetDriveSectors(0)));
  }
  if (m_primary_hdd_controller->IsDrivePresent(1))
  {
    m_cmos->SetVariable(0x12, m_cmos->GetVariable(0x12) | 0x0F);
    m_cmos->SetVariable(0x1A, 47); // user-defined type
    m_cmos->SetVariable(0x24, Truncate8(m_primary_hdd_controller->GetDriveCylinders(1)));
    m_cmos->SetVariable(0x25, Truncate8(m_primary_hdd_controller->GetDriveCylinders(1) >> 8));
    m_cmos->SetVariable(0x26, Truncate8(m_primary_hdd_controller->GetDriveHeads(1)));
    m_cmos->SetVariable(0x27, 0xFF);
    m_cmos->SetVariable(0x28, 0xFF);
    m_cmos->SetVariable(0x29, 0xC0 | ((m_primary_hdd_controller->GetDriveHeads(1) > 8) ? 8 : 0));
    m_cmos->SetVariable(0x2A, m_cmos->GetVariable(0x1B));
    m_cmos->SetVariable(0x2B, m_cmos->GetVariable(0x1C));
    m_cmos->SetVariable(0x2C, Truncate8(m_primary_hdd_controller->GetDriveSectors(1)));
  }
}

} // namespace Systems