#include "host_interface.h"
#include "YBaseLib/Log.h"
Log_SetChannel(HostInterface);

bool HostInterface::Initialize(System* system)
{
  m_system = system;
  return true;
}

void HostInterface::Reset() {}

void HostInterface::Cleanup()
{
  // Clear all callbacks.
  m_system = nullptr;
  m_keyboard_callbacks.clear();
  m_mouse_position_change_callbacks.clear();
  m_mouse_button_change_callbacks.clear();
  m_component_ui_elements.clear();
}

void HostInterface::RemoveAllCallbacks(const void* owner)
{
  auto callback = [owner](const auto& it) { return (it.first == owner); };
  std::remove_if(m_keyboard_callbacks.begin(), m_keyboard_callbacks.end(), callback);
  std::remove_if(m_mouse_position_change_callbacks.begin(), m_mouse_position_change_callbacks.end(), callback);
  std::remove_if(m_mouse_button_change_callbacks.begin(), m_mouse_button_change_callbacks.end(), callback);
}

void HostInterface::AddKeyboardCallback(const void* owner, KeyboardCallback callback)
{
  m_keyboard_callbacks.emplace_back(owner, std::move(callback));
}

void HostInterface::AddMousePositionChangeCallback(const void* owner, MousePositionChangeCallback callback)
{
  m_mouse_position_change_callbacks.emplace_back(owner, std::move(callback));
}

void HostInterface::AddMouseButtonChangeCallback(const void* owner, MouseButtonChangeCallback callback)
{
  m_mouse_button_change_callbacks.emplace_back(owner, std::move(callback));
}

void HostInterface::ReportMessage(const char* message)
{
  Log_InfoPrintf("Report message: %s", message);
}

void HostInterface::ReportFormattedMessage(const char* format, ...)
{
  std::va_list ap;
  va_start(ap, format);

  SmallString message;
  message.FormatVA(format, ap);
  va_end(ap);

  ReportMessage(message);
}

void HostInterface::OnSimulationResumed() {}

void HostInterface::OnSimulationPaused() {}

void HostInterface::OnSimulationStopped() {}

void HostInterface::OnSimulationSpeedUpdate(float speed_percent) {}

void HostInterface::AddUIIndicator(const Component* component, IndicatorType type)
{
  ComponentUIElement* ui = GetOrCreateComponentUIElement(component);
  ui->indicator_type = type;
}

void HostInterface::SetUIIndicatorState(const Component* component, IndicatorState state)
{
  ComponentUIElement* ui = GetComponentUIElement(component);
  ui->indicator_state = state;
}

void HostInterface::AddUICallback(const Component* component, const String& label, UICallback callback)
{
  ComponentUIElement* ui = GetOrCreateComponentUIElement(component);
  ui->callbacks.emplace_back(label, std::move(callback));
}

void HostInterface::AddUIFileCallback(const Component* component, const String& label, UIFileCallback callback)
{
  ComponentUIElement* ui = GetOrCreateComponentUIElement(component);
  ui->file_callbacks.emplace_back(label, std::move(callback));
}

void HostInterface::ExecuteKeyboardCallback(GenScanCode scancode, bool key_down) const
{
  Log_DevPrintf("Key scancode %u %s", uint32(scancode), key_down ? "down" : "up");
  for (const auto& it : m_keyboard_callbacks)
    it.second(scancode, key_down);
}

void HostInterface::ExecuteMousePositionChangeCallbacks(int32 dx, int32 dy) const
{
  Log_DevPrintf("Mouse position change: %d %d", dx, dy);
  for (const auto& it : m_mouse_position_change_callbacks)
    it.second(dx, dy);
}

void HostInterface::ExecuteMouseButtonChangeCallbacks(uint32 button, bool state) const
{
  Log_DevPrintf("Mouse button change: %u %s", button, state ? "down" : "up");
  for (const auto& it : m_mouse_button_change_callbacks)
    it.second(button, state);
}

HostInterface::ComponentUIElement* HostInterface::CreateComponentUIElement(const Component* component)
{
  ComponentUIElement ui;
  ui.component = component;
  m_component_ui_elements.push_back(std::move(ui));
  return &m_component_ui_elements.back();
}

HostInterface::ComponentUIElement* HostInterface::GetOrCreateComponentUIElement(const Component* component)
{
  ComponentUIElement* ui = GetComponentUIElement(component);
  if (!ui)
    ui = CreateComponentUIElement(component);

  return ui;
}

HostInterface::ComponentUIElement* HostInterface::GetComponentUIElement(const Component* component)
{
  for (ComponentUIElement& it : m_component_ui_elements)
  {
    if (it.component == component)
      return &it;
  }

  return nullptr;
}
