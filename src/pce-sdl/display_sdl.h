#pragma once

#include "pce/display.h"
#include <SDL.h>
#include <memory>

class DisplaySDL : public Display
{
public:
  DisplaySDL();
  ~DisplaySDL();

  static std::unique_ptr<Display> Create();

  virtual void ResizeDisplay(uint32 width = 0, uint32 height = 0) override;
  virtual void ResizeFramebuffer(uint32 width, uint32 height) override;
  virtual void DisplayFramebuffer() override;

private:
  SDL_Window* m_window = nullptr;
  SDL_Surface* m_window_surface = nullptr;
  SDL_Surface* m_offscreen_surface = nullptr;
};
