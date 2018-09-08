#include "pce/host_interface.h"
#include "pce-sdl/audio_sdl.h"
#include "pce-sdl/display_d3d.h"
#include "pce-sdl/display_gl.h"
#include "pce-sdl/display_sdl.h"
#include "pce-sdl/scancodes_sdl.h"

class SDLHostInterface : public HostInterface
{
public:
  // using DisplayType = DisplayGL;
  using DisplayType = DisplayD3D;
  using MixerType = Mixer_SDL;
  // using MixerType = Audio::NullMixer;

  SDLHostInterface(std::unique_ptr<DisplayType> display, std::unique_ptr<MixerType> mixer);
  ~SDLHostInterface();

  static std::unique_ptr<SDLHostInterface> Create();
  static TinyString GetSaveStateFilename(u32 index);

  bool CreateSystem(const char* filename, s32 save_state_index = -1);

  Display* GetDisplay() const override { return m_display.get(); }
  Audio::Mixer* GetAudioMixer() const override { return m_mixer.get(); }
  // bool NeedsRender() const { return m_display->NeedsRender(); }

  void ReportMessage(const char* message) override;

  void Run();

protected:
  void OnSimulationSpeedUpdate(float speed_percent) override;

  // We only pass mouse input through if it's grabbed
  bool IsMouseGrabbed() const;
  void GrabMouse();
  void ReleaseMouse();
  void RenderImGui();
  void DoLoadState(uint32 index);
  void DoSaveState(uint32 index);

  bool HandleSDLEvent(const SDL_Event* ev);
  void Render();

  std::unique_ptr<DisplayType> m_display;
  std::unique_ptr<MixerType> m_mixer;
  std::thread m_simulation_thread;

  String m_last_message;
  Timer m_last_message_time;

  bool m_running = false;
};
