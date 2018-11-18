#include "pce/hw/cga.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Memory.h"
#include "common/display.h"
#include "pce/bus.h"
#include "pce/host_interface.h"
#include "pce/mmio.h"
#include "pce/system.h"
#include <utility>

namespace HW {
#include "cga_font.inl"
#include "cga_palette.inl"

DEFINE_OBJECT_TYPE_INFO(CGA);
DEFINE_GENERIC_COMPONENT_FACTORY(CGA);
BEGIN_OBJECT_PROPERTY_MAP(CGA)
END_OBJECT_PROPERTY_MAP()

CGA::CGA(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_clock("CGA", 3579545)
{
}

CGA::~CGA() {}

bool CGA::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_display = system->GetHostInterface()->CreateDisplay(
    SmallString::FromFormat("%s (CGA)", m_identifier.GetCharArray()), Display::Type::Primary);
  if (!m_display)
    return false;

  m_display->SetDisplayAspectRatio(4, 3);

  m_clock.SetManager(system->GetTimingManager());
  ConnectIOPorts(bus);

  m_line_event = m_clock.NewEvent("Tick", 1, std::bind(&CGA::RenderLineEvent, this, std::placeholders::_2));
  return true;
}

void CGA::Reset()
{
  // 40x25 configuration
  m_crtc_registers = {0x38, 0x28, 0x2D, 0x0A, 0x1F, 0x06, 0x19, 0x1C, 0x02,
                      0x07, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  // m_crtc_registers = { 97, 80, 82, 15, 25, 6, 25, 25, 2, 13, 11, 12, 0, 0, 0, 0, 0, 0 };

  m_mode_control_register.high_resolution = false;
  m_mode_control_register.graphics_mode = false;
  m_mode_control_register.monochrome = false;
  m_mode_control_register.enable_video_output = false;
  m_mode_control_register.high_resolution_graphics = false;
  m_mode_control_register.enable_blink = false;
  RecalculateEventTiming();
  BeginFrame();

  // Start with a clear framebuffer.
  m_display->ClearFramebuffer();
}

bool CGA::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  bool valid = true;
  valid &= reader.SafeReadBytes(m_vram, sizeof(m_vram));
  valid &= reader.SafeReadByte(&m_mode_control_register.raw);
  valid &= reader.SafeReadByte(&m_color_control_register.raw);
  valid &= reader.SafeReadBytes(m_crtc_registers.index, sizeof(m_crtc_registers.index));
  valid &= reader.SafeReadByte(&m_crtc_index_register);
  valid &= reader.SafeReadUInt32(&m_address_counter);
  valid &= reader.SafeReadUInt32(&m_character_row_counter);
  valid &= reader.SafeReadUInt32(&m_current_row);
  valid &= reader.SafeReadUInt32(&m_remaining_adjust_lines);

  valid &= reader.SafeReadUInt32(&m_current_frame_offset);
  valid &= reader.SafeReadUInt32(&m_current_frame_width);
  valid &= reader.SafeReadUInt32(&m_current_frame_line);
  if (valid && m_current_frame_offset > 0)
  {
    m_current_frame.resize(m_current_frame_offset);
    valid &= reader.SafeReadBytes(m_current_frame.data(), Truncate32(m_current_frame_offset * sizeof(uint32)));
  }

  if (valid)
    RecalculateEventTiming();

  return valid;
}

bool CGA::SaveState(BinaryWriter& writer)
{
  bool valid = writer.SafeWriteUInt32(SERIALIZATION_ID);
  valid &= writer.SafeWriteBytes(m_vram, sizeof(m_vram));
  valid &= writer.SafeWriteByte(m_mode_control_register.raw);
  valid &= writer.SafeWriteByte(m_color_control_register.raw);
  valid &= writer.SafeWriteBytes(m_crtc_registers.index, sizeof(m_crtc_registers.index));
  valid &= writer.SafeWriteByte(m_crtc_index_register);
  valid &= writer.SafeWriteUInt32(m_address_counter);
  valid &= writer.SafeWriteUInt32(m_character_row_counter);
  valid &= writer.SafeWriteUInt32(m_current_row);
  valid &= writer.SafeWriteUInt32(m_remaining_adjust_lines);

  valid &= writer.SafeWriteUInt32(m_current_frame_offset);
  valid &= writer.SafeWriteUInt32(m_current_frame_width);
  valid &= writer.SafeWriteUInt32(m_current_frame_line);
  if (m_current_frame_offset > 0)
    valid &= writer.SafeWriteBytes(m_current_frame.data(), Truncate32(m_current_frame_offset * sizeof(uint32)));

  return valid;
}

uint32 CGA::GetBorderColor() const
{
  return CGA_PALETTE[m_color_control_register.background_color];
}

uint32 CGA::GetCursorAddress() const
{
  return (ZeroExtend32(m_crtc_registers.cursor_location_high) << 8) |
         ZeroExtend32(m_crtc_registers.cursor_location_low);
}

uint32 CGA::InCursorBox() const
{
  const uint8 cursor_mask = BoolToUInt8(m_character_row_counter >= ZeroExtend32(m_crtc_registers.cursor_start & 0x3F) &&
                                        m_character_row_counter <= ZeroExtend32(m_crtc_registers.cursor_end));
  return ((cursor_mask & m_cursor_state) != 0);
}

void CGA::ConnectIOPorts(Bus* bus)
{
  bus->ConnectIOPortReadToPointer(0x03D0, this, &m_crtc_index_register);
  bus->ConnectIOPortWriteToPointer(0x03D0, this, &m_crtc_index_register);
  bus->ConnectIOPortReadToPointer(0x03D2, this, &m_crtc_index_register);
  bus->ConnectIOPortWriteToPointer(0x03D2, this, &m_crtc_index_register);
  bus->ConnectIOPortRead(0x03D1, this, std::bind(&CGA::CRTDataRegisterRead, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x03D1, this, std::bind(&CGA::CRTDataRegisterWrite, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x03D3, this, std::bind(&CGA::CRTDataRegisterRead, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x03D3, this, std::bind(&CGA::CRTDataRegisterWrite, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x03D5, this, std::bind(&CGA::CRTDataRegisterRead, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x03D5, this, std::bind(&CGA::CRTDataRegisterWrite, this, std::placeholders::_2));
  bus->ConnectIOPortReadToPointer(0x03D4, this, &m_crtc_index_register);
  bus->ConnectIOPortWriteToPointer(0x03D4, this, &m_crtc_index_register);
  bus->ConnectIOPortReadToPointer(0x03D8, this, &m_mode_control_register.raw);
  bus->ConnectIOPortWrite(0x03D8, this, std::bind(&CGA::ModeControlRegisterWrite, this, std::placeholders::_2));
  bus->ConnectIOPortReadToPointer(0x03D9, this, &m_color_control_register.raw);
  bus->ConnectIOPortWrite(0x03D9, this, std::bind(&CGA::ColorControlRegisterWrite, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x03DA, this, std::bind(&CGA::StatusRegisterRead, this, std::placeholders::_2));

  // CGA is never removed from the system, so don't bother cleaning up the MMIO
  MMIO* mmio_B8000 = MMIO::CreateDirect(0x000B8000, sizeof(m_vram), m_vram);
  MMIO* mmio_BC000 = MMIO::CreateDirect(0x000BC000, sizeof(m_vram), m_vram);
  bus->ConnectMMIO(mmio_B8000);
  bus->ConnectMMIO(mmio_BC000);
  mmio_B8000->Release();
  mmio_BC000->Release();
}

void CGA::RenderLineEvent(CycleCount cycles)
{
  while (cycles > 0)
  {
    cycles--;

    if (m_current_row < m_timing.vertical_display_end)
    {
      if (m_mode_control_register.graphics_mode)
        RenderLineGraphics();
      else
        RenderLineText();
    }
    else if (m_current_row < m_timing.vertical_sync_start || m_current_row >= m_timing.vertical_sync_end)
    {
      RenderLineBorder();
    }

    if (m_character_row_counter == 0)
    {
      if (m_current_row == m_timing.vertical_sync_start)
      {
        FlushFrame();
      }
      else if (m_current_row == m_crtc_registers.vertical_total)
      {
        // Are we still in the adjust region?
        if (m_remaining_adjust_lines == 0)
        {
          // Beginning of frame.
          // This is actually the border/overscan area. We start rendering when we hit row=vtotal.
          BeginFrame();
          continue;
        }
        else
        {
          // Still in the adjust region.
          m_remaining_adjust_lines--;
          continue;
        }
      }
    }

    // Increment character row counter. The comparison occurs with the old value.
    if (m_character_row_counter == m_crtc_registers.maximum_scan_lines)
    {
      // Next row.
      m_character_row_counter = 0;
      m_current_row = (m_current_row + 1) & VERTICAL_COUNTER_MASK;

      // Address counter is only incremented in during active lines.
      // TODO: Is this correct?
      if (m_current_row < m_timing.vertical_display_end)
      {
        m_address_counter =
          (m_address_counter + ZeroExtend32(m_crtc_registers.horizontal_displayed)) & ADDRESS_COUNTER_MASK;
      }
    }
    else
    {
      // Next line within character.
      m_character_row_counter = (m_character_row_counter + 1) & CHARACTER_ROW_COUNTER_MASK;
    }
  }
}

void CGA::BeginFrame()
{
  // latch address register, reset character row counter
  m_address_counter =
    (ZeroExtend32(m_crtc_registers.start_address_high) << 8) | ZeroExtend32(m_crtc_registers.start_address_low);
  m_character_row_counter = 0;
  m_current_row = 0;
  m_remaining_adjust_lines = m_crtc_registers.vertical_total_adjust;

  // Update blink state.
  if ((--m_blink_frame_counter) == 0)
  {
    m_blink_frame_counter = BLINK_INTERVAL;
    m_blink_state ^= 1;
  }

  // Update cursor state.
  // TODO: Should this be at the start of a line, or the start of a frame?
  const uint8 cursor_val = (m_crtc_registers.cursor_start >> 5) & 3;
  switch (cursor_val)
  {
    case 1:
      m_cursor_state = 0;
      break;
    case 0:
    case 2:
    case 3:
    {
      if ((--m_cursor_frame_counter) == 0)
      {
        m_cursor_frame_counter = BLINK_INTERVAL << ((cursor_val == 3) ? 1 : 0);
        m_cursor_state ^= 1;
      }
    }
    break;
  }
}

void CGA::RenderLineText()
{
  const uint32 num_characters =
    std::min(m_current_frame_width / CHARACTER_WIDTH, ZeroExtend32(m_crtc_registers.horizontal_displayed));
  const uint32 line_width = (num_characters * CHARACTER_WIDTH) + m_timing.horizontal_left_border_pixels +
                            m_timing.horizontal_right_border_pixels;
  if (m_current_frame.size() < (m_current_frame_offset + line_width))
    m_current_frame.resize(m_current_frame_offset + line_width);

  const uint32 border_color = GetBorderColor();
  for (uint32 i = 0; i < m_timing.horizontal_left_border_pixels; i++)
    m_current_frame[m_current_frame_offset++] = border_color;

  uint32 cursor_address = GetCursorAddress();
  uint32 address_register = m_address_counter;
  uint32 character_start_y = m_character_row_counter & 0x7;
  for (uint32 j = 0; j < num_characters; j++)
  {
    uint32 vram_address = (address_register & ADDRESS_COUNTER_VRAM_MASK_TEXT) << CRTC_ADDRESS_SHIFT;
    uint8 character_code = m_vram[vram_address + 0];
    uint8 character_attributes = m_vram[vram_address + 1];

    uint8 source_bits = CGA_FONT[character_code][character_start_y];
    uint32 foreground_color = CGA_PALETTE[character_attributes & 0xF];
    uint32 background_color = CGA_PALETTE[(character_attributes >> 4) & 0x7];
    if (address_register == cursor_address && InCursorBox())
      source_bits = 0xFF;
    if ((character_attributes >> 7) & m_blink_state)
      foreground_color = background_color;

    for (uint32 k = 0; k < 8; k++)
    {
      // This goes MSB..LSB.
      m_current_frame[m_current_frame_offset++] = (source_bits >> 7) ? foreground_color : background_color;
      source_bits <<= 1;
    }

    address_register = (address_register + 1) & ADDRESS_COUNTER_MASK;
  }

  for (uint32 i = 0; i < m_timing.horizontal_right_border_pixels; i++)
    m_current_frame[m_current_frame_offset++] = border_color;

  m_current_frame_line++;
}

void CGA::RenderLineGraphics()
{
  const bool high_resolution = m_mode_control_register.high_resolution_graphics;
  const uint32 num_characters =
    std::min(m_current_frame_width / (high_resolution ? (CHARACTER_WIDTH * 2) : CHARACTER_WIDTH),
             ZeroExtend32(m_crtc_registers.horizontal_displayed));
  const uint32 line_width = (num_characters * (high_resolution ? (CHARACTER_WIDTH * 2) : CHARACTER_WIDTH)) +
                            m_timing.horizontal_left_border_pixels + m_timing.horizontal_right_border_pixels;
  if (m_current_frame.size() < (m_current_frame_offset + line_width))
    m_current_frame.resize(m_current_frame_offset + line_width);

  const uint32 border_color = GetBorderColor();
  for (uint32 i = 0; i < m_timing.horizontal_left_border_pixels; i++)
    m_current_frame[m_current_frame_offset++] = border_color;

  uint32 address_counter = m_address_counter;
  for (uint32 j = 0; j < num_characters; j++)
  {
    // TODO: Is this correct?
    uint32 vram_offset = ((address_counter++ & ADDRESS_COUNTER_VRAM_MASK_GRAPHICS) << CRTC_ADDRESS_SHIFT) |
                         ((m_character_row_counter & 1) << 13);
    uint16 pixels = (ZeroExtend16(m_vram[vram_offset + 0]) << 8) | ZeroExtend16(m_vram[vram_offset + 1]);

    if (high_resolution)
    {
      uint32 foreground_color = CGA_PALETTE[m_color_control_register.background_color];
      uint32 background_color = CGA_PALETTE[0];

      // 1 bit per pixel, 8 pixels per byte
      for (uint32 k = 0; k < 16; k++)
      {
        m_current_frame[m_current_frame_offset++] = (pixels >> 15) ? foreground_color : background_color;
        pixels <<= 1;
      }
    }
    else
    {
      const uint32* foreground_palette =
        (m_mode_control_register.monochrome ?
           CGA_GRAPHICS_PALETTE_2 :
           (m_color_control_register.palette_select ? CGA_GRAPHICS_PALETTE_1 : CGA_GRAPHICS_PALETTE_0));
      uint8 foreground_intensity = m_color_control_register.foreground_intensity ? 1 : 0;
      uint32 background_color = CGA_PALETTE[m_color_control_register.background_color];

      // 2 bits per pixel, 4 pixels per byte
      for (uint32 k = 0; k < 8; k++)
      {
        uint8 index = (pixels >> 14);
        m_current_frame[m_current_frame_offset++] =
          (index == 0) ? background_color : foreground_palette[(index << 1) | foreground_intensity];
        pixels <<= 2;
      }
    }
  }

  for (uint32 i = 0; i < m_timing.horizontal_right_border_pixels; i++)
    m_current_frame[m_current_frame_offset++] = border_color;

  m_current_frame_line++;
}

void CGA::RenderLineBorder()
{
  const uint32 border_color = GetBorderColor();
  if (m_current_frame.size() < (m_current_frame_offset + m_current_frame_width))
    m_current_frame.resize(m_current_frame_offset + m_current_frame_width);

  for (uint32 i = 0; i < m_current_frame_width; i++)
    m_current_frame[m_current_frame_offset++] = border_color;

  m_current_frame_line++;
}

void CGA::FlushFrame()
{
  const uint32 frame_width = m_current_frame_width;
  const uint32 frame_height = m_current_frame_line;
  if (frame_width > 0 && frame_height > 0)
  {
    if (m_display->GetFramebufferWidth() != frame_width || m_display->GetFramebufferHeight() != frame_height)
    {
      m_display->ResizeFramebuffer(frame_width, frame_height);
      m_display->ResizeDisplay();
    }

    m_display->CopyFrame(m_current_frame.data(), m_current_frame_width * sizeof(uint32));
    m_display->SwapFramebuffer();
  }

  // clear our buffered frame state, pull new width from the crtc
  const bool hires_graphics = m_mode_control_register.graphics_mode && m_mode_control_register.high_resolution_graphics;
  m_current_frame_width =
    (ZeroExtend32(m_crtc_registers.horizontal_displayed) * (hires_graphics ? (CHARACTER_WIDTH * 2) : CHARACTER_WIDTH)) +
    m_timing.horizontal_left_border_pixels + m_timing.horizontal_right_border_pixels;
  m_current_frame_line = 0;
  m_current_frame_offset = 0;
}

void CGA::ModeControlRegisterWrite(uint8 value)
{
  m_line_event->InvokeEarly();
  m_mode_control_register.raw = value;
  RecalculateEventTiming();
}

void CGA::ColorControlRegisterWrite(uint8 value)
{
  m_line_event->InvokeEarly();
  m_color_control_register.raw = value;
}

void CGA::StatusRegisterRead(uint8* value)
{
  m_line_event->InvokeEarly();

  StatusRegister sr = {};
  if (m_mode_control_register.enable_video_output)
  {
    const bool in_vertical_blank = m_current_row >= m_timing.vertical_sync_start;
    bool safe_vram_access = in_vertical_blank;
    if (!in_vertical_blank)
    {
      const SimulationTime line_time = m_line_event->GetTimeSinceLastExecution();
      const bool in_horizontal_blank =
        line_time < m_timing.horizontal_display_start_time || line_time >= m_timing.horizontal_display_end_time;
      safe_vram_access |= in_horizontal_blank;
    }

    sr.vblank = in_vertical_blank;
    sr.safe_vram_access = safe_vram_access;
  }
  else
  {
    sr.vblank = false;
    sr.safe_vram_access = true;
  }

  sr.light_pen_switch_status = false;
  sr.light_pen_trigger_set = false;
  *value = sr.raw;
}

void CGA::CRTDataRegisterRead(uint8* value)
{
  // Can only read C-F
  if (m_crtc_index_register >= 0xC && m_crtc_index_register <= 0xF)
    *value = m_crtc_registers.index[m_crtc_index_register];
  else
    *value = 0;
}

void CGA::CRTDataRegisterWrite(uint8 value)
{
  static constexpr uint8 masks[NUM_CRTC_REGISTERS] = {0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x7F, 0x7F, 0xF3,
                                                      0x1F, 0x7F, 0x1F, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF};
  if (m_crtc_index_register >= NUM_CRTC_REGISTERS)
    return;

  m_line_event->InvokeEarly();
  m_crtc_registers.index[m_crtc_index_register] = value & masks[m_crtc_index_register];
  RecalculateEventTiming();
}

void CGA::RecalculateEventTiming()
{
  if (!m_mode_control_register.enable_video_output)
  {
    if (m_line_event->IsActive())
    {
      m_line_event->Deactivate();
      m_timing = {};
    }
    return;
  }

  const double dot_clock = m_mode_control_register.high_resolution ? PIXEL_CLOCK : (PIXEL_CLOCK / 2.0);

  Timing timing;

  const uint32 horizontal_total_pixels = (ZeroExtend32(m_crtc_registers.horizontal_total) + 1) * CHARACTER_WIDTH;
  timing.horizontal_frequency = dot_clock / double(horizontal_total_pixels);

  // Calculate sync times.
  int32 horizontal_total = int32(ZeroExtend32(m_crtc_registers.horizontal_total) + 1);
  int32 horizontal_displayed = std::min(int32(ZeroExtend32(m_crtc_registers.horizontal_displayed)), horizontal_total);
  int32 horizontal_sync_start =
    std::min(int32(ZeroExtend32(m_crtc_registers.horizontal_sync_position)), horizontal_total);
  int32 horizontal_sync_end = std::min(
    horizontal_sync_start + int32(ZeroExtend32(m_crtc_registers.horizontal_sync_pulse_width)), horizontal_total);
  int32 horizontal_display_start = std::clamp(horizontal_total - horizontal_sync_end, int32(0), horizontal_total);
  int32 horizontal_display_end = std::min(horizontal_display_start + horizontal_displayed, horizontal_total);
  timing.horizontal_display_start_time = SimulationTime((1000000000.0 * horizontal_display_start) / dot_clock);
  timing.horizontal_display_end_time = SimulationTime((1000000000.0 * horizontal_display_end) / dot_clock);

#if 0
  // Calculate left/right border sizes.
  const uint32 border_character_width =
    (m_mode_control_register.graphics_mode && m_mode_control_register.high_resolution_graphics) ?
      (CHARACTER_WIDTH * 2) :
      CHARACTER_WIDTH;
  timing.horizontal_left_border_pixels = horizontal_display_start * border_character_width;
  timing.horizontal_right_border_pixels = (horizontal_total - horizontal_sync_end) * border_character_width;
#else
  timing.horizontal_left_border_pixels = horizontal_display_start;
  timing.horizontal_right_border_pixels = (horizontal_total - horizontal_sync_end);
#endif

  // Vertical timing.
  const uint32 character_height = ZeroExtend32(m_crtc_registers.maximum_scan_lines + 1);
  uint32 vertical_total_rows = ZeroExtend32(m_crtc_registers.vertical_total) + 1;
  timing.vertical_display_end = ZeroExtend32(m_crtc_registers.vertical_displayed);
  timing.vertical_sync_start = std::min(ZeroExtend32(m_crtc_registers.vertical_sync_position), vertical_total_rows);
  timing.vertical_sync_end =
    std::min(timing.vertical_sync_start + (VSYNC_PULSE_WIDTH / character_height), vertical_total_rows);

  if (m_timing == timing)
    return;

  m_timing = timing;

  // Render a whole frame if possible, by delaying the event vtotal lines, to reduce CPU load.
  uint32 vertical_total_scanlines = vertical_total_rows * character_height;
  m_line_event->SetFrequency(float(m_timing.horizontal_frequency), vertical_total_scanlines);
  if (!m_line_event->IsActive())
    m_line_event->Activate();
}

bool CGA::Timing::operator==(const Timing& rhs) const
{
  return std::tie(horizontal_frequency, horizontal_left_border_pixels, horizontal_right_border_pixels,
                  horizontal_display_start_time, horizontal_display_end_time, vertical_display_end, vertical_sync_start,
                  vertical_sync_end) == std::tie(rhs.horizontal_frequency, rhs.horizontal_left_border_pixels,
                                                 rhs.horizontal_right_border_pixels, rhs.horizontal_display_start_time,
                                                 rhs.horizontal_display_end_time, rhs.vertical_display_end,
                                                 rhs.vertical_sync_start, rhs.vertical_sync_end);
}
} // namespace HW