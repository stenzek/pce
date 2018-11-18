#pragma once
#include "pce/hw/hdc.h"

class ByteStream;

namespace HW {

class XT_IDE : public HDC
{
  DECLARE_OBJECT_TYPE_INFO(XT_IDE, HDC);
  DECLARE_GENERIC_COMPONENT_FACTORY(XT_IDE);
  DECLARE_OBJECT_PROPERTY_MAP(XT_IDE);

public:
  static constexpr PhysicalMemoryAddress XTIDE_BIOS_ROM_ADDRESS = 0xD8000;

  XT_IDE(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~XT_IDE();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;

  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

private:
  static constexpr uint32 SERIALIZATION_ID = Component::MakeSerializationID('X', 'I', 'D', 'E');

  void ConnectIOPorts(Bus* bus) override;

  String m_bios_file_path;
  u32 m_bios_address = XTIDE_BIOS_ROM_ADDRESS;
  u32 m_io_base = 0x0300;
  u8 m_data_high = 0;
};

} // namespace HW