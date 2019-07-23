#pragma once
#include "common/bitfield.h"
#include "common/display.h"
#include "common/display_timing.h"
#include "pce/component.h"
#include "pce/system.h"
#include <array>
#include <memory>
#include <optional>
#include <string>

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
  static constexpr Display::FramebufferFormat BASE_FRAMEBUFFER_FORMAT = Display::FramebufferFormat::C8RGBX8;

  enum : u32
  {
    // Maximum number of registers per group for all derived classes.
    NUM_CRTC_REGISTERS = 64,
    NUM_GRAPHICS_REGISTERS = 16,
    NUM_ATTRIBUTE_REGISTERS = 32,
    NUM_SEQUENCER_REGISTERS = 8,

    // Maximum number of registers for base class.
    MAX_VGA_CRTC_REGISTER = 0x18,
    MAX_VGA_GRAPHICS_REGISTER = 0x08,
    MAX_VGA_ATTRIBUTE_REGISTER = 0x14,
    MAX_VGA_SEQUENCER_REGISTER = 0x04
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

  // Helper functions
  inline constexpr u32 Convert6BitColorTo8Bit(u32 color);
  inline constexpr u32 Convert8BitColorTo6Bit(u32 color);

protected:
  // Register definitions
  struct BaseCRTCRegisters
  {
    u8 horizontal_total;          // 0  0x00
    u8 horizontal_display_end;    // 1  0x01
    u8 horizontal_blanking_start; // 2  0x02
    u8 horizontal_blanking_end;   // 3  0x03
    u8 horizontal_sync_start;     // 4  0x04
    u8 horizontal_sync_end;       // 5  0x05
    u8 vertical_total;            // 6  0x06
    union
    {
      BitField<u8, u8, 0, 1> vertical_total_8;
      BitField<u8, u8, 1, 1> vertical_display_end_8;
      BitField<u8, u8, 2, 1> vertical_sync_start_8;
      BitField<u8, u8, 3, 1> vertical_blanking_start_8;
      BitField<u8, u8, 4, 1> line_compare_8;
      BitField<u8, u8, 5, 1> vertical_total_9;
      BitField<u8, u8, 6, 1> vertical_display_end_9;
      BitField<u8, u8, 7, 1> vertical_sync_start_9;
    }; // 7  0x07
    union
    {
      BitField<u8, u8, 0, 5> preset_row_scan;
      BitField<u8, u8, 5, 2> byte_panning;
    }; // 8  0x08
    union
    {
      BitField<u8, u8, 0, 5> maximum_scan_line;
      BitField<u8, u8, 5, 1> start_vertical_blanking_9;
      BitField<u8, u8, 6, 1> line_compare_9;
      BitField<u8, bool, 7, 1> scan_doubling;
    }; // 9  0x09
    union
    {
      BitField<u8, u8, 0, 5> cursor_start_line;
      BitField<u8, bool, 5, 1> cursor_disable;
    }; // 10 0x0A
    union
    {
      BitField<u8, u8, 0, 5> cursor_end_line;
    };                       // 11 0x0B
    u8 start_address_high;   // 12 0x0C
    u8 start_address_low;    // 13 0x0D
    u8 cursor_location_high; // 14 0x0E
    u8 cursor_location_low;  // 15 0x0F
    u8 vertical_sync_start;  // 16 0x10
    u8 vertical_sync_end;    // 17 0x11
    u8 vertical_display_end; // 18 0x12
    u8 offset;               // 19 0x13
    union
    {
      BitField<u8, u8, 0, 5> underline_location;
      BitField<u8, bool, 5, 1> memory_address_div4;
      BitField<u8, bool, 6, 1> double_word_mode;
    };                          // 20 0x14
    u8 vertical_blanking_start; // 21 0x15
    u8 vertical_blanking_end;   // 22 0x16
    union
    {
      BitField<u8, bool, 0, 1> alternate_la13_n;
      BitField<u8, bool, 1, 1> alternate_la14_n;
      BitField<u8, bool, 2, 1> line_counter_mul2;
      BitField<u8, bool, 3, 1> memory_address_div2;
      BitField<u8, bool, 4, 1> memory_address_output_control;
      BitField<u8, bool, 5, 1> alternate_ma00_output;
      BitField<u8, bool, 6, 1> byte_mode;
      BitField<u8, bool, 7, 1> hold_mode;
      u8 mode_control_bits;
    };               // 23 0x17
    u8 line_compare; // 24 0x18
    u8 unk1;         // 25 0x19
    u8 unk2;         // 26 0x1A
    u8 unk3;         // 27 0x1B
    u8 unk4;         // 28 0x1C
    u8 unk5;         // 29 0x1D
    u8 unk6;         // 30 0x1E
    u8 unk7;         // 31 0x1F
    u8 unk8;         // 32 0x20
    u8 unk9;         // 33 0x21
    u8 unk10;        // 34 0x22
    u8 unk11;        // 35 0x23
    union
    {
      u8 unk12; // 36 0x24
      BitField<u8, bool, 7, 1> attribute_register_flipflop;
    };
  };
  struct BaseGraphicsRegisters
  {
    union
    {
      u8 set_reset_register;
      BitField<u8, u8, 0, 4> set_reset;
    }; // 0x00
    union
    {
      u8 enable_set_reset_register;
      BitField<u8, u8, 0, 4> enable_set_reset;
    }; // 0x01
    union
    {
      u8 color_compare_register;
      BitField<u8, u8, 0, 4> color_compare;
    }; // 0x02
    union
    {
      u8 data_rotate_register;
      BitField<u8, u8, 0, 4> rotate_count;
      BitField<u8, u8, 3, 2> logic_op;
    }; // 0x03
    union
    {
      u8 read_map_select_register;
      BitField<u8, u8, 0, 2> read_map_select;
    }; // 0x04
    union
    {
      u8 graphics_mode_register;

      BitField<u8, u8, 0, 2> write_mode;
      BitField<u8, u8, 3, 1> read_mode;
      BitField<u8, bool, 4, 1> host_odd_even;
      BitField<u8, bool, 5, 1> shift_reg;
      BitField<u8, bool, 6, 1> shift_256;
    }; // 0x05
    union
    {
      u8 miscellaneous_graphics_register;

      BitField<u8, bool, 0, 1> graphics_mode_enable;
      BitField<u8, bool, 1, 1> chain_odd_even_enable;
      BitField<u8, u8, 2, 2> memory_map_select;
    }; // 0x06
    union
    {
      u8 color_dont_care_register;
      BitField<u8, u8, 0, 4> color_dont_care;
    }; // 0x07
    u8 bit_mask;
  };
  struct BaseAttributeRegisters
  {
    u8 palette[16];
    union
    {
      u8 bits;
      BitField<u8, bool, 0, 1> graphics_enable;
      BitField<u8, bool, 1, 1> mono_emulation;
      BitField<u8, bool, 2, 1> line_graphics_enable;
      BitField<u8, bool, 3, 1> blink_enable;
      BitField<u8, bool, 5, 1> pixel_panning_mode; // disable panning while in split screen
      BitField<u8, bool, 6, 1> eight_bit_mode;
      BitField<u8, bool, 7, 1> palette_bits_5_4_select;
    };
    u8 overscan_color;
    union
    {
      BitField<u8, u8, 0, 4> plane_read_mask;
    };
    union
    {
      BitField<u8, u8, 0, 4> horizontal_pixel_panning;
    };
    u8 color_select;
  };
  struct BaseSequencerRegisters
  {
    union
    {
      BitField<u8, bool, 0, 1> asynchronous_reset;
      BitField<u8, bool, 1, 1> synchronous_reset;
    }; // 00

    union
    {
      BitField<u8, u8, 0, 1> dot8; // set - 8 dots/char
      BitField<u8, bool, 2, 1> shift_load_rate;
      BitField<u8, bool, 3, 1> dot_clock_div2;
      BitField<u8, bool, 4, 1> shift_four_enable;
      BitField<u8, bool, 5, 1> screen_disable;
    }; // 01

    union
    {
      BitField<u8, u8, 0, 4> plane_write_mask;
    }; // 02

    union
    {
      BitField<u8, u8, 0, 2> character_set_b_select_01;
      BitField<u8, u8, 2, 2> character_set_a_select_01;
      BitField<u8, u8, 4, 1> character_set_b_select_2;
      BitField<u8, u8, 5, 1> character_set_a_select_2;
    }; // 03

    union
    {
      BitField<u8, bool, 1, 1> extended_memory;
      BitField<u8, bool, 2, 1> odd_even_host_memory;
      BitField<u8, bool, 3, 1> chain_4_enable;
    }; // 04
  };

  virtual void ConnectIOPorts();
  virtual void DisconnectIOPorts();
  virtual void GetDisplayTiming(DisplayTiming& timing) const;
  virtual void LatchStartAddress();
  virtual void RenderTextMode();
  virtual void RenderGraphicsMode();

  u32 ReadVRAMPlanes(u32 base_address, u32 address_counter, u32 row_scan_counter) const;
  u32 CRTCWrapAddress(u32 base_address, u32 address_counter, u32 row_scan_counter) const;

  std::unique_ptr<Display> m_display;
  std::unique_ptr<TimingEvent> m_display_event;
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
  union
  {
    std::array<u8, NUM_CRTC_REGISTERS> m_crtc_register_index{};
    BaseCRTCRegisters m_crtc_registers;
  };
  std::array<u8, NUM_CRTC_REGISTERS> m_crtc_register_mask{};
  u8 m_crtc_index_register = 0;
  bool m_crtc_timing_changed = false;

  // 03D1/3/5: CRT data register
  virtual void IOCRTCDataRegisterRead(u8* value);
  virtual void IOCRTCDataRegisterWrite(u8 value);
  void CRTCTimingChanged();

  // Graphics Registers
  union
  {
    std::array<u8, NUM_GRAPHICS_REGISTERS> m_graphics_register_index{};
    BaseGraphicsRegisters m_graphics_registers;
  };
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
  union
  {
    std::array<u8, NUM_ATTRIBUTE_REGISTERS> m_attribute_register_index{};
    BaseAttributeRegisters m_attribute_registers;
  };
  std::array<u8, NUM_ATTRIBUTE_REGISTERS> m_attribute_register_mask{};
  u8 m_attribute_index_register = 0;
  bool m_attribute_register_flipflop = false;
  bool m_output_enable = false;
  virtual void IOAttributeAddressRead(u8* value);
  virtual void IOAttributeDataRead(u8* value);
  virtual void IOAttributeAddressDataWrite(u8 value);

  // 03C4/03C5: Sequencer Registers (also known as TS)
  union
  {
    std::array<u8, NUM_SEQUENCER_REGISTERS> m_sequencer_register_index{};
    BaseSequencerRegisters m_sequencer_registers;
  };
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
  u8 m_dac_color_mask = 0x3F;
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

  void HandleVGAVRAMRead(u32 segment_base, u32 offset, u8* value);
  void HandleVGAVRAMWrite(u32 segment_base, u32 offset, u8 value);

  void GetVGAMemoryMapping(PhysicalMemoryAddress* base_address, u32* size);
  virtual void UpdateVGAMemoryMapping();

  // palette used when rendering
  void SetOutputPalette16();
  void SetOutputPalette256();

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
    bool graphics_mode;
  } m_render_latch = {};

private:
  void UpdateDisplayTiming();
  void Render();
};

} // namespace HW

#include "vga_base.inl"
