#include "pce-sdl/display_sdl.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"
#include <algorithm>

inline SDL_Surface* CreateOffscreenSurface(uint32 width, uint32 height)
{
  return SDL_CreateRGBSurface(0, width, height, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
}

DisplaySDL::DisplaySDL() {}

DisplaySDL::~DisplaySDL()
{
  if (m_offscreen_surface)
    SDL_FreeSurface(m_offscreen_surface);

  if (m_window)
    SDL_DestroyWindow(m_window);
}

std::unique_ptr<Display> DisplaySDL::Create()
{
  DisplaySDL* display = new DisplaySDL();

  display->m_window = SDL_CreateWindow("Slow-ass SDL window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       static_cast<int>(display->m_display_width),
                                       static_cast<int>(display->m_display_height), SDL_WINDOW_SHOWN);

  if (!display->m_window)
    return nullptr;

  display->m_window_surface = SDL_GetWindowSurface(display->m_window);
  if (!display->m_window_surface)
    return nullptr;

  display->m_offscreen_surface = CreateOffscreenSurface(display->m_framebuffer_width, display->m_framebuffer_height);
  if (!display->m_offscreen_surface)
    return nullptr;

  if (SDL_MUSTLOCK(display->m_offscreen_surface))
    SDL_LockSurface(display->m_offscreen_surface);

  display->m_framebuffer_pointer = static_cast<uint8*>(display->m_offscreen_surface->pixels);
  display->m_framebuffer_pitch = static_cast<uint32>(display->m_offscreen_surface->pitch);
  Y_memzero(display->m_framebuffer_pointer, display->m_framebuffer_pitch * display->m_framebuffer_height);
  display->DisplayFramebuffer();

  return std::unique_ptr<Display>(display);
}

void DisplaySDL::ResizeDisplay(uint32 width /*= 0*/, uint32 height /*= 0*/)
{
  Display::ResizeDisplay(width, height);

  SDL_SetWindowSize(m_window, static_cast<int>(m_display_width), static_cast<int>(m_display_height));

  m_window_surface = SDL_GetWindowSurface(m_window);
  Assert(m_window_surface != nullptr);
}

void DisplaySDL::ResizeFramebuffer(uint32 width, uint32 height)
{
  SDL_Surface* new_surface = CreateOffscreenSurface(width, height);
  Assert(new_surface != nullptr);

  // Copy as much as possible in
  uint32 copy_width = std::min(m_framebuffer_width, width);
  uint32 copy_height = std::min(m_framebuffer_height, height);
  if (SDL_MUSTLOCK(new_surface))
    SDL_LockSurface(new_surface);

  Y_memzero(new_surface->pixels, new_surface->pitch * height);
  Y_memcpy_stride(new_surface->pixels, new_surface->pitch, m_offscreen_surface->pixels, m_offscreen_surface->pitch,
                  copy_width * 4, copy_height);

  if (SDL_MUSTLOCK(m_offscreen_surface))
    SDL_UnlockSurface(m_offscreen_surface);

  SDL_FreeSurface(m_offscreen_surface);
  m_offscreen_surface = new_surface;
  m_framebuffer_pointer = reinterpret_cast<uint8*>(new_surface->pixels);
  m_framebuffer_pitch = static_cast<uint32>(new_surface->pitch);
  m_framebuffer_width = width;
  m_framebuffer_height = height;
}

void DisplaySDL::DisplayFramebuffer()
{
  if (SDL_MUSTLOCK(m_window_surface))
    SDL_LockSurface(m_window_surface);

  SDL_Rect src_rect = {0, 0, static_cast<int>(m_framebuffer_width), static_cast<int>(m_framebuffer_height)};
  SDL_Rect dst_rect = {0, 0, static_cast<int>(m_display_width), static_cast<int>(m_display_height)};
  SDL_BlitScaled(m_offscreen_surface, &src_rect, m_window_surface, &dst_rect);

  if (SDL_MUSTLOCK(m_window_surface))
    SDL_UnlockSurface(m_window_surface);

  SDL_UpdateWindowSurface(m_window);
}

bool DisplaySDL::IsFullscreen() const
{
  return false;
}

void DisplaySDL::SetFullscreen(bool enable) {}

void DisplaySDL::OnWindowResized() {}

void DisplaySDL::MakeCurrent() {}
