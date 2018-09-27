#pragma once
#include "YBaseLib/Barrier.h"
#include "YBaseLib/Event.h"
#include "YBaseLib/Semaphore.h"
#include "YBaseLib/String.h"
#include "YBaseLib/TaskQueue.h"
#include "YBaseLib/Timer.h"
#include "cpu.h"
#include "scancodes.h"
#include "system.h"
#include "types.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class Error;

namespace Audio {
class Mixer;
}
class Component;
class Display;
class System;

class HostInterface
{
public:
  enum class IndicatorType : u8
  {
    None,
    FDD,
    HDD,
    CDROM,
    Serial
  };
  enum class IndicatorState : u8
  {
    Off,
    Reading,
    Writing
  };

  using ExternalEventCallback = std::function<void()>;

  HostInterface();
  virtual ~HostInterface();

  // System pointer, can be null.
  System* GetSystem() { return m_system.get(); }

  // Loads/creates a system.
  bool CreateSystem(const char* inifile, Error* error);

  // Resets the system.
  void ResetSystem();

  // Load/save state. If load fails, system is in an undefined state, Reset it.
  // This occurs asynchronously, the event maintains a reference to the stream.
  // The stream is committed upon success, or discarded upon fail.
  bool LoadSystemState(const char* filename, Error* error);
  void SaveSystemState(const char* filename);

  // External events, will interrupt the CPU and execute.
  // Use care when calling this variant, deadlocks can occur.
  void QueueExternalEvent(ExternalEventCallback callback, bool wait);

  // Safely changes CPU backend.
  // This change is done asynchronously.
  CPU::BackendType GetCPUBackend() const;
  float GetCPUFrequency() const;
  bool SetCPUBackend(CPU::BackendType backend);
  void SetCPUFrequency(float frequency);
  void FlushCPUCodeCache();

  // Speed limiter.
  bool IsSpeedLimiterEnabled() const { return m_speed_limiter_enabled; }
  void SetSpeedLimiterEnabled(bool enabled);

  // Simulation pausing/resuming/stopping.
  void PauseSimulation();
  void ResumeSimulation();
  void StopSimulation();

  // Display
  virtual Display* GetDisplay() const = 0;

  // Audio
  virtual Audio::Mixer* GetAudioMixer() const = 0;

  // Remove all callbacks with this owner
  void RemoveAllCallbacks(const void* owner);

  // Keyboard
  using KeyboardCallback = std::function<void(GenScanCode scancode, bool key_down)>;
  void AddKeyboardCallback(const void* owner, KeyboardCallback callback);
  void InjectKeyEvent(GenScanCode sc, bool down);

  // Mouse
  using MousePositionChangeCallback = std::function<void(int32 dx, int32 dy)>;
  using MouseButtonChangeCallback = std::function<void(uint32 button, bool state)>;
  void AddMousePositionChangeCallback(const void* owner, MousePositionChangeCallback callback);
  void AddMouseButtonChangeCallback(const void* owner, MouseButtonChangeCallback callback);

  // Error reporting. May block.
  virtual void ReportError(const char* message);
  void ReportFormattedError(const char* format, ...);

  // Status message logging.
  virtual void ReportMessage(const char* message);
  void ReportFormattedMessage(const char* format, ...);

  // Helper to check if the caller is on the simulation thread.
  bool IsOnSimulationThread() const;

  // Sends CTRL+ALT+DELETE to the simulated machine.
  void SendCtrlAltDel();

  // UI elements.
  using UICallback = std::function<void()>;
  using UIFileCallback = std::function<void(const String&)>;
  virtual void AddUIIndicator(const Component* component, IndicatorType type);
  virtual void SetUIIndicatorState(const Component* component, IndicatorState state);
  virtual void AddUICallback(const Component* component, const String& label, UICallback callback);
  virtual void AddUIFileCallback(const Component* component, const String& label, UIFileCallback callback);

protected:
  struct ComponentUIElement
  {
    const Component* component;
    std::vector<std::pair<String, UICallback>> callbacks;
    std::vector<std::pair<String, std::function<void(const String&)>>> file_callbacks;
    IndicatorType indicator_type = IndicatorType::None;
    IndicatorState indicator_state = IndicatorState::Off;
  };

  // Implemented in derived classes.
  virtual void OnSystemInitialized();
  virtual void OnSystemReset();
  virtual void OnSystemDestroy();
  virtual void OnSimulationSpeedUpdate(float speed_percent);
  virtual void OnSimulationResumed();
  virtual void OnSimulationPaused();

  // Yields execution, so that the main thread doesn't deadlock.
  virtual void YieldToUI();

  void ExecuteKeyboardCallbacks(GenScanCode scancode, bool key_down);
  void ExecuteMousePositionChangeCallbacks(int32 dx, int32 dy);
  void ExecuteMouseButtonChangeCallbacks(uint32 button, bool state);

  // Simulation thread entry point.
  void SimulationThreadRoutine();
  void WaitForSimulationThread();
  void StopSimulationThread();

  ComponentUIElement* CreateComponentUIElement(const Component* component);
  ComponentUIElement* GetOrCreateComponentUIElement(const Component* component);
  ComponentUIElement* GetComponentUIElement(const Component* component);

private:
  SimulationTime GetSimulationSliceTime() const;
  SimulationTime GetMaxSimulationSliceTime() const;
  SimulationTime GetMaxSimulationVarianceTime() const;
  void HandleStateChange();
  bool ExecuteExternalEvents();
  void ExecuteSlice();
  void UpdateExecutionSpeed();
  void WaitForCallingThread();
  void ShutdownSystem();

protected:
  std::unique_ptr<System> m_system;
  std::vector<ComponentUIElement> m_component_ui_elements;

private:
  std::vector<std::pair<const void*, KeyboardCallback>> m_keyboard_callbacks;
  std::vector<std::pair<const void*, MousePositionChangeCallback>> m_mouse_position_change_callbacks;
  std::vector<std::pair<const void*, MouseButtonChangeCallback>> m_mouse_button_change_callbacks;

  // Throttle event
  Timer m_elapsed_real_time;
  SimulationTime m_pending_execution_time = 0;
  bool m_speed_limiter_enabled = true;

  // Emulation speed tracking
  Timer m_speed_elapsed_real_time;
  SimulationTime m_speed_elapsed_simulation_time = 0;
  u64 m_speed_elapsed_user_time = 0;
  u64 m_speed_elapsed_kernel_time = 0;

  // Threaded running state
  std::thread::id m_simulation_thread_id;
  Barrier m_simulation_thread_barrier{2};
  Semaphore m_simulation_thread_semaphore;
  std::atomic_bool m_simulation_thread_running{true};
  System::State m_last_system_state = System::State::Stopped;

  // External event queue
  std::queue<std::pair<ExternalEventCallback, bool>> m_external_events;
  std::mutex m_external_events_lock;
};
