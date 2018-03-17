#pragma once

#include "YBaseLib/PODArray.h"
#include "pce/bitfield.h"
#include "pce/component.h"
#include "pce/scancodes.h"
#include "pce/system.h"
#include <array>

namespace HW {

class PS2Controller : public Component
{
public:
  PS2Controller();
  ~PS2Controller();

  virtual void Initialize(System* system, Bus* bus) override;
  virtual void Reset() override;
  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

  // Input/output port updating from outside
  using OutputPortWrittenCallback = std::function<void(uint8)>;
  void SetInputPort(uint8 value) { m_input_port = value; }
  uint8 GetOutputPort() const { return m_output_port.raw; }
  void SetOutputPort(uint8 value) { m_output_port.raw = value; }
  void SetOutputPortWrittenCallback(OutputPortWrittenCallback callback)
  {
    m_output_port_written_callback = std::move(callback);
  }

  // Lower keyboard interrupt, needed for the PPI implementation
  void LowerKeyboardInterrupt(bool force_when_full = false);

private:
  static const uint32 SERIALIZATION_ID = MakeSerializationID('8', '0', '4', '2');
  static const uint32 KEYBOARD_IRQ = 1;

  void AddScanCode(GenScanCode scancode, bool key_down);
  bool AppendToOutputBuffer(const void* data, uint32 length);
  bool AppendToOutputBuffer(uint8 data);
  void RemoveKeyboardBufferBytes(uint32 count);
  void ConnectIOPorts(Bus* bus);

  void IOReadDataPort(uint8* value);
  void IOWriteDataPort(uint8 value);
  void IOReadStatusRegister(uint8* value);
  void IOWriteCommandRegister(uint8 value);
  void HandleControllerCommand(bool has_parameter);
  void HandleKeyboardCommand(uint8 value);

  System* m_system = nullptr;
  OutputPortWrittenCallback m_output_port_written_callback;

  union
  {
    uint8 raw = 0;
    BitField<uint8, bool, 0, 1> output_buffer_status; // KBC->CPU
    BitField<uint8, bool, 1, 1> input_buffer_status;  // CPU->KBC
    BitField<uint8, bool, 2, 1> self_test_flag;
    BitField<uint8, bool, 3, 1> command_flag;
    BitField<uint8, bool, 4, 1> keyboard_unlocked;
    BitField<uint8, bool, 5, 1> mouse_buffer_status;
    BitField<uint8, bool, 6, 1> timeout_error;
    BitField<uint8, bool, 7, 1> parity_error;
  } m_status_register;

  union
  {
    uint8 raw = 0;
    BitField<uint8, bool, 0, 1> port_1_interrupt;
    BitField<uint8, bool, 1, 1> port_2_interrupt;
    BitField<uint8, bool, 2, 1> self_test_flag;
    BitField<uint8, bool, 3, 1> zero;
    BitField<uint8, bool, 4, 1> port_1_clock;
    BitField<uint8, bool, 5, 1> port_2_clock;
    BitField<uint8, bool, 6, 1> port_1_translation;
    BitField<uint8, bool, 7, 1> zero2;
  } m_configuration_byte;

  uint8 m_input_port = 0;

  union
  {
    uint8 raw = 0;

    // BitField<uint8, bool, 0, 1> system_reset;
    // BitField<uint8, bool, 1, 1> a20_gate;
    BitField<uint8, bool, 4, 1> output_buffer_full;
    BitField<uint8, bool, 5, 1> input_buffer_empty;
    BitField<uint8, bool, 6, 1> keyboard_data_output;
    BitField<uint8, bool, 7, 1> keyboard_clock_output;
  } m_output_port;

  static const uint32 KEYBOARD_BUFFER_SIZE = 32;
  uint8 m_output_buffer[KEYBOARD_BUFFER_SIZE] = {};
  uint32 m_output_buffer_pos = 0;

  uint8 m_pending_command = 0;
  uint8 m_input_buffer = 0;

  uint8 m_pending_keyboard_command = 0;
};

} // namespace HW
