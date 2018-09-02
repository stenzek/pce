#include "ide_hdd.h"
#include "YBaseLib/Log.h"
#include "hdc.h"
#include "pce/system.h"
Log_SetChannel(HW::IDEHDD);

namespace HW {

DEFINE_OBJECT_TYPE_INFO(IDEHDD);
DEFINE_GENERIC_COMPONENT_FACTORY(IDEHDD);
BEGIN_OBJECT_PROPERTY_MAP(IDEHDD)
PROPERTY_TABLE_MEMBER_UINT("Channel", 0, offsetof(IDEHDD, m_ide_channel), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Drive", 0, offsetof(IDEHDD, m_ide_drive), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("ImageFile", 0, offsetof(IDEHDD, m_image_filename), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Cylinders", 0, offsetof(IDEHDD, m_cyclinders), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Heads", 0, offsetof(IDEHDD, m_heads), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Sectors", 0, offsetof(IDEHDD, m_sectors), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

IDEHDD::IDEHDD(const String& identifier, const char* image_filename /* = "" */, u32 cylinders /* = 0 */,
               u32 heads /* = 0 */, u32 sectors /* = 0 */, u32 ide_channel /* = 0 */, u32 ide_device /* = 0 */,
               const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_cyclinders(cylinders), m_heads(heads), m_sectors(sectors),
    m_ide_channel(ide_channel), m_ide_drive(ide_device)
{
}

bool IDEHDD::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  HW::HDC* hdc = system->GetComponentByType<HDC>(m_ide_channel);
  if (!hdc)
  {
    Log_ErrorPrintf("Failed to find IDE channel %u", m_ide_channel);
    return false;
  }

  if (!hdc->AttachDrive(m_ide_drive, m_image_filename, m_cyclinders, m_heads, m_sectors))
  {
    Log_ErrorPrintf("Failed to attach HDD to IDE channel %u device %u", m_ide_channel, m_ide_drive);
    return false;
  }

  return true;
}

} // namespace HW
