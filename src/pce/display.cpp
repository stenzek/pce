#include "pce/display.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Math.h"
#include <algorithm>
#include <cstring>

Display::Display() {}

Display::~Display() {}

void Display::SetDisplayAspectRatio(uint32 numerator, uint32 denominator)
{
  if (m_display_aspect_numerator == numerator && m_display_aspect_denominator == denominator)
    return;

  m_display_aspect_numerator = numerator;
  m_display_aspect_denominator = denominator;
  ResizeDisplay();
}

void Display::ResizeDisplay(uint32 width /*= 0*/, uint32 height /*= 0*/)
{
  // If width/height == 0, use aspect ratio to calculate size
  if (width == 0 && height == 0)
  {
    // TODO: Remove floating point math here
    // float pixel_aspect_ratio = static_cast<float>(m_framebuffer_width) / static_cast<float>(m_framebuffer_height);
    float display_aspect_ratio =
      static_cast<float>(m_display_aspect_numerator) / static_cast<float>(m_display_aspect_denominator);
    // float ratio = pixel_aspect_ratio / display_aspect_ratio;
    m_display_width = std::max(1u, m_framebuffer_width * m_display_scale);
    m_display_height = std::max(1u, static_cast<uint32>(static_cast<float>(m_display_width) / display_aspect_ratio));
  }
  else
  {
    DebugAssert(width > 0 && height > 0);
    m_display_width = width * m_display_scale;
    m_display_height = height * m_display_scale;
  }
}

void Display::ClearFramebuffer()
{
  std::memset(m_framebuffer_pointer, 0, m_framebuffer_pitch * m_framebuffer_height);
}

void Display::SetPixel(uint32 x, uint32 y, uint8 r, uint8 g, uint8 b)
{
  SetPixel(x, y, PackRGB(r, g, b));
}

void Display::SetPixel(uint32 x, uint32 y, uint32 rgb)
{
  DebugAssert(x < m_framebuffer_width && y < m_framebuffer_height);

#if 1
  // Assumes LE order in rgb and framebuffer.
  rgb |= 0xFF000000;
  std::memcpy(&m_framebuffer_pointer[y * m_framebuffer_pitch + x * 4], &rgb, sizeof(rgb));
#else
  m_framebuffer_pointer[y * m_framebuffer_pitch + x * 4 + 0] = static_cast<uint8>((rgb >> 0) & 0xFF);
  m_framebuffer_pointer[y * m_framebuffer_pitch + x * 4 + 1] = static_cast<uint8>((rgb >> 8) & 0xFF);
  m_framebuffer_pointer[y * m_framebuffer_pitch + x * 4 + 2] = static_cast<uint8>((rgb >> 16) & 0xFF);
  m_framebuffer_pointer[y * m_framebuffer_pitch + x * 4 + 3] = 0xFF;
#endif
}

void Display::CopyFrame(const void* pixels, uint32 stride)
{
  if (stride == m_framebuffer_pitch)
  {
    std::memcpy(m_framebuffer_pointer, pixels, stride * m_framebuffer_height);
    return;
  }

  const byte* pixels_src = reinterpret_cast<const byte*>(pixels);
  byte* pixels_dst = m_framebuffer_pointer;
  uint32 copy_stride = std::min(m_framebuffer_pitch, stride);
  for (uint32 i = 0; i < m_framebuffer_height; i++)
  {
    std::memcpy(pixels_dst, pixels_src, copy_stride);
    pixels_src += stride;
    pixels_dst += m_framebuffer_pitch;
  }
}

void Display::AddFrameRendered()
{
  m_frames_rendered++;

  // Update every 500ms
  float dt = float(m_frame_counter_timer.GetTimeSeconds());
  if (dt >= 1.0f)
  {
    m_fps = float(m_frames_rendered) * (1.0f / dt);
    m_frames_rendered = 0;
    m_frame_counter_timer.Reset();
  }
}

NullDisplay::NullDisplay() {}

NullDisplay::~NullDisplay() {}

std::unique_ptr<Display> NullDisplay::Create()
{
  return std::make_unique<NullDisplay>();
}

void NullDisplay::ResizeFramebuffer(uint32 width, uint32 height) {}

void NullDisplay::DisplayFramebuffer() {}
