#pragma once
#include "cdrom.h"

namespace HW {

class IDECDROM final : public CDROM
{
  DECLARE_OBJECT_TYPE_INFO(IDECDROM, CDROM);
  DECLARE_GENERIC_COMPONENT_FACTORY(IDECDROM);
  DECLARE_OBJECT_PROPERTY_MAP(IDECDROM);

public:
  IDECDROM(const String& identifier, u32 ide_channel = 0, u32 ide_device = 0,
           const ObjectTypeInfo* type_info = &s_type_info);

  virtual bool Initialize(System* system, Bus* bus);

private:
  u32 m_ide_channel;
  u32 m_ide_drive;
};

} // namespace HW
