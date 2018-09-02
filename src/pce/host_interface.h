#pragma once
#include "YBaseLib/String.h"
#include "pce/cpu.h"
#include "pce/scancodes.h"
#include "pce/types.h"
#include <functional>
#include <memory>
#include <vector>

namespace Audio {
class Mixer;
}
class Component;
class Display;

class HostInterface
{
public:
  enum class IndicatorType : u8
  {
    None,
    FDD,
    HDD,
    Serial
  };
  enum class IndicatorState : u8
  {
    Off,
    Reading,
    Writing
  };

  HostInterface() = default;
  virtual ~HostInterface() = default;

  // System pointer, can be null.
  System* GetSystem() const { return m_system; }

  // Initialization, called on system thread.
  virtual bool Initialize(System* system);

  // Reset, called on system thread.
  virtual void Reset();

  // System shutdown, called on system thread.
  virtual void Cleanup();

  // Display
  virtual Display* GetDisplay() const = 0;

  // Audio
  virtual Audio::Mixer* GetAudioMixer() const = 0;

  // Remove all callbacks with this owner
  void RemoveAllCallbacks(const void* owner);

  // Keyboard
  using KeyboardCallback = std::function<void(GenScanCode scancode, bool key_down)>;
  void AddKeyboardCallback(const void* owner, KeyboardCallback callback);

  // Mouse
  using MousePositionChangeCallback = std::function<void(int32 dx, int32 dy)>;
  using MouseButtonChangeCallback = std::function<void(uint32 button, bool state)>;
  void AddMousePositionChangeCallback(const void* owner, MousePositionChangeCallback callback);
  void AddMouseButtonChangeCallback(const void* owner, MouseButtonChangeCallback callback);

  // Status message logging.
  virtual void ReportMessage(const char* message);
  void ReportFormattedMessage(const char* format, ...);

  // State changes.
  virtual void OnSimulationResumed();
  virtual void OnSimulationPaused();
  virtual void OnSimulationStopped();

  // Emulation speed updates.
  virtual void OnSimulationSpeedUpdate(float speed_percent);

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

  void ExecuteKeyboardCallback(GenScanCode scancode, bool key_down) const;
  void ExecuteMousePositionChangeCallbacks(int32 dx, int32 dy) const;
  void ExecuteMouseButtonChangeCallbacks(uint32 button, bool state) const;

  ComponentUIElement* CreateComponentUIElement(const Component* component);
  ComponentUIElement* GetOrCreateComponentUIElement(const Component* component);
  ComponentUIElement* GetComponentUIElement(const Component* component);

  System* m_system;
  std::vector<std::pair<const void*, KeyboardCallback>> m_keyboard_callbacks;
  std::vector<std::pair<const void*, MousePositionChangeCallback>> m_mouse_position_change_callbacks;
  std::vector<std::pair<const void*, MouseButtonChangeCallback>> m_mouse_button_change_callbacks;
  std::vector<ComponentUIElement> m_component_ui_elements;
};
