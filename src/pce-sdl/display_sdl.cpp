#include "pce-sdl/display_sdl.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"
#include "imgui.h"

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

  return true;
}

bool DisplaySDL::HandleSDLEvent(const SDL_Event* ev)
{
  if (ev->type == SDL_WINDOWEVENT && ev->window.event == SDL_WINDOWEVENT_RESIZED)
  {
    OnWindowResized();
    return true;
  }

  if (PassEventToImGui(ev))
    return true;

  return false;
}

bool DisplaySDL::PassEventToImGui(const SDL_Event* event)
{
  ImGuiIO& io = ImGui::GetIO();
  switch (event->type)
  {
    case SDL_MOUSEWHEEL:
    {
      if (event->wheel.x > 0)
        io.MouseWheelH += 1;
      if (event->wheel.x < 0)
        io.MouseWheelH -= 1;
      if (event->wheel.y > 0)
        io.MouseWheel += 1;
      if (event->wheel.y < 0)
        io.MouseWheel -= 1;
      return io.WantCaptureMouse;
    }

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    {
      bool down = event->type == SDL_MOUSEBUTTONDOWN;
      if (event->button.button == SDL_BUTTON_LEFT)
        io.MouseDown[0] = down;
      if (event->button.button == SDL_BUTTON_RIGHT)
        io.MouseDown[1] = down;
      if (event->button.button == SDL_BUTTON_MIDDLE)
        io.MouseDown[2] = down;
      return io.WantCaptureMouse;
    }

    case SDL_MOUSEMOTION:
    {
      io.MousePos.x = float(event->motion.x);
      io.MousePos.y = float(event->motion.y);
      return io.WantCaptureMouse;
    }

    case SDL_TEXTINPUT:
    {
      io.AddInputCharactersUTF8(event->text.text);
      return io.WantCaptureKeyboard;
    }

    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
      int key = event->key.keysym.scancode;
      IM_ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
      io.KeysDown[key] = (event->type == SDL_KEYDOWN);
      io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
      io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
      io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
      io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
      return io.WantCaptureKeyboard;
    }
  }
  return false;
}

void DisplaySDL::RenderFrame()
{
  RenderImpl();
  m_needs_render = false;
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
