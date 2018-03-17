#include "pce/hw/cga.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Memory.h"
#include "pce/bus.h"
#include "pce/display.h"
#include "pce/host_interface.h"
#include "pce/mmio.h"
#include "pce/system.h"

namespace HW {

static constexpr uint32 CHARACTER_WIDTH = 8;
static constexpr uint32 CHARACTER_HEIGHT = 8;
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
  m_mode_control_register.high_resolution = false;
  m_mode_control_register.graphics_mode = false;
  m_mode_control_register.monochrome = false;
  m_mode_control_register.enable_video_output = true;
  m_mode_control_register.high_resolution_graphics = false;
  m_mode_control_register.enable_blink = false;
  m_status_register.safe_vram_access = false;
  m_status_register.vblank = false;
  m_state = 0;
  m_current_line = 0;
  m_downcount = m_active_line_duration;
  m_time_to_vsync = m_vsync_interval;

  // Set mode register to output 40x25 text
  m_mode_control_register.raw = 0;
  m_mode_control_register.enable_video_output = true;
  m_mode_control_register.high_resolution = false;

  // Not accurate at all..
  // see https://www.vogons.org/viewtopic.php?f=9&t=47052
  m_active_line_duration = 160;
  m_hblank_duration = 228 - 160;
  m_vblank_duration = 228 * 16;
  m_vsync_interval = 59736; // 262 scanlines
  m_tick_event->Reset();

  Render();
}

bool CGA::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.SafeReadBytes(m_vram, sizeof(m_vram));
  reader.SafeReadByte(&m_mode_control_register.raw);
  reader.SafeReadByte(&m_color_control_register.raw);
  reader.SafeReadByte(&m_status_register.raw);
  reader.SafeReadBytes(m_crtc_registers.index, sizeof(m_crtc_registers.index));
  reader.SafeReadByte(&m_crtc_index_register);
  reader.SafeReadInt64(&m_downcount);
  reader.SafeReadInt64(&m_time_to_vsync);
  reader.SafeReadUInt32(&m_current_line);
  reader.SafeReadUInt32(&m_state);
  reader.SafeReadUInt32(&m_display_page);
  return true;
}

bool CGA::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteBytes(m_vram, sizeof(m_vram));
  writer.WriteByte(m_mode_control_register.raw);
  writer.WriteByte(m_color_control_register.raw);
  writer.WriteByte(m_status_register.raw);
  writer.WriteBytes(m_crtc_registers.index, sizeof(m_crtc_registers.index));
  writer.WriteByte(m_crtc_index_register);
  writer.WriteInt64(m_downcount);
  writer.WriteInt64(m_time_to_vsync);
  writer.WriteUInt32(m_current_line);
  writer.WriteUInt32(m_state);
  writer.WriteUInt32(m_display_page);
  return true;
}

void CGA::SetTextModeCharacter(uint32 page, uint32 x, uint32 y, uint8 character_code, uint8 attributes)
{
  uint32 pitch = (m_mode_control_register.high_resolution) ? 160 : 80;
  uint32 page_size = pitch * 25;

  m_vram[page * page_size + y * pitch + (x * 2) + 0] = character_code;
  m_vram[page * page_size + y * pitch + (x * 2) + 1] = attributes;

  // TODO: Check for active page
  if (!m_mode_control_register.graphics_mode)
  {
    RenderTextModeCharacter(page, x, y);
    m_display->DisplayFramebuffer();
  }
}

void CGA::ScrollLine(uint32 page)
{
  uint32 pitch = (m_mode_control_register.high_resolution) ? 160 : 80;
  uint32 page_size = pitch * 25;

  Y_memmove(&m_vram[page * page_size], &m_vram[page * page_size + pitch], pitch * 24);
  Y_memzero(&m_vram[page * page_size + 24 * pitch], pitch);
  if (!m_mode_control_register.graphics_mode)
  {
    RenderTextMode();
    m_display->DisplayFramebuffer();
  }
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
  bus->ConnectIOPortWriteToPointer(0x03D8, this, &m_mode_control_register.raw);
  bus->ConnectIOPortReadToPointer(0x03D9, this, &m_color_control_register.raw);
  bus->ConnectIOPortWriteToPointer(0x03D9, this, &m_color_control_register.raw);
  bus->ConnectIOPortReadToPointer(0x03DA, this, &m_status_register.raw);

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
  m_downcount -= cycles;
  while (m_downcount <= 0)
  {
    if (m_state == 0)
    {
      // active -> hblank
      m_status_register.safe_vram_access = true;
      m_downcount += m_hblank_duration;
      m_state = 1;
    }
    else if (m_state == 1)
    {
      // hblank -> active
      m_current_line++;
      if (m_current_line == 480)
      {
        // hblank -> vblank
        m_status_register.safe_vram_access = true;
        m_status_register.vblank = true;
        m_state = 2;
        m_downcount += m_vblank_duration;
      }
      else
      {
        m_status_register.safe_vram_access = false;
        m_downcount += m_active_line_duration;
        m_state = 0;
      }
    }
    else if (m_state == 2)
    {
      // vblank -> line 0 active
      m_status_register.safe_vram_access = false;
      m_status_register.vblank = false;
      m_downcount += m_active_line_duration;
      m_current_line = 0;
      m_state = 0;
    }
  }

  m_time_to_vsync -= cycles;
  if (m_time_to_vsync <= 0)
    Render();
  while (m_time_to_vsync <= 0)
    m_time_to_vsync += m_vsync_interval;
}

void CGA::Render()
{
  ResizeFramebuffer();

  if (!m_mode_control_register.enable_video_output)
    return;

  if (!m_mode_control_register.graphics_mode)
    RenderTextMode();
  else
    RenderGraphicsMode();
}

void CGA::ResizeFramebuffer()
{
  if (!m_mode_control_register.graphics_mode)
  {
    // Text mode
    uint32 character_count_per_line = (m_mode_control_register.high_resolution) ? 80 : 40;
    uint32 framebuffer_width = character_count_per_line * CHARACTER_WIDTH;
    uint32 framebuffer_height = 25 * CHARACTER_HEIGHT;
    m_display->ResizeFramebuffer(framebuffer_width, framebuffer_height);
  }
  else
  {
    uint32 pixels_wide = (m_mode_control_register.high_resolution_graphics) ? 640 : 320;
    uint32 pixels_high = 200;
    m_display->ResizeFramebuffer(pixels_wide, pixels_high);
  }
}

void CGA::CRTDataRegisterRead(uint8* value)
{
  // Always a byte value
  uint32 index = m_crtc_index_register % countof(m_crtc_registers.index);

  // Can only read C-F
  if (index >= 0xC && index <= 0xF)
    *value = m_crtc_registers.index[index];
  else
    *value = 0;
}

void CGA::CRTDataRegisterWrite(uint8 value)
{
  uint32 index = m_crtc_index_register % countof(m_crtc_registers.index);
  m_crtc_registers.index[index] = value;
}

void CGA::RenderTextMode()
{
  // Update size if it has changed
  uint32 character_count_per_line = (m_mode_control_register.high_resolution) ? 80 : 40;

  // Render each character with attributes
  // This would be nice to accelerate on the GPU
  // TODO: Page select?
  for (uint32 y = 0; y < 25; y++)
  {
    for (uint32 x = 0; x < character_count_per_line; x++)
      RenderTextModeCharacter(m_display_page, x, y);
  }

  m_display->DisplayFramebuffer();
}

void CGA::RenderTextModeCharacter(uint32 page, uint32 x, uint32 y)
{
  uint32 character_count_per_line = (m_mode_control_register.high_resolution) ? 80 : 40;
  uint32 character_data_pitch = character_count_per_line * 2;
  uint32 page_size = character_data_pitch * 25;
  uint32 page_base = m_display_page * page_size;

  const uint8* line_base = &m_vram[page_base + y * character_data_pitch];

  uint8 character_code = line_base[x * 2 + 0];
  uint8 character_attributes = line_base[x * 2 + 1];

  // TODO: Blinking text
  uint32 foreground_color = CGA_PALETTE[character_attributes & 0xF];
  uint32 background_color = CGA_PALETTE[character_attributes >> 4];

  const uint8* source_bits = CGA_FONT[character_code];
  uint32 character_start_x = x * CHARACTER_WIDTH;
  uint32 character_start_y = y * CHARACTER_HEIGHT;
  uint32 colors[2] = {background_color, foreground_color};
  for (uint32 row = 0; row < CHARACTER_HEIGHT; row++)
  {
    uint8 source_row = source_bits[row];
    uint32 dest_row = character_start_y + row;
    m_display->SetPixel(character_start_x + 0, dest_row, colors[(source_row >> 7) & 1]);
    m_display->SetPixel(character_start_x + 1, dest_row, colors[(source_row >> 6) & 1]);
    m_display->SetPixel(character_start_x + 2, dest_row, colors[(source_row >> 5) & 1]);
    m_display->SetPixel(character_start_x + 3, dest_row, colors[(source_row >> 4) & 1]);
    m_display->SetPixel(character_start_x + 4, dest_row, colors[(source_row >> 3) & 1]);
    m_display->SetPixel(character_start_x + 5, dest_row, colors[(source_row >> 2) & 1]);
    m_display->SetPixel(character_start_x + 6, dest_row, colors[(source_row >> 1) & 1]);
    m_display->SetPixel(character_start_x + 7, dest_row, colors[(source_row >> 0) & 1]);
  }
}

void CGA::RenderGraphicsMode()
{
  // Always 200 scanlines
  for (uint32 i = 0; i < 200; i++)
    RenderGraphicsModeScanline(i);

  m_display->DisplayFramebuffer();
}

void CGA::RenderGraphicsModeScanline(uint32 scanline)
{
  // Rows 0,2,4,...,198, 1,3,5,...,199
  const uint32 pitch = 80;
  uint32 start_address = ((scanline & 1) * 8192) + ((scanline >> 1) * pitch);

  if (m_mode_control_register.high_resolution_graphics)
  {
    const uint32 pixels_wide = 640;
    uint32 foreground_color = CGA_PALETTE[m_color_control_register.background_color];
    uint32 background_color = CGA_PALETTE[0];

    for (uint32 i = 0; i < pixels_wide; i++)
    {
      // 1 bit per pixel, 8 pixels per byte
      uint32 address = i / 8;
      uint32 shift = 7 - (i % 8);
      uint8 value = (m_vram[start_address + address] >> shift) & 1;
      uint32 color = value ? foreground_color : background_color;
      m_display->SetPixel(i, scanline, color);
    }
  }
  else
  {
    const uint32 pixels_wide = 320;
    const uint32* foreground_palette =
      (m_mode_control_register.monochrome ?
         CGA_GRAPHICS_PALETTE_2 :
         (m_color_control_register.palette_select ? CGA_GRAPHICS_PALETTE_1 : CGA_GRAPHICS_PALETTE_0));
    uint8 foreground_intensity = m_color_control_register.foreground_intensity ? 1 : 0;
    uint32 background_color = CGA_PALETTE[m_color_control_register.background_color];

    for (uint32 i = 0; i < pixels_wide; i++)
    {
      // 2 bits per pixel, 4 pixels per byte
      uint32 address = i / 4;
      uint32 shift = 6 - ((i % 4) * 2);
      uint8 value = (m_vram[start_address + address] >> shift) & 3;

      uint32 color;
      if (value == 0)
      {
        // background color
        color = background_color;
      }
      else
      {
        // foreground color
        color = foreground_palette[value << 1 | foreground_intensity];
      }

      m_display->SetPixel(i, scanline, color);
    }
  }
}
} // namespace HW