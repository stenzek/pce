#pragma once

#include "YBaseLib/PODArray.h"
#include "pce/bitfield.h"
#include "pce/component.h"
#include "pce/scancodes.h"
#include "pce/system.h"
#include <array>
#include <deque>

namespace HW {

class i8042_PS2 : public Component
{
public:
  i8042_PS2();
  ~i8042_PS2();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  // Input/output port updating from outside
  using OutputPortWrittenCallback = std::function<void(uint8, uint8, bool)>;
  void SetInputPort(uint8 value) { m_input_port = value; }
  uint8 GetOutputPort() const { return m_output_port.raw; }
  void SetOutputPort(uint8 value) { m_output_port.raw = value; }
  void SetOutputPortWrittenCallback(OutputPortWrittenCallback callback)
  {
    m_output_port_written_callback = std::move(callback);
  }

private:
  static const uint32 SERIALIZATION_ID = MakeSerializationID('8', '0', '4', '2');
  static const uint32 PORT_1_IRQ = 1;
  static const uint32 PORT_2_IRQ = 12;
  static const uint32 NUM_PORTS = 2;
  static const uint32 INTERNAL_RAM_SIZE = 32;
  static const uint32 INPUT_BUFFER_SIZE = 32;
  static const uint32 PORT_BUFFER_SIZE = 32;
  static const uint32 EXTERNAL_BUFFER_SIZE = 1024;
  static const uint16 NO_PENDING_COMMAND = 0xFFFF;
  static const CycleCount SERIAL_TRANSFER_DELAY = 250;
  static const uint8 DEFAULT_MOUSE_SAMPLE_RATE = 100;
  static const uint32 NUM_MOUSE_BUTTONS = 3;

  void SoftReset();

  void IOReadDataPort(uint8* value);
  void IOWriteDataPort(uint8 value);
  void IOReadStatusRegister(uint8* value);
  void IOWriteCommandRegister(uint8 value);

  void OnHostKeyboardEvent(GenScanCode scancode, bool key_down);
  void OnHostMousePositionChanged(int32 dx, int32 dy);
  void OnHostMouseButtonChanged(uint32 button, bool down);

  void UpdateEvents();
  void OnTransferEvent();
  void OnMouseReportEvent();

  // Overwrites whatever was in the output buffer.
  void SetOutputBuffer(uint32 port, uint8 data);

  // Writes to the *data* buffer.
  void AppendToKeyboardBuffer(uint8 data);
  void AppendToMouseBuffer(uint8 data);

  void EnqueueCommandOrData(uint8 data, bool is_data);
  void OnCommandEvent();

  bool HandleControllerCommand(uint8 command, uint8 data, bool has_data);
  bool HandleKeyboardCommand(uint8 command, uint8 data, bool has_data);
  bool HandleMouseCommand(uint8 command, uint8 data, bool has_data);

  void ResetKeyboard();
  void ResetMouse();
  void CreateMousePacket();

  Clock m_clock;
  System* m_system = nullptr;
  OutputPortWrittenCallback m_output_port_written_callback;

  union
  {
    uint8 raw = 0;
    BitField<uint8, bool, 0, 1> output_buffer_status; // KBC->CPU, outb
    BitField<uint8, bool, 1, 1> input_buffer_status;  // CPU->KBC, inpb
    BitField<uint8, bool, 2, 1> self_test_flag;       // sysf
    BitField<uint8, bool, 3, 1> command_flag;         // c_d
    BitField<uint8, bool, 4, 1> keyboard_unlocked;    // keyl
    BitField<uint8, bool, 5, 1> mouse_buffer_status;  // auxb
    BitField<uint8, bool, 6, 1> timeout_error;        // tim
    BitField<uint8, bool, 7, 1> parity_error;         // pare
  } m_status_register;

  uint8 m_input_port = 0;

  union
  {
    uint8 raw = 0;

    // BitField<uint8, bool, 0, 1> system_reset;
    BitField<uint8, bool, 1, 1> a20_gate;
    BitField<uint8, bool, 4, 1> port_1_interrupt_requested;
    BitField<uint8, bool, 5, 1> port_2_interrupt_requested;
    // BitField<uint8, bool, 6, 1> keyboard_data_output;
    // BitField<uint8, bool, 7, 1> keyboard_clock_output;
  } m_output_port;

  union
  {
    union
    {
      uint8 raw = 0;
      BitField<uint8, bool, 0, 1> port_1_interrupt;
      BitField<uint8, bool, 1, 1> port_2_interrupt;
      BitField<uint8, bool, 2, 1> self_test_flag;
      BitField<uint8, bool, 3, 1> zero;
      BitField<uint8, bool, 4, 1> port_1_clock_disable;
      BitField<uint8, bool, 5, 1> port_2_clock_disable;
      BitField<uint8, bool, 6, 1> port_1_translation;
      BitField<uint8, bool, 7, 1> zero2;
    } m_configuration_byte;

    uint8 m_internal_ram[INTERNAL_RAM_SIZE] = {};
  };

  uint8 m_input_buffer = 0;
  uint8 m_output_buffer = 0;

  uint16 m_pending_command = NO_PENDING_COMMAND;
  uint16 m_pending_keyboard_command = NO_PENDING_COMMAND;
  uint16 m_pending_mouse_command = NO_PENDING_COMMAND;

  // buffers
  struct
  {
    std::deque<uint8> scan_buffer; // for pending scancodes
    std::deque<uint8> data_buffer; // for command replies
    bool enabled = true;
  } m_keyboard;

  struct
  {
    std::deque<uint8> data_buffer; // for command replies and packets

    int32 delta_x = 0;
    int32 delta_y = 0;
    uint8 button_state = 0;
    bool buttons_changed = false;

    uint8 sample_rate = 100;
    bool enabled = true;
    bool stream_mode = true;
  } m_mouse;

  // timer events
  TimingEvent::Pointer m_command_event{};
  TimingEvent::Pointer m_transfer_event{};
  TimingEvent::Pointer m_mouse_report_event{};
};

} // namespace HW
