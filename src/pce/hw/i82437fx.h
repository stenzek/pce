#pragma once
#include "pce/hw/pci_device.h"

namespace HW {
class i82437FX final : public PCIDevice
{
  DECLARE_OBJECT_TYPE_INFO(i82437FX, PCIDevice);
  DECLARE_OBJECT_NO_FACTORY(i82437FX);
  DECLARE_OBJECT_PROPERTY_MAP(i82437FX);

public:
  i82437FX(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~i82437FX();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;

  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  u8 ReadConfigSpace(u8 function, u8 offset) override;
  void WriteConfigSpace(u8 function, u8 offset, u8 value) override;

private:
  static constexpr u8 NUM_PAM_REGISTERS = 7;
  static constexpr u8 PAM_BASE_OFFSET = 0x59;

  void SetPAMMapping(u32 base, u32 size, u8 flag);
  void UpdatePAMMapping(u8 offset);
};

} // namespace HW