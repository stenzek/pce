#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/Log.h"
#include "common/audio.h"
#include "imgui.h"
#include "nfd.h"
#include "pce-sdl/audio_sdl.h"
#include "pce-sdl/display_d3d.h"
#include "pce-sdl/display_gl.h"
#include "pce-sdl/display_sdl.h"
#include "pce-sdl/scancodes_sdl.h"
#include "pce/host_interface.h"
#include "pce/system.h"
#include <SDL.h>
#include <cstdio>
Log_SetChannel(Main);

class SDLHostInterface : public HostInterface
{
public:
  SDLHostInterface() = default;
  ~SDLHostInterface();

  static std::unique_ptr<SDLHostInterface> Create();

  bool Initialize(System* system) override;
  void Reset() override;
  void Cleanup() override;

  DisplaySDL* GetDisplay() const override { return m_display.get(); }
  Audio::Mixer* GetAudioMixer() const override { return m_mixer.get(); }
  bool NeedsRender() const { return m_display->NeedsRender(); }

  void ReportMessage(const char* message) override;
  void OnSimulationSpeedUpdate(float speed_percent) override;

  bool HandleSDLEvent(const SDL_Event* ev);
  void InjectKeyEvent(GenScanCode sc, bool down);

  void Render();

protected:
  // We only pass mouse input through if it's grabbed
  bool IsMouseGrabbed() const;
  void GrabMouse();
  void ReleaseMouse();
  void RenderImGui();
  void DoLoadState(uint32 index);
  void DoSaveState(uint32 index);

  std::unique_ptr<DisplaySDL> m_display;
  std::unique_ptr<Audio::Mixer> m_mixer;

  String m_last_message;
  Timer m_last_message_time;
};

std::unique_ptr<SDLHostInterface> SDLHostInterface::Create()
{
  // Initialize imgui.
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;

  // std::unique_ptr<DisplaySDL> display = DisplayGL::Create();
  std::unique_ptr<DisplaySDL> display = DisplayD3D::Create();
  if (!display)
  {
    Panic("Failed to create display");
    ImGui::DestroyContext();
    return nullptr;
  }

  std::unique_ptr<SDLHostInterface> hi = std::make_unique<SDLHostInterface>();
  hi->m_display = std::move(display);
  return hi;
}

SDLHostInterface::~SDLHostInterface()
{
  ImGui::DestroyContext();
}

bool SDLHostInterface::Initialize(System* system)
{
  if (!HostInterface::Initialize(system))
    return false;

  m_mixer = Mixer_SDL::Create();
  // m_mixer = Audio::NullMixer::Create();
  if (!m_mixer)
  {
    Panic("Failed to create audio mixer");
    return false;
  }

  m_display->SetDisplayAspectRatio(4, 3);
  return true;
}

void SDLHostInterface::Reset()
{
  HostInterface::Reset();
}

void SDLHostInterface::Cleanup()
{
  HostInterface::Cleanup();
  m_mixer.reset();
  m_display.reset();
}

void SDLHostInterface::ReportMessage(const char* message)
{
  m_last_message = message;
  m_last_message_time.Reset();
}

void SDLHostInterface::OnSimulationSpeedUpdate(float speed_percent)
{
  // Persist each message for only 3 seconds.
  if (!m_last_message.IsEmpty() && m_last_message_time.GetTimeSeconds() >= 3.0f)
    m_last_message.Clear();

  LargeString window_title;
  window_title.Format("PCE | System: %s | CPU: %s (%.2f MHz, %s) | Speed: %.1f%% | VPS: %.1f%s%s",
                      m_system->GetTypeInfo()->GetTypeName(), m_system->GetCPU()->GetModelString(),
                      m_system->GetCPU()->GetFrequency() / 1000000.0f, m_system->GetCPU()->GetCurrentBackendString(),
                      speed_percent, m_display->GetFramesPerSecond(), m_last_message.IsEmpty() ? "" : " | ",
                      m_last_message.GetCharArray());

  SDL_SetWindowTitle(m_display->GetSDLWindow(), window_title);
}

static inline uint32 SDLButtonToHostButton(uint32 button)
{
  // SDL left = 1, middle = 2, right = 3 :/
  switch (button)
  {
    case 1:
      return 0;
    case 2:
      return 2;
    case 3:
      return 1;
    default:
      return 0xFFFFFFFF;
  }
}

bool SDLHostInterface::HandleSDLEvent(const SDL_Event* ev)
{
  if (m_display->HandleSDLEvent(ev))
    return true;

  switch (ev->type)
  {
    case SDL_MOUSEBUTTONDOWN:
    {
      uint32 button = SDLButtonToHostButton(ev->button.button);
      if (IsMouseGrabbed())
      {
        m_system->QueueExternalEvent([this, button]() { ExecuteMouseButtonChangeCallbacks(button, true); });
        return true;
      }
    }
    break;

    case SDL_MOUSEBUTTONUP:
    {
      uint32 button = SDLButtonToHostButton(ev->button.button);
      if (IsMouseGrabbed())
      {
        m_system->QueueExternalEvent([this, button]() { ExecuteMouseButtonChangeCallbacks(button, false); });
        return true;
      }
      else
      {
        // Are we capturing the mouse?
        if (button == 0)
          GrabMouse();
      }
    }
    break;

    case SDL_MOUSEMOTION:
    {
      if (!IsMouseGrabbed())
        return false;

      int32 dx = ev->motion.xrel;
      int32 dy = ev->motion.yrel;
      m_system->QueueExternalEvent([this, dx, dy]() { ExecuteMousePositionChangeCallbacks(dx, dy); });
      return true;
    }
    break;

    case SDL_KEYDOWN:
    {
      // Release mouse key combination
      if (((ev->key.keysym.sym == SDLK_LCTRL || ev->key.keysym.sym == SDLK_RCTRL) &&
           (SDL_GetModState() & KMOD_ALT) != 0) ||
          ((ev->key.keysym.sym == SDLK_LALT || ev->key.keysym.sym == SDLK_RALT) &&
           (SDL_GetModState() & KMOD_CTRL) != 0))
      {
        // But don't consume the key event.
        ReleaseMouse();
      }

      // Create keyboard event.
      // TODO: Since we have crap in the input polling, we can't return true here.
      GenScanCode scancode;
      if (MapSDLScanCode(&scancode, ev->key.keysym.scancode))
      {
        m_system->QueueExternalEvent([this, scancode]() { ExecuteKeyboardCallback(scancode, true); });
        return false;
      }
    }
    break;

    case SDL_KEYUP:
    {
      // Create keyboard event.
      // TODO: Since we have crap in the input polling, we can't return true here.
      GenScanCode scancode;
      if (MapSDLScanCode(&scancode, ev->key.keysym.scancode))
      {
        m_system->QueueExternalEvent([this, scancode]() { ExecuteKeyboardCallback(scancode, false); });
        return false;
      }
    }
    break;
  }

  return false;
}

void SDLHostInterface::InjectKeyEvent(GenScanCode sc, bool down)
{
  m_system->QueueExternalEvent([this, sc, down]() { ExecuteKeyboardCallback(sc, down); });
}

bool SDLHostInterface::IsMouseGrabbed() const
{
  // Giant hack, TODO
  return (SDL_GetRelativeMouseMode() == SDL_TRUE);
  // return (SDL_GetWindowGrab(static_cast<DisplayGL*>(m_display.get())->GetSDLWindow()) == SDL_TRUE);
}

void SDLHostInterface::GrabMouse()
{
  SDL_SetWindowGrab(m_display->GetSDLWindow(), SDL_TRUE);
  SDL_SetRelativeMouseMode(SDL_TRUE);
}

void SDLHostInterface::ReleaseMouse()
{
  SDL_SetWindowGrab(m_display->GetSDLWindow(), SDL_FALSE);
  SDL_SetRelativeMouseMode(SDL_FALSE);
}

void SDLHostInterface::Render()
{
  RenderImGui();
  m_display->RenderFrame();
}

void SDLHostInterface::RenderImGui()
{
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("System"))
    {
      if (ImGui::MenuItem("Reset"))
        m_system->ExternalReset();

      ImGui::Separator();

      if (ImGui::BeginMenu("CPU Backend"))
      {
        CPUBackendType current_backend = m_system->GetCPU()->GetCurrentBackend();
        if (ImGui::MenuItem("Interpreter", nullptr, current_backend == CPUBackendType::Interpreter))
          m_system->SetCPUBackend(CPUBackendType::Interpreter);
        if (ImGui::MenuItem("Cached Interpreter", nullptr, current_backend == CPUBackendType::CachedInterpreter))
          m_system->SetCPUBackend(CPUBackendType::CachedInterpreter);
        if (ImGui::MenuItem("Recompiler", nullptr, current_backend == CPUBackendType::Recompiler))
          m_system->SetCPUBackend(CPUBackendType::Recompiler);

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("CPU Speed"))
      {
        float frequency = m_system->GetCPU()->GetFrequency();
        if (ImGui::InputFloat("Frequency", &frequency, 1000000.0f))
        {
          frequency = std::max(frequency, 1000000.0f);
          m_system->QueueExternalEvent([this, frequency]() { m_system->GetCPU()->SetFrequency(float(frequency)); });
        }
        ImGui::EndMenu();
      }

      if (ImGui::MenuItem("Enable Speed Limiter", nullptr, m_system->IsSpeedLimiterEnabled()))
        m_system->SetSpeedLimiterEnabled(!m_system->IsSpeedLimiterEnabled());

      if (ImGui::MenuItem("Flush Code Cache"))
        m_system->QueueExternalEvent([this]() { m_system->GetCPU()->FlushCodeCache(); });

      ImGui::Separator();

      if (ImGui::BeginMenu("Load State"))
      {
        for (uint32 i = 1; i <= 8; i++)
        {
          if (ImGui::MenuItem(TinyString::FromFormat("State %u", i).GetCharArray()))
            DoLoadState(i);
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Save State"))
      {
        for (uint32 i = 1; i <= 8; i++)
        {
          if (ImGui::MenuItem(TinyString::FromFormat("State %u", i).GetCharArray()))
            DoSaveState(i);
        }
        ImGui::EndMenu();
      }

      if (ImGui::MenuItem("Exit"))
        Log_DevPrintf("TODO");

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
      if (ImGui::MenuItem("Fullscreen", nullptr, m_display->IsFullscreen()))
        m_display->SetFullscreen(!m_display->IsFullscreen());

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Devices"))
    {
      if (ImGui::MenuItem("Capture Mouse") && !IsMouseGrabbed())
        GrabMouse();

      ImGui::Separator();

      if (ImGui::MenuItem("Send CTRL+ALT+DEL"))
      {
        // This has no delay, but the scancodes will still get enqueued.
        InjectKeyEvent(GenScanCode_LeftControl, true);
        InjectKeyEvent(GenScanCode_LeftAlt, true);
        InjectKeyEvent(GenScanCode_Delete, true);
        InjectKeyEvent(GenScanCode_LeftControl, false);
        InjectKeyEvent(GenScanCode_LeftAlt, false);
        InjectKeyEvent(GenScanCode_Delete, false);
        ReportMessage("Sent CTRL+ALT+DEL to machine.");
      }

      ImGui::Separator();

      for (const ComponentUIElement& ui : m_component_ui_elements)
      {
        const size_t total_callbacks = ui.callbacks.size() + ui.file_callbacks.size();
        if (total_callbacks == 0)
          continue;

        if (ImGui::BeginMenu(ui.component->GetIdentifier()))
        {
          for (const auto& it : ui.file_callbacks)
          {
            if (ImGui::MenuItem(it.first))
            {
              nfdchar_t* path;
              if (NFD_OpenDialog("", "", &path) == NFD_OKAY)
              {
                const auto& callback = it.second;
                String str(path);
                m_system->QueueExternalEvent([&callback, str]() { callback(str); });
              }
            }
          }

          for (const auto& it : ui.callbacks)
          {
            if (ImGui::MenuItem(it.first))
            {
              const auto& callback = it.second;
              m_system->QueueExternalEvent([&callback]() { callback(); });
            }
          }

          ImGui::EndMenu();
        }
      }

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

void SDLHostInterface::DoLoadState(uint32 index)
{
  SmallString filename;
  filename.Format("savestate_%u.bin", index);

  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE, &stream))
  {
    Log_ErrorPrintf("Failed to open %s", filename.GetCharArray());
    return;
  }

  m_system->LoadState(stream);
  stream->Release();
}

void SDLHostInterface::DoSaveState(uint32 index)
{
  SmallString filename;
  filename.Format("savestate_%u.bin", index);

  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename,
                                 BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE |
                                   BYTESTREAM_OPEN_SEEKABLE | BYTESTREAM_OPEN_ATOMIC_UPDATE,
                                 &stream))
  {
    Log_ErrorPrintf("Failed to open %s", filename.GetCharArray());
    return;
  }

  m_system->SaveState(stream);
  stream->Release();
}

std::unique_ptr<System> CreateSystem(const char* filename)
{
  Error error;
  std::unique_ptr<System> system = System::ParseConfig(filename, &error);
  if (!system)
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Parse Error", error.GetErrorCodeAndDescription(), nullptr);
    return nullptr;
  }

  return system;
}

static void Run(SDLHostInterface* host_interface, System* system)
{
  system->SetHostInterface(host_interface);

  system->Start(true);
  while (system->GetState() == System::State::Uninitialized)
  {
    SDL_PumpEvents();
    Thread::Sleep(0);
  }

#if 0
  {
    ByteStream* stream;
    if (ByteStream_OpenFileStream("savestate_4.bin", BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE, &stream))
    {
      Log_InfoPrintf("Loading state...");
      system->LoadState(stream);
      system->QueueExternalEvent([&]() { system->SetState(System::State::Running); });
      //host_interface->InjectKeyEvent(GenScanCode_Return, true);
      stream->Release();
    }
  }
#else
  system->QueueExternalEvent([&]() { system->SetState(System::State::Running); });
#endif

  while (system->GetState() != System::State::Stopped)
  {
    // SDL event loop...
    for (;;)
    {
      SDL_Event ev;
      if (!SDL_PollEvent(&ev))
      {
        // If we don't have to render, sleep until we get an event.
        if (!host_interface->GetDisplay()->NeedsRender())
        {
          SDL_WaitEvent(nullptr);
          continue;
        }
        else
        {
          break;
        }
      }

      host_interface->HandleSDLEvent(&ev);
    }

    if (host_interface->NeedsRender())
      host_interface->Render();
  }
}

// SDL requires the entry point declared without c++ decoration
#undef main
int main(int argc, char* argv[])
{
  RegisterAllTypes();

  // set log flags
  // g_pLog->SetConsoleOutputParams(true);
  // g_pLog->SetConsoleOutputParams(true, nullptr, LOGLEVEL_PROFILE);
  // g_pLog->SetConsoleOutputParams(true, "Bus HW::Serial", LOGLEVEL_PROFILE);
  g_pLog->SetConsoleOutputParams(true, "CPU_X86::CPU Bus HW::Serial", LOGLEVEL_PROFILE);
  // g_pLog->SetConsoleOutputParams(true, nullptr, LOGLEVEL_INFO);
  // g_pLog->SetConsoleOutputParams(true, nullptr, LOGLEVEL_WARNING);
  // g_pLog->SetConsoleOutputParams(true, nullptr, LOGLEVEL_ERROR);
  // g_pLog->SetFilterLevel(LOGLEVEL_PROFILE);
  // g_pLog->SetDebugOutputParams(true);

#ifdef Y_BUILD_CONFIG_RELEASE
  g_pLog->SetFilterLevel(LOGLEVEL_ERROR);
#else
  g_pLog->SetFilterLevel(LOGLEVEL_PROFILE);
#endif

  // init sdl
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
  {
    Panic("SDL initialization failed");
    return -1;
  }

  // create display and host interface
  std::unique_ptr<SDLHostInterface> host_interface = SDLHostInterface::Create();
  if (!host_interface)
    Panic("Failed to create host interface");

  // create system
  std::unique_ptr<System> system = CreateSystem(argv[1]);
  if (!system)
    return -1;

  Run(host_interface.get(), system.get());

  // done
  system.reset();
  host_interface.reset();

  SDL_Quit();
  return 0;
}
