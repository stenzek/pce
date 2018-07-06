#pragma once

#include "pce/bitfield.h"
#include "pce/clock.h"
#include "pce/component.h"
#include "pce/system.h"
#include <array>

namespace HW {

class Serial;

class SerialMouse final : public Component
{
public:
  SerialMouse(Serial* serial_port);
  ~SerialMouse();

  bool Initialize(System* system, Bus* bus) override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;
  void Reset() override;

private:
  static constexpr uint32 SERIALIZATION_ID = MakeSerializationID('S', 'E', 'M', 'S');
  static constexpr size_t NUM_BUTTONS = 2;
  static constexpr float UPDATES_PER_SEC = 50;

  void ConnectCallbacks();
  void OnHostMousePositionChanged(int32 dx, int32 dy);
  void OnHostMouseButtonChanged(uint32 button, bool down);
  void OnSerialPortDTRChanged(bool high);
  void OnSerialPortRTSChanged(bool high);

  void SendUpdate();

  Clock m_clock;

  System* m_system = nullptr;
  Serial* m_serial_port;
  bool m_dtr_active = false;
  bool m_active = false;

  int32 m_delta_x = 0;
  int32 m_delta_y = 0;
  std::array<bool, NUM_BUTTONS> m_button_states = {};
  bool m_update_pending = false;

  TimingEvent::Pointer m_update_event;
};

} // namespace HW