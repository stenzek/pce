#pragma once

#include "pce/display.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <memory>

class DisplayGL : public Display
{
public:
  DisplayGL();
  ~DisplayGL();

  static std::unique_ptr<Display> Create();

  SDL_Window* GetSDLWindow() const { return m_window; }

  void ResizeDisplay(uint32 width = 0, uint32 height = 0) override;
  void ResizeFramebuffer(uint32 width, uint32 height) override;
  void DisplayFramebuffer() override;

private:
  SDL_Window* m_window = nullptr;
  SDL_GLContext m_gl_context = nullptr;
  GLuint m_framebuffer_texture = 0;
  std::unique_ptr<byte[]> m_framebuffer_texture_buffer;
};
