#include "host_interface.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/Log.h"
#include "imgui.h"
#include "nfd.h"
#include "pce-sdl/scancodes_sdl.h"
#include "pce/system.h"
#include <SDL.h>
Log_SetChannel(SDLHostInterface);

SDLHostInterface::SDLHostInterface(std::unique_ptr<DisplayType> display, std::unique_ptr<MixerType> mixer)
  : m_display(std::move(display)), m_mixer(std::move(mixer))
{
  m_simulation_thread = std::thread([this]() { SimulationThreadRoutine(); });
}

SDLHostInterface::~SDLHostInterface()
{
  StopSimulationThread();
  m_simulation_thread.join();
  m_mixer.reset();
  m_display.reset();
  ImGui::DestroyContext();
}

std::unique_ptr<SDLHostInterface> SDLHostInterface::Create()
{
  // Initialize imgui.
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;

  auto display = DisplayType::Create();
  if (!display)
  {
    Panic("Failed to create display");
    ImGui::DestroyContext();
    return nullptr;
  }

  auto mixer = MixerType::Create();
  if (!mixer)
  {
    Panic("Failed to create audio mixer");
    display.reset();
    ImGui::DestroyContext();
    return false;
  }

  display->SetDisplayAspectRatio(4, 3);

  return std::make_unique<SDLHostInterface>(std::move(display), std::move(mixer));
}

TinyString SDLHostInterface::GetSaveStateFilename(u32 index)
{
  return TinyString::FromFormat("savestate_%u.bin", index);
}

bool SDLHostInterface::CreateSystem(const char* filename, s32 save_state_index /* = -1 */)
{
  Error error;
  if (!HostInterface::CreateSystem(filename, &error))
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Creating system failed", error.GetErrorCodeAndDescription(),
                             m_display->GetSDLWindow());
    return false;
  }

  if (save_state_index >= 0)
  {
    // Load the save state.
    if (!HostInterface::LoadSystemState(GetSaveStateFilename(static_cast<u32>(save_state_index)), &error))
    {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Loading save state failed", error.GetErrorCodeAndDescription(),
                               m_display->GetSDLWindow());
    }
  }

  // Resume execution.
  ResumeSimulation();
  m_running = true;
  return true;
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
        ExecuteMouseButtonChangeCallbacks(button, true);
        return true;
      }
    }
    break;

    case SDL_MOUSEBUTTONUP:
    {
      uint32 button = SDLButtonToHostButton(ev->button.button);
      if (IsMouseGrabbed())
      {
        ExecuteMouseButtonChangeCallbacks(button, false);
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
      ExecuteMousePositionChangeCallbacks(dx, dy);
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
        ExecuteKeyboardCallbacks(scancode, true);
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
        ExecuteKeyboardCallbacks(scancode, false);
        return false;
      }
    }
    break;

    case SDL_QUIT:
      m_running = false;
      break;
  }

  return false;
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
        ResetSystem();

      ImGui::Separator();

      if (ImGui::BeginMenu("CPU Backend"))
      {
        CPUBackendType current_backend = GetCPUBackend();
        if (ImGui::MenuItem("Interpreter", nullptr, current_backend == CPUBackendType::Interpreter))
          SetCPUBackend(CPUBackendType::Interpreter);
        if (ImGui::MenuItem("Cached Interpreter", nullptr, current_backend == CPUBackendType::CachedInterpreter))
          SetCPUBackend(CPUBackendType::CachedInterpreter);
        if (ImGui::MenuItem("Recompiler", nullptr, current_backend == CPUBackendType::Recompiler))
          SetCPUBackend(CPUBackendType::Recompiler);

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("CPU Speed"))
      {
        float frequency = GetCPUFrequency();
        if (ImGui::InputFloat("Frequency", &frequency, 1000000.0f))
        {
          frequency = std::max(frequency, 1000000.0f);
          SetCPUFrequency(frequency);
        }
        ImGui::EndMenu();
      }

      if (ImGui::MenuItem("Enable Speed Limiter", nullptr, IsSpeedLimiterEnabled()))
        SetSpeedLimiterEnabled(!IsSpeedLimiterEnabled());

      if (ImGui::MenuItem("Flush Code Cache"))
        FlushCPUCodeCache();

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
        m_running = false;

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
        SendCtrlAltDel();

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
                QueueExternalEvent([&callback, str]() { callback(str); }, false);
              }
            }
          }

          for (const auto& it : ui.callbacks)
          {
            if (ImGui::MenuItem(it.first))
            {
              const auto& callback = it.second;
              QueueExternalEvent([&callback]() { callback(); }, false);
            }
          }

          ImGui::EndMenu();
        }
      }

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  // Activity window
  {
    bool has_activity = false;
    for (const auto& elem : m_component_ui_elements)
    {
      if (elem.indicator_state != IndicatorState::Off)
      {
        has_activity = true;
        break;
      }
    }
    if (has_activity)
    {
      ImGui::SetNextWindowPos(ImVec2(1.0f, 32.0f));
      if (ImGui::Begin("Activity", nullptr,
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoInputs))
      {
        for (const auto& elem : m_component_ui_elements)
        {
          if (elem.indicator_state == IndicatorState::Off)
            continue;

          const char* text = elem.indicator_state == IndicatorState::Reading ? "Reading" : "Writing";
          const ImVec4 color = elem.indicator_state == IndicatorState::Reading ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :
                                                                                 ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
          ImGui::TextColored(color, "%s (%s): %s", elem.component->GetIdentifier().GetCharArray(),
                             elem.component->GetTypeInfo()->GetTypeName(), text);
        }
        ImGui::End();
      }
    }
  }
}

void SDLHostInterface::DoLoadState(uint32 index)
{
  Error error;
  if (!LoadSystemState(TinyString::FromFormat("savestate_%u.bin", index), &error))
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Loading save state failed", error.GetErrorCodeAndDescription(),
                             m_display->GetSDLWindow());
  }
}

void SDLHostInterface::DoSaveState(uint32 index)
{
  SaveSystemState(TinyString::FromFormat("savestate_%u.bin", index));
}

void SDLHostInterface::Run()
{
  while (m_running)
  {
    for (;;)
    {
      SDL_Event ev;
      if (SDL_PollEvent(&ev))
        HandleSDLEvent(&ev);
      else
        break;
    }

    Render();
  }

  StopSimulation();
}
