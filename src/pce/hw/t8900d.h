#pragma once

#include "pce/hw/vgabase.h"

namespace HW {

class T8900D final : public VGABase
{
  DECLARE_OBJECT_TYPE_INFO(T8900D, VGABase);
  DECLARE_GENERIC_COMPONENT_FACTORY(T8900D);
  DECLARE_OBJECT_PROPERTY_MAP(T8900D);

public:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('8', '9', '0', '0');
  static constexpr u32 MAX_BIOS_SIZE = 32768;
  static constexpr u32 MAX_VRAM_SIZE = 2 * 1024 * 1024;

public:
  T8900D(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~T8900D();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

private:
  bool LoadBIOSROM();

  void ConnectIOPorts() override;
  
  void UpdateVGAMemoryMapping() override;
  void HandleVRAMRead(u32 offset, u8 value);
  void HandleVRAMWrite(u32 offset, u8 value);
  
  void IOSequencerDataRegisterWrite(u8 value) override;

  MMIO* m_vram_mmio = nullptr;

  String m_bios_file_path;
};
} // namespace HW