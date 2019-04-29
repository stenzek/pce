#include "common/display_renderer.h"
#include "pce-sdl/audio_sdl.h"
#include "pce-sdl/scancodes_sdl.h"
#include "pce/host_interface.h"

struct SDL_Window;
union SDL_Event;

namespace CPUDebugger
{
class CPUDebugger;
}

class SDLHostInterface : public HostInterface
{
public:
  using MixerType = Mixer_SDL;
  // using MixerType = Audio::NullMixer;

  SDLHostInterface(SDL_Window* window, std::unique_ptr<DisplayRenderer> display_renderer,
                   std::unique_ptr<MixerType> mixer);
  ~SDLHostInterface();

  static std::unique_ptr<SDLHostInterface>
  Create(DisplayRenderer::BackendType display_renderer_backend = DisplayRenderer::GetDefaultBackendType());

  static TinyString GetSaveStateFilename(u32 index);

  bool CreateSystem(const char* filename, bool start_paused, s32 save_state_index = -1);

  DisplayRenderer* GetDisplayRenderer() const override { return m_display_renderer.get(); }
  Audio::Mixer* GetAudioMixer() const override { return m_mixer.get(); }
  // bool NeedsRender() const { return m_display->NeedsRender(); }

  void ReportMessage(const char* message) override;

  void Run();

protected:
  void OnSimulationSpeedUpdate(float speed_percent) override;

  // We only pass mouse input through if it's grabbed
  bool IsWindowFullscreen() const;
  bool IsMouseGrabbed() const;
  void GrabMouse();
  void ReleaseMouse();
  void RenderImGui();
  void DoLoadState(u32 index);
  void DoSaveState(u32 index);
  void DoEnableDebugger(bool enabled);

  bool HandleSDLEvent(const SDL_Event* event);
  bool PassEventToImGui(const SDL_Event* event);
  void Render();

  SDL_Window* m_window;

  std::unique_ptr<DisplayRenderer> m_display_renderer;
  std::unique_ptr<MixerType> m_mixer;
  std::thread m_simulation_thread;

  String m_last_message;
  Timer m_last_message_time;

  bool m_running = false;

  std::unique_ptr<CPUDebugger::CPUDebugger> m_cpu_debugger;
};
