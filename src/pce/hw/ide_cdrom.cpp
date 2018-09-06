#include "ide_cdrom.h"
#include "YBaseLib/Log.h"
#include "hdc.h"
#include "pce/system.h"
Log_SetChannel(HW::IDECDROM);

namespace HW {

DEFINE_OBJECT_TYPE_INFO(IDECDROM);
DEFINE_GENERIC_COMPONENT_FACTORY(IDECDROM);
BEGIN_OBJECT_PROPERTY_MAP(IDECDROM)
PROPERTY_TABLE_MEMBER_UINT("Channel", 0, offsetof(IDECDROM, m_ide_channel), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Drive", 0, offsetof(IDECDROM, m_ide_drive), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

IDECDROM::IDECDROM(const String& identifier, u32 ide_channel /* = 0 */, u32 ide_device /* = 0 */,
                   const ObjectTypeInfo* type_info /* = &s_type_info */)
  : m_ide_channel(ide_channel), m_ide_drive(ide_device), BaseClass(identifier, type_info)
{
}

bool IDECDROM::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  HW::HDC* hdc = system->GetComponentByType<HDC>(m_ide_channel);
  if (!hdc)
  {
    Log_ErrorPrintf("Failed to find IDE channel %u", m_ide_channel);
    return false;
  }

  // if (!hdc->AttachATAPIDevice(m_ide_drive, this))
  {
    Log_ErrorPrintf("Failed to attach CDROM to IDE channel %u device %u", m_ide_channel, m_ide_drive);
    return false;
  }

  return true;
}

} // namespace HW
