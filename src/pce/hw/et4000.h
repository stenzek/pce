#pragma once
#include "common/bitfield.h"
#include "common/clock.h"
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

class ET4000 : public Component
{
  DECLARE_OBJECT_TYPE_INFO(ET4000, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(ET4000);
  DECLARE_OBJECT_PROPERTY_MAP(ET4000);

public:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('E', 'T', '4', 'K');
  static constexpr u32 MAX_BIOS_SIZE = 32768;
  static constexpr u32 VRAM_SIZE = 1048576;
  static constexpr u32 VRAM_MASK = 1048576 - 1;
  static constexpr u32 VRAM_SIZE_PER_PLANE = VRAM_SIZE / 4;
  static constexpr u32 VRAM_MASK_PER_PLANE = (VRAM_SIZE / 4) - 1;

public:
  ET4000(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~ET4000();

  void SetBIOSFilePath(const std::string& path) { m_bios_file_path = path; }

  const u8* GetVRAM() const { return m_vram; }
  u8* GetVRAM() { return m_vram; }

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

private:
  void ConnectIOPorts();

  u32 CRTCReadVRAMPlanes(u32 address_counter, u32 row_scan_counter) const;
  u32 CRTCWrapAddress(u32 address_counter, u32 row_scan_counter) const;

  void Render();
  void RenderTextMode();
  void RenderGraphicsMode();

  void DrawTextGlyph8(u32 fb_x, u32 fb_y, const u8* glyph, u32 rows, u32 fg_color, u32 bg_color, s32 dup9);
  void DrawTextGlyph16(u32 fb_x, u32 fb_y, const u8* glyph, u32 rows, u32 fg_color, u32 bg_color);

  // void DrawGraphicsLine

  std::unique_ptr<Display> m_display;

  // 03C2h: Status register 0
  u8 m_st0 = 0;

  // 03DAh: Status register 1
  union StatusRegister1
  {
    BitField<u8, bool, 0, 1> display_enable_n;
    BitField<u8, bool, 3, 1> vertical_blank;
    BitField<u8, u8, 4, 2> display_feedback_test;
    BitField<u8, bool, 7, 1> vertical_blank_n;
    u8 bits = 0;
  };
  void IOReadStatusRegister1(u8* value);

  // CRTC registers
  union
  {
    struct
    {
      u8 horizontal_total;          // 0  0x00
      u8 end_horizontal_display;    // 1  0x01
      u8 start_horizontal_blanking; // 2  0x02
      u8 end_horizontal_blanking;   // 3  0x03
      u8 start_horizontal_retrace;  // 4  0x04
      u8 end_horizontal_retrace;    // 5  0x05
      u8 vertical_total;            // 6  0x06
      union
      {
        BitField<u8, u8, 0, 1> vertical_total_8;
        BitField<u8, u8, 1, 1> vertical_display_end_8;
        BitField<u8, u8, 2, 1> vertical_retrace_start_8;
        BitField<u8, u8, 3, 1> start_vertical_blanking_8;
        BitField<u8, u8, 4, 1> line_compare_8;
        BitField<u8, u8, 5, 1> vertical_total_9;
        BitField<u8, u8, 6, 1> vertical_display_end_9;
        BitField<u8, u8, 7, 1> vertical_retrace_start_9;
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
      };                          // 9  0x09
      u8 cursor_start;            // 10 0x0A
      u8 cursor_end;              // 11 0x0B
      u8 start_address_high;      // 12 0x0C
      u8 start_address_low;       // 13 0x0D
      u8 cursor_location_high;    // 14 0x0E
      u8 cursor_location_low;     // 15 0x0F
      u8 vertical_retrace_start;  // 16 0x10
      u8 vertical_retrace_end;    // 17 0x11
      u8 vertical_display_end;    // 18 0x12
      u8 offset;                  // 19 0x13
      u8 underline_location;      // 20 0x14
      u8 start_vertical_blanking; // 21 0x15
      u8 end_vertical_blanking;   // 22 0x16
      union
      {
        BitField<u8, bool, 0, 1> alternate_la13;
        BitField<u8, bool, 1, 1> alternate_la14;
        BitField<u8, bool, 2, 1> line_counter_mul2;
        BitField<u8, bool, 3, 1> linear_counter_mul2;
        BitField<u8, bool, 4, 1> memory_address_output_control;
        BitField<u8, bool, 5, 1> alternate_ma00_output;
        BitField<u8, bool, 6, 1> byte_mode;
        BitField<u8, bool, 7, 1> hold_mode;
        u8 bits;
      } crtc_mode_control; // 23 0x17
      u8 line_compare;     // 24 0x18
      u8 unk1;             // 25 0x19
      u8 unk2;             // 26 0x1A
      u8 unk3;             // 27 0x1B
      u8 unk4;             // 28 0x1C
      u8 unk5;             // 29 0x1D
      u8 unk6;             // 30 0x1E
      u8 unk7;             // 31 0x1F
      u8 unk8;             // 32 0x20
      u8 unk9;             // 33 0x21
      u8 unk10;            // 34 0x22
      u8 unk11;            // 35 0x23
      union
      {
        u8 unk12; // 36 0x24
        BitField<u8, bool, 7, 1> attribute_register_flipflop;
      };
      u8 unk13; // 37 0x25
      u8 unk14; // 38 0x26
      u8 unk15; // 39 0x27
      u8 unk16; // 40 0x28
      u8 unk17; // 41 0x29
      u8 unk18; // 42 0x2A
      u8 unk19; // 43 0x2B
      u8 unk20; // 44 0x2C
      u8 unk21; // 45 0x2D
      u8 unk22; // 46 0x2E
      u8 unk23; // 47 0x2F
      u8 unk24; // 48 0x30
      u8 unk25; // 49 0x31
      union
      {
        BitField<u8, u8, 0, 2> csw;
        BitField<u8, u8, 2, 1> csp;
        BitField<u8, u8, 3, 2> rsp;
        BitField<u8, u8, 5, 1> ras_to_cas;
        BitField<u8, u8, 6, 1> col_setup_time;
        BitField<u8, u8, 7, 1> static_col_mem;
      } ras_cas_config; // 50 0x32
      union
      {
        BitField<u8, u8, 0, 2> extended_start_address;
        BitField<u8, u8, 2, 2> extended_cursor_address;
      }; // 51 0x33
      union
      {
        BitField<u8, bool, 0, 1> cs0_translation;
        BitField<u8, u8, 1, 1> clock_select_2;
        BitField<u8, bool, 2, 1> tristate;
        BitField<u8, bool, 3, 1> vse_register_port;    // 1=46E8, 0=3C3
        BitField<u8, bool, 4, 1> read_translation;     // ENXR
        BitField<u8, bool, 5, 1> write_translation;    // ENXL
        BitField<u8, bool, 6, 1> doublescan_underline; // ENBA
        BitField<u8, bool, 7, 1> enable;
      } mc6845_compatibility_control; // 52 0x34
      union
      {
        BitField<u8, u8, 0, 1> vertical_blank_start_10;
        BitField<u8, u8, 1, 1> vertical_total_10;
        BitField<u8, u8, 2, 1> vertical_display_end_10;
        BitField<u8, u8, 3, 1> vertical_sync_start_10;
        BitField<u8, u8, 4, 1> line_compare_10;
        BitField<u8, bool, 5, 1> external_sync_counters;
        BitField<u8, bool, 6, 1> alternate_rmw_control;
        BitField<u8, bool, 7, 1> vertical_interlace_mode; // doubles vertical resolution with same timing
      };                                                  // 53 0x35
      union
      {
        BitField<u8, u8, 0, 3> refresh_count;
        BitField<u8, bool, 3, 1> font_width_control;
        BitField<u8, bool, 4, 1> linear_mapping;     // from CPU
        BitField<u8, bool, 5, 1> contiguous_mapping; // from CPU
        BitField<u8, bool, 6, 1> enable_16bit_vram;
        BitField<u8, bool, 7, 1> enable_16bit_io;
      }; // 54 0x36
      union
      {
        BitField<u8, u8, 0, 2> display_memory_data_bus_width;
        BitField<u8, u8, 2, 2> display_memory_data_depth;
        BitField<u8, bool, 4, 1> enable_16bit_rom;
        BitField<u8, bool, 5, 1> priority_threshold_control;
        BitField<u8, bool, 6, 1> tli_internal_test;
        BitField<u8, bool, 7, 1> display_memory_type;
      };        // 55 0x37
      u8 unk32; // 56 0x38
      u8 unk33; // 57 0x39
      u8 unk34; // 58 0x3A
      u8 unk35; // 59 0x3B
      u8 unk36; // 60 0x3C
      u8 unk37; // 61 0x3D
      u8 unk38; // 62 0x3E
      u8 unk39; // 63 0x3F
    };
    u8 index[64] = {};

    // In characters, not pixels.
    u32 GetHorizontalDisplayed() const { return u32(end_horizontal_display) + 1; }
    u32 GetHorizontalTotal() const { return u32(horizontal_total) + 5; }

    u32 GetVerticalDisplayed() const
    {
      return (u32(vertical_display_end) | (u32(vertical_display_end_8) << 8) | (u32(vertical_display_end_9) << 9) |
              (u32(vertical_display_end_10) << 10)) +
             1;
    }

    u32 GetVerticalTotal() const
    {
      return (u32(vertical_total) | (u32(vertical_total_8) << 8) | (u32(vertical_total_9) << 9) |
              (u32(vertical_total_10) << 10)) +
             1;
    }

    u32 GetStartAddress() const
    {
      return (ZeroExtend32(extended_start_address.GetValue()) << 16) | (ZeroExtend32(start_address_high) << 8) |
             (ZeroExtend32(start_address_low));
    }

    u32 GetCursorAddress() const
    {
      return (ZeroExtend32(extended_cursor_address.GetValue()) << 16) | (ZeroExtend32(cursor_location_high) << 8) |
             (ZeroExtend32(cursor_location_low));
    }

    u32 GetScanlinesPerRow() const { return u32(maximum_scan_line) + 1; }

    u32 GetLineCompare() const
    {
      return (u32(line_compare) | (u32(line_compare_8) << 8) | (u32(line_compare_9) << 9) |
              (u32(line_compare_10) << 10));
    }
  } m_crtc_registers;

  // 03D0/2/4: CRT (6845) index register
  u8 m_crtc_index_register = 0;
  union
  {
    BitField<u8, u8, 0, 4> write_segment;
    BitField<u8, u8, 4, 4> read_segment;
    u8 bits = 0;
  } m_segment_select_register;

  // 03D1/3/5: CRT data register
  void IOCRTCDataRegisterRead(u8* value);
  void IOCRTCDataRegisterWrite(u8 value);

  // 03CE/03CF: VGA Graphics Registers
  void IOGraphicsDataRegisterRead(u8* value);
  void IOGraphicsDataRegisterWrite(u8 value);

  // Graphics Registers
  union
  {
    u8 index[16] = {};

    struct
    {
      union
      {
        u8 set_reset_register;
        BitField<u8, u8, 0, 4> set_reset;
      };
      union
      {
        u8 enable_set_reset_register;
        BitField<u8, u8, 0, 4> enable_set_reset;
      };
      union
      {
        u8 color_compare_register;
        BitField<u8, u8, 0, 4> color_compare;
      };
      union
      {
        u8 data_rotate_register;
        BitField<u8, u8, 0, 4> rotate_count;
        BitField<u8, u8, 3, 2> logic_op;
      };
      union
      {
        u8 read_map_select_register;
        BitField<u8, u8, 0, 2> read_map_select;
      };
      union
      {
        u8 graphics_mode_register;

        BitField<u8, u8, 0, 2> write_mode;
        BitField<u8, u8, 3, 1> read_mode;
        BitField<u8, bool, 4, 1> host_odd_even;
        BitField<u8, bool, 5, 1> shift_reg;
        BitField<u8, bool, 6, 1> shift_256;
      } mode;
      union
      {
        u8 miscellaneous_graphics_register;

        BitField<u8, bool, 0, 1> text_mode_disable;
        BitField<u8, bool, 1, 1> chain_odd_even_enable;
        BitField<u8, u8, 2, 2> memory_map_select;
      } misc;
      union
      {
        u8 color_dont_care_register;
        BitField<u8, u8, 0, 4> color_dont_care;
      };
      u8 bit_mask;
    };
  } m_graphics_registers;
  u8 m_graphics_address_register = 0;

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
  void IOMiscOutputRegisterWrite(u8 value);

  // 03CA/03DA: Feature Control Register
  u8 m_feature_control_register = 0;

  // 46E8/03C3: VGA adapter enable
  union
  {
    u8 bits = 0;

    BitField<u8, bool, 3, 1> enable_io;
  } m_vga_adapter_enable;

  // 3C0/3C1: Attribute Controller Registers
  union
  {
    u8 index[32] = {};

    struct
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
        BitField<u8, bool, 6, 1> pelclock_div2;      // halves pixel clock, reducing horizontal resolution
        BitField<u8, bool, 7, 1> palette_bits_5_4_select;
      } attribute_mode_control;
      u8 overscan_color;
      union
      {
        BitField<u8, u8, 0, 4> color_plane_enable;
      };
      u8 horizontal_pixel_panning;
      u8 color_select;
      union
      {
        BitField<u8, u8, 4, 2> high_resolution_mode;
        BitField<u8, bool, 6, 1> character_code_2byte;
        BitField<u8, bool, 7, 1> disable_internal_palette;
      };
    };
  } m_attribute_registers;
  u8 m_attribute_address_register = 0;
  bool m_atc_palette_access = false;

  void IOAttributeAddressRead(u8* value);
  void IOAttributeDataRead(u8* value);
  void IOAttributeAddressDataWrite(u8 value);

  // 03C4/03C5: Sequencer Registers (also known as TS)
  union
  {
    u8 index[8];

    struct
    {
      union
      {
        BitField<u8, bool, 0, 1> asynchronous_reset;
        BitField<u8, bool, 1, 1> synchronous_reset;
      } reset_register; // 00

      union
      {
        BitField<u8, u8, 0, 1> dot_mode; // set - 8 dots/char
        BitField<u8, bool, 2, 1> shift_load_rate;
        BitField<u8, bool, 3, 1> dot_clock_rate;
        BitField<u8, bool, 4, 1> shift_four_enable;
        BitField<u8, bool, 5, 1> screen_disable;
      } clocking_mode; // 01

      union
      {
        BitField<u8, u8, 0, 4> memory_plane_write_enable;
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
      } sequencer_memory_mode; // 04

      u8 reserved; // 05

      union
      {
        BitField<u8, u8, 1, 2> timing_sequencer_state;
        // affects dots per character in text mode plus dot_mode
        // 111 - 16, 100 - 12, 011 - 11, 010 - 10, 001 - 8, 000 - 9
      }; // 06

      union
      {
        BitField<u8, bool, 0, 1> mclk_div4;
        BitField<u8, bool, 1, 1> sclk_div2;
        BitField<u8, u8, 3, 1> bios_rom_address_map_0;
        BitField<u8, u8, 5, 1> bios_rom_address_map_1;
        BitField<u8, bool, 6, 1> mclk_div2;
        BitField<u8, bool, 7, 1> vga_mode;
      }; // 07
    };

    u32 GetCharacterWidth() const { return clocking_mode.dot_mode ? 8 : 9; }
  } m_sequencer_registers;
  u8 m_sequencer_address_register = 0;
  void IOSequencerDataRegisterRead(u8* value);
  void IOSequencerDataRegisterWrite(u8 value);

  // 03C7/03C8/03C9: DAC/color registers
  // Colors are organized as RGBA values
  std::array<u32, 256> m_dac_palette;
  u8 m_dac_ctrl = 0;
  u8 m_dac_mask = 0xFF;
  u8 m_dac_status_register = 0;
  u8 m_dac_state_register = 0;
  u8 m_dac_write_address = 0;
  u8 m_dac_read_address = 0;
  u8 m_dac_color_index = 0;
  void IODACMaskRead(u8* value);
  void IODACMaskWrite(u8 value);
  void IODACStateRegisterRead(u8* value);
  void IODACReadAddressWrite(u8 value);
  void IODACWriteAddressRead(u8* value);
  void IODACWriteAddressWrite(u8 value);
  void IODACDataRegisterRead(u8* value);
  void IODACDataRegisterWrite(u8 value);

  // 03D8/03B8: 6845 compatibility registers
  // uint8 m_mc6845_compat_reg_mode_control = 0;
  // uint8 m_mc6845_compat_reg_mono_mode_control = 0;

  Clock m_clock;
  std::string m_bios_file_path;
  std::unique_ptr<byte[]> m_bios_rom;
  u32 m_bios_size = 0;
  MMIO* m_bios_mmio = nullptr;

  u8 m_vram[VRAM_SIZE];
  MMIO* m_vram_mmio = nullptr;
  bool MapToVRAMOffset(u32* offset);
  void HandleVRAMRead(u32 offset, u8* value);
  void HandleVRAMWrite(u32 offset, u8 value);
  bool IsBIOSAddressMapped(u32 offset, u32 size);
  bool LoadBIOSROM();
  void RegisterVRAMMMIO();

  // latch for vram reads
  u32 m_latch = 0;

  // palette used when rendering
  void SetOutputPalette16();
  void SetOutputPalette256();
  std::array<u32, 256> m_output_palette;

  // retrace event
  void RecalculateEventTiming();
  TimingEvent::Pointer m_retrace_event;

  // Timing
  struct
  {
    float horizontal_frequency;
    float vertical_frequency;

    // NOTE: We're not calculating the front porch/sync/back porch durations here, just the whole thing.
    SimulationTime horizontal_active_duration;
    SimulationTime horizontal_total_duration;
    SimulationTime vertical_active_duration;
    SimulationTime vertical_total_duration;
  } m_timing;

  struct ScanoutInfo
  {
    u32 current_line;
    bool in_horizontal_blank;
    bool in_vertical_blank;
    bool display_active;
  };
  ScanoutInfo GetScanoutInfo();

  // We only keep this for stats purposes currently, but it could be extended to when we need to redraw for per-scanline
  // rendering.
  float m_last_rendered_vertical_frequency = 0.0f;

  // Cursor state for text modes
  u8 m_cursor_counter = 0;
  bool m_cursor_state = false;
};
} // namespace HW
