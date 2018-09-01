#pragma once

#include "YBaseLib/PODArray.h"
#include "common/bitfield.h"
#include "pce/component.h"
#include "pce/scancodes.h"
#include "pce/system.h"
#include <array>

namespace HW {

class XT_PPI : public Component
{
public:
  XT_PPI();
  ~XT_PPI();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  // Switches are determined by hardware state.
  // Most of them are active low.
  void SetSwitch(size_t index, bool value) { m_switches[index] = value; }
  bool GetSwitch(size_t index) const { return m_switches[index]; }

  // Callbacks for external devices
  using SetBoolCallback = std::function<void(bool)>;
  using GetBoolCallback = std::function<bool()>;
  void SetSpeakerGateCallback(SetBoolCallback callback) { m_speaker_gate_callback = std::move(callback); }
  void SetSpeakerEnableCallback(SetBoolCallback callback) { m_speaker_enable_callback = std::move(callback); }
  void SetSpeakerOutputCallback(GetBoolCallback callback) { m_speaker_output_callback = std::move(callback); }

  // HLE access to keyboard buffer
  const uint8* GetKeyboardBuffer() const { return m_output_buffer; }
  uint32 GetKeyboardBufferSize() const { return m_output_buffer_pos; }
  void RemoveKeyboardBufferBytes(uint32 count);

private:
  static constexpr uint32 SERIALIZATION_ID = MakeSerializationID('8', '2', '5', '5');
  static constexpr uint32 KEYBOARD_IRQ = 1;
  static constexpr size_t SWITCH_COUNT = 8;

  void AddScanCode(GenScanCode scancode, bool key_down);
  bool AppendToOutputBuffer(const void* data, uint32 length);
  bool AppendToOutputBuffer(uint8 data);
  void ConnectIOPorts(Bus* bus);

  void IORead(uint32 port, uint8* value);
  void IOWrite(uint32 port, uint8 value);

  System* m_system = nullptr;

  // Callbacks
  SetBoolCallback m_speaker_gate_callback;
  SetBoolCallback m_speaker_enable_callback;
  GetBoolCallback m_speaker_output_callback;

  // NOTE: No need to save state for switches, system sets them.
  std::array<bool, SWITCH_COUNT> m_switches = {};

  union
  {
    uint8 bits = 0;

    BitField<uint8, bool, 7, 1> mode_set_active;
    BitField<uint8, bool, 5, 2> port_a_mode;
    BitField<uint8, bool, 4, 1> port_a_input;
    BitField<uint8, bool, 3, 1> port_c_upper_input;
    BitField<uint8, bool, 2, 1> port_b_mode;
    BitField<uint8, bool, 1, 1> port_b_input;
    BitField<uint8, bool, 0, 1> port_c_lower_input;
  } m_control_register;

  // Port A value when it's not connected to keyboard.
  uint8 m_port_a = 0;

  // Port B when it is configured as an output (from the CPU's point of view).
  union
  {
    uint8 bits = 0;

    BitField<uint8, bool, 7, 1> clear_keyboard;
    BitField<uint8, bool, 6, 1> keyboard_disable;
    BitField<uint8, bool, 5, 1> disable_io_check;
    BitField<uint8, bool, 4, 1> disable_parity_check;
    BitField<uint8, bool, 3, 1> enable_high_switches;   // Cassette motor on PC
    BitField<uint8, bool, 2, 1> enable_high_switches_2; // Unused on XT
    BitField<uint8, bool, 1, 1> speaker_enable;
    BitField<uint8, bool, 0, 1> speaker_gate;
  } m_port_b;

  // Port C values.
  union PortCValue
  {
    uint8 bits = 0;
    BitField<uint8, bool, 7, 1> parity_error_occurred;
    BitField<uint8, bool, 6, 1> io_error_occurred;
    BitField<uint8, bool, 5, 1> speaker_output;
    BitField<uint8, uint8, 4, 1> cassette_data_in;
    BitField<uint8, bool, 3, 1> switch_4;
    BitField<uint8, bool, 2, 1> switch_3;
    BitField<uint8, bool, 1, 1> switch_2;
    BitField<uint8, bool, 0, 1> switch_1;
  };

  static const uint32 KEYBOARD_BUFFER_SIZE = 32;
  uint8 m_output_buffer[KEYBOARD_BUFFER_SIZE] = {};
  uint32 m_output_buffer_pos = 0;

  uint8 m_pending_command = 0;
  uint8 m_input_buffer = 0;

  uint8 m_pending_keyboard_command = 0;
};

} // namespace HW
