#include "ide_hdd.h"
#include "../host_interface.h"
#include "../system.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "common/hdd_image.h"
#include "hdc.h"
Log_SetChannel(HW::IDEHDD);

namespace HW {

DEFINE_OBJECT_TYPE_INFO(IDEHDD);
DEFINE_GENERIC_COMPONENT_FACTORY(IDEHDD);
BEGIN_OBJECT_PROPERTY_MAP(IDEHDD)
PROPERTY_TABLE_MEMBER_UINT("Channel", 0, offsetof(IDEHDD, m_ide_channel), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Drive", 0, offsetof(IDEHDD, m_ide_drive), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("ImageFile", 0, offsetof(IDEHDD, m_image_filename), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Cylinders", 0, offsetof(IDEHDD, m_cylinders), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Heads", 0, offsetof(IDEHDD, m_heads), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Sectors", 0, offsetof(IDEHDD, m_sectors), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

IDEHDD::IDEHDD(const String& identifier, const char* image_filename /* = "" */, u32 cylinders /* = 0 */,
               u32 heads /* = 0 */, u32 sectors /* = 0 */, u32 ide_channel /* = 0 */, u32 ide_device /* = 0 */,
               const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_cylinders(cylinders), m_heads(heads), m_sectors(sectors),
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
  else if (hdc->IsDrivePresent(m_ide_drive))
  {
    Log_ErrorPrintf("IDE channel %u already has a drive %u attached", m_ide_channel, m_ide_drive);
    return false;
  }

  m_image = HDDImage::Open(m_image_filename);
  if (!m_image)
  {
    Log_ErrorPrintf("Failed to open image for drive %u/%u (%s)", m_ide_channel, m_ide_drive,
                    m_image_filename.GetCharArray());
    return false;
  }

  m_lbas = m_image->GetImageSize() / SECTOR_SIZE;
  if (m_cylinders == 0 || m_heads == 0 || m_sectors == 0)
    HDC::CalculateCHSForSize(&m_cylinders, &m_heads, &m_sectors, m_image->GetImageSize());

  if ((static_cast<u64>(m_cylinders) * static_cast<u64>(m_heads) * static_cast<u64>(m_sectors) * SECTOR_SIZE) !=
      m_image->GetImageSize())
  {
    Log_ErrorPrintf("CHS geometry does not match disk size: %u/%u/%u -> %u LBAs, real size %u LBAs", m_cylinders,
                    m_heads, m_sectors, m_cylinders * m_heads * m_sectors, u32(m_image->GetImageSize() / SECTOR_SIZE));
    return false;
  }

  if (!hdc->AttachHDD(m_ide_drive, this))
  {
    Log_ErrorPrintf("Failed to attach HDD to IDE channel %u device %u", m_ide_channel, m_ide_drive);
    return false;
  }

  Log_DevPrintf("Attached IDE HDD on channel %u drive %u, C/H/S: %u/%u/%u", m_ide_channel, m_ide_drive, m_cylinders,
                m_heads, m_sectors);

  // Create indicator and menu options.
  system->GetHostInterface()->AddUIIndicator(this, HostInterface::IndicatorType::HDD);
  system->GetHostInterface()->AddUICallback(this, "Commit Log to Image", [this]() { m_image->CommitLog(); });
  system->GetHostInterface()->AddUICallback(this, "Revert Log and Reset", [this]() {
    m_image->CommitLog();
    m_system->ExternalReset();
  });
  return true;
}

bool IDEHDD::LoadState(BinaryReader& reader)
{
  const u64 num_lbas = reader.ReadUInt64();
  const u32 num_cylinders = reader.ReadUInt32();
  const u32 num_heads = reader.ReadUInt32();
  const u32 num_sectors = reader.ReadUInt32();
  const u32 channel = reader.ReadUInt32();
  const u32 drive = reader.ReadUInt32();
  if (num_cylinders != m_cylinders || num_heads != m_heads || num_sectors != m_sectors || num_lbas != m_lbas ||
      channel != m_ide_channel || drive != m_ide_drive)
  {
    Log_ErrorPrintf("Save state geometry mismatch");
    return false;
  }

  return m_image->LoadState(reader.GetStream());
}

bool IDEHDD::SaveState(BinaryWriter& writer)
{
  bool result = true;

  result &= writer.SafeWriteUInt64(m_lbas);
  result &= writer.SafeWriteUInt32(m_cylinders);
  result &= writer.SafeWriteUInt32(m_heads);
  result &= writer.SafeWriteUInt32(m_sectors);
  result &= writer.SafeWriteUInt32(m_ide_channel);
  result &= writer.SafeWriteUInt32(m_ide_drive);
  if (!result)
    return false;

  return m_image->SaveState(writer.GetStream());
}

} // namespace HW
