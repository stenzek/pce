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

public:
  CGA();
  ~CGA();

  const uint8* GetVRAM() const { return m_vram; }
  uint8* GetVRAM() { return m_vram; }

  uint8 GetModeControlRegister() const { return m_mode_control_register.raw; }
  void SetModeControlRegister(uint8 value);

  virtual void Initialize(System* system, Bus* bus) override;
  virtual void Reset() override;
  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

  // Helpers for HLE to set a character in text mode
  void SetTextModeCharacter(uint32 page, uint32 x, uint32 y, uint8 character_code, uint8 attributes);
  void ScrollLine(uint32 page);

  // This should change the CRTC register
  void SetDisplayPage(uint32 page) { m_display_page = page; }

  void Render();

private:
  void ConnectIOPorts(Bus* bus);
  void Tick(CycleCount cycles);
  void RenderTextMode();
  void RenderTextModeCharacter(uint32 page, uint32 x, uint32 y);
  void RenderGraphicsMode();
  void RenderGraphicsModeScanline(uint32 scanline);
  void ResizeFramebuffer();

  System* m_system = nullptr;
  Display* m_display;
  uint8 m_vram[16384];

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

  // 03D9h: Colour control register
  union
  {
    uint8 raw = 0;
    BitField<uint8, bool, 5, 1> palette_select;
    BitField<uint8, bool, 4, 1> foreground_intensity;
    BitField<uint8, uint8, 0, 4> background_color;
  } m_color_control_register;

  // 03DAh: Status register
  union
  {
    uint8 raw = 0;
    BitField<uint8, bool, 0, 1> safe_vram_access;
    BitField<uint8, bool, 1, 1> light_pen_trigger_set;
    BitField<uint8, bool, 2, 1> light_pen_switch_status;
    BitField<uint8, bool, 3, 1> vblank;
  } m_status_register;

  // CRTC registers
  union
  {
    struct
    {
      uint8 horizontal_total;
      uint8 horizontal_displayed;
      uint8 horizontal_sync_position;
      uint8 horizontal_sync_pulse_width;
      uint8 vertical_total;
      uint8 vertical_displayed;
      uint8 vertical_sync_position;
      uint8 vertical_sync_pulse_width;
      uint8 interlace_mode;
      uint8 maximum_scan_lines;
      uint8 cursor_start;
      uint8 cursor_end;
      uint8 start_address_high; // Big-endian
      uint8 start_address_low;
      uint8 cursor_location_high; // Big-endian
      uint8 cursor_location_low;
      uint8 light_pen_high;
      uint8 light_pen_low;
    };
    uint8 index[18] = {};
  } m_crtc_registers;

  // 03D0/2/4: CRT (6845) index register
  uint8 m_crtc_index_register = 0;

  // 03D1/3/5: CRT data register
  void CRTDataRegisterRead(uint8* value);
  void CRTDataRegisterWrite(uint8 value);

  Clock m_clock;
  SimulationTime m_active_line_duration = 1;
  SimulationTime m_hblank_duration = 1;
  SimulationTime m_vblank_duration = 1;
  SimulationTime m_vsync_interval = 1;
  SimulationTime m_downcount = 0;
  SimulationTime m_time_to_vsync = 0;
  uint32 m_current_line = 0;
  uint32 m_state = 0;

  uint32 m_display_page = 0;

  TimingEvent::Pointer m_tick_event;
};
} // namespace HW