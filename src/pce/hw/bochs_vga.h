#pragma once
#include "pce/hw/vga_base.h"

namespace HW {

class BochsVGA final : public VGABase
{
  DECLARE_OBJECT_TYPE_INFO(BochsVGA, VGABase);
  DECLARE_GENERIC_COMPONENT_FACTORY(BochsVGA);
  DECLARE_OBJECT_PROPERTY_MAP(BochsVGA);

public:
  static constexpr uint32 SERIALIZATION_ID = MakeSerializationID('V', 'G', 'A');
  static constexpr uint32 MAX_BIOS_SIZE = 65536;
  static constexpr uint32 VRAM_SIZE = 256 * 1024;

public:
  BochsVGA(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~BochsVGA();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

private:
  bool LoadBIOSROM();
  void UpdateVGAMemoryMapping() override;

  MMIO* m_vram_mmio = nullptr;

  String m_bios_file_path;
};
} // namespace HW