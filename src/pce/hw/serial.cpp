#include "pce/hw/serial.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <algorithm>
Log_SetChannel(HW::Serial);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(Serial);
DEFINE_GENERIC_COMPONENT_FACTORY(Serial);
BEGIN_OBJECT_PROPERTY_MAP(Serial)
END_OBJECT_PROPERTY_MAP()

inline std::size_t GetFifoSize(Serial::Model model)
{
  switch (model)
  {
    case Serial::Model_16750:
      return 64;
    case Serial::Model_16550:
      return 16;
    case Serial::Model_8250:
    default:
      return 0;
  }
}

Serial::Serial(const String& identifier, Model model /* = Model_8250 */, u32 base_io_address /* = 0x03F8 */,
               u32 irq_number /* = 4 */, s32 base_rate /* = 1843200 */,
               const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_model(model), m_base_io_address(base_io_address), m_irq_number(irq_number),
    m_base_rate(base_rate), m_fifo_capacity(GetFifoSize(model)), m_fifo_interrupt_size(1), m_input_fifo_size(0),
    m_output_fifo_size(0), m_input_buffer(ExternalBufferSize), m_output_buffer(ExternalBufferSize)
{
}

Serial::~Serial() {}

bool Serial::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_interrupt_controller = system->GetComponentByType<InterruptController>();
  if (!m_interrupt_controller)
  {
    Log_ErrorPrintf("Failed to locate interrupt controller.");
    return false;
  }

  ConnectIOPorts(bus);

  // Create transfer event, but leave it deactivated by default.
  m_transfer_event =
    m_system->CreateClockedEvent("Serial Port '%s' Transfer", float(std::max(m_base_rate / 16, 1)), 1,
                                 std::bind(&Serial::HandleTransferEvent, this, std::placeholders::_2), false);

  return true;
}

void Serial::Reset()
{
  m_input_buffer_size = 0;
  m_output_buffer_size = 0;

  m_interrupt_enable_register.bits = 0;
  m_interrupt_identification_register.bits = 0;
  m_line_control_register.bits = 0;
  m_line_status_register.bits = 0;
  m_line_status_register.empty_receive_register = true;
  m_line_status_register.empty_transmit_register = true;

  if (m_modem_control_register.data_terminal_ready && m_data_terminal_ready_changed_callback)
    m_data_terminal_ready_changed_callback(false);
  m_modem_control_register.bits = 0;
  m_modem_status_register.bits = 0;
  m_scratch_register = 0;

  m_interrupt_state = 0;
  m_data_send_buffer = 0;
  m_data_receive_buffer = 0;
  m_input_fifo.fill(0);
  m_input_fifo_size = 0;
  m_output_fifo.fill(0);
  m_output_fifo_size = 0;
  m_fifo_interrupt_size = 1;

  // Start with transmit interrupt enabled, as we can send data.
  m_interrupt_state |= InterruptState_TransmitDataEmpty;

  // Deactivate event by default.
  if (m_transfer_event->IsActive())
    m_transfer_event->Deactivate();
}

bool Serial::LoadState(BinaryReader& reader)
{
  u32 serialization_id;
  if (!reader.SafeReadUInt32(&serialization_id) || serialization_id != SERIALIZATION_ID)
    return false;

  u32 model;
  if (!reader.SafeReadUInt32(&model) || model != u32(m_model))
    return false;

  u32 fifo_size;
  if (!reader.SafeReadUInt32(&fifo_size) || fifo_size != Truncate32(m_fifo_capacity))
    return false;

  u32 fifo_trigger_size, input_fifo_size, output_fifo_size;
  if (!reader.SafeReadUInt32(&fifo_trigger_size) || !reader.SafeReadUInt32(&input_fifo_size) ||
      !reader.SafeReadUInt32(&output_fifo_size) || input_fifo_size > fifo_size || output_fifo_size > fifo_size)
  {
    return false;
  }
  m_fifo_interrupt_size = fifo_trigger_size;
  m_input_fifo_size = input_fifo_size;
  m_output_fifo_size = output_fifo_size;
  if (fifo_size > 0)
  {
    reader.SafeReadBytes(m_input_fifo.data(), Truncate32(m_fifo_capacity));
    reader.SafeReadBytes(m_output_fifo.data(), Truncate32(m_fifo_capacity));
  }

  u32 input_buffer_size = 0;
  reader.SafeReadUInt32(&input_buffer_size);
  m_input_buffer_size = input_buffer_size;
  if (input_buffer_size > 0)
    reader.SafeReadBytes(m_input_buffer.data(), input_buffer_size);

  u32 output_buffer_size = 0;
  reader.SafeReadUInt32(&output_buffer_size);
  m_output_buffer_size = output_buffer_size;
  if (output_buffer_size > 0)
    reader.SafeReadBytes(m_output_buffer.data(), output_buffer_size);

  reader.SafeReadUInt16(&m_clock_divider);
  reader.SafeReadUInt8(&m_interrupt_enable_register.bits);
  reader.SafeReadUInt8(&m_interrupt_identification_register.bits);
  reader.SafeReadUInt8(&m_line_control_register.bits);
  reader.SafeReadUInt8(&m_modem_control_register.bits);
  reader.SafeReadUInt8(&m_modem_status_register.bits);
  reader.SafeReadUInt8(&m_line_status_register.bits);
  reader.SafeReadUInt8(&m_scratch_register);
  reader.SafeReadUInt8(&m_interrupt_state);
  reader.SafeReadUInt8(&m_data_send_buffer);
  reader.SafeReadUInt8(&m_data_receive_buffer);

  if (reader.GetErrorState())
    return false;

  UpdateInterruptState();
  UpdateTransmitEvent();
  return true;
}

bool Serial::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteUInt32(u32(m_model));
  writer.WriteUInt32(Truncate32(m_fifo_capacity));
  writer.WriteUInt32(Truncate32(m_fifo_interrupt_size));
  writer.WriteUInt32(Truncate32(m_input_fifo_size));
  writer.WriteUInt32(Truncate32(m_output_fifo_size));
  if (m_fifo_capacity > 0)
  {
    writer.WriteBytes(m_input_fifo.data(), Truncate32(m_fifo_capacity));
    writer.WriteBytes(m_output_fifo.data(), Truncate32(m_fifo_capacity));
  }

  writer.WriteUInt32(Truncate32(m_input_buffer_size));
  if (m_input_buffer_size > 0)
    writer.WriteBytes(m_input_buffer.data(), Truncate32(m_input_buffer_size));

  writer.WriteUInt32(Truncate32(m_output_buffer_size));
  if (m_output_buffer_size > 0)
    writer.WriteBytes(m_output_buffer.data(), Truncate32(m_output_buffer_size));

  writer.WriteUInt16(m_clock_divider);
  writer.WriteUInt8(m_interrupt_enable_register.bits);
  writer.WriteUInt8(m_interrupt_identification_register.bits);
  writer.WriteUInt8(m_line_control_register.bits);
  writer.WriteUInt8(m_modem_control_register.bits);
  writer.WriteUInt8(m_modem_status_register.bits);
  writer.WriteUInt8(m_line_status_register.bits);
  writer.WriteUInt8(m_scratch_register);
  writer.WriteUInt8(m_interrupt_state);
  writer.WriteUInt8(m_data_send_buffer);
  writer.WriteUInt8(m_data_receive_buffer);

  return !writer.InErrorState();
}

bool Serial::ReadData(void* ptr, size_t count)
{
  if (count > m_output_buffer_size)
    return false;

  std::memcpy(ptr, m_output_buffer.data(), count);
  m_output_buffer_size -= count;

  if (m_output_buffer_size > 0)
    std::memmove(m_output_buffer.data(), m_output_buffer.data() + count, m_output_buffer_size);

  return true;
}

bool Serial::WriteData(const void* ptr, size_t count)
{
  // When loopback is enabled, eat all data.
  if (m_modem_control_register.loopback_mode)
    return true;

  if ((m_input_buffer_size + count) > m_input_buffer.size())
    return false;

  std::memcpy(m_input_buffer.data() + m_input_buffer_size, ptr, count);
  m_input_buffer_size += count;
  UpdateTransmitEvent();
  return true;
}

void Serial::SetClearToSend(bool enabled)
{
  if (m_modem_status_register.clear_to_send == enabled)
    return;

  m_modem_status_register.clear_to_send = enabled;
  m_modem_status_register.delta_clear_to_send = true;
}

void Serial::ConnectIOPorts(Bus* bus)
{
  auto read_func = std::bind(&Serial::HandleIORead, this, std::placeholders::_1);
  auto write_func = std::bind(&Serial::HandleIOWrite, this, std::placeholders::_1, std::placeholders::_2);
  for (u16 offset = 0; offset <= 7; offset++)
  {
    bus->ConnectIOPortRead(Truncate16(m_base_io_address + offset), this, read_func);
    bus->ConnectIOPortWrite(Truncate16(m_base_io_address + offset), this, write_func);
  }
}

u8 Serial::HandleIORead(u16 address)
{
  u16 offset = Truncate16(address - m_base_io_address);
  // Log_DevPrintf("serial read offset %u", offset);

  // MSB/LSB of divisor
  if (offset <= 1 && m_line_control_register.divisor_access_latch)
    return Truncate8((offset == 0) ? m_clock_divider : (m_clock_divider >> 8));

  switch (offset)
  {
      // Data register
    case 0:
    {
      u8 value;
      size_t waiting_size;
      if (IsFifoEnabled())
      {
        if (!ReadFromFifo(m_input_fifo, m_input_fifo_size, &value))
        {
          Log_WarningPrintf("FIFO empty when read. Setting to zero.");
          value = 0;
        }

        waiting_size = m_input_fifo_size;
      }
      else
      {
        // if (m_line_status_register.empty_receive_register)
        // Log_WarningPrintf("Receive register empty when read.");

        value = m_data_receive_buffer;
        m_data_receive_buffer = 0;
        waiting_size = 0;
      }

      bool has_data = (waiting_size > 0);
      m_line_status_register.empty_receive_register = !has_data;
      m_line_status_register.data_ready = has_data;
      if (waiting_size < m_fifo_interrupt_size && m_interrupt_state & InterruptState_ReceivedDataAvailable)
      {
        m_interrupt_state &= ~InterruptBits_ReceivedDataAvailable;
        UpdateInterruptState();
      }

      // Transfer data again on the next tick.
      UpdateTransmitEvent();
      return value;
    }

      // Interrupt enable register
    case 1:
      return m_interrupt_enable_register.bits;

      // Interrupt identification register
    case 2:
    {
      const u8 value = m_interrupt_identification_register.bits;

      // Apparently we should clear the interrupt here?
      switch (m_interrupt_identification_register.type)
      {
        case InterruptBits_ModemStatusChange:
          m_interrupt_state &= ~InterruptState_ModemStatusChange;
          UpdateInterruptState();
          break;
        case InterruptBits_TransmitterDataEmpty:
          m_interrupt_state &= ~InterruptState_TransmitDataEmpty;
          UpdateInterruptState();
          break;
        case InterruptBits_ReceivedDataAvailable:
          m_interrupt_state &= ~InterruptState_ReceivedDataAvailable;
          UpdateInterruptState();
          break;
        case InterruptBits_LineStatusChange:
          m_interrupt_state &= ~InterruptState_LineStatus;
          UpdateInterruptState();
          break;
        default:
          break;
      }

      return value;
    }

      // Line control register
    case 3:
      return m_line_control_register.bits;

      // Modem control register
    case 4:
      return m_modem_control_register.bits;

      // Line status register
    case 5:
      return m_line_status_register.bits;

      // Modem status register
    case 6:
    {
      // Loopback enabled?
      u8 value;
      if (!m_modem_control_register.loopback_mode)
      {
        // Clear delta bits while we're at it.
        value = m_modem_status_register.bits;
        m_modem_status_register.delta_clear_to_send = false;
        m_modem_status_register.delta_data_set_ready = false;
        m_modem_status_register.delta_data_carrier_detect = false;
      }
      else
      {
        // If loopback is enabled, we need to populate this with our side.
        // TODO: This is a bit of a hack, I think we should raise interrupts based on this too?
        decltype(m_modem_status_register) temp;
        temp.bits = m_modem_status_register.bits;
        temp.data_set_ready = m_modem_control_register.data_terminal_ready;
        temp.clear_to_send = m_modem_control_register.request_to_send;
        temp.ring_indicator = m_modem_control_register.aux_output_1;
        temp.carrier_detect = m_modem_control_register.aux_output_2;
        temp.delta_clear_to_send = m_modem_status_register.clear_to_send ^ temp.clear_to_send;
        temp.delta_data_set_ready = m_modem_status_register.delta_data_set_ready ^ temp.data_set_ready;
        temp.trailing_edge_ring_indicator = m_modem_status_register.trailing_edge_ring_indicator ^ temp.ring_indicator;
        temp.delta_data_carrier_detect = m_modem_status_register.carrier_detect ^ temp.carrier_detect;
        value = temp.bits;
      }
      return value;
    }

      // Scratch register
    case 7:
      return (m_model >= Model_16550) ? m_scratch_register : 0;

    default:
      return 0xFF;
  }
}

void Serial::HandleIOWrite(u16 address, u8 value)
{
  u16 offset = Truncate16(address - m_base_io_address);
  Log_DebugPrintf("serial write offset %u 0x%02X", offset, ZeroExtend32(value));

  // MSB/LSB of divisor
  if (offset <= 1 && m_line_control_register.divisor_access_latch)
  {
    if (offset == 0)
      m_clock_divider = (m_clock_divider & 0xFF00) | ZeroExtend16(value);
    else
      m_clock_divider = (m_clock_divider & 0x00FF) | (ZeroExtend16(value) << 8);

    Log_DebugPrintf("Clock divider <-- 0x%02X", ZeroExtend32(m_clock_divider));
    UpdateTransmitEvent();
    return;
  }

  switch (offset)
  {
      // Data register
    case 0:
    {
      // Using FIFO?
      size_t waiting_size;
      if (IsFifoEnabled())
      {
        if (!WriteToFifo(m_output_fifo, m_output_fifo_size, value))
          Log_WarningPrintf("Serial FIFO full on write.");

        waiting_size = m_output_fifo_size;
      }
      else
      {
        m_data_send_buffer = value;
        if (!m_line_status_register.empty_transmit_register)
          Log_WarningPrintf("Serial transmit register not empty when written. Data lost.");

        waiting_size = 1;
      }

      // Clear interrupt.
      m_line_status_register.empty_transmit_register = false;
      if (waiting_size > m_fifo_interrupt_size && m_interrupt_state & InterruptState_TransmitDataEmpty)
      {
        m_interrupt_state &= ~InterruptState_TransmitDataEmpty;
        UpdateInterruptState();
      }

      // Transmit data on next tick.
      UpdateTransmitEvent();
    }
    break;

      // Interrupt enable register
    case 1:
      Log_DebugPrintf("Interrupt enable register <-- 0x%02X", ZeroExtend32(value));
      m_interrupt_enable_register.bits = value & INTERRUPT_ENABLE_REGISTER_MASK;
      if (m_line_status_register.empty_transmit_register)
        m_interrupt_state |= InterruptState_TransmitDataEmpty;
      UpdateInterruptState();
      break;

      // FIFO control register
    case 2:
    {
      Log_DebugPrintf("FIFO control register <-- 0x%02X", ZeroExtend32(value));

      // Ignore writes to this register on an 8250.
      if (m_model < Model_16550)
        return;

      // We don't store this anywhere, the fifo state is in the IIR
      bool fifo_enable = !!(value & 0x01);
      bool clear_receive_fifo = !!(value & 0x02);
      bool clear_transmit_fifo = !!(value & 0x04);
      // bool dma_mode_select = !!(value & 0x08);
      bool enable_fifo64 = !!(value & 0x20);
      u8 fifo_trigger_bits = value >> 6;
      if (fifo_enable != m_interrupt_identification_register.fifo_enabled)
      {
        // FIFO enable changed. We need to clear the input/output fifos.
        clear_receive_fifo = true;
        clear_transmit_fifo = true;

        // Update IIR with fifo state.
        m_interrupt_identification_register.fifo_enabled = fifo_enable;
        m_interrupt_identification_register.fifo_usable = fifo_enable;
        m_interrupt_identification_register.fifo64_enabled = (m_model >= Model_16750 && enable_fifo64);
      }

      if (clear_receive_fifo)
      {
        m_input_fifo_size = 0;
        m_input_fifo.fill(0);
      }
      if (clear_transmit_fifo)
      {
        m_output_fifo_size = 0;
        m_output_fifo.fill(0);
      }

      // Update fifo trigger size
      switch (fifo_trigger_bits)
      {
        case 0:
          m_fifo_interrupt_size = 1;
          break;
        case 1:
          m_fifo_interrupt_size = 4;
          break;
        case 2:
          m_fifo_interrupt_size = 8;
          break;
        case 3:
          m_fifo_interrupt_size = 14;
          break;
      }

      // If the fifo is disabled, force an interrupt size of 1
      if (!fifo_enable)
        m_fifo_interrupt_size = 1;

      // Update interrupt states, since the trigger size could be different.
      if (m_input_fifo_size >= m_fifo_interrupt_size)
        m_interrupt_state |= InterruptBits_ReceivedDataAvailable;
      else
        m_interrupt_state &= ~InterruptBits_ReceivedDataAvailable;
      if (m_output_fifo_size <= m_fifo_interrupt_size)
        m_interrupt_state |= InterruptBits_TransmitterDataEmpty;
      else
        m_interrupt_state &= ~InterruptBits_TransmitterDataEmpty;
      UpdateInterruptState();
    }
    break;

      // Line control register
    case 3:
      Log_DebugPrintf("Line control register <-- 0x%02X", ZeroExtend32(value));
      m_line_control_register.bits = value;
      break;

      // Modem control register
    case 4:
    {
      Log_DebugPrintf("Modem control register <-- 0x%02X", ZeroExtend32(value));

      decltype(m_modem_control_register) changed_bits;
      changed_bits.bits = m_modem_control_register.bits ^ value;
      m_modem_control_register.bits = value;

      if (changed_bits.data_terminal_ready && m_data_terminal_ready_changed_callback)
        m_data_terminal_ready_changed_callback(m_modem_control_register.data_terminal_ready);

      if (changed_bits.request_to_send && m_request_to_send_callback)
        m_request_to_send_callback(m_modem_control_register.request_to_send);
    }
    break;

      // ???
    case 5:
      Log_WarningPrintf("BAR5 register <-- 0x%02X", ZeroExtend32(value));
      break;

      // ???
    case 6:
      Log_WarningPrintf("BAR6 register <-- 0x%02X", ZeroExtend32(value));
      break;

      // Scratch register
    case 7:
    {
      Log_DebugPrintf("Scratch register <-- 0x%02X", ZeroExtend32(value));
      if (m_model >= Model_16550)
        m_scratch_register = value;
    }
    break;
  }
}

bool Serial::ReadFromFifo(std::array<byte, MAX_FIFO_SIZE>& fifo_data, size_t& fifo_size, u8* value)
{
  if (fifo_size == 0)
    return false;

  *value = fifo_data[0];
  fifo_size--;
  if (fifo_size > 0)
    std::memmove(fifo_data.data(), fifo_data.data() + 1, fifo_size);

  return true;
}

bool Serial::WriteToFifo(std::array<byte, MAX_FIFO_SIZE>& fifo_data, size_t& fifo_size, u8 value)
{
  if ((fifo_size + 1) >= GetEffectiveFifoSize())
    return false;

  fifo_data[fifo_size++] = value;
  return true;
}

CycleCount Serial::CalculateCyclesPerByte() const
{
  // uint32 baud_rate = std::max(115200u / std::max(ZeroExtend32(m_clock_divider), 1u), 1u);
  return 8 * std::max(CycleCount(m_clock_divider), CycleCount(1));
}

void Serial::HandleTransferEvent(CycleCount cycles)
{
  bool fire_callback = false;
  u32 num_bytes = std::max(u32(cycles / CalculateCyclesPerByte()), UINT32_C(1));
  Log_DebugPrintf("Serial execute cycles %u bytes %u", u32(cycles), num_bytes);

  while ((num_bytes--) > 0)
  {
    // Do we have data to transmit?
    if (!m_line_status_register.empty_transmit_register)
    {
      u8 data;
      size_t remaining_size;
      if (IsFifoEnabled())
      {
        if (!ReadFromFifo(m_output_fifo, m_output_fifo_size, &data))
        {
          Panic("Non-empty transmit register with empty fifo");
          data = 0;
        }
        remaining_size = m_output_fifo_size;
      }
      else
      {
        data = m_data_send_buffer;
        remaining_size = 0;
      }

      // Loopback mode enabled?
      if (!m_modem_control_register.loopback_mode)
      {
        // Do we have space in our output buffer?
        if (m_output_buffer_size == m_output_buffer.size())
        {
          // Drop a byte.
          Log_WarningPrintf("Lost byte from output buffer because it wasn't read");
          m_output_buffer_size--;
        }

        // Write to the output buffer.
        m_output_buffer[m_output_buffer_size++] = data;
        Log_DebugPrintf("Serial sent byte: 0x%02X", ZeroExtend32(data));
      }
      else
      {
        // If loopback is enabled, we enqueue this data on the read side.
        WriteData(&data, sizeof(data));
      }

      // Fire interrupt if enabled.
      m_line_status_register.empty_transmit_register = (remaining_size == 0);
      if (remaining_size <= m_fifo_interrupt_size)
      {
        m_interrupt_state |= InterruptState_TransmitDataEmpty;
        UpdateInterruptState();
      }
      fire_callback = true;
    }

    // Do we have data to receive?
    if (m_input_buffer_size > 0)
    {
      u8 data = m_input_buffer[0];
      m_input_buffer_size--;
      if (m_input_buffer_size > 0)
        std::memmove(m_input_buffer.data(), m_input_buffer.data() + 1, m_input_buffer_size);

      Log_DebugPrintf("Serial received byte: 0x%02X", ZeroExtend32(data));

      // Add to input fifo.
      size_t waiting_size;
      if (IsFifoEnabled())
      {
        if (!WriteToFifo(m_input_fifo, m_input_fifo_size, data))
          Log_WarningPrintf("Receive FIFO full, dropping data.");

        waiting_size = m_input_fifo_size;
      }
      else
      {
        if (!m_line_status_register.empty_receive_register)
          Log_WarningPrintf("Data not read by guest, overwriting receive buffer");

        m_data_receive_buffer = data;
        waiting_size = 1;
      }

      // Fire interrupt if enabled.
      m_line_status_register.data_ready = true;
      m_line_status_register.empty_receive_register = false;
      if (waiting_size >= m_fifo_interrupt_size)
      {
        m_interrupt_state |= InterruptState_ReceivedDataAvailable;
        UpdateInterruptState();
      }
    }
  }

  // Fire callback if we wrote data to the output buffer.
  if (fire_callback && m_output_data_ready_callback)
    m_output_data_ready_callback(m_output_buffer_size);

  // We only re-queue here when there is data in the input buffer.
  // When more data is to be transmitted we'll re-queue the event.
  if (m_input_buffer_size == 0 && m_line_status_register.empty_transmit_register)
    m_transfer_event->Deactivate();
}

void Serial::UpdateInterruptState()
{
  // Start with highest priority first
  if (m_interrupt_state & InterruptState_LineStatus && m_interrupt_enable_register.break_or_error)
    m_interrupt_identification_register.type = InterruptBits_LineStatusChange;
  else if (m_interrupt_state & InterruptState_ReceivedDataAvailable && m_interrupt_enable_register.data_available)
    m_interrupt_identification_register.type = InterruptBits_ReceivedDataAvailable;
  else if (m_interrupt_state & InterruptState_TransmitDataEmpty && m_interrupt_enable_register.transmitter_empty)
    m_interrupt_identification_register.type = InterruptBits_TransmitterDataEmpty;
  else if (m_interrupt_state & InterruptState_ModemStatusChange && m_interrupt_enable_register.status_change)
    m_interrupt_identification_register.type = InterruptBits_ModemStatusChange;
  else
  {
    m_interrupt_identification_register.pending = true;
    m_interrupt_controller->LowerInterrupt(m_irq_number);
    return;
  }

  m_interrupt_identification_register.pending = false;
  m_interrupt_controller->RaiseInterrupt(m_irq_number);
}

void Serial::UpdateTransmitEvent()
{
  // Check whether the event should be active at all
  if (m_input_buffer_size == 0 && m_line_status_register.empty_transmit_register)
  {
    if (m_transfer_event->IsActive())
      m_transfer_event->Deactivate();

    return;
  }

  // Calculate the baud rate
  CycleCount cycles_per_byte = CalculateCyclesPerByte();

  // If the baud rate has changed, this will be slightly inaccurate compared to hardware.
  // But seriously, who changes a baud rate while a transfer is in progress in the first place?
  if (m_transfer_event->IsActive())
  {
    if (m_transfer_event->GetInterval() != cycles_per_byte)
      m_transfer_event->Reschedule(cycles_per_byte);
  }
  else
  {
    m_transfer_event->Queue(cycles_per_byte);
  }
}

} // namespace HW
