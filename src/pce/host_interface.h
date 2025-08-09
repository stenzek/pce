#pragma once
#include "YBaseLib/String.h"
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
#include <utility>
#include <vector>

class Error;

class Component;
class System;
class TimingEvent;

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

  HostInterface();
  ~HostInterface();

  // System pointer, can be null.
  System* GetSystem() { return m_system.get(); }

  // Loads/creates a system.
  bool CreateSystem(const char* inifile, Error* error);
  void ShutdownSystem();

  // Load/save state. If load fails, system is in an undefined state, Reset it.
  // This occurs asynchronously, the event maintains a reference to the stream.
  // The stream is committed upon success, or discarded upon fail.
  bool LoadSystemState(const char* filename, Error* error);
  bool SaveSystemState(const char* filename);

  // Safely changes CPU backend.
  // This change is done asynchronously.
  CPU::BackendType GetCPUBackend() const;
  float GetCPUFrequency() const;
  bool SetCPUBackend(CPU::BackendType backend);
  void SetCPUFrequency(float frequency);
  void FlushCPUCodeCache();

  // Error reporting. May block.
  virtual void ReportError(const char* message);
  void ReportFormattedError(const char* format, ...);

  // Status message logging.
  virtual void ReportMessage(const char* message);
  void ReportFormattedMessage(const char* format, ...);

  // UI elements.
  using UICallback = std::function<void()>;
  using UIFileCallback = std::function<void(const String&)>;
  virtual void AddUIIndicator(const Component* component, IndicatorType type);
  virtual void SetUIIndicatorState(const Component* component, IndicatorState state);
  virtual void AddUICallback(const Component* component, const String& label, UICallback callback);
  virtual void AddUIFileCallback(const Component* component, const String& label, UIFileCallback callback);

  // Adds OSD messages, duration is in seconds.
  void AddOSDMessage(const char* message, float duration = 2.0f);

protected:
  struct ComponentUIElement
  {
    const Component* component;
    std::vector<std::pair<String, UICallback>> callbacks;
    std::vector<std::pair<String, std::function<void(const String&)>>> file_callbacks;
    IndicatorType indicator_type = IndicatorType::None;
    IndicatorState indicator_state = IndicatorState::Off;
  };

  struct SimulationStats
  {
    float simulation_speed;
    float host_cpu_usage;
    u64 total_time_simulated;
    u64 delta_time_simulated;
    CPU::ExecutionStats cpu_stats;
    u64 cpu_delta_cycles_executed;
    u64 cpu_delta_instructions_interpreted;
    u64 cpu_delta_exceptions_raised;
    u64 cpu_delata_interrupts_serviced;
    u64 cpu_delta_code_cache_blocks_executed;
    u64 cpu_delta_code_cache_instructions_executed;

    // TODO: Frames
  };

  ComponentUIElement* CreateComponentUIElement(const Component* component);
  ComponentUIElement* GetOrCreateComponentUIElement(const Component* component);
  ComponentUIElement* GetComponentUIElement(const Component* component);

  std::unique_ptr<System> m_system;
  std::vector<ComponentUIElement> m_component_ui_elements;

private:
  void UpdateExecutionSpeed();

  // Stats tracking
  CPU::ExecutionStats m_last_cpu_execution_stats = {};
};

// Retro2 Interfaces
extern struct IRetro2Error R2Error;
extern struct IRetro2Log R2Log;
extern struct IRetro2Session R2Session;
extern struct IRetro2Settings R2Settings;
extern struct IRetro2FramebufferVideo R2FramebufferVideo;
