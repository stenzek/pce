#pragma once
#include "../component.h"

namespace HW {

class IDEHDD final : public Component
{
  DECLARE_OBJECT_TYPE_INFO(IDEHDD, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(IDEHDD);
  DECLARE_OBJECT_PROPERTY_MAP(IDEHDD);

public:
  IDEHDD(const String& identifier, const char* image_filename = "", u32 cylinders = 0, u32 heads = 0, u32 sectors = 0,
         u32 ide_channel = 0, u32 ide_device = 0, const ObjectTypeInfo* type_info = &s_type_info);

  virtual bool Initialize(System* system, Bus* bus);

private:
  String m_image_filename;

  u32 m_cyclinders;
  u32 m_heads;
  u32 m_sectors;

  u32 m_ide_channel;
  u32 m_ide_drive;
};

} // namespace HW
