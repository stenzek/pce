#pragma once

#include <array>
#include <vector>

#include "common/bitfield.h"
#include "common/clock.h"
#include "pce/component.h"
#include "pce/interrupt_controller.h"
#include "pce/types.h"

namespace HW {

// TODO: Use correct frequency for base rate.
// TODO: Mask bits away depending on data bit count.

class Serial : public Component
{
  DECLARE_OBJECT_TYPE_INFO(Serial, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(Serial);
  DECLARE_OBJECT_PROPERTY_MAP(Serial);

public:
  // Input/output buffer sizes. These are external to the chip emulation, allowing senders to buffer large
  // amounts of data without multiple calls into this class.
  static constexpr size_t ExternalBufferSize = 128 * 1024;

  // Serial IC types
  enum Model : u32
  {
    Model_8250,
    Model_16550,
    Model_16750
  };

  Serial(const String& identifier, Model model = Model_8250, u32 base_io_address = 0x03F8, u32 irq_number = 4,
         s32 base_rate = 1843200, const ObjectTypeInfo* type_info = &s_type_info);
  ~Serial();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  // Checks if there is data in the external write buffer.
  size_t GetDataSize() const { return m_output_buffer_size; }
  bool HasData() const { return GetDataSize() > 0; }

  // Reads data from the output buffer.
  bool ReadData(void* ptr, size_t count);

  // Writes data to the input buffer.
  // If the buffer is full (because the PC hasn't read the data yet) false is returned.
  bool WriteData(const void* ptr, size_t count);

  // Sets a callback that is invoked when data is first added to the output buffer.
  using DataReadyCallback = std::function<void(size_t count)>;
  void SetDataReadyCallback(DataReadyCallback callback) { m_output_data_ready_callback = std::move(callback); }

  // Called when data terminal ready changes.
  using DataTerminalReadyChangedCallback = std::function<void(bool)>;
  void SetDataTerminalReadyChangedCallback(DataTerminalReadyChangedCallback callback)
  {
    m_data_terminal_ready_changed_callback = std::move(callback);
  }

  // Called when request to send changes.
  using RequestToSendCallback = std::function<void(bool)>;
  void SetRequestToSendChangedCallback(RequestToSendCallback callback)
  {
    m_request_to_send_callback = std::move(callback);
  }

  // Set clear to send on the remote side.
  void SetClearToSend(bool enabled);

private:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('8', '2', '5', '0');
  static constexpr CycleCount CLOCK_FREQUENCY = 115200;
  static constexpr size_t MAX_FIFO_SIZE = 64;

  static constexpr u8 INTERRUPT_ENABLE_REGISTER_MASK = 0x0F;
  static constexpr u8 INTERRUPT_IDENTIFICATION_REGISTER_MASK = 0xFF;

  enum InterruptBits : u8
  {
    InterruptBits_ModemStatusChange = 0,
    InterruptBits_TransmitterDataEmpty = 1,
    InterruptBits_ReceivedDataAvailable = 2,
    InterruptBits_LineStatusChange = 3,
    InterruptBits_CharacterTimeout = 5
  };

  void ConnectIOPorts(Bus* bus);
  void HandleIORead(u16 address, u8* value);
  void HandleIOWrite(u16 address, u8 value);

  bool IsFifoEnabled() const { return m_interrupt_identification_register.fifo_enabled; }
  size_t GetEffectiveFifoSize() const
  {
    return IsFifoEnabled() ? (m_interrupt_identification_register.fifo64_enabled ? 64 : 16) : 0;
  }
  bool ReadFromFifo(std::array<byte, MAX_FIFO_SIZE>& fifo_data, size_t& fifo_size, u8* value);
  bool WriteToFifo(std::array<byte, MAX_FIFO_SIZE>& fifo_data, size_t& fifo_size, u8 value);

  CycleCount CalculateCyclesPerByte() const;

  void HandleTransferEvent(CycleCount cycles);

  void UpdateInterruptState();
  void UpdateTransmitEvent();

  InterruptController* m_interrupt_controller = nullptr;

  Clock m_clock;
  Model m_model;

  u32 m_base_io_address;
  u32 m_irq_number;

  // Read/write fifos
  std::array<byte, MAX_FIFO_SIZE> m_input_fifo;
  std::array<byte, MAX_FIFO_SIZE> m_output_fifo;
  size_t m_fifo_capacity;
  size_t m_fifo_interrupt_size;
  size_t m_input_fifo_size;
  size_t m_output_fifo_size;

  // External data buffers - this is from the point of view of the emulator, not the PC
  // TODO: Possibly use circular buffers here.. but is it worth it?
  std::vector<byte> m_input_buffer;
  std::vector<byte> m_output_buffer;
  size_t m_input_buffer_size = 0;
  size_t m_output_buffer_size = 0;

  // Data ready callback, executed every time data is written to the external buffers
  DataReadyCallback m_output_data_ready_callback;
  DataTerminalReadyChangedCallback m_data_terminal_ready_changed_callback;
  RequestToSendCallback m_request_to_send_callback;

  // Transfer event, only active when data is being transmitted/received
  TimingEvent::Pointer m_transfer_event;

  // Clock divider
  u16 m_clock_divider = 1; // BAR+0/1 with DLAB=1

  // Registers
  union
  {
    u8 bits = 0;

    BitField<u8, bool, 0, 1> data_available;
    BitField<u8, bool, 1, 1> transmitter_empty;
    BitField<u8, bool, 2, 1> break_or_error;
    BitField<u8, bool, 3, 1> status_change;
    BitField<u8, bool, 4, 1> sleep_mode;     // 16750+
    BitField<u8, bool, 5, 1> low_power_mode; // 16750+
  } m_interrupt_enable_register;             // BAR+1

  union
  {
    u8 bits = 0;

    BitField<u8, bool, 0, 1> pending; // active low
    BitField<u8, InterruptBits, 1, 3> type;
    BitField<u8, bool, 5, 1> fifo64_enabled;
    BitField<u8, bool, 6, 1> fifo_usable; // unusable if fifo_enabled=1 and this=0
    BitField<u8, bool, 7, 1> fifo_enabled;
  } m_interrupt_identification_register; // BAR+2

  union
  {
    u8 bits = 0;

    BitField<u8, u8, 0, 2> data_bits;
    BitField<u8, u8, 2, 1> stop_bits;
    BitField<u8, u8, 3, 3> parity;
    BitField<u8, bool, 6, 1> break_enable;
    BitField<u8, bool, 7, 1> divisor_access_latch;

    // Helper methods
    u8 NumDataBits() const { return data_bits + 3; }

  } m_line_control_register; // BAR+3

  union
  {
    u8 bits = 0;

    BitField<u8, bool, 0, 1> data_terminal_ready;
    BitField<u8, bool, 1, 1> request_to_send;
    BitField<u8, bool, 2, 1> aux_output_1;
    BitField<u8, bool, 3, 1> aux_output_2;
    BitField<u8, bool, 4, 1> loopback_mode;
    BitField<u8, bool, 5, 1> autoflow_control_enabled;
  } m_modem_control_register;

  union
  {
    u8 bits = 0;

    BitField<u8, bool, 0, 1> delta_clear_to_send;
    BitField<u8, bool, 1, 1> delta_data_set_ready;
    BitField<u8, bool, 2, 1> trailing_edge_ring_indicator;
    BitField<u8, bool, 3, 1> delta_data_carrier_detect;
    BitField<u8, bool, 4, 1> clear_to_send;
    BitField<u8, bool, 5, 1> data_set_ready;
    BitField<u8, bool, 6, 1> ring_indicator;
    BitField<u8, bool, 7, 1> carrier_detect;
  } m_modem_status_register; // BAR+4

  union
  {
    u8 bits = 0;

    BitField<u8, bool, 7, 1> fifo_error;
    BitField<u8, bool, 6, 1> empty_receive_register;
    BitField<u8, bool, 5, 1> empty_transmit_register;
    BitField<u8, bool, 4, 1> break_interrupt;
    BitField<u8, bool, 3, 1> framing_error;
    BitField<u8, bool, 2, 1> parity_error;
    BitField<u8, bool, 1, 1> overrun_error;
    BitField<u8, bool, 0, 1> data_ready;
  } m_line_status_register; // BAR+5

  u8 m_scratch_register = 0; // BAR+7

  // Internal interrupt types
  enum InterruptState : u8
  {
    InterruptState_LineStatus = (1 << 0),
    InterruptState_ReceivedDataAvailable = (1 << 1),
    InterruptState_TransmitDataEmpty = (1 << 2),
    InterruptState_ModemStatusChange = (1 << 3),
  };
  u8 m_interrupt_state = 0;

  // TODO: Use fifos instead here
  u8 m_data_send_buffer = 0;
  u8 m_data_receive_buffer = 0;
};

} // namespace HW
