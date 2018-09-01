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
  DECLARE_OBJECT_NO_PROPERTIES(AdLib);
  DECLARE_OBJECT_GENERIC_FACTORY(AdLib);

public:
  AdLib();
  ~AdLib();

  bool Initialize(System* system, Bus* bus) override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;
  void Reset() override;

private:
  void IOPortRead(uint32 port, uint8* value);
  void IOPortWrite(uint32 port, uint8 value);

  YMF262 m_chip;
};

} // namespace HW