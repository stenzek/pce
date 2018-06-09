#include "pce/hw/cmos.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Timestamp.h"
#include "pce/bus.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
Log_SetChannel(HW::CMOS);

namespace HW {

CMOS::CMOS() : m_rtc_clock("CMOS", 32768) {}

CMOS::~CMOS() {}

void CMOS::Initialize(System* system, Bus* bus)
{
  std::fill(m_data.begin(), m_data.end(), uint8(0));

  m_system = system;
  ConnectIOPorts(bus);

  m_rtc_clock.SetManager(system->GetTimingManager());
  m_rtc_interrupt_event =
    m_rtc_clock.NewEvent("RTC Interrupt", 1, std::bind(&CMOS::RTCInterruptEvent, this, std::placeholders::_2), false);

  //     static const byte values[] = {
  //         0x11, 0x00, 0x49, 0x00, 0x17, 0x00, 0x01, 0x03, 0x12, 0x12, 0x06, 0x00,
  //         0x40, 0x80, 0x28, 0x00, 0x44, 0x22, 0x00, 0x90, 0x45, 0x80, 0x02, 0x00,
  //         0x04, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0a, 0x0a, 0x60,
  //         0x07, 0x00, 0xe3, 0xe3, 0xba, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xb6,
  //         0x00, 0x04, 0x20, 0x80, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00,
  //         0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //         0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  //     };
  //     std::memcpy(m_data.data(), values, sizeof(values));

  // Leaving alarm variables to zero causes divide by zero
  // m_data[0x01] = 0x01;
  // m_data[0x03] = 0x01;
  // m_data[0x05] = 0x01;

  // RTC has power
  m_data[0x0D] |= (1 << 7);

  // RTC didn't lose power
  m_data[0x0E] |= (1 << 7);

  // Set the RTC base frequency to 32768hz
  // Set the RTC interrupt divider to the max so we can run as fast as possible
  m_data[0x0A] |= (2 << 4);
  m_data[0x0A] |= 6;
  UpdateRTCFrequency();
}

void CMOS::Reset()
{
  m_nmi_enabled = false;
  m_index_register = 0;
}

bool CMOS::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.ReadBytes(m_data.data(), Truncate32(m_data.size()));
  m_index_register = reader.ReadUInt8();
  m_nmi_enabled = reader.ReadBool();
  UpdateRTCFrequency();
  return true;
}

bool CMOS::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteBytes(m_data.data(), Truncate32(m_data.size()));
  writer.WriteUInt8(m_index_register);
  writer.WriteBool(m_nmi_enabled);
  return true;
}

uint16 CMOS::GetWordVariable(uint8 base_index) const
{
  uint16 value;
  value = ZeroExtend16(m_data[base_index + 0]);
  value |= (ZeroExtend16(m_data[base_index + 1]) << 8);
  return value;
}

void CMOS::SetWordVariable(uint8 base_index, uint16 value)
{
  m_data[base_index + 0] = Truncate8(value);
  m_data[base_index + 1] = Truncate8(value >> 8);
}

void CMOS::SetFloppyType(uint8 index, FDC::DriveType type)
{
  Assert(index < 2);

  // 0000 - no drive
  // 0001 - 360KB
  // 0010 - 1.2MB
  // 0011 - 720KB
  // 0100 - 1.44MB

  uint8 floppy_type;
  switch (type)
  {
    case FDC::DriveType_5_25:
      floppy_type = 0b0010;
      break;
    case FDC::DriveType_3_5:
      floppy_type = 0b0100;
      break;
    default:
      floppy_type = 0b0000;
      break;
  }

  if (index == 0)
  {
    m_data[0x10] &= 0b00001111;
    m_data[0x10] |= (floppy_type << 4);
  }
  else
  {
    m_data[0x10] &= 0b11110000;
    m_data[0x10] |= floppy_type;
  }
}

void CMOS::SetFloppyCount(uint32 count)
{
  m_data[0x14] &= 0b00111111;
  m_data[0x14] |= ((count == 2) ? 0b01 : 0b00) << 6;
  m_data[0x14] |= 0x01;
}

void CMOS::ConnectIOPorts(Bus* bus)
{
  // We synchronize when needed.
  bus->ConnectIOPortRead(IOPORT_INDEX_REGISTER, this,
                         std::bind(&CMOS::IOWriteReadRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(IOPORT_INDEX_REGISTER, this,
                          std::bind(&CMOS::IOWriteIndexRegister, this, std::placeholders::_2));
  bus->ConnectIOPortRead(IOPORT_DATA_PORT, this, std::bind(&CMOS::IOReadDataPort, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(IOPORT_DATA_PORT, this, std::bind(&CMOS::IOWriteDataPort, this, std::placeholders::_2));
}

bool CMOS::HandleKnownCMOSRead(uint8 index, uint8* value)
{
  // RTC registers
  if (index <= 0x32)
  {
    Timestamp time = Timestamp::Now();
    Timestamp::ExpandedTime exp_time = time.AsExpandedTime();

    switch (index)
    {
      case 0x00: // Current second
        *value = DecimalToBCD(Truncate8(exp_time.Second));
        return true;
      case 0x02: // Current minute
        *value = DecimalToBCD(Truncate8(exp_time.Minute));
        return true;
      case 0x04: // Current hour
        *value = DecimalToBCD(Truncate8(exp_time.Hour));
        return true;

      case 0x06: // Day of week
        *value = DecimalToBCD(Truncate8(exp_time.DayOfWeek + 1));
        return true;
      case 0x07: // Day of month
        *value = DecimalToBCD(Truncate8(exp_time.DayOfMonth));
        return true;
      case 0x08: // Month
        *value = DecimalToBCD(Truncate8(exp_time.Month));
        return true;
      case 0x09: // Year in BCD, least-signiciant two digits
        *value = DecimalToBCD(Truncate8(exp_time.Year % 100));
        return true;
      case 0x32: // Year in BCD, most-significant two digits
        *value = DecimalToBCD(Truncate8(exp_time.Year / 100));
        return true;

      case 0x01: // Alarm second
      case 0x03: // Alarm minute
      case 0x05: // Alarm hour
        return false;

      case 0x0A: // Status register A
      {
        // Flip the update-in-progress bit to fool code that polls this
        // TODO: implement properly
        m_data[0x0A] ^= 0x80;
        *value = m_data[0x0A];
        return true;
      }

      case RTC_REGISTER_STATUS_REGISTER_C:
      {
        // Clear interrupt bits once read back
        *value = m_data[RTC_REGISTER_STATUS_REGISTER_C];
        m_data[RTC_REGISTER_STATUS_REGISTER_C] &= ~(RTC_SRC_PERIODIC_INTERRUPT);
        UpdateRTCFrequency();
        return true;
      }

      default:
        return false;
    }
  }

  return false;
}

bool CMOS::HandleKnownCMOSWrite(uint8 index, uint8 value)
{
  if (index >= 0x0A && index <= 0x0A)
  {
    switch (index)
    {
      case 0x0A: // Status register A
      {
        m_data[0x0A] = value;
        UpdateRTCFrequency();
        return true;
      }

      default:
        return false;
    }
  }

  return false;
}

void CMOS::IOWriteReadRegister(uint8* value)
{
  *value = (uint8(m_nmi_enabled) << 7) | m_index_register;
}

void CMOS::IOWriteIndexRegister(uint8 value)
{
  m_nmi_enabled = !!(value & 0x80);
  m_index_register = (value & 0x7F);

  // Log_DevPrintf("Index register <- 0x%02X", m_index_register);
}

void CMOS::IOReadDataPort(uint8* value)
{
  // Handle RTC and special stuff.
  if (!HandleKnownCMOSRead(m_index_register, value))
    *value = m_data[m_index_register];

  Log_DevPrintf("Read register 0x%02X, value=0x%02X", ZeroExtend32(m_index_register), ZeroExtend32(*value));
}

void CMOS::IOWriteDataPort(uint8 value)
{
  Log_DevPrintf("Write register 0x%02X value=0x%02X", ZeroExtend32(m_index_register), ZeroExtend32(value));

  // Handle RTC and special stuff.
  if (!HandleKnownCMOSWrite(m_index_register, value))
    m_data[m_index_register] = value;
}

void CMOS::UpdateRTCFrequency()
{
  static const uint32 base_rate_table[] = {4194304, 1048576, 32768, 16384};

  uint8 base_rate_index = ((m_data[0x0A] >> 4) & 3);
  uint8 rate_divider = (m_data[0x0A] & 15);

  uint32 base_rate = base_rate_table[base_rate_index];
  uint32 interrupt_rate = base_rate >> rate_divider;

  Log_DevPrintf("Base rate = %u (%u hz), rate divider = %u, interrupt rate = %u hz", ZeroExtend32(base_rate_index),
                base_rate, ZeroExtend32(rate_divider), interrupt_rate);

  CycleCount interval = (interrupt_rate > 0) ? static_cast<CycleCount>(base_rate / interrupt_rate) : 1;

  // Only schedule the event when the interrupt flag is clear.
  if (!(m_data[RTC_REGISTER_STATUS_REGISTER_C] & RTC_SRC_PERIODIC_INTERRUPT))
  {
    if (!m_rtc_interrupt_event->IsActive())
      m_rtc_interrupt_event->Queue(interval);
    else if (m_rtc_interrupt_event->GetInterval() != interval)
      m_rtc_interrupt_event->Reschedule(interval);
  }
  else if (m_rtc_interrupt_event->IsActive())
  {
    m_rtc_interrupt_event->Deactivate();
  }
}

void CMOS::RTCInterruptEvent(CycleCount cycles)
{
  // We don't trigger the interrupt unless the flag has been read and cleared.
  if (m_data[RTC_REGISTER_STATUS_REGISTER_B] & RTC_SRB_PERIODIC_INTERRUPT_ENABLE &&
      !(m_data[RTC_REGISTER_STATUS_REGISTER_C] & RTC_SRC_PERIODIC_INTERRUPT))
  {
    m_system->GetInterruptController()->TriggerInterrupt(RTC_INTERRUPT);
  }

  m_data[RTC_REGISTER_STATUS_REGISTER_C] |= RTC_SRC_PERIODIC_INTERRUPT;
  m_rtc_interrupt_event->Deactivate();
}

} // namespace HW