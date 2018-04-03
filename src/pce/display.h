#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/Timer.h"
#include <memory>

class Display
{
public:
  Display();
  virtual ~Display();

  uint32 GetFramesRendered() const { return m_frames_rendered; }
  float GetFramesPerSecond() const { return m_fps; }
  void ResetFramesRendered() { m_frames_rendered = 0; }

  uint32 GetDisplayWidth() const { return m_display_width; }
  uint32 GetDisplayHeight() const { return m_display_height; }
  void SetDisplayScale(uint32 scale) { m_display_scale = scale; }
  void SetDisplayAspectRatio(uint32 numerator, uint32 denominator);
  virtual void ResizeDisplay(uint32 width = 0, uint32 height = 0);

  uint32 GetFramebufferWidth() const { return m_framebuffer_width; }
  uint32 GetFramebufferHeight() const { return m_framebuffer_height; }
  void ClearFramebuffer();
  virtual void ResizeFramebuffer(uint32 width, uint32 height) = 0;
  virtual void DisplayFramebuffer() = 0;

  virtual bool IsFullscreen() const = 0;
  virtual void SetFullscreen(bool enable) = 0;

  virtual void OnWindowResized() = 0;
  virtual void MakeCurrent() = 0;

  static constexpr uint32 PackRGB(uint8 r, uint8 g, uint8 b)
  {
    return (static_cast<uint32>(r) << 0) | (static_cast<uint32>(g) << 8) | (static_cast<uint32>(b) << 16) |
           (static_cast<uint32>(0xFF) << 24);
  }

  void SetPixel(uint32 x, uint32 y, uint8 r, uint8 g, uint8 b);
  void SetPixel(uint32 x, uint32 y, uint32 rgb);

protected:
  void AddFrameRendered();

  uint32 m_framebuffer_width = 640;
  uint32 m_framebuffer_height = 480;
  uint8* m_framebuffer_pointer = nullptr;
  uint32 m_framebuffer_pitch = 0;

  uint32 m_display_width = 640;
  uint32 m_display_height = 480;
  uint32 m_display_scale = 1;
  uint32 m_display_aspect_numerator = 1;
  uint32 m_display_aspect_denominator = 1;

  static const uint32 FRAME_COUNTER_FRAME_COUNT = 100;
  Timer m_frame_counter_timer;
  uint32 m_frames_rendered = 0;
  float m_fps = 0.0f;
};

class NullDisplay : public Display
{
public:
  NullDisplay();
  ~NullDisplay();

  static std::unique_ptr<Display> Create();

  void ResizeFramebuffer(uint32 width, uint32 height) override;
  void DisplayFramebuffer() override;

  bool IsFullscreen() const override;
  void SetFullscreen(bool enable) override;

  void OnWindowResized() override;
  void MakeCurrent() override;
};