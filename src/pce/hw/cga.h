#pragma once

#include "../component.h"
#include "../system.h"
#include "common/bitfield.h"

class Display;

namespace HW {

class CGA : public Component
{
  DECLARE_OBJECT_TYPE_INFO(CGA, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(CGA);
  DECLARE_OBJECT_PROPERTY_MAP(CGA);

public:
  static constexpr float CLOCK_FREQUENCY = 3579545.0f;
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('C', 'G', 'A');
  static constexpr u32 VRAM_SIZE = 16384;
  static constexpr u32 PIXEL_CLOCK = 14318181;
  static constexpr u32 NUM_CRTC_REGISTERS = 18;
  static constexpr u32 CHARACTER_WIDTH = 8;
  static constexpr u32 CHARACTER_HEIGHT = 8;
  static constexpr u32 ADDRESS_COUNTER_MASK = 0x3FFF;
  static constexpr u32 ADDRESS_COUNTER_VRAM_MASK_TEXT = 0x1FFF;
  static constexpr u32 ADDRESS_COUNTER_VRAM_MASK_GRAPHICS = 0x0FFF;
  static constexpr u32 CHARACTER_ROW_COUNTER_MASK = 0x1F;
  static constexpr u32 VERTICAL_COUNTER_MASK = 0x7F;
  static constexpr u32 CRTC_ADDRESS_SHIFT = 1;
  static constexpr u32 VSYNC_PULSE_WIDTH = 16;
  static constexpr u8 BLINK_INTERVAL = 8;

public:
  CGA(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~CGA();

  const u8* GetVRAM() const { return m_vram; }
  u8* GetVRAM() { return m_vram; }

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

private:
  u32 GetBorderColor() const;
  u32 GetCursorAddress() const;
  u32 InCursorBox() const;
  void ConnectIOPorts(Bus* bus);
  void RenderLineEvent(CycleCount cycles);
  void BeginFrame();
  void RenderLineText();
  void RenderLineGraphics();
  void RenderLineBorder();
  void FlushFrame();

  std::unique_ptr<Display> m_display;
  u8 m_vram[VRAM_SIZE];

  // 03D8h: Mode control register
  union
  {
    u8 raw = 0;
    BitField<u8, bool, 0, 1> high_resolution;
    BitField<u8, bool, 1, 1> graphics_mode;
    BitField<u8, bool, 2, 1> monochrome;
    BitField<u8, bool, 3, 1> enable_video_output;
    BitField<u8, bool, 4, 1> high_resolution_graphics;
    BitField<u8, bool, 5, 1> enable_blink;
  } m_mode_control_register;
  void ModeControlRegisterWrite(u8 value);

  // 03D9h: Colour control register
  union
  {
    u8 raw = 0;
    BitField<u8, bool, 5, 1> palette_select;
    BitField<u8, bool, 4, 1> foreground_intensity;
    BitField<u8, u8, 0, 4> background_color;
  } m_color_control_register;
  void ColorControlRegisterWrite(u8 value);

  // 03DAh: Status register
  union StatusRegister
  {
    u8 raw;
    BitField<u8, bool, 0, 1> safe_vram_access;
    BitField<u8, bool, 1, 1> light_pen_trigger_set;
    BitField<u8, bool, 2, 1> light_pen_switch_status;
    BitField<u8, bool, 3, 1> vblank;
  };
  void StatusRegisterRead(u8* value);

  // CRTC registers
  union
  {
    struct
    {
      u8 horizontal_total;            // characters
      u8 horizontal_displayed;        // characters
      u8 horizontal_sync_position;    // characters
      u8 horizontal_sync_pulse_width; // characters
      u8 vertical_total;              // character rows
      u8 vertical_total_adjust;       // scanlines
      u8 vertical_displayed;          // character rows
      u8 vertical_sync_position;      // character rows
      u8 interlace_mode;
      u8 maximum_scan_lines; // scan lines
      u8 cursor_start;       // scan lines
      u8 cursor_end;         // scan lines
      u8 start_address_high; // Big-endian
      u8 start_address_low;
      u8 cursor_location_high; // Big-endian
      u8 cursor_location_low;
      u8 light_pen_high;
      u8 light_pen_low;
    };
    u8 index[NUM_CRTC_REGISTERS] = {};
  } m_crtc_registers;

  // 03D0/2/4: CRT (6845) index register
  u8 m_crtc_index_register = 0;

  // 03D1/3/5: CRT data register
  void CRTDataRegisterRead(u8* value);
  void CRTDataRegisterWrite(u8 value);

  // Timing
  struct Timing
  {
    double horizontal_frequency;

    u32 horizontal_left_border_pixels;
    u32 horizontal_right_border_pixels;
    SimulationTime horizontal_display_start_time;
    SimulationTime horizontal_display_end_time;

    u32 vertical_display_end;
    u32 vertical_sync_start;
    u32 vertical_sync_end;

    bool operator==(const Timing& rhs) const;
  };
  Timing m_timing = {};

  void RecalculateEventTiming();

  u32 m_address_counter = 0;
  u32 m_character_row_counter = 0;
  u32 m_current_row = 0;
  u32 m_remaining_adjust_lines = 0;
  std::unique_ptr<TimingEvent> m_line_event;

  // Currently-rendering frame.
  std::vector<u32> m_current_frame;
  u32 m_current_frame_width = 0;
  u32 m_current_frame_line = 0;
  u32 m_current_frame_offset = 0;

  // Blink bit. XOR with the character value.
  u8 m_blink_frame_counter = BLINK_INTERVAL;
  u8 m_cursor_frame_counter = BLINK_INTERVAL;
  u8 m_blink_state = 0;
  u8 m_cursor_state = 0;
};
} // namespace HW