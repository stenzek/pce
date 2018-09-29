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
  static const uint32 SERIALIZATION_ID = MakeSerializationID('E', 'T', '4', 'K');
  static const uint32 MAX_BIOS_SIZE = 32768;
  static const uint32 VRAM_SIZE = 1048576;
  static const uint32 VRAM_MASK = 1048576 - 1;
  static const uint32 VRAM_SIZE_PER_PLANE = VRAM_SIZE / 4;
  static const uint32 VRAM_MASK_PER_PLANE = (VRAM_SIZE / 4) - 1;

public:
  ET4000(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~ET4000();

  void SetBIOSFilePath(const std::string& path) { m_bios_file_path = path; }

  const uint8* GetVRAM() const { return m_vram; }
  uint8* GetVRAM() { return m_vram; }

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

private:
  void ConnectIOPorts();

  uint32 CRTCReadVRAMPlanes(uint32 address_counter, uint32 row_scan_counter) const;
  uint32 CRTCWrapAddress(uint32 address_counter, uint32 row_scan_counter) const;

  void Render();
  void RenderTextMode();
  void RenderGraphicsMode();

  void DrawTextGlyph8(uint32 fb_x, uint32 fb_y, const uint8* glyph, uint32 rows, uint32 fg_color, uint32 bg_color,
                      int32 dup9);
  void DrawTextGlyph16(uint32 fb_x, uint32 fb_y, const uint8* glyph, uint32 rows, uint32 fg_color, uint32 bg_color);

  // void DrawGraphicsLine

  std::unique_ptr<Display> m_display = nullptr;

  // 03C2h: Status register 0
  uint8 m_st0 = 0;

  // 03DAh: Status register 1
  union StatusRegister1
  {
    BitField<uint8, bool, 0, 1> display_enable_n;
    BitField<uint8, bool, 3, 1> vertical_blank;
    BitField<uint8, uint8, 4, 2> display_feedback_test;
    BitField<uint8, bool, 7, 1> vertical_blank_n;
    uint8 bits = 0;
  };
  void IOReadStatusRegister1(uint8* value);

  // CRTC registers
  union
  {
    struct
    {
      uint8 horizontal_total;          // 0  0x00
      uint8 end_horizontal_display;    // 1  0x01
      uint8 start_horizontal_blanking; // 2  0x02
      uint8 end_horizontal_blanking;   // 3  0x03
      uint8 start_horizontal_retrace;  // 4  0x04
      uint8 end_horizontal_retrace;    // 5  0x05
      uint8 vertical_total;            // 6  0x06
      union
      {
        BitField<uint8, uint8, 0, 1> vertical_total_8;
        BitField<uint8, uint8, 1, 1> vertical_display_end_8;
        BitField<uint8, uint8, 2, 1> vertical_retrace_start_8;
        BitField<uint8, uint8, 3, 1> start_vertical_blanking_8;
        BitField<uint8, uint8, 4, 1> line_compare_8;
        BitField<uint8, uint8, 5, 1> vertical_total_9;
        BitField<uint8, uint8, 6, 1> vertical_display_end_9;
        BitField<uint8, uint8, 7, 1> vertical_retrace_start_9;
      }; // 7  0x07
      union
      {
        BitField<uint8, uint8, 0, 5> preset_row_scan;
        BitField<uint8, uint8, 5, 2> byte_panning;
      }; // 8  0x08
      union
      {
        BitField<uint8, uint8, 0, 5> maximum_scan_line;
        BitField<uint8, uint8, 5, 1> start_vertical_blanking_9;
        BitField<uint8, uint8, 6, 1> line_compare_9;
        BitField<uint8, bool, 7, 1> scan_doubling;
      };                             // 9  0x09
      uint8 cursor_start;            // 10 0x0A
      uint8 cursor_end;              // 11 0x0B
      uint8 start_address_high;      // 12 0x0C
      uint8 start_address_low;       // 13 0x0D
      uint8 cursor_location_high;    // 14 0x0E
      uint8 cursor_location_low;     // 15 0x0F
      uint8 vertical_retrace_start;  // 16 0x10
      uint8 vertical_retrace_end;    // 17 0x11
      uint8 vertical_display_end;    // 18 0x12
      uint8 offset;                  // 19 0x13
      uint8 underline_location;      // 20 0x14
      uint8 start_vertical_blanking; // 21 0x15
      uint8 end_vertical_blanking;   // 22 0x16
      union
      {
        BitField<uint8, bool, 0, 1> alternate_la13;
        BitField<uint8, bool, 1, 1> alternate_la14;
        BitField<uint8, bool, 2, 1> line_counter_mul2;
        BitField<uint8, bool, 3, 1> linear_counter_mul2;
        BitField<uint8, bool, 4, 1> memory_address_output_control;
        BitField<uint8, bool, 5, 1> alternate_ma00_output;
        BitField<uint8, bool, 6, 1> byte_mode;
        BitField<uint8, bool, 7, 1> hold_mode;
        uint8 bits;
      } crtc_mode_control; // 23 0x17
      uint8 line_compare;  // 24 0x18
      uint8 unk1;          // 25 0x19
      uint8 unk2;          // 26 0x1A
      uint8 unk3;          // 27 0x1B
      uint8 unk4;          // 28 0x1C
      uint8 unk5;          // 29 0x1D
      uint8 unk6;          // 30 0x1E
      uint8 unk7;          // 31 0x1F
      uint8 unk8;          // 32 0x20
      uint8 unk9;          // 33 0x21
      uint8 unk10;         // 34 0x22
      uint8 unk11;         // 35 0x23
      union
      {
        uint8 unk12; // 36 0x24
        BitField<uint8, bool, 7, 1> attribute_register_flipflop;
      };
      uint8 unk13; // 37 0x25
      uint8 unk14; // 38 0x26
      uint8 unk15; // 39 0x27
      uint8 unk16; // 40 0x28
      uint8 unk17; // 41 0x29
      uint8 unk18; // 42 0x2A
      uint8 unk19; // 43 0x2B
      uint8 unk20; // 44 0x2C
      uint8 unk21; // 45 0x2D
      uint8 unk22; // 46 0x2E
      uint8 unk23; // 47 0x2F
      uint8 unk24; // 48 0x30
      uint8 unk25; // 49 0x31
      union
      {
        BitField<uint8, uint8, 0, 2> csw;
        BitField<uint8, uint8, 2, 1> csp;
        BitField<uint8, uint8, 3, 2> rsp;
        BitField<uint8, uint8, 5, 1> ras_to_cas;
        BitField<uint8, uint8, 6, 1> col_setup_time;
        BitField<uint8, uint8, 7, 1> static_col_mem;
      } ras_cas_config; // 50 0x32
      union
      {
        BitField<uint8, uint8, 0, 2> extended_start_address;
        BitField<uint8, uint8, 2, 2> extended_cursor_address;
      }; // 51 0x33
      union
      {
        BitField<uint8, bool, 0, 1> cs0_translation;
        BitField<uint8, uint8, 1, 1> clock_select_2;
        BitField<uint8, bool, 2, 1> tristate;
        BitField<uint8, bool, 3, 1> vse_register_port;    // 1=46E8, 0=3C3
        BitField<uint8, bool, 4, 1> read_translation;     // ENXR
        BitField<uint8, bool, 5, 1> write_translation;    // ENXL
        BitField<uint8, bool, 6, 1> doublescan_underline; // ENBA
        BitField<uint8, bool, 7, 1> enable;
      } mc6845_compatibility_control; // 52 0x34
      union
      {
        BitField<uint8, uint8, 0, 1> vertical_blank_start_10;
        BitField<uint8, uint8, 1, 1> vertical_total_10;
        BitField<uint8, uint8, 2, 1> vertical_display_end_10;
        BitField<uint8, uint8, 3, 1> vertical_sync_start_10;
        BitField<uint8, uint8, 4, 1> line_compare_10;
        BitField<uint8, bool, 5, 1> external_sync_counters;
        BitField<uint8, bool, 6, 1> alternate_rmw_control;
        BitField<uint8, bool, 7, 1> vertical_interlace_mode; // doubles vertical resolution with same timing
      };                                                     // 53 0x35
      union
      {
        BitField<uint8, uint8, 0, 3> refresh_count;
        BitField<uint8, bool, 3, 1> font_width_control;
        BitField<uint8, bool, 4, 1> linear_mapping;     // from CPU
        BitField<uint8, bool, 5, 1> contiguous_mapping; // from CPU
        BitField<uint8, bool, 6, 1> enable_16bit_vram;
        BitField<uint8, bool, 7, 1> enable_16bit_io;
      }; // 54 0x36
      union
      {
        BitField<uint8, uint8, 0, 2> display_memory_data_bus_width;
        BitField<uint8, uint8, 2, 2> display_memory_data_depth;
        BitField<uint8, bool, 4, 1> enable_16bit_rom;
        BitField<uint8, bool, 5, 1> priority_threshold_control;
        BitField<uint8, bool, 6, 1> tli_internal_test;
        BitField<uint8, bool, 7, 1> display_memory_type;
      };           // 55 0x37
      uint8 unk32; // 56 0x38
      uint8 unk33; // 57 0x39
      uint8 unk34; // 58 0x3A
      uint8 unk35; // 59 0x3B
      uint8 unk36; // 60 0x3C
      uint8 unk37; // 61 0x3D
      uint8 unk38; // 62 0x3E
      uint8 unk39; // 63 0x3F
    };
    uint8 index[64] = {};

    // In characters, not pixels.
    uint32 GetHorizontalDisplayed() const { return uint32(end_horizontal_display) + 1; }
    uint32 GetHorizontalTotal() const { return uint32(horizontal_total) + 5; }

    uint32 GetVerticalDisplayed() const
    {
      return (uint32(vertical_display_end) | (uint32(vertical_display_end_8) << 8) |
              (uint32(vertical_display_end_9) << 9) | (uint32(vertical_display_end_10) << 10)) +
             1;
    }

    uint32 GetVerticalTotal() const
    {
      return (uint32(vertical_total) | (uint32(vertical_total_8) << 8) | (uint32(vertical_total_9) << 9) |
              (uint32(vertical_total_10) << 10)) +
             1;
    }

    uint32 GetStartAddress() const
    {
      return (ZeroExtend32(extended_start_address.GetValue()) << 16) | (ZeroExtend32(start_address_high) << 8) |
             (ZeroExtend32(start_address_low));
    }

    uint32 GetCursorAddress() const
    {
      return (ZeroExtend32(extended_cursor_address.GetValue()) << 16) | (ZeroExtend32(cursor_location_high) << 8) |
             (ZeroExtend32(cursor_location_low));
    }

    uint32 GetScanlinesPerRow() const { return uint32(maximum_scan_line) + 1; }

    uint32 GetLineCompare() const
    {
      return (uint32(line_compare) | (uint32(line_compare_8) << 8) | (uint32(line_compare_9) << 9) |
              (uint32(line_compare_10) << 10));
    }
  } m_crtc_registers;

  // 03D0/2/4: CRT (6845) index register
  uint8 m_crtc_index_register = 0;
  union
  {
    BitField<uint8, uint8, 0, 4> write_segment;
    BitField<uint8, uint8, 4, 4> read_segment;
    uint8 bits = 0;
  } m_segment_select_register;

  // 03D1/3/5: CRT data register
  void IOCRTCDataRegisterRead(uint8* value);
  void IOCRTCDataRegisterWrite(uint8 value);

  // 03CE/03CF: VGA Graphics Registers
  void IOGraphicsDataRegisterRead(uint8* value);
  void IOGraphicsDataRegisterWrite(uint8 value);

  // Graphics Registers
  union
  {
    uint8 index[16] = {};

    struct
    {
      union
      {
        uint8 set_reset_register;
        BitField<uint8, uint8, 0, 4> set_reset;
      };
      union
      {
        uint8 enable_set_reset_register;
        BitField<uint8, uint8, 0, 4> enable_set_reset;
      };
      union
      {
        uint8 color_compare_register;
        BitField<uint8, uint8, 0, 4> color_compare;
      };
      union
      {
        uint8 data_rotate_register;
        BitField<uint8, uint8, 0, 4> rotate_count;
        BitField<uint8, uint8, 3, 2> logic_op;
      };
      union
      {
        uint8 read_map_select_register;
        BitField<uint8, uint8, 0, 2> read_map_select;
      };
      union
      {
        uint8 graphics_mode_register;

        BitField<uint8, uint8, 0, 2> write_mode;
        BitField<uint8, uint8, 3, 1> read_mode;
        BitField<uint8, bool, 4, 1> host_odd_even;
        BitField<uint8, bool, 5, 1> shift_reg;
        BitField<uint8, bool, 6, 1> shift_256;
      } mode;
      union
      {
        uint8 miscellaneous_graphics_register;

        BitField<uint8, bool, 0, 1> text_mode_disable;
        BitField<uint8, bool, 1, 1> chain_odd_even_enable;
        BitField<uint8, uint8, 2, 2> memory_map_select;
      } misc;
      union
      {
        uint8 color_dont_care_register;
        BitField<uint8, uint8, 0, 4> color_dont_care;
      };
      uint8 bit_mask;
    };
  } m_graphics_registers;
  uint8 m_graphics_address_register = 0;

  // 03CC/03C2: Miscellaneous Output Register
  union
  {
    uint8 bits = 0;

    BitField<uint8, bool, 0, 1> io_address_select;
    BitField<uint8, bool, 1, 1> ram_enable;
    BitField<uint8, uint8, 2, 2> clock_select;
    BitField<uint8, bool, 5, 1> odd_even_page;
    BitField<uint8, bool, 6, 1> hsync_polarity;
    BitField<uint8, bool, 7, 1> vsync_polarity;
  } m_misc_output_register;
  void IOMiscOutputRegisterWrite(uint8 value);

  // 03CA/03DA: Feature Control Register
  uint8 m_feature_control_register = 0;

  // 46E8/03C3: VGA adapter enable
  union
  {
    uint8 bits = 0;

    BitField<uint8, bool, 3, 1> enable_io;
  } m_vga_adapter_enable;

  // 3C0/3C1: Attribute Controller Registers
  union
  {
    uint8 index[32] = {};

    struct
    {
      uint8 palette[16];
      union
      {
        uint8 bits;
        BitField<uint8, bool, 0, 1> graphics_enable;
        BitField<uint8, bool, 1, 1> mono_emulation;
        BitField<uint8, bool, 2, 1> line_graphics_enable;
        BitField<uint8, bool, 3, 1> blink_enable;
        BitField<uint8, bool, 5, 1> pixel_panning_mode; // disable panning while in split screen
        BitField<uint8, bool, 6, 1> pelclock_div2;      // halves pixel clock, reducing horizontal resolution
        BitField<uint8, bool, 7, 1> palette_bits_5_4_select;
      } attribute_mode_control;
      uint8 overscan_color;
      union
      {
        BitField<uint8, uint8, 0, 4> color_plane_enable;
      };
      uint8 horizontal_pixel_panning;
      uint8 color_select;
      union
      {
        BitField<uint8, uint8, 4, 2> high_resolution_mode;
        BitField<uint8, bool, 6, 1> character_code_2byte;
        BitField<uint8, bool, 7, 1> disable_internal_palette;
      };
    };
  } m_attribute_registers;
  uint8 m_attribute_address_register = 0;
  bool m_atc_palette_access = false;

  void IOAttributeAddressRead(uint8* value);
  void IOAttributeDataRead(uint8* value);
  void IOAttributeAddressDataWrite(uint8 value);

  // 03C4/03C5: Sequencer Registers (also known as TS)
  union
  {
    uint8 index[8];

    struct
    {
      union
      {
        BitField<uint8, bool, 0, 1> asynchronous_reset;
        BitField<uint8, bool, 1, 1> synchronous_reset;
      } reset_register; // 00

      union
      {
        BitField<uint8, uint8, 0, 1> dot_mode; // set - 8 dots/char
        BitField<uint8, bool, 2, 1> shift_load_rate;
        BitField<uint8, bool, 3, 1> dot_clock_rate;
        BitField<uint8, bool, 4, 1> shift_four_enable;
        BitField<uint8, bool, 5, 1> screen_disable;
      } clocking_mode; // 01

      union
      {
        BitField<uint8, uint8, 0, 4> memory_plane_write_enable;
      }; // 02

      union
      {
        BitField<uint8, uint8, 0, 2> character_set_b_select_01;
        BitField<uint8, uint8, 2, 2> character_set_a_select_01;
        BitField<uint8, uint8, 4, 1> character_set_b_select_2;
        BitField<uint8, uint8, 5, 1> character_set_a_select_2;
      }; // 03

      union
      {
        BitField<uint8, bool, 1, 1> extended_memory;
        BitField<uint8, bool, 2, 1> odd_even_host_memory;
        BitField<uint8, bool, 3, 1> chain_4_enable;
      } sequencer_memory_mode; // 04

      uint8 reserved; // 05

      union
      {
        BitField<uint8, uint8, 1, 2> timing_sequencer_state;
        // affects dots per character in text mode plus dot_mode
        // 111 - 16, 100 - 12, 011 - 11, 010 - 10, 001 - 8, 000 - 9
      }; // 06

      union
      {
        BitField<uint8, bool, 0, 1> mclk_div4;
        BitField<uint8, bool, 1, 1> sclk_div2;
        BitField<uint8, uint8, 3, 1> bios_rom_address_map_0;
        BitField<uint8, uint8, 5, 1> bios_rom_address_map_1;
        BitField<uint8, bool, 6, 1> mclk_div2;
        BitField<uint8, bool, 7, 1> vga_mode;
      }; // 07
    };

    uint32 GetCharacterWidth() const { return clocking_mode.dot_mode ? 8 : 9; }
  } m_sequencer_registers;
  uint8 m_sequencer_address_register = 0;
  void IOSequencerDataRegisterRead(uint8* value);
  void IOSequencerDataRegisterWrite(uint8 value);

  // 03C7/03C8/03C9: DAC/color registers
  // Colors are organized as RGBA values
  std::array<uint32, 256> m_dac_palette;
  uint8 m_dac_ctrl = 0;
  uint8 m_dac_mask = 0xFF;
  uint8 m_dac_status_register = 0;
  uint8 m_dac_state_register = 0;
  uint8 m_dac_write_address = 0;
  uint8 m_dac_read_address = 0;
  uint8 m_dac_color_index = 0;
  void IODACMaskRead(uint8* value);
  void IODACMaskWrite(uint8 value);
  void IODACStateRegisterRead(uint8* value);
  void IODACReadAddressWrite(uint8 value);
  void IODACWriteAddressRead(uint8* value);
  void IODACWriteAddressWrite(uint8 value);
  void IODACDataRegisterRead(uint8* value);
  void IODACDataRegisterWrite(uint8 value);

  // 03D8/03B8: 6845 compatibility registers
  // uint8 m_mc6845_compat_reg_mode_control = 0;
  // uint8 m_mc6845_compat_reg_mono_mode_control = 0;

  Clock m_clock;
  std::string m_bios_file_path;
  std::unique_ptr<byte[]> m_bios_rom;
  uint32 m_bios_size = 0;
  MMIO* m_bios_mmio = nullptr;

  uint8 m_vram[VRAM_SIZE];
  MMIO* m_vram_mmio = nullptr;
  bool MapToVRAMOffset(uint32* offset);
  void HandleVRAMRead(uint32 offset, uint8* value);
  void HandleVRAMWrite(uint32 offset, uint8 value);
  bool IsBIOSAddressMapped(uint32 offset, uint32 size);
  bool LoadBIOSROM();
  void RegisterVRAMMMIO();

  // latch for vram reads
  uint32 m_latch = 0;

  // palette used when rendering
  void SetOutputPalette16();
  void SetOutputPalette256();
  std::array<uint32, 256> m_output_palette;

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
    uint32 current_line;
    bool in_horizontal_blank;
    bool in_vertical_blank;
    bool display_active;
  };
  ScanoutInfo GetScanoutInfo();

  // We only keep this for stats purposes currently, but it could be extended to when we need to redraw for per-scanline
  // rendering.
  float m_last_rendered_vertical_frequency = 0.0f;

  // Cursor state for text modes
  uint8 m_cursor_counter = 0;
  bool m_cursor_state = false;
};
} // namespace HW
