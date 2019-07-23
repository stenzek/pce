#pragma once

#include "YBaseLib/PODArray.h"
#include "common/bitfield.h"
#include "pce/component.h"
#include "pce/scancodes.h"
#include "pce/system.h"
#include <array>
#include <deque>

class InterruptController;
class TimingEvent;

namespace HW {

class i8042_PS2 : public Component
{
  DECLARE_OBJECT_TYPE_INFO(i8042_PS2, Component);
  DECLARE_OBJECT_NO_FACTORY(i8042_PS2);
  DECLARE_OBJECT_PROPERTY_MAP(i8042_PS2);

public:
  i8042_PS2(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~i8042_PS2();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  // Input/output port updating from outside
  using OutputPortWrittenCallback = std::function<void(u8, u8, bool)>;
  void SetInputPort(u8 value) { m_input_port = value; }
  u8 GetOutputPort() const { return m_output_port.raw; }
  void SetOutputPort(u8 value) { m_output_port.raw = value; }
  void SetOutputPortWrittenCallback(OutputPortWrittenCallback callback)
  {
    m_output_port_written_callback = std::move(callback);
  }

private:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('8', '0', '4', '2');
  static constexpr u32 PORT_1_IRQ = 1;
  static constexpr u32 PORT_2_IRQ = 12;
  static constexpr u32 NUM_PORTS = 2;
  static constexpr u32 INTERNAL_RAM_SIZE = 32;
  static constexpr u32 INPUT_BUFFER_SIZE = 32;
  static constexpr u32 PORT_BUFFER_SIZE = 32;
  static constexpr u32 EXTERNAL_BUFFER_SIZE = 1024;
  static constexpr u16 NO_PENDING_COMMAND = 0xFFFF;
  static constexpr CycleCount SERIAL_TRANSFER_DELAY = 250;
  static constexpr u8 DEFAULT_MOUSE_SAMPLE_RATE = 100;
  static constexpr u32 NUM_MOUSE_BUTTONS = 3;

  void SoftReset();

  u8 IOReadDataPort();
  void IOWriteDataPort(u8 value);
  u8 IOReadStatusRegister();
  void IOWriteCommandRegister(u8 value);

  void OnHostKeyboardEvent(GenScanCode scancode, bool key_down);
  void OnHostMousePositionChanged(s32 dx, s32 dy);
  void OnHostMouseButtonChanged(u32 button, bool down);

  void UpdateEvents();
  void OnTransferEvent();
  void OnMouseReportEvent();

  // Overwrites whatever was in the output buffer.
  void SetOutputBuffer(u32 port, u8 data);

  // Writes to the *data* buffer.
  void AppendToKeyboardBuffer(u8 data);
  void AppendToMouseBuffer(u8 data);

  void EnqueueCommandOrData(u8 data, bool is_data);
  void OnCommandEvent();

  bool HandleControllerCommand(u8 command, u8 data, bool has_data);
  bool HandleKeyboardCommand(u8 command, u8 data, bool has_data);
  bool HandleMouseCommand(u8 command, u8 data, bool has_data);

  void ResetKeyboard();
  void ResetMouse();
  void CreateMousePacket();

  InterruptController* m_interrupt_controller = nullptr;
  OutputPortWrittenCallback m_output_port_written_callback;

  union
  {
    u8 raw = 0;
    BitField<u8, bool, 0, 1> output_buffer_status; // KBC->CPU, outb
    BitField<u8, bool, 1, 1> input_buffer_status;  // CPU->KBC, inpb
    BitField<u8, bool, 2, 1> self_test_flag;       // sysf
    BitField<u8, bool, 3, 1> command_flag;         // c_d
    BitField<u8, bool, 4, 1> keyboard_unlocked;    // keyl
    BitField<u8, bool, 5, 1> mouse_buffer_status;  // auxb
    BitField<u8, bool, 6, 1> timeout_error;        // tim
    BitField<u8, bool, 7, 1> parity_error;         // pare
  } m_status_register;

  u8 m_input_port = 0;

  union
  {
    u8 raw = 0;

    // BitField<uint8, bool, 0, 1> system_reset;
    BitField<u8, bool, 1, 1> a20_gate;
    BitField<u8, bool, 4, 1> port_1_interrupt_requested;
    BitField<u8, bool, 5, 1> port_2_interrupt_requested;
    // BitField<uint8, bool, 6, 1> keyboard_data_output;
    // BitField<uint8, bool, 7, 1> keyboard_clock_output;
  } m_output_port;

  union
  {
    union
    {
      u8 raw = 0;
      BitField<u8, bool, 0, 1> port_1_interrupt;
      BitField<u8, bool, 1, 1> port_2_interrupt;
      BitField<u8, bool, 2, 1> self_test_flag;
      BitField<u8, bool, 3, 1> zero;
      BitField<u8, bool, 4, 1> port_1_clock_disable;
      BitField<u8, bool, 5, 1> port_2_clock_disable;
      BitField<u8, bool, 6, 1> port_1_translation;
      BitField<u8, bool, 7, 1> zero2;
    } m_configuration_byte;

    u8 m_internal_ram[INTERNAL_RAM_SIZE] = {};
  };

  u8 m_input_buffer = 0;
  u8 m_output_buffer = 0;

  u16 m_pending_command = NO_PENDING_COMMAND;
  u16 m_pending_keyboard_command = NO_PENDING_COMMAND;
  u16 m_pending_mouse_command = NO_PENDING_COMMAND;

  // buffers
  struct
  {
    std::deque<u8> scan_buffer; // for pending scancodes
    std::deque<u8> data_buffer; // for command replies
    bool enabled = true;
  } m_keyboard;

  struct
  {
    std::deque<u8> data_buffer; // for command replies and packets

    s32 delta_x = 0;
    s32 delta_y = 0;
    u8 button_state = 0;
    bool buttons_changed = false;

    u8 sample_rate = 100;
    bool enabled = true;
    bool stream_mode = true;
  } m_mouse;

  // timer events
  std::unique_ptr<TimingEvent> m_command_event;
  std::unique_ptr<TimingEvent> m_transfer_event;
  std::unique_ptr<TimingEvent> m_mouse_report_event;
};

} // namespace HW
