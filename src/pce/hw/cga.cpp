#include "pce/hw/cga.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "pce/bus.h"
#include "pce/display.h"
#include "pce/host_interface.h"
#include "pce/mmio.h"
#include "pce/system.h"
#include <utility>
Log_SetChannel(CGA);

namespace HW {
#include "cga_font.inl"
#include "cga_palette.inl"

CGA::CGA() : m_clock("CGA", 3579545) {}

CGA::~CGA() {}

void CGA::SetModeControlRegister(uint8 value)
{
  m_mode_control_register.raw = value;
}

void CGA::Initialize(System* system, Bus* bus)
{
  m_system = system;
  m_display = system->GetHostInterface()->GetDisplay();
  m_clock.SetManager(system->GetTimingManager());
  ConnectIOPorts(bus);

  m_tick_event = m_clock.NewEvent("Tick", 1, std::bind(&CGA::Tick, this, std::placeholders::_2));
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

  m_last_rendered_line = 0;
  m_address_register = 0;

  // Start with a clear framebuffer.
  m_display->ClearFramebuffer();
  m_display->DisplayFramebuffer();
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
  valid &= reader.SafeReadFloat(&m_timing.horizontal_frequency);
  valid &= reader.SafeReadFloat(&m_timing.vertical_frequency);
  valid &= reader.SafeReadUInt32(&m_timing.horizontal_displayed_pixels);
  valid &= reader.SafeReadUInt32(&m_timing.vertical_displayed_lines);
  valid &= reader.SafeReadInt64(&m_timing.horizontal_active_duration);
  valid &= reader.SafeReadInt64(&m_timing.horizontal_total_duration);
  valid &= reader.SafeReadInt64(&m_timing.vertical_active_duration);
  valid &= reader.SafeReadInt64(&m_timing.vertical_total_duration);
  valid &= reader.SafeReadUInt32(&m_last_rendered_line);
  valid &= reader.SafeReadUInt32(&m_address_register);
  return valid;
}

bool CGA::SaveState(BinaryWriter& writer)
{
  bool valid = writer.SafeWriteUInt32(SERIALIZATION_ID);
  valid = writer.SafeWriteBytes(m_vram, sizeof(m_vram));
  valid = writer.SafeWriteByte(m_mode_control_register.raw);
  valid = writer.SafeWriteByte(m_color_control_register.raw);
  valid = writer.SafeWriteBytes(m_crtc_registers.index, sizeof(m_crtc_registers.index));
  valid = writer.SafeWriteByte(m_crtc_index_register);
  valid = writer.SafeWriteFloat(m_timing.horizontal_frequency);
  valid = writer.SafeWriteFloat(m_timing.vertical_frequency);
  valid = writer.SafeWriteUInt32(m_timing.horizontal_displayed_pixels);
  valid = writer.SafeWriteUInt32(m_timing.vertical_displayed_lines);
  valid = writer.SafeWriteInt64(m_timing.horizontal_active_duration);
  valid = writer.SafeWriteInt64(m_timing.horizontal_total_duration);
  valid = writer.SafeWriteInt64(m_timing.vertical_active_duration);
  valid = writer.SafeWriteInt64(m_timing.vertical_total_duration);
  valid = writer.SafeWriteUInt32(m_last_rendered_line);
  valid = writer.SafeWriteUInt32(m_address_register);
  return valid;
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
  bus->RegisterMMIO(mmio_B8000);
  bus->RegisterMMIO(mmio_BC000);
  mmio_B8000->Release();
  mmio_BC000->Release();
}

void CGA::Tick(CycleCount cycles)
{
  RenderFramebuffer(true);

  // vsync/end of vblank
  m_display->DisplayFramebuffer();
  m_last_rendered_line = 0;

  // re-latch address register
  m_address_register =
    (ZeroExtend32(m_crtc_registers.start_address_high) << 8) | ZeroExtend32(m_crtc_registers.start_address_low);
}

void CGA::RenderFramebuffer(bool end_of_vblank /* = false */)
{
  // TODO: This is currently at a per-scanline granularity.
  // Mid-line palette changes won't have any effect.
  uint32 max_line_to_render;
  if (end_of_vblank)
  {
    DebugAssert(m_timing.vertical_displayed_lines > 0);
    max_line_to_render = m_timing.vertical_displayed_lines - 1;
  }
  else
  {
    ScanoutInfo info = GetScanoutInfo();
    if (info.current_line == 0 && !info.in_horizontal_blank)
    {
      // Can't even render one line yet.
      return;
    }

    if (info.in_vertical_blank)
      max_line_to_render = m_timing.vertical_displayed_lines - 1;
    else if (info.in_horizontal_blank)
      max_line_to_render = info.current_line;
    else
      max_line_to_render = info.current_line - 1;
  }

  if (m_last_rendered_line < max_line_to_render)
  {
    uint32 lines_to_render = max_line_to_render - m_last_rendered_line;
    if (m_mode_control_register.enable_video_output)
    {
      if (!m_mode_control_register.graphics_mode)
        RenderFramebufferLinesText(lines_to_render);
      else
        RenderFramebufferLinesGraphics(lines_to_render);
    }
    else
    {
      for (uint32 i = 0; i < lines_to_render; i++)
      {
        for (uint32 x = 0; x < m_timing.horizontal_displayed_pixels; x++)
          m_display->SetPixel(x, m_last_rendered_line, 0);
        m_last_rendered_line++;
      }
    }
  }
}

void CGA::RenderFramebufferLinesText(uint32 count)
{
  uint32 num_characters = m_crtc_registers.horizontal_displayed;

  for (uint32 i = 0; i < count; i++)
  {
    uint32 x = 0;
    uint32 address_register = m_address_register;
    uint32 character_start_y = m_last_rendered_line % CHARACTER_HEIGHT;

    for (uint32 j = 0; j < num_characters; j++)
    {
      uint8 character_code = m_vram[address_register++];
      uint8 character_attributes = m_vram[address_register++];

      // TODO: Blinking text
      uint8 source_bits = CGA_FONT[character_code][character_start_y];
      uint32 foreground_color = CGA_PALETTE[character_attributes & 0xF];
      uint32 background_color = CGA_PALETTE[character_attributes >> 4];
      uint32 colors[2] = {background_color, foreground_color};

      for (uint32 k = 0; k < 8; k++)
      {
        // This goes MSB..LSB.
        m_display->SetPixel(x++, m_last_rendered_line, colors[source_bits >> 7]);
        source_bits <<= 1;
      }
    }

    m_last_rendered_line++;
    if ((m_last_rendered_line % CHARACTER_HEIGHT) == 0)
      m_address_register += num_characters * 2;
  }
}

void CGA::RenderFramebufferLinesGraphics(uint32 count)
{
  for (uint32 i = 0; i < count; i++)
  {
    m_last_rendered_line++;
  }
}

// void CGA::RenderTextModeCharacter(uint32 page, uint32 x, uint32 y)
// {
//   uint32 character_count_per_line = (m_mode_control_register.high_resolution) ? 80 : 40;
//   uint32 character_data_pitch = character_count_per_line * 2;
//   uint32 page_size = character_data_pitch * 25;
//   uint32 page_base = m_display_page * page_size;
//
//   const uint8* line_base = &m_vram[page_base + y * character_data_pitch];
//
//   uint8 character_code = line_base[x * 2 + 0];
//   uint8 character_attributes = line_base[x * 2 + 1];
//
//   // TODO: Blinking text
//   uint32 foreground_color = CGA_PALETTE[character_attributes & 0xF];
//   uint32 background_color = CGA_PALETTE[character_attributes >> 4];
//
//   const uint8* source_bits = CGA_FONT[character_code];
//   uint32 character_start_x = x * CHARACTER_WIDTH;
//   uint32 character_start_y = y * CHARACTER_HEIGHT;
//   uint32 colors[2] = {background_color, foreground_color};
//   for (uint32 row = 0; row < CHARACTER_HEIGHT; row++)
//   {
//     uint8 source_row = source_bits[row];
//     uint32 dest_row = character_start_y + row;
//     m_display->SetPixel(character_start_x + 0, dest_row, colors[(source_row >> 7) & 1]);
//     m_display->SetPixel(character_start_x + 1, dest_row, colors[(source_row >> 6) & 1]);
//     m_display->SetPixel(character_start_x + 2, dest_row, colors[(source_row >> 5) & 1]);
//     m_display->SetPixel(character_start_x + 3, dest_row, colors[(source_row >> 4) & 1]);
//     m_display->SetPixel(character_start_x + 4, dest_row, colors[(source_row >> 3) & 1]);
//     m_display->SetPixel(character_start_x + 5, dest_row, colors[(source_row >> 2) & 1]);
//     m_display->SetPixel(character_start_x + 6, dest_row, colors[(source_row >> 1) & 1]);
//     m_display->SetPixel(character_start_x + 7, dest_row, colors[(source_row >> 0) & 1]);
//   }
// }
//
// void CGA::RenderGraphicsMode()
// {
//   // Always 200 scanlines
//   for (uint32 i = 0; i < 200; i++)
//     RenderGraphicsModeScanline(i);
//
//   m_display->DisplayFramebuffer();
// }
//
// void CGA::RenderGraphicsModeScanline(uint32 scanline)
// {
//   // Rows 0,2,4,...,198, 1,3,5,...,199
//   const uint32 pitch = 80;
//   uint32 start_address = ((scanline & 1) * 8192) + ((scanline >> 1) * pitch);
//
//   if (m_mode_control_register.high_resolution_graphics)
//   {
//     const uint32 pixels_wide = 640;
//     uint32 foreground_color = CGA_PALETTE[m_color_control_register.background_color];
//     uint32 background_color = CGA_PALETTE[0];
//
//     for (uint32 i = 0; i < pixels_wide; i++)
//     {
//       // 1 bit per pixel, 8 pixels per byte
//       uint32 address = i / 8;
//       uint32 shift = 7 - (i % 8);
//       uint8 value = (m_vram[start_address + address] >> shift) & 1;
//       uint32 color = value ? foreground_color : background_color;
//       m_display->SetPixel(i, scanline, color);
//     }
//   }
//   else
//   {
//     const uint32 pixels_wide = 320;
//     const uint32* foreground_palette =
//       (m_mode_control_register.monochrome ?
//          CGA_GRAPHICS_PALETTE_2 :
//          (m_color_control_register.palette_select ? CGA_GRAPHICS_PALETTE_1 : CGA_GRAPHICS_PALETTE_0));
//     uint8 foreground_intensity = m_color_control_register.foreground_intensity ? 1 : 0;
//     uint32 background_color = CGA_PALETTE[m_color_control_register.background_color];
//
//     for (uint32 i = 0; i < pixels_wide; i++)
//     {
//       // 2 bits per pixel, 4 pixels per byte
//       uint32 address = i / 4;
//       uint32 shift = 6 - ((i % 4) * 2);
//       uint8 value = (m_vram[start_address + address] >> shift) & 3;
//
//       uint32 color;
//       if (value == 0)
//       {
//         // background color
//         color = background_color;
//       }
//       else
//       {
//         // foreground color
//         color = foreground_palette[value << 1 | foreground_intensity];
//       }
//
//       m_display->SetPixel(i, scanline, color);
//     }
//   }
// }

void CGA::ModeControlRegisterWrite(uint8 value)
{
  RenderFramebuffer();
  m_mode_control_register.raw = value;
  RecalculateEventTiming();
}

void CGA::ColorControlRegisterWrite(uint8 value)
{
  RenderFramebuffer();
  m_color_control_register.raw = value;
}

void CGA::StatusRegisterRead(uint8* value)
{
  StatusRegister sr = {};
  if (m_mode_control_register.enable_video_output)
  {
    ScanoutInfo si = GetScanoutInfo();
    sr.vblank = si.in_vertical_blank;
    sr.safe_vram_access = !si.in_horizontal_blank && !si.in_vertical_blank;
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
  // Always a byte value
  uint32 index = m_crtc_index_register % NUM_CRTC_REGISTERS;

  // Can only read C-F
  if (index >= 0xC && index <= 0xF)
    *value = m_crtc_registers.index[index];
  else
    *value = 0;
}

void CGA::CRTDataRegisterWrite(uint8 value)
{
  RenderFramebuffer();

  static constexpr uint8 masks[NUM_CRTC_REGISTERS] = {0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x7F, 0x7F, 0xF3,
                                                      0x1F, 0x7F, 0x1F, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF};
  uint32 index = m_crtc_index_register % NUM_CRTC_REGISTERS;
  m_crtc_registers.index[index] = value & masks[index];
  RecalculateEventTiming();
}

void CGA::RecalculateEventTiming()
{
  const bool graphics_mode = m_mode_control_register.graphics_mode;
  const bool high_resolution = graphics_mode ? m_mode_control_register.high_resolution_graphics.GetValue() :
                                               m_mode_control_register.high_resolution.GetValue();
  const double dot_clock = high_resolution ? 14318000.0 : (14318000.0 / 2.0);
  uint32 character_height = graphics_mode ? 2 : 8;

  uint32 horizontal_total_pixels = (ZeroExtend32(m_crtc_registers.horizontal_total) + 1) * CHARACTER_WIDTH;
  double horizontal_frequency = dot_clock / double(horizontal_total_pixels);
  uint32 vertical_total_lines = ((ZeroExtend32(m_crtc_registers.vertical_total) + 1) * character_height) +
                                ZeroExtend32(m_crtc_registers.vertical_total_adjust);
  double vertical_frequency = horizontal_frequency / double(vertical_total_lines);

  // This register should be programmed with the number of character clocks in the active display - 1.
  uint32 horizontal_displayed_pixels = ZeroExtend32(m_crtc_registers.horizontal_displayed) * CHARACTER_WIDTH;
  double horizontal_active_duration = (1000000000.0 * horizontal_displayed_pixels) / dot_clock;
  double horizontal_total_duration = double(1000000000.0) / horizontal_frequency;

  // The field contains the value of the vertical scanline counter at the beggining of the scanline immediately after
  // the last scanline of active display.
  uint32 vertical_displayed_lines = ZeroExtend32(m_crtc_registers.vertical_displayed) * character_height;
  double vertical_active_duration = horizontal_total_duration * double(vertical_displayed_lines);
  double vertical_total_duration = double(1000000000.0) / vertical_frequency;

  // The active duration can be programmed so that there is no blanking period?
  if (horizontal_active_duration > horizontal_total_duration)
    horizontal_active_duration = horizontal_total_duration;

  Timing timing;
  timing.horizontal_frequency = std::max(float(horizontal_frequency), 1.0f);
  timing.vertical_frequency = std::max(float(vertical_frequency), 1.0f);
  timing.horizontal_displayed_pixels = horizontal_displayed_pixels;
  timing.vertical_displayed_lines = vertical_displayed_lines;
  timing.horizontal_active_duration = SimulationTime(horizontal_active_duration);
  timing.horizontal_total_duration = std::max(SimulationTime(horizontal_total_duration), SimulationTime(1));
  timing.vertical_active_duration = SimulationTime(vertical_active_duration);
  timing.vertical_total_duration = std::max(SimulationTime(vertical_total_duration), SimulationTime(1));
  if (m_timing == timing)
    return;

  // Vertical frequency must be between 35-75hz?
  if (vertical_frequency < 35.0 || vertical_frequency > 75.0)
  {
    Log_WarningPrintf("Horizontal frequency: %.4f kHz, vertical frequency: %.4f hz out of range.",
                      horizontal_frequency / 1000.0, vertical_frequency);

    m_timing = {};
    m_display->ClearFramebuffer();
    m_display->DisplayFramebuffer();
    if (m_tick_event->IsActive())
      m_tick_event->Deactivate();
    return;
  }

  Log_InfoPrintf("Horizontal frequency: %.4f kHz, vertical frequency: %.4f hz, displayed %ux%u",
                 horizontal_frequency / 1000.0, vertical_frequency, horizontal_displayed_pixels,
                 vertical_displayed_lines);
  m_timing = timing;

  Assert(timing.horizontal_displayed_pixels > 0 && timing.vertical_displayed_lines > 0);
  if (m_display->GetFramebufferWidth() != m_timing.horizontal_displayed_pixels ||
      m_display->GetFramebufferHeight() != m_timing.vertical_displayed_lines)
  {
    m_display->ResizeFramebuffer(m_timing.horizontal_displayed_pixels, m_timing.vertical_displayed_lines);
    m_display->ResizeDisplay();
  }

  m_tick_event->SetFrequency(m_timing.vertical_frequency);
  if (!m_tick_event->IsActive())
    m_tick_event->Activate();
}

bool CGA::Timing::operator==(const Timing& rhs) const
{
  return std::tie(horizontal_frequency, vertical_frequency, horizontal_displayed_pixels, vertical_displayed_lines) ==
         std::tie(rhs.horizontal_frequency, rhs.vertical_frequency, rhs.horizontal_displayed_pixels,
                  rhs.vertical_displayed_lines);
}

CGA::ScanoutInfo CGA::GetScanoutInfo()
{
  ScanoutInfo si;
  if (!m_tick_event->IsActive())
  {
    // Display off, so let's just say we're in vblank
    si.current_line = 0;
    si.in_horizontal_blank = false;
    si.in_vertical_blank = false;
    si.display_active = false;
    return si;
  }

  SimulationTime time_since_retrace = m_tick_event->GetTimeSinceLastExecution();
  si.current_line = uint32(time_since_retrace / m_timing.horizontal_total_duration);

  // Check if we're in vertical retrace.
  si.in_vertical_blank = (time_since_retrace > m_timing.vertical_active_duration);
  if (si.in_vertical_blank)
  {
    si.in_horizontal_blank = false;
  }
  else
  {
    // Check if we're in horizontal retrace.
    SimulationTime scanline_time = (time_since_retrace % m_timing.horizontal_total_duration);
    si.in_horizontal_blank = (scanline_time >= m_timing.horizontal_active_duration);
    si.display_active = !si.in_horizontal_blank;
  }

  si.display_active = !si.in_horizontal_blank && !si.in_vertical_blank;
  return si;
}
} // namespace HW