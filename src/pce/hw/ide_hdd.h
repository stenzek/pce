#pragma once
#include "ata_device.h"

class HDDImage;

namespace HW {

// TODO: Move common stuff to "base HDD" class, like CDROM.

class IDEHDD final : public ATADevice
{
  DECLARE_OBJECT_TYPE_INFO(IDEHDD, ATADevice);
  DECLARE_GENERIC_COMPONENT_FACTORY(IDEHDD);
  DECLARE_OBJECT_PROPERTY_MAP(IDEHDD);

public:
  IDEHDD(const String& identifier, const char* image_filename = "", u32 cylinders = 0, u32 heads = 0, u32 sectors = 0,
         u32 ide_channel = 0, u32 ide_device = 0, const ObjectTypeInfo* type_info = &s_type_info);

  virtual bool Initialize(System* system, Bus* bus) override;
  virtual void Reset() override;

  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

  HDDImage* GetImage() const { return m_image.get(); }

  u64 GetNumLBAs() const { return m_lbas; }
  u32 GetNumCylinders() const { return m_cylinders; }
  u32 GetNumHeads() const { return m_heads; }
  u32 GetNumSectors() const { return m_sectors; }

  u32 GetIDEChannelNumber() const { return m_ide_channel; }
  u32 GetIDEDriveNumber() const { return m_ide_drive; }

  void SetActivity(bool writing);
  void ClearActivity();

protected:
  void DoReset(bool is_hardware_reset) override;

private:
  static const u32 SECTOR_SIZE = 512;

  void SetSignature();
  void SetupBuffer(u32 num_sectors);

  String m_image_filename;
  std::unique_ptr<HDDImage> m_image;

  u64 m_lbas;
  u32 m_cylinders;
  u32 m_heads;
  u32 m_sectors;

  u32 m_ide_channel;
  u32 m_ide_drive;

  // parameters in current translation mode
  u32 m_current_num_cylinders = 0;
  u32 m_current_num_heads = 0;
  u32 m_current_num_sectors = 0;

  // current seek position
  // sector is one-based as per convention
  u32 m_current_cylinder = 0;
  u32 m_current_head = 0;
  u32 m_current_sector = 1;
  u64 m_current_lba = 0;

  // Current command being executed.
  u8 m_current_command = 0;

  struct
  {
    std::vector<byte> data;
    size_t position;

  } m_buffer = {};
};

} // namespace HW
