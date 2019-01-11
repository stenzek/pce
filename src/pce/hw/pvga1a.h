#pragma once

#include "pce/hw/vgabase.h"

namespace HW {

/// Paradise VGA PVGA1A
class PVGA1A final : public VGABase
{
  DECLARE_OBJECT_TYPE_INFO(PVGA1A, VGABase);
  DECLARE_GENERIC_COMPONENT_FACTORY(PVGA1A);
  DECLARE_OBJECT_PROPERTY_MAP(PVGA1A);

public:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('P', 'V', '1', 'A');
  static constexpr u32 MAX_BIOS_SIZE = 32768;
  static constexpr u32 MAX_VRAM_SIZE = 1 * 1024 * 1024;

public:
  PVGA1A(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~PVGA1A();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

private:
  enum : u8     // Stored in graphics registers.
  {
    PR0A = 0x09,    // Address Offset A
    PR0B = 0x0A,    // Address Offset B
    PR1 = 0x0B,     // Memory Size
    PR2 = 0x0C,     // Video Select
    PR3 = 0x0D,     // CRTC Control
    PR4 = 0x0E,     // Video Control
    PR5 = 0x0F      // Lock/Status
  };

  bool LoadBIOSROM();

  void ConnectIOPorts() override;
  
  void UpdateVGAMemoryMapping() override;
  void HandleVRAMRead(u32 offset, u8 value);
  void HandleVRAMWrite(u32 offset, u8 value);
  
  void IOGraphicsRegisterWrite(u8 value) override;

  MMIO* m_vram_mmio = nullptr;

  String m_bios_file_path;
};
} // namespace HW