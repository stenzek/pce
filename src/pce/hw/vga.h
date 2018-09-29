#pragma once

#include "common/bitfield.h"
#include "common/clock.h"
#include "pce/component.h"
#include "pce/system.h"
#include <array>
#include <memory>
#include <string>

class Display;
class ByteStream;
class MMIO;

namespace HW {

class VGA : public Component
{
  DECLARE_OBJECT_TYPE_INFO(VGA, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(VGA);
  DECLARE_OBJECT_PROPERTY_MAP(VGA);

public:
  static const uint32 SERIALIZATION_ID = MakeSerializationID('V', 'G', 'A');
  static const uint32 MAX_BIOS_SIZE = 65536;
  static const uint32 VRAM_SIZE = 524288;

public:
  VGA(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~VGA();

  void SetBIOSFilePath(const std::string& path) { m_bios_file_path = path; }

  const uint8* GetVRAM() const { return m_vram; }
  uint8* GetVRAM() { return m_vram; }

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

private:
  void ConnectIOPorts();
  bool LoadBIOSROM();

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
  union
  {
    uint8 bits = 0;
    BitField<uint8, bool, 0, 1> display_disabled;
    BitField<uint8, bool, 3, 1> vblank;
  } m_st1;

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
      uint8 overflow_register;         // 7  0x07
      uint8 preset_row_scan;           // 8  0x08
      uint8 maximum_scan_lines;        // 9  0x09
      uint8 cursor_start;              // 10 0x0A
      uint8 cursor_end;                // 11 0x0B
      uint8 start_address_high;        // 12 0x0C
      uint8 start_address_low;         // 13 0x0D
      uint8 cursor_location_high;      // 14 0x0E
      uint8 cursor_location_low;       // 15 0x0F
      uint8 vertical_retrace_start;    // 16 0x10
      uint8 vertical_retrace_end;      // 17 0x11
      uint8 vertical_display_end;      // 18 0x12
      uint8 offset;                    // 19 0x13
      uint8 underline_location;        // 20 0x14
      uint8 start_vertical_blanking;   // 21 0x15
      uint8 end_vertical_blanking;     // 22 0x16
      uint8 crtc_mode_control;         // 23 0x17
      uint8 line_compare;              // 24 0x18
      uint8 unk1;
      uint8 unk2;
      uint8 unk3;
      uint8 unk4;
      uint8 unk5;
      uint8 unk6;
      uint8 unk7;
      uint8 unk8;
      uint8 unk9;
      uint8 unk10;
      uint8 unk11;
      union
      {
        uint8 unk12;
        BitField<uint8, bool, 7, 1> attribute_register_flipflop;
      };
    };
    uint8 index[37] = {};
  } m_crtc_registers;

  // 03D0/2/4: CRT (6845) index register
  uint8 m_crtc_index_register = 0;

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
    uint8 index[21] = {};

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
        BitField<uint8, bool, 5, 1> pixel_panning_mode;
        BitField<uint8, bool, 6, 1> eight_bit_mode;
        BitField<uint8, bool, 7, 1> palette_bits_5_4_select;
      } attribute_mode_control;
      uint8 overscan_color;
      union
      {
        BitField<uint8, uint8, 0, 4> color_plane_enable;
      };
      uint8 horizontal_pixel_panning;
      uint8 color_select;
    };
  } m_attribute_registers;
  uint8 m_attribute_address_register = 0;
  bool m_attribute_video_enabled = false;

  void IOAttributeAddressRead(uint8* value);
  void IOAttributeDataRead(uint8* value);
  void IOAttributeAddressDataWrite(uint8 value);

  // 03C4/03C5: Sequencer Registers
  union
  {
    uint8 index[5];

    struct
    {
      union
      {
        BitField<uint8, bool, 0, 1> asynchronous_reset;
        BitField<uint8, bool, 1, 1> synchronous_reset;
      } reset_register; // 00

      union
      {
        BitField<uint8, bool, 0, 1> dot_mode;
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
    };
  } m_sequencer_registers;
  uint8 m_sequencer_address_register = 0;
  void IOSequencerDataRegisterRead(uint8* value);
  void IOSequencerDataRegisterWrite(uint8 value);

  // 03C7/03C8/03C9: DAC/color registers
  // Colors are organized as RGBA values
  std::array<uint32, 256> m_dac_palette;
  uint8 m_dac_state_register = 0; // TODO Handling
  uint8 m_dac_write_address = 0;
  uint8 m_dac_read_address = 0;
  uint8 m_dac_color_index = 0;
  void IODACReadAddressWrite(uint8 value);
  void IODACWriteAddressWrite(uint8 value);
  void IODACDataRegisterRead(uint8* value);
  void IODACDataRegisterWrite(uint8 value);

  Clock m_clock;
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
  uint8 m_vram[VRAM_SIZE];
  MMIO* m_vram_mmio = nullptr;
  bool MapToVRAMOffset(uint32* offset);
  void HandleVRAMRead(uint32 offset, uint8* value);
  void HandleVRAMWrite(uint32 offset, uint8 value);
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