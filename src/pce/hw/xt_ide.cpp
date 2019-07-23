#include "pce/hw/xt_ide.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "pce/bus.h"

namespace HW {
DEFINE_OBJECT_TYPE_INFO(XT_IDE);
DEFINE_GENERIC_COMPONENT_FACTORY(XT_IDE);
BEGIN_OBJECT_PROPERTY_MAP(XT_IDE)
PROPERTY_TABLE_MEMBER_UINT("BIOSAddress", 0, offsetof(XT_IDE, m_bios_address), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("BIOSPath", 0, offsetof(XT_IDE, m_bios_file_path), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("IOBase", 0, offsetof(XT_IDE, m_io_base), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

XT_IDE::XT_IDE(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, 1, type_info), m_bios_file_path("romimages/ide_xt.bin")
{
}

XT_IDE::~XT_IDE() = default;

bool XT_IDE::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  if (!m_bus->CreateROMRegionFromFile(m_bios_file_path, 0, m_bios_address))
    return false;

  return true;
}

void XT_IDE::Reset()
{
  BaseClass::Reset();
  m_data_high = 0;
}

bool XT_IDE::LoadState(BinaryReader& reader)
{
  if (!BaseClass::LoadState(reader))
    return false;

  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  m_data_high = reader.ReadUInt8();

  return !reader.GetErrorState();
}

bool XT_IDE::SaveState(BinaryWriter& writer)
{
  if (!BaseClass::SaveState(writer))
    return false;

  writer.WriteUInt32(SERIALIZATION_ID);

  writer.WriteUInt8(m_data_high);

  return !writer.InErrorState();
}

void XT_IDE::ConnectIOPorts(Bus* bus)
{
  // Matches XTIDE Universal BIOS v2.x.x
  //   bus->ConnectIOPortRead(Truncate16(m_io_base + 0x0), this, [this](u16, u8* value) {
  //     u16 word_value;
  //     IOReadDataRegisterWord(&word_value);
  //     m_data_high = Truncate8(word_value >> 8);
  //     return Truncate8(word_value);
  //   });
  //   bus->ConnectIOPortWrite(Truncate16(m_io_base + 0x0), this, [this](u16, u8 value) {
  //     IOWriteDataRegisterWord(ZeroExtend16(value) | (ZeroExtend16(m_data_high) << 8));
  //   });
  //   bus->ConnectIOPortReadToPointer(Truncate16(m_io_base + 0x1), this, &m_data_high);
  //   bus->ConnectIOPortWriteToPointer(Truncate16(m_io_base + 0x1), this, &m_data_high);
  bus->ConnectIOPortRead(Truncate16(m_io_base + 0x0), this, [this](u16) { return IOReadDataRegisterByte(0); });
  bus->ConnectIOPortWrite(Truncate16(m_io_base + 0x0), this,
                          [this](u16, u8 value) { IOWriteDataRegisterByte(0, value); });

  bus->ConnectIOPortRead(Truncate16(m_io_base + 0x2), this, [this](u16) { return IOReadErrorRegister(0); });
  bus->ConnectIOPortWrite(Truncate16(m_io_base + 0x2), this,
                          [this](u16, u8 value) { IOWriteCommandBlockFeatures(0, value); });

  bus->ConnectIOPortRead(Truncate16(m_io_base + 0x4), this, [this](u16) { return IOReadCommandBlockSectorCount(0); });
  bus->ConnectIOPortWrite(Truncate16(m_io_base + 0x4), this,
                          [this](u16, u8 value) { IOWriteCommandBlockSectorCount(0, value); });

  bus->ConnectIOPortRead(Truncate16(m_io_base + 0x6), this, [this](u16) { return IOReadCommandBlockSectorNumber(0); });
  bus->ConnectIOPortWrite(Truncate16(m_io_base + 0x6), this,
                          [this](u16, u8 value) { IOWriteCommandBlockSectorNumber(0, value); });

  bus->ConnectIOPortRead(Truncate16(m_io_base + 0x8), this, [this](u16) { return IOReadCommandBlockCylinderLow(0); });
  bus->ConnectIOPortWrite(Truncate16(m_io_base + 0x8), this,
                          [this](u16, u8 value) { IOWriteCommandBlockCylinderLow(0, value); });

  bus->ConnectIOPortRead(Truncate16(m_io_base + 0xA), this, [this](u16) { return IOReadCommandBlockCylinderHigh(0); });
  bus->ConnectIOPortWrite(Truncate16(m_io_base + 0xA), this,
                          [this](u16, u8 value) { IOWriteCommandBlockCylinderHigh(0, value); });

  bus->ConnectIOPortRead(Truncate16(m_io_base + 0xC), this, [this](u16) { return IOReadDriveSelectRegister(0); });
  bus->ConnectIOPortWrite(Truncate16(m_io_base + 0xC), this,
                          [this](u16, u8 value) { IOWriteDriveSelectRegister(0, value); });

  bus->ConnectIOPortRead(Truncate16(m_io_base + 0xE), this, [this](u16) { return IOReadStatusRegister(0); });
  bus->ConnectIOPortWrite(Truncate16(m_io_base + 0xE), this,
                          [this](u16, u8 value) { IOWriteCommandRegister(0, value); });

  //   bus->ConnectIOPortRead(Truncate16(m_io_base + 0xE), this, [this](u16, u8* value) {
  //   IOReadAltStatusRegister(value); }); bus->ConnectIOPortWrite(Truncate16(m_io_base + 0xE), this, [this](u16, u8
  //   value) { IOWriteControlRegister(value); });
}

} // namespace HW