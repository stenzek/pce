#pragma once

#include "pce/hw/vgabase.h"

namespace HW {

class VGA final : public VGABase
{
  DECLARE_OBJECT_TYPE_INFO(VGA, VGABase);
  DECLARE_GENERIC_COMPONENT_FACTORY(VGA);
  DECLARE_OBJECT_PROPERTY_MAP(VGA);

public:
  static constexpr uint32 SERIALIZATION_ID = MakeSerializationID('V', 'G', 'A');
  static constexpr uint32 MAX_BIOS_SIZE = 65536;
  static constexpr uint32 VRAM_SIZE = 256 * 1024;

public:
  VGA(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~VGA();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

private:
  void ConnectIOPorts() override;
  bool LoadBIOSROM();
  void RegisterVRAMMMIO();

  std::vector<u8> m_vram;
  MMIO* m_vram_mmio = nullptr;

  std::array<u8, 37> m_crtc_registers{};
  std::array<u8, 16> m_graphics_registers{};
  std::array<u8, 21> m_attribute_registers{};
  std::array<u8, 5> m_sequencer_registers{};

  // 46E8/03C3: VGA adapter enable
  union
  {
    uint8 bits = 0;

    BitField<uint8, bool, 3, 1> enable_io;
  } m_vga_adapter_enable;

  String m_bios_file_path;
};
} // namespace HW