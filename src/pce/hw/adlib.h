#pragma once

#include "common/audio.h"
#include "pce/component.h"
#include "pce/hw/ymf262.h"
#include "pce/system.h"
#include <array>

namespace HW {

class AdLib final : public Component
{
  DECLARE_OBJECT_TYPE_INFO(AdLib, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(AdLib);
  DECLARE_OBJECT_PROPERTY_MAP(AdLib);

public:
  AdLib(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~AdLib();

  bool Initialize(System* system, Bus* bus) override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;
  void Reset() override;

private:
  void IOPortRead(u16 port, u8* value);
  void IOPortWrite(u16 port, u8 value);

  YMF262 m_chip;
  u32 m_io_base = 0x0388;
};

} // namespace HW