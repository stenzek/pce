#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/String.h"
#include "YBaseLib/Thread.h"
#include "YBaseLib/Timer.h"
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

class SDLHostInterface : public HostInterface
{
public:
  SDLHostInterface() = default;
  ~SDLHostInterface() = default;

  static std::unique_ptr<SDLHostInterface> Create();

  bool Initialize(System* system) override;
  void Reset() override;
  void Cleanup() override;

  Display* GetDisplay() const override { return m_display.get(); }
  Audio::Mixer* GetAudioMixer() const override { return m_mixer.get(); }

  void ReportMessage(const char* message) override;
  void OnSimulationSpeedUpdate(float speed_percent) override;

  bool IsInputSDLEvent(const SDL_Event* ev);
  bool HandleSDLEvent(const SDL_Event* ev);
  void InjectKeyEvent(GenScanCode sc, bool down);

protected:
  // We only pass mouse input through if it's grabbed
  bool IsMouseGrabbed() const;
  void GrabMouse();
  void ReleaseMouse();

  std::unique_ptr<Display> m_display;
  std::unique_ptr<Audio::Mixer> m_mixer;

  String m_last_message;
  Timer m_last_message_time;
};

std::unique_ptr<SDLHostInterface> SDLHostInterface::Create()
{
  // std::unique_ptr<Display> display = DisplayGL::Create();
  std::unique_ptr<Display> display = DisplayD3D::Create();
  if (!display)
  {
    Panic("Failed to create display");
    return nullptr;
  }

  std::unique_ptr<SDLHostInterface> hi = std::make_unique<SDLHostInterface>();
  hi->m_display = std::move(display);
  return hi;
}

bool SDLHostInterface::Initialize(System* system)
{
  if (!HostInterface::Initialize(system))
    return false;

  m_display->MakeCurrent();

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
    case CPUBackendType::FastInterpreter:
      cpu_backend = "Fast Interpreter";
      break;
    case CPUBackendType::CachedInterpreter:
      cpu_backend = "Cached Interpreter";
      break;
    case CPUBackendType::NewInterpreter:
      cpu_backend = "New Interpreter";
      break;
    case CPUBackendType::Recompiler:
      cpu_backend = "Recompiler";
      break;
  }

  SmallString window_title;
  window_title.Format("PCE | %s | CPU: %s | Speed: %.1f%% | VPS: %.1f%s%s", m_system->GetSystemName(), cpu_backend,
                      speed_percent, m_display->GetFramesPerSecond(), m_last_message.IsEmpty() ? "" : " | ",
                      m_last_message.GetCharArray());

  SDL_Window* window = static_cast<DisplayGL*>(m_display.get())->GetSDLWindow();
  SDL_SetWindowTitle(window, window_title);
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

bool SDLHostInterface::IsInputSDLEvent(const SDL_Event* ev)
{
  switch (ev->type)
  {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEMOTION:
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      return true;

    default:
      return false;
  }
}

bool SDLHostInterface::HandleSDLEvent(const SDL_Event* ev)
{
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
        ExecuteKeyboardCallback(scancode, true);
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
        ExecuteKeyboardCallback(scancode, false);
        return false;
      }
    }
    break;
  }

  return false;
}

void SDLHostInterface::InjectKeyEvent(GenScanCode sc, bool down)
{
  ExecuteKeyboardCallback(sc, down);
}

bool SDLHostInterface::IsMouseGrabbed() const
{
  // Giant hack, TODO
  return (SDL_GetRelativeMouseMode() == SDL_TRUE);
  // return (SDL_GetWindowGrab(static_cast<DisplayGL*>(m_display.get())->GetSDLWindow()) == SDL_TRUE);
}

void SDLHostInterface::GrabMouse()
{
  SDL_Window* window = static_cast<DisplayGL*>(m_display.get())->GetSDLWindow();
  SDL_SetWindowGrab(window, SDL_TRUE);
  SDL_SetRelativeMouseMode(SDL_TRUE);
}

void SDLHostInterface::ReleaseMouse()
{
  SDL_Window* window = static_cast<DisplayGL*>(m_display.get())->GetSDLWindow();
  SDL_SetWindowGrab(window, SDL_FALSE);
  SDL_SetRelativeMouseMode(SDL_FALSE);
}

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

static void TestBIOS()
{
  std::unique_ptr<SDLHostInterface> host_interface = SDLHostInterface::Create();
  if (!host_interface)
    Panic("Failed to create host interface");

  Systems::PCXT* system =
    new Systems::PCXT(host_interface.get(), 1000000.0f, 640 * 1024, Systems::PCXT::VideoType::CGA80);
  // Systems::PCAT* system = new Systems::PCAT(host_interface.get(), cpu, 1 * 1024 * 1024);
  // Systems::PCAT* system = new Systems::PCAT(host_interface.get(), cpu, 4 * 1024 * 1024);
  // Systems::PCAT* system = new Systems::PCAT(host_interface.get(), cpu, 8 * 1024 * 1024);
  // Systems::PCBochs* system = new Systems::PCBochs(host_interface.get(), cpu, 4 * 1024 * 1024);
  // Systems::PCBochs* system = new Systems::PCBochs(host_interface.get(), cpu, 8 * 1024 * 1024);
  // Systems::PCBochs* system = new Systems::PCBochs(host_interface.get(), cpu, 16 * 1024 * 1024);
  // Systems::PCBochs* system = new Systems::PCBochs(host_interface.get(), cpu, 20 * 1024 * 1024);
  // Systems::PCBochs* system = new Systems::PCBochs(host_interface.get(), CPU_X86::MODEL_486, 1000000, 32 * 1024 *
  // Systems::PCBochs* system = new Systems::PCBochs(host_interface.get(), CPU_X86::MODEL_486, 20000000, 32 * 1024 *
  // 1024); Systems::PC_AMI_386* system = new Systems::PC_AMI_386(host_interface.get(), CPU_X86::MODEL_386, 4000000, 4 *
  // 1024 * 1024);

  system->GetCPU()->SetBackend(CPUBackendType::Interpreter);
  system->GetCPU()->SetBackend(CPUBackendType::FastInterpreter);
  system->GetCPU()->SetBackend(CPUBackendType::NewInterpreter);
  // system->GetCPU()->SetBackend(CPUBackendType::CachedInterpreter);
  // system->GetCPU()->SetBackend(CPUBackendType::Recompiler);

#if 1
  HW::CGA* cga = new HW::CGA();
  system->AddComponent(cga);
#else
  HW::VGA* vga = new HW::VGA();
  LoadBIOS("romimages\\VGABIOS-lgpl-latest", [vga](ByteStream* s) { return vga->SetBIOSROM(s); });
  system->AddComponent(vga);
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

  // LoadFloppy(system->GetFDDController(), 0, "images\\386bench.img");
  LoadFloppy(system->GetFDDController(), 0, "images\\DOS33-DISK01.IMG");

  LoadBIOS("romimages\\PCXTBIOS.BIN", [&system](ByteStream* s) { return system->AddROM(0xFE000, s); });
  // LoadBIOS("romimages\\386_ami.bin", [&system](ByteStream* s) { return system->AddROM(0xF0000, s); });
  // LoadBIOS("romimages\\ami386.bin", [&system](ByteStream* s) { return system->AddROM(0xF0000, s); });
  // LoadBIOS("romimages\\BIOS-bochs-legacy",
  // [&system](ByteStream* s) { return system->AddROM(0xF0000, s) && system->AddROM(0xFFFF0000u, s); });

  // LoadHDD(system->GetHDDController(), 0, "images\\HD-DOS33.img", 41, 16, 63);
  // LoadHDD(system->GetHDDController(), 0, "images\\HD-DOS6-WFW311.img", 81, 16, 63);
  // LoadHDD(system->GetHDDController(), 0, "images\\hd10meg.img", 306, 4, 17);
  // LoadHDD(system->GetHDDController(), 0, "images\\win95.img", 243, 16, 63);
  // LoadHDD(system->GetHDDController(), 0, "images\\win98.img", 609, 16, 63);
  // LoadHDD(system->GetHDDController(), 0, "images\\c.img", 81, 16, 63);
  // LoadHDD(system->GetHDDController(), 1, "images\\utils.img", 162, 16, 63);

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
      system->QueueExternalEvent([&]() {
        system->SetState(System::State::Running);
        host_interface->InjectKeyEvent(GenScanCode_LeftControl, false);
        host_interface->InjectKeyEvent(GenScanCode_LeftAlt, false);
        host_interface->InjectKeyEvent(GenScanCode_Y, true);
      });
      stream->Release();
    }
  }
#else
  system->Start();
#endif

  while (system->GetState() != System::State::Stopped)
  {
    SDL_Event ev;
    if (!SDL_WaitEvent(&ev))
      continue;

    // Needs to push event to the guest?
    if (host_interface->IsInputSDLEvent(&ev))
    {
      // Use an external event so we call handle on the simulation thread.
      system->QueueExternalEvent([&host_interface, ev]() { host_interface->HandleSDLEvent(&ev); });
    }
    else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_RESIZED)
    {
      host_interface->GetDisplay()->OnWindowResized();
    }
    else if (ev.type == SDL_KEYUP && (SDL_GetModState() & (KMOD_LCTRL | KMOD_LALT)) == (KMOD_LCTRL | KMOD_LALT))
    {
      if (ev.key.keysym.sym == SDLK_r)
      {
        system->Reset();
      }
      else if (ev.key.keysym.sym == SDLK_l)
      {
        ByteStream* stream;
        if (ByteStream_OpenFileStream("savestate.bin", BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE, &stream))
        {
          system->LoadState(stream);
          stream->Release();
        }
        else
        {
          Log_ErrorPrintf("Failed to open stream");
        }
      }
      else if (ev.key.keysym.sym == SDLK_s)
      {
        ByteStream* stream;
        if (ByteStream_OpenFileStream("savestate.bin",
                                      BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE |
                                        BYTESTREAM_OPEN_SEEKABLE | BYTESTREAM_OPEN_ATOMIC_UPDATE,
                                      &stream))
        {
          system->SaveState(stream);
          stream->Release();
        }
        else
        {
          Log_ErrorPrintf("Failed to open stream");
        }
      }
      else if (ev.key.keysym.sym == SDLK_1)
      {
        system->SetCPUBackend(CPUBackendType::Interpreter);
      }
      else if (ev.key.keysym.sym == SDLK_2)
      {
        system->SetCPUBackend(CPUBackendType::FastInterpreter);
      }
      else if (ev.key.keysym.sym == SDLK_3)
      {
        system->SetCPUBackend(CPUBackendType::CachedInterpreter);
      }
      else if (ev.key.keysym.sym == SDLK_4)
      {
        system->SetCPUBackend(CPUBackendType::Recompiler);
      }
      else if (ev.key.keysym.sym == SDLK_5)
      {
        system->SetCPUBackend(CPUBackendType::NewInterpreter);
      }
      else if (ev.key.keysym.sym == SDLK_SPACE)
      {
        system->SetSpeedLimiterEnabled(!system->IsSpeedLimiterEnabled());
      }
      else if (ev.key.keysym.sym == SDLK_KP_PLUS)
      {
        system->GetCPU()->SetFrequency(std::max(system->GetCPU()->GetFrequency() + 1000000.0f, 1000000.0f));
        host_interface->ReportFormattedMessage("CPU frequency set to %.1f M-IPS",
                                               system->GetCPU()->GetFrequency() / 1000000.0f);
      }
      else if (ev.key.keysym.sym == SDLK_KP_MINUS)
      {
        system->GetCPU()->SetFrequency(std::max(system->GetCPU()->GetFrequency() - 1000000.0f, 1000000.0f));
        host_interface->ReportFormattedMessage("CPU frequency set to %.1f M-IPS",
                                               system->GetCPU()->GetFrequency() / 1000000.0f);
      }
      else if (ev.key.keysym.sym == SDLK_RETURN)
      {
        host_interface->GetDisplay()->SetFullscreen(!host_interface->GetDisplay()->IsFullscreen());
      }
      else if (ev.key.keysym.sym == SDLK_BACKSPACE)
      {
        system->QueueExternalEvent([system]() { system->GetCPU()->FlushCodeCache(); });
        host_interface->ReportMessage("Flushed code cache.");
      }
    }
  }
}

// SDL requires the entry point declared without c++ decoration
#undef main
int main(int argc, char* argv[])
{
  // set log flags
  // g_pLog->SetConsoleOutputParams(true);
  // g_pLog->SetConsoleOutputParams(true, nullptr, LOGLEVEL_PROFILE);
  g_pLog->SetConsoleOutputParams(true, nullptr, LOGLEVEL_INFO);
  // g_pLog->SetConsoleOutputParams(true, nullptr, LOGLEVEL_WARNING);
  // g_pLog->SetConsoleOutputParams(true, nullptr, LOGLEVEL_ERROR);
  // g_pLog->SetFilterLevel(LOGLEVEL_PROFILE);
  // g_pLog->SetDebugOutputParams(true);

#ifdef Y_BUILD_CONFIG_RELEASE
  g_pLog->SetFilterLevel(LOGLEVEL_ERROR);
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

  TestBIOS();

  SDL_Quit();
  return 0;
}
