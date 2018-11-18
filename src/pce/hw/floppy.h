#pragma once
#include "../component.h"
#include <vector>

class ByteStream;
class Error;

namespace HW {

// TODO: Move common stuff to "base HDD" class, like CDROM.

class Floppy final : public Component
{
  DECLARE_OBJECT_TYPE_INFO(Floppy, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(Floppy);
  DECLARE_OBJECT_PROPERTY_MAP(Floppy);

public:
  enum DriveType : u32
  {
    DriveType_None,
    DriveType_5_25,
    DriveType_3_5,
    DriveType_Count
  };

  enum DiskType : u32
  {
    DiskType_None,

    DiskType_160K,  // 5.25-inch, single-sided, 40 tracks, 8 sectors per track
    DiskType_180K,  // 5.25-inch, single-sided, 40 tracks, 9 sectors per track
    DiskType_320K,  // 5.25-inch, double-sided, 40 tracks, 8 sectors per track
    DiskType_360K,  // 5.25-inch, double-sided, 40 tracks, 9 sectors per track
    DiskType_640K,  // 5.25-inch, double-sided, 80 tracks, 8 sectors per track
    DiskType_720K,  // 3.5-inch, double-sided, 80 tracks, 9 sectors per track
    DiskType_1220K, // 5.25-inch, double-sided, 80 tracks, 15 sectors per track
    DiskType_1440K, // 3.5-inch, double-sided, 80 tracks, 18 sectors per track
    DiskType_1680K, // 3.5-inch, double-sided, 80 tracks, 21 sectors per track, "DMF"
    DiskType_1840K, // 3.5-inch, double-sided, 80 tracks, 23 sectors per track, "XDF"
    DiskType_2880K, // 3.5-inch, double-sided, 80 tracks, 36 sectors per track

    DiskType_Count
  };

public:
  Floppy(const String& identifier, DriveType type = DriveType_5_25, u32 drive_number = 0,
         const ObjectTypeInfo* type_info = &s_type_info);

  static DiskType DetectDiskType(const char* filename, Error* error);
  static DiskType DetectDiskType(ByteStream* stream, Error* error);
  static DriveType GetDriveTypeForDiskType(DiskType type);

  virtual bool Initialize(System* system, Bus* bus) override;
  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

  // Renamed due to winapi conflicts
  DriveType GetDriveType_() { return m_drive_type; }
  DiskType GetDiskType() { return m_disk_type; }
  u32 GetDriveNumber() const { return m_drive_number; }

  u32 GetNumTracks() const { return m_tracks; }
  u32 GetNumHeads() const { return m_heads; }
  u32 GetSectorsPerTrack() const { return m_sectors_per_track; }
  u32 GetTotalSectors() const { return m_total_sectors; }

  // TODO: Write protect
  bool IsDiskInserted() const { return !m_image_data.empty(); }

  void SetActivity(bool writing);
  void ClearActivity();

  void Read(void* buffer, u32 offset, u32 size);
  void Write(const void* buffer, u32 offset, u32 size);

  bool InsertDisk(const char* filename, Error* error);
  bool InsertDisk(const char* filename, ByteStream* stream, Error* error);
  void EjectDisk();

  bool SaveImage(Error* error);
  bool SaveImage(const char* filename, Error* error);

private:
  static constexpr u32 SECTOR_SIZE = 512;

  DriveType m_drive_type;
  u32 m_drive_number;

  DiskType m_disk_type = DiskType_None;

  u32 m_tracks = 0;
  u32 m_heads = 0;
  u32 m_sectors_per_track = 0;
  u32 m_total_sectors = 0;

  String m_image_filename;
  std::vector<byte> m_image_data;
};

} // namespace HW
