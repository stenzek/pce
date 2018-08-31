#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/String.h"
#include "YBaseLib/Thread.h"
#include "YBaseLib/Timer.h"
#include "imgui.h"
#include "nfd.h"
#include "pce-sdl/audio_sdl.h"
#include "pce-sdl/display_d3d.h"
#include "pce-sdl/display_gl.h"
#include "pce-sdl/display_sdl.h"
#include "pce-sdl/scancodes_sdl.h"
#include "pce/audio.h"
#include "pce/bus.h"
#include "pce/cpu_x86/cpu.h"
#include "pce/host_interface.h"
#include "pce/hw/adlib.h"
#include "pce/hw/cdrom.h"
#include "pce/hw/cga.h"
#include "pce/hw/cmos.h"
#include "pce/hw/et4000.h"
#include "pce/hw/fdc.h"
#include "pce/hw/i8042_ps2.h"
#include "pce/hw/i8237_dma.h"
#include "pce/hw/i8253_pit.h"
#include "pce/hw/i8259_pic.h"
#include "pce/hw/serial.h"
#include "pce/hw/serial_mouse.h"
#include "pce/hw/soundblaster.h"
#include "pce/hw/vga.h"
#include "pce/mmio.h"
#include "pce/system.h"
#include "pce/systems/ali1429.h"
#include "pce/systems/ami386.h"
#include "pce/systems/bochs.h"
#include "pce/systems/i430fx.h"
#include "pce/systems/ibmat.h"
#include "pce/systems/ibmxt.h"
#include <SDL.h>
#include <cstdio>
Log_SetChannel(Main);

static bool LoadFloppy(HW::FDC* fdc, uint32 disk, const char* path)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(path, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE, &stream))
  {
    Log_ErrorPrintf("Failed to load floppy at %s", path);
    return false;
  }

  HW::FDC::DiskType disk_type = HW::FDC::DetectDiskType(stream);
  if (disk_type == HW::FDC::DiskType_None)
  {
    stream->Release();
    return false;
  }

  bool result = fdc->InsertDisk(disk, disk_type, stream);
  stream->Release();
  return result;
}

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

  void AddDeviceFileCallback(const char* title, std::function<void(const std::string&)>&& callback);

protected:
  struct DeviceFileEntry
  {
    String title;
    std::function<void(const std::string&)> callback;
  };

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

  std::vector<DeviceFileEntry> m_device_files;
  const DeviceFileEntry* m_current_device_file = nullptr;
  String m_current_device_filename;
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
  hi->m_current_device_filename.Resize(512);
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

  const char* cpu_backend = "";
  switch (m_system->GetCPU()->GetCurrentBackend())
  {
    case CPUBackendType::Interpreter:
      cpu_backend = "Interpreter";
      break;
    case CPUBackendType::CachedInterpreter:
      cpu_backend = "Cached Interpreter";
      break;
    case CPUBackendType::Recompiler:
      cpu_backend = "Recompiler";
      break;
  }

  SmallString window_title;
  window_title.Format("PCE | %s | CPU: %s | Speed: %.1f%% | VPS: %.1f%s%s", m_system->GetSystemName(), cpu_backend,
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

void SDLHostInterface::AddDeviceFileCallback(const char* title, std::function<void(const std::string&)>&& callback)
{
  DeviceFileEntry dfe;
  dfe.title = title;
  dfe.callback = callback;
  m_device_files.push_back(dfe);
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

      for (const DeviceFileEntry& dfe : m_device_files)
      {
        if (ImGui::MenuItem(dfe.title))
          m_current_device_file = &dfe;
      }

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  bool opened = false;
  if (m_current_device_file)
  {
    ImGui::SetNextWindowSize(ImVec2(250, 100));
    if (ImGui::Begin("Change Image", &opened))
    {
      ImGui::Text("Device: %s", m_current_device_file->title.GetCharArray());
      if (ImGui::InputText("", m_current_device_filename.GetWriteableCharArray(),
                           m_current_device_filename.GetBufferSize()))
      {
        m_current_device_filename.UpdateSize();
      }

      ImGui::SameLine();
      if (ImGui::Button("..."))
      {
        nfdchar_t* path;
        if (NFD_OpenDialog("", "", &path) == NFD_OKAY)
          m_current_device_filename = path;
      }

      if (ImGui::Button("Mount"))
      {
        const DeviceFileEntry* dfe = m_current_device_file;
        std::string str(m_current_device_filename);
        m_system->QueueExternalEvent([str, dfe]() { dfe->callback(str); });
        m_current_device_file = nullptr;
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel"))
        m_current_device_file = nullptr;

      ImGui::End();
    }
    else if (!opened)
    {
      m_current_device_file = nullptr;
    }
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

static void TestBIOS(SDLHostInterface* host_interface)
{
  // auto* system = new Systems::PCXT(host_interface, 1000000.0f, 640 * 1024, Systems::PCXT::VideoType::CGA80);
  // auto* system = new Systems::PCBochs(host_interface, CPU_X86::MODEL_486, 1000000, 32 * 1024 * 1024);
  // auto* system = new Systems::Bochs(host_interface, CPU_X86::MODEL_PENTIUM, 10000000, 32 * 1024 * 1024);
  // auto* system = new Systems::PC_AMI_386(host_interface, CPU_X86::MODEL_386, 4000000, 4 * 1024 * 1024);
  // auto* system = new Systems::PCALI1429(host_interface, CPU_X86::MODEL_486, 1000000, 16 * 1024 * 1024);
  auto* system = new Systems::i430FX(host_interface, CPU_X86::MODEL_PENTIUM, 10000000, 32 * 1024 * 1024);

  system->GetCPU()->SetBackend(CPUBackendType::Interpreter);
  // system->GetCPU()->SetBackend(CPUBackendType::CachedInterpreter);
  // system->GetCPU()->SetBackend(CPUBackendType::Recompiler);

#if 0
  system->AddComponent(new HW::CGA());
#elif 1
  system->AddComponent(new HW::VGA());
#else
  system->AddComponent(new HW::ET4000());
#endif

#if 0
  // Adding a serial mouse to COM1, because why not
  HW::Serial* serial_port_COM1 = new HW::Serial(system->GetInterruptController(), HW::Serial::Model_16550);
  HW::SerialMouse* serial_mouse = new HW::SerialMouse(serial_port_COM1);
  system->AddComponent(serial_port_COM1);
  system->AddComponent(serial_mouse);
#endif

#if 0
  // Adlib synth card
  system->AddComponent(new HW::AdLib());
#endif
#if 0
  // Sound blaster card
  system->AddComponent(new HW::SoundBlaster(system->GetDMAController(), HW::SoundBlaster::Type::SoundBlaster16));
#endif
#if 0
  // cdrom
  HW::CDROM* cdrom = new HW::CDROM();
  system->AddComponent(cdrom);
  system->GetHDDController()->AttachATAPIDevice(1, cdrom);
  host_interface->AddDeviceFileCallback(
    "CDROM", [&system, cdrom](const std::string& filename) { cdrom->InsertMedia(filename.c_str()); });
#endif

  // system->GetFDDController()->SetDriveType(0, HW::FDC::DriveType_5_25);
  system->GetFDDController()->SetDriveType(0, HW::FDC::DriveType_3_5);
  LoadFloppy(system->GetFDDController(), 0, "images\\bootfloppy.img");
  host_interface->AddDeviceFileCallback("Floppy A", [&system](const std::string& filename) {
    LoadFloppy(system->GetFDDController(), 0, filename.c_str());
  });
  host_interface->AddDeviceFileCallback("Floppy B", [&system](const std::string& filename) {
    LoadFloppy(system->GetFDDController(), 1, filename.c_str());
  });

  system->GetHDDController()->AttachDrive(0, "images\\blank.img", 243, 16, 63);

#if 0
  system->Start(true);
  while (system->GetState() == System::State::Uninitialized)
  {
    SDL_PumpEvents();
    Thread::Sleep(0);
  }

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
  system->Start();
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
  // set log flags
  // g_pLog->SetConsoleOutputParams(true);
  g_pLog->SetConsoleOutputParams(true, nullptr, LOGLEVEL_PROFILE);
  // g_pLog->SetConsoleOutputParams(true, "Bus HW::Serial", LOGLEVEL_PROFILE);
  // g_pLog->SetConsoleOutputParams(true, "CPU_X86::CPU Bus HW::Serial", LOGLEVEL_PROFILE);
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

#if defined(__WIN32__)
  // fix up stdout/stderr on win32
  // freopen("CONOUT$", "w", stdout);
  // freopen("CONOUT$", "w", stderr);
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

  TestBIOS(host_interface.get());

  // done
  host_interface.reset();

  SDL_Quit();
  return 0;
}
