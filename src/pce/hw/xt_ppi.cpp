#include "pce/hw/xt_ppi.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/String.h"
#include "pce/bus.h"
#include "pce/cpu.h"
#include "pce/host_interface.h"
#include "pce/hw/keyboard_scancodes.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cstring>
Log_SetChannel(HW::XT_PPI);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(XT_PPI);
BEGIN_OBJECT_PROPERTY_MAP(XT_PPI)
END_OBJECT_PROPERTY_MAP()

XT_PPI::XT_PPI(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info)
{
}

XT_PPI::~XT_PPI() = default;

bool XT_PPI::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_interrupt_controller = system->GetComponentByType<InterruptController>();
  if (!m_interrupt_controller)
  {
    Log_ErrorPrintf("Failed to locate interrupt controller");
    return false;
  }

  ConnectIOPorts(bus);

  m_system->GetHostInterface()->AddKeyboardCallback(
    this, std::bind(&XT_PPI::AddScanCode, this, std::placeholders::_1, std::placeholders::_2));

  return true;
}

void XT_PPI::Reset()
{
  if (m_output_buffer_pos > 0)
  {
    Y_memzero(m_output_buffer, m_output_buffer_pos);
    m_output_buffer_pos = 0;
  }

  m_control_register.bits = 0x99;
  m_port_b.bits = 0;

  if (m_speaker_gate_callback)
    m_speaker_gate_callback(false);
  if (m_speaker_enable_callback)
    m_speaker_enable_callback(false);
}

bool XT_PPI::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.SafeReadUInt8(&m_control_register.bits);
  reader.SafeReadUInt8(&m_port_a);
  reader.SafeReadUInt8(&m_port_b.bits);
  reader.SafeReadBytes(m_output_buffer, sizeof(m_output_buffer));
  reader.SafeReadUInt32(&m_output_buffer_pos);
  reader.SafeReadUInt8(&m_pending_command);
  reader.SafeReadUInt8(&m_input_buffer);
  return true;
}

bool XT_PPI::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  writer.SafeWriteUInt8(m_control_register.bits);
  writer.SafeWriteUInt8(m_port_a);
  writer.SafeWriteUInt8(m_port_b.bits);
  writer.SafeWriteBytes(m_output_buffer, sizeof(m_output_buffer));
  writer.SafeWriteUInt32(m_output_buffer_pos);
  writer.SafeWriteUInt8(m_pending_command);
  writer.SafeWriteUInt8(m_input_buffer);
  return true;
}

void XT_PPI::RemoveKeyboardBufferBytes(u32 count)
{
  DebugAssert(count <= m_output_buffer_pos);
  if (count == m_output_buffer_pos)
  {
    // Y_memzero(m_output_buffer, m_output_buffer_pos);
    m_output_buffer_pos = 0;
    return;
  }

  std::memmove(m_output_buffer, m_output_buffer + count, m_output_buffer_pos - count);
  m_output_buffer_pos -= count;

  Y_memzero(m_output_buffer + m_output_buffer_pos, countof(m_output_buffer) - m_output_buffer_pos);
}

void XT_PPI::AddScanCode(GenScanCode scancode, bool key_down)
{
  u8 index = key_down ? 0 : 1;
  DebugAssert(index < 2);

  const u8* hw_scancode = KeyboardScanCodes::Set1Mapping[scancode][index];

  u32 length = 0;
  for (; hw_scancode[length] != 0; length++)
    ;

  if (!AppendToOutputBuffer(hw_scancode, length))
  {
    Log_WarningPrint("Keyboard buffer full");
    return;
  }

  m_interrupt_controller->RaiseInterrupt(KEYBOARD_IRQ);
}

bool XT_PPI::AppendToOutputBuffer(const void* data, u32 length)
{
  if (length == 0)
    return true;

  if ((m_output_buffer_pos + length) >= KEYBOARD_BUFFER_SIZE)
    return false;

  std::memcpy(&m_output_buffer[m_output_buffer_pos], data, length);
  m_output_buffer_pos += length;

  SmallString buffer_str;
  for (u32 i = 0; i < m_output_buffer_pos; i++)
    buffer_str.AppendFormattedString("%02X ", u32(m_output_buffer[i]));
  Log_DebugPrintf("Output buffer contents: %s", buffer_str.GetCharArray());
  return true;
}

bool XT_PPI::AppendToOutputBuffer(u8 data)
{
  return AppendToOutputBuffer(&data, sizeof(data));
}

void XT_PPI::ConnectIOPorts(Bus* bus)
{
  for (u16 i = 0x60; i <= 0x63; i++)
  {
    bus->ConnectIOPortRead(i, this, std::bind(&XT_PPI::IORead, this, std::placeholders::_1));
    bus->ConnectIOPortWrite(i, this, std::bind(&XT_PPI::IOWrite, this, std::placeholders::_1, std::placeholders::_2));
  }
}

u8 XT_PPI::IORead(u32 port)
{
  switch (port)
  {
      // Port A
    case 0x60:
    {
      // Not keyboard data?
      if (!m_control_register.port_a_input)
        return m_port_a;

      // Keyboard data
      Log_DebugPrintf("Read data port, size = %u, value = 0x%02X", m_output_buffer_pos, u32(m_output_buffer[0]));

      u8 value;
      if (m_output_buffer_pos == 0)
      {
        // Buffer is empty and read happened..
        value = m_output_buffer[0];
      }
      else
      {
        value = m_output_buffer[0];
        RemoveKeyboardBufferBytes(1);
      }

      return value;
    }

      // Port B
    case 0x61:
    {
      // Port B can be configured as an input too.
      if (m_control_register.port_b_input)
        return m_port_b.bits;
      else
        return 0;
    }

      // Port C
    case 0x62:
    {
      // Port C is always an output.
      PortCValue out_value;
      out_value.parity_error_occurred = false;
      out_value.io_error_occurred = false;
      out_value.speaker_output = m_speaker_output_callback ? m_speaker_output_callback() : false;
      out_value.cassette_data_in = 0;
      if (m_port_b.enable_high_switches)
      {
        out_value.switch_1 = m_switches[4];
        out_value.switch_2 = m_switches[5];
        out_value.switch_3 = m_switches[6];
        out_value.switch_4 = m_switches[7];
      }
      else
      {
        out_value.switch_1 = m_switches[0];
        out_value.switch_2 = m_switches[1];
        out_value.switch_3 = m_switches[2];
        out_value.switch_4 = m_switches[3];
      }
      return out_value.bits;
    }
    
      // Control Register
    case 0x63:
      return m_control_register.bits;
  }
}

void XT_PPI::IOWrite(u32 port, u8 value)
{
  switch (port)
  {
      // Port A
    case 0x60:
    {
      if (m_control_register.port_a_input)
      {
        // Pass command to keyboard.
        Log_WarningPrintf("To keyboard: 0x%02X", ZeroExtend32(value));
        break;
      }

      Log_WarningPrintf("PPI Port A <- 0x%02X", ZeroExtend32(value));
      m_port_a = value;
    }
    break;

      // Port B
    case 0x61:
    {
      if (m_control_register.port_b_input)
      {
        Log_WarningPrintf("PPI ignored port B write as it is an input <- 0x%02X", ZeroExtend32(value));
        break;
      }

      decltype(m_port_b) changed_bits;
      changed_bits.bits = m_port_b.bits ^ value;
      Log_DebugPrintf("PPI Port B <- 0x%02X", ZeroExtend32(value));
      m_port_b.bits = value;

      // Thing we need to act on:
      if (changed_bits.speaker_gate && m_speaker_gate_callback)
        m_speaker_gate_callback(m_port_b.speaker_gate);
      if (changed_bits.speaker_enable && m_speaker_enable_callback)
        m_speaker_enable_callback(m_port_b.speaker_enable);
      if (m_port_b.clear_keyboard)
      {
        m_interrupt_controller->LowerInterrupt(KEYBOARD_IRQ);

        // If there is still data (more keys?) we re-raise the interrupt.
        if (m_output_buffer_pos > 0)
          m_interrupt_controller->RaiseInterrupt(KEYBOARD_IRQ);
      }

      // Clear bits that aren't used.
      m_port_b.clear_keyboard = false;
    }
    break;

      // Port C. Not used as an output, so drop it.
    case 0x62:
      Log_WarningPrintf("PPI ignored Port C write <- 0x%02X", ZeroExtend32(value));
      break;

      // Control Register
    case 0x63:
      Log_DebugPrintf("PPI Control Register <- 0x%02X", ZeroExtend32(value));
      m_control_register.bits = value;
      break;
  }
}

} // namespace HW
