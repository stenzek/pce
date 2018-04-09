#pragma once

#include "pce/display.h"
#include <SDL.h>
#include <array>
#include <memory>
#include <mutex>
#include <vector>

class DisplaySDL : public Display
{
public:
  DisplaySDL();
  ~DisplaySDL();

  void ResizeDisplay(uint32 width = 0, uint32 height = 0) override;
  void ResizeFramebuffer(uint32 width, uint32 height) override;
  void DisplayFramebuffer() override;

  SDL_Window* GetSDLWindow() const { return m_window; }
  bool IsUIActive() const { return m_ui_active; }
  bool NeedsRender() const { return m_needs_render || m_ui_active; }
  bool HandleSDLEvent(const SDL_Event* ev);
  void RenderFrame();

  bool IsFullscreen() const;
  void SetFullscreen(bool enable);

protected:
  virtual uint32 GetAdditionalWindowCreateFlags() { return 0; }
  virtual bool Initialize();
  virtual void OnWindowResized();
  virtual void RenderImpl() = 0;

  SDL_Window* m_window = nullptr;
  uint32 m_window_width = 0;
  uint32 m_window_height = 0;

  static const uint32 NUM_FRAMEBUFFERS = 2;
  struct FrameBuffer
  {
    std::vector<uint32> data;
    uint32 stride = 0;
    uint32 width = 0;
    uint32 height = 0;
  };

  std::array<FrameBuffer, NUM_FRAMEBUFFERS> m_framebuffers;
  uint32 m_read_framebuffer_index = 0;
  uint32 m_write_framebuffer_index = 0;
  std::mutex m_framebuffer_mutex;

  uint32 m_render_event_type = 0;
  bool m_needs_render = false;
  bool m_ui_active = false;
};
