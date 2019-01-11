#pragma once
#include "common/bitfield.h"
#include "common/display_timing.h"
#include "pce/component.h"
#include "pce/system.h"
#include <array>
#include <memory>
#include <optional>
#include <string>

class Display;
class ByteStream;
class MMIO;

namespace HW {

class VGABase : public Component
{
  DECLARE_OBJECT_TYPE_INFO(VGABase, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(VGABase);
  DECLARE_OBJECT_PROPERTY_MAP(VGABase);

public:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('V', 'G', 'A', 'B');

  enum : u32
  {
    NUM_CRTC_REGISTERS = 64,
    NUM_GRAPHICS_REGISTERS = 16,
    NUM_ATTRIBUTE_REGISTERS = 32,
    NUM_SEQUENCER_REGISTERS = 8
  };

public:
  VGABase(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~VGABase();

  const u8* GetVRAM() const { return m_vram.data(); }
  u8* GetVRAM() { return m_vram.data(); }

  virtual bool Initialize(System* system, Bus* bus) override;
  virtual void Reset() override;
  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

protected:
  virtual void ConnectIOPorts();
  virtual void DisconnectIOPorts();
  virtual void GetDisplayTiming(DisplayTiming& timing) const;
  virtual void LatchStartAddress();
  virtual void RenderTextMode();
  virtual void RenderGraphicsMode();

  u32 ReadVRAMPlanes(u32 base_address, u32 address_counter, u32 row_scan_counter) const;
  u32 CRTCWrapAddress(u32 base_address, u32 address_counter, u32 row_scan_counter) const;

  void DrawTextGlyph8(u32 fb_x, u32 fb_y, const u8* glyph, u32 rows, u32 fg_color, u32 bg_color, s32 dup9);
  void DrawTextGlyph16(u32 fb_x, u32 fb_y, const u8* glyph, u32 rows, u32 fg_color, u32 bg_color);

  std::unique_ptr<Display> m_display;
  TimingEvent::Pointer m_display_event;
  DisplayTiming m_display_timing;

  // VRAM
  // The 4 planes of 64KB (256KB) VRAM is interleaved here.
  // Array Offset | Plane | Offset
  // -----------------------------
  //            0 |     0 |      0
  //            1 |     1 |      0
  //            2 |     2 |      0
  //            3 |     3 |      0
  //            4 |     0 |      1
  //            5 |     1 |      1
  //            6 |     2 |      1
  //            7 |     3 |      1
  std::vector<u8> m_vram;
  u32 m_vram_size = 0;
  u32 m_vram_mask = 0;

  // latch for vram reads
  u32 m_latch = 0;

  // CRTC registers
  std::array<u8, NUM_CRTC_REGISTERS> m_crtc_registers{};
  std::array<u8, NUM_CRTC_REGISTERS> m_crtc_register_mask{};
  u8 m_crtc_index_register = 0;
  bool m_crtc_timing_changed = false;

  // 03D1/3/5: CRT data register
  virtual void IOCRTCDataRegisterRead(u8* value);
  virtual void IOCRTCDataRegisterWrite(u8 value);
  void CRTCTimingChanged();

  // Graphics Registers
  std::array<u8, NUM_GRAPHICS_REGISTERS> m_graphics_registers{};
  std::array<u8, NUM_GRAPHICS_REGISTERS> m_graphics_register_mask{};
  u8 m_graphics_index_register = 0;

  // 03CE/03CF: VGA Graphics Registers
  virtual void IOGraphicsRegisterRead(u8* value);
  virtual void IOGraphicsRegisterWrite(u8 value);

  // 03CC/03C2: Miscellaneous Output Register
  union
  {
    u8 bits = 0;

    BitField<u8, bool, 0, 1> io_address_select;
    BitField<u8, bool, 1, 1> ram_enable;
    BitField<u8, u8, 2, 2> clock_select;
    BitField<u8, bool, 5, 1> odd_even_page;
    BitField<u8, bool, 6, 1> hsync_polarity;
    BitField<u8, bool, 7, 1> vsync_polarity;
  } m_misc_output_register;
  virtual void IOMiscOutputRegisterWrite(u8 value);

  virtual void IOReadStatusRegister0(u8* value);
  virtual void IOReadStatusRegister1(u8* value);

  // 03CA/03DA: Feature Control Register
  union
  {
    u8 bits = 0;
    BitField<u8, bool, 3, 1> vsync_control;
  } m_feature_control_register;

  // 3C0/3C1: Attribute Controller Registers
  std::array<u8, NUM_ATTRIBUTE_REGISTERS> m_attribute_registers{};
  std::array<u8, NUM_ATTRIBUTE_REGISTERS> m_attribute_register_mask{};
  u8 m_attribute_index_register = 0;
  bool m_attribute_register_flipflop = false;
  virtual void IOAttributeAddressRead(u8* value);
  virtual void IOAttributeDataRead(u8* value);
  virtual void IOAttributeAddressDataWrite(u8 value);

  // 03C4/03C5: Sequencer Registers (also known as TS)
  std::array<u8, NUM_SEQUENCER_REGISTERS> m_sequencer_registers{};
  std::array<u8, NUM_SEQUENCER_REGISTERS> m_sequencer_register_mask{};
  u8 m_sequencer_index_register = 0;
  virtual void IOSequencerDataRegisterRead(u8* value);
  virtual void IOSequencerDataRegisterWrite(u8 value);

  // 03C7/03C8/03C9: DAC/color registers
  // Colors are organized as RGBA values
  std::array<u32, 256> m_dac_palette;
  u8 m_dac_state_register = 0; // TODO Handling
  u8 m_dac_write_address = 0;
  u8 m_dac_read_address = 0;
  u8 m_dac_color_index = 0;
  virtual void IODACStateRegisterRead(u8* value);
  virtual void IODACReadAddressWrite(u8 value);
  virtual void IODACWriteAddressRead(u8* value);
  virtual void IODACWriteAddressWrite(u8 value);
  virtual void IODACDataRegisterRead(u8* value);
  virtual void IODACDataRegisterWrite(u8 value);

  // 46E8/03C3: VGA adapter enable
  union
  {
    uint8 bits = 0;

    BitField<uint8, bool, 3, 1> enable_io;
  } m_vga_adapter_enable;
  virtual void IOVGAAdapterEnableWrite(u8 value);

  bool MapToVGAVRAMOffset(u32* offset_ptr);
  void HandleVGAVRAMRead(u32 segment_base, u32 offset, u8* value);
  void HandleVGAVRAMWrite(u32 segment_base, u32 offset, u8 value);
  virtual void UpdateVGAMemoryMapping();

  // palette used when rendering
  void SetOutputPalette16();
  void SetOutputPalette256();
  std::array<u32, 256> m_output_palette;

  // Cursor state for text modes
  u8 m_cursor_counter = 0;
  bool m_cursor_state = false;

  // Information for rendering the screen, latched after vblank
  struct
  {
    u32 render_width;
    u32 render_height;
    u32 start_address;
    u32 cursor_address;
    u32 pitch;
    u32 line_compare;
    u8 character_width;
    u8 character_height; // or scanlines per row in graphics mode
    u8 horizontal_panning;
    u8 row_scan_counter;
    u8 cursor_start_line;
    u8 cursor_end_line;
  } m_render_latch = {};

private:
  void UpdateDisplayTiming();
  void Render();
};
} // namespace HW
