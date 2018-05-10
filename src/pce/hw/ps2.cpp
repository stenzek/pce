#include "pce/hw/ps2.h"
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
Log_SetChannel(HW::PS2Controller);

namespace HW {

PS2Controller::PS2Controller() {}

PS2Controller::~PS2Controller() {}

void PS2Controller::Initialize(System* system, Bus* bus)
{
  m_system = system;
  ConnectIOPorts(bus);

  m_system->GetHostInterface()->AddKeyboardCallback(
    this, std::bind(&PS2Controller::AddScanCode, this, std::placeholders::_1, std::placeholders::_2));
}

void PS2Controller::Reset()
{
  if (m_output_buffer_pos > 0)
  {
    Y_memzero(m_output_buffer, m_output_buffer_pos);
    m_output_buffer_pos = 0;
  }

  m_status_register.output_buffer_status = false;
  m_status_register.input_buffer_status = false;
  m_status_register.self_test_flag = false;
  m_status_register.command_flag = true;
  m_status_register.keyboard_unlocked = true;
  m_status_register.mouse_buffer_status = false;
  m_status_register.timeout_error = false;
  m_status_register.parity_error = false;

  // Enable translation by default. This should be put in firmware instead..
  m_configuration_byte.port_1_interrupt = true;
  m_configuration_byte.port_1_clock = true;
  m_configuration_byte.port_1_translation = true;

  m_output_port.raw = 0;
  m_output_port.keyboard_clock_output = true;
  m_output_port.keyboard_data_output = true;
  m_output_port.input_buffer_empty = true;
  m_output_port.output_buffer_full = false;

  // This should be set after boot when some time has elapsed?
  m_status_register.self_test_flag = true;
}

bool PS2Controller::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.SafeReadUInt8(&m_status_register.raw);
  reader.SafeReadUInt8(&m_configuration_byte.raw);
  reader.SafeReadUInt8(&m_output_port.raw);
  reader.SafeReadBytes(m_output_buffer, sizeof(m_output_buffer));
  reader.SafeReadUInt32(&m_output_buffer_pos);
  reader.SafeReadUInt8(&m_pending_command);
  reader.SafeReadUInt8(&m_input_buffer);
  return true;
}

bool PS2Controller::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  writer.SafeWriteUInt8(m_status_register.raw);
  writer.SafeWriteUInt8(m_configuration_byte.raw);
  writer.SafeWriteUInt8(m_output_port.raw);
  writer.SafeWriteBytes(m_output_buffer, sizeof(m_output_buffer));
  writer.SafeWriteUInt32(m_output_buffer_pos);
  writer.SafeWriteUInt8(m_pending_command);
  writer.SafeWriteUInt8(m_input_buffer);
  return true;
}

void PS2Controller::RemoveKeyboardBufferBytes(uint32 count)
{
  DebugAssert(count <= m_output_buffer_pos);
  if (count == m_output_buffer_pos)
  {
    // Y_memzero(m_output_buffer, m_output_buffer_pos);
    m_output_buffer_pos = 0;
    m_status_register.output_buffer_status = false;
    m_output_port.output_buffer_full = false;
    return;
  }

  std::memmove(m_output_buffer, m_output_buffer + count, m_output_buffer_pos - count);
  m_output_buffer_pos -= count;

  Y_memzero(m_output_buffer + m_output_buffer_pos, countof(m_output_buffer) - m_output_buffer_pos);
  m_status_register.output_buffer_status = (m_output_buffer_pos != 0);
  m_output_port.output_buffer_full = (m_output_buffer_pos != 0);
}

void PS2Controller::LowerKeyboardInterrupt(bool force_when_full /*= false*/)
{
  if (m_output_buffer_pos == 0 || force_when_full)
    m_system->GetInterruptController()->LowerInterrupt(KEYBOARD_IRQ);
}

void PS2Controller::AddScanCode(GenScanCode scancode, bool key_down)
{
  uint8 index = key_down ? 0 : 1;
  DebugAssert(index < 2);

  const uint8* hw_scancode;
  if (m_configuration_byte.port_1_translation)
    hw_scancode = KeyboardScanCodes::Set1Mapping[scancode][index];
  else
    hw_scancode = KeyboardScanCodes::Set2Mapping[scancode][index];

  uint32 length = 0;
  for (; hw_scancode[length] != 0; length++)
    ;

  if (!AppendToOutputBuffer(hw_scancode, length))
  {
    Log_WarningPrint("Keyboard buffer full");
    return;
  }

  if (m_configuration_byte.port_1_interrupt)
    m_system->GetInterruptController()->TriggerInterrupt(KEYBOARD_IRQ);
}

bool PS2Controller::AppendToOutputBuffer(const void* data, uint32 length)
{
  if (length == 0)
    return true;

  if ((m_output_buffer_pos + length) >= KEYBOARD_BUFFER_SIZE)
    return false;

  std::memcpy(&m_output_buffer[m_output_buffer_pos], data, length);
  m_output_buffer_pos += length;
  m_status_register.output_buffer_status = true;
  m_output_port.output_buffer_full = true;

  SmallString buffer_str;
  for (uint32 i = 0; i < m_output_buffer_pos; i++)
    buffer_str.AppendFormattedString("%02X ", uint32(m_output_buffer[i]));
  Log_DevPrintf("Output buffer contents: %s", buffer_str.GetCharArray());
  return true;
}

bool PS2Controller::AppendToOutputBuffer(uint8 data)
{
  return AppendToOutputBuffer(&data, sizeof(data));
}

void PS2Controller::ConnectIOPorts(Bus* bus)
{
  bus->ConnectIOPortRead(0x60, this, std::bind(&PS2Controller::IOReadDataPort, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x60, this, std::bind(&PS2Controller::IOWriteDataPort, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x64, this, std::bind(&PS2Controller::IOReadStatusRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x64, this, std::bind(&PS2Controller::IOWriteCommandRegister, this, std::placeholders::_2));
}

void PS2Controller::IOReadStatusRegister(uint8* value)
{
  *value = m_status_register.raw;

  // TODO: Should be based on time
  m_status_register.input_buffer_status = false;
}

void PS2Controller::IOReadDataPort(uint8* value)
{
  Log_TracePrintf("Read data port, size = %u, value = 0x%02X", m_output_buffer_pos, uint32(m_output_buffer[0]));

  if (m_output_buffer_pos == 0)
  {
    //*value = 0;
    *value = m_output_buffer[0];
    return;
  }

  *value = m_output_buffer[0];
  RemoveKeyboardBufferBytes(1);

  // Re-trigger IRQ if there are still bytes in the buffer
  if (m_output_buffer_pos > 0)
    m_system->GetInterruptController()->TriggerInterrupt(KEYBOARD_IRQ);
}

void PS2Controller::IOWriteDataPort(uint8 value)
{
  Log_TracePrintf("Write data port, pending command = 0x%02X, value = 0x%02X", uint32(m_pending_command), value);
  m_input_buffer = value;
  m_status_register.input_buffer_status = true;

  if (m_pending_command != 0)
    HandleControllerCommand(true);
  else
    HandleKeyboardCommand(value);

  // eat the input buffer
  // m_status_register.input_buffer_status = false;
  m_status_register.command_flag = false;
  m_input_buffer = 0;
}

void PS2Controller::IOWriteCommandRegister(uint8 value)
{
  Log_TracePrintf("Write command register, value = 0x%02X", value);
  m_status_register.input_buffer_status = true;
  m_pending_command = value;
  m_status_register.command_flag = true;
  HandleControllerCommand(false);

  // m_status_register.input_buffer_status = false;
}

void PS2Controller::HandleControllerCommand(bool has_parameter)
{
  switch (m_pending_command)
  {
    case 0x20: // Read configuration byte
    {
      Log_DevPrintf("KBC read config byte");
      AppendToOutputBuffer(m_configuration_byte.raw);
    }
    break;

    case 0x60: // Write configuration byte
    {
      if (!has_parameter)
        return;

      Log_DevPrintf("KBC configuration byte = 0x%02X", m_input_buffer);
      m_configuration_byte.raw = m_input_buffer;
    }
    break;

    case 0xA1: // Get controller version
    {
      AppendToOutputBuffer(0x30);
    }
    break;

    case 0xAA: // Test controller
    {
      Log_DevPrintf("KBC test controller");
      m_status_register.self_test_flag = true;
      AppendToOutputBuffer(0x55); // test passed
    }
    break;

    case 0xAB: // Keyboard interface test
    {
      Log_DevPrintf("KBC test interface");
      AppendToOutputBuffer(0x00);
    }
    break;

    case 0xAD: // Disable keyboard interface
    {
      Log_DevPrintf("KBC disable keyboard");
      m_configuration_byte.port_1_interrupt = false;
      m_system->GetInterruptController()->LowerInterrupt(KEYBOARD_IRQ);
    }
    break;

    case 0xAE: // Enable keyboard interface
    {
      Log_DevPrintf("KBC enable keyboard");
      m_configuration_byte.port_1_interrupt = true;

      // Raise IRQ if there is still stuff in the buffer left-over
      if (m_output_buffer_pos > 0)
        m_system->GetInterruptController()->TriggerInterrupt(KEYBOARD_IRQ);
    }
    break;

    case 0xC0: // Read input port
    {
      Log_DevPrintf("KBC read input port = 0x%02X", ZeroExtend32(m_input_port));
      AppendToOutputBuffer(m_input_port);
    }
    break;

    case 0xD0: // Read output port
    {
      Log_DevPrintf("KBC read output port");
      AppendToOutputBuffer(m_output_port.raw);
    }
    break;

    case 0xD1: // Write output port
    {
      if (!has_parameter)
        return;

      Log_DevPrintf("KBC write output port = 0x%02X", ZeroExtend32(m_input_buffer));
      m_output_port.raw = m_input_buffer;

      // This can control the A20 gate as a legacy method, so pass it to the system.
      if (m_output_port_written_callback)
        m_output_port_written_callback(m_output_port.raw);
    }
    break;

      // Unknown, sent by AMI 386 bios.
    case 0xC9:
      Log_WarningPrintf("Unknown 0xC9 command");
      AppendToOutputBuffer(0x00);
      break;

      // Unknown, sent by AMI 486 bios.
    case 0xB4:
      Log_WarningPrintf("Unknown 0xB4 command");
      AppendToOutputBuffer(0x00);
      break;

      // These are actually pulsing the output lines.
    case 0xF1:
    case 0xF2:
    case 0xF3:
    case 0xF4:
    case 0xF5:
    case 0xF6:
    case 0xF7:
    case 0xF8:
    case 0xF9:
    case 0xFA:
    case 0xFB:
    case 0xFC:
    case 0xFD:
    case 0xFE:
    case 0xFF:
    {
      if (m_output_port_written_callback)
      {
        uint8 mask = (m_pending_command & 0x0F);
        uint8 temp_value = m_output_port.raw ^ mask;
        m_output_port_written_callback(temp_value);
        m_output_port_written_callback(m_output_port.raw);
      }
    }
    break;

    default:
      Log_WarningPrintf("Unknown command: 0x%02X 0x%02X", uint32(m_pending_command), uint32(m_input_buffer));
      break;
  }

  m_pending_command = 0;
}

void PS2Controller::HandleKeyboardCommand(uint8 value)
{
  uint8 command;
  bool has_parameter;
  uint8 parameter;
  if (m_pending_keyboard_command != 0)
  {
    command = m_pending_keyboard_command;
    has_parameter = true;
    parameter = value;
  }
  else
  {
    command = value;
    has_parameter = false;
    parameter = 0;
  }

  switch (command)
  {
    case 0xF0: // Set scan code set
    {
      AppendToOutputBuffer(0xFA); // ACK
      if (!has_parameter)
      {
        m_pending_keyboard_command = command;
        return;
      }

      if (parameter == 0)
      {
        // Query current scan code set
        AppendToOutputBuffer(0x02);
      }
      else
      {
        Log_DevPrintf("Set keyboard scan code set %u", ZeroExtend32(parameter));
      }
    }
    break;

    case 0xF2: // Read ID
    {
      // Log_DevPrintf("Read keyboard ID");
      // AppendToOutputBuffer(0xFA);
      // AppendToOutputBuffer(0xAB);
      // AppendToOutputBuffer(0x83);
    }
    break;

    case 0xF4: // Enable keyboard
    {
      Log_DevPrintf("Enable keyboard (Keyboard command)");
      AppendToOutputBuffer(0xFA);
    }
    break;

    case 0xF5: // Disable keyboard
    {
      Log_DevPrintf("Disable keyboard (Keyboard command)");
      AppendToOutputBuffer(0xFA);
    }
    break;

    case 0xFF: // Reset keyboard
    {
      Log_DevPrintf("Keyboard reset");

      AppendToOutputBuffer(0xFA); // Command ACK
      AppendToOutputBuffer(0xAA); // Power on reset
    }
    break;

    default:
      Log_WarningPrintf("Unknown command: 0x%02X", uint32(value));
      break;
  }

  m_pending_keyboard_command = 0;
}

} // namespace HW
