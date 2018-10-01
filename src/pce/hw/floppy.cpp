#include "floppy.h"
#include "../host_interface.h"
#include "../system.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/Log.h"
#include "common/hdd_image.h"
#include "fdc.h"
Log_SetChannel(HW::Floppy);

namespace HW {

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
  Floppy::DriveType drive_type;
  uint32 size;
  uint32 num_tracks;
  uint32 num_heads;
  uint32 num_sectors_per_track;
  uint8 media_descriptor_byte;
};
static const DiskTypeInfo disk_types[Floppy::DiskType_Count] = {
  {Floppy::DriveType_None, 0, 0, 0, 0, 0x00},          // DiskType_None
  {Floppy::DriveType_5_25, 163840, 40, 1, 8, 0xFE},    // DiskType_160K
  {Floppy::DriveType_5_25, 184320, 40, 1, 9, 0xFC},    // DiskType_180K
  {Floppy::DriveType_5_25, 327680, 40, 2, 8, 0xFF},    // DiskType_320K
  {Floppy::DriveType_5_25, 368640, 40, 2, 9, 0xFD},    // DiskType_360K
  {Floppy::DriveType_5_25, 655360, 80, 2, 8, 0xFB},    // DiskType_640K
  {Floppy::DriveType_3_5, 737280, 80, 2, 9, 0xF9},     // DiskType_720K
  {Floppy::DriveType_5_25, 1310720, 80, 2, 15, 0xF9}, // DiskType_1220K
  {Floppy::DriveType_3_5, 1474560, 80, 2, 18, 0xF0},  // DiskType_1440K
  {Floppy::DriveType_3_5, 1720320, 80, 2, 21, 0xF0},  // DiskType_1680K
  {Floppy::DriveType_3_5, 2949120, 80, 2, 36, 0xF0}   // DiskType_2880K
};

DEFINE_OBJECT_TYPE_INFO(Floppy);
DEFINE_GENERIC_COMPONENT_FACTORY(Floppy);
BEGIN_OBJECT_PROPERTY_MAP(Floppy)
PROPERTY_TABLE_MEMBER_UINT("Drive", 0, offsetof(Floppy, m_drive_number), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("DriveType", 0, offsetof(Floppy, m_drive_type), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("ImageFile", 0, offsetof(Floppy, m_image_filename), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

Floppy::Floppy(const String& identifier, DriveType type /* = DriveType_5_25 */, u32 drive_number /* = 0 */,
               const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_drive_type(type), m_drive_number(drive_number)
{
}

Floppy::DiskType Floppy::DetectDiskType(const char* filename, Error* error)
{
  ByteStream* stream = FileSystem::OpenFile(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
  if (!stream)
  {
    error->SetErrorUserFormatted(0, "Failed to open '%s'", filename);
    return DiskType_None;
  }

  DiskType type = DetectDiskType(stream, error);
  stream->Release();
  return type;
}

Floppy::DiskType Floppy::DetectDiskType(ByteStream* stream, Error* error)
{
  const u64 old_position = stream->GetPosition();

  // Get file size
  uint32 file_size = static_cast<uint32>(stream->GetSize());
  Log_DevPrintf("Disk size: %u bytes", file_size);

  // Check for FAT header
  FAT_HEADER fat_header;
  if (!stream->SeekAbsolute(0) || !stream->Read2(&fat_header, sizeof(fat_header)))
  {
    stream->SeekAbsolute(old_position);
    error->SetErrorUserFormatted(0, "Failed to read FAT header");
    return DiskType_None;
  }

  stream->SeekAbsolute(old_position);

  // Validate FAT header
  if (fat_header.signature == 0xAA55)
  {
    Log_DevPrintf("FAT detected, media descriptor = 0x%02X", fat_header.media_descriptor_type);

    // Use media descriptor byte to find a matching type
    for (u32 i = 0; i < DiskType_Count; i++)
    {
      if (fat_header.media_descriptor_type == disk_types[i].media_descriptor_byte && file_size <= disk_types[i].size)
        return static_cast<DiskType>(i);
    }
  }

  // Use size alone to find a matching type
  for (u32 i = 0; i < DiskType_Count; i++)
  {
    if (file_size <= disk_types[i].size)
      return static_cast<DiskType>(i);
  }

  // Unknown
  error->SetErrorUserFormatted(0, "Unknown disk size: %u bytes", file_size);
  return DiskType_None;
}

Floppy::DriveType Floppy::GetDriveTypeForDiskType(DiskType type)
{
  return (type >= DiskType_Count) ? DriveType_None : disk_types[type].drive_type;
}

bool Floppy::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  HW::FDC* fdc = system->GetComponentByType<FDC>();
  if (!fdc)
  {
    Log_ErrorPrintf("Failed to find floppy controller");
    return false;
  }
  else if (fdc->IsDrivePresent(m_drive_number))
  {
    Log_ErrorPrintf("Floppy controller already has a drive %u attached", m_drive_number);
    return false;
  }

  if (!fdc->AttachDrive(m_drive_number, this))
    return false;

  if (!m_image_filename.IsEmpty())
  {
    Error error;
    if (!InsertDisk(m_image_filename, &error))
    {
      system->GetHostInterface()->ReportFormattedError("Failed to insert '%s' into drive %u: %s",
                                                       m_image_filename.GetCharArray(), m_drive_number,
                                                       error.GetErrorCodeAndDescription().GetCharArray());
    }
  }

  Log_DevPrintf("Attached floppy drive %u (%s)", m_drive_number, (m_drive_type == DriveType_3_5) ? "3.5\"" : "5.25\"");

  // Create indicator and menu options.
  system->GetHostInterface()->AddUIIndicator(this, HostInterface::IndicatorType::FDD);
  system->GetHostInterface()->AddUIFileCallback(this, "Insert Disk...", [this](const String& filename) {
    Error error;
    if (!InsertDisk(filename, &error))
    {
      m_system->GetHostInterface()->ReportFormattedError("Failed to insert image: %s",
                                                         error.GetErrorCodeAndDescription().GetCharArray());
    }
  });
  system->GetHostInterface()->AddUIFileCallback(this, "Save Image...", [this](const String& filename) {
    Error error;
    if (!SaveImage(filename, &error))
    {
      m_system->GetHostInterface()->ReportFormattedError("Failed to save floppy image: %s",
                                                         error.GetErrorCodeAndDescription().GetCharArray());
    }
  });
  system->GetHostInterface()->AddUICallback(this, "Save Image to Existing File", [this]() {
    Error error;
    if (!SaveImage(&error))
    {
      m_system->GetHostInterface()->ReportFormattedError("Failed to save floppy image: %s",
                                                         error.GetErrorCodeAndDescription().GetCharArray());
    }
  });
  system->GetHostInterface()->AddUICallback(this, "Eject Disk", [this]() { EjectDisk(); });
  return true;
}

bool Floppy::LoadState(BinaryReader& reader)
{
  u32 type = reader.ReadUInt32();
  u32 drive_number = reader.ReadUInt32();
  if (type != static_cast<u32>(m_drive_type) || drive_number != m_drive_number)
    return false;

  m_disk_type = static_cast<DiskType>(reader.ReadUInt32());
  m_tracks = reader.ReadUInt32();
  m_heads = reader.ReadUInt32();
  m_sectors_per_track = reader.ReadUInt32();
  m_total_sectors = reader.ReadUInt32();
  m_image_filename = reader.ReadCString();

  u32 data_size = reader.ReadUInt32();
  if (reader.GetErrorState())
    return false;

  m_image_data.clear();
  if (data_size > 0)
  {
    m_image_data.resize(data_size);
    if (!reader.SafeReadBytes(m_image_data.data(), data_size))
      return false;
  }

  return true;
}

bool Floppy::SaveState(BinaryWriter& writer)
{
  bool result = true;

  result &= writer.SafeWriteUInt32(static_cast<u32>(m_drive_type));
  result &= writer.SafeWriteUInt32(m_drive_number);
  result &= writer.SafeWriteUInt32(static_cast<u32>(m_disk_type));
  result &= writer.SafeWriteUInt32(m_tracks);
  result &= writer.SafeWriteUInt32(m_heads);
  result &= writer.SafeWriteUInt32(m_sectors_per_track);
  result &= writer.SafeWriteUInt32(m_total_sectors);
  result &= writer.SafeWriteCString(m_image_filename);
  result &= writer.SafeWriteUInt32(static_cast<u32>(m_image_data.size()));
  if (!result)
    return false;

  if (!writer.SafeWriteBytes(m_image_data.data(), static_cast<u32>(m_image_data.size())))
    return false;

  return true;
}

void Floppy::SetActivity(bool writing)
{
  // writing > reading > idle
  m_system->GetHostInterface()->SetUIIndicatorState(
    this, (writing ? HostInterface::IndicatorState::Writing : HostInterface::IndicatorState::Reading));
}
void Floppy::ClearActivity()
{
  m_system->GetHostInterface()->SetUIIndicatorState(this, HostInterface::IndicatorState::Off);
}

void Floppy::Read(void* buffer, u32 offset, u32 size)
{
  Assert(!m_image_data.empty() && (offset + size) <= m_image_data.size());
  std::memcpy(buffer, &m_image_data[offset], size);
}

void Floppy::Write(const void* buffer, u32 offset, u32 size)
{
  Assert(!m_image_data.empty() && (offset + size) <= m_image_data.size());
  std::memcpy(&m_image_data[offset], buffer, size);
}

bool Floppy::InsertDisk(const char* filename, Error* error)
{
  ByteStream* stream = FileSystem::OpenFile(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
  if (!stream)
  {
    error->SetErrorUserFormatted(0, "Failed to open '%s'", filename);
    return false;
  }

  const bool result = InsertDisk(filename, stream, error);
  stream->Release();
  return result;
}

bool Floppy::InsertDisk(const char* filename, ByteStream* stream, Error* error)
{
  DiskType disk_type = DetectDiskType(stream, error);
  if (disk_type == DiskType_None)
  {
    return false;
  }
  else if (GetDriveTypeForDiskType(disk_type) != m_drive_type)
  {
    error->SetErrorUserFormatted(0, "Incompatible drive type for disk type");
    return false;
  }

  if (IsDiskInserted())
    EjectDisk();

  // DetectDiskType seeks to zero.
  m_image_data.resize(static_cast<u32>(stream->GetSize()));
  if (!stream->Read2(m_image_data.data(), static_cast<u32>(stream->GetSize())))
  {
    error->SetErrorUserFormatted(0, "Read error");
    return false;
  }

  const DiskTypeInfo* info = &disk_types[disk_type];
  m_disk_type = disk_type;
  m_tracks = info->num_tracks;
  m_heads = info->num_heads;
  m_sectors_per_track = info->num_sectors_per_track;
  m_total_sectors = static_cast<u32>(m_image_data.size() / SECTOR_SIZE);
  if (filename)
    m_image_filename = filename;
  else
    m_image_filename.Clear();

  m_system->GetHostInterface()->ReportFormattedMessage("Inserted image '%s' into floppy '%s'", filename,
                                                       m_identifier.GetCharArray());
  return true;
}

void Floppy::EjectDisk()
{
  m_disk_type = DiskType_None;
  m_tracks = 0;
  m_heads = 0;
  m_sectors_per_track = 0;
  m_total_sectors = 0;
  m_image_filename.Clear();
  m_image_data.clear();
}

bool Floppy::SaveImage(Error* error)
{
  if (m_image_filename.IsEmpty())
  {
    error->SetErrorUserFormatted(0, "No image file is set.");
    return false;
  }

  return SaveImage(m_image_filename, error);
}

bool Floppy::SaveImage(const char* filename, Error* error)
{
  if (IsDiskInserted())
  {
    error->SetErrorUserFormatted(0, "No disk is inserted.");
    return false;
  }

  ByteStream* stream =
    FileSystem::OpenFile(filename, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_WRITE |
                                     BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE);
  if (!stream)
  {
    error->SetErrorUserFormatted(0, "Failed to open file '%s'", filename);
    return false;
  }

  if (!stream->Write2(m_image_data.data(), static_cast<u32>(m_image_data.size())))
  {
    error->SetErrorUserFormatted(0, "Write error");
    stream->Discard();
    stream->Release();
    return false;
  }

  if (!stream->Commit())
  {
    error->SetErrorUserFormatted(0, "Failed to commit image file");
    return false;
  }

  stream->Release();
  m_system->GetHostInterface()->ReportFormattedMessage("Floppy '%s' saved to '%s'", m_identifier.GetCharArray(),
                                                       filename);
  return true;
}

} // namespace HW
