#pragma once
#include "../component.h"
#include "../system.h"
#include <array>
#include <memory>

class TimingEvent;

namespace HW {

class Serial;

class SerialMouse final : public Component
{
  DECLARE_OBJECT_TYPE_INFO(SerialMouse, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(SerialMouse);
  DECLARE_OBJECT_PROPERTY_MAP(SerialMouse);

public:
  SerialMouse(const String& identifier, const String& serial_port_name = "COM1",
              const ObjectTypeInfo* type_info = &s_type_info);
  ~SerialMouse();

  const Serial* GetSerialPort() const { return m_serial_port; }
  const String& GetSerialPortName() const { return m_serial_port_name; }
  void SetSerialPortName(const String& name) { m_serial_port_name = name; }

  bool Initialize(System* system, Bus* bus) override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;
  void Reset() override;

private:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('S', 'E', 'M', 'S');
  static constexpr size_t NUM_BUTTONS = 2;
  static constexpr float UPDATES_PER_SEC = 50;

  void ConnectCallbacks();
  void OnHostMousePositionChanged(s32 dx, s32 dy);
  void OnHostMouseButtonChanged(u32 button, bool down);
  void OnSerialPortDTRChanged(bool high);
  void OnSerialPortRTSChanged(bool high);

  void SendUpdate();

  String m_serial_port_name;
  Serial* m_serial_port = nullptr;

  bool m_dtr_active = false;
  bool m_active = false;

  s32 m_delta_x = 0;
  s32 m_delta_y = 0;
  std::array<bool, NUM_BUTTONS> m_button_states = {};
  bool m_update_pending = false;

  std::unique_ptr<TimingEvent> m_update_event;
};

} // namespace HW