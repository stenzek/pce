#pragma once
#include "pce/hw/pci_device.h"

class Display;
class MMIO;
class TimingEvent;

struct voodoo_state;

namespace HW {
class Voodoo final : public PCIDevice
{
  DECLARE_OBJECT_TYPE_INFO(Voodoo, PCIDevice);
  DECLARE_GENERIC_COMPONENT_FACTORY(Voodoo);
  DECLARE_OBJECT_PROPERTY_MAP(Voodoo);

  enum class Type
  {
    Voodoo1 = 0,
    Voodoo1DualTMU = 1,
    Voodoo2 = 2
  };

public:
  Voodoo(const String& identifier, Type type = Type::Voodoo1, u32 fb_mem = 2 * 1024 * 1024,
         u32 tmu_mem = 4 * 1024 * 1024, const ObjectTypeInfo* type_info = &s_type_info);
  ~Voodoo() override;

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;

  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

protected:
  static constexpr PhysicalMemoryAddress DEFAULT_MEMORY_REGION_ADDRESS = 0xFF000000;
  static constexpr u32 MEMORY_REGION_SIZE = 16 * 1024 * 1024;
  static constexpr u32 MEMORY_REGION_DWORD_MASK = (MEMORY_REGION_SIZE / 4) - 1;

  u8 ReadConfigSpace(u8 function, u8 offset) override;
  void WriteConfigSpace(u8 function, u8 offset, u8 value) override;
  void OnMemoryRegionChanged(u8 function, MemoryRegion region, bool active) override;

  void HandleBusByteRead(u32 offset, u8* val);
  void HandleBusByteWrite(u32 offset, u8 val);
  void HandleBusWordRead(u32 offset, u16* val);
  void HandleBusWordWrite(u32 offset, u16 val);
  void HandleBusDWordRead(u32 offset, u32* val);
  void HandleBusDWordWrite(u32 offset, u32 val);

  Type m_type;
  u32 m_fb_mem_size;
  u32 m_tmu_mem_size;

  std::unique_ptr<Display> m_display;
  std::unique_ptr<TimingEvent> m_retrace_event;

  voodoo_state* v = nullptr;

  MMIO* m_mmio_mapping = nullptr;
};

} // namespace HW