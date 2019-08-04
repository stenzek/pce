#pragma once
#include "pce/hw/pci_device.h"
#include "pce/hw/vga_base.h"

namespace HW {

class BochsVGA final : public VGABase, public PCIDevice
{
  DECLARE_OBJECT_TYPE_INFO(BochsVGA, VGABase);
  DECLARE_GENERIC_COMPONENT_FACTORY(BochsVGA);
  DECLARE_OBJECT_PROPERTY_MAP(BochsVGA);

public:
  static constexpr uint32 SERIALIZATION_ID = MakeSerializationID('V', 'G', 'A');

public:
  BochsVGA(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~BochsVGA();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

private:
  enum : u32
  {
    BIOS_ROM_LOCATION = 0xC0000,
    BIOS_ROM_SIZE = 0x10000
  };

  enum : u32
  {
    VBE_DISPI_TOTAL_VIDEO_MEMORY_BYTES = 16 * 1024 * 1024,

    VBE_DISPI_BANK_ADDRESS = 0xA0000,
    VBE_DISPI_BANK_SIZE = 0x10000,

    VBE_DISPI_DEFAULT_LFB_PHYSICAL_ADDRESS = 0xE0000000,

    VBE_DISPI_4BPP_PLANE_SHIFT = 22,

    VBE_DISPI_MAX_XRES = 2560,
    VBE_DISPI_MAX_YRES = 1600,
    VBE_DISPI_MAX_BPP = 32,

    VBE_DISPI_IOPORT_INDEX = 0x01CE,
    VBE_DISPI_IOPORT_DATA = 0x01CF,

    VBE_DISPI_INDEX_ID = 0x0,
    VBE_DISPI_INDEX_XRES = 0x1,
    VBE_DISPI_INDEX_YRES = 0x2,
    VBE_DISPI_INDEX_BPP = 0x3,
    VBE_DISPI_INDEX_ENABLE = 0x4,
    VBE_DISPI_INDEX_BANK = 0x5,
    VBE_DISPI_INDEX_VIRT_WIDTH = 0x6,
    VBE_DISPI_INDEX_VIRT_HEIGHT = 0x7,
    VBE_DISPI_INDEX_X_OFFSET = 0x8,
    VBE_DISPI_INDEX_Y_OFFSET = 0x9,
    VBE_DISPI_INDEX_VIDEO_MEMORY_64K = 0xa,
    VBE_DISPI_INDEX_DDC = 0xb,

    VBE_DISPI_ID0 = 0xB0C0,
    VBE_DISPI_ID1 = 0xB0C1,
    VBE_DISPI_ID2 = 0xB0C2,
    VBE_DISPI_ID3 = 0xB0C3,
    VBE_DISPI_ID4 = 0xB0C4,
    VBE_DISPI_ID5 = 0xB0C5,

    VBE_DISPI_BPP_4 = 0x04,
    VBE_DISPI_BPP_8 = 0x08,
    VBE_DISPI_BPP_15 = 0x0F,
    VBE_DISPI_BPP_16 = 0x10,
    VBE_DISPI_BPP_24 = 0x18,
    VBE_DISPI_BPP_32 = 0x20,

    VBE_DISPI_DISABLED = 0x00,
    VBE_DISPI_ENABLED = 0x01,
    VBE_DISPI_GETCAPS = 0x02,
    VBE_DISPI_8BIT_DAC = 0x20,
    VBE_DISPI_LFB_ENABLED = 0x40,
    VBE_DISPI_NOCLEARMEM = 0x80,
  };

  static bool IsValidBPP(u16 bpp);

  bool LoadBIOSROM();
  void ConnectIOPorts() override;
  void UpdateVGAMemoryMapping() override;
  void OnMemoryRegionChanged(u8 function, MemoryRegion region, bool active) override;

  void GetDisplayTiming(DisplayTiming& timing) const override;
  void LatchStartAddress() override;

  void RenderGraphicsMode() override;

  void UpdateBIOSMemoryMapping();

  bool IsLFBEnabled() const;
  void UpdateFramebufferFormat();

  u16 IOReadVBEDataRegister();
  void IOWriteVBEDataRegister(u16 value);

  void Render4BPP();
  void Render8BPP();
  void RenderDirect();

  MMIO* m_bios_mmio = nullptr;
  MMIO* m_vga_mmio = nullptr;
  MMIO* m_lfb_mmio = nullptr;

  String m_bios_file_path;
  std::vector<u8> m_bios_rom_data;

  u16 m_vbe_index_register = 0;

  union
  {
    BitField<u16, bool, 0, 1> enable;
    BitField<u16, bool, 1, 1> read_capabilities;
    BitField<u16, bool, 5, 1> dac_8bit;
    BitField<u16, bool, 6, 1> lfb_enable;
    BitField<u16, bool, 7, 1> no_clear_mem;
    u16 bits = 0;
  } m_vbe_enable;

  u16 m_vbe_id = VBE_DISPI_ID5;
  u16 m_vbe_bank = 0;

  u16 m_vbe_width = 720;
  u16 m_vbe_height = 400;
  u16 m_vbe_bpp = 32;

  u16 m_vbe_offset_x = 0;
  u16 m_vbe_offset_y = 0;

  u16 m_vbe_virt_width = 0;
  u16 m_vbe_virt_height = 0;
};
} // namespace HW