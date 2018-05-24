#include "pce/hw/hdc.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "pce/bus.h"
#include "pce/hw/cdrom.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cstring>
Log_SetChannel(HW::HDC);

namespace HW {

HDC::HDC(CHANNEL channel) : m_channel(channel) {}

HDC::~HDC() {}
void HDC::Initialize(System* system, Bus* bus)
{
  m_system = system;
  ConnectIOPorts(bus);
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
    bool present = reader.ReadBool();
    if (!present)
    {
      m_drives[i].reset();
      continue;
    }

    m_drives[i] = std::make_unique<DriveState>();
    auto& drive = m_drives[i];
    drive->type = static_cast<DRIVE_TYPE>(reader.ReadUInt32());
    drive->num_cylinders = reader.ReadUInt32();
    drive->num_heads = reader.ReadUInt32();
    drive->num_sectors = reader.ReadUInt32();
    drive->num_lbas = reader.ReadUInt64();
    drive->current_num_cylinders = reader.ReadUInt32();
    drive->current_num_heads = reader.ReadUInt32();
    drive->current_num_sectors = reader.ReadUInt32();
    drive->current_cylinder = reader.ReadUInt32();
    drive->current_head = reader.ReadUInt32();
    drive->current_sector = reader.ReadUInt32();
    drive->current_lba = reader.ReadUInt64();
    drive->ata_sector_count = reader.ReadUInt16();
    drive->ata_sector_number = reader.ReadUInt16();
    drive->ata_cylinder_low = reader.ReadUInt16();
    drive->ata_cylinder_high = reader.ReadUInt16();
    drive->multiple_sectors = reader.ReadUInt16();
  }

  m_status_register = reader.ReadUInt8();
  m_busy_hold = reader.ReadUInt8();
  m_error_register = reader.ReadUInt8();
  m_control_register.bits = reader.ReadUInt8();
  m_drive_select.bits = reader.ReadUInt8();
  m_feature_select = reader.ReadUInt8();

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

  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    auto& drive = m_drives[i];
    if (!drive || drive->type != DRIVE_TYPE_HDD)
      continue;

    uint32 size = reader.ReadUInt32();
    drive->data.resize(size);
    if (size > 0)
      reader.ReadBytes(drive->data.data(), size);
  }

  return true;
}

bool HDC::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteUInt32(static_cast<uint32>(m_channel));

  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    auto& drive = m_drives[i];
    writer.WriteBool((drive.get() != nullptr));
    if (!drive)
      continue;

    writer.WriteUInt32(static_cast<uint32>(drive->type));
    writer.WriteUInt32(drive->num_cylinders);
    writer.WriteUInt32(drive->num_heads);
    writer.WriteUInt32(drive->num_sectors);
    writer.WriteUInt64(drive->num_lbas);
    writer.WriteUInt32(drive->current_num_cylinders);
    writer.WriteUInt32(drive->current_num_heads);
    writer.WriteUInt32(drive->current_num_sectors);
    writer.WriteUInt32(drive->current_cylinder);
    writer.WriteUInt32(drive->current_head);
    writer.WriteUInt32(drive->current_sector);
    writer.WriteUInt64(drive->current_lba);
    writer.WriteUInt16(drive->ata_sector_count);
    writer.WriteUInt16(drive->ata_sector_number);
    writer.WriteUInt16(drive->ata_cylinder_low);
    writer.WriteUInt16(drive->ata_cylinder_high);
    writer.WriteUInt16(drive->multiple_sectors);
  }

  writer.WriteUInt8(m_status_register);
  writer.WriteUInt8(m_error_register);
  writer.WriteUInt8(m_busy_hold);
  writer.WriteUInt8(m_control_register.bits);
  writer.WriteUInt8(m_drive_select.bits);
  writer.WriteUInt8(m_feature_select);

  writer.WriteUInt32(Truncate32(m_current_transfer.buffer.size()));
  if (!m_current_transfer.buffer.empty())
    writer.WriteBytes(m_current_transfer.buffer.data(), Truncate32(m_current_transfer.buffer.size()));

  writer.WriteUInt32(Truncate32(m_current_transfer.buffer_position));
  writer.WriteUInt32(m_current_transfer.drive_index);
  writer.WriteUInt32(m_current_transfer.sectors_per_block);
  writer.WriteUInt32(m_current_transfer.remaining_sectors);
  writer.WriteBool(m_current_transfer.is_write);

  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    auto& drive = m_drives[i];
    if (!drive || drive->type != DRIVE_TYPE_HDD)
      continue;

    uint32 size = Truncate32(drive->data.size());
    writer.WriteUInt32(size);
    if (size > 0)
      writer.WriteBytes(drive->data.data(), size);
  }

  return true;
}

uint32 HDC::GetDriveCount() const
{
  uint32 count = 0;
  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    // stop at the first null drive
    if (!m_drives[i])
      break;

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

bool HDC::AttachDrive(uint32 number, ByteStream* stream, uint32 cylinders /*= 0*/, uint32 heads /*= 0*/,
                      uint32 sectors /*= 0*/)
{
  DebugAssert(number < MAX_DRIVES);

  uint64 stream_size = stream->GetSize();
  if (cylinders == 0 || heads == 0 || sectors == 0)
    CalculateCHSForSize(&cylinders, &heads, &sectors, stream_size);

  if (stream_size != (cylinders * heads * sectors * SECTOR_SIZE))
  {
    Log_ErrorPrintf("CHS geometry does not match disk size: %u/%u/%u -> %u LBAs, real size %u LBAs", cylinders, heads,
                    sectors, cylinders * heads * sectors, uint32(stream_size / SECTOR_SIZE));
    return false;
  }

  std::unique_ptr<DriveState> drive_state = std::make_unique<DriveState>();
  drive_state->type = DRIVE_TYPE_HDD;
  drive_state->num_cylinders = cylinders;
  drive_state->num_heads = heads;
  drive_state->num_sectors = sectors;
  drive_state->num_lbas = cylinders * heads * sectors;
  drive_state->current_num_cylinders = cylinders;
  drive_state->current_num_heads = heads;
  drive_state->current_num_sectors = sectors;
  drive_state->data.resize(size_t(stream_size));

  if (!stream->SeekAbsolute(0) || !stream->Read2(drive_state->data.data(), uint32(stream_size)))
    return false;

  m_drives[number] = std::move(drive_state);
  return true;
}

bool HDC::AttachATAPIDevice(uint32 number, CDROM* cdrom)
{
  DebugAssert(number < MAX_DRIVES);

  std::unique_ptr<DriveState> drive_state = std::make_unique<DriveState>();
  drive_state->type = DRIVE_TYPE_ATAPI;
  drive_state->atapi_device = cdrom;
  m_drives[number] = std::move(drive_state);

  cdrom->SetCommandCompletedCallback(std::bind(&HDC::HandleATAPICommandCompleted, this, number));
  return true;
}

bool HDC::SeekDrive(uint32 drive, uint64 lba)
{
  DebugAssert(drive < MAX_DRIVES && m_drives[drive]);

  DriveState* state = m_drives[drive].get();
  if (lba >= state->num_lbas)
    return false;

  // TODO: Leave CHS unupdated for now?
  state->current_cylinder = 0;
  state->current_head = 0;
  state->current_sector = 0;
  state->current_lba = lba;
  m_status_register |= ATA_SR_DSC;
  return true;
}

bool HDC::SeekDrive(uint32 drive, uint32 cylinder, uint32 head, uint32 sector)
{
  DebugAssert(drive < MAX_DRIVES && m_drives[drive]);

  DriveState* state = m_drives[drive].get();
  if (cylinder >= state->current_num_cylinders || head >= state->current_num_heads || sector < 1 ||
      sector > state->current_num_sectors)
    return false;

  state->current_cylinder = cylinder;
  state->current_head = head;
  state->current_sector = sector;
  state->current_lba = (cylinder * state->current_num_heads + head) * state->current_num_sectors + (sector - 1);
  m_status_register |= ATA_SR_DSC;
  return true;
}

bool HDC::SeekToNextSector(uint32 drive)
{
  DriveState* state = m_drives[drive].get();
  if (state->current_sector == 0)
  {
    // using LBA
    if ((state->current_lba + 1) < state->num_lbas)
    {
      state->current_lba++;
      return true;
    }
    else
    {
      // end of disk
      return false;
    }
  }

  // CHS addressing
  uint32 cylinder = state->current_cylinder;
  uint32 head = state->current_head;
  uint32 sector = state->current_sector;

  // move sectors -> heads -> cylinders
  sector++;
  if (sector > state->current_num_sectors)
  {
    sector = 1;
    head++;
    if (head >= state->current_num_heads)
    {
      head = 0;
      cylinder++;
      if (cylinder >= state->current_num_cylinders)
      {
        // end of disk
        return false;
      }
    }
  }
  state->current_cylinder = cylinder;
  state->current_head = head;
  state->current_sector = sector;

  uint64 old_lba = state->current_lba;
  state->current_lba = (cylinder * state->current_num_heads + head) * state->current_num_sectors + (sector - 1);
  DebugAssert((old_lba + 1) == state->current_lba);
  return true;
}

void HDC::ReadCurrentSector(uint32 drive, void* data)
{
  DebugAssert(drive < MAX_DRIVES && m_drives[drive]);

  DriveState* state = m_drives[drive].get();
  Log_DevPrintf("HDC read lba %u offset %u", state->current_lba, state->current_lba * SECTOR_SIZE);

  std::memcpy(data, &state->data[state->current_lba * SECTOR_SIZE], SECTOR_SIZE);
}

void HDC::WriteCurrentSector(uint32 drive, const void* data)
{
  DebugAssert(drive < MAX_DRIVES && m_drives[drive]);

  DriveState* state = m_drives[drive].get();
  Log_DevPrintf("HDC write lba %u offset %u", state->current_lba, state->current_lba * SECTOR_SIZE);

  std::memcpy(&state->data[state->current_lba * SECTOR_SIZE], data, SECTOR_SIZE);
}

bool HDC::ReadSector(uint32 drive, uint32 cylinder, uint32 head, uint32 sector, void* data)
{
  if (!SeekDrive(drive, cylinder, head, sector))
    return false;

  ReadCurrentSector(drive, data);
  return true;
}

bool HDC::WriteSector(uint32 drive, uint32 cylinder, uint32 head, uint32 sector, const void* data)
{
  if (!SeekDrive(drive, cylinder, head, sector))
    return false;

  WriteCurrentSector(drive, data);
  return true;
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
  m_status_register = ATA_SR_DRDY | ATA_SR_DSC; /* | ATA_SR_IDX;*/
  m_error_register = 0;
  m_busy_hold = 0;
  Y_memzero(&m_drive_select, sizeof(m_drive_select));
  m_current_transfer.buffer.clear();
  m_current_transfer.buffer_position = 0;
  m_current_transfer.drive_index = MAX_DRIVES;
  m_current_transfer.remaining_sectors = 0;

  // Set signature bytes in current CHS
  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    DriveState* state = m_drives[i].get();
    if (!state)
      continue;

    state->current_num_cylinders = state->num_cylinders;
    state->current_num_heads = state->num_heads;
    state->current_num_sectors = state->num_sectors;
    SetSignature(state);
  }

  // TODO: Stop any ATAPI commands.
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
}

uint8 HDC::GetStatusRegisterValue()
{
  //     *value |= (0 << 7);     // Executing command
  //     *value |= (1 << 6);     // Drive is ready
  //     *value |= (0 << 5);     // Write fault
  //     *value |= (1 << 4);     // Seek complete
  //     *value |= (0 << 3);     // Sector buffer requires servicing
  //     *value |= (0 << 2);     // Disk data read successfully corrected
  //     *value |= (1 << 1);     // Index - set to 1 each revolution
  //     *value |= (0 << 0);     // Previous command ended in error

  // Clear busy bit. This probably should be done by a timer instead..
  if (m_busy_hold > 0)
    m_busy_hold--;
  if (m_busy_hold == 0)
  {
    m_status_register &= ~ATA_SR_BSY;
    m_status_register |= ATA_SR_DRDY;
  }

  uint8 value = m_status_register;

  // If BSY is set, don't return DRDY
  if (m_status_register & ATA_SR_BSY)
    value &= ~(ATA_SR_DRQ | ATA_SR_DRDY);

  return value;
}

void HDC::IOReadStatusRegister(uint8* value)
{
  // Lower interrupt
  m_system->GetInterruptController()->LowerInterrupt(m_irq_number);
  *value = GetStatusRegisterValue();
}

void HDC::IOReadAltStatusRegister(uint8* value)
{
  *value = GetStatusRegisterValue();
}

void HDC::IOWriteCommandRegister(uint8 value)
{
  HandleATACommand(value);
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
    m_status_register |= ATA_SR_BSY;
    m_status_register &= ~ATA_SR_DRDY;
    m_busy_hold = 4;
  }
}

void HDC::IOReadDataRegisterByte(uint8* value)
{
  if (!(m_status_register & ATA_SR_DRQ) || m_current_transfer.buffer_position == m_current_transfer.buffer.size())
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
  if (!(m_status_register & ATA_SR_DRQ) || m_current_transfer.buffer_position >= (m_current_transfer.buffer.size() - 1))
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
  if (!(m_status_register & ATA_SR_DRQ) || m_current_transfer.buffer_position >= (m_current_transfer.buffer.size() - 3))
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
  if (!(m_status_register & ATA_SR_DRQ) || m_current_transfer.buffer_position == m_current_transfer.buffer.size())
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
  if (!(m_status_register & ATA_SR_DRQ) || m_current_transfer.buffer_position >= (m_current_transfer.buffer.size() - 1))
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
  if (!(m_status_register & ATA_SR_DRQ) || m_current_transfer.buffer_position >= (m_current_transfer.buffer.size() - 3))
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
  // Lower 4 bits are the offset into the command block
  uint32 offset = port & 7;

  // TODO: Handle HOB
  DriveState* state = GetCurrentDrive();
  if (!state)
  {
    *value = 0;
    return;
  }

  bool hob = m_control_register.high_order_byte_readback;

  switch (offset)
  {
      // Number of sectors to read/write
    case 2:
      *value = Truncate8(hob ? (state->ata_sector_count >> 8) : (state->ata_sector_count));
      break;

      // Sector number/LBA0
    case 3:
      *value = Truncate8(hob ? (state->ata_sector_number >> 8) : (state->ata_sector_number));
      break;

      // Cylinder low/LBA1
    case 4:
      *value = Truncate8(hob ? (state->ata_cylinder_low >> 8) : (state->ata_cylinder_low));
      break;

      // Cylinder high/LBA2
    case 5:
      *value = Truncate8(hob ? (state->ata_cylinder_high >> 8) : (state->ata_cylinder_high));
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
  uint32 offset = port & 7;

  // We have to write the value to both drives
  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    auto& state = m_drives[i];
    if (!state)
      continue;

    switch (offset)
    {
      case 1:
        // Features
        m_feature_select = value;
        break;

        // Number of sectors to read/write
      case 2:
        state->ata_sector_count <<= 8;
        state->ata_sector_count |= ZeroExtend16(value);
        break;

        // Sector number/LBA0
        // Shift bits 0-7 to 24-31
      case 3:
        state->ata_sector_number <<= 8;
        state->ata_sector_number |= ZeroExtend16(value);
        break;

        // Cylinder low/LBA1
        // Shift bits 8-15 to 32-39
      case 4:
        state->ata_cylinder_low <<= 8;
        state->ata_cylinder_low |= ZeroExtend16(value);
        break;

        // Cylinder high/LBA2
        // Shift bits 16-23 to 40-47
      case 5:
        state->ata_cylinder_high <<= 8;
        state->ata_cylinder_high |= ZeroExtend16(value);
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
    DebugAssert(m_drives[drive_index] && m_drives[drive_index]->current_lba < m_drives[drive_index]->num_lbas);
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
    DebugAssert(m_drives[drive_index] && m_drives[drive_index]->current_lba < m_drives[drive_index]->num_lbas);
    WriteCurrentSector(drive_index, &m_current_transfer.buffer[i * SECTOR_SIZE]);
    if (!SeekToNextSector(drive_index) && (i != (sector_count - 1)))
      return false;
  }

  return true;
}

void HDC::HandleATACommand(uint8 command)
{
  Log_DevPrintf("Received ATA command 0x%02X", ZeroExtend32(command));

  DriveState* drive = GetCurrentDrive();
  if (!drive)
  {
    // Is this correct?
    AbortCommand();
    return;
  }

  switch (drive->type)
  {
    case DRIVE_TYPE_HDD:
    {
      switch (command)
      {
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
        case ATA_CMD_IDENTIFY_PACKET:
          HandleATAPIIdentify();
          break;

        case ATAPI_CMD_DEVICE_RESET:
          HandleATAPIDeviceReset();
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
  uint16 unused_5[16];                 // 84-99
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

  ATA_IDENTIFY_RESPONSE response = {};
  response.flags |= (1 << 10); // >10mbit/sec transfer speed
  response.flags |= (1 << 6);  // Fixed drive
  response.flags |= (1 << 2);  // Soft sectored
  response.cylinders = Truncate16(drive->num_cylinders);
  response.heads = Truncate16(drive->num_heads);
  response.unformatted_bytes_per_track = Truncate16(SECTOR_SIZE * drive->num_sectors);
  response.unformatted_bytes_per_sector = Truncate16(SECTOR_SIZE);
  response.sectors_per_track = Truncate16(drive->num_sectors);
  response.buffer_type = 3;
  response.buffer_size = 512; // 256KB cache
  response.ecc_bytes = 4;
  PutIdentifyString(response.serial_number, sizeof(response.serial_number), "DERP123");
  PutIdentifyString(response.firmware_revision, sizeof(response.firmware_revision), "HURR101");
  response.readwrite_multiple_supported = 16; // this is actually the number of sectors, tweak it
  response.dword_io_supported = 1;
  response.support |= (1 << 9); // LBA supported
  // response.support |= (1 << 8);       // DMA supported
  response.pio_timing_mode = 0x200;
  response.dma_timing_mode = 0x200;
  response.user_fields_valid = 0x07;
  response.user_cylinders = Truncate16(drive->current_num_cylinders);
  response.user_heads = Truncate16(drive->current_num_heads);
  response.user_sectors_per_track = Truncate16(drive->current_num_sectors);
  response.user_total_sectors =
    Truncate32(drive->current_num_cylinders * drive->current_num_heads * drive->current_num_sectors);
  response.lba_sectors = Truncate32(std::min(drive->num_lbas, ZeroExtend64(std::numeric_limits<uint32>::max())));
  PutIdentifyString(response.model, sizeof(response.model), "Herp derpity derp");
  // response.singleword_dma_modes = (1 << 0) | (1 << 8);
  // response.multiword_dma_modes = (1 << 0) | (1 << 8);
  for (size_t i = 0; i < countof(response.pio_cycle_time); i++)
    response.pio_cycle_time[i] = 120;
  response.word_80 = 0x7E;
  response.word_82 = (1 << 14);

  // 512 bytes total
  BeginTransfer(MAX_DRIVES, 1, 1, false);
  std::memcpy(m_current_transfer.buffer.data(), &response, sizeof(response));
}

void HDC::HandleATAPIIdentify()
{
  Log_DevPrintf("ATA identify packet drive %u", ZeroExtend32(GetCurrentDriveIndex()));
  DriveState* drive = GetCurrentDrive();
  DebugAssert(drive);

  ATA_IDENTIFY_RESPONSE response = {};
  response.flags |= (1 << 10); // >10mbit/sec transfer speed
  PutIdentifyString(response.serial_number, sizeof(response.serial_number), "DERP123");
  PutIdentifyString(response.firmware_revision, sizeof(response.firmware_revision), "HURR101");
  response.pio_timing_mode = 0x200;
  PutIdentifyString(response.model, sizeof(response.model), "DERP ATAPI CDROM");
  for (size_t i = 0; i < countof(response.pio_cycle_time); i++)
    response.pio_cycle_time[i] = 120;
  response.word_80 = 0x7E;
  response.word_82 = (1 << 14);

  // 512 bytes total
  BeginTransfer(MAX_DRIVES, 1, 1, false);
  std::memcpy(m_current_transfer.buffer.data(), &response, sizeof(response));

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
        ((drive->current_lba + ZeroExtend64(count)) > drive->num_lbas))
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
    if (!SeekDrive(drive_index, start_lba) || ((drive->current_lba + ZeroExtend64(count)) > drive->num_lbas))
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
        ((drive->current_lba + ZeroExtend64(count)) > drive->num_lbas) || count == 0)
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
    if (!SeekDrive(drive_index, start_lba) || ((drive->current_lba + ZeroExtend64(count)) > drive->num_lbas) ||
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
  uint32 num_cylinders = ZeroExtend32(drive->num_cylinders);
  uint32 num_heads = ZeroExtend32(m_drive_select.head.GetValue()) + 1;
  uint32 num_sectors = ZeroExtend32(drive->ata_sector_count & 0xFF);

  Log_DevPrintf("ATA initialize drive parameters drive=%u,heads=%u,sectors=%u", GetCurrentDriveIndex(), num_heads,
                num_sectors);

  // Abort the command if the translation is not possible.
  if ((num_cylinders * num_heads * num_sectors) > drive->num_lbas)
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
  Log_DevPrintf("ATA set features 0x%02X", ZeroExtend32(m_feature_select));

  switch (m_feature_select)
  {
    default:
      Log_ErrorPrintf("Unknown feature 0x%02X", ZeroExtend32(m_feature_select));
      AbortCommand();
      return;
  }
}

void HDC::HandleATAPIDeviceReset()
{
  DriveState* drive = GetCurrentDrive();
  DebugAssert(drive);

  Log_DevPrintf("ATAPI reset drive %u", ZeroExtend32(GetCurrentDriveIndex()));
  SetSignature(drive);

  // If the device is reset to its default state, ERR bit is set.
  // Signature bits are set
  CompleteCommand();
}

void HDC::HandleATAPIPacket()
{
  DriveState* drive = GetCurrentDrive();
  DebugAssert(drive);

  // Must be in PIO mode now
  if (m_feature_select != 0)
  {
    Log_ErrorPrintf("ATAPI dma requested");
    AbortCommand();
    return;
  }

  uint32 max_packet_size = ZeroExtend32(drive->ata_cylinder_low) | (ZeroExtend32(drive->ata_cylinder_high) << 8);
  Log_DevPrintf("ATAPI packet drive %u, packet size = %u", ZeroExtend32(GetCurrentDriveIndex()), max_packet_size);

  if (max_packet_size == 0 || drive->atapi_device->IsBusy())
  {
    AbortCommand();
    return;
  }

  StopTransfer();

  // The interrupt isn't raised here.
  m_status_register &= ~(ATA_SR_BSY);
  m_status_register |= ATA_SR_DRQ | ATA_SR_DRDY;
  m_error_register = 0;
  m_current_transfer.buffer.resize(max_packet_size);
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
  auto* device = m_drives[drive_index]->atapi_device;
  StopTransfer();

  // Was there an error?
  if (device->HasError() || device->GetDataSize() > m_current_transfer.buffer.size())
  {
    AbortCommand();
    return;
  }

  // No response?
  if (device->GetDataSize() == 0)
  {
    CompleteCommand();
    return;
  }

  // Set up the transfer.
  m_current_transfer.buffer.resize(device->GetDataSize());
  m_current_transfer.buffer_position = 0;
  m_current_transfer.drive_index = drive_index;
  m_current_transfer.sectors_per_block = 1;
  m_current_transfer.remaining_sectors = 1;
  m_current_transfer.is_write = false;
  m_current_transfer.is_packet_command = false;
  m_current_transfer.is_packet_data = true;

  // Copy data in.
  std::memcpy(m_current_transfer.buffer.data(), device->GetDataBuffer(), device->GetDataSize());
  device->ClearDataBuffer();

  // Clear the busy flag, and raise interrupt.
  m_status_register &= ~(ATA_SR_BSY);
  m_status_register |= ATA_SR_DRDY | ATA_SR_BSY;
  RaiseInterrupt();
}

void HDC::AbortCommand(uint8 error /* = ATA_ERR_ABRT */)
{
  Log_WarningPrintf("Command aborted with error = 0x%02X", ZeroExtend32(error));
  StopTransfer();

  m_status_register &= ~(ATA_SR_BSY | ATA_SR_DRQ);
  m_status_register |= ATA_SR_DRDY;
  m_status_register |= ATA_SR_ERR;
  m_error_register = error;
  RaiseInterrupt();
}

void HDC::CompleteCommand()
{
  StopTransfer();
  m_status_register &= ~(ATA_SR_BSY | ATA_SR_DRQ);
  m_status_register |= ATA_SR_DRDY;
  m_status_register &= ~ATA_SR_ERR;
  RaiseInterrupt();
}

void HDC::BeginTransfer(uint32 drive_index, uint32 sectors_per_block, uint32 num_sectors, bool is_write)
{
  Assert(num_sectors > 0);
  Assert(drive_index < MAX_DRIVES || num_sectors == 1);

  m_status_register &= ~ATA_SR_ERR;
  m_status_register |= ATA_SR_DRDY;
  m_status_register |= ATA_SR_DRQ;
  m_current_transfer.buffer.resize(SECTOR_SIZE * std::min(sectors_per_block, num_sectors));
  m_current_transfer.buffer_position = 0;
  m_current_transfer.drive_index = drive_index;
  m_current_transfer.sectors_per_block = sectors_per_block;
  m_current_transfer.remaining_sectors = num_sectors;
  m_current_transfer.is_write = is_write;
  m_current_transfer.is_packet_command = false;
  m_current_transfer.is_packet_data = false;

  // Busy flag?
  // m_status_register |= ATA_SR_BSY;
  // m_busy_hold = 4;

  // Raise interrupt if enabled
  RaiseInterrupt();
}

void HDC::UpdatePacketCommand(const void* data, size_t data_size)
{
  DebugAssert(m_current_transfer.is_packet_command && m_current_transfer.drive_index < MAX_DRIVES &&
              m_drives[m_current_transfer.drive_index]->type == DRIVE_TYPE_ATAPI);

  DriveState* drive = m_drives[m_current_transfer.drive_index].get();
  auto device = drive->atapi_device;
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
  m_status_register &= ~(ATA_SR_DRDY | ATA_SR_DRQ);
  m_status_register |= ATA_SR_BSY;
}

void HDC::UpdateTransferBuffer()
{
  if (m_current_transfer.buffer_position != m_current_transfer.buffer.size())
    return;

  // End of current sector
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
    CompleteCommand();
    return;
  }

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

  m_current_transfer.buffer_position = 0;
  m_status_register |= ATA_SR_DRDY;
  m_status_register |= ATA_SR_DRQ;

  // Raise interrupt if enabled
  RaiseInterrupt();
}

void HDC::RaiseInterrupt()
{
  if (!m_control_register.disable_interrupts)
  {
    Log_DevPrintf("Raising HDD interrupt");
    m_system->GetInterruptController()->RaiseInterrupt(m_irq_number);
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

} // namespace HW