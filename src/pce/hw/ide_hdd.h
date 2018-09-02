#pragma once
#include "../component.h"

class HDDImage;

namespace HW {

// TODO: Move common stuff to "base HDD" class, like CDROM.

class IDEHDD final : public Component
{
  DECLARE_OBJECT_TYPE_INFO(IDEHDD, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(IDEHDD);
  DECLARE_OBJECT_PROPERTY_MAP(IDEHDD);

public:
  IDEHDD(const String& identifier, const char* image_filename = "", u32 cylinders = 0, u32 heads = 0, u32 sectors = 0,
         u32 ide_channel = 0, u32 ide_device = 0, const ObjectTypeInfo* type_info = &s_type_info);

  virtual bool Initialize(System* system, Bus* bus) override;
  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

  HDDImage* GetImage() const { return m_image.get(); }

  u64 GetNumLBAs() const { return m_lbas; }
  u32 GetNumCylinders() const { return m_cylinders; }
  u32 GetNumHeads() const { return m_heads; }
  u32 GetNumSectors() const { return m_sectors; }

  u32 GetIDEChannelNumber() const { return m_ide_channel; }
  u32 GetIDEDriveNumber() const { return m_ide_drive; }

  void SetActivity(bool reading, bool writing);

private:
  static const u32 SECTOR_SIZE = 512;

  String m_image_filename;
  std::unique_ptr<HDDImage> m_image;

  u64 m_lbas;
  u32 m_cylinders;
  u32 m_heads;
  u32 m_sectors;

  u32 m_ide_channel;
  u32 m_ide_drive;
};

} // namespace HW
