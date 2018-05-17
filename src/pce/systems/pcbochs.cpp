#include "pce/systems/pcbochs.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu.h"
Log_SetChannel(Systems::PCBochs);

namespace Systems {

PCBochs::PCBochs(HostInterface* host_interface, CPU_X86::Model model /* = CPU_X86::MODEL_486 */,
                 float cpu_frequency /* = 8000000.0f */, uint32 memory_size /* = 16 * 1024 * 1024 */)
  : PCBase(host_interface)
{
  m_cpu = new CPU_X86::CPU(model, cpu_frequency);
  m_bus = new Bus(PHYSICAL_MEMORY_BITS);
  AllocatePhysicalMemory(memory_size, false);
  AddComponents();
}

PCBochs::~PCBochs() {}

void PCBochs::Initialize()
{
  PCBase::Initialize();

  ConnectSystemIOPorts();
  SetCMOSVariables();
}

void PCBochs::Reset()
{
  PCBase::Reset();

  m_refresh_bit = false;

  // Default gate A20 to on
  SetA20State(true);
  UpdateKeyboardControllerOutputPort();
}

bool PCBochs::LoadSystemState(BinaryReader& reader)
{
  if (!PCBase::LoadSystemState(reader))
    return false;

  reader.SafeReadBool(&m_refresh_bit);
  return !reader.GetErrorState();
}

bool PCBochs::SaveSystemState(BinaryWriter& writer)
{
  if (!PCBase::SaveSystemState(writer))
    return false;

  writer.SafeWriteBool(m_refresh_bit);
  return !writer.InErrorState();
}

void PCBochs::ConnectSystemIOPorts()
{
  // System control ports
  m_bus->ConnectIOPortRead(0x0092, this, std::bind(&PCBochs::IOReadSystemControlPortA, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0092, this, std::bind(&PCBochs::IOWriteSystemControlPortA, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x0061, this, std::bind(&PCBochs::IOReadSystemControlPortB, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0061, this, std::bind(&PCBochs::IOWriteSystemControlPortB, this, std::placeholders::_2));

  // Connect the keyboard controller output port to the lower 2 bits of system control port A.
  m_keyboard_controller->SetOutputPortWrittenCallback([this](uint8 value) {
    // We're doing something wrong here, the BIOS resets the CPU almost immediately after booting?
    value &= ~uint8(0x01);
    IOWriteSystemControlPortA(value & 0x03);
    IOReadSystemControlPortA(&value);
    m_keyboard_controller->SetOutputPort(value);
  });

  // PCI stuff
  m_bus->ConnectIOPortReadDWord(0x0CF8, this, [](uint32 port, uint32* value) { *value = 0xFFFFFFFF; });
  m_bus->ConnectIOPortReadDWord(0x0CFA, this, [](uint32 port, uint32* value) { *value = 0xFFFFFFFF; });
  m_bus->ConnectIOPortReadWord(0x0CFC, this, [](uint32 port, uint16* value) { *value = 0xFFFF; });
  m_bus->ConnectIOPortReadWord(0x0CFD, this, [](uint32 port, uint16* value) { *value = 0xFFFF; });
  m_bus->ConnectIOPortWriteDWord(0x0CF8, this, [](uint32 port, uint32 value) { });
  m_bus->ConnectIOPortWriteDWord(0x0CFA, this, [](uint32 port, uint32 value) {});
  m_bus->ConnectIOPortWriteWord(0x0CFC, this, [](uint32 port, uint16 value) {});
  m_bus->ConnectIOPortWriteWord(0x0CFD, this, [](uint32 port, uint16 value) {});

  String* blah = new String();
  m_bus->ConnectIOPortWrite(0x0500, this, [blah](uint32 port, uint8 value) {
    // Log_DevPrintf("Debug port: %u: %c", uint32(value), value);
    if (value == '\n')
    {
      Log_DevPrintf("Debug message 0500: %s", blah->GetCharArray());
      blah->Clear();
    }
    else
    {
      blah->AppendCharacter(char(value));
    }
  });

  String* blah2 = new String();
  m_bus->ConnectIOPortWrite(0x0402, this, [blah2](uint32 port, uint8 value) {
    // Log_DevPrintf("Debug port: %u: %c", uint32(value), value);
    if (value == '\n')
    {
      Log_DevPrintf("Debug message 0402: %s", blah2->GetCharArray());
      blah2->Clear();
    }
    else
    {
      blah2->AppendCharacter(char(value));
    }
  });
  String* blah3 = new String();
  m_bus->ConnectIOPortWrite(0x0403, this, [blah3](uint32 port, uint8 value) {
    // Log_DevPrintf("Debug port: %u: %c", uint32(value), value);
    if (value == '\n')
    {
      Log_DevPrintf("Debug message 0403: %s", blah3->GetCharArray());
      blah3->Clear();
    }
    else
    {
      blah3->AppendCharacter(char(value));
    }
  });
}

void PCBochs::IOReadSystemControlPortA(uint8* value)
{
  *value = (BoolToUInt8(GetA20State()) << 1);
}

void PCBochs::IOWriteSystemControlPortA(uint8 value)
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

void PCBochs::IOReadSystemControlPortB(uint8* value)
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

void PCBochs::IOWriteSystemControlPortB(uint8 value)
{
  Log_DevPrintf("Write system control port A: 0x%02X", ZeroExtend32(value));

  m_timer->SetChannelGateInput(2, !!(value & (1 << 0))); // Timer 2 gate input
  m_speaker->SetOutputEnabled(!!(value & (1 << 1)));     // Speaker data enable
}

void PCBochs::UpdateKeyboardControllerOutputPort()
{
  uint8 value = m_keyboard_controller->GetOutputPort();
  value &= ~uint8(0x03);
  value |= (BoolToUInt8(GetA20State()) << 1);
  m_keyboard_controller->SetOutputPort(value);
}

void PCBochs::AddComponents()
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

  // Connect channel 0 of the PIT to the interrupt controller
  m_timer->SetChannelOutputChangeCallback(0, [this](bool value) {
    if (value)
      m_interrupt_controller->TriggerInterrupt(0);
  });

  m_speaker = new HW::PCSpeaker();
  AddComponent(m_speaker);

  // Connect channel 2 of the PIT to the speaker
  m_timer->SetChannelOutputChangeCallback(2, [this](bool value) { m_speaker->SetLevel(value); });
}

void PCBochs::SetCMOSVariables()
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

  uint32 fdd_drive_type_variable = 0;
  for (uint32 i = 0; i < HW::FDC::MAX_DRIVES; i++)
  {
    HW::FDC::DriveType drive_type = m_fdd_controller->GetDriveType_(i);
    HW::FDC::DiskType disk_type = m_fdd_controller->GetDiskType(i);
    if (drive_type == HW::FDC::DriveType_None)
      continue;

    // 1 - 360K 5.25", 2 - 1.2MB 5.25", 3 - 720K 3.5", 4 - 1.44MB 3.5", 5 - 2.88M 3.5"
    uint32 cmos_type;
    switch (drive_type)
    {
      case HW::FDC::DriveType_5_25:
      {
        switch (disk_type)
        {
          case HW::FDC::DiskType_160K:
          case HW::FDC::DiskType_180K:
          case HW::FDC::DiskType_320K:
          case HW::FDC::DiskType_360K:
          case HW::FDC::DiskType_640K:
          case HW::FDC::DiskType_1220K:
          default:
            cmos_type = 2;
            break;
        }
      }
      break;

      case HW::FDC::DriveType_3_5:
      {
        switch (disk_type)
        {
          case HW::FDC::DiskType_2880K:
            cmos_type = 5;
            break;
          case HW::FDC::DiskType_720K:
          case HW::FDC::DiskType_1440K:
          default:
            cmos_type = 4;
            break;
        }
      }
      break;

      default:
        cmos_type = 0;
        break;
    }

    if (i == 0)
      fdd_drive_type_variable |= ((cmos_type & 0xF) << 4);
    else if (i == 1)
      fdd_drive_type_variable |= ((cmos_type & 0xF) << 0);
  }
  m_cmos->SetVariable(0x10, Truncate8(fdd_drive_type_variable));

  // Equipment byte
  uint8 equipment_byte = 0;
  // equipment_byte |= (1 << 1);     // coprocessor installed
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
  if (m_hdd_controller->GetDriveCount() > 0)
    m_cmos->SetVariable(0x3D, 0x02);
  else
    m_cmos->SetVariable(0x3D, 0x01);

  // HDD information
  m_cmos->SetVariable(0x12, 0);
  if (m_hdd_controller->IsDrivePresent(0))
  {
    m_cmos->SetVariable(0x12, m_cmos->GetVariable(0x12) | 0xF0);
    m_cmos->SetVariable(0x19, 47); // user-defined type
    m_cmos->SetVariable(0x1B, Truncate8(m_hdd_controller->GetDriveCylinders(0)));
    m_cmos->SetVariable(0x1C, Truncate8(m_hdd_controller->GetDriveCylinders(0) >> 8));
    m_cmos->SetVariable(0x1D, Truncate8(m_hdd_controller->GetDriveHeads(0)));
    m_cmos->SetVariable(0x1E, 0xFF);
    m_cmos->SetVariable(0x1F, 0xFF);
    m_cmos->SetVariable(0x20, 0xC0 | ((m_hdd_controller->GetDriveHeads(0) > 8) ? 8 : 0));
    m_cmos->SetVariable(0x21, m_cmos->GetVariable(0x1B));
    m_cmos->SetVariable(0x22, m_cmos->GetVariable(0x1C));
    m_cmos->SetVariable(0x23, Truncate8(m_hdd_controller->GetDriveSectors(0)));
  }
  if (m_hdd_controller->IsDrivePresent(1))
  {
    m_cmos->SetVariable(0x12, m_cmos->GetVariable(0x12) | 0x0F);
    m_cmos->SetVariable(0x1A, 47); // user-defined type
    m_cmos->SetVariable(0x24, Truncate8(m_hdd_controller->GetDriveCylinders(1)));
    m_cmos->SetVariable(0x25, Truncate8(m_hdd_controller->GetDriveCylinders(1) >> 8));
    m_cmos->SetVariable(0x26, Truncate8(m_hdd_controller->GetDriveHeads(1)));
    m_cmos->SetVariable(0x27, 0xFF);
    m_cmos->SetVariable(0x28, 0xFF);
    m_cmos->SetVariable(0x29, 0xC0 | ((m_hdd_controller->GetDriveHeads(1) > 8) ? 8 : 0));
    m_cmos->SetVariable(0x2A, m_cmos->GetVariable(0x1B));
    m_cmos->SetVariable(0x2B, m_cmos->GetVariable(0x1C));
    m_cmos->SetVariable(0x2C, Truncate8(m_hdd_controller->GetDriveSectors(1)));
  }
}

} // namespace Systems