#include "ata_hdd.h"
#include "../host_interface.h"
#include "../system.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "common/hdd_image.h"
#include "hdc.h"
#include <cinttypes>
Log_SetChannel(HW::ATAHDD);

// http://margo.student.utwente.nl/el/pc/hd-info/ide-tech.htm

namespace HW {

DEFINE_OBJECT_TYPE_INFO(ATAHDD);
DEFINE_GENERIC_COMPONENT_FACTORY(ATAHDD);
BEGIN_OBJECT_PROPERTY_MAP(ATAHDD)
PROPERTY_TABLE_MEMBER_STRING("ImageFile", 0, offsetof(ATAHDD, m_image_filename), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Cylinders", 0, offsetof(ATAHDD, m_cylinders), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Heads", 0, offsetof(ATAHDD, m_heads), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Sectors", 0, offsetof(ATAHDD, m_sectors_per_track), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

ATAHDD::ATAHDD(const String& identifier, const char* image_filename /* = "" */, u32 cylinders /* = 0 */,
               u32 heads /* = 0 */, u32 sectors /* = 0 */, u32 ide_channel /* = 0 */, u32 ide_device /* = 0 */,
               const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, ide_channel, ide_device, type_info), m_cylinders(cylinders), m_heads(heads),
    m_sectors_per_track(sectors)
{
}

bool ATAHDD::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_image = HDDImage::Open(m_image_filename);
  if (!m_image)
  {
    Log_ErrorPrintf("Failed to open image for drive %u/%u (%s)", m_ata_channel_number, m_ata_drive_number,
                    m_image_filename.GetCharArray());
    return false;
  }

  m_lbas = m_image->GetImageSize() / SECTOR_SIZE;
  if (m_cylinders == 0 || m_heads == 0 || m_sectors_per_track == 0)
  {
    if (ComputeCHSGeometry(m_image->GetImageSize(), m_cylinders, m_heads, m_sectors_per_track))
    {
      Log_DevPrintf("Computed CHS geometry for " PRIu64 " LBAs: %u/%u/%u", m_lbas, m_cylinders, m_heads,
                    m_sectors_per_track);
    }
    else
    {
      Log_ErrorPrintf("Could not compute CHS geometry for " PRIu64 " LBAs", m_lbas);
      return false;
    }
  }

  if ((static_cast<u64>(m_cylinders) * static_cast<u64>(m_heads) * static_cast<u64>(m_sectors_per_track) *
       SECTOR_SIZE) != m_image->GetImageSize())
  {
    Log_ErrorPrintf("CHS geometry does not match disk size: %u/%u/%u -> %u LBAs, real size %u LBAs", m_cylinders,
                    m_heads, m_sectors_per_track, m_cylinders * m_heads * m_sectors_per_track,
                    u32(m_image->GetImageSize() / SECTOR_SIZE));
    return false;
  }

  Log_DevPrintf("Attached ATA HDD on channel %u drive %u, C/H/S: %u/%u/%u", m_ata_channel_number, m_ata_drive_number,
                m_cylinders, m_heads, m_sectors_per_track);

  m_current_num_cylinders = m_cylinders;
  m_current_num_heads = m_heads;
  m_current_num_sectors_per_track = m_sectors_per_track;

  // Flush the HDD images once a second, to ensure data isn't lost.
  m_flush_event =
    m_system->GetTimingManager()->CreateFrequencyEvent("HDD Image Flush", 1.0f, std::bind(&ATAHDD::FlushImage, this));
  m_command_event = m_system->GetTimingManager()->CreateMicrosecondIntervalEvent(
    "ATA HDD Command", 1, std::bind(&ATAHDD::ExecutePendingCommand, this), false);
  m_read_write_event = m_system->GetTimingManager()->CreateMicrosecondIntervalEvent(
    "ATA HDD Read/Write", 1, std::bind(&ATAHDD::ExecutePendingReadWrite, this), false);

  // Create indicator and menu options.
  system->GetHostInterface()->AddUIIndicator(this, HostInterface::IndicatorType::HDD);
  system->GetHostInterface()->AddUICallback(this, "Commit Log to Image", [this]() { m_image->CommitLog(); });
  system->GetHostInterface()->AddUICallback(this, "Revert Log and Reset", [this]() {
    m_image->RevertLog();
    m_system->Reset();
  });
  return true;
}

void ATAHDD::Reset()
{
  BaseClass::Reset();
}

bool ATAHDD::LoadState(BinaryReader& reader)
{
  if (!BaseClass::LoadState(reader))
    return false;

  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  const u64 num_lbas = reader.ReadUInt64();
  const u32 num_cylinders = reader.ReadUInt32();
  const u32 num_heads = reader.ReadUInt32();
  const u32 num_sectors = reader.ReadUInt32();
  if (num_cylinders != m_cylinders || num_heads != m_heads || num_sectors != m_sectors_per_track || num_lbas != m_lbas)
  {
    Log_ErrorPrintf("Save state geometry mismatch");
    return false;
  }

  m_current_num_cylinders = reader.ReadUInt32();
  m_current_num_heads = reader.ReadUInt32();
  m_current_num_sectors_per_track = reader.ReadUInt32();
  m_multiple_sectors = reader.ReadUInt32();

  m_current_lba = reader.ReadUInt64();

  m_current_command = reader.ReadUInt16();
  m_command_event->SetActive(false);
  if (reader.ReadBool())
    m_command_event->Queue(1);

  m_transfer_remaining_sectors = reader.ReadUInt32();
  m_transfer_block_size = reader.ReadUInt32();

  if (reader.GetErrorState())
    return false;

  return m_image->LoadState(reader.GetStream());
}

bool ATAHDD::SaveState(BinaryWriter& writer)
{
  if (!BaseClass::SaveState(writer))
    return false;

  writer.WriteUInt32(SERIALIZATION_ID);

  writer.WriteUInt64(m_lbas);
  writer.WriteUInt32(m_cylinders);
  writer.WriteUInt32(m_heads);
  writer.WriteUInt32(m_sectors_per_track);

  writer.WriteUInt32(m_current_num_cylinders);
  writer.WriteUInt32(m_current_num_heads);
  writer.WriteUInt32(m_current_num_sectors_per_track);
  writer.WriteUInt32(m_multiple_sectors);

  writer.WriteUInt64(m_current_lba);

  writer.WriteUInt16(m_current_command);
  writer.WriteBool(m_command_event->IsActive());

  writer.WriteUInt32(m_transfer_remaining_sectors);
  writer.WriteUInt32(m_transfer_block_size);

  if (writer.InErrorState())
    return false;

  return m_image->SaveState(writer.GetStream());
}

void ATAHDD::DoReset(bool is_hardware_reset)
{
  BaseClass::DoReset(is_hardware_reset);
  SetSignature();

  // TODO: This should be behind a flag check.
  if (is_hardware_reset)
  {
    m_current_num_cylinders = m_cylinders;
    m_current_num_heads = m_heads;
    m_current_num_sectors_per_track = m_sectors_per_track;
    m_current_lba = 0;
    m_multiple_sectors = 0;
  }

  // Abort any commands.
  m_command_event->SetActive(false);
  m_read_write_event->SetActive(false);
  m_current_command = INVALID_COMMAND;
  m_transfer_remaining_sectors = 0;
  m_transfer_block_size = 0;
  ClearActivity();
}

void ATAHDD::SetSignature()
{
  m_registers.sector_count = 1;
  m_registers.sector_number = 1;
  m_registers.cylinder_low = 0;
  m_registers.cylinder_high = 0;
}

bool ATAHDD::SupportsDMA() const
{
  return true;
}

void ATAHDD::ClearActivity()
{
  m_system->GetHostInterface()->SetUIIndicatorState(this, HostInterface::IndicatorState::Off);
}

void ATAHDD::FlushImage()
{
  m_image->Flush();
}

bool ATAHDD::ComputeCHSGeometry(const u64 size, u32& cylinders, u32& heads, u32& sectors_per_track)
{
  // assume 16 heads/63 sectors per track
  const u32 DEFAULT_SECTORS = 63;
  const u32 DEFAULT_HEADS = 16;

  if (sectors_per_track == 0)
    sectors_per_track = DEFAULT_SECTORS;
  if (heads == 0)
    heads = DEFAULT_HEADS;

  const u32 cylinder_size = heads * sectors_per_track * SECTOR_SIZE;
  cylinders = static_cast<u32>(size / cylinder_size);
  if ((size % cylinder_size) != 0)
  {
    Log_ErrorPrintf("Image size %" PRId64 " with H/S %u/%u does not evenly divide by cylinder size", size, heads,
                    sectors_per_track, cylinder_size);
    return false;
  }

  return true;
}

void ATAHDD::TranslateLBAToCHS(const u64 lba, u32* cylinder, u32* head, u32* sector) const
{
  const u32 head_size = m_current_num_sectors_per_track;
  const u32 cylinder_size = m_current_num_heads * head_size;

  // order is sectors -> heads -> cylinders
  *cylinder = Truncate32(lba / cylinder_size);
  *head = Truncate32((lba % cylinder_size) / head_size);
  *sector = Truncate32((lba % cylinder_size) % head_size) + 1;
}

u64 ATAHDD::TranslateCHSToLBA(const u32 cylinder, const u32 head, const u32 sector) const
{
  const u32 head_size = m_current_num_sectors_per_track;
  const u32 cylinder_size = m_current_num_heads * head_size;
  DebugAssert(sector > 0);

  u64 lba = static_cast<u64>(cylinder) * cylinder_size;
  lba += static_cast<u64>(head) * head_size;
  lba += static_cast<u64>(sector - 1);
  return lba;
}

u64 ATAHDD::GetTransferLBA(bool is_lba, bool is_lba48) const
{
  u64 lba;
  if (!is_lba)
  {
    // CHS
    const u32 cylinder = (m_registers.cylinder_low & 0xFF) | ((m_registers.cylinder_high & 0xFF) << 8);
    const u32 head = m_registers.drive_select.head.GetValue();
    const u32 sector = (m_registers.sector_number & 0xFF);
    if (sector == 0)
    {
      // Sector must be at least one. Return an out-of-range value.
      return m_lbas;
    }

    lba = TranslateCHSToLBA(cylinder, head, sector);
  }
  else if (!is_lba48)
  {
    // Using LBA24
    lba = (ZeroExtend64(m_registers.sector_number & 0xFF) << 0);
    lba |= (ZeroExtend64(m_registers.cylinder_low & 0xFF) << 8);
    lba |= (ZeroExtend64(m_registers.cylinder_high & 0xFF) << 16);
    lba |= (ZeroExtend64(m_registers.drive_select.head.GetValue()) << 24);
  }
  else
  {
    // Using LBA48
    lba = (ZeroExtend64(m_registers.sector_number & 0xFF));
    lba |= (ZeroExtend64(m_registers.cylinder_low & 0xFF) << 8);
    lba |= (ZeroExtend64(m_registers.cylinder_high & 0xFF) << 16);
    lba |= (ZeroExtend64(m_registers.sector_number >> 8) << 24);
    lba |= (ZeroExtend64(m_registers.cylinder_low >> 8) << 32);
    lba |= (ZeroExtend64(m_registers.cylinder_high >> 8) << 40);
  }
  return lba;
}

u32 ATAHDD::GetTransferSectorCount(bool is_lba, bool is_lba48) const
{
  if (!is_lba || !is_lba48)
  {
    u32 count = ZeroExtend32(m_registers.sector_count & 0xFF);
    return (count > 0) ? count : 256;
  }
  else
  {
    u32 count = ZeroExtend32(m_registers.sector_count);
    return (count > 0) ? count : 65536;
  }
}

void ATAHDD::SetTransferLBA(bool is_lba, bool is_lba48, u64 lba)
{
  if (!is_lba)
  {
    u32 cylinder, head, sector;
    TranslateLBAToCHS(lba, &cylinder, &head, &sector);
    m_registers.cylinder_low = Truncate8(cylinder);
    m_registers.cylinder_high = Truncate16((cylinder >> 8) & 0xFF);
    m_registers.sector_number = Truncate16(sector);
    m_registers.drive_select.head = Truncate8(head);
  }
  else if (!is_lba48)
  {
    m_registers.sector_number = ZeroExtend16(Truncate8(m_current_lba));
    m_registers.cylinder_low = ZeroExtend16(Truncate8(m_current_lba >> 8));
    m_registers.cylinder_high = ZeroExtend16(Truncate8(m_current_lba >> 16));
  }
  else
  {
    m_registers.sector_number = Truncate16(m_current_lba);
    m_registers.cylinder_low = Truncate16(m_current_lba >> 16);
    m_registers.cylinder_high = Truncate16(m_current_lba >> 24);
  }
}

bool ATAHDD::Seek(const u64 lba)
{
  if (lba >= m_lbas)
    return false;

  m_current_lba = lba;
  return true;
}

void ATAHDD::CompleteCommand(bool seek_complete /* = false */, bool raise_interrupt /* = true */)
{
  m_transfer_remaining_sectors = 0;
  m_transfer_block_size = 0;
  ResetBuffer();

  m_current_command = INVALID_COMMAND;
  m_registers.status.SetReady();
  if (seek_complete)
    m_registers.status.seek_complete = true;

  m_registers.error = 0;
  ClearActivity();

  if (raise_interrupt)
    RaiseInterrupt();
}

void ATAHDD::AbortCommand(ATA_ERR error /* = ATA_ERR_ABRT */, bool device_fault /* = false */)
{
  m_transfer_remaining_sectors = 0;
  m_transfer_block_size = 0;
  ResetBuffer();

  m_current_command = INVALID_COMMAND;
  m_registers.status.SetError(device_fault);
  m_registers.error = error;
  RaiseInterrupt();
  ClearActivity();
}

void ATAHDD::SetupTransfer(u32 num_sectors, u32 block_size, bool is_write, bool dma)
{
  DebugAssert(num_sectors > 0);
  m_transfer_remaining_sectors = num_sectors;
  m_transfer_block_size = block_size;
  SetupBuffer(std::min(block_size, num_sectors) * SECTOR_SIZE, is_write, dma);
}

void ATAHDD::SetupReadWriteEvent(CycleCount seek_time, u32 num_sectors)
{
  const CycleCount rw_time = CalculateReadWriteTime(num_sectors);
  Log_DebugPrintf("Sector r/w time for %u sectors at lba %" PRIu64 ": %u us", num_sectors, m_current_lba,
                  static_cast<unsigned>(rw_time));

  DebugAssert(!m_read_write_event->IsActive());
  m_read_write_event->Queue(seek_time + rw_time);
}

void ATAHDD::FillReadBuffer()
{
  const u32 sector_count = std::min(m_transfer_remaining_sectors, m_transfer_block_size);
  DebugAssert(m_buffer.size >= (sector_count * SECTOR_SIZE));
  DebugAssert((m_current_lba + sector_count) * SECTOR_SIZE <= m_image->GetImageSize());
  m_image->Read(m_buffer.data.data(), m_current_lba * SECTOR_SIZE, sector_count * SECTOR_SIZE);
  m_current_lba += sector_count;
}

void ATAHDD::FlushWriteBuffer()
{
  const u32 sector_count = std::min(m_transfer_remaining_sectors, m_transfer_block_size);
  DebugAssert(m_buffer.size >= (sector_count * SECTOR_SIZE));
  DebugAssert((m_current_lba + sector_count) * SECTOR_SIZE <= m_image->GetImageSize());
  m_image->Write(m_buffer.data.data(), m_current_lba * SECTOR_SIZE, sector_count * SECTOR_SIZE);
  m_current_lba += sector_count;
}

void ATAHDD::OnBufferEnd()
{
  if (m_buffer.is_write)
  {
    // If it's a write, the buffer must be written to the backing image.
    FlushWriteBuffer();

    // Setup the write event.
    m_registers.status.SetBusy();
    SetupReadWriteEvent(0, std::min(m_transfer_remaining_sectors, m_transfer_block_size));
    return;
  }
  else
  {
    OnReadWriteEnd();
  }
}

void ATAHDD::ExecutePendingReadWrite()
{
  m_read_write_event->Deactivate();

  if (m_buffer.is_write)
  {
    // Write completed.
    OnReadWriteEnd();
  }
  else
  {
    // Command is a read, the buffer is now ready for the host to access.
    FillReadBuffer();
    BufferReady(true);
  }
}

void ATAHDD::OnReadWriteEnd()
{
  // Are we finished with the command?
  const u32 transferred_sectors = std::min(m_transfer_remaining_sectors, m_transfer_block_size);
  m_transfer_remaining_sectors -= transferred_sectors;
  if (m_transfer_remaining_sectors == 0)
  {
    // PIO read commands do not raise an interrupt after the command finishes, but write commands do.
    CompleteCommand(true, m_buffer.is_dma || IsWriteCommand(Truncate8(m_current_command)));
    return;
  }

  // Reset buffer position for the next block.
  const u32 next_transfer_sectors = std::min(m_transfer_remaining_sectors, m_transfer_block_size);
  SetupBuffer(next_transfer_sectors * SECTOR_SIZE, m_buffer.is_write, m_buffer.is_dma);
  if (m_buffer.is_write)
  {
    // Writes - request data from host.
    BufferReady(true);
  }
  else
  {
    // Reads - do the read.
    SetupReadWriteEvent(0, next_transfer_sectors);
  }
}

void ATAHDD::WriteCommandRegister(u8 value)
{
  Log_TracePrintf("ATA drive %u/%u command register <- 0x%02X", m_ata_channel_number, m_ata_drive_number, value);

  // Ignore writes to the command register when busy.
  if (!m_registers.status.IsReady() || HasPendingCommand())
  {
    Log_WarningPrintf("ATA drive %u/%u is not ready", m_ata_channel_number, m_ata_drive_number);
    return;
  }

  // We're busy now, and can't accept other commands.
  m_registers.status.SetBusy();

  // Set the activity indicator.
  m_system->GetHostInterface()->SetUIIndicatorState(
    this, IsWriteCommand(value) ? HostInterface::IndicatorState::Writing : HostInterface::IndicatorState::Reading);

  // Determine how long the command will take to execute.
  const CycleCount cycles = CalculateCommandTime(value);
  Log_DebugPrintf("Queueing ATA command 0x%02X in %u us", value, unsigned(cycles));
  m_current_command = ZeroExtend16(value);
  if (cycles > 0)
    m_command_event->Queue(cycles);
  else
    ExecutePendingCommand();
}

CycleCount ATAHDD::CalculateCommandTime(u8 command) const
{
#if 0
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
      return 10;

      // Writes should respond immediately, but the actual write takes time.
    case ATA_CMD_WRITE_PIO:
    case ATA_CMD_WRITE_PIO_NO_RETRY:
    case ATA_CMD_WRITE_PIO_EXT:
    case ATA_CMD_WRITE_DMA:
    case ATA_CMD_WRITE_DMA_EXT:
      return 1;

      // Default to 1us for all other commands.
    default:
      return 1;
  }
#else
  return 1;
#endif
}

CycleCount ATAHDD::CalculateSeekTime(u64 from_lba, u64 to_lba) const
{
  return 1;
}

CycleCount ATAHDD::CalculateReadWriteTime(u32 num_sectors) const
{
  return 1 * num_sectors;
}

bool ATAHDD::IsWriteCommand(u8 command) const
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

bool ATAHDD::HasPendingCommand() const
{
  return (m_current_command != INVALID_COMMAND);
}

void ATAHDD::ExecutePendingCommand()
{
  const u8 command = Truncate8(m_current_command);
  m_command_event->SetActive(false);

  Log_TracePrintf("Executing ATA command 0x%02X on drive %u/%u", command, m_ata_channel_number, m_ata_drive_number);

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
      HandleATATransfer(false, false, false, false);
      break;

    case ATA_CMD_READ_MULTIPLE_PIO:
      HandleATATransfer(false, false, false, true);
      break;

    case ATA_CMD_READ_PIO_EXT:
    case ATA_CMD_READ_PIO_NO_RETRY:
      HandleATATransfer(false, false, true, false);
      break;

    case ATA_CMD_READ_DMA:
      HandleATATransfer(true, false, false, false);
      break;

    case ATA_CMD_READ_DMA_EXT:
      HandleATATransfer(true, false, true, false);
      break;

    case ATA_CMD_READ_VERIFY:
      HandleATAReadVerifySectors(false, true);
      break;

    case ATA_CMD_READ_VERIFY_EXT:
      HandleATAReadVerifySectors(true, false);
      break;

    case ATA_CMD_WRITE_PIO:
    case ATA_CMD_WRITE_PIO_NO_RETRY:
      HandleATATransfer(false, true, false, false);
      break;

    case ATA_CMD_WRITE_MULTIPLE_PIO:
      HandleATATransfer(false, true, false, true);
      break;

    case ATA_CMD_WRITE_PIO_EXT:
      HandleATATransfer(false, true, true, false);
      break;

    case ATA_CMD_WRITE_DMA:
      HandleATATransfer(true, true, false, false);
      break;

    case ATA_CMD_WRITE_DMA_EXT:
      HandleATATransfer(true, true, true, false);
      break;

    case ATA_CMD_READ_NATIVE_MAX_ADDRESS:
      HandleATAReadMaxNativeAddress(false);
      break;

    case ATA_CMD_READ_NATIVE_MAX_ADDRESS_EXT:
      HandleATAReadMaxNativeAddress(false);
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

void ATAHDD::HandleATADeviceReset()
{
  Log_DevPrintf("ATA reset drive %u/%u", m_ata_channel_number, m_ata_drive_number);
  CompleteCommand();
  DoReset(false);
}

void ATAHDD::HandleATAIdentify()
{
  Log_DevPrintf("ATA identify drive %u/%u", m_ata_channel_number, m_ata_drive_number);

  ATA_IDENTIFY_RESPONSE response = {};
  // response.flags |= (1 << 10); // >10mbit/sec transfer speed
  response.flags |= (1 << 6); // Fixed drive
  // response.flags |= (1 << 2);  // Soft sectored
  response.cylinders = Truncate16(m_cylinders);
  response.heads = Truncate16(m_heads);
  response.unformatted_bytes_per_track = Truncate16(SECTOR_SIZE * m_sectors_per_track);
  response.unformatted_bytes_per_sector = Truncate16(SECTOR_SIZE);
  response.sectors_per_track = Truncate16(m_sectors_per_track);
  response.buffer_type = 3;
  response.buffer_size = 512; // 256KB cache
  response.ecc_bytes = 4;
  PutIdentifyString(response.serial_number, sizeof(response.serial_number), "DERP123");
  PutIdentifyString(response.firmware_revision, sizeof(response.firmware_revision), "HURR101");

  // Temporarily disabled as it seems to be broken.
  // response.readwrite_multiple_supported = 0x8000 | 16; // this is actually the number of sectors, tweak it
  response.readwrite_multiple_supported = 0;

  response.dword_io_supported = 1;
  response.support = 0xf000u;
  response.support |= (1 << 9); // LBA supported
  if (m_ata_controller->SupportsDMA())
    response.support |= (1 << 8); // DMA supported
  response.pio_timing_mode = 2;
  response.dma_timing_mode = 1;
  response.user_fields_valid = 3;
  response.user_cylinders = Truncate16(m_current_num_cylinders);
  response.user_heads = Truncate16(m_current_num_heads);
  response.user_sectors_per_track = Truncate16(m_current_num_sectors_per_track);
  response.user_total_sectors =
    Truncate32(m_current_num_cylinders * m_current_num_heads * m_current_num_sectors_per_track);
  response.lba_sectors = Truncate32(std::min(m_lbas, ZeroExtend64(std::numeric_limits<uint32>::max())));
  PutIdentifyString(response.model, sizeof(response.model), "Herp derpity derp");
  if (m_ata_controller->SupportsDMA())
  {
    response.singleword_dma_modes = 0x07 | (4 << 8);
    response.multiword_dma_modes = 0x07 | (4 << 8);
  }
  response.pio_modes_supported = 0x03;
  for (size_t i = 0; i < countof(response.pio_cycle_time); i++)
    response.pio_cycle_time[i] = 120;
  response.word_80 = 0x7E;
  response.minor_version_number = 0x00;
  response.word_82 = (1 << 14);
  response.supports_lba48 = (1 << 10);
  response.word_84 = (1 << 14);
  response.word_85 = (1 << 14);
  response.word_86 = (1 << 10);
  response.word_88 = 0x3F | 0x01; // UDMA mode
  response.word_93 = 1 | (1 << 14) | 0x2000;
  response.lba48_sectors = m_lbas;

  // 512 bytes total
  SetupTransfer(1, 1, false, false);
  std::memcpy(m_buffer.data.data(), &response, sizeof(response));
  BufferReady(true);
}

void ATAHDD::HandleATARecalibrate()
{
  Log_DevPrintf("ATA recalibrate drive %u/%u", m_ata_channel_number, m_ata_drive_number);
  m_current_lba = 0;
  CompleteCommand(true);
}

void ATAHDD::HandleATATransfer(bool dma, bool write, bool extended, bool multiple)
{
  if (dma && (!m_ata_controller->SupportsDMA() || m_ata_controller->IsDMARequested(m_ata_channel_number)))
  {
    AbortCommand();
    return;
  }
  else if (multiple && m_multiple_sectors == 0)
  {
    Log_WarningPrintf("Multiple command without multiple sectors set");
    AbortCommand();
    return;
  }

  const u64 lba = GetTransferLBA(m_registers.drive_select.lba_enable, extended);
  const u32 count = GetTransferSectorCount(m_registers.drive_select.lba_enable, extended);
  Log_DevPrintf("%s %s %u sectors from LBA %u", dma ? "DMA" : "PIO", write ? "write" : "read", count, Truncate32(lba));
  if (!Seek(lba) || (m_current_lba + count) > m_lbas)
  {
    AbortCommand(ATA_ERR_ABRT);
    return;
  }

  // Setup transfer and fire irq
  SetupTransfer(count, multiple ? m_multiple_sectors : 1, write, dma);
  if (!write)
  {
    // Reads are delayed.
    SetupReadWriteEvent(0, std::min(m_transfer_remaining_sectors, m_transfer_block_size));
  }
  else
  {
    // Writes begin immediately, and write to the disk after the host sends the data.
    BufferReady(true);
  }
}

void ATAHDD::HandleATAReadVerifySectors(bool extended, bool with_retry)
{
  const u64 lba = GetTransferLBA(m_registers.drive_select.lba_enable, extended);
  const u32 count = GetTransferSectorCount(m_registers.drive_select.lba_enable, extended);

  // Check that it's in range
  Log_DevPrintf("Verify %u sectors from LBA %u", count, Truncate32(lba));
  if (!Seek(lba))
  {
    AbortCommand(ATA_ERR_IDNF);
    return;
  }

  const bool error = ((m_current_lba + count) > m_lbas);
  m_current_lba = std::min(m_current_lba + count, m_lbas);

  // The command block contains the last sector verified.
  SetTransferLBA(m_registers.drive_select.lba_enable, extended, (m_current_lba > 0) ? (m_current_lba - 1) : 0);

  if (error)
    AbortCommand(ATA_ERR_IDNF);
  else
    CompleteCommand(true);
}

void ATAHDD::HandleATAReadMaxNativeAddress(bool extended)
{
  if (!m_registers.drive_select.lba_enable)
  {
    AbortCommand();
    return;
  }

  SetTransferLBA(m_registers.drive_select.lba_enable, extended, m_lbas);
  CompleteCommand(true);
}

void ATAHDD::HandleATASetMultipleMode()
{
  // TODO: We should probably set an upper bound on the number of sectors
  const u16 multiple_sectors = (m_registers.sector_count & 0xFF);
  Log_DevPrintf("ATA set multiple for drive %u/%u to %u", m_ata_channel_number, m_ata_drive_number, multiple_sectors);

  // Has to be aligned
  if (multiple_sectors != 0 && (multiple_sectors > 16 || (multiple_sectors & (multiple_sectors - 1)) != 0))
  {
    AbortCommand();
  }
  else
  {
    m_multiple_sectors = multiple_sectors;
    CompleteCommand();
  }
}

void ATAHDD::HandleATAExecuteDriveDiagnostic()
{
  Log_DevPrintf("ATA execute drive diagnostic %u/%u", m_ata_channel_number, m_ata_drive_number);
  m_registers.error = 1; // No error detected
  CompleteCommand();
}

void ATAHDD::HandleATAInitializeDriveParameters()
{
  u32 num_cylinders = ZeroExtend32(m_cylinders);
  u32 num_heads = ZeroExtend32(m_registers.drive_select.head.GetValue()) + 1;
  u32 num_sectors = ZeroExtend32(m_registers.sector_count & 0xFF);

  Log_DevPrintf("ATA initialize drive parameters drive=%u/%u,cylinders=%u,heads=%u,sectors=%u", m_ata_channel_number,
                m_ata_drive_number, num_cylinders, num_heads, num_sectors);

  // Abort the command if the translation is not possible.
  if ((num_cylinders * num_heads * num_sectors) > m_lbas)
  {
    Log_WarningPrintf("ATA invalid geometry");
    AbortCommand();
    return;
  }

  m_current_num_cylinders = num_cylinders;
  m_current_num_heads = num_heads;
  m_current_num_sectors_per_track = num_sectors;
  CompleteCommand();
}

void ATAHDD::HandleATASetFeatures()
{
  Log_DevPrintf("ATA drive %u/%u set features 0x%02X", m_ata_channel_number, m_ata_drive_number,
                m_registers.feature_select);

  switch (m_registers.feature_select)
  {
    case 0x03: // Set transfer mode
    {
      u8 transfer_mode = Truncate8((m_registers.sector_count >> 3) & 0x1F);
      u8 sub_mode = Truncate8(m_registers.sector_count & 0x03);
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
      Log_ErrorPrintf("Unknown feature 0x%02X", m_registers.feature_select);
      AbortCommand();
      return;
  }
}

} // namespace HW
