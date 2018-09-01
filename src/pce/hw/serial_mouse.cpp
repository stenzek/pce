#include "pce/hw/serial_mouse.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Timer.h"
#include "pce/host_interface.h"
#include "pce/hw/serial.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
Log_SetChannel(HW::SerialMouse);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(SerialMouse);
DEFINE_OBJECT_GENERIC_FACTORY(SerialMouse);
BEGIN_OBJECT_PROPERTY_MAP(SerialMouse)
END_OBJECT_PROPERTY_MAP()

SerialMouse::SerialMouse(Serial* serial_port) : m_clock("SerialMouse", UPDATES_PER_SEC), m_serial_port(serial_port) {}

SerialMouse::~SerialMouse()
{
  if (m_system)
    m_system->GetHostInterface()->RemoveAllCallbacks(this);
}

bool SerialMouse::Initialize(System* system, Bus* bus)
{
  m_system = system;
  m_clock.SetManager(system->GetTimingManager());
  m_update_event = m_clock.NewEvent("Update", 1, std::bind(&SerialMouse::SendUpdate, this), false);
  ConnectCallbacks();
  return true;
}

bool SerialMouse::LoadState(BinaryReader& reader)
{
  uint32 serialization_id;
  if (!reader.SafeReadUInt32(&serialization_id) || serialization_id != SERIALIZATION_ID)
    return false;

  reader.SafeReadBool(&m_dtr_active);
  reader.SafeReadBool(&m_active);

  reader.SafeReadInt32(&m_delta_x);
  reader.SafeReadInt32(&m_delta_y);
  for (auto& button : m_button_states)
    reader.SafeReadBool(&button);
  reader.SafeReadBool(&m_update_pending);

  if (m_active != m_update_event->IsActive())
    m_active ? m_update_event->Queue(1) : m_update_event->Deactivate();

  return !reader.GetErrorState();
}

bool SerialMouse::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  writer.WriteBool(m_dtr_active);
  writer.WriteBool(m_active);

  writer.WriteInt32(m_delta_x);
  writer.WriteInt32(m_delta_y);
  for (const auto& button : m_button_states)
    writer.WriteBool(button);
  writer.WriteBool(m_update_pending);

  return true;
}

void SerialMouse::Reset()
{
  m_button_states.fill(false);
  m_dtr_active = false;
  m_active = false;
  m_update_pending = false;
  m_delta_x = 0;
  m_delta_y = 0;

  if (m_update_event->IsActive())
    m_update_event->Deactivate();
}

void SerialMouse::ConnectCallbacks()
{
  m_system->GetHostInterface()->AddMousePositionChangeCallback(
    this, std::bind(&SerialMouse::OnHostMousePositionChanged, this, std::placeholders::_1, std::placeholders::_2));
  m_system->GetHostInterface()->AddMouseButtonChangeCallback(
    this, std::bind(&SerialMouse::OnHostMouseButtonChanged, this, std::placeholders::_1, std::placeholders::_2));
  m_serial_port->SetDataTerminalReadyChangedCallback(
    std::bind(&SerialMouse::OnSerialPortDTRChanged, this, std::placeholders::_1));
  m_serial_port->SetRequestToSendChangedCallback(
    std::bind(&SerialMouse::OnSerialPortRTSChanged, this, std::placeholders::_1));
}

void SerialMouse::OnHostMousePositionChanged(int32 dx, int32 dy)
{
  //     // Cap the delta at 1.
  //     if (std::abs(dx) > 1)
  //         dx = (dx < 0) ? -1 : 1;
  //     if (std::abs(dy) > 1)
  //         dy = (dy < 0) ? -1 : 1;

  m_delta_x += dx;
  m_delta_y += dy;
  if (m_delta_x != 0 || m_delta_y != 0)
    m_update_pending = true;
}

void SerialMouse::OnHostMouseButtonChanged(uint32 button, bool down)
{
  if (button > NUM_BUTTONS)
    return;

  m_button_states[button] = down;
  m_update_pending = true;
}

void SerialMouse::OnSerialPortDTRChanged(bool high)
{
  m_dtr_active = high;

  // If it's going high (power source), reset and send initialization reply.
  if (high)
  {
    Log_DevPrintf("Serial mouse reset");
    m_delta_x = 0;
    m_delta_y = 0;
    m_update_pending = false;

    // TODO: This should have a 14ms delay, but we can probably get away with no delay.
    // m_update_event->Queue(2);
    uint8 data = 0x4D;
    m_serial_port->WriteData(&data, sizeof(data));
    m_active = true;
    m_update_event->Queue(1);
  }
  else
  {
    m_active = false;
    m_update_pending = false;
    m_serial_port->SetClearToSend(false);
    if (m_update_event->IsActive())
      m_update_event->Deactivate();
  }
}

void SerialMouse::SendUpdate()
{
  // If we just became active, send the initialization packet.
  if (!m_active)
  {
    uint8 data = 0x4D;
    m_serial_port->WriteData(&data, sizeof(data));
    m_active = true;
    m_update_event->Reschedule(1);
    return;
  }

  // Don't send updates when DTR is not high (in reality the mouse would not have power)
  if (!m_update_pending)
    return;

  // Cap the dx/dy at (-128, 127)
  uint8 packet_dx = uint8(int8(std::max(-128, std::min(m_delta_x, 127))));
  uint8 packet_dy = uint8(int8(std::max(-128, std::min(m_delta_y, 127))));
  m_delta_x = 0;
  m_delta_y = 0;

  // Build packet
  // From https://linux.die.net/man/4/mouse
  // byte 	d6 	d5 	d4 	d3 	d2 	d1 	d0
  // 1 	1 	lb 	rb 	dy7 	dy6 	dx7 	dx6
  // 2 	0 	dx5 	dx4 	dx3 	dx2 	dx1 	dx0
  // 3 	0 	dy5 	dy4 	dy3 	dy2 	dy1 	dy0
  std::array<uint8, 3> packet;
  packet[0] = 0x40 | (uint8(m_button_states[0]) << 5) | (uint8(m_button_states[1]) << 4) | ((packet_dy & 0xC0) >> 4) |
              ((packet_dx & 0xC0) >> 6);
  packet[1] = (packet_dx & 0x3F);
  packet[2] = (packet_dy & 0x3F);
  if (!m_serial_port->WriteData(packet.data(), packet.size()))
    Log_WarningPrintf("Failed to write mouse update packet to serial port");

  m_update_pending = false;
}

void SerialMouse::OnSerialPortRTSChanged(bool high)
{
  Log_DevPrintf("Serial mouse RTS");
  m_serial_port->SetClearToSend(high);

  // No clue if this is correct, Windows seems to expect it though..
  uint8 data = 0x4D;
  m_serial_port->WriteData(&data, sizeof(data));
}

} // namespace HW