#pragma once
#include "cdrom.h"

namespace HW {

class ATACDROM final : public CDROM
{
  DECLARE_OBJECT_TYPE_INFO(ATACDROM, CDROM);
  DECLARE_GENERIC_COMPONENT_FACTORY(ATACDROM);
  DECLARE_OBJECT_PROPERTY_MAP(ATACDROM);

public:
  ATACDROM(const String& identifier, u32 ide_channel = 0, u32 ide_device = 0,
           const ObjectTypeInfo* type_info = &s_type_info);

  virtual bool Initialize(System* system, Bus* bus);

private:
  u32 m_ide_channel;
  u32 m_ide_drive;
};

} // namespace HW
