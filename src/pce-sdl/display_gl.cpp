#include "pce-sdl/display_gl.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/String.h"
#include <algorithm>

#pragma comment(lib, "opengl32.lib")

DisplayGL::DisplayGL() {}

DisplayGL::~DisplayGL()
{
  if (m_framebuffer_texture != 0)
    glDeleteTextures(1, &m_framebuffer_texture);

  if (m_gl_context)
  {
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DeleteContext(m_gl_context);
  }

  if (m_window)
    SDL_DestroyWindow(m_window);
}

std::unique_ptr<Display> DisplayGL::Create()
{
  DisplayGL* display = new DisplayGL();

  display->m_window =
    SDL_CreateWindow("Slightly-faster but still deprecated GL window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                     static_cast<int>(display->m_display_width), static_cast<int>(display->m_display_height),
                     SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (!display->m_window)
    return nullptr;

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  display->m_gl_context = SDL_GL_CreateContext(display->m_window);
  if (!display->m_gl_context)
    return nullptr;

  SDL_GL_MakeCurrent(display->m_window, display->m_gl_context);

  uint32 width = display->m_framebuffer_width;
  uint32 height = display->m_framebuffer_height;
  display->m_framebuffer_width = 0;
  display->m_framebuffer_height = 0;
  display->ResizeFramebuffer(width, height);
  display->DisplayFramebuffer();

  SDL_GL_MakeCurrent(nullptr, nullptr);

  return std::unique_ptr<Display>(display);
}

void DisplayGL::ResizeDisplay(uint32 width /*= 0*/, uint32 height /*= 0*/)
{
  Display::ResizeDisplay(width, height);

  // Don't do anything when it's maximized or fullscreen
  if (SDL_GetWindowFlags(m_window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN))
    return;

  SDL_SetWindowSize(m_window, static_cast<int>(m_display_width), static_cast<int>(m_display_height));
}

void DisplayGL::ResizeFramebuffer(uint32 width, uint32 height)
{
  if (m_framebuffer_width == width && m_framebuffer_height == height)
    return;

  if (m_framebuffer_texture == 0)
    glGenTextures(1, &m_framebuffer_texture);

  glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  uint32 pitch = sizeof(byte) * 4 * width;
  std::unique_ptr<byte[]> buffer = std::make_unique<byte[]>(pitch * height);

  // Copy as much as possible in
  if (m_framebuffer_pointer)
  {
    uint32 copy_width = std::min(m_framebuffer_width, width);
    uint32 copy_height = std::min(m_framebuffer_height, height);
    Y_memzero(buffer.get(), pitch * height);
    Y_memcpy_stride(buffer.get(), pitch, m_framebuffer_pointer, m_framebuffer_pitch, copy_width * 4, copy_height);
  }

  m_framebuffer_width = width;
  m_framebuffer_height = height;
  m_framebuffer_pitch = pitch;
  m_framebuffer_texture_buffer = std::move(buffer);
  m_framebuffer_pointer = m_framebuffer_texture_buffer.get();
}

void DisplayGL::DisplayFramebuffer()
{
  AddFrameRendered();

  int window_width, window_height;
  SDL_GetWindowSize(m_window, &window_width, &window_height);

  float display_ratio = float(m_display_width) / float(m_display_height);
  float window_ratio = float(window_width) / float(window_height);
  int viewport_width = 1;
  int viewport_height = 1;
  if (window_ratio >= display_ratio)
  {
    viewport_width = int(float(window_height) * display_ratio);
    viewport_height = window_height;
  }
  else
  {
    viewport_width = window_width;
    viewport_height = int(float(window_width) / display_ratio);
  }

  int viewport_x = (window_width - viewport_width) / 2;
  int viewport_y = (window_height - viewport_height) / 2;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(viewport_x, viewport_y, viewport_width, viewport_height);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_framebuffer_width, m_framebuffer_height, GL_RGBA, GL_UNSIGNED_BYTE,
                  m_framebuffer_pointer);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex4f(-1.0f, 1.0f, 0.0f, 1.0f);
  glTexCoord2f(0.0f, 1.0f);
  glVertex4f(-1.0f, -1.0f, 0.0f, 1.0f);
  glTexCoord2f(1.0f, 1.0f);
  glVertex4f(1.0f, -1.0f, 0.0f, 1.0f);
  glTexCoord2f(1.0f, 0.0f);
  glVertex4f(1.0f, 1.0f, 0.0f, 1.0f);
  glEnd();

  SDL_GL_SwapWindow(m_window);
}

bool DisplayGL::IsFullscreen() const
{
  return ((SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) != 0);
}

void DisplayGL::SetFullscreen(bool enable)
{
  SDL_SetWindowFullscreen(m_window, enable ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void DisplayGL::MakeCurrent()
{
  SDL_GL_MakeCurrent(m_window, m_gl_context);
  SDL_GL_SetSwapInterval(0);
}
