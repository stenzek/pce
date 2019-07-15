#pragma once

#include "common/bitfield.h"
#include "../component.h"
#include "../system.h"
#include <array>
#include <memory>
#include <string>

class Display;
class MMIO;

namespace HW {

class VGA : public Component
{
  DECLARE_OBJECT_TYPE_INFO(VGA, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(VGA);
  DECLARE_OBJECT_PROPERTY_MAP(VGA);

public:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('V', 'G', 'A');
  static constexpr u32 MAX_BIOS_SIZE = 65536;
  static constexpr u32 VRAM_SIZE = 524288;

public:
  VGA(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~VGA();

  void SetBIOSFilePath(const std::string& path) { m_bios_file_path = path; }

  const u8* GetVRAM() const { return m_vram; }
  u8* GetVRAM() { return m_vram; }

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

private:
  void ConnectIOPorts();
  bool LoadBIOSROM();

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
  union
  {
    u8 bits = 0;
    BitField<u8, bool, 0, 1> display_disabled;
    BitField<u8, bool, 3, 1> vblank;
  } m_st1;

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
      u8 overflow_register;         // 7  0x07
      u8 preset_row_scan;           // 8  0x08
      u8 maximum_scan_lines;        // 9  0x09
      u8 cursor_start;              // 10 0x0A
      u8 cursor_end;                // 11 0x0B
      u8 start_address_high;        // 12 0x0C
      u8 start_address_low;         // 13 0x0D
      u8 cursor_location_high;      // 14 0x0E
      u8 cursor_location_low;       // 15 0x0F
      u8 vertical_retrace_start;    // 16 0x10
      u8 vertical_retrace_end;      // 17 0x11
      u8 vertical_display_end;      // 18 0x12
      u8 offset;                    // 19 0x13
      u8 underline_location;        // 20 0x14
      u8 start_vertical_blanking;   // 21 0x15
      u8 end_vertical_blanking;     // 22 0x16
      u8 crtc_mode_control;         // 23 0x17
      u8 line_compare;              // 24 0x18
      u8 unk1;
      u8 unk2;
      u8 unk3;
      u8 unk4;
      u8 unk5;
      u8 unk6;
      u8 unk7;
      u8 unk8;
      u8 unk9;
      u8 unk10;
      u8 unk11;
      union
      {
        u8 unk12;
        BitField<u8, bool, 7, 1> attribute_register_flipflop;
      };
    };
    u8 index[37] = {};
  } m_crtc_registers;

  // 03D0/2/4: CRT (6845) index register
  u8 m_crtc_index_register = 0;

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
    u8 index[21] = {};

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
        BitField<u8, bool, 5, 1> pixel_panning_mode;
        BitField<u8, bool, 6, 1> eight_bit_mode;
        BitField<u8, bool, 7, 1> palette_bits_5_4_select;
      } attribute_mode_control;
      u8 overscan_color;
      union
      {
        BitField<u8, u8, 0, 4> color_plane_enable;
      };
      u8 horizontal_pixel_panning;
      u8 color_select;
    };
  } m_attribute_registers;
  u8 m_attribute_address_register = 0;
  bool m_attribute_video_enabled = false;

  void IOAttributeAddressRead(u8* value);
  void IOAttributeDataRead(u8* value);
  void IOAttributeAddressDataWrite(u8 value);

  // 03C4/03C5: Sequencer Registers
  union
  {
    u8 index[5];

    struct
    {
      union
      {
        BitField<u8, bool, 0, 1> asynchronous_reset;
        BitField<u8, bool, 1, 1> synchronous_reset;
      } reset_register; // 00

      union
      {
        BitField<u8, bool, 0, 1> dot_mode;
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
    };
  } m_sequencer_registers;
  u8 m_sequencer_address_register = 0;
  void IOSequencerDataRegisterRead(u8* value);
  void IOSequencerDataRegisterWrite(u8 value);

  // 03C7/03C8/03C9: DAC/color registers
  // Colors are organized as RGBA values
  std::array<u32, 256> m_dac_palette;
  u8 m_dac_state_register = 0; // TODO Handling
  u8 m_dac_write_address = 0;
  u8 m_dac_read_address = 0;
  u8 m_dac_color_index = 0;
  void IODACReadAddressWrite(u8 value);
  void IODACWriteAddressWrite(u8 value);
  void IODACDataRegisterRead(u8* value);
  void IODACDataRegisterWrite(u8 value);

  std::string m_bios_file_path;

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
  u8 m_vram[VRAM_SIZE];
  MMIO* m_vram_mmio = nullptr;
  bool MapToVRAMOffset(u32* offset);
  void HandleVRAMRead(u32 offset, u8* value);
  void HandleVRAMWrite(u32 offset, u8 value);
  void RegisterVRAMMMIO();

  // latch for vram reads
  u32 m_latch = 0;

  // palette used when rendering
  void SetOutputPalette16();
  void SetOutputPalette256();
  std::array<u32, 256> m_output_palette;

  // retrace event
  void RecalculateEventTiming();
  std::unique_ptr<TimingEvent> m_retrace_event;

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