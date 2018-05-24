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
#include "pce/hw/cga.h"
#include "pce/hw/cmos.h"
#include "pce/hw/et4000.h"
#include "pce/hw/fdc.h"
#include "pce/hw/i8237_dma.h"
#include "pce/hw/i8253_pit.h"
#include "pce/hw/i8259_pic.h"
#include "pce/hw/ps2.h"
#include "pce/hw/serial.h"
#include "pce/hw/serial_mouse.h"
#include "pce/hw/soundblaster.h"
#include "pce/hw/vga.h"
#include "pce/mmio.h"
#include "pce/system.h"
#include "pce/systems/pcami386.h"
#include "pce/systems/pcat.h"
#include "pce/systems/pcbochs.h"
#include "pce/systems/pcxt.h"
#include <SDL.h>
#include <cstdio>
Log_SetChannel(Main);

static bool LoadBIOS(const char* filename, std::function<bool(ByteStream*)> callback)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &stream))
  {
    Log_ErrorPrintf("Failed to open code file %s", filename);
    return false;
  }

  bool result = callback(stream);
  stream->Release();
  return result;
}

#if 0
static bool LoadATBios(const char* u27_filename, const char* u47_filename,
                       std::function<bool(ByteStream*, ByteStream*)> callback)
{
  ByteStream* u27_stream;
  ByteStream* u47_stream;
  if (!ByteStream_OpenFileStream(u27_filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &u27_stream))
  {
    Log_ErrorPrintf("Failed to open code file %s", u27_filename);
    return false;
  }
  if (!ByteStream_OpenFileStream(u47_filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &u47_stream))
  {
    Log_ErrorPrintf("Failed to open code file %s", u47_filename);
    u27_stream->Release();
    return false;
  }

  bool result = callback(u27_stream, u47_stream);
  u47_stream->Release();
  u27_stream->Release();
  return result;
}
#endif

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

static bool LoadHDD(HW::HDC* hdc, uint32 drive, const char* path, uint32 cylinders, uint32 heads, uint32 sectors)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(path, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE, &stream))
  {
    Log_ErrorPrintf("Failed to load floppy at %s", path);
    return false;
  }

  bool result = hdc->AttachDrive(drive, stream, cylinders, heads, sectors);
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
  // Systems::PCXT* system = new Systems::PCXT(host_interface, 1000000.0f, 640 * 1024, Systems::PCXT::VideoType::CGA80);
  // Systems::PCBochs* system = new Systems::PCBochs(host_interface, CPU_X86::MODEL_486, 1000000, 32 * 1024 * 1024);
  Systems::PCBochs* system = new Systems::PCBochs(host_interface, CPU_X86::MODEL_486, 20000000, 32 * 1024 * 1024);
  // Systems::PCBochs* system = new Systems::PCBochs(host_interface, CPU_X86::MODEL_486, 40000000, 32 * 1024 * 1024);
  // Systems::PC_AMI_386* system = new Systems::PC_AMI_386(host_interface, CPU_X86::MODEL_386, 4000000, 4 * 1024 *
  // 1024);

  system->GetCPU()->SetBackend(CPUBackendType::Interpreter);
  system->GetCPU()->SetBackend(CPUBackendType::CachedInterpreter);
  // system->GetCPU()->SetBackend(CPUBackendType::Recompiler);

#if 0
  HW::CGA* cga = new HW::CGA();
  system->AddComponent(cga);
#elif 0
  HW::VGA* vga = new HW::VGA();
  LoadBIOS("romimages\\VGABIOS-lgpl-latest", [vga](ByteStream* s) { return vga->SetBIOSROM(s); });
  system->AddComponent(vga);
#else
  HW::ET4000* vga = new HW::ET4000();
  LoadBIOS("romimages\\et4000.bin", [vga](ByteStream* s) { return vga->SetBIOSROM(s); });
  system->AddComponent(vga);
#endif

#if 1
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
#if 1
  // Sound blaster card
  system->AddComponent(new HW::SoundBlaster(system->GetDMAController(), HW::SoundBlaster::Type::SoundBlaster16));
#endif

  // system->GetFDDController()->SetDriveType(0, HW::FDC::DriveType_5_25);
  system->GetFDDController()->SetDriveType(0, HW::FDC::DriveType_3_5);
  // LoadFloppy(system->GetFDDController(), 0, "images\\386bench.img");
  // LoadFloppy(system->GetFDDController(), 0, "images\\DOS33-DISK01.IMG");
  // LoadFloppy(system->GetFDDController(), 1, "images\\8088mph.img");
  // LoadFloppy(system->GetFDDController(), 1, "images\\checkit3a.img");
  host_interface->AddDeviceFileCallback("Floppy A", [&system](const std::string& filename) {
    LoadFloppy(system->GetFDDController(), 0, filename.c_str());
  });
  host_interface->AddDeviceFileCallback("Floppy B", [&system](const std::string& filename) {
    LoadFloppy(system->GetFDDController(), 1, filename.c_str());
  });

  // LoadBIOS("romimages\\PCXTBIOS.BIN", [&system](ByteStream* s) { return system->AddROM(0xFE000, s); });
  // LoadBIOS("romimages\\386_ami.bin", [&system](ByteStream* s) { return system->AddROM(0xF0000, s); });
  // LoadBIOS("romimages\\ami386.bin", [&system](ByteStream* s) { return system->AddROM(0xF0000, s); });
  LoadBIOS("romimages\\BIOS-bochs-legacy",
           [&system](ByteStream* s) { return system->AddROM(0xF0000, s) && system->AddROM(0xFFFF0000u, s); });

  // LoadHDD(system->GetHDDController(), 0, "images\\HD-DOS33.img", 41, 16, 63);
  LoadHDD(system->GetHDDController(), 0, "images\\HD-DOS6-WFW311.img", 81, 16, 63);
  // LoadHDD(system->GetHDDController(), 0, "images\\hd10meg.img", 306, 4, 17);
  // LoadHDD(system->GetHDDController(), 0, "images\\win95.img", 243, 16, 63);
  // LoadHDD(system->GetHDDController(), 0, "images\\win98.img", 609, 16, 63);
  // LoadHDD(system->GetHDDController(), 0, "images\\c.img", 81, 16, 63);
  LoadHDD(system->GetHDDController(), 1, "images\\utils.img", 162, 16, 63);

#if 0
  system->Start(true);
  while (system->GetState() == System::State::Uninitialized)
  {
    SDL_PumpEvents();
    Thread::Sleep(0);
  }

  {
    ByteStream* stream;
    if (ByteStream_OpenFileStream("savestate.bin", BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE, &stream))
    {
      Log_InfoPrintf("Loading state...");
      system->LoadState(stream);
      system->QueueExternalEvent([&]() { system->SetState(System::State::Running); });
      host_interface->InjectKeyEvent(GenScanCode_LeftControl, false);
      host_interface->InjectKeyEvent(GenScanCode_LeftAlt, false);
      host_interface->InjectKeyEvent(GenScanCode_Y, true);
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
