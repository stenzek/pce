#include "pce/hw/cdrom.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "pce/host_interface.h"
#include "pce/system.h"
#include <functional>
Log_SetChannel(HW::CDROM);

namespace HW {

DEFINE_OBJECT_TYPE_INFO(CDROM);
BEGIN_OBJECT_PROPERTY_MAP(CDROM)
PROPERTY_TABLE_MEMBER_STRING("VendorID", 0, offsetof(CDROM, m_vendor_id_string), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("ModelID", 0, offsetof(CDROM, m_model_id_string), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("FirmwareVersion", 0, offsetof(CDROM, m_firmware_version_string), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

CDROM::CDROM(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_clock("CDROM", 1000000.0f), m_vendor_id_string("POTATO"),
    m_model_id_string("POTATOROM"), m_firmware_version_string("1.00")
{
}

CDROM::~CDROM() {}

bool CDROM::Initialize(System* system, Bus* bus)
{
  m_clock.SetManager(system->GetTimingManager());
  m_command_event = m_clock.NewEvent("CDROM Command Event", 1, std::bind(&CDROM::ExecuteCommand, this), false);

  // Create indicator and menu options.
  system->GetHostInterface()->AddUIIndicator(this, HostInterface::IndicatorType::CDROM);
  system->GetHostInterface()->AddUIFileCallback(this, "Insert Media...", [this](const String& filename) {
    if (!InsertMedia(filename))
      m_system->GetHostInterface()->ReportError("Failed to insert media");
  });
  system->GetHostInterface()->AddUICallback(this, "Eject Media", [this]() { EjectMedia(); });
  return true;
}

void CDROM::Reset()
{
  m_command_buffer.clear();
  m_data_buffer.clear();
  m_data_response_size = 0;
  m_error = false;
  m_busy = false;

  UpdateSenseInfo(SENSE_NO_STATUS, 0);

  m_current_lba = 0;
  m_tray_locked = false;

  if (m_command_event->IsActive())
    m_command_event->Deactivate();
}

bool CDROM::LoadState(BinaryReader& reader)
{
  SAFE_RELEASE(m_media.stream);

  uint32 magic;
  if (!reader.SafeReadUInt32(&magic) || magic != SERIALIZATION_ID)
    return false;

  uint32 size;
  if (!reader.SafeReadUInt32(&size))
    return false;
  m_command_buffer.resize(size);
  if (size > 0 && !reader.SafeReadBytes(m_command_buffer.data(), size))
    return false;

  if (!reader.SafeReadUInt32(&size))
    return false;
  m_data_buffer.resize(size);
  if (size > 0 && !reader.SafeReadBytes(m_data_buffer.data(), size))
    return false;

  bool result = reader.SafeReadUInt32(&m_data_response_size);
  result &= reader.SafeReadBool(&m_busy);
  result &= reader.SafeReadBool(&m_error);

  uint8 sense_key = 0;
  result &= reader.SafeReadUInt8(&sense_key);
  m_sense.key = static_cast<SENSE_KEY>(sense_key);
  result &= reader.SafeReadBytes(&m_sense.information, sizeof(m_sense.information));
  result &= reader.SafeReadBytes(&m_sense.specific_information, sizeof(m_sense.specific_information));
  result &= reader.SafeReadBytes(&m_sense.key_spec, sizeof(m_sense.key_spec));
  result &= reader.SafeReadUInt8(&m_sense.fruc);
  result &= reader.SafeReadUInt8(&m_sense.asc);
  result &= reader.SafeReadUInt8(&m_sense.ascq);
  result &= reader.SafeReadCString(&m_media.filename);
  result &= reader.SafeReadUInt64(&m_media.total_sectors);
  result &= reader.SafeReadUInt64(&m_current_lba);
  result &= reader.SafeReadUInt32(&m_remaining_sectors);
  result &= reader.SafeReadBool(&m_tray_locked);

  if (!result)
    return false;

  // Load up the media, and make sure it matches in size.
  if (!m_media.filename.IsEmpty())
  {
    if (!ByteStream_OpenFileStream(m_media.filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE,
                                   &m_media.stream) ||
        m_media.stream->GetSize() != (m_media.total_sectors * SECTOR_SIZE))
    {
      Log_ErrorPrintf("Failed to re-insert CD media from save state: '%s'. Ejecting.", m_media.filename.GetCharArray());
      EjectMedia();
    }
  }

  return true;
}

bool CDROM::SaveState(BinaryWriter& writer)
{
  bool result = writer.SafeWriteUInt32(SERIALIZATION_ID);
  result &= writer.SafeWriteUInt32(static_cast<uint32>(m_command_buffer.size()));
  if (!m_command_buffer.empty())
    result &= writer.SafeWriteBytes(m_command_buffer.data(), static_cast<uint32>(m_command_buffer.size()));
  result &= writer.SafeWriteUInt32(static_cast<uint32>(m_data_buffer.size()));
  if (!m_data_buffer.empty())
    result &= writer.SafeWriteBytes(m_data_buffer.data(), static_cast<uint32>(m_data_buffer.size()));

  result &= writer.SafeWriteUInt32(m_data_response_size);
  result &= writer.SafeWriteBool(m_busy);
  result &= writer.SafeWriteBool(m_error);

  result &= writer.SafeWriteUInt8(static_cast<uint8>(m_sense.key));
  result &= writer.SafeWriteBytes(m_sense.information, sizeof(m_sense.information));
  result &= writer.SafeWriteBytes(m_sense.specific_information, sizeof(m_sense.specific_information));
  result &= writer.SafeWriteBytes(m_sense.key_spec, sizeof(m_sense.key_spec));
  result &= writer.SafeWriteUInt8(m_sense.fruc);
  result &= writer.SafeWriteUInt8(m_sense.asc);
  result &= writer.SafeWriteUInt8(m_sense.ascq);
  result &= writer.SafeWriteCString(m_media.filename);
  result &= writer.SafeWriteUInt64(m_media.total_sectors);
  result &= writer.SafeWriteUInt64(m_current_lba);
  result &= writer.SafeWriteUInt32(m_remaining_sectors);
  result &= writer.SafeWriteBool(m_tray_locked);
  return result;
}

bool CDROM::InsertMedia(const char* filename)
{
  if (HasMedia())
    EjectMedia();

  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE, &m_media.stream))
  {
    Log_ErrorPrintf("Failed to open CD media: %s", filename);
    return false;
  }

  uint64 file_size = m_media.stream->GetSize();
  if (file_size < SECTOR_SIZE)
  {
    Log_ErrorPrintf("File '%s' does not contain at least one sector", unsigned(file_size));
    return false;
  }

  if ((file_size % SECTOR_SIZE) != 0)
    Log_WarningPrintf("File '%s' is not aligned to sector size. (%u vs %u bytes)", unsigned(file_size), SECTOR_SIZE);

  m_media.filename = filename;
  m_media.total_sectors = file_size / SECTOR_SIZE;
  m_current_lba = 0;
  Log_InfoPrintf("Inserted CD media '%s': %u sectors", filename, uint32(m_media.total_sectors));

  // TODO: Fire interrupt.
  UpdateSenseInfo(SENSE_UNIT_ATTENTION, ASC_MEDIUM_MAY_HAVE_CHANGED);

  return true;
}

void CDROM::EjectMedia()
{
  SAFE_RELEASE(m_media.stream);
  m_media.filename.Clear();
  m_media.total_sectors = 0;
  m_current_lba = 0;

  // TODO: Fire interrupt.
  UpdateSenseInfo(SENSE_UNIT_ATTENTION, ASC_MEDIUM_MAY_HAVE_CHANGED);
}

void CDROM::UpdateSenseInfo(SENSE_KEY key, uint8 asc)
{
  m_sense = {};
  m_sense.key = key;
  m_sense.asc = asc;
}

void CDROM::AllocateData(uint32 reserve_length, uint32 response_length)
{
  m_data_buffer.resize(std::max(reserve_length, response_length));
  m_data_response_size = std::min(reserve_length, response_length);
}

uint8 CDROM::ReadCommandBufferByte(uint32 offset) const
{
  if (offset > m_command_buffer.size())
    return 0;
  return m_command_buffer[offset];
}

uint16 CDROM::ReadCommandBufferWord(uint32 offset) const
{
  // TODO: Endian conversion
  if ((offset + 1) > m_command_buffer.size())
    return 0;

  return (ZeroExtend16(m_command_buffer[offset]) << 8) | ZeroExtend16(m_command_buffer[offset + 1]);
}

uint32 CDROM::ReadCommandBufferDWord(uint32 offset) const
{
  // TODO: Endian conversion
  if ((offset + 3) > m_command_buffer.size())
    return 0;

  return (ZeroExtend32(m_command_buffer[offset]) << 24) | (ZeroExtend32(m_command_buffer[offset + 1]) << 16) |
         (ZeroExtend32(m_command_buffer[offset + 2]) << 8) | (ZeroExtend32(m_command_buffer[offset + 3]));
}

uint32 CDROM::ReadCommandBufferLBA24(uint32 offset) const
{
  // TODO: Endian conversion
  if ((offset + 3) > m_command_buffer.size())
    return 0;

  return (ZeroExtend32(m_command_buffer[offset]) << 16) | (ZeroExtend32(m_command_buffer[offset + 1]) << 8) |
         (ZeroExtend32(m_command_buffer[offset + 2]));
}

uint64 CDROM::ReadCommandBufferLBA48(uint32 offset) const
{
  // TODO: Endian conversion
  if ((offset + 5) > m_command_buffer.size())
    return 0;

  return (ZeroExtend64(m_command_buffer[offset]) << 40) | (ZeroExtend64(m_command_buffer[offset + 1]) << 32) |
         (ZeroExtend64(m_command_buffer[offset + 2]) << 24) | (ZeroExtend64(m_command_buffer[offset + 3]) << 16) |
         (ZeroExtend64(m_command_buffer[offset + 4]) << 8) | (ZeroExtend64(m_command_buffer[offset + 5]));
}

void CDROM::WriteDataBufferByte(uint32 offset, uint8 value)
{
  Assert(offset < m_data_buffer.size());
  m_data_buffer[offset] = value;
}

void CDROM::WriteDataBufferWord(uint32 offset, uint16 value)
{
  Assert((offset + 1) < m_data_buffer.size());
  m_data_buffer[offset] = Truncate8(value >> 8);
  m_data_buffer[offset + 1] = Truncate8(value);
}

void CDROM::WriteDataBufferDWord(uint32 offset, uint32 value)
{
  Assert((offset + 3) < m_data_buffer.size());
  m_data_buffer[offset] = Truncate8(value >> 24);
  m_data_buffer[offset + 1] = Truncate8(value >> 16);
  m_data_buffer[offset + 2] = Truncate8(value >> 8);
  m_data_buffer[offset + 3] = Truncate8(value);
}

void CDROM::WriteDataBufferLBA24(uint32 offset, uint32 value)
{
  Assert((offset + 3) < m_data_buffer.size());
  m_data_buffer[offset] = Truncate8(value >> 16);
  m_data_buffer[offset + 1] = Truncate8(value >> 8);
  m_data_buffer[offset + 2] = Truncate8(value);
}

void CDROM::WriteDataBufferLBA48(uint32 offset, uint64 value)
{
  Assert((offset + 5) < m_data_buffer.size());
  m_data_buffer[offset] = Truncate8(value >> 40);
  m_data_buffer[offset + 1] = Truncate8(value >> 32);
  m_data_buffer[offset + 2] = Truncate8(value >> 24);
  m_data_buffer[offset + 3] = Truncate8(value >> 16);
  m_data_buffer[offset + 4] = Truncate8(value >> 8);
  m_data_buffer[offset + 5] = Truncate8(value);
}

void CDROM::SetCommandCompletedCallback(CommandCompletedCallback callback)
{
  m_command_completed_callback = std::move(callback);
}

bool CDROM::WriteCommandBuffer(const void* data, size_t data_len)
{
  if (m_busy)
    return true;

  const byte* data_ptr = reinterpret_cast<const byte*>(data);
  for (size_t i = 0; i < data_len; i++)
  {
    m_command_buffer.push_back(*(data_ptr++));
    if (BeginCommand())
      return true;
  }

  return false;
}

bool CDROM::BeginCommand()
{
  uint8 opcode = uint8(m_command_buffer[0]);
  switch (opcode)
  {
    case SCSI_CMD_TEST_UNIT_READY:
    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
    {
      if (m_command_buffer.size() < 12)
        return false;

      QueueCommand(1);
      return true;
    }
    case SCSI_CMD_REQUEST_SENSE:
    {
      if (m_command_buffer.size() < 12)
        return false;

      QueueCommand(1);
      return true;
    }

    case SCSI_CMD_INQUIRY:
    {
      if (m_command_buffer.size() < 12)
        return false;

      QueueCommand(1);
      return true;
    }

    case SCSI_CMD_READ_CAPACITY:
    {
      if (m_command_buffer.size() < 12)
        return false;

      QueueCommand(1000);
      return true;
    }

    case SCSI_CMD_READ_TOC:
    {
      if (m_command_buffer.size() < 12)
        return false;

      QueueCommand(1000);
      return true;
    }

    case SCSI_CMD_READ_SUB_CHANNEL:
    {
      if (m_command_buffer.size() < 12)
        return false;

      QueueCommand(1000);
      return true;
    }

    case SCSI_CMD_MODE_SENSE_6:
    case SCSI_CMD_MODE_SENSE_10:
    {
      if (m_command_buffer.size() < (opcode == SCSI_CMD_MODE_SENSE_6 ? 6 : 10))
        return false;

      QueueCommand(1);
      return true;
    }

    case SCSI_CMD_READ_10:
    case SCSI_CMD_READ_12:
    {
      if (m_command_buffer.size() < (opcode == SCSI_CMD_READ_10 ? 10 : 12))
        return false;

      uint32 sector_count =
        (opcode == SCSI_CMD_READ_10) ? ZeroExtend32(ReadCommandBufferWord(7)) : ReadCommandBufferDWord(6);
      uint64 lba = ReadCommandBufferDWord(2);
      if ((lba + sector_count) > m_media.total_sectors)
      {
        AbortCommand(SENSE_ILLEGAL_REQUEST, ASC_LOGICAL_BLOCK_OUT_OF_RANGE);
        return true;
      }

      QueueCommand(CalculateSeekTime(m_current_lba, lba) + CalculateReadTime(lba, 1));
      return true;
    }

    case SCSI_CMD_READ_CD:
    {
      if (m_command_buffer.size() < 12)
        return false;

      uint64 lba = ZeroExtend64(ReadCommandBufferDWord(2));
      uint32 sector_count = ReadCommandBufferLBA24(6);
      if ((lba + sector_count) > m_media.total_sectors)
      {
        AbortCommand(SENSE_ILLEGAL_REQUEST, ASC_LOGICAL_BLOCK_OUT_OF_RANGE);
        return true;
      }

      QueueCommand(CalculateSeekTime(m_current_lba, lba) + CalculateReadTime(lba, 1));
      return true;
    }

    case SCSI_CMD_MECHANISM_STATUS:
    {
      if (m_command_buffer.size() < 12)
        return false;

      QueueCommand(1);
      return true;
    }
  }

  // Unknown command.
  Log_ErrorPrintf("Unhandled SCSI command 0x%02X", ZeroExtend32(opcode));
  m_command_buffer.clear();
  m_error = true;
  return true;
}

void CDROM::QueueCommand(CycleCount time_in_microseconds)
{
  DebugAssert(!m_command_event->IsActive());
  m_error = false;
  m_busy = true;
  m_command_event->Queue(time_in_microseconds);
}

void CDROM::ExecuteCommand()
{
  Log_TracePrintf("CDROM executing command 0x%02X length %zu", ZeroExtend32(m_command_buffer[0]),
                  m_command_buffer.size());

  m_data_buffer.clear();
  m_data_response_size = 0;

  switch (m_command_buffer[0])
  {
    case SCSI_CMD_TEST_UNIT_READY:
      HandleTestUnitReadyCommand();
      break;

    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
      HandlePreventMediumRemovalCommand();
      break;

    case SCSI_CMD_REQUEST_SENSE:
      HandleRequestSenseCommand();
      break;

    case SCSI_CMD_INQUIRY:
      HandleInquiryCommand();
      break;

    case SCSI_CMD_READ_CAPACITY:
      HandleReadCapacityCommand();
      break;

    case SCSI_CMD_READ_TOC:
      HandleReadTOCCommand();
      break;

    case SCSI_CMD_READ_SUB_CHANNEL:
      HandleReadSubChannelCommand();
      break;

    case SCSI_CMD_MODE_SENSE_6:
    case SCSI_CMD_MODE_SENSE_10:
      HandleModeSenseCommand();
      break;

    case SCSI_CMD_READ_10:
    case SCSI_CMD_READ_12:
    case SCSI_CMD_READ_CD:
      HandleReadCommand();
      break;

    case SCSI_CMD_MECHANISM_STATUS:
      HandleMechanismStatusCommand();
      break;
  }

  m_command_event->Deactivate();
}

void CDROM::CompleteCommand()
{
  m_command_buffer.clear();
  m_busy = false;
  m_error = false;
  m_remaining_sectors = 0;

  if (m_command_completed_callback)
    m_command_completed_callback();
}

void CDROM::AbortCommand(SENSE_KEY key, uint8 asc)
{
  UpdateSenseInfo(key, asc);
  m_command_buffer.clear();
  m_busy = false;
  m_error = true;
  m_remaining_sectors = 0;

  if (m_command_completed_callback)
    m_command_completed_callback();
}

CycleCount CDROM::CalculateSeekTime(uint64 current_lba, uint64 destination_lba) const
{
  return 1;
}

CycleCount CDROM::CalculateReadTime(uint64 lba, uint32 sector_count) const
{
  return 1;
}

void CDROM::HandleTestUnitReadyCommand()
{
  Log_DevPrintf("CDROM test unit ready");

  // if not ready
  if (HasMedia())
    UpdateSenseInfo(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
  else
    UpdateSenseInfo(SENSE_NO_STATUS, 0);

  CompleteCommand();
}

void CDROM::HandlePreventMediumRemovalCommand()
{
  const bool locked = ConvertToBool(ReadCommandBufferByte(4) & 1);
  Log_DevPrintf("CDROM %s tray", locked ? "lock" : "unlock");
  UpdateSenseInfo(SENSE_NO_STATUS, 0);
  CompleteCommand();
}

void CDROM::HandleRequestSenseCommand()
{
  Log_DevPrintf("CDROM sense interrupt length %u", ZeroExtend32(m_command_buffer[4]));

  AllocateData(18, m_command_buffer[4]);
  m_data_buffer[0] = 0xF0;
  m_data_buffer[1] = 0;
  m_data_buffer[2] = uint8(m_sense.key);
  std::memcpy(&m_data_buffer[3], m_sense.information, sizeof(m_sense.information));
  m_data_buffer[7] = 10;
  std::memcpy(&m_data_buffer[8], m_sense.specific_information, sizeof(m_sense.specific_information));
  m_data_buffer[12] = m_sense.asc;
  m_data_buffer[13] = m_sense.ascq;
  m_data_buffer[14] = m_sense.fruc;
  std::memcpy(&m_data_buffer[15], m_sense.key_spec, sizeof(m_sense.key_spec));

  // Clear attention flag.
  if (m_sense.key == SENSE_UNIT_ATTENTION)
    m_sense.key = SENSE_NO_STATUS;

  CompleteCommand();
}

void CDROM::HandleInquiryCommand()
{
  Log_DevPrintf("CDROM identify");

  AllocateData(36, m_command_buffer[4]);
  WriteDataBufferByte(0, 0x05); // CD-ROM
  WriteDataBufferByte(1, 0x80); // Removable
  WriteDataBufferByte(2, 0x00); // Version
  WriteDataBufferByte(3, 0x21); // ATAPI-2
  WriteDataBufferByte(4, 31);   // Length following
  WriteDataBufferByte(5, 0);
  WriteDataBufferByte(6, 0);
  WriteDataBufferByte(7, 0);

  auto PutString = [this](uint32 offset, const String& str, uint32 write_length) {
    uint32 i;
    for (i = 0; i < write_length && i < str.GetLength(); i++)
      WriteDataBufferByte(offset + i, str[i]);
    for (; i < write_length; i++)
      WriteDataBufferByte(offset + i, ' ');
  };

  PutString(8, m_vendor_id_string, 8);
  PutString(16, m_model_id_string, 16);
  PutString(32, m_firmware_version_string, 4);

  UpdateSenseInfo(SENSE_NO_STATUS, 0);
  CompleteCommand();
}

void CDROM::HandleReadCapacityCommand()
{
  Log_DevPrintf("CDROM read capacity - %u blocks", m_media.stream ? Truncate32(m_media.total_sectors) : 0);

  if (!HasMedia())
  {
    AbortCommand(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
    return;
  }

  AllocateData(8, 8);
  WriteDataBufferDWord(0, Truncate32(m_media.total_sectors));
  WriteDataBufferDWord(4, 2048); // sector size
  UpdateSenseInfo(SENSE_NO_STATUS, 0);
  CompleteCommand();
}

void CDROM::HandleReadTOCCommand()
{
  bool msf = ConvertToBool((ReadCommandBufferByte(1) >> 1) & 1);
  uint8 start_track = ReadCommandBufferByte(6);
  uint8 format = ReadCommandBufferByte(9) >> 6;
  uint16 max_length = ReadCommandBufferWord(7);

  Log_DevPrintf("CDROM read TOC msf=%u,start_track=%u,format=%u,max_length=%u", BoolToUInt32(msf),
                ZeroExtend32(start_track), ZeroExtend32(format), ZeroExtend32(max_length));

  if (!HasMedia())
  {
    AbortCommand(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
    return;
  }

  // from bochs
  switch (format)
  {
    case 0:
    {
      if (start_track > 1 && start_track != 0xAA)
      {
        AbortCommand(SENSE_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CMD_PACKET);
        return;
      }

      uint32 len = 4;
      AllocateData((start_track <= 1) ? 20 : 8, max_length);
      m_data_buffer[2] = 1;
      m_data_buffer[3] = 1;

      if (start_track <= 1)
      {
        m_data_buffer[len++] = 0;    // Reserved
        m_data_buffer[len++] = 0x14; // ADR, control
        m_data_buffer[len++] = 1;    // Track number
        m_data_buffer[len++] = 0;    // Reserved

        // Start address
        if (msf)
        {
          m_data_buffer[len++] = 0; // reserved
          m_data_buffer[len++] = 0; // minute
          m_data_buffer[len++] = 2; // second
          m_data_buffer[len++] = 0; // frame
        }
        else
        {
          m_data_buffer[len++] = 0;
          m_data_buffer[len++] = 0;
          m_data_buffer[len++] = 0;
          m_data_buffer[len++] = 0; // logical sector 0
        }

        // Lead out track
        m_data_buffer[len++] = 0;    // Reserved
        m_data_buffer[len++] = 0x16; // ADR, control
        m_data_buffer[len++] = 0xaa; // Track number
        m_data_buffer[len++] = 0;    // Reserved
      }

      uint32 blocks = uint32(m_media.total_sectors);

      // Start address
      if (msf)
      {
        m_data_buffer[len++] = 0;                                 // reserved
        m_data_buffer[len++] = uint8(((blocks + 150) / 75) / 60); // minute
        m_data_buffer[len++] = uint8(((blocks + 150) / 75) % 60); // second
        m_data_buffer[len++] = uint8((blocks + 150) % 75);        // frame;
      }
      else
      {
        m_data_buffer[len++] = uint8((blocks >> 24) & 0xff);
        m_data_buffer[len++] = uint8((blocks >> 16) & 0xff);
        m_data_buffer[len++] = uint8((blocks >> 8) & 0xff);
        m_data_buffer[len++] = uint8((blocks >> 0) & 0xff);
      }
      m_data_buffer[0] = uint8(((len - 2) >> 8) & 0xff);
      m_data_buffer[1] = uint8((len - 2) & 0xff);

      UpdateSenseInfo(SENSE_NO_STATUS, 0);
      CompleteCommand();
    }
    break;

    case 1:
    {
      // emulate a single session
      AllocateData(12, max_length);
      m_data_buffer[0] = 0;
      m_data_buffer[1] = 0x0A;
      m_data_buffer[2] = 1;
      m_data_buffer[3] = 1;
      for (uint32 i = 0; i < 8; i++)
        m_data_buffer[4 + i] = 0;

      UpdateSenseInfo(SENSE_NO_STATUS, 0);
      CompleteCommand();
    }
    break;

    case 2:
    {
      // raw toc
      uint32 len = 4;
      m_data_buffer[2] = 1;
      m_data_buffer[3] = 1;

      for (uint32 i = 0; i < 4; i++)
      {
        m_data_buffer[len++] = 1;
        m_data_buffer[len++] = 0x14;
        m_data_buffer[len++] = 0;
        if (i < 3)
        {
          m_data_buffer[len++] = uint8(0xa0 + i);
        }
        else
        {
          m_data_buffer[len++] = 1;
        }
        m_data_buffer[len++] = 0;
        m_data_buffer[len++] = 0;
        m_data_buffer[len++] = 0;
        if (i < 2)
        {
          m_data_buffer[len++] = 0;
          m_data_buffer[len++] = 1;
          m_data_buffer[len++] = 0;
          m_data_buffer[len++] = 0;
        }
        else if (i == 2)
        {
          uint32 blocks = uint32(m_media.total_sectors);

          if (msf)
          {
            m_data_buffer[len++] = 0;                                 // reserved
            m_data_buffer[len++] = uint8(((blocks + 150) / 75) / 60); // minute
            m_data_buffer[len++] = uint8(((blocks + 150) / 75) % 60); // second
            m_data_buffer[len++] = uint8((blocks + 150) % 75);        // frame;
          }
          else
          {
            m_data_buffer[len++] = uint8((blocks >> 24) & 0xff);
            m_data_buffer[len++] = uint8((blocks >> 16) & 0xff);
            m_data_buffer[len++] = uint8((blocks >> 8) & 0xff);
            m_data_buffer[len++] = uint8((blocks >> 0) & 0xff);
          }
        }
        else
        {
          m_data_buffer[len++] = 0;
          m_data_buffer[len++] = 0;
          m_data_buffer[len++] = 0;
          m_data_buffer[len++] = 0;
        }
      }
      m_data_buffer[0] = uint8(((len - 2) >> 8) & 0xff);
      m_data_buffer[1] = uint8((len - 2) & 0xff);

      UpdateSenseInfo(SENSE_NO_STATUS, 0);
      CompleteCommand();
    }
    break;

    default:
      AbortCommand(SENSE_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CMD_PACKET);
      break;
  }
}

void CDROM::HandleReadSubChannelCommand()
{
  bool msf = ConvertToBool((ReadCommandBufferByte(1) >> 1) & 1);
  bool sub_q = ConvertToBool((ReadCommandBufferByte(2) >> 6) & 1);
  uint8 data_format = ReadCommandBufferByte(3);
  uint8 track_number = ReadCommandBufferByte(6);
  uint16 max_length = ReadCommandBufferWord(7);

  Log_DevPrintf("CDROM read subchannel msf=%u,sub_q=%u,data_format=%u,track_number=%u,max_length=%u", BoolToUInt32(msf),
                BoolToUInt32(sub_q), ZeroExtend32(data_format), ZeroExtend32(track_number), ZeroExtend32(max_length));

  if (!HasMedia())
  {
    AbortCommand(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
    return;
  }

  AllocateData((sub_q && (data_format == 2 || data_format == 3)) ? 24 : 4, max_length);

  m_data_buffer[0] = 0;
  m_data_buffer[1] = 0;
  m_data_buffer[2] = 0;
  m_data_buffer[3] = 0;

  if (sub_q)
  {
    if (data_format == 2 || data_format == 3)
    {
      m_data_buffer[4] = data_format;
      if (data_format == 3)
      {
        m_data_buffer[5] = 0x14;
        m_data_buffer[6] = 0x01;
      }
      else
      {
        m_data_buffer[5] = 0x00;
        m_data_buffer[6] = 0x00;
      }
      m_data_buffer[7] = 0;
      m_data_buffer[8] = 0;
      std::memset(&m_data_buffer[9], 0, 15);
    }
    else
    {
      AbortCommand(SENSE_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CMD_PACKET);
      return;
    }
  }

  UpdateSenseInfo(SENSE_NO_STATUS, 0);
  CompleteCommand();
}

void CDROM::HandleModeSenseCommand()
{
  // from bochs
  uint8 pc = (ReadCommandBufferByte(2) >> 6);
  uint8 page_code = (ReadCommandBufferByte(2) & 0x3F);
  uint16 max_length =
    (m_command_buffer[0] == SCSI_CMD_MODE_SENSE_10) ? ReadCommandBufferWord(7) : ZeroExtend16(ReadCommandBufferByte(4));
  Log_DevPrintf("CDROM mode sense pc=%u,page_code=%u,max_length=%u", ZeroExtend32(pc), ZeroExtend32(page_code),
                ZeroExtend32(max_length));

  auto InitResponse = [this, max_length](uint32 size) {
    AllocateData(8 + size, max_length);
    m_data_buffer[0] = Truncate8((size + 6) >> 8);
    m_data_buffer[1] = Truncate8(size + 6);

    if (HasMedia())
      m_data_buffer[2] = 0x12; // media present 120mm CD-ROM (CD-R) data/audio  door closed
    else
      m_data_buffer[2] = 0x70; // no media present

    m_data_buffer[3] = 0;
    m_data_buffer[4] = 0;
    m_data_buffer[5] = 0;
    m_data_buffer[6] = 0;
    m_data_buffer[7] = 0;
  };

  switch (pc)
  {
    case 0: // current values
    case 1: // default values
    {
      switch (page_code)
      {
        case 0x01: // error recovery
        {
          InitResponse(8);
          std::memset(&m_data_buffer[8], 0, 8);
          UpdateSenseInfo(SENSE_NO_STATUS, 0);
          CompleteCommand();
          return;
        }

        case 0x2a: // capabilities/mech status
        {
          InitResponse(20);
          m_data_buffer[8] = 0x2a;
          m_data_buffer[9] = 0x12;
          m_data_buffer[10] = 0x03;
          m_data_buffer[11] = 0x00;
          // Multisession, Mode 2 Form 2, Mode 2 Form 1, Audio
          m_data_buffer[12] = 0x71;
          m_data_buffer[13] = (3 << 5);
          m_data_buffer[14] = uint8(1 | (m_tray_locked ? (1 << 1) : 0) | (1 << 3) | (1 << 5));
          m_data_buffer[15] = 0x00;
          m_data_buffer[16] = ((16 * 176) >> 8) & 0xff;
          m_data_buffer[17] = (16 * 176) & 0xff;
          m_data_buffer[18] = 0;
          m_data_buffer[19] = 2;
          m_data_buffer[20] = (512 >> 8) & 0xff;
          m_data_buffer[21] = 512 & 0xff;
          m_data_buffer[22] = ((16 * 176) >> 8) & 0xff;
          m_data_buffer[23] = (16 * 176) & 0xff;
          m_data_buffer[24] = 0;
          m_data_buffer[25] = 0;
          m_data_buffer[26] = 0;
          m_data_buffer[27] = 0;
          UpdateSenseInfo(SENSE_NO_STATUS, 0);
          CompleteCommand();
          return;
        }
      }
    }
  }

  Log_ErrorPrintf("mode sense not implemented pc=%u,page_code=%u,max_length=%u", ZeroExtend32(pc),
                  ZeroExtend32(page_code), ZeroExtend32(max_length));
  AbortCommand(SENSE_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CMD_PACKET);
}

void CDROM::HandleReadCommand()
{
  if (!HasMedia())
  {
    AbortCommand(SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
    return;
  }

  uint32 lba;
  uint32 sector_count;
  switch (m_command_buffer[0])
  {
    case SCSI_CMD_READ_10:
      lba = ReadCommandBufferDWord(2);
      sector_count = ZeroExtend32(ReadCommandBufferWord(7));
      break;
    case SCSI_CMD_READ_12:
      lba = ReadCommandBufferDWord(2);
      sector_count = ReadCommandBufferDWord(6);
      break;
    case SCSI_CMD_READ_CD:
    {
      lba = ReadCommandBufferDWord(2);
      sector_count = ReadCommandBufferLBA24(6);
      if ((m_command_buffer[9] & 0xF8) == 0x00)
      {
        // No transfer.
        CompleteCommand();
        return;
      }

      // Only support data reads for now.
      if ((m_command_buffer[9] & 0xF8) != 0x10)
      {
        AbortCommand(SENSE_ILLEGAL_REQUEST, ASC_INVALID_FIELD_IN_CMD_PACKET);
        return;
      }
    }
    break;

    default:
      AbortCommand(SENSE_ILLEGAL_REQUEST, ASC_ILLEGAL_OPCODE);
      return;
  }

  Log_DevPrintf("CDROM read %u sectors at LBA %u", sector_count, lba);

  // Transfers of length zero are fine.
  m_remaining_sectors = sector_count;
  if (sector_count == 0)
  {
    UpdateSenseInfo(SENSE_NO_STATUS, 0);
    CompleteCommand();
    return;
  }

  // Read a single sector at a time.
  AllocateData(SECTOR_SIZE, SECTOR_SIZE);
  if (!m_media.stream->SeekAbsolute(lba * uint64(m_data_response_size)) ||
      !m_media.stream->Read2(m_data_buffer.data(), m_data_response_size))
  {
    Log_ErrorPrintf("CDROM read error at LBA %u", uint32(lba));
    AbortCommand(SENSE_ILLEGAL_REQUEST, ASC_MEDIUM_NOT_PRESENT);
    return;
  }

  UpdateSenseInfo(SENSE_NO_STATUS, 0);
  m_current_lba = lba + 1;
  m_remaining_sectors--;

  // Single sector transfer?
  if (m_remaining_sectors == 0)
  {
    CompleteCommand();
    return;
  }

  // Multiple sector transfer. Keep the busy flag set.
  m_error = false;
  m_busy = true;
  if (m_command_completed_callback)
    m_command_completed_callback();
}

bool CDROM::TransferNextSector()
{
  // Read next sector.
  AllocateData(SECTOR_SIZE, SECTOR_SIZE);
  if (!m_media.stream->Read2(m_data_buffer.data(), m_data_response_size))
  {
    Log_ErrorPrintf("CDROM read error at LBA %u", uint32(m_current_lba));
    AbortCommand(SENSE_ILLEGAL_REQUEST, ASC_MEDIUM_NOT_PRESENT);
    return false;
  }

  m_current_lba++;
  m_remaining_sectors--;

  // Done?
  if (m_remaining_sectors == 0)
  {
    CompleteCommand();
    return true;
  }

  // >1 remaining sector.
  if (m_command_completed_callback)
    m_command_completed_callback();

  return true;
}

void CDROM::HandleMechanismStatusCommand()
{
  Log_DevPrintf("CDROM read mechanism status");

  uint16 max_length = ReadCommandBufferWord(8);
  AllocateData(8, max_length);

  WriteDataBufferByte(0, (0 << 7) /* fault */ | (0 << 5) /* changer state */ | (0 << 0) /* current slot */);
  WriteDataBufferByte(1, (0 << 5) /* CD mechanism state */);
  WriteDataBufferLBA24(2, Truncate32(m_current_lba));
  WriteDataBufferByte(5, 0 /* number of slots available */);
  WriteDataBufferWord(6, 0 /* length of slot tables */);

  UpdateSenseInfo(SENSE_NO_STATUS, 0);
  CompleteCommand();
}

} // namespace HW
