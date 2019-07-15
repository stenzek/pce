#pragma once
#include "common/display_renderer.h"
#include "pce-sdl/audio_sdl.h"
#include "pce-sdl/scancodes_sdl.h"
#include "pce/host_interface.h"
#include <array>
#include <mutex>

struct SDL_Window;
union SDL_Event;

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

  bool CreateSystem(const char* filename, s32 save_state_index = -1);

  DisplayRenderer* GetDisplayRenderer() const override { return m_display_renderer.get(); }
  Audio::Mixer* GetAudioMixer() const override { return m_mixer.get(); }
  // bool NeedsRender() const { return m_display->NeedsRender(); }

  void ReportMessage(const char* message) override;

  void Run();

protected:
  static const u32 NUM_STATS_HISTORY_VALUES = 128;
  
  void OnSimulationStatsUpdate(const SimulationStats& stats) override;

  // We only pass mouse input through if it's grabbed
  bool IsWindowFullscreen() const;
  bool IsMouseGrabbed() const;
  void GrabMouse();
  void ReleaseMouse();
  void RenderImGui();
  void DoLoadState(u32 index);
  void DoSaveState(u32 index);

  bool HandleSDLEvent(const SDL_Event* event);
  bool PassEventToImGui(const SDL_Event* event);
  void Render();
  void RenderMainMenuBar();
  void RenderActivityWindow();
  void RenderStatsWindow();
  void RenderOSDMessages();

  SDL_Window* m_window;

  std::unique_ptr<DisplayRenderer> m_display_renderer;
  std::unique_ptr<MixerType> m_mixer;
  std::thread m_simulation_thread;

  bool m_running = false;
  bool m_show_stats = false;

  std::mutex m_stats_mutex;
  struct StatsAndHistory
  {
    SimulationStats last_stats = {};
    std::array<float, NUM_STATS_HISTORY_VALUES> simulation_speed_history = {};
    std::array<float, NUM_STATS_HISTORY_VALUES> host_cpu_usage_history = {};
    std::array<float, NUM_STATS_HISTORY_VALUES> instructions_executed_history = {};
    std::array<float, NUM_STATS_HISTORY_VALUES> interrupts_serviced_history = {};
    std::array<float, NUM_STATS_HISTORY_VALUES> exceptions_raised_history = {};
    u32 history_position = 0;
  } m_stats;

};
