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
class Display;

class HostInterface
{
public:
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

protected:
  void ExecuteKeyboardCallback(GenScanCode scancode, bool key_down) const;
  void ExecuteMousePositionChangeCallbacks(int32 dx, int32 dy) const;
  void ExecuteMouseButtonChangeCallbacks(uint32 button, bool state) const;

  System* m_system;
  std::vector<std::pair<const void*, KeyboardCallback>> m_keyboard_callbacks;
  std::vector<std::pair<const void*, MousePositionChangeCallback>> m_mouse_position_change_callbacks;
  std::vector<std::pair<const void*, MouseButtonChangeCallback>> m_mouse_button_change_callbacks;
};
