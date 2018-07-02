#include "pce/hw/fdc.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "pce/bus.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cstring>
Log_SetChannel(HW::FDC);

#pragma pack(push, 1)
struct FAT_HEADER
{
  uint8 bootstrap_jump[3];
  char oem_name[8];
  uint16 bytes_per_sector;
  uint8 sectors_per_cluster;
  uint16 num_reserved_sectors;
  uint8 num_fat_copies;
  uint16 num_root_directory_entries;
  uint16 num_sectors;
  uint8 media_descriptor_type;
  uint16 num_sectors_per_fat;
  uint16 num_sectors_per_track;
  uint16 num_heads;
  uint16 num_hidden_sectors;
  uint8 bootloader_code[480];
  uint16 signature;
};
static_assert(sizeof(FAT_HEADER) == 512, "FAT header is 512 bytes");
#pragma pack(pop)

struct DiskTypeInfo
{
  HW::FDC::DriveType drive_type;
  HW::FDC::DiskType disk_type;
  uint32 size;
  uint32 num_tracks;
  uint32 num_heads;
  uint32 num_sectors_per_track;
  uint8 media_descriptor_byte;
};
static const DiskTypeInfo disk_types[] = {
  {HW::FDC::DriveType_5_25, HW::FDC::DiskType_160K, 163840, 40, 1, 8, 0xFE},    // DiskType_160K
  {HW::FDC::DriveType_5_25, HW::FDC::DiskType_180K, 184320, 40, 1, 9, 0xFC},    // DiskType_180K
  {HW::FDC::DriveType_5_25, HW::FDC::DiskType_320K, 327680, 40, 2, 8, 0xFF},    // DiskType_320K
  {HW::FDC::DriveType_5_25, HW::FDC::DiskType_360K, 368640, 40, 2, 9, 0xFD},    // DiskType_360K
  {HW::FDC::DriveType_5_25, HW::FDC::DiskType_640K, 655360, 80, 2, 8, 0xFB},    // DiskType_640K
  {HW::FDC::DriveType_3_5, HW::FDC::DiskType_720K, 737280, 80, 2, 9, 0xF9},     // DiskType_720K
  {HW::FDC::DriveType_5_25, HW::FDC::DiskType_1220K, 1310720, 80, 2, 15, 0xF9}, // DiskType_1220K
  {HW::FDC::DriveType_3_5, HW::FDC::DiskType_1440K, 1474560, 80, 2, 18, 0xF0},  // DiskType_1440K
  {HW::FDC::DriveType_3_5, HW::FDC::DiskType_1680K, 1720320, 80, 2, 21, 0xF0},  // DiskType_1680K
  {HW::FDC::DriveType_3_5, HW::FDC::DiskType_2880K, 2949120, 80, 2, 36, 0xF0},  // DiskType_2880K
};

// Data rates in kb/s.
static const uint32 data_rates[4] = {500, 300, 250, 1000};

namespace HW {

FDC::DiskType FDC::DetectDiskType(ByteStream* pStream)
{
  // Get file size
  uint32 file_size = static_cast<uint32>(pStream->GetSize());
  Log_DevPrintf("Disk size: %u bytes", file_size);

  // Check for FAT header
  FAT_HEADER fat_header;
  if (!pStream->SeekAbsolute(0) || !pStream->Read2(&fat_header, sizeof(fat_header)))
    return DiskType_None;

  // Validate FAT header
  if (fat_header.signature == 0xAA55)
  {
    Log_DevPrintf("FAT detected, media descriptor = 0x%02X", fat_header.media_descriptor_type);

    // Use media descriptor byte to find a matching type
    for (size_t i = 0; i < countof(disk_types); i++)
    {
      if (fat_header.media_descriptor_type == disk_types[i].media_descriptor_byte && file_size <= disk_types[i].size)
      {
        return disk_types[i].disk_type;
      }
    }
  }

  // Use size alone to find a matching type
  for (size_t i = 0; i < countof(disk_types); i++)
  {
    if (file_size <= disk_types[i].size)
      return disk_types[i].disk_type;
  }

  // Unknown
  Log_ErrorPrintf("Unable to determine disk type for size %u", file_size);
  return DiskType_None;
}

FDC::DriveType FDC::GetDriveTypeForDiskType(DiskType type)
{
  for (size_t i = 0; i < countof(disk_types); i++)
  {
    if (disk_types[i].disk_type == type)
      return disk_types[i].drive_type;
  }

  return DriveType_None;
}

FDC::FDC(DMAController* dma) : m_dma(dma), m_clock("Floppy Controller", CLOCK_FREQUENCY) {}

FDC::~FDC() {}

void FDC::Initialize(System* system, Bus* bus)
{
  m_system = system;
  m_clock.SetManager(system->GetTimingManager());
  ConnectIOPorts(bus);

  m_command_event = m_clock.NewEvent("Floppy Command", 1, std::bind(&FDC::EndCommand, this), false);
}

void FDC::Reset()
{
  m_main_status_register.ClearActivity();
  m_main_status_register.command_busy = false;
  if (m_command_event->IsActive())
    m_command_event->Deactivate();

  TransitionToCommandPhase();

  // Interrupts start enabled always.
  m_interrupt_enable = true;
  LowerInterrupt();

  m_motor_on.fill(false);
  m_nreset = false;

  m_step_rate_time = 0;
  m_head_load_time = 0;
  m_head_unload_time = 0;
  m_pio_mode = false;

  m_data_rate_index = 0;

  // Abort any transfers
  if (m_current_transfer.active)
  {
    m_current_transfer.active = false;
    m_dma->SetDMAState(DMA_CHANNEL, false);
  }

  // Reset disk states
  for (DriveState& drive : m_drives)
  {
    drive.data_was_read = false;
    drive.data_was_written = false;
    drive.direction = false;
    drive.step_latch = false;
  }
}

bool FDC::LoadState(BinaryReader& reader)
{
  uint32 magic;
  if (!reader.SafeReadUInt32(&magic) || magic != SERIALIZATION_ID)
    return false;

  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    DriveState* drive = &m_drives[i];
    uint32 drive_type = 0, disk_type = 0;
    reader.SafeReadUInt32(&drive_type);
    reader.SafeReadUInt32(&disk_type);
    drive->drive_type = static_cast<DriveType>(drive_type);
    drive->disk_type = static_cast<DiskType>(disk_type);
    reader.SafeReadUInt32(&drive->num_cylinders);
    reader.SafeReadUInt32(&drive->num_heads);
    reader.SafeReadUInt32(&drive->num_sectors);
    reader.SafeReadUInt32(&drive->current_cylinder);
    reader.SafeReadUInt32(&drive->current_head);
    reader.SafeReadUInt32(&drive->current_sector);
    reader.SafeReadUInt32(&drive->current_lba);
    reader.SafeReadBool(&drive->data_was_read);
    reader.SafeReadBool(&drive->data_was_written);
    reader.SafeReadBool(&drive->step_latch);
    reader.SafeReadBool(&drive->direction);
  }

  reader.SafeReadUInt8(&m_current_drive);
  for (size_t i = 0; i < m_motor_on.size(); i++)
    reader.SafeReadBool(&m_motor_on[i]);
  reader.SafeReadBool(&m_interrupt_enable);
  reader.SafeReadBool(&m_interrupt_pending);
  reader.SafeReadBool(&m_nreset);

  reader.SafeReadUInt8(&m_main_status_register.bits);
  reader.SafeReadUInt8(&m_data_rate_index);
  reader.SafeReadBytes(m_fifo.data(), Truncate32(m_fifo.size()));
  reader.SafeReadUInt32(&m_fifo_command_position);
  reader.SafeReadUInt32(&m_fifo_result_size);
  reader.SafeReadUInt32(&m_fifo_result_position);
  reader.SafeReadUInt8(&m_reset_sense_interrupt_count);
  reader.SafeReadUInt8(&m_current_drive);
  reader.SafeReadBool(&m_interrupt_pending);
  reader.SafeReadUInt8(&m_reset_sense_interrupt_count);
  reader.SafeReadUInt8(&m_step_rate_time);
  reader.SafeReadUInt8(&m_head_load_time);
  reader.SafeReadUInt8(&m_head_unload_time);
  reader.SafeReadBool(&m_pio_mode);
  reader.SafeReadBool(&m_current_transfer.active);
  reader.SafeReadBool(&m_current_transfer.multi_track);
  reader.SafeReadUInt8(&m_current_transfer.drive);
  reader.SafeReadUInt32(&m_current_transfer.bytes_per_sector);
  reader.SafeReadUInt32(&m_current_transfer.sectors_per_track);
  reader.SafeReadUInt32(&m_current_transfer.sector_offset);
  reader.SafeReadBytes(m_current_transfer.sector_buffer, sizeof(m_current_transfer.sector_buffer));
  reader.SafeReadUInt8(&m_st0);
  reader.SafeReadUInt8(&m_st1);
  reader.SafeReadUInt8(&m_st2);

  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    DriveState* drive = &m_drives[i];
    uint32 size = 0;

    reader.SafeReadBool(&drive->write_protect);
    reader.SafeReadUInt32(&size);
    drive->data.Resize(size);
    if (size > 0)
      reader.SafeReadBytes(drive->data.GetBasePointer(), size);
  }

  return !reader.GetErrorState();
}

bool FDC::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    DriveState* drive = &m_drives[i];
    writer.SafeWriteUInt32(static_cast<uint32>(drive->drive_type));
    writer.SafeWriteUInt32(static_cast<uint32>(drive->disk_type));
    writer.SafeWriteUInt32(drive->num_cylinders);
    writer.SafeWriteUInt32(drive->num_heads);
    writer.SafeWriteUInt32(drive->num_sectors);
    writer.SafeWriteUInt32(drive->current_cylinder);
    writer.SafeWriteUInt32(drive->current_head);
    writer.SafeWriteUInt32(drive->current_sector);
    writer.SafeWriteUInt32(drive->current_lba);
    writer.SafeWriteBool(drive->data_was_read);
    writer.SafeWriteBool(drive->data_was_written);
    writer.SafeWriteBool(drive->step_latch);
    writer.SafeWriteBool(drive->direction);
  }

  writer.SafeWriteUInt8(m_current_drive);
  for (size_t i = 0; i < m_motor_on.size(); i++)
    writer.SafeWriteBool(m_motor_on[i]);
  writer.SafeWriteBool(m_interrupt_enable);
  writer.SafeWriteBool(m_interrupt_pending);
  writer.SafeWriteBool(m_nreset);

  writer.SafeWriteUInt8(m_main_status_register.bits);
  writer.SafeWriteUInt8(m_data_rate_index);
  writer.SafeWriteBytes(m_fifo.data(), Truncate32(m_fifo.size()));
  writer.SafeWriteUInt32(m_fifo_command_position);
  writer.SafeWriteUInt32(m_fifo_result_size);
  writer.SafeWriteUInt32(m_fifo_result_position);
  writer.SafeWriteUInt8(m_reset_sense_interrupt_count);
  writer.SafeWriteUInt8(m_current_drive);
  writer.SafeWriteBool(m_interrupt_pending);
  writer.SafeWriteUInt8(m_reset_sense_interrupt_count);
  writer.SafeWriteUInt8(m_step_rate_time);
  writer.SafeWriteUInt8(m_head_load_time);
  writer.SafeWriteUInt8(m_head_unload_time);
  writer.SafeWriteBool(m_pio_mode);
  writer.SafeWriteBool(m_current_transfer.active);
  writer.SafeWriteBool(m_current_transfer.multi_track);
  writer.SafeWriteUInt8(m_current_transfer.drive);
  writer.SafeWriteUInt32(m_current_transfer.bytes_per_sector);
  writer.SafeWriteUInt32(m_current_transfer.sectors_per_track);
  writer.SafeWriteUInt32(m_current_transfer.sector_offset);
  writer.SafeWriteBytes(m_current_transfer.sector_buffer, sizeof(m_current_transfer.sector_buffer));
  writer.SafeWriteUInt8(m_st0);
  writer.SafeWriteUInt8(m_st1);
  writer.SafeWriteUInt8(m_st2);

  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    DriveState* drive = &m_drives[i];
    uint32 size = Truncate32(drive->data.GetSize());
    writer.SafeWriteBool(drive->write_protect);
    writer.SafeWriteUInt32(size);
    if (size > 0)
      writer.SafeWriteBytes(drive->data.GetBasePointer(), size);
  }

  return true;
}

void FDC::SetDriveType(uint32 drive, DriveType type)
{
  DebugAssert(drive < MAX_DRIVES);
  m_drives[drive].drive_type = type;
}

uint32 FDC::GetDriveCount() const
{
  uint32 count = 0;
  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    if (IsDrivePresent(i))
      count++;
  }
  return count;
}

bool FDC::InsertDisk(uint32 drive, DiskType type, ByteStream* pStream)
{
  RemoveDisk(drive);
  if (!pStream)
    return true;

  if (type == DiskType_AutoDetect)
  {
    type = DetectDiskType(pStream);
    if (type == DiskType_None)
      return false;
  }

  const DiskTypeInfo* info = nullptr;
  for (size_t i = 0; i < countof(disk_types); i++)
  {
    if (disk_types[i].disk_type == type)
    {
      info = &disk_types[i];
      break;
    }
  }
  if (!info)
    return false;

  DriveState* state = &m_drives[drive];
  if (state->drive_type != info->drive_type)
  {
    Log_WarningPrintf("Changing drive %u type from %u to %u", drive, uint32(state->drive_type),
                      uint32(info->drive_type));
  }

  PODArray<uint8> data;
  uint32 image_size = static_cast<uint32>(pStream->GetSize());
  data.Resize(image_size);
  if (!pStream->SeekAbsolute(0) || !pStream->Read2(data.GetBasePointer(), image_size))
    return false;

  // Allocate extra data when the image is smaller?
  if (info->size < image_size)
  {
    data.Resize(info->size);
    Y_memzero(data.GetBasePointer() + image_size, info->size - image_size);
  }

  state->disk_type = type;
  state->drive_type = info->drive_type;
  state->num_cylinders = info->num_tracks;
  state->num_heads = info->num_heads;
  state->num_sectors = info->num_sectors_per_track;
  state->data.Swap(data);
  Log_InfoPrintf("Disk %u inserted: CHS %u/%u/%u", drive, info->num_tracks, info->num_heads,
                 info->num_sectors_per_track);
  return true;
}

void FDC::RemoveDisk(uint32 drive)
{
  DriveState* state = &m_drives[drive];
  state->disk_type = DiskType_None;
  state->num_cylinders = 0;
  state->num_heads = 0;
  state->num_sectors = 0;
  state->current_cylinder = 0;
  state->current_head = 0;
  state->current_sector = 0;
  state->current_lba = 0;
  state->data.Obliterate();
}

void FDC::ClearFIFO()
{
  m_fifo_command_position = 0;
  m_fifo_result_position = 0;
  m_fifo_result_size = 0;
}

void FDC::WriteToFIFO(uint8 value)
{
  Assert(m_fifo_result_size < FIFO_SIZE);
  m_fifo[m_fifo_result_size++] = value;
}

bool FDC::SeekDrive(uint32 drive, uint32 cylinder, uint32 head, uint32 sector)
{
  DriveState* state = &m_drives[drive];
  if (cylinder >= state->num_cylinders || head >= state->num_heads || sector < 1 || sector > state->num_sectors)
    return false;

  state->current_cylinder = cylinder;
  state->current_head = head;
  state->current_sector = sector;
  state->current_lba = (cylinder * state->num_heads + head) * state->num_sectors + (sector - 1);
  return true;
}

bool FDC::SeekToNextSector(uint32 drive)
{
  DriveState* state = &m_drives[drive];

  // CHS addressing
  uint32 cylinder = state->current_cylinder;
  uint32 head = state->current_head;
  uint32 sector = state->current_sector;

  // move sectors -> heads -> cylinders
  sector++;
  if (sector > state->num_sectors)
  {
    sector = 1;
    head++;
    if (head >= state->num_heads)
    {
      head = 0;
      cylinder++;
      if (cylinder >= state->num_cylinders)
      {
        // end of disk
        return false;
      }
    }
  }
  state->current_cylinder = cylinder;
  state->current_head = head;
  state->current_sector = sector;

  uint32 old_lba = state->current_lba;
  state->current_lba = (cylinder * state->num_heads + head) * state->num_sectors + (sector - 1);
  DebugAssert((old_lba + 1) == state->current_lba);
  return true;
}

void FDC::ReadCurrentSector(uint32 drive, void* data)
{
  DriveState* state = &m_drives[drive];
  Log_DevPrintf("FDC read lba %u offset %u", state->current_lba, state->current_lba * SECTOR_SIZE);

  std::memcpy(data, &state->data[state->current_lba * SECTOR_SIZE], SECTOR_SIZE);
}

void FDC::WriteCurrentSector(uint32 drive, const void* data)
{
  DriveState* state = &m_drives[drive];
  Log_DevPrintf("FDC write lba %u offset %u", state->current_lba, state->current_lba * SECTOR_SIZE);

  std::memcpy(&state->data[state->current_lba * SECTOR_SIZE], data, SECTOR_SIZE);
}

bool FDC::ReadSector(uint32 drive, uint32 cylinder, uint32 head, uint32 sector, void* data)
{
  if (!SeekDrive(drive, cylinder, head, sector))
    return false;

  ReadCurrentSector(drive, data);
  return true;
}

bool FDC::WriteSector(uint32 drive, uint32 cylinder, uint32 head, uint32 sector, const void* data)
{
  if (!SeekDrive(drive, cylinder, head, sector))
    return false;

  WriteCurrentSector(drive, data);
  return true;
}

//////////////////////////////////////////////////////////////////////////
// LLE stuff
//////////////////////////////////////////////////////////////////////////

void FDC::SoftReset()
{
  ClearFIFO();

  // Reset status register
  m_main_status_register.command_busy = false;
  m_main_status_register.ClearActivity();
  if (m_command_event->IsActive())
    m_command_event->Deactivate();
  TransitionToCommandPhase();

  // Reset disk states
  for (DriveState& drive : m_drives)
  {
    drive.data_was_read = false;
    drive.data_was_written = false;
    drive.direction = false;
    drive.step_latch = false;
  }

  // Cancel any DMA requests
  if (m_current_transfer.active)
  {
    m_current_transfer.active = false;
    m_dma->SetDMAState(DMA_CHANNEL, false);
  }

  // Reset interrupt line
  LowerInterrupt();
  RaiseInterrupt();
  m_reset_sense_interrupt_count = 4;
  // m_reset_sense_interrupt_count = 0;
}

uint8 FDC::GetCurrentCommandLength()
{
  uint8 command = m_fifo[0] & 0x1F;

  switch (command)
  {
    case 0x03: // Specify
      return 3;
    case 0x05: // Write Data
    case 0x06: // Read Data
      return 9;
    case 0x07: // Recalibrate
      return 2;
    case 0x08: // Sense Interrupt
      return 1;
    case 0x0A: // Read ID
      return 2;
    case 0x0F: // Seek
      return 3;
    default:
      return 1;
  }
}

enum Command : uint8
{
  CMD_SPECIFY = 0x03,
  CMD_SENSE_STATUS = 0x04,
  CMD_WRITE_DATA = 0x05,
  CMD_READ_DATA = 0x06,
  CMD_RECALIBRATE = 0x07,
  CMD_SENSE_INTERRUPT = 0x08,
  CMD_READ_ID = 0x0A,
  CMD_SEEK = 0x0F,

  // Enhanced drive
  CMD_DUMP_REGISTERS = 0x0E,
  CMD_VERSION = 0x10,
  CMD_UNLOCK = 0x14,
  CMD_LOCK = 0x94,
};

void FDC::BeginCommand()
{
  uint8 command = m_fifo[0] & 0x1F;
  bool sk = !!((m_fifo[0] >> 5) & 0x01);
  bool mf = !!((m_fifo[0] >> 6) & 0x01);
  bool mt = !!((m_fifo[0] >> 7) & 0x01);
  Log_DevPrintf("Floppy command 0x%02X MT=%u,MF=%u,SK=%u", uint32(command), uint32(mt), uint32(mf), uint32(sk));

  switch (command)
  {
    case CMD_SPECIFY: // Specify
    {
      DebugAssert(m_fifo_command_position >= 3);

      m_step_rate_time = (m_fifo[1] >> 4) & 0b1111;
      m_head_unload_time = (m_fifo[1]) & 0b1111;
      m_head_load_time = (m_fifo[2] >> 1) & 0b1111;
      m_pio_mode = !!(m_fifo[2] & 0b1);

      // No result bytes or interrupt
      TransitionToCommandPhase();
    }
    break;

    case CMD_WRITE_DATA: // Write Data
    case CMD_READ_DATA:  // Read Data
    {
      DebugAssert(m_fifo_command_position >= 9);

      // Update drive number
      m_current_drive = m_fifo[1] & 0x03;
      if (!IsDrivePresent(m_current_drive) || !IsDiskPresent(m_current_drive))
      {
        Log_DevPrintf("Write: drive/disk not present");
        HangController();
        return;
      }

      bool is_write = (command == 0x05);
      uint8 head_number2 = (m_fifo[1] >> 2);
      uint8 cylinder_number = (m_fifo[2]);
      uint8 head_number = (m_fifo[3]);
      uint8 sector_number = (m_fifo[4]);
      uint8 sector_type = (m_fifo[5]);
      uint8 end_of_track = (m_fifo[6]);
      // uint8 gap1_size = (m_fifo[7]);
      uint8 sector_type2 = (m_fifo[8]);

      // Header number in bit2 should equal head_number
      if (head_number2 != head_number)
      {
        EndTransfer(m_current_drive, ST0_IC_AT, ST1_ND, 0);
        return;
      }

      // TODO: Properly validate ranges
      DebugAssert(sector_type == 0x02);
      DebugAssert(cylinder_number < m_drives[m_current_drive].num_cylinders);
      DebugAssert(sector_number <= m_drives[m_current_drive].num_sectors);
      if (sector_type == 0)
      {
        // sector type 2 means the number of bytes is specified in the last command byte
        m_current_transfer.bytes_per_sector = sector_type2;
      }
      else
      {
        m_current_transfer.bytes_per_sector = 128 << sector_type;
      }

      m_current_transfer.active = true;
      m_current_transfer.multi_track = mt;
      m_current_transfer.is_write = is_write;
      m_current_transfer.sectors_per_track = end_of_track;
      m_current_transfer.drive = m_current_drive;
      m_current_transfer.sector_offset = 0;

      // TODO: Non-dma transfer
      m_main_status_register.pio_mode = false;

      Log_DevPrintf("Floppy start %s %u/%u/%u", is_write ? "write" : "read", cylinder_number, head_number,
                    sector_number);

      // Clear RFM bit and wait for transfer to finish
      m_main_status_register.request_for_master = false;
      m_main_status_register.data_direction = !is_write;
      m_main_status_register.command_busy = true;

      // Already on the correct track?
      if (m_drives[m_current_drive].current_cylinder != cylinder_number)
      {
        // We need to seek to the correct sector.
        CycleCount seek_time = CalculateHeadSeekTime(m_drives[m_current_drive].current_cylinder, cylinder_number);
        SeekDrive(m_current_drive, cylinder_number, head_number, sector_number);
        m_command_event->Queue(seek_time);
      }
      else
      {
        // Start reading sectors. Still need to seek to the starting sector.
        SeekDrive(m_current_drive, cylinder_number, head_number, sector_number);
        m_command_event->Queue(CalculateSectorReadTime());
      }
    }
    break;

    case CMD_RECALIBRATE: // Recalibrate
    {
      DebugAssert(m_fifo_command_position >= 2);

      uint8 drive = m_fifo[1] & 0x03;
      Log_DevPrintf("Recalibrate drive %u", ZeroExtend32(drive));

      // TODO: Set errors
      if (!IsDrivePresent(drive))
      {
        HangController();
        return;
      }

      // Calculate time to seek.
      CycleCount seek_time = CalculateHeadSeekTime(m_drives[drive].current_cylinder, 0);
      m_main_status_register.command_busy = true;
      m_main_status_register.SetActivity(drive);
      m_command_event->Queue(seek_time);
    }
    break;

    case CMD_SENSE_INTERRUPT: // Sense interrupt
    {
      ClearFIFO();

      if (m_reset_sense_interrupt_count > 0)
      {
        WriteToFIFO(0xC0 | (4 - m_reset_sense_interrupt_count));
        WriteToFIFO(Truncate8(m_drives[m_current_drive].current_cylinder));

        TransitionToResultPhase();
        m_reset_sense_interrupt_count--;
      }
      else
      {
        WriteToFIFO(m_st0 | (m_interrupt_pending ? ST0_IC_NT : ST0_IC_IC));
        WriteToFIFO(Truncate8(m_drives[m_current_drive].current_cylinder));

        TransitionToResultPhase();
      }
    }
    break;

    case CMD_SENSE_STATUS: // Get Status
    {
      uint8 drive_number = m_fifo[1] & 0x03;
      uint8 head = (m_fifo[1] >> 2) & 0x01;
      SeekDrive(drive_number, m_drives[drive_number].current_cylinder, head, m_drives[drive_number].current_sector);

      // Writes ST3 to the result, but does not raise an interrupt.
      ClearFIFO();
      WriteToFIFO(GetST3(drive_number, 0));
      TransitionToResultPhase();
    }
    break;

    case CMD_READ_ID: // Read ID
    {
      DebugAssert(m_fifo_command_position >= 2);

      m_current_drive = m_fifo[1] & 0x03;
      uint8 head = (m_fifo[1] >> 2) & 0x01;

      if (!m_motor_on[m_current_drive])
      {
        Log_DevPrintf("Motor not on");
        HangController();
        return;
      }
      if (!IsDrivePresent(m_current_drive))
      {
        Log_DevPrintf("Invalid drive number");
        HangController();
        return;
      }
      if (!IsDiskPresent(m_current_drive))
      {
        Log_DevPrintf("Media not present");
        HangController();
        return;
      }

      // TODO: Check data rate
      if (!SeekDrive(m_current_drive, m_drives[m_current_drive].current_cylinder, head,
                     m_drives[m_current_drive].current_sector))
      {
        // Head not valid.
        EndTransfer(m_current_drive, ST0_IC_AT, ST1_MA, 0);
        return;
      }

      m_main_status_register.command_busy = true;
      m_main_status_register.SetActivity(m_current_drive);
      m_command_event->Queue(CalculateSectorReadTime());
    }
    break;

    case CMD_SEEK: // Seek
    {
      DebugAssert(m_fifo_command_position >= 3);

      uint8 drive_number = (m_fifo[1] & 0b11);
      uint8 head_number = (m_fifo[1] >> 2);
      uint8 cylinder_number = (m_fifo[2]);
      Log_DevPrintf("Floppy seek drive %u head %u cylinder %u", drive_number, head_number, cylinder_number);

      m_current_drive = drive_number;
      if (!IsDrivePresent(drive_number) || !IsDiskPresent(drive_number))
      {
        HangController();
        return;
      }

      // Calculate time to seek.
      CycleCount seek_time = CalculateHeadSeekTime(m_drives[drive_number].current_cylinder, cylinder_number);
      m_main_status_register.command_busy = true;
      m_main_status_register.SetActivity(drive_number);
      m_command_event->Queue(seek_time);
    }
    break;

    default:
      Log_WarningPrintf("Unknown floppy command 0x%02X, MT=%u, MF=%u, SK=%u", uint32(command), uint32(mt), uint32(mf),
                        uint32(sk));
      break;
  }
}

void FDC::EndCommand()
{
  const uint8 command = m_fifo[0] & 0x1F;
  switch (command)
  {
    case CMD_RECALIBRATE:
    {
      const uint8 drive_number = m_fifo[1] & 0x03;
      SeekDrive(drive_number, 0, 0, 1);
      m_st0 = ST0_SE;

      // No result bytes, but sends interrupt
      m_command_event->Deactivate();
      TransitionToCommandPhase();
      RaiseInterrupt();
    }
    break;

    case CMD_SEEK: // Seek
    {
      DebugAssert(m_fifo_command_position >= 3);

      uint8 drive_number = (m_fifo[1] & 0b11);
      uint8 head_number = (m_fifo[1] >> 2);
      uint8 cylinder_number = (m_fifo[2]);
      SeekDrive(drive_number, cylinder_number, head_number, 1);
      m_st0 = ST0_SE;

      // No result bytes, but sends interrupt
      m_command_event->Deactivate();
      TransitionToCommandPhase();
      RaiseInterrupt();
    }
    break;

    case CMD_READ_ID:
    {
      // EndTransfer will cancel the event.
      EndTransfer(m_current_drive, ST0_IC_NT, 0, 0);
    }
    break;

    case CMD_READ_DATA:
    case CMD_WRITE_DATA:
    {
      // Everything is already set up, we just need to kick the DMA transfer.
      // We leave the event active for the next sector.
      m_dma->SetDMAState(DMA_CHANNEL, true);
      m_command_event->Reschedule(CalculateSectorReadTime());
    }
    break;
  }
}

void FDC::HangController()
{
  m_main_status_register.ClearActivity();
  m_main_status_register.command_busy = true;
  m_main_status_register.data_direction = false;
  m_main_status_register.request_for_master = false;

  if (m_command_event->IsActive())
    m_command_event->Deactivate();

  if (m_current_transfer.active)
  {
    m_current_transfer.active = false;
    m_dma->SetDMAState(DMA_CHANNEL, false);
  }
}

void FDC::TransitionToCommandPhase()
{
  ClearFIFO();
  LowerInterrupt();

  m_main_status_register.command_busy = false;
  m_main_status_register.data_direction = false;
  m_main_status_register.request_for_master = true;
  m_main_status_register.ClearActivity();
}

void FDC::TransitionToResultPhase()
{
  m_main_status_register.command_busy = false;
  m_main_status_register.data_direction = true;
  m_main_status_register.request_for_master = true;
  m_main_status_register.ClearActivity();
}

void FDC::ConnectIOPorts(Bus* bus)
{
  bus->ConnectIOPortRead(0x03F0, this, std::bind(&FDC::IOReadStatusRegisterA, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x03F1, this, std::bind(&FDC::IOReadStatusRegisterB, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x03F2, this, std::bind(&FDC::IOReadDigitalOutputRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x03F2, this, std::bind(&FDC::IOWriteDigitalOutputRegister, this, std::placeholders::_2));
  bus->ConnectIOPortReadToPointer(0x03F4, this, &m_main_status_register.bits);
  bus->ConnectIOPortWrite(0x03F4, this, std::bind(&FDC::IOWriteDataRateSelectRegister, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x03F5, this, std::bind(&FDC::IOReadFIFO, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x03F5, this, std::bind(&FDC::IOWriteFIFO, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x03F7, this, std::bind(&FDC::IOReadDigitalInputRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x03F7, this,
                          std::bind(&FDC::IOWriteConfigurationControlRegister, this, std::placeholders::_2));

  // connect DMA channel
  if (m_dma)
  {
    m_dma->ConnectDMAChannel(
      DMA_CHANNEL,
      std::bind(&FDC::DMAReadCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
      std::bind(&FDC::DMAWriteCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  }
}

void FDC::RaiseInterrupt()
{
  m_interrupt_pending = true;
  m_reset_sense_interrupt_count = 0;
  if (m_interrupt_enable)
    m_system->GetInterruptController()->RaiseInterrupt(6);
}

void FDC::LowerInterrupt()
{
  m_interrupt_pending = false;
  m_system->GetInterruptController()->LowerInterrupt(6);
}

void FDC::IOReadStatusRegisterA(uint8* value)
{
  // 0x80 - interrupt pending
  // 0x40 - dma request
  // 0x20 - step latch - set on seek/recalibrate, cleared on digital input register read or reset
  // 0x10 - track 0
  // 0x08 - head 0
  // 0x04 - index - sector == 0
  // 0x02 - write protect
  // 0x01 - direction == 0
  *value = (BoolToUInt8(m_interrupt_pending) << 7) | (BoolToUInt8(m_dma->GetDMAState(DMA_CHANNEL)) << 6) |
           (BoolToUInt8(m_drives[m_current_drive].step_latch) << 5) |
           (BoolToUInt8(m_drives[m_current_drive].current_cylinder == 0) << 4) |
           (BoolToUInt8(m_drives[m_current_drive].current_head == 0) << 3) |
           (BoolToUInt8(m_drives[m_current_drive].current_sector == 0) << 2) |
           (BoolToUInt8(m_drives[m_current_drive].write_protect) << 1) |
           (BoolToUInt8(m_drives[m_current_drive].direction) << 0);
}

void FDC::IOReadStatusRegisterB(uint8* value)
{
  // 0x80 - no second drive
  // 0x40 - not ds1
  // 0x20 - not ds0
  // 0x10 - data written
  // 0x08 - data read
  // 0x04 - data written again
  // 0x02 - not ds3
  // 0x01 - not ds2
  *value = (BoolToUInt8(m_drives[1].drive_type == DriveType_None) << 7) | (BoolToUInt8(m_current_drive != 1) << 6) |
           (BoolToUInt8(m_current_drive != 0) << 5) | (BoolToUInt8(m_drives[m_current_drive].data_was_written) << 4) |
           (BoolToUInt8(m_drives[m_current_drive].data_was_read) << 3) |
           (BoolToUInt8(m_drives[m_current_drive].data_was_written) << 2) | (BoolToUInt8(m_current_drive != 3) << 1) |
           (BoolToUInt8(m_current_drive != 2) << 0);
}

void FDC::IOReadDigitalInputRegister(uint8* value)
{
  // Bit 7 - not disk change
  // Bit 3 - interrupt enable
  // Bit 2 - CCR?
  // Bit 1-0 - Data rate select
  Log_WarningPrintf("Read digital input register - stubbed");
  *value = uint8(0 << 7) | (BoolToUInt8(m_interrupt_enable) << 3) | uint8(0 << 2) | (m_data_rate_index & 0x03);

  // Clear step bit of current drive
  m_drives[m_current_drive].step_latch = false;
}

void FDC::IOReadDigitalOutputRegister(uint8* value)
{
  // Bit 7 - Motor on disk 3 enable
  // Bit 6 - Motor on disk 2 enable
  // Bit 5 - Motor on disk 1 enable
  // Bit 4 - Motor on disk 0 enable
  // Bit 3 - Interrupt enable
  // Bit 2 - Reset
  // Bit 1-0 - Drive select
  *value = (BoolToUInt8(m_motor_on[3]) << 7) | (BoolToUInt8(m_motor_on[2]) << 6) | (BoolToUInt8(m_motor_on[1]) << 5) |
           (BoolToUInt8(m_motor_on[0]) << 4) | (BoolToUInt8(m_interrupt_enable) << 3) | (BoolToUInt8(m_nreset) << 2) |
           (m_current_drive << 0);
}

void FDC::IOWriteDigitalOutputRegister(uint8 value)
{
  Log_TracePrintf("FDC write DOR=0x%02X", value);

  uint8 drive_select = value & 0x03;
  bool reset = !!(value & (1 << 2));
  bool interrupt_enable = !!(value & (1 << 3));
  uint8 motor_enable = (value >> 4);

  m_current_drive = drive_select;

  m_interrupt_enable = interrupt_enable;
  if (!m_interrupt_enable)
    LowerInterrupt();

  for (size_t i = 0; i < m_motor_on.size(); i++)
    m_motor_on[i] = ((motor_enable & (1 << i)) != 0);

  // This bit is inverted.
  if (reset != m_nreset)
  {
    m_nreset = reset;
    if (!reset)
    {
      Log_DevPrintf("FDC enter reset");
    }
    else if (reset)
    {
      Log_DevPrintf("FDC leave reset");
      SoftReset();
      RaiseInterrupt();
      m_reset_sense_interrupt_count = 4;
    }
  }

  // Clear FIFO, TODO move data direction to ClearFIFO
  ClearFIFO();
  m_main_status_register.data_direction = false;
}

void FDC::IOWriteDataRateSelectRegister(uint8 value)
{
  // TODO: Handle upper bits (software reset, etc).
  m_data_rate_index = value & 0x03;
}

void FDC::IOWriteConfigurationControlRegister(uint8 value)
{
  m_data_rate_index = value & 0x03;
}

void FDC::IOReadFIFO(uint8* value)
{
  DebugAssert(m_main_status_register.data_direction && m_main_status_register.request_for_master);

  Log_TracePrintf("FDC read fifo pos=%u", m_fifo_result_position);
  if (m_fifo_result_position == m_fifo_result_size)
  {
    // Bad read
    Log_WarningPrintf("Bad floppy data read");
    *value = 0;
    return;
  }

  *value = m_fifo[m_fifo_result_position++];

  // XT bios requires that this bit is cleared.
  // m_main_status_register.data_direction = false;

  // Interrupt is lowered after the first byte is read.
  LowerInterrupt();

  // Flags are cleared after result is read in its entirety.
  if (m_fifo_result_position == m_fifo_result_size)
    TransitionToCommandPhase();
}

void FDC::IOWriteFIFO(uint8 value)
{
  DebugAssert(!m_main_status_register.data_direction && m_main_status_register.request_for_master);
  Assert(m_fifo_command_position < FIFO_SIZE);

  Log_TracePrintf("FDC write fifo pos=%u,value=0x%02X", m_fifo_command_position, value);

  m_fifo[m_fifo_command_position++] = value;
  if (m_fifo_command_position == GetCurrentCommandLength())
    BeginCommand();
}

void FDC::EndTransfer(uint32 drive, uint8 st0_bits, uint8 st1_bits, uint8 st2_bits)
{
  if (m_current_transfer.active)
  {
    m_current_transfer.active = false;
    m_dma->SetDMAState(DMA_CHANNEL, false);
  }
  if (m_command_event->IsActive())
    m_command_event->Deactivate();

  m_st0 = GetST0(drive, st0_bits);
  m_st1 = GetST1(drive, st1_bits);
  m_st2 = GetST2(drive, st2_bits);

  Log_DevPrintf("End transfer, Drive=%u ST0=0x%02X ST1=0x%02X ST2=0x%02X", drive, ZeroExtend32(m_st0),
                ZeroExtend32(m_st1), ZeroExtend32(m_st2));

  ClearFIFO();
  WriteToFIFO(m_st0);
  WriteToFIFO(m_st1);
  WriteToFIFO(m_st2);
  WriteToFIFO(Truncate8(m_drives[drive].current_cylinder));
  WriteToFIFO(Truncate8(m_drives[drive].current_head));
  WriteToFIFO(Truncate8(m_drives[drive].current_sector));
  WriteToFIFO(Truncate8(2)); // from request sector size
  TransitionToResultPhase();
  RaiseInterrupt();
}

bool FDC::MoveToNextTransferSector()
{
  DriveState* drive_state = &m_drives[m_current_transfer.drive];

  // TODO: Handle errors here.
  if ((drive_state->current_sector + 1) > m_current_transfer.sectors_per_track)
  {
    // Move to next head if multi-track mode is on
    if (m_current_transfer.multi_track && drive_state->current_head == 0)
    {
      return SeekDrive(m_current_transfer.drive, drive_state->current_cylinder, 1, 1);
    }
    else
    {
      // Move to next cylinder
      return SeekDrive(m_current_transfer.drive, drive_state->current_cylinder + 1, 0, 1);
    }
  }
  else
  {
    return SeekDrive(m_current_transfer.drive, drive_state->current_cylinder, drive_state->current_head,
                     drive_state->current_sector + 1);
  }
}

void FDC::DMAReadCallback(IOPortDataSize size, uint32* value, uint32 remaining_bytes)
{
  Assert(m_current_transfer.active);

  // TODO: Is it defined what happens when we configure a DMA read with a write command?
  if (m_current_transfer.is_write)
  {
    Log_ErrorPrintf("DMA read with write command");
    EndTransfer(m_current_transfer.drive, ST0_IC_AT, ST1_ND, 0);
    return;
  }

  if (m_current_transfer.sector_offset == 0)
    ReadCurrentSector(m_current_transfer.drive, m_current_transfer.sector_buffer);

  *value = m_current_transfer.sector_buffer[m_current_transfer.sector_offset];

  // Check for early exit of transfer
  if (remaining_bytes == 0)
  {
    EndTransfer(m_current_transfer.drive, ST0_IC_NT, 0, 0);
    return;
  }

  m_current_transfer.sector_offset++;
  if (m_current_transfer.sector_offset >= m_current_transfer.bytes_per_sector)
  {
    m_current_transfer.sector_offset = 0;

    // TODO: Timing for seeking to next cylinder.
    if (!MoveToNextTransferSector())
    {
      EndTransfer(m_current_drive, ST0_IC_AT, ST1_EN, 0);
      return;
    }

    // Clear the request flag, while we're reading the next sector. EndCommand() will re-enable it.
    m_dma->SetDMAState(DMA_CHANNEL, false);
  }
}

void FDC::DMAWriteCallback(IOPortDataSize size, uint32 value, uint32 remaining_bytes)
{
  Assert(m_current_transfer.active);

  // TODO: Is it defined what happens when we configure a DMA read with a write command?
  if (!m_current_transfer.is_write)
  {
    Log_ErrorPrintf("DMA write with read command");
    EndTransfer(m_current_transfer.drive, ST0_IC_AT, ST1_ND, 0);
    return;
  }

  m_current_transfer.sector_buffer[m_current_transfer.sector_offset++] = Truncate8(value);
  if (m_current_transfer.sector_offset >= m_current_transfer.bytes_per_sector)
  {
    // Data will be lost if the sector size doesn't match..
    if (m_current_transfer.bytes_per_sector != SECTOR_SIZE)
      Log_ErrorPrintf("Incorrect sector size, data will be lost");

    WriteCurrentSector(m_current_transfer.drive, m_current_transfer.sector_buffer);
    m_current_transfer.sector_offset = 0;
  }

  // Check for early exit of transfer.
  if (remaining_bytes == 0)
  {
    if (m_current_transfer.sector_offset != 0)
      Log_ErrorPrintf("Incomplete sector DMA transfer, data will be lost");

    EndTransfer(m_current_transfer.drive, ST0_IC_NT, 0, 0);
    return;
  }

  // Move to next sector if there's still sectors remaining
  if (m_current_transfer.sector_offset == 0)
  {
    if (!MoveToNextTransferSector())
    {
      EndTransfer(m_current_drive, ST0_IC_AT, ST1_EN, 0);
      return;
    }

    // Clear the request flag, while we're writing the next sector. EndCommand() will re-enable it.
    m_dma->SetDMAState(DMA_CHANNEL, false);
  }
}

uint8 FDC::GetST0(uint32 drive, uint8 bits) const
{
  return (bits & 0xF8) | (Truncate8(m_drives[drive].current_head & 0x01) << 2) | (Truncate8(drive) << 0);
}

uint8 FDC::GetST1(uint32 drive, uint8 bits) const
{
  return bits;
}

uint8 FDC::GetST2(uint32 drive, uint8 bits) const
{
  return bits;
}

uint8 FDC::GetST3(uint32 drive, uint8 bits) const
{
  return (BoolToUInt8(m_drives[drive].write_protect) << 6) | uint8(1 << 5) |
         (BoolToUInt8(m_drives[drive].current_cylinder == 0) << 4) | uint8(1 << 3) |
         (Truncate8(m_drives[drive].current_head & 0x01) << 2) | (Truncate8(drive) << 0);
}

CycleCount FDC::CalculateHeadSeekTime(uint32 current_track, uint32 destination_track) const
{
  uint32 move_count =
    (current_track >= destination_track) ? (current_track - destination_track) : (destination_track - current_track);
  return CalculateHeadSeekTime() * CycleCount(std::max(move_count, 1u));
}

CycleCount FDC::CalculateHeadSeekTime() const
{
  // SRT value = 16 - (milliseconds * data_rate / 500000) (https://wiki.osdev.org/Floppy_Disk_Controller)
  uint32 val = uint32(m_step_rate_time ^ 0x0F) + 1;
  return (val * 500000) / data_rates[m_data_rate_index];
}

CycleCount FDC::CalculateSectorReadTime() const
{
  // to-microseconds, bits-per-sector / (data-rate-in-kbs*1000)
  return 1000000u * (512u * 8u) / (data_rates[m_data_rate_index] * 1000);
}

} // namespace HW
