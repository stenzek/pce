#include "pce/hw/i8042_ps2.h"
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
Log_SetChannel(HW::i8042_PS2);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(i8042_PS2);
BEGIN_OBJECT_PROPERTY_MAP(i8042_PS2)
END_OBJECT_PROPERTY_MAP()

i8042_PS2::i8042_PS2(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_clock("8042 Keyboard Controller", 1000000.0f)
{
}

i8042_PS2::~i8042_PS2() = default;

bool i8042_PS2::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_interrupt_controller = m_system->GetComponentByType<InterruptController>();
  if (!m_interrupt_controller)
  {
    Log_ErrorPrintf("Failed to locate interrupt controller.");
    return false;
  }

  m_clock.SetManager(system->GetTimingManager());
  m_command_event = m_clock.NewEvent("Keyboard Command", 10, std::bind(&i8042_PS2::OnCommandEvent, this), false);
  m_transfer_event =
    m_clock.NewEvent("Keyboard Transfer", SERIAL_TRANSFER_DELAY, std::bind(&i8042_PS2::OnTransferEvent, this), false);
  m_mouse_report_event = m_clock.NewFrequencyEvent("Mouse Report", float(DEFAULT_MOUSE_SAMPLE_RATE),
                                                   std::bind(&i8042_PS2::OnMouseReportEvent, this), false);

  bus->ConnectIOPortRead(0x60, this, std::bind(&i8042_PS2::IOReadDataPort, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x60, this, std::bind(&i8042_PS2::IOWriteDataPort, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x64, this, std::bind(&i8042_PS2::IOReadStatusRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x64, this, std::bind(&i8042_PS2::IOWriteCommandRegister, this, std::placeholders::_2));

  m_system->GetHostInterface()->AddKeyboardCallback(
    this, std::bind(&i8042_PS2::OnHostKeyboardEvent, this, std::placeholders::_1, std::placeholders::_2));
  m_system->GetHostInterface()->AddMousePositionChangeCallback(
    this, std::bind(&i8042_PS2::OnHostMousePositionChanged, this, std::placeholders::_1, std::placeholders::_2));
  m_system->GetHostInterface()->AddMouseButtonChangeCallback(
    this, std::bind(&i8042_PS2::OnHostMouseButtonChanged, this, std::placeholders::_1, std::placeholders::_2));

  return true;
}

void i8042_PS2::Reset()
{
  SoftReset();

  m_output_port.raw = 0;
  m_output_port.a20_gate = true;
  if (m_output_port_written_callback)
    m_output_port_written_callback(m_output_port.raw, 0, false);

  m_status_register.keyboard_unlocked = true;
  m_status_register.self_test_flag = false;
  m_configuration_byte.self_test_flag = false;

  m_keyboard.scan_buffer.clear();
  m_keyboard.data_buffer.clear();
  ResetKeyboard();

  m_mouse.data_buffer.clear();
  ResetMouse();
}

void i8042_PS2::SoftReset()
{
  m_status_register.output_buffer_status = false;
  m_status_register.input_buffer_status = false;
  m_status_register.self_test_flag = false;
  m_status_register.command_flag = false;
  m_status_register.mouse_buffer_status = false;
  m_status_register.timeout_error = false;
  m_status_register.parity_error = false;

  // Enable translation by default. This should be put in firmware instead..
  m_configuration_byte.port_1_interrupt = true;
  m_configuration_byte.port_1_clock_disable = false;
  m_configuration_byte.port_1_translation = true;
  m_configuration_byte.port_2_clock_disable = true;

  // This should be set after boot when some time has elapsed?
  m_status_register.self_test_flag = true;

  m_input_buffer = 0;
  m_output_buffer = 0;
  m_pending_command = NO_PENDING_COMMAND;
  m_pending_keyboard_command = NO_PENDING_COMMAND;
  m_pending_mouse_command = NO_PENDING_COMMAND;

  UpdateEvents();

  if (m_command_event->IsActive())
    m_command_event->Deactivate();
}

bool i8042_PS2::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.SafeReadUInt8(&m_status_register.raw);
  reader.SafeReadUInt8(&m_configuration_byte.raw);
  reader.SafeReadUInt8(&m_input_port);
  reader.SafeReadUInt8(&m_output_port.raw);
  reader.SafeReadBytes(m_internal_ram, sizeof(m_internal_ram));
  reader.SafeReadUInt8(&m_input_buffer);
  reader.SafeReadUInt16(&m_pending_command);
  reader.SafeReadUInt16(&m_pending_keyboard_command);

  auto ReadBuffer = [&reader](std::deque<uint8>& buffer) {
    uint32 size = 0;
    reader.SafeReadUInt32(&size);
    buffer.clear();
    if (size > 0)
    {
      uint8 data = 0;
      for (uint32 j = 0; j < size; j++)
        reader.SafeReadUInt8(&data);
      buffer.push_back(data);
    }
  };

  ReadBuffer(m_keyboard.scan_buffer);
  ReadBuffer(m_keyboard.data_buffer);
  reader.SafeReadBool(&m_keyboard.enabled);

  ReadBuffer(m_mouse.data_buffer);
  reader.SafeReadInt32(&m_mouse.delta_x);
  reader.SafeReadInt32(&m_mouse.delta_y);
  reader.SafeReadUInt8(&m_mouse.button_state);
  reader.SafeReadBool(&m_mouse.buttons_changed);
  reader.SafeReadBool(&m_mouse.enabled);
  reader.SafeReadBool(&m_mouse.stream_mode);
  reader.SafeReadUInt8(&m_mouse.sample_rate);

  return !reader.GetErrorState();
}

bool i8042_PS2::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  writer.SafeWriteUInt8(m_status_register.raw);
  writer.SafeWriteUInt8(m_configuration_byte.raw);
  writer.SafeWriteUInt8(m_input_port);
  writer.SafeWriteUInt8(m_output_port.raw);
  writer.SafeWriteBytes(m_internal_ram, sizeof(m_internal_ram));
  writer.SafeWriteUInt8(m_input_buffer);
  writer.SafeWriteUInt16(m_pending_command);
  writer.SafeWriteUInt16(m_pending_keyboard_command);

  auto WriteBuffer = [&writer](std::deque<uint8>& buffer) {
    writer.SafeWriteUInt32(static_cast<uint32>(buffer.size()));
    if (!buffer.empty())
    {
      for (size_t j = 0; j < buffer.size(); j++)
        writer.SafeWriteUInt8(buffer[j]);
    }
  };

  WriteBuffer(m_keyboard.scan_buffer);
  WriteBuffer(m_keyboard.data_buffer);
  writer.SafeWriteBool(m_keyboard.enabled);

  WriteBuffer(m_mouse.data_buffer);
  writer.SafeWriteInt32(m_mouse.delta_x);
  writer.SafeWriteInt32(m_mouse.delta_y);
  writer.SafeWriteUInt8(m_mouse.button_state);
  writer.SafeWriteBool(m_mouse.buttons_changed);
  writer.SafeWriteBool(m_mouse.enabled);
  writer.SafeWriteBool(m_mouse.stream_mode);
  writer.SafeWriteUInt8(m_mouse.sample_rate);

  return true;
}

void i8042_PS2::IOReadStatusRegister(uint8* value)
{
  *value = m_status_register.raw;
}

void i8042_PS2::IOReadDataPort(uint8* value)
{
  // Lower interrupts after read
  if (m_status_register.mouse_buffer_status)
  {
    m_output_port.port_2_interrupt_requested = false;
    m_interrupt_controller->LowerInterrupt(PORT_2_IRQ);
  }
  else
  {
    m_output_port.port_1_interrupt_requested = false;
    m_interrupt_controller->LowerInterrupt(PORT_1_IRQ);
  }

  *value = m_output_buffer;
  m_status_register.output_buffer_status = false;
  m_status_register.mouse_buffer_status = false;
  UpdateEvents();
}

void i8042_PS2::IOWriteDataPort(uint8 value)
{
  if (m_command_event->IsActive())
    m_command_event->InvokeEarly(true);

  Log_TracePrintf("Write data port, pending command = 0x%02X, value = 0x%02X", uint32(m_pending_command), value);
  m_status_register.command_flag = false;

  EnqueueCommandOrData(value, true);
}

void i8042_PS2::IOWriteCommandRegister(uint8 value)
{
  if (m_command_event->IsActive())
    m_command_event->InvokeEarly(true);

  Log_TracePrintf("Write command register, value = 0x%02X", value);
  m_status_register.command_flag = true;

  EnqueueCommandOrData(value, false);
}

void i8042_PS2::OnHostKeyboardEvent(GenScanCode scancode, bool key_down)
{
  if (!m_keyboard.enabled)
    return;

  uint8 index = key_down ? 0 : 1;
  DebugAssert(index < 2);

  const uint8* hw_scancode;
  if (m_configuration_byte.port_1_translation)
    hw_scancode = KeyboardScanCodes::Set1Mapping[scancode][index];
  else
    hw_scancode = KeyboardScanCodes::Set2Mapping[scancode][index];

  for (uint32 i = 0; hw_scancode[i] != 0; i++)
    m_keyboard.scan_buffer.push_back(hw_scancode[i]);

  UpdateEvents();
}

void i8042_PS2::OnHostMousePositionChanged(int32 dx, int32 dy)
{
  if (dx == 0 && dy == 0)
    return;

  m_mouse.delta_x += dx;
  m_mouse.delta_y -= dy;
  UpdateEvents();
}

void i8042_PS2::OnHostMouseButtonChanged(uint32 button, bool down)
{
  if (button >= NUM_MOUSE_BUTTONS)
    return;

  const uint8 mask = uint8(1) << button;
  const uint8 new_value = (down) ? (m_mouse.button_state | mask) : (m_mouse.button_state & (~mask));
  if (new_value != m_mouse.button_state)
  {
    m_mouse.button_state = new_value;
    m_mouse.buttons_changed = true;
    UpdateEvents();
  }
}

void i8042_PS2::UpdateEvents()
{
  // Buffer has to be empty to be able to transfer.
  const bool keyboard_has_command_data = !m_keyboard.data_buffer.empty();
  const bool keyboard_has_scan_data = (!m_keyboard.scan_buffer.empty() && !m_configuration_byte.port_1_clock_disable);
  const bool mouse_has_data = !m_mouse.data_buffer.empty();
  m_transfer_event->SetActive(!m_status_register.output_buffer_status &&
                              (keyboard_has_command_data | keyboard_has_scan_data | mouse_has_data));

  // Mouse reporting.
  const bool mouse_active = (!m_configuration_byte.port_2_clock_disable && m_mouse.enabled && m_mouse.stream_mode);
  const bool mouse_has_events = (m_mouse.delta_x != 0 || m_mouse.delta_y != 0 || m_mouse.buttons_changed);
  m_mouse_report_event->SetActive(mouse_active && mouse_has_events);
}

void i8042_PS2::OnTransferEvent()
{
  // Don't transfer if something else got in the way.
  if (m_status_register.output_buffer_status)
  {
    m_transfer_event->Deactivate();
    return;
  }

  // Only transfer one byte at a time.
  // Keyboard has higher priority.
  if (!m_keyboard.data_buffer.empty() ||
      (!m_keyboard.scan_buffer.empty() && !m_configuration_byte.port_1_clock_disable))
  {
    auto& src_buffer = (!m_keyboard.data_buffer.empty()) ? m_keyboard.data_buffer : m_keyboard.scan_buffer;
    m_output_buffer = src_buffer.front();
    src_buffer.pop_front();

    m_status_register.output_buffer_status = true;
    m_status_register.mouse_buffer_status = false;

    if (m_configuration_byte.port_1_interrupt)
    {
      m_output_port.port_1_interrupt_requested = true;
      m_interrupt_controller->RaiseInterrupt(PORT_1_IRQ);
    }
  }
  else if (!m_mouse.data_buffer.empty())
  {
    m_output_buffer = m_mouse.data_buffer.front();
    m_mouse.data_buffer.pop_front();

    m_status_register.output_buffer_status = true;
    m_status_register.mouse_buffer_status = true;

    if (m_configuration_byte.port_2_interrupt)
    {
      m_output_port.port_2_interrupt_requested = true;
      m_interrupt_controller->RaiseInterrupt(PORT_2_IRQ);
    }
  }

  UpdateEvents();
}

void i8042_PS2::OnMouseReportEvent()
{
  DebugAssert(m_mouse.enabled && m_mouse.stream_mode);
  CreateMousePacket();
  UpdateEvents();
}

void i8042_PS2::SetOutputBuffer(uint32 port, uint8 data)
{
  m_output_buffer = data;
  m_status_register.output_buffer_status = true;
  m_status_register.mouse_buffer_status = (port != 0);
  UpdateEvents();
}

void i8042_PS2::AppendToKeyboardBuffer(uint8 data)
{
  m_keyboard.data_buffer.push_back(data);
  UpdateEvents();
}

void i8042_PS2::AppendToMouseBuffer(uint8 data)
{
  m_mouse.data_buffer.push_back(data);
  UpdateEvents();
}

void i8042_PS2::EnqueueCommandOrData(uint8 data, bool is_data)
{
  m_status_register.input_buffer_status = true;

  CycleCount delay = 10; // 10us
  if (is_data)
  {
    m_input_buffer = data;
    if (m_pending_command != NO_PENDING_COMMAND)
      m_pending_command |= (1 << 8);
  }
  else
  {
    m_pending_command = ZeroExtend16(data);
  }

  // Some commands take longer than others
  const uint8 command = Truncate8(m_pending_command);
  if (command == 0xAA) // self-test
    delay = 100;
  else if (command == 0x60) // write config byte, bochs bios expects this to happen instantly
    delay = 0;

  if (delay)
    m_command_event->Queue(delay); // 10us
  else
    OnCommandEvent();
}

void i8042_PS2::OnCommandEvent()
{
  m_status_register.input_buffer_status = false;
  if (m_command_event->IsActive())
    m_command_event->Deactivate();

  if (m_pending_command != NO_PENDING_COMMAND)
  {
    const bool has_data = !!(m_pending_command & (1 << 8));
    if (HandleControllerCommand(Truncate8(m_pending_command), m_input_buffer, has_data))
      m_pending_command = NO_PENDING_COMMAND;
  }
  else if (m_pending_keyboard_command != NO_PENDING_COMMAND)
  {
    // keyboard command done?
    if (HandleKeyboardCommand(Truncate8(m_pending_keyboard_command), m_input_buffer, true))
      m_pending_keyboard_command = NO_PENDING_COMMAND;
  }
  else
  {
    // hold on to the command
    if (!HandleKeyboardCommand(m_input_buffer, m_input_buffer, false))
      m_pending_keyboard_command = ZeroExtend16(m_input_buffer) | (1 << 8);
  }
}

bool i8042_PS2::HandleControllerCommand(uint8 command, uint8 data, bool has_data)
{
  if (command >= 0x20 && command <= 0x3F)
  {
    if (command == 0x20)
      Log_DevPrintf("KBC read config byte");

    // Read byte (CMD&0x1F) of internal RAM
    SetOutputBuffer(0, m_internal_ram[command & 0x1F]);
    return true;
  }
  else if (command >= 0x60 && command <= 0x7F)
  {
    if (!has_data)
      return false;

    // Write byte (CMD&0x1F) of internal RAM
    if (command == 0x60)
      Log_DevPrintf("KBC configuration byte = 0x%02X", data);

    m_internal_ram[command & 0x1F] = data;

    // config byte can re-enable clocking
    if (command == 0x60)
      UpdateEvents();

    return true;
  }

  switch (command)
  {
    case 0xA1: // Get controller version
    {
      SetOutputBuffer(0, 0x30);
      return true;
    }

    case 0xAA: // Test controller
    {
      Log_DevPrintf("KBC test controller");
      SoftReset();
      SetOutputBuffer(0, 0x55); // test passed
      return true;
    }

    case 0xAB: // Keyboard interface test
    {
      Log_DevPrintf("KBC test interface");
      SetOutputBuffer(0, 0x00);
      return true;
    }

    case 0xA7: // Disable aux device
    {
      Log_DevPrintf("KBC disable mouse");
      m_configuration_byte.port_2_clock_disable = true;
      UpdateEvents();
      return true;
    }

    case 0xA8: // Enable aux device
    {
      Log_DevPrintf("KBC enable mouse");
      m_configuration_byte.port_2_clock_disable = false;
      UpdateEvents();
      return true;
    }

    case 0xA9: // mouse interface test
    {
      Log_DevPrintf("Mouse test interface");
      SetOutputBuffer(0, 0x00);
      return true;
    }

    case 0xAD: // Disable keyboard interface
    {
      Log_DevPrintf("KBC disable keyboard");
      m_configuration_byte.port_1_clock_disable = true;
      UpdateEvents();
      return true;
    }

    case 0xAE: // Enable keyboard interface
    {
      Log_DevPrintf("KBC enable keyboard");
      m_configuration_byte.port_1_clock_disable = false;
      UpdateEvents();
      return true;
    }

    case 0xC0: // Read input port
    {
      Log_DevPrintf("KBC read input port = 0x%02X", ZeroExtend32(m_input_port));
      SetOutputBuffer(0, m_input_port);
      return true;
    }

    case 0xD0: // Read output port
    {
      Log_DevPrintf("KBC read output port");
      SetOutputBuffer(0, m_output_port.raw);
      return true;
    }

    case 0xD1: // Write output port
    {
      if (!has_data)
        return false;

      const uint8 old_value = m_output_port.raw;
      Log_DevPrintf("KBC write output port: 0x%02X -> 0x%02X", ZeroExtend32(old_value), ZeroExtend32(data));
      m_output_port.raw = data;

      // This can control the A20 gate as a legacy method, so pass it to the system.
      if (m_output_port_written_callback)
        m_output_port_written_callback(m_output_port.raw, old_value, false);

      return true;
    }

    case 0xD2: // Write to keyboard output buffer
    {
      if (!has_data)
        return false;

      Log_DevPrintf("Write to keyboard buffer 0x%02X", ZeroExtend32(data));
      AppendToKeyboardBuffer(data);
      UpdateEvents();
      return true;
    }

    case 0xD3: // Write to aux port buffer (mouse)
    {
      if (!has_data)
        return false;

      Log_DevPrintf("Write to mouse buffer 0x%02X", ZeroExtend32(data));
      AppendToMouseBuffer(data);
      UpdateEvents();
      return true;
    }

    case 0xD4: // Write to aux port (mouse)
    {
      if (!has_data)
        return false;

      Log_DevPrintf("KBC write mouse port = 0x%02X", ZeroExtend32(data));
      if (m_pending_mouse_command != NO_PENDING_COMMAND)
      {
        if (HandleMouseCommand(Truncate8(m_pending_mouse_command), data, true))
          m_pending_mouse_command = NO_PENDING_COMMAND;
      }
      else
      {
        if (!HandleMouseCommand(Truncate8(data), 0, false))
          m_pending_mouse_command = ZeroExtend16(data);
      }

      return true;
    }

      // Unknown, sent by AMI 386 bios.
    case 0xC9:
      Log_WarningPrintf("Unknown 0xC9 command");
      SetOutputBuffer(0, 0x00);
      return true;

      // Unknown, sent by AMI 486 bios.
    case 0xB4:
      Log_WarningPrintf("Unknown 0xB4 command");
      SetOutputBuffer(0, 0x00);
      return true;

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
        uint8 pulse_value = m_output_port.raw | ~mask;
        uint8 restore_value = m_output_port.raw;
        m_output_port_written_callback(pulse_value, restore_value, true);
        m_output_port_written_callback(restore_value, pulse_value, true);
      }

      return true;
    }

    default:
      Log_WarningPrintf("Unknown command: 0x%02X data=%s 0x%02X", uint32(command), has_data ? "yes" : "no",
                        uint32(data));
      return true;
  }
}

bool i8042_PS2::HandleKeyboardCommand(uint8 command, uint8 data, bool has_data)
{
  auto SendACK = [this]() { AppendToKeyboardBuffer(0xFA); };

  switch (command)
  {
    case 0xEE: // Echo
    {
      AppendToKeyboardBuffer(0xEE);
      return true;
    }

    case 0xED: // Set LED state
    {
      SendACK();
      if (!has_data)
        return false;

      Log_DevPrintf("Keyboard set LEDs %02X", ZeroExtend32(data));
      return true;
    }

    case 0xF0: // Set scan code set
    {
      SendACK();
      if (!has_data)
        return false;

      if (data == 0)
      {
        // Query current scan code set
        AppendToKeyboardBuffer(0x02);
      }
      else
      {
        Log_DevPrintf("Set keyboard scan code set %u", ZeroExtend32(data));
      }

      return true;
    }

    case 0xF2: // Read ID
    {
      Log_DevPrintf("Read keyboard ID");
      SendACK();
      AppendToKeyboardBuffer(0xAB);
      AppendToKeyboardBuffer(m_configuration_byte.port_1_translation ? 0x41 : 0x83);
      return true;
    }

    case 0xF3: // Set typematic/repeat rate
    {
      SendACK();
      if (!has_data)
        return false;

      Log_DevPrintf("Set typematic rate 0x%02X", data);
      return true;
    }

    case 0xF4: // Enable keyboard
    {
      Log_DevPrintf("Enable keyboard (Keyboard command)");
      m_keyboard.enabled = true;
      SendACK();
      return true;
    }

    case 0xF5: // Disable keyboard
    {
      Log_DevPrintf("Disable keyboard (Keyboard command)");
      m_keyboard.enabled = false;
      SendACK();
      return true;
    }

    case 0xFF: // Reset keyboard
    {
      Log_DevPrintf("Keyboard reset");
      ResetKeyboard();

      SendACK();
      AppendToKeyboardBuffer(0xAA); // Power on reset
      return true;
    }

    default:
      Log_WarningPrintf("Unknown command: 0x%02X", uint32(command));
      AppendToKeyboardBuffer(0xFE); // NACK
      return true;
  }
}

void i8042_PS2::ResetKeyboard()
{
  m_keyboard.enabled = true;
}

bool i8042_PS2::HandleMouseCommand(uint8 command, uint8 data, bool has_data)
{
  auto SendACK = [this]() { AppendToMouseBuffer(0xFA); };

  switch (command)
  {
    case 0xE6: // set scaling to 1:1
    case 0xE7: // set scaling to 2:1
    {
      const bool is_2_1 = (command == 0xE7);
      Log_DevPrintf("Set mouse scaling to %u:1", is_2_1 ? 2 : 1);
      SendACK();
      return true;
    }

    case 0xE8: // Set resolution
    {
      if (!has_data)
      {
        SendACK();
        return false;
      }

      // 0 - 25dpi, 1 count per mm
      // 1 - 50dpi, 2 counts per mm
      // 2 - 100dpi, 4 counts per mm
      // 3 - 200dpi, 8 counts per mm
      Log_DevPrintf("Set mouse resolution 0x%02X", ZeroExtend32(data));
      SendACK();
      return true;
    }

    case 0xE9: // Get information
    {
      SendACK();

      uint8 status_byte = (((!m_mouse.stream_mode) ? 0x40 : 0x00) | (!m_mouse.enabled ? 0x20 : 0x00) |
                           // scaling==0 -> 0x10
                           ((m_mouse.button_state & 1) << 2) | ((m_mouse.button_state & 2) << 0));
      AppendToMouseBuffer(status_byte);
      AppendToMouseBuffer(0); // resolution byte
      AppendToMouseBuffer(m_mouse.sample_rate);
      return true;
    }

    case 0xEA: // Set stream mode
    {
      SendACK();
      if (!m_mouse.stream_mode)
      {
        m_mouse.stream_mode = true;
        UpdateEvents();
      }
      return true;
    }

    case 0xEB: // Read data (for remote/polling mode)
    {
      SendACK();
      CreateMousePacket();
      return true;
    }

    case 0xF0: // set remote mode
    {
      SendACK();
      if (m_mouse.stream_mode)
      {
        m_mouse.stream_mode = false;
        m_mouse.data_buffer.clear();
        UpdateEvents();
      }
      return true;
    }

    case 0xF2: // read device type
    {
      Log_DevPrintf("Read mouse ID");
      SendACK();
      AppendToMouseBuffer(0x00); // 0x03 for wheel
      return true;
    }

    case 0xF3: // set sample rate
    {
      if (!has_data)
      {
        SendACK();
        return false;
      }

      Log_DevPrintf("Set sample rate %u", ZeroExtend32(data));
      SendACK();

      // Ensure it's sane..
      if (data == 0)
        return true;

      if (m_mouse.sample_rate != data)
      {
        m_mouse.sample_rate = data;
        m_mouse_report_event->SetFrequency(float(data));
        UpdateEvents();
      }

      return true;
    }

    case 0xF4: // Enable mouse
    {
      Log_DevPrintf("Enable mouse (mouse command)");
      SendACK();
      if (!m_mouse.enabled)
      {
        m_mouse.enabled = true;
        UpdateEvents();
      }
      return true;
    }

    case 0xF5: // Disable mouse
    {
      Log_DevPrintf("Disable mouse (mouse command)");
      SendACK();
      if (m_mouse.enabled)
      {
        m_mouse.enabled = false;
        UpdateEvents();
      }
      return true;
    }

    case 0xF6: // Set defaults
    {
      Log_DevPrintf("Set mouse defaults");
      SendACK();
      if (m_mouse.sample_rate != DEFAULT_MOUSE_SAMPLE_RATE)
      {
        m_mouse.sample_rate = DEFAULT_MOUSE_SAMPLE_RATE;
        UpdateEvents();
      }

      // resolution_cpmm = 4
      // scalilng = 1
      if (!m_mouse.stream_mode)
      {
        m_mouse.stream_mode = true;
        UpdateEvents();
      }

      return true;
    }

    case 0xFF: // Reset keyboard
    {
      Log_DevPrintf("Mouse reset");
      ResetMouse();

      // If no mouse is attached, 0xFE should be sent, TIM bit should be set
      SendACK();                 // Command ACK
      AppendToMouseBuffer(0xAA); // Power on reset
      AppendToMouseBuffer(0x00); // ID code
      return true;
    }

    default:
      Log_WarningPrintf("Unknown command: 0x%02X", uint32(command));
      AppendToMouseBuffer(0xFE); // NACK
      return true;
  }
}

void i8042_PS2::ResetMouse()
{
  m_mouse.delta_x = 0;
  m_mouse.delta_y = 0;
  m_mouse.sample_rate = DEFAULT_MOUSE_SAMPLE_RATE;
  m_mouse.enabled = true;
  m_mouse.stream_mode = true;
  m_mouse_report_event->SetFrequency(float(DEFAULT_MOUSE_SAMPLE_RATE));
  UpdateEvents();
}

void i8042_PS2::CreateMousePacket()
{
  union
  {
    uint8 bits;
    BitField<uint8, uint8, 0, 3> button_bits;
    BitField<uint8, bool, 0, 1> button_left;
    BitField<uint8, bool, 1, 1> button_right;
    BitField<uint8, bool, 2, 1> button_middle;
    BitField<uint8, bool, 3, 1> always_one;
    BitField<uint8, bool, 4, 1> dx9;
    BitField<uint8, bool, 5, 1> dy9;
    BitField<uint8, bool, 6, 1> x_overflow;
    BitField<uint8, bool, 7, 1> y_overflow;
  } byte0;

  // We match the button states to this packet byte.
  byte0.bits = 0;
  byte0.button_bits = m_mouse.button_state;
  byte0.always_one = true;
  byte0.dx9 = (m_mouse.delta_x < 0);
  byte0.dy9 = (m_mouse.delta_y < 0);
  byte0.x_overflow = (m_mouse.delta_x > 255 || m_mouse.delta_x < -256);
  byte0.y_overflow = (m_mouse.delta_y > 255 || m_mouse.delta_y < -256);
  AppendToMouseBuffer(byte0.bits);
  AppendToMouseBuffer(uint8(m_mouse.delta_x));
  AppendToMouseBuffer(uint8(m_mouse.delta_y));

  // Clear delta.
  // TODO: We could handle the overflow better by only subtracting a max of 256.
  m_mouse.delta_x = 0;
  m_mouse.delta_y = 0;
  m_mouse.buttons_changed = false;
}

} // namespace HW
