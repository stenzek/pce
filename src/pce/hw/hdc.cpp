#include "pce/hw/hdc.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "common/hdd_image.h"
#include "pce/bus.h"
#include "pce/hw/cdrom.h"
#include "pce/hw/ide_hdd.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cstring>
Log_SetChannel(HW::HDC);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(HDC);
BEGIN_OBJECT_PROPERTY_MAP(HDC)
END_OBJECT_PROPERTY_MAP()

HDC::HDC(const String& identifier, CHANNEL channel /* = CHANNEL_PRIMARY */,
         const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_channel(channel)
{
}

HDC::~HDC() = default;

bool HDC::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_interrupt_controller = m_system->GetComponentByType<InterruptController>();
  if (!m_interrupt_controller)
  {
    Log_ErrorPrintf("Failed to locate interrupt controller.");
    return false;
  }

  ConnectIOPorts(bus);

  // Flush the HDD images once a second, to ensure data isn't lost.
  m_image_flush_event = m_system->GetTimingManager()->CreateFrequencyEvent("HDD Image Flush", 1.0f,
                                                                           std::bind(&HDC::FlushImagesEvent, this));
  m_command_event = m_system->GetTimingManager()->CreateMicrosecondIntervalEvent(
    "HDC Command", 1, std::bind(&HDC::ExecutePendingCommand, this), false);
  return true;
}

void HDC::Reset()
{
  SoftReset();
}

bool HDC::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  uint32 channel = reader.ReadUInt32();
  if (channel != static_cast<uint32>(m_channel))
    return false;

  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    auto& drive = m_drives[i];
    const bool present = reader.ReadBool();
    if (present != IsDrivePresent(i))
    {
      Log_ErrorPrintf("Save state mismatch for drive %u", i);
      return false;
    }
    if (!present)
      continue;

    const DRIVE_TYPE type = static_cast<DRIVE_TYPE>(reader.ReadUInt32());
    if (type != drive.type)
    {
      Log_ErrorPrintf("Save state mismatch for drive %u", i);
      return false;
    }

    drive.current_num_cylinders = reader.ReadUInt32();
    drive.current_num_heads = reader.ReadUInt32();
    drive.current_num_sectors = reader.ReadUInt32();
    drive.current_cylinder = reader.ReadUInt32();
    drive.current_head = reader.ReadUInt32();
    drive.current_sector = reader.ReadUInt32();
    drive.current_lba = reader.ReadUInt64();
    drive.ata_sector_count = reader.ReadUInt16();
    drive.ata_sector_number = reader.ReadUInt16();
    drive.ata_cylinder_low = reader.ReadUInt16();
    drive.ata_cylinder_high = reader.ReadUInt16();
    drive.multiple_sectors = reader.ReadUInt16();
  }

  m_status_register.bits = reader.ReadUInt8();
  m_error_register = reader.ReadUInt8();
  m_control_register.bits = reader.ReadUInt8();
  m_drive_select.bits = reader.ReadUInt8();
  m_feature_select = reader.ReadUInt8();
  m_pending_command = reader.ReadUInt8();

  size_t buffer_size = reader.ReadUInt32();
  if (buffer_size > 0)
  {
    m_current_transfer.buffer.resize(buffer_size);
    reader.ReadBytes(m_current_transfer.buffer.data(), Truncate32(buffer_size));
  }
  else
  {
    m_current_transfer.buffer.clear();
  }

  m_current_transfer.buffer_position = reader.ReadUInt32();
  m_current_transfer.drive_index = reader.ReadUInt32();
  m_current_transfer.sectors_per_block = reader.ReadUInt32();
  m_current_transfer.remaining_sectors = reader.ReadUInt32();
  m_current_transfer.is_write = reader.ReadBool();
  m_current_transfer.is_packet_command = reader.ReadBool();
  m_current_transfer.is_packet_data = reader.ReadBool();
  return true;
}

bool HDC::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteUInt32(static_cast<uint32>(m_channel));

  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    const bool present = IsDrivePresent(i);
    writer.WriteBool(present);
    if (!present)
      continue;

    const auto& drive = m_drives[i];
    writer.WriteUInt32(static_cast<uint32>(drive.type));
    writer.WriteUInt32(drive.current_num_cylinders);
    writer.WriteUInt32(drive.current_num_heads);
    writer.WriteUInt32(drive.current_num_sectors);
    writer.WriteUInt32(drive.current_cylinder);
    writer.WriteUInt32(drive.current_head);
    writer.WriteUInt32(drive.current_sector);
    writer.WriteUInt64(drive.current_lba);
    writer.WriteUInt16(drive.ata_sector_count);
    writer.WriteUInt16(drive.ata_sector_number);
    writer.WriteUInt16(drive.ata_cylinder_low);
    writer.WriteUInt16(drive.ata_cylinder_high);
    writer.WriteUInt16(drive.multiple_sectors);
  }

  writer.WriteUInt8(m_status_register.bits);
  writer.WriteUInt8(m_error_register);
  writer.WriteUInt8(m_control_register.bits);
  writer.WriteUInt8(m_drive_select.bits);
  writer.WriteUInt8(m_feature_select);
  writer.WriteUInt8(m_pending_command);

  writer.WriteUInt32(Truncate32(m_current_transfer.buffer.size()));
  if (!m_current_transfer.buffer.empty())
    writer.WriteBytes(m_current_transfer.buffer.data(), Truncate32(m_current_transfer.buffer.size()));

  writer.WriteUInt32(Truncate32(m_current_transfer.buffer_position));
  writer.WriteUInt32(m_current_transfer.drive_index);
  writer.WriteUInt32(m_current_transfer.sectors_per_block);
  writer.WriteUInt32(m_current_transfer.remaining_sectors);
  writer.WriteBool(m_current_transfer.is_write);
  writer.WriteBool(m_current_transfer.is_packet_command);
  writer.WriteBool(m_current_transfer.is_packet_data);
  return true;
}

bool HDC::IsDrivePresent(uint32 number) const
{
  return (number < MAX_DRIVES && m_drives[number].type != DRIVE_TYPE_NONE);
}

uint32 HDC::GetDriveCount() const
{
  u32 count = 0;
  for (u32 i = 0; i < MAX_DRIVES; i++)
  {
    if (m_drives[i].type != DRIVE_TYPE_NONE)
      count++;
  }
  return count;
}

void HDC::CalculateCHSForSize(uint32* cylinders, uint32* heads, uint32* sectors, uint64 disk_size)
{
  static const uint32 MAX_CYLINDERS = 1023;
  static const uint32 MAX_HEADS = 255;
  static const uint32 MAX_SECTORS = 63;
  Assert((disk_size % 512) == 0);

  // uint32 ncylinders = 1;
  // uint32 nsectors = 1;
  // uint32 nheads = 1;
  // uint64 current_size = 512;

  // TODO
}

uint32 HDC::GetDriveCylinders(uint32 number) const
{
  return (number < MAX_DRIVES && m_drives[number].hdd) ? m_drives[number].hdd->GetNumCylinders() : 0;
}

uint32 HDC::GetDriveHeads(uint32 number) const
{
  return (number < MAX_DRIVES && m_drives[number].hdd) ? m_drives[number].hdd->GetNumHeads() : 0;
}

uint32 HDC::GetDriveSectors(uint32 number) const
{
  return (number < MAX_DRIVES && m_drives[number].hdd) ? m_drives[number].hdd->GetNumSectors() : 0;
}

uint64 HDC::GetDriveLBAs(uint32 number) const
{
  return (number < MAX_DRIVES && m_drives[number].hdd) ? m_drives[number].hdd->GetNumLBAs() : 0;
}

bool HDC::AttachHDD(uint32 number, IDEHDD* dev)
{
  if (number >= MAX_DRIVES || m_drives[number].type != DRIVE_TYPE_NONE)
    return false;

  DriveState& drive_state = m_drives[number];
  drive_state.type = DRIVE_TYPE_HDD;
  drive_state.current_num_cylinders = dev->GetNumCylinders();
  drive_state.current_num_heads = dev->GetNumHeads();
  drive_state.current_num_sectors = dev->GetNumSectors();
  drive_state.hdd = dev;
  return true;
}

bool HDC::AttachATAPIDevice(uint32 number, CDROM* atapi_dev)
{
  if (number >= MAX_DRIVES || m_drives[number].type != DRIVE_TYPE_NONE)
    return false;

  DriveState& drive_state = m_drives[number];
  drive_state.type = DRIVE_TYPE_ATAPI;
  drive_state.atapi_dev = atapi_dev;

  atapi_dev->SetCommandCompletedCallback(std::bind(&HDC::HandleATAPICommandCompleted, this, number));
  return true;
}

void HDC::DetachDrive(uint32 number)
{
  if (number >= MAX_DRIVES || m_drives[number].type == DRIVE_TYPE_NONE)
    return;

  if (m_drives[number].type == DRIVE_TYPE_ATAPI)
    m_drives[number].atapi_dev->SetCommandCompletedCallback({});

  m_drives[number] = {};
  m_drives[number].type = DRIVE_TYPE_NONE;
}

bool HDC::SeekDrive(uint32 drive, uint64 lba)
{
  DebugAssert(drive < MAX_DRIVES && m_drives[drive].hdd);

  DriveState& state = m_drives[drive];
  if (lba >= state.hdd->GetNumLBAs())
    return false;

  // TODO: Leave CHS unupdated for now?
  state.current_cylinder = 0;
  state.current_head = 0;
  state.current_sector = 0;
  state.current_lba = lba;
  m_status_register.bits |= ATA_SR_DSC;
  return true;
}

bool HDC::SeekDrive(uint32 drive, uint32 cylinder, uint32 head, uint32 sector)
{
  DebugAssert(drive < MAX_DRIVES && m_drives[drive].type == DRIVE_TYPE_HDD);

  DriveState& state = m_drives[drive];
  if (cylinder >= state.current_num_cylinders || head >= state.current_num_heads || sector < 1 ||
      sector > state.current_num_sectors)
  {
    return false;
  }

  state.current_cylinder = cylinder;
  state.current_head = head;
  state.current_sector = sector;
  state.current_lba = (cylinder * state.current_num_heads + head) * state.current_num_sectors + (sector - 1);
  m_status_register.bits |= ATA_SR_DSC;
  return true;
}

bool HDC::SeekToNextSector(uint32 drive)
{
  DriveState& state = m_drives[drive];
  if (state.current_sector == 0)
  {
    // using LBA
    if ((state.current_lba + 1) < state.hdd->GetNumLBAs())
    {
      state.current_lba++;
      return true;
    }
    else
    {
      // end of disk
      return false;
    }
  }

  // CHS addressing
  uint32 cylinder = state.current_cylinder;
  uint32 head = state.current_head;
  uint32 sector = state.current_sector;

  // move sectors -> heads -> cylinders
  sector++;
  if (sector > state.current_num_sectors)
  {
    sector = 1;
    head++;
    if (head >= state.current_num_heads)
    {
      head = 0;
      cylinder++;
      if (cylinder >= state.current_num_cylinders)
      {
        // end of disk
        return false;
      }
    }
  }
  state.current_cylinder = cylinder;
  state.current_head = head;
  state.current_sector = sector;

  uint64 old_lba = state.current_lba;
  state.current_lba = (cylinder * state.current_num_heads + head) * state.current_num_sectors + (sector - 1);
  DebugAssert((old_lba + 1) == state.current_lba);
  return true;
}

void HDC::ReadCurrentSector(uint32 drive, void* data)
{
  DebugAssert(drive < MAX_DRIVES);
  DriveState& state = m_drives[drive];
  Log_DevPrintf("HDC read lba %u offset %u", state.current_lba, state.current_lba * SECTOR_SIZE);
  state.hdd->GetImage()->Read(data, state.current_lba * SECTOR_SIZE, SECTOR_SIZE);
}

void HDC::WriteCurrentSector(uint32 drive, const void* data)
{
  DebugAssert(drive < MAX_DRIVES);
  DriveState& state = m_drives[drive];
  Log_DevPrintf("HDC write lba %u offset %u", state.current_lba, state.current_lba * SECTOR_SIZE);
  state.hdd->GetImage()->Write(data, state.current_lba * SECTOR_SIZE, SECTOR_SIZE);
}

void HDC::ConnectIOPorts(Bus* bus)
{
  uint32 BAR0, BAR1;
  if (m_channel == CHANNEL_PRIMARY)
  {
    // Primary channel
    BAR0 = 0x01F0;
    BAR1 = 0x03F6;
    m_irq_number = 14;
  }
  else
  {
    // Secondary channel
    BAR0 = 0x0170;
    BAR1 = 0x0376;
    m_irq_number = 15;
  }

  // 01F0 - Data register (R/W)
  bus->ConnectIOPortRead(BAR0 + 0, this, std::bind(&HDC::IOReadDataRegisterByte, this, std::placeholders::_2));
  bus->ConnectIOPortReadWord(BAR0 + 0, this, std::bind(&HDC::IOReadDataRegisterWord, this, std::placeholders::_2));
  bus->ConnectIOPortReadDWord(BAR0 + 0, this, std::bind(&HDC::IOReadDataRegisterDWord, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 0, this, std::bind(&HDC::IOWriteDataRegisterByte, this, std::placeholders::_2));
  bus->ConnectIOPortWriteWord(BAR0 + 0, this, std::bind(&HDC::IOWriteDataRegisterWord, this, std::placeholders::_2));
  bus->ConnectIOPortWriteDWord(BAR0 + 0, this, std::bind(&HDC::IOWriteDataRegisterDWord, this, std::placeholders::_2));

  // 01F1 - Status register (R)
  bus->ConnectIOPortRead(BAR0 + 1, this, std::bind(&HDC::IOReadErrorRegister, this, std::placeholders::_2));

  // 01F7 - Status register (R) / Command register (W)
  bus->ConnectIOPortRead(BAR0 + 7, this, std::bind(&HDC::IOReadStatusRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 7, this, std::bind(&HDC::IOWriteCommandRegister, this, std::placeholders::_2));

  // Command block
  // 01F1	w	WPC/4  (Write Precompensation Cylinder divided by 4)
  // 01F2	r/w	sector count
  // 01F3	r/w	sector number
  // 01F4	r/w	cylinder low
  // 01F5	r/w	cylinder high
  auto read_command_block = std::bind(&HDC::IOReadCommandBlock, this, std::placeholders::_1, std::placeholders::_2);
  auto write_command_block = std::bind(&HDC::IOWriteCommandBlock, this, std::placeholders::_1, std::placeholders::_2);
  for (uint32 i = 2; i <= 5; i++)
    bus->ConnectIOPortRead(BAR0 + i, this, read_command_block);
  for (uint32 i = 1; i <= 5; i++)
    bus->ConnectIOPortWrite(BAR0 + i, this, write_command_block);

  // 01F6: Drive select (R/W)
  bus->ConnectIOPortRead(BAR0 + 6, this, std::bind(&HDC::IOReadDriveSelect, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 6, this, std::bind(&HDC::IOWriteDriveSelect, this, std::placeholders::_2));

  // 03F7: Alternate status register (R) / Control register (W)
  bus->ConnectIOPortRead(BAR1 + 0, this, std::bind(&HDC::IOReadAltStatusRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR1 + 0, this, std::bind(&HDC::IOWriteControlRegister, this, std::placeholders::_2));
}

void HDC::SoftReset()
{
  ClearDriveActivity();

  // The 430FX bios seems to require that the error register be 1 after soft reset.
  m_status_register.Reset();
  m_error_register = 0x01;
  Y_memzero(&m_drive_select, sizeof(m_drive_select));
  m_current_transfer.buffer.clear();
  m_current_transfer.buffer_position = 0;
  m_current_transfer.drive_index = MAX_DRIVES;
  m_current_transfer.remaining_sectors = 0;
  m_pending_command = 0;
  m_command_event->SetActive(false);

  // Set signature bytes in current CHS
  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    DriveState& state = m_drives[i];
    if (state.type == DRIVE_TYPE_HDD)
    {
      // Reset translation? This should be behind a flag check..
      state.current_num_cylinders = state.hdd ? state.hdd->GetNumCylinders() : 0;
      state.current_num_heads = state.hdd ? state.hdd->GetNumHeads() : 0;
      state.current_num_sectors = state.hdd ? state.hdd->GetNumSectors() : 0;
    }

    SetSignature(&state);
  }

  // TODO: Stop any ATAPI commands.
}

void HDC::SetDriveActivity(u32 drive, bool writing)
{
  DriveState& state = m_drives[drive];
  if (state.type == DRIVE_TYPE_HDD)
    state.hdd->SetActivity(writing);
}

void HDC::ClearDriveActivity()
{
  // Clear activity on all drives.
  for (DriveState& state : m_drives)
  {
    if (state.type == DRIVE_TYPE_HDD)
      state.hdd->ClearActivity();
  }
}

void HDC::FlushImagesEvent()
{
  for (u32 i = 0; i < MAX_DRIVES; i++)
  {
    if (m_drives[i].hdd)
      m_drives[i].hdd->GetImage()->Flush();
  }
}

uint8 HDC::GetCurrentDriveIndex() const
{
  return m_drive_select.drive;
}

HDC::DriveState* HDC::GetCurrentDrive()
{
  DriveState& ds = m_drives[m_drive_select.drive];
  return (ds.type != DRIVE_TYPE_NONE) ? &ds : nullptr;
}

CDROM* HDC::GetCurrentATAPIDevice()
{
  return (m_drives[m_drive_select.drive].type == DRIVE_TYPE_ATAPI) ? m_drives[m_drive_select.drive].atapi_dev : nullptr;
}

void HDC::SetSignature(DriveState* drive)
{
  if (drive->type == DRIVE_TYPE_HDD)
  {
    drive->ata_sector_count = 1;
    drive->ata_sector_number = 1;
    drive->ata_cylinder_low = 0;
    drive->ata_cylinder_high = 0;
  }
  else if (drive->type == DRIVE_TYPE_ATAPI)
  {
    drive->ata_sector_count = 1;
    drive->ata_sector_number = 1;
    drive->ata_cylinder_low = 0x14;
    drive->ata_cylinder_high = 0xEB;
  }
  else
  {
    // None
    drive->ata_sector_count = 1;
    drive->ata_sector_number = 1;
    drive->ata_cylinder_low = 0xFF;
    drive->ata_cylinder_high = 0xFF;
  }
}

void HDC::IOReadStatusRegister(uint8* value)
{
  //     *value |= (0 << 7);     // Executing command
  //     *value |= (1 << 6);     // Drive is ready
  //     *value |= (0 << 5);     // Write fault
  //     *value |= (1 << 4);     // Seek complete
  //     *value |= (0 << 3);     // Sector buffer requires servicing
  //     *value |= (0 << 2);     // Disk data read successfully corrected
  //     *value |= (1 << 1);     // Index - set to 1 each revolution
  //     *value |= (0 << 0);     // Previous command ended in error

  // Lower interrupt
  m_interrupt_controller->LowerInterrupt(m_irq_number);
  *value = m_status_register.bits;
}

void HDC::IOReadAltStatusRegister(uint8* value)
{
  *value = m_status_register.bits;
}

void HDC::IOWriteCommandRegister(uint8 value)
{
  Log_DevPrintf("ATA write command register <- 0x%02X", ZeroExtend32(value));

  // Ignore writes to the command register when busy.
  if (!m_status_register.IsReady() || m_command_event->IsActive())
  {
    Log_WarningPrintf("ATA controller is not ready");
    return;
  }

  // Determine how long the command will take to execute.
  const CycleCount cycles = CalculateCommandTime(value);
  Log_DevPrintf("Queueing ATA command 0x%02X in %u us", value, unsigned(cycles));
  m_command_event->Queue(cycles);
  m_pending_command = value;

  // We're busy now, and can't accept other commands.
  m_status_register.SetBusy();

  // Set the activity indicator on the current drive, if any.
  SetDriveActivity(GetCurrentDriveIndex(), IsWriteCommand(value));
}

void HDC::IOReadErrorRegister(uint8* value)
{
  *value = m_error_register;
}

void HDC::IOWriteControlRegister(uint8 value)
{
  Log_DevPrintf("ATA write control register <- 0x%02X", ZeroExtend32(value));

  decltype(m_control_register) old_value = m_control_register;
  m_control_register.bits = value;

  if (!m_control_register.software_reset && old_value.software_reset)
  {
    // Software reset
    Log_WarningPrintf("Software reset");
    SoftReset();
  }
  else if (m_control_register.software_reset)
  {
    // Preparing for software reset, set BSY
    m_status_register.SetBusy();
  }
}

void HDC::IOReadDataRegisterByte(uint8* value)
{
  if (!m_status_register.IsAcceptingData() || m_current_transfer.buffer_position == m_current_transfer.buffer.size())
  {
    *value = 0;
    return;
  }

  *value = m_current_transfer.buffer[m_current_transfer.buffer_position];
  m_current_transfer.buffer_position++;
  UpdateTransferBuffer();
}

void HDC::IOReadDataRegisterWord(uint16* value)
{
  if (!m_status_register.IsAcceptingData() ||
      m_current_transfer.buffer_position >= (m_current_transfer.buffer.size() - 1))
  {
    *value = 0;
    return;
  }

  std::memcpy(value, &m_current_transfer.buffer[m_current_transfer.buffer_position], sizeof(uint16));
  m_current_transfer.buffer_position += sizeof(uint16);
  UpdateTransferBuffer();
}

void HDC::IOReadDataRegisterDWord(uint32* value)
{
  if (!m_status_register.IsAcceptingData() ||
      m_current_transfer.buffer_position >= (m_current_transfer.buffer.size() - 3))
  {
    *value = 0;
    return;
  }

  std::memcpy(value, &m_current_transfer.buffer[m_current_transfer.buffer_position], sizeof(uint32));
  m_current_transfer.buffer_position += sizeof(uint32);
  UpdateTransferBuffer();
}

void HDC::IOWriteDataRegisterByte(uint8 value)
{
  if (!m_status_register.IsAcceptingData() || m_current_transfer.buffer_position == m_current_transfer.buffer.size())
    return;

  if (m_current_transfer.is_packet_command)
  {
    UpdatePacketCommand(&value, sizeof(value));
    return;
  }

  m_current_transfer.buffer[m_current_transfer.buffer_position] = value;
  m_current_transfer.buffer_position++;
  UpdateTransferBuffer();
}

void HDC::IOWriteDataRegisterWord(uint16 value)
{
  if (!m_status_register.IsAcceptingData() ||
      m_current_transfer.buffer_position >= (m_current_transfer.buffer.size() - 1))
    return;

  if (m_current_transfer.is_packet_command)
  {
    UpdatePacketCommand(&value, sizeof(value));
    return;
  }

  std::memcpy(&m_current_transfer.buffer[m_current_transfer.buffer_position], &value, sizeof(uint16));
  m_current_transfer.buffer_position += sizeof(uint16);
  UpdateTransferBuffer();
}

void HDC::IOWriteDataRegisterDWord(uint32 value)
{
  if (!m_status_register.IsAcceptingData() ||
      m_current_transfer.buffer_position >= (m_current_transfer.buffer.size() - 3))
    return;

  if (m_current_transfer.is_packet_command)
  {
    UpdatePacketCommand(&value, sizeof(value));
    return;
  }

  std::memcpy(&m_current_transfer.buffer[m_current_transfer.buffer_position], &value, sizeof(uint32));
  m_current_transfer.buffer_position += sizeof(uint32);
  UpdateTransferBuffer();
}

void HDC::IOReadCommandBlock(uint32 port, uint8* value)
{
  const DriveState& state = m_drives[m_drive_select.drive];
  if (state.type == DRIVE_TYPE_NONE)
  {
    *value = 0;
    return;
  }

  // Lower 4 bits are the offset into the command block
  const u32 offset = port & 7;
  const bool hob = m_control_register.high_order_byte_readback;
  switch (offset)
  {
      // Number of sectors to read/write
    case 2:
      *value = Truncate8(hob ? (state.ata_sector_count >> 8) : (state.ata_sector_count));
      break;

      // Sector number/LBA0
    case 3:
      *value = Truncate8(hob ? (state.ata_sector_number >> 8) : (state.ata_sector_number));
      break;

      // Cylinder low/LBA1
    case 4:
      *value = Truncate8(hob ? (state.ata_cylinder_low >> 8) : (state.ata_cylinder_low));
      break;

      // Cylinder high/LBA2
    case 5:
      *value = Truncate8(hob ? (state.ata_cylinder_high >> 8) : (state.ata_cylinder_high));
      break;

    default:
      break;
  }

  Log_DevPrintf("ATA read command block %04X <- 0x%02X", port, ZeroExtend32(*value));
}

void HDC::IOWriteCommandBlock(uint32 port, uint8 value)
{
  Log_DevPrintf("ATA write command block %04X -> 0x%02X", port, ZeroExtend32(value));

  // Lower 4 bits are the offset into the command block
  uint32 offset = port & 15;

  // We have to write the value to both drives
  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    DriveState& state = m_drives[i];
    if (state.type == DRIVE_TYPE_NONE)
      continue;

    switch (offset)
    {
      case 1:
        // Features
        m_feature_select = value;
        break;

        // Number of sectors to read/write
      case 2:
        state.ata_sector_count <<= 8;
        state.ata_sector_count |= ZeroExtend16(value);
        break;

        // Sector number/LBA0
        // Shift bits 0-7 to 24-31
      case 3:
        state.ata_sector_number <<= 8;
        state.ata_sector_number |= ZeroExtend16(value);
        break;

        // Cylinder low/LBA1
        // Shift bits 8-15 to 32-39
      case 4:
        state.ata_cylinder_low <<= 8;
        state.ata_cylinder_low |= ZeroExtend16(value);
        break;

        // Cylinder high/LBA2
        // Shift bits 16-23 to 40-47
      case 5:
        state.ata_cylinder_high <<= 8;
        state.ata_cylinder_high |= ZeroExtend16(value);
        break;

      default:
        break;
    }
  }
}

void HDC::IOReadDriveSelect(uint8* value)
{
  *value = m_drive_select.bits;
}

void HDC::IOWriteDriveSelect(uint8 value)
{
  Log_DevPrintf("ATA write drive select <- 0x%02X", ZeroExtend32(value));

  m_drive_select.bits = value;

  //     // Update current head for selected drive
  //     DriveState* state = GetCurrentDrive();
  //     if (state)
  //         state->current_head = m_drive_select.head;
}

bool HDC::FillReadBuffer(uint32 drive_index, uint32 sector_count)
{
  // NOTE: If this fails it should set error state
  // TODO: Handle end of disk...
  for (uint32 i = 0; i < sector_count; i++)
  {
    DebugAssert(m_drives[drive_index].current_lba < m_drives[drive_index].hdd->GetNumLBAs());
    ReadCurrentSector(drive_index, &m_current_transfer.buffer[i * SECTOR_SIZE]);
    if (!SeekToNextSector(drive_index) && (i != (sector_count - 1)))
      return false;
  }

  return true;
}

void HDC::PrepareWriteBuffer(uint32 drive_index, uint32 sector_count)
{
  m_current_transfer.buffer.resize(sector_count * SECTOR_SIZE);
}

bool HDC::FlushWriteBuffer(uint32 drive_index, uint32 sector_count)
{
  // NOTE: If this fails it should set error state
  // TODO: Handle end of disk...
  for (uint32 i = 0; i < sector_count; i++)
  {
    DebugAssert(m_drives[drive_index].current_lba < m_drives[drive_index].hdd->GetNumLBAs());
    WriteCurrentSector(drive_index, &m_current_transfer.buffer[i * SECTOR_SIZE]);
    if (!SeekToNextSector(drive_index) && (i != (sector_count - 1)))
      return false;
  }

  return true;
}

CycleCount HDC::CalculateCommandTime(u8 command) const
{
  const DriveState& ds = m_drives[m_drive_select.drive];
  switch (ds.type)
  {
    case DRIVE_TYPE_HDD:
    {
      switch (command)
      {
        // Make reads take slightly longer.
        // TODO: Use number of sectors, etc.
        case ATA_CMD_READ_PIO:
        case ATA_CMD_READ_PIO_NO_RETRY:
        case ATA_CMD_READ_PIO_EXT:
        case ATA_CMD_READ_DMA:
        case ATA_CMD_READ_DMA_EXT:
        case ATA_CMD_READ_VERIFY:
        case ATA_CMD_READ_VERIFY_EXT:
        case ATA_CMD_WRITE_PIO:
        case ATA_CMD_WRITE_PIO_NO_RETRY:
        case ATA_CMD_WRITE_PIO_EXT:
        case ATA_CMD_WRITE_DMA:
        case ATA_CMD_WRITE_DMA_EXT:
          return 10;

          // Default to 1us for all other commands.
        default:
          return 1;
      }
    }
    break;

    case DRIVE_TYPE_ATAPI:
    {
      // Most commands are invalid here, but the controller responds immediately so the packet can be written.
      return 1;
    }

    case DRIVE_TYPE_NONE:
    default:
    {
      // No drive here, just assume it takes 1us.
      return 1;
    }
  }
}

bool HDC::IsWriteCommand(u8 command) const
{
  switch (command)
  {
    case ATA_CMD_WRITE_PIO:
    case ATA_CMD_WRITE_PIO_NO_RETRY:
    case ATA_CMD_WRITE_PIO_EXT:
    case ATA_CMD_WRITE_DMA:
    case ATA_CMD_WRITE_DMA_EXT:
      return true;

    default:
      return false;
  }
}

void HDC::ExecutePendingCommand()
{
  const u8 command = m_pending_command;
  m_pending_command = 0;
  m_command_event->Deactivate();

  Log_DevPrintf("Executing ATA command 0x%02X on drive %u", command, GetCurrentDriveIndex());

  DriveState& ds = m_drives[m_drive_select.drive];
  if (ds.type == DRIVE_TYPE_NONE)
  {
    // Is this correct?
    AbortCommand();
    return;
  }

  switch (ds.type)
  {
    case DRIVE_TYPE_HDD:
    {
      switch (command)
      {
        case ATA_CMD_DEVICE_RESET:
          HandleATADeviceReset();
          break;

        case ATA_CMD_IDENTIFY:
          HandleATAIdentify();
          break;

        case ATA_CMD_RECALIBRATE + 0x0:
        case ATA_CMD_RECALIBRATE + 0x1:
        case ATA_CMD_RECALIBRATE + 0x2:
        case ATA_CMD_RECALIBRATE + 0x3:
        case ATA_CMD_RECALIBRATE + 0x4:
        case ATA_CMD_RECALIBRATE + 0x5:
        case ATA_CMD_RECALIBRATE + 0x6:
        case ATA_CMD_RECALIBRATE + 0x7:
        case ATA_CMD_RECALIBRATE + 0x8:
        case ATA_CMD_RECALIBRATE + 0x9:
        case ATA_CMD_RECALIBRATE + 0xA:
        case ATA_CMD_RECALIBRATE + 0xB:
        case ATA_CMD_RECALIBRATE + 0xC:
        case ATA_CMD_RECALIBRATE + 0xD:
        case ATA_CMD_RECALIBRATE + 0xE:
        case ATA_CMD_RECALIBRATE + 0xF:
          HandleATARecalibrate();
          break;

        case ATA_CMD_READ_PIO:
          HandleATATransferPIO(false, false, false);
          break;

        case ATA_CMD_READ_MULTIPLE_PIO:
          HandleATATransferPIO(false, false, true);
          break;

        case ATA_CMD_READ_PIO_EXT:
        case ATA_CMD_READ_PIO_NO_RETRY:
          HandleATATransferPIO(false, true, false);
          break;

        case ATA_CMD_READ_VERIFY:
          HandleATAReadVerifySectors(false, true);
          break;

        case ATA_CMD_READ_VERIFY_EXT:
          HandleATAReadVerifySectors(true, false);
          break;

        case ATA_CMD_WRITE_PIO:
        case ATA_CMD_WRITE_PIO_NO_RETRY:
          HandleATATransferPIO(true, false, false);
          break;

        case ATA_CMD_WRITE_MULTIPLE_PIO:
          HandleATATransferPIO(true, false, true);
          break;

        case ATA_CMD_WRITE_PIO_EXT:
          HandleATATransferPIO(true, true, false);
          break;

        case ATA_CMD_SET_MULTIPLE_MODE:
          HandleATASetMultipleMode();
          break;

        case ATA_CMD_EXECUTE_DRIVE_DIAGNOSTIC:
          HandleATAExecuteDriveDiagnostic();
          break;

        case ATA_CMD_INITIALIZE_DRIVE_PARAMETERS:
          HandleATAInitializeDriveParameters();
          break;

        case ATA_CMD_SET_FEATURES:
          HandleATASetFeatures();
          break;

        default:
        {
          // Unknown command - abort is correct?
          Log_ErrorPrintf("Unknown ATA command 0x%02X", ZeroExtend32(command));
          AbortCommand();
        }
        break;
      }
    }
    break;

    case DRIVE_TYPE_ATAPI:
    {
      switch (command)
      {
        case ATA_CMD_IDENTIFY:
          HandleATAIdentify();
          break;

        case ATA_CMD_IDENTIFY_PACKET:
          HandleATAPIIdentify();
          break;

        case ATA_CMD_DEVICE_RESET:
          HandleATADeviceReset();
          break;

        case ATAPI_CMD_PACKET:
          HandleATAPIPacket();
          break;

        default:
        {
          // Unknown command - abort is correct?
          Log_ErrorPrintf("Unknown ATAPI command 0x%02X", ZeroExtend32(command));
          AbortCommand();
        }
        break;
      }
    }
    break;

    default:
    {
      Log_ErrorPrintf("Unknown drive type");
      AbortCommand();
    }
    break;
  }
}

#pragma pack(push, 1)
struct ATA_IDENTIFY_RESPONSE
{
  uint16 flags;                        // 0
  uint16 cylinders;                    // 1
  uint16 unused_0;                     // 2
  uint16 heads;                        // 3
  uint16 unformatted_bytes_per_track;  // 4
  uint16 unformatted_bytes_per_sector; // 5
  uint16 sectors_per_track;            // 6
  uint16 unused_1[3];                  // 7-9
  char serial_number[20];              // 10-19
  uint16 buffer_type;                  // 20
  uint16 buffer_size;                  // 21 - in 512kb increments
  uint16 ecc_bytes;                    // 22 - should be 4
  char firmware_revision[8];           // 23-26
  char model[40];                      // 27-46
  uint16 readwrite_multiple_supported; // 47
  uint16 dword_io_supported;           // 48
  uint16 support;                      // 49
  uint16 unused_2;                     // 50
  uint16 pio_timing_mode;              // 51
  uint16 dma_timing_mode;              // 52
  uint16 user_fields_valid;            // 53
  uint16 user_cylinders;               // 54
  uint16 user_heads;                   // 55
  uint16 user_sectors_per_track;       // 56
  uint32 user_total_sectors;           // 57-58
  uint16 multiple_sector_setting;      // 59
  uint32 lba_sectors;                  // 60-61
  uint16 singleword_dma_modes;         // 62
  uint16 multiword_dma_modes;          // 63
  uint16 pio_modes_supported;          // 64
  uint16 pio_cycle_time[4];            // 65-68
  uint16 unused_4[11];                 // 69-79
  uint16 word_80;                      // 80
  uint16 minor_version_number;         // 81
  uint16 word_82;                      // 82
  uint16 supports_lba48;               // 83 set (1 << 10) here
  uint16 word_84;                      // 84
  uint16 word_85;                      // 85
  uint16 word_86;                      // 86
  uint16 word_87;                      // 87
  uint16 word_88;                      // 88
  uint16 word_89;                      // 89
  uint16 word_90;                      // 90
  uint16 word_91;                      // 91
  uint16 word_92;                      // 92
  uint16 word_93;                      // 93
  uint16 unused_5[6];                  // 94-99
  uint64 lba48_sectors;                // 100-103
  uint16 unused_6[152];                // 104-255
};
static_assert(sizeof(ATA_IDENTIFY_RESPONSE) == HDC::SECTOR_SIZE, "ATA identify response is one sector in length");
#pragma pack(pop)

static void PutIdentifyString(char* buffer, uint32 buffer_size, const char* str)
{
  Assert(buffer_size <= 40);
  char temp_buffer[40 + 1];
  size_t len = std::strlen(str);
  Y_strncpy(temp_buffer, countof(temp_buffer), str);
  for (size_t i = len; i < countof(temp_buffer); i++)
    temp_buffer[i] = ' ';

  uint16* word_buffer = reinterpret_cast<uint16*>(buffer);
  for (uint32 i = 0; i < (buffer_size / sizeof(uint16)); i++)
    word_buffer[i] = ZeroExtend16(temp_buffer[i * 2] << 8) | ZeroExtend16(temp_buffer[i * 2 + 1]);
}

void HDC::HandleATAIdentify()
{
  Log_DevPrintf("ATA identify drive %u", ZeroExtend32(GetCurrentDriveIndex()));
  DriveState* drive = GetCurrentDrive();
  DebugAssert(drive);

  // ATAPI devices should set their signature, then abort.
  if (drive->type == DRIVE_TYPE_ATAPI)
  {
    SetSignature(drive);
    AbortCommand(ATA_ERR_ABRT);
    return;
  }

  ATA_IDENTIFY_RESPONSE response = {};
  // response.flags |= (1 << 10); // >10mbit/sec transfer speed
  response.flags |= (1 << 6); // Fixed drive
  // response.flags |= (1 << 2);  // Soft sectored
  response.cylinders = Truncate16(drive->hdd->GetNumCylinders());
  response.heads = Truncate16(drive->hdd->GetNumHeads());
  response.unformatted_bytes_per_track = Truncate16(SECTOR_SIZE * drive->hdd->GetNumSectors());
  response.unformatted_bytes_per_sector = Truncate16(SECTOR_SIZE);
  response.sectors_per_track = Truncate16(drive->hdd->GetNumSectors());
  response.buffer_type = 3;
  response.buffer_size = 512; // 256KB cache
  response.ecc_bytes = 4;
  PutIdentifyString(response.serial_number, sizeof(response.serial_number), "DERP123");
  PutIdentifyString(response.firmware_revision, sizeof(response.firmware_revision), "HURR101");

  // Temporarily disabled as it seems to be broken.
  // response.readwrite_multiple_supported = 0x8000 | 16; // this is actually the number of sectors, tweak it
  response.readwrite_multiple_supported = 0;

  response.dword_io_supported = 1;
  // response.support |= (1 << 9); // LBA supported
  // response.support |= (1 << 8);       // DMA supported
  response.support = 0xf000u & ~uint32(1 << 8);
  response.pio_timing_mode = 2;
  response.dma_timing_mode = 1;
  response.user_fields_valid = 3;
  response.user_cylinders = Truncate16(drive->current_num_cylinders);
  response.user_heads = Truncate16(drive->current_num_heads);
  response.user_sectors_per_track = Truncate16(drive->current_num_sectors);
  response.user_total_sectors =
    Truncate32(drive->current_num_cylinders * drive->current_num_heads * drive->current_num_sectors);
  response.lba_sectors =
    Truncate32(std::min(drive->hdd->GetNumLBAs(), ZeroExtend64(std::numeric_limits<uint32>::max())));
  PutIdentifyString(response.model, sizeof(response.model), "Herp derpity derp");
  // response.singleword_dma_modes = (1 << 0) | (1 << 8);
  // response.multiword_dma_modes = (1 << 0) | (1 << 8);
  response.pio_modes_supported = 0x03;
  for (size_t i = 0; i < countof(response.pio_cycle_time); i++)
    response.pio_cycle_time[i] = 120;
  response.word_80 = 0xF0;
  response.minor_version_number = 0x16;
  response.word_82 = (1 << 14);
  response.supports_lba48 = (1 << 10);
  response.word_84 = (1 << 14);
  response.word_85 = (1 << 14);
  response.word_86 = (1 << 10);
  response.word_93 = 1 | (1 << 14) | 0x2000;
  response.lba48_sectors = drive->hdd->GetNumLBAs();

  // 512 bytes total
  BeginTransfer(MAX_DRIVES, 1, 1, false);
  std::memcpy(m_current_transfer.buffer.data(), &response, sizeof(response));
  DataRequestReady();
}

void HDC::HandleATAPIIdentify()
{
  Log_DevPrintf("ATA identify packet drive %u", ZeroExtend32(GetCurrentDriveIndex()));
  DriveState* drive = GetCurrentDrive();
  DebugAssert(drive);

  ATA_IDENTIFY_RESPONSE response = {};
  for (int i = 0; i < 512; i++)
    response.flags |= (2 << 5);
  response.flags |= (1 << 7);  // removable
  response.flags |= (5 << 8);  // cdrom
  response.flags |= (2 << 14); // atapi device
  PutIdentifyString(response.serial_number, sizeof(response.serial_number), "DERP123");
  PutIdentifyString(response.firmware_revision, sizeof(response.firmware_revision), "HURR101");
  response.dword_io_supported = 0;
  response.support = (1 << 9);
  response.pio_timing_mode = 0x200;
  PutIdentifyString(response.model, sizeof(response.model), drive->atapi_dev->GetModelIDString());
  for (size_t i = 0; i < countof(response.pio_cycle_time); i++)
    response.pio_cycle_time[i] = 120;
  response.word_80 = (1 << 4);
  response.minor_version_number = 0x0017;
  response.word_82 = (1 << 14) | (1 << 9) | (1 << 4);

  // 512 bytes total
  BeginTransfer(MAX_DRIVES, 1, 1, false);
  std::memcpy(m_current_transfer.buffer.data(), &response, sizeof(response));
  DataRequestReady();

  // Signature is reset
  SetSignature(drive);
}

void HDC::HandleATARecalibrate()
{
  Log_DevPrintf("ATA identify drive %u", ZeroExtend32(GetCurrentDriveIndex()));

  // Shouldn't fail if the drive exists.
  if (!SeekDrive(GetCurrentDriveIndex(), 0))
  {
    AbortCommand();
    return;
  }

  // Do we have to set the IO registers too?
  CompleteCommand();
}

void HDC::HandleATATransferPIO(bool write, bool extended, bool multiple)
{
  DriveState* drive = GetCurrentDrive();
  uint32 drive_index = GetCurrentDriveIndex();
  DebugAssert(drive);

  if (multiple && drive->multiple_sectors == 0)
  {
    AbortCommand();
    return;
  }

  // Using CHS mode?
  if (!m_drive_select.lba_enable)
  {
    // Validate that the CHS are in range and seek to it
    uint32 cylinder = ZeroExtend32((drive->ata_cylinder_low & 0xFF) | ((drive->ata_cylinder_high & 0xFF) << 8));
    uint32 head = ZeroExtend32(m_drive_select.head.GetValue());
    uint32 sector = ZeroExtend32(drive->ata_sector_number & 0xFF);
    uint32 count = ZeroExtend32(drive->ata_sector_count & 0xFF);
    if (!SeekDrive(drive_index, cylinder, head, sector) ||
        ((drive->current_lba + ZeroExtend64(count)) > drive->hdd->GetNumLBAs()))
    {
      AbortCommand(ATA_ERR_IDNF);
      return;
    }

    Log_DevPrintf("PIO %s %u sectors from CHS %u/%u/%u", write ? "write" : "read", count, cylinder, head, sector);

    // Setup transfer and fire irq
    BeginTransfer(drive_index, multiple ? drive->multiple_sectors : 1, count, write);
    uint32 sectors_in_block = std::min(m_current_transfer.remaining_sectors, m_current_transfer.sectors_per_block);
    if (!write)
    {
      if (!FillReadBuffer(drive_index, sectors_in_block))
      {
        StopTransfer();
        AbortCommand(ATA_ERR_IDNF);
        return;
      }
    }
    else
    {
      PrepareWriteBuffer(drive_index, sectors_in_block);
    }

    DataRequestReady();
  }
  else
  {
    // Not using LBA48?
    uint64 start_lba;
    uint32 count;
    if (!extended)
    {
      // Using LBA24
      start_lba = (ZeroExtend64(drive->ata_sector_number & 0xFF) << 0);
      start_lba |= (ZeroExtend64(drive->ata_cylinder_low & 0xFF) << 8);
      start_lba |= (ZeroExtend64(drive->ata_cylinder_high & 0xFF) << 16);
      count = ZeroExtend32(drive->ata_sector_count & 0xFF);
    }
    else
    {
      // Using LBA48
      start_lba = (ZeroExtend64(drive->ata_sector_number) << 0);
      start_lba |= (ZeroExtend64(drive->ata_cylinder_low) << 16);
      start_lba |= (ZeroExtend64(drive->ata_cylinder_high) << 32);
      count = ZeroExtend32(drive->ata_sector_count);
    }

    // Check that it's in range
    if (!SeekDrive(drive_index, start_lba) || ((drive->current_lba + ZeroExtend64(count)) > drive->hdd->GetNumLBAs()))
    {
      AbortCommand(ATA_ERR_IDNF);
      return;
    }

    Log_DevPrintf("PIO %s %u sectors from LBA %u", write ? "write" : "read", count, Truncate32(start_lba));

    // Setup transfer and fire irq
    BeginTransfer(drive_index, multiple ? drive->multiple_sectors : 1, count, write);
    uint32 sectors_in_block = std::min(m_current_transfer.remaining_sectors, m_current_transfer.sectors_per_block);
    if (!write)
    {
      if (!FillReadBuffer(drive_index, sectors_in_block))
      {
        AbortCommand(ATA_ERR_IDNF);
        return;
      }
    }
    else
    {
      PrepareWriteBuffer(drive_index, sectors_in_block);
    }

    DataRequestReady();
  }
}

void HDC::HandleATAReadVerifySectors(bool extended, bool with_retry)
{
  // TODO: Timer
  DriveState* drive = GetCurrentDrive();
  uint32 drive_index = GetCurrentDriveIndex();
  DebugAssert(drive);

  // Using CHS mode?
  if (!m_drive_select.lba_enable)
  {
    // Validate that the CHS are in range and seek to it
    uint32 cylinder = ZeroExtend32((drive->ata_cylinder_low & 0xFF) | ((drive->ata_cylinder_high & 0xFF) << 8));
    uint32 head = ZeroExtend32(m_drive_select.head.GetValue());
    uint32 sector = ZeroExtend32(drive->ata_sector_number & 0xFF);
    uint32 count = ZeroExtend32(drive->ata_sector_count & 0xFF);
    if (!SeekDrive(drive_index, cylinder, head, sector) ||
        ((drive->current_lba + ZeroExtend64(count)) > drive->hdd->GetNumLBAs()) || count == 0)
    {
      // AbortCommand(ATA_ERR_IDNF);
      CompleteCommand();
      return;
    }

    Log_DevPrintf("Verify %u sectors from CHS %u/%u/%u", count, cylinder, head, sector);

    // The command block needs to contain the last sector verified
    bool error = false;
    for (uint32 i = 0; i < (count - 1); i++)
    {
      if (!SeekToNextSector(drive_index))
      {
        error = true;
        break;
      }
    }

    drive->ata_cylinder_low = Truncate8(drive->current_cylinder);
    drive->ata_cylinder_high = Truncate16((drive->current_cylinder >> 8) & 0xFF);
    drive->ata_sector_number = Truncate16(drive->current_sector);
    m_drive_select.head = Truncate8(drive->current_head);
    if (!error)
      CompleteCommand();
    else
      AbortCommand(ATA_ERR_BBK);
  }
  else
  {
    // Not using LBA48?
    uint64 start_lba;
    uint32 count;
    if (!extended)
    {
      // Using LBA24
      start_lba = (ZeroExtend64(drive->ata_sector_number & 0xFF) << 0);
      start_lba |= (ZeroExtend64(drive->ata_cylinder_low & 0xFF) << 8);
      start_lba |= (ZeroExtend64(drive->ata_cylinder_high & 0xFF) << 16);
      count = ZeroExtend32(drive->ata_sector_count & 0xFF);
    }
    else
    {
      // Using LBA48
      start_lba = (ZeroExtend64(drive->ata_sector_number) << 0);
      start_lba |= (ZeroExtend64(drive->ata_cylinder_low) << 16);
      start_lba |= (ZeroExtend64(drive->ata_cylinder_high) << 32);
      count = ZeroExtend32(drive->ata_sector_count);
    }

    // Check that it's in range
    if (!SeekDrive(drive_index, start_lba) || ((drive->current_lba + ZeroExtend64(count)) > drive->hdd->GetNumLBAs()) ||
        count == 0)
    {
      AbortCommand(ATA_ERR_IDNF);
      return;
    }

    Log_DevPrintf("Verify %u sectors from LBA %u", count, Truncate32(start_lba));

    // The command block needs to contain the last sector verified
    bool error = false;
    for (uint32 i = 0; i < (count - 1); i++)
    {
      if (!SeekToNextSector(drive_index))
      {
        error = true;
        break;
      }
    }

    if (!extended)
    {
      drive->ata_sector_number = ZeroExtend16(Truncate8(drive->current_lba));
      drive->ata_cylinder_low = ZeroExtend16(Truncate8(drive->current_lba >> 8));
      drive->ata_cylinder_high = ZeroExtend16(Truncate8(drive->current_lba >> 16));
    }
    else
    {
      drive->ata_sector_number = Truncate16(drive->current_lba);
      drive->ata_cylinder_low = Truncate16(drive->current_lba >> 16);
      drive->ata_cylinder_high = Truncate16(drive->current_lba >> 24);
    }

    if (!error)
      CompleteCommand();
    else
      AbortCommand(ATA_ERR_BBK);
  }
}

void HDC::HandleATASetMultipleMode()
{
  DriveState* drive = GetCurrentDrive();
  DebugAssert(drive);

  // TODO: We should probably set an upper bound on the number of sectors
  uint16 multiple_sectors = drive->ata_sector_count & 0xFF;

  Log_DevPrintf("ATA set multiple for drive %u to %u", GetCurrentDriveIndex(), multiple_sectors);

  // Has to be aligned
  if (multiple_sectors != 0 && (multiple_sectors > 16 || (multiple_sectors & (multiple_sectors - 1)) != 0))
  {
    AbortCommand();
  }
  else
  {
    drive->multiple_sectors = multiple_sectors;
    CompleteCommand();
  }
}

void HDC::HandleATAExecuteDriveDiagnostic()
{
  Log_DevPrintf("ATA execute drive diagnostic");

  // This is sent to both drives.
  m_error_register = 0x01; // No error detected
  CompleteCommand();
}

void HDC::HandleATAInitializeDriveParameters()
{
  DriveState* drive = GetCurrentDrive();
  DebugAssert(drive);

  // TODO: These should be controller-specific not drive-specific.. but the reset command :S
  uint32 num_cylinders = ZeroExtend32(drive->hdd->GetNumCylinders());
  uint32 num_heads = ZeroExtend32(m_drive_select.head.GetValue()) + 1;
  uint32 num_sectors = ZeroExtend32(drive->ata_sector_count & 0xFF);

  Log_DevPrintf("ATA initialize drive parameters drive=%u,heads=%u,sectors=%u", GetCurrentDriveIndex(), num_heads,
                num_sectors);

  // Abort the command if the translation is not possible.
  if ((num_cylinders * num_heads * num_sectors) > drive->hdd->GetNumLBAs())
  {
    Log_WarningPrintf("ATA invalid geometry");
    AbortCommand();
    return;
  }

  drive->current_num_cylinders = num_cylinders;
  drive->current_num_heads = num_heads;
  drive->current_num_sectors = num_sectors;
  CompleteCommand();
}

void HDC::HandleATASetFeatures()
{
  DriveState* drive = GetCurrentDrive();
  DebugAssert(drive);

  Log_DevPrintf("ATA set features 0x%02X", ZeroExtend32(m_feature_select));

  switch (m_feature_select)
  {
    case 0x03: // Set transfer mode
    {
      u8 transfer_mode = Truncate8((drive->ata_sector_count >> 3) & 0x1F);
      u8 sub_mode = Truncate8(drive->ata_sector_count & 0x03);
      Log_DevPrintf("ATA set transfer mode 0x%x 0x%x", transfer_mode, sub_mode);
      switch (transfer_mode)
      {
        case 0: // PIO mode
        case 1: // PIO flow control transfer mode
        {
          // This is okay.
          CompleteCommand();
        }
        break;

        default:
        {
          Log_ErrorPrintf("Unknown ATA transfer mode 0x%x 0x%x", transfer_mode, sub_mode);
          AbortCommand();
        }
        break;
      }
    }
    break;

    case 0x66: // Use current settings as default
      Log_DevPrintf("ATA use current settings as default");
      CompleteCommand();
      return;

    default:
      Log_ErrorPrintf("Unknown feature 0x%02X", ZeroExtend32(m_feature_select));
      AbortCommand();
      return;
  }
}

void HDC::HandleATADeviceReset()
{
  DriveState* drive = GetCurrentDrive();
  DebugAssert(drive);

  Log_DevPrintf("ATAPI reset drive %u", ZeroExtend32(GetCurrentDriveIndex()));

  if (drive->type == DRIVE_TYPE_HDD)
  {
    StopTransfer();
    CompleteCommand();
    SetSignature(drive);
  }
  else if (drive->type == DRIVE_TYPE_ATAPI)
  {
    // If the device is reset to its default state, ERR bit is set.
    // Signature bits are set
    CompleteCommand();
    SetSignature(drive);
  }
  else
  {
    AbortCommand();
  }
}

void HDC::HandleATAPIPacket()
{
  // uint32 max_packet_size = ZeroExtend32(drive->ata_cylinder_low & 0xFF) | (ZeroExtend32(drive->ata_cylinder_high &
  // 0xFF) << 8);
  Log_DevPrintf("ATAPI packet drive %u", GetCurrentDriveIndex());
  DriveState* drive = GetCurrentDrive();
  DebugAssert(drive->atapi_dev);

  // Must be in PIO mode now
  if (m_feature_select != 0)
  {
    Log_ErrorPrintf("ATAPI dma requested");
    AbortCommand();
    return;
  }
  else if (drive->atapi_dev->IsBusy())
  {
    Log_ErrorPrintf("ATAPI device busy");
    AbortCommand();
    return;
  }

  StopTransfer();
  drive->SetATAPIInterruptReason(true, false, false);

  // The interrupt isn't raised here.
  m_status_register.bits &= ~(ATA_SR_BSY | ATA_SR_ERR);
  m_status_register.bits |= ATA_SR_DRQ | ATA_SR_DRDY;
  m_error_register = 0;
  m_current_transfer.buffer.resize(SECTOR_SIZE);
  m_current_transfer.buffer_position = 0;
  m_current_transfer.drive_index = GetCurrentDriveIndex();
  m_current_transfer.sectors_per_block = 0;
  m_current_transfer.remaining_sectors = 1;
  m_current_transfer.is_packet_command = true;
  m_current_transfer.is_packet_data = false;
  m_current_transfer.is_write = false;
}

void HDC::HandleATAPICommandCompleted(uint32 drive_index)
{
  DriveState& drive = m_drives[drive_index];
  auto* device = drive.atapi_dev;
  DebugAssert(device);

  StopTransfer();

  // Was there an error?
  if (device->HasError())
  {
    drive.SetATAPIInterruptReason(true, true, false);
    AbortCommand(device->GetSenseKey() << 4);
    return;
  }

  // No response?
  if (device->GetDataResponseSize() == 0)
  {
    drive.SetATAPIInterruptReason(true, true, false);
    CompleteCommand();
    return;
  }

  // Update the last cylinder with the transfer size.
  drive.ata_cylinder_low = Truncate8(device->GetDataResponseSize());
  drive.ata_cylinder_high = Truncate8(device->GetDataResponseSize() >> 8);
  drive.SetATAPIInterruptReason(false, true, false);

  // Set up the transfer.
  m_current_transfer.buffer.resize(device->GetDataResponseSize());
  m_current_transfer.buffer_position = 0;
  m_current_transfer.drive_index = drive_index;
  m_current_transfer.sectors_per_block = 1;
  m_current_transfer.remaining_sectors = 1;
  m_current_transfer.is_write = false;
  m_current_transfer.is_packet_command = false;
  m_current_transfer.is_packet_data = true;

  // Copy data in.
  std::memcpy(m_current_transfer.buffer.data(), device->GetDataBuffer(), device->GetDataResponseSize());
  device->ClearDataBuffer();

  // Clear the busy flag, and raise interrupt.
  m_status_register.bits = (m_status_register.bits & ~(ATA_SR_BSY | ATA_SR_ERR)) | ATA_SR_DRQ;
  if (device->GetRemainingSectors() == 0)
    m_status_register.bits |= ATA_SR_DRDY;
  m_error_register = 0;
  RaiseInterrupt();
}

void HDC::AbortCommand(uint8 error /* = ATA_ERR_ABRT */, bool write_fault /* = false */)
{
  Log_WarningPrintf("Command aborted with error = 0x%02X", ZeroExtend32(error));
  StopTransfer();
  ClearDriveActivity();

  m_status_register.SetError(write_fault);
  m_error_register = error;
  RaiseInterrupt();
}

void HDC::CompleteCommand()
{
  StopTransfer();
  ClearDriveActivity();
  m_status_register.SetReady();
  m_error_register = 0;
  RaiseInterrupt();
}

void HDC::BeginTransfer(uint32 drive_index, uint32 sectors_per_block, uint32 num_sectors, bool is_write)
{
  Assert(num_sectors > 0);
  Assert(drive_index < MAX_DRIVES || num_sectors == 1);

  m_status_register.bits &= ~ATA_SR_ERR;
  m_status_register.bits |= ATA_SR_DRDY | ATA_SR_DRQ;
  m_current_transfer.buffer.resize(SECTOR_SIZE * std::min(sectors_per_block, num_sectors));
  m_current_transfer.buffer_position = 0;
  m_current_transfer.drive_index = drive_index;
  m_current_transfer.sectors_per_block = sectors_per_block;
  m_current_transfer.remaining_sectors = num_sectors;
  m_current_transfer.is_write = is_write;
  m_current_transfer.is_packet_command = false;
  m_current_transfer.is_packet_data = false;
}

void HDC::UpdatePacketCommand(const void* data, size_t data_size)
{
  DebugAssert(m_current_transfer.is_packet_command && m_current_transfer.drive_index < MAX_DRIVES &&
              m_drives[m_current_transfer.drive_index].type == DRIVE_TYPE_ATAPI);

  auto* device = m_drives[m_current_transfer.drive_index].atapi_dev;
  if (!device->WriteCommandBuffer(data, data_size))
  {
    // Command still incomplete.
    return;
  }

  // Command complete. Error?
  if (device->HasError())
  {
    // Raise interrupt and set error flag.
    StopTransfer();
    AbortCommand();
    return;
  }

  // Set the busy flag, and wait for the command to complete.
  m_status_register.bits &= ~(ATA_SR_DRDY | ATA_SR_DRQ);
  m_status_register.bits |= ATA_SR_BSY;
}

void HDC::UpdateTransferBuffer()
{
  if (m_current_transfer.buffer_position != m_current_transfer.buffer.size())
    return;

  // End of current sector
  m_current_transfer.buffer_position = 0;

  // If we're writing we need to flush this sector to the backend
  uint32 sectors_transferred = std::min(m_current_transfer.remaining_sectors, m_current_transfer.sectors_per_block);
  if (m_current_transfer.is_write)
  {
    if (!FlushWriteBuffer(m_current_transfer.drive_index, sectors_transferred))
    {
      AbortCommand(ATA_ERR_BBK);
      return;
    }
  }

  // Do we have more data to transfer?
  m_current_transfer.remaining_sectors -= sectors_transferred;
  if (m_current_transfer.remaining_sectors == 0)
  {
    if (m_current_transfer.is_packet_data)
    {
      DriveState& drive = m_drives[m_current_transfer.drive_index];
      auto* dev = drive.atapi_dev;
      if (dev->GetRemainingSectors() > 0)
      {
        // TODO: ATAPI writes
        DebugAssert(!m_current_transfer.is_write);
        dev->TransferNextSector();
        return;
      }

      drive.SetATAPIInterruptReason(true, true, false);
    }

    CompleteCommand();
    return;
  }

  // Set busy flag for the next read.
  m_status_register.SetBusy();

  // If we're reading, we need to populate the sector buffer
  uint32 sectors_in_block = std::min(m_current_transfer.remaining_sectors, m_current_transfer.sectors_per_block);
  DebugAssert(sectors_in_block > 0);
  if (!m_current_transfer.is_write)
  {
    if (!FillReadBuffer(m_current_transfer.drive_index, sectors_in_block))
    {
      AbortCommand(ATA_ERR_BBK);
      return;
    }
  }
  else
  {
    // Otherwise we need to ensure the buffer is the correct size
    PrepareWriteBuffer(m_current_transfer.drive_index, sectors_in_block);
  }

  // Next read/write ready.
  DataRequestReady();
}

void HDC::DataRequestReady()
{
  m_status_register.SetDRQ();

  // Raise interrupt if enabled
  RaiseInterrupt();
}

void HDC::RaiseInterrupt()
{
  if (!m_control_register.disable_interrupts)
  {
    Log_DevPrintf("Raising HDD interrupt");
    m_interrupt_controller->RaiseInterrupt(m_irq_number);
  }
}

void HDC::StopTransfer()
{
  m_current_transfer.buffer.clear();
  m_current_transfer.buffer_position = 0;
  m_current_transfer.drive_index = MAX_DRIVES;
  m_current_transfer.sectors_per_block = 0;
  m_current_transfer.is_write = false;
  m_current_transfer.remaining_sectors = 0;
  m_current_transfer.is_packet_command = false;
  m_current_transfer.is_packet_data = false;
}

void HDC::DriveState::SetATAPIInterruptReason(bool is_command, bool data_from_device, bool release)
{
  // Bit 0 - CoD, Bit 1 - I/O, Bit 2- RELEASE
  ata_sector_count = BoolToUInt8(is_command) | (BoolToUInt8(data_from_device) << 1) | (BoolToUInt8(release) << 2);
}

} // namespace HW