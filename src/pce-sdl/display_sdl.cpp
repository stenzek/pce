#include "pce-sdl/display_sdl.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"
#include <algorithm>

DisplaySDL::DisplaySDL()
{
  m_window_width = 900;
  m_window_height = 700;
}

DisplaySDL::~DisplaySDL()
{
  if (m_window)
    SDL_DestroyWindow(m_window);
}

bool DisplaySDL::Initialize()
{
  const uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
  m_window = SDL_CreateWindow("PCE - Initializing...", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_window_width,
                              m_window_height, flags | GetAdditionalWindowCreateFlags());
  if (!m_window)
    return false;

  m_render_event_type = SDL_RegisterEvents(1);
  return true;
}

bool DisplaySDL::HandleSDLEvent(const SDL_Event* ev)
{
  if (ev->type == m_render_event_type)
  {
    m_needs_render = true;
    return true;
  }

  if (ev->type == SDL_WINDOWEVENT && ev->window.event == SDL_WINDOWEVENT_RESIZED)
  {
    OnWindowResized();
    return true;
  }

  return false;
}

void DisplaySDL::RenderFrame()
{
  RenderImpl();
}

void DisplaySDL::ResizeDisplay(uint32 width /*= 0*/, uint32 height /*= 0*/)
{
  Display::ResizeDisplay(width, height);
  // SDL_SetWindowSize(m_window, static_cast<int>(m_display_width), static_cast<int>(m_display_height));
  // Don't do anything when it's maximized or fullscreen
  // if (SDL_GetWindowFlags(m_window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN))
  // return;
}

void DisplaySDL::ResizeFramebuffer(uint32 width, uint32 height)
{
  m_framebuffer_width = width;
  m_framebuffer_height = height;

  FrameBuffer& fb = m_framebuffers[m_write_framebuffer_index];
  if (fb.width != width || fb.height != height)
  {
    fb.width = width;
    fb.height = height;
    fb.data.resize(fb.width * fb.height);
    fb.stride = width * sizeof(uint32);
    m_framebuffer_pointer = reinterpret_cast<uint8*>(fb.data.data());
    m_framebuffer_pitch = width * sizeof(uint32);
  }
}

void DisplaySDL::DisplayFramebuffer()
{
  // Queue current FB.
  {
    std::lock_guard<std::mutex> guard(m_framebuffer_mutex);
    m_read_framebuffer_index = m_write_framebuffer_index;
    m_write_framebuffer_index = (m_write_framebuffer_index + 1) % NUM_FRAMEBUFFERS;
  }

  // Push event to present the current FB.
  {
    SDL_Event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.type = m_render_event_type;
    SDL_PushEvent(&ev);
  }

  // Set up the next framebuffer.
  FrameBuffer& fb = m_framebuffers[m_write_framebuffer_index];
  if (fb.width != m_framebuffer_width || fb.height != m_framebuffer_height)
  {
    fb.width = m_framebuffer_width;
    fb.height = m_framebuffer_height;
    fb.data.resize(fb.width * fb.height);
    fb.stride = fb.width * sizeof(uint32);
  }
  m_framebuffer_pointer = reinterpret_cast<uint8*>(fb.data.data());
  AddFrameRendered();
}

bool DisplaySDL::IsFullscreen() const
{
  return ((SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) != 0);
}

void DisplaySDL::SetFullscreen(bool enable)
{
  SDL_SetWindowFullscreen(m_window, enable ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void DisplaySDL::OnWindowResized()
{
  int width, height;
  SDL_GetWindowSize(m_window, &width, &height);
  m_window_width = width;
  m_window_height = height;
}
