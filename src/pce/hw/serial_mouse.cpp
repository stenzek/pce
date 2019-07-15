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
DEFINE_GENERIC_COMPONENT_FACTORY(SerialMouse);
BEGIN_OBJECT_PROPERTY_MAP(SerialMouse)
PROPERTY_TABLE_MEMBER_STRING("SerialPortName", 0, offsetof(SerialMouse, m_serial_port_name), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

SerialMouse::SerialMouse(const String& identifier, const String& serial_port_name /* = "COM1" */,
                         const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_serial_port_name(serial_port_name)
{
}

SerialMouse::~SerialMouse()
{
  if (m_system)
    m_system->GetHostInterface()->RemoveAllCallbacks(this);
}

bool SerialMouse::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_update_event = m_system->CreateFrequencyEvent("Serial Mouse Update", UPDATES_PER_SEC,
                                                  std::bind(&SerialMouse::SendUpdate, this), false);

  // Find the serial port.
  m_serial_port = system->GetComponentByIdentifier<Serial>(m_serial_port_name);
  if (!m_serial_port)
  {
    Log_ErrorPrintf("Failed to find serial port '%s' for serial mouse '%s'", m_serial_port_name.GetCharArray(),
                    m_identifier.GetCharArray());
    return false;
  }

  ConnectCallbacks();
  return true;
}

bool SerialMouse::LoadState(BinaryReader& reader)
{
  u32 serialization_id;
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

  Log_InfoPrintf("Attached serial mouse '%s' to serial port '%s'", m_identifier.GetCharArray(),
                 m_serial_port->GetIdentifier().GetCharArray());
}

void SerialMouse::OnHostMousePositionChanged(s32 dx, s32 dy)
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

void SerialMouse::OnHostMouseButtonChanged(u32 button, bool down)
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
    u8 data = 0x4D;
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
    u8 data = 0x4D;
    m_serial_port->WriteData(&data, sizeof(data));
    m_active = true;
    m_update_event->Reschedule(1);
    return;
  }

  // Don't send updates when DTR is not high (in reality the mouse would not have power)
  if (!m_update_pending)
    return;

  // Cap the dx/dy at (-128, 127)
  u8 packet_dx = u8(s8(std::max(-128, std::min(m_delta_x, 127))));
  u8 packet_dy = u8(s8(std::max(-128, std::min(m_delta_y, 127))));
  m_delta_x = 0;
  m_delta_y = 0;

  // Build packet
  // From https://linux.die.net/man/4/mouse
  // byte 	d6 	d5 	d4 	d3 	d2 	d1 	d0
  // 1 	1 	lb 	rb 	dy7 	dy6 	dx7 	dx6
  // 2 	0 	dx5 	dx4 	dx3 	dx2 	dx1 	dx0
  // 3 	0 	dy5 	dy4 	dy3 	dy2 	dy1 	dy0
  std::array<u8, 3> packet;
  packet[0] = 0x40 | (u8(m_button_states[0]) << 5) | (u8(m_button_states[1]) << 4) | ((packet_dy & 0xC0) >> 4) |
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
  u8 data = 0x4D;
  m_serial_port->WriteData(&data, sizeof(data));
}

} // namespace HW