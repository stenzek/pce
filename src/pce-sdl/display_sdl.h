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

  void ResizeDisplay(uint32 width = 0, uint32 height = 0) override;
  void ResizeFramebuffer(uint32 width, uint32 height) override;
  void DisplayFramebuffer() override;

  bool IsFullscreen() const override;
  void SetFullscreen(bool enable) override;

  void OnWindowResized() override;
  void MakeCurrent() override;

private:
  SDL_Window* m_window = nullptr;
  SDL_Surface* m_window_surface = nullptr;
  SDL_Surface* m_offscreen_surface = nullptr;
};
