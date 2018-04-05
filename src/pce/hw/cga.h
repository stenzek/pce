#pragma once

#include "pce/bitfield.h"
#include "pce/clock.h"
#include "pce/component.h"
#include "pce/system.h"

class Display;

namespace HW {

class CGA : public Component
{
public:
  static const uint32 SERIALIZATION_ID = MakeSerializationID('C', 'G', 'A');
  static const uint32 VRAM_SIZE = 16384;
  static const uint32 PIXEL_CLOCK = 14318181;
  static const uint32 NUM_CRTC_REGISTERS = 18;
  static const uint32 CHARACTER_WIDTH = 8;
  static const uint32 CHARACTER_HEIGHT = 8;
  static const uint32 ADDRESS_COUNTER_MASK = 0x3FFF;
  static const uint32 ADDRESS_COUNTER_VRAM_MASK_TEXT = 0x1FFF;
  static const uint32 ADDRESS_COUNTER_VRAM_MASK_GRAPHICS = 0x0FFF;
  static const uint32 ROW_COUNTER_MASK = 0x1F;
  static const uint32 CRTC_ADDRESS_SHIFT = 1;

public:
  CGA();
  ~CGA();

  const uint8* GetVRAM() const { return m_vram; }
  uint8* GetVRAM() { return m_vram; }

  virtual void Initialize(System* system, Bus* bus) override;
  virtual void Reset() override;
  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

private:
  void ConnectIOPorts(Bus* bus);
  void Tick(CycleCount cycles);
  void RenderFramebuffer(bool end_of_vblank = false);
  void RenderFramebufferLinesText(uint32 count);
  void RenderFramebufferLinesGraphics(uint32 count);

  System* m_system = nullptr;
  Display* m_display;
  uint8 m_vram[VRAM_SIZE];

  // 03D8h: Mode control register
  union
  {
    uint8 raw = 0;
    BitField<uint8, bool, 0, 1> high_resolution;
    BitField<uint8, bool, 1, 1> graphics_mode;
    BitField<uint8, bool, 2, 1> monochrome;
    BitField<uint8, bool, 3, 1> enable_video_output;
    BitField<uint8, bool, 4, 1> high_resolution_graphics;
    BitField<uint8, bool, 5, 1> enable_blink;
  } m_mode_control_register;
  void ModeControlRegisterWrite(uint8 value);

  // 03D9h: Colour control register
  union
  {
    uint8 raw = 0;
    BitField<uint8, bool, 5, 1> palette_select;
    BitField<uint8, bool, 4, 1> foreground_intensity;
    BitField<uint8, uint8, 0, 4> background_color;
  } m_color_control_register;
  void ColorControlRegisterWrite(uint8 value);

  // 03DAh: Status register
  union StatusRegister
  {
    uint8 raw;
    BitField<uint8, bool, 0, 1> safe_vram_access;
    BitField<uint8, bool, 1, 1> light_pen_trigger_set;
    BitField<uint8, bool, 2, 1> light_pen_switch_status;
    BitField<uint8, bool, 3, 1> vblank;
  };
  void StatusRegisterRead(uint8* value);

  // CRTC registers
  union
  {
    struct
    {
      uint8 horizontal_total;            // characters
      uint8 horizontal_displayed;        // characters
      uint8 horizontal_sync_position;    // characters
      uint8 horizontal_sync_pulse_width; // characters
      uint8 vertical_total;              // character rows
      uint8 vertical_total_adjust;       // scanlines
      uint8 vertical_displayed;          // character rows
      uint8 vertical_sync_position;      // character rows
      uint8 interlace_mode;
      uint8 maximum_scan_lines; // scan lines
      uint8 cursor_start;       // scan lines
      uint8 cursor_end;         // scan lines
      uint8 start_address_high; // Big-endian
      uint8 start_address_low;
      uint8 cursor_location_high; // Big-endian
      uint8 cursor_location_low;
      uint8 light_pen_high;
      uint8 light_pen_low;
    };
    uint8 index[NUM_CRTC_REGISTERS] = {};
  } m_crtc_registers;

  // 03D0/2/4: CRT (6845) index register
  uint8 m_crtc_index_register = 0;

  // 03D1/3/5: CRT data register
  void CRTDataRegisterRead(uint8* value);
  void CRTDataRegisterWrite(uint8 value);

  // Timing
  struct Timing
  {
    float horizontal_frequency;
    float vertical_frequency;

    uint32 horizontal_displayed_pixels;
    uint32 vertical_displayed_lines;

    // NOTE: We're not calculating the front porch/sync/back porch durations here, just the whole thing.
    SimulationTime horizontal_active_duration;
    SimulationTime horizontal_total_duration;
    SimulationTime vertical_active_duration;
    SimulationTime vertical_total_duration;

    bool operator==(const Timing& rhs) const;
  };
  Timing m_timing = {};

  struct ScanoutInfo
  {
    uint32 current_line;
    bool in_horizontal_blank;
    bool in_vertical_blank;
    bool display_active;
  };

  void RecalculateEventTiming();
  ScanoutInfo GetScanoutInfo();

  Clock m_clock;
  uint32 m_last_rendered_line = 0;
  uint32 m_address_register = 0;
  uint32 m_character_row_register = 0;
  TimingEvent::Pointer m_tick_event;
};
} // namespace HW