#include "pce/hw/ds12887.h"
#include "YBaseLib/AutoReleasePtr.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Timestamp.h"
#include "pce/bus.h"
#include "pce/hw/fdc.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <ctime>
Log_SetChannel(HW::DS12887);

namespace HW {

DEFINE_OBJECT_TYPE_INFO(DS12887);
BEGIN_OBJECT_PROPERTY_MAP(DS12887)
PROPERTY_TABLE_MEMBER_UINT("RAMSize", 0, offsetof(DS12887, m_size), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("IRQ", 0, offsetof(DS12887, m_irq), nullptr, 0)
PROPERTY_TABLE_MEMBER_BOOL("SyncTimeOnReset", 0, offsetof(DS12887, m_sync_time_on_reset), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("SaveFileSuffix", 0, offsetof(DS12887, m_save_filename_suffix), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

DS12887::DS12887(const String& identifier, u32 size /* = 128 */, u32 irq /* = 8 */,
                 const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_size(size), m_irq(irq)
{
}

DS12887::~DS12887() = default;

bool DS12887::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_interrupt_controller = system->GetComponentByType<InterruptController>();
  if (!m_interrupt_controller)
  {
    Log_ErrorPrintf("Failed to locate interrupt controller");
    return false;
  }

  m_data.resize(m_size);
  std::fill(m_data.begin(), m_data.end(), u8(0));

  ConnectIOPorts(bus);

  m_rtc_interrupt_event = system->GetTimingManager()->CreateFrequencyEvent(
    "RTC Interrupt", 32768.0f, std::bind(&DS12887::RTCInterruptEvent, this), false);

  // Set up file saving.
  m_save_ram_event = system->GetTimingManager()->CreateMillisecondIntervalEvent(
    "RAM Save Event", SAVE_TO_FILE_DELAY_MS, std::bind(&DS12887::SaveRAMEvent, this), false);
  m_save_filename.Format("%s%s", m_system->GetConfigBasePath().GetCharArray(), m_save_filename_suffix.GetCharArray());
  Log_DevPrintf("RTC saving to file '%s'", m_save_filename.GetCharArray());

  // Index register mask clears the high bit for older RTCs.
  if (m_size > 128)
    m_index_register_mask = 0xFF;
  else if (m_size > 64)
    m_index_register_mask = 0x7F;
  else
    m_index_register_mask = 0x3F;

  // Load RAM, this may fail
  if (!LoadRAM())
  {
    Log_WarningPrintf("Failed to load RAM from %s, using default values", m_save_filename.GetCharArray());
    ResetRAM();
  }
  Log_InfoPrintf("RTC time is %04u-%02u-%02u %02u:%02u:%02u",
                 (u32(ReadClockRegister(RTC_REGISTER_CENTURY)) * 100) + ReadClockRegister(RTC_REGISTER_YEAR),
                 ReadClockRegister(RTC_REGISTER_MONTH) + u32(1), ReadClockRegister(RTC_REGISTER_DATE_OF_MONTH) + u32(1),
                 ReadClockRegister(RTC_REGISTER_HOURS), ReadClockRegister(RTC_REGISTER_MINUTES),
                 ReadClockRegister(RTC_REGISTER_SECONDS));

  UpdateRTCFrequency();
  return true;
}

void DS12887::Reset()
{
  m_index_register = 0;

  // Interrupt bits are cleared on reset.
  m_data[RTC_REGISTER_STATUS_REGISTER_C] &=
    ~(RTC_SRC_PERIODIC_INTERRUPT | RTC_SRC_ALARM_INTERRUPT | RTC_SRC_UPDATE_ENDED_INTERRUPT);
  UpdateInterruptState();

  if (m_sync_time_on_reset)
    SynchronizeTimeWithHost();

  m_last_clock_update_time = m_system->GetTimingManager()->GetTotalEmulatedTime();
  m_clock_partial_time = 0;
}

bool DS12887::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.ReadBytes(m_data.data(), Truncate32(m_data.size()));
  m_index_register = reader.ReadUInt8();
  m_last_clock_update_time = reader.ReadInt64();
  m_clock_partial_time = reader.ReadInt64();
  UpdateRTCFrequency();
  return true;
}

bool DS12887::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteBytes(m_data.data(), Truncate32(m_data.size()));
  writer.WriteUInt8(m_index_register);
  writer.WriteInt64(m_last_clock_update_time);
  writer.WriteInt64(m_clock_partial_time);
  return true;
}

void DS12887::SynchronizeTimeWithHost()
{
  const std::time_t host_time_t = time(nullptr);
  tm host_time;
#ifdef Y_PLATFORM_WINDOWS
  localtime_s(&host_time, &host_time_t);
#else
  localtime_r(&host_time_t, &host_time);
#endif
  // Handle 24 hour time.
  const u8 hours = Truncate8((m_data[RTC_REGISTER_STATUS_REGISTER_B] & RTC_SRB_24_HOUR_MODE) ?
                               host_time.tm_hour :
                               ((host_time.tm_hour % 12) + 1 + ((host_time.tm_hour / 12) * 0x80)));

  WriteClockRegister(RTC_REGISTER_SECONDS, Truncate8(host_time.tm_sec));
  WriteClockRegister(RTC_REGISTER_MINUTES, Truncate8(host_time.tm_min));
  WriteClockRegister(RTC_REGISTER_HOURS, hours);
  WriteClockRegister(RTC_REGISTER_DAY_OF_WEEK, Truncate8(host_time.tm_wday + 1));
  WriteClockRegister(RTC_REGISTER_DATE_OF_MONTH, Truncate8(host_time.tm_mday));
  WriteClockRegister(RTC_REGISTER_MONTH, Truncate8(host_time.tm_mon + 1));
  WriteClockRegister(RTC_REGISTER_YEAR, Truncate8(host_time.tm_year % 100));
  WriteClockRegister(RTC_REGISTER_CENTURY, Truncate8((host_time.tm_year + 1900) / 100));
}

u16 DS12887::GetConfigWordVariable(u8 base_index) const
{
  u16 value;
  value = ZeroExtend16(m_data[base_index + 0]);
  value |= (ZeroExtend16(m_data[base_index + 1]) << 8);
  return value;
}

void DS12887::SetConfigWordVariable(u8 base_index, u16 value)
{
  m_data[base_index + 0] = Truncate8(value);
  m_data[base_index + 1] = Truncate8(value >> 8);
}

void DS12887::SetConfigFloppyType(u32 index, u32 type)
{
  Assert(index < 2);

  // 0000 - no drive
  // 0001 - 360KB
  // 0010 - 1.2MB
  // 0011 - 720KB
  // 0100 - 1.44MB

  u8 floppy_type;
  switch (type)
  {
    case Floppy::DriveType_5_25:
      floppy_type = 0b0010;
      break;
    case Floppy::DriveType_3_5:
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

void DS12887::SetConfigFloppyCount(u32 count)
{
  m_data[0x14] &= 0b00111111;
  m_data[0x14] |= ((count == 2) ? 0b01 : 0b00) << 6;
  m_data[0x14] |= 0x01;
}

void DS12887::ConnectIOPorts(Bus* bus)
{
  bus->ConnectIOPortReadToPointer(IOPORT_INDEX_REGISTER, this, &m_index_register);
  bus->ConnectIOPortWriteToPointer(IOPORT_INDEX_REGISTER, this, &m_index_register);
  bus->ConnectIOPortRead(IOPORT_DATA_PORT, this, std::bind(&DS12887::IOReadDataPort, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(IOPORT_DATA_PORT, this, std::bind(&DS12887::IOWriteDataPort, this, std::placeholders::_2));
}

void DS12887::IOReadDataPort(u8* value)
{
  const u8 index = m_index_register & m_index_register_mask;
  if (index < RTC_REGISTER_STATUS_REGISTER_A || index == RTC_REGISTER_CENTURY)
  {
    // Handle RTC and special stuff.
    UpdateClock();
  }

  *value = m_data[index];

#ifdef Y_BUILD_CONFIG_DEBUG
  if (index >= RTC_REGISTER_STATUS_REGISTER_A && index != RTC_REGISTER_CENTURY)
    Log_DebugPrintf("Read register 0x%02X, value=0x%02X", ZeroExtend32(index), ZeroExtend32(*value));
#endif

  if (index == RTC_REGISTER_STATUS_REGISTER_C)
  {
    // Reading status register C resets the interrupts.
    m_data[RTC_REGISTER_STATUS_REGISTER_C] &=
      ~(RTC_SRC_PERIODIC_INTERRUPT | RTC_SRC_ALARM_INTERRUPT | RTC_SRC_UPDATE_ENDED_INTERRUPT);
    UpdateInterruptState();
    m_rtc_interrupt_event->SetActive(true);
  }
}

void DS12887::IOWriteDataPort(u8 value)
{
  const u8 index = m_index_register & m_index_register_mask;

#ifdef Y_BUILD_CONFIG_DEBUG
  if (index >= RTC_REGISTER_STATUS_REGISTER_D && index != RTC_REGISTER_CENTURY)
    Log_DebugPrintf("Write register 0x%02X value=0x%02X", ZeroExtend32(index), ZeroExtend32(value));
#endif

  m_data[index] = value;

  // Handle RTC and special stuff.
  if (index < RTC_REGISTER_STATUS_REGISTER_A || index == RTC_REGISTER_CENTURY)
    UpdateClock();
  else if (index >= RTC_REGISTER_STATUS_REGISTER_A && index <= RTC_REGISTER_STATUS_REGISTER_D)
    UpdateRTCFrequency();

  QueueSaveRAM();
}

void DS12887::UpdateRTCFrequency()
{
  static const u32 base_rate_table[] = {4194304, 1048576, 32768, 16384};

  u8 base_rate_index = ((m_data[RTC_REGISTER_STATUS_REGISTER_A] >> RTC_SRA_DV_SHIFT) & RTC_SRA_DV_MASK);
  u8 rate_divider = ((m_data[RTC_REGISTER_STATUS_REGISTER_A] >> RTC_SRA_RS_SHIFT) & RTC_SRA_RS_MASK);

  u32 base_rate = base_rate_table[base_rate_index];
  u32 interrupt_rate = base_rate >> rate_divider;

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

void DS12887::RTCInterruptEvent()
{
  if (m_data[RTC_REGISTER_STATUS_REGISTER_C] & RTC_SRC_PERIODIC_INTERRUPT)
  {
    // We don't trigger the interrupt unless the flag has been read and cleared.
    m_rtc_interrupt_event->Deactivate();
    return;
  }

  m_data[RTC_REGISTER_STATUS_REGISTER_C] |= RTC_SRC_PERIODIC_INTERRUPT;
  UpdateInterruptState();

  // Keep the event inactive until it's read again.
  // This may not be accurate...
  m_rtc_interrupt_event->Deactivate();
}

void DS12887::SaveRAMEvent()
{
  SaveRAM();
  m_save_ram_event->Deactivate();
}

void DS12887::UpdateInterruptState()
{
  // These use the same bit position.
  const u8 interrupt_bits =
    (m_data[RTC_REGISTER_STATUS_REGISTER_B] &
     (RTC_SRB_PERIODIC_INTERRUPT_ENABLE | RTC_SRB_ALARM_INTERRUPT_ENABLE | RTC_SRB_UPDATE_ENDED_INTERRUPT_ENABLE) &
     (m_data[RTC_REGISTER_STATUS_REGISTER_C] &
      (RTC_SRC_PERIODIC_INTERRUPT | RTC_SRC_ALARM_INTERRUPT | RTC_SRC_UPDATE_ENDED_INTERRUPT)));
  if (interrupt_bits != 0)
  {
    m_data[RTC_REGISTER_STATUS_REGISTER_C] |= RTC_SRC_INTERRUPT_REQUEST;
    m_interrupt_controller->RaiseInterrupt(m_irq);
  }
  else
  {
    m_data[RTC_REGISTER_STATUS_REGISTER_C] &= ~RTC_SRC_INTERRUPT_REQUEST;
    m_interrupt_controller->LowerInterrupt(m_irq);
  }
}

void DS12887::UpdateClock()
{
  const SimulationTime elapsed_time =
    m_system->GetTimingManager()->GetEmulatedTimeDifference(m_last_clock_update_time) + m_clock_partial_time;
  m_last_clock_update_time = m_system->GetTimingManager()->GetTotalEmulatedTime();

  // Time greater than the lowest unit of time (seconds)?
  const u32 elapsed_seconds = Truncate32(SimulationTimeToSeconds(elapsed_time));
  if (elapsed_seconds > 0)
  {
    Log_ErrorPrintf("Adding %u seconds", elapsed_seconds);
    // Leave the fraction for the next update.
    m_clock_partial_time = elapsed_time - SecondsToSimulationTime(elapsed_seconds);
    if ((m_data[RTC_REGISTER_STATUS_REGISTER_B] & RTC_SRB_SET) == 0)
      AddClockSeconds(elapsed_seconds);
  }
  else
  {
    m_clock_partial_time = elapsed_time;
  }
}

void DS12887::ResetRAM()
{
  std::fill(m_data.begin(), m_data.end(), u8(0));

  // Set the RTC base frequency to 32768hz
  m_data[RTC_REGISTER_STATUS_REGISTER_A] = 0b0000 | 0b0100000;

  // Default to 24 hour mode.
  m_data[RTC_REGISTER_STATUS_REGISTER_B] = RTC_SRB_24_HOUR_MODE;

  // RTC has power
  m_data[RTC_REGISTER_STATUS_REGISTER_D] = RTC_SRD_RAM_VALID;

  m_last_clock_update_time = m_system->GetTimingManager()->GetTotalEmulatedTime();
  m_clock_partial_time = 0;
  SynchronizeTimeWithHost();
}

bool DS12887::LoadRAM()
{
  // Get last modification time.
  FILESYSTEM_STAT_DATA sd;
  if (!FileSystem::StatFile(m_save_filename, &sd))
    return false;

  // Load the RAM.
  AutoReleasePtr<ByteStream> stream =
    FileSystem::OpenFile(m_save_filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
  if (!stream)
    return false;

  // Check the size.
  if (stream->GetSize() != static_cast<u64>(m_size))
  {
    Log_WarningPrintf("Incorrect size for RAM: found %u expected %u", static_cast<u32>(stream->GetSize()), m_size);
    return false;
  }

  if (!stream->Read2(m_data.data(), m_size))
    return false;

  Log_DevPrintf("Loaded %u bytes of RAM from %s", m_size, m_save_filename.GetCharArray());

  // Determine how much time has passed since the RAM was saved.
  // That is the offset which we need to add to the time.
  const double time_since_saved = Timestamp::Now().DifferenceInSeconds(sd.ModificationTime);
  if (time_since_saved > 0.0)
  {
    Log_InfoPrintf("Adding %f seconds of time to RTC", time_since_saved);

    const u32 seconds = static_cast<u32>(time_since_saved);
    if (seconds > 0)
      AddClockSeconds(seconds);

    m_last_clock_update_time = m_system->GetTimingManager()->GetTotalEmulatedTime();
    m_clock_partial_time =
      static_cast<SimulationTime>(time_since_saved - static_cast<double>(seconds)) * INT64_C(1000000000);
  }

  return true;
}

void DS12887::SaveRAM()
{
  AutoReleasePtr<ByteStream> stream =
    FileSystem::OpenFile(m_save_filename, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE |
                                            BYTESTREAM_OPEN_STREAMED);
  if (!stream)
  {
    Log_WarningPrintf("Failed to open '%s'", m_save_filename.GetCharArray());
    return;
  }

  stream->Write(m_data.data(), m_size);
  Log_InfoPrintf("Saved RAM to '%s'", m_save_filename.GetCharArray());
}

void DS12887::QueueSaveRAM()
{
  if (m_save_ram_event->IsActive())
    return;

  m_save_ram_event->SetActive(true);
}

u8 DS12887::ReadClockRegister(u8 index) const
{
  return (m_data[RTC_REGISTER_STATUS_REGISTER_B] & RTC_SRB_BINARY_MODE) ? m_data[index] : BCDToDecimal(m_data[index]);
}

void DS12887::WriteClockRegister(u8 index, u8 value)
{
  m_data[index] = (m_data[RTC_REGISTER_STATUS_REGISTER_B] & RTC_SRB_BINARY_MODE) ? value : DecimalToBCD(value);
}

void DS12887::AddClockSeconds(const u32 elapsed_seconds)
{
  u32 seconds = ZeroExtend32(ReadClockRegister(RTC_REGISTER_SECONDS));
  seconds += elapsed_seconds;
  if (seconds >= 60)
  {
    AddClockMinutes(seconds / 60);
    seconds %= 60;
  }
  WriteClockRegister(RTC_REGISTER_SECONDS, Truncate8(seconds));
}

void DS12887::AddClockMinutes(const u32 elapsed_minutes)
{
  u32 minutes = ZeroExtend32(ReadClockRegister(RTC_REGISTER_MINUTES));
  minutes += elapsed_minutes;
  if (minutes >= 60)
  {
    AddClockHours(minutes / 60);
    minutes %= 60;
  }
  WriteClockRegister(RTC_REGISTER_MINUTES, Truncate8(minutes));
}

void DS12887::AddClockHours(const u32 elapsed_hours)
{
  const bool binary_mode = (m_data[RTC_REGISTER_STATUS_REGISTER_B] & RTC_SRB_BINARY_MODE) != 0;
  const bool is_24_hour = (m_data[RTC_REGISTER_STATUS_REGISTER_B] & RTC_SRB_24_HOUR_MODE) != 0;
  u32 hours = ZeroExtend32(m_data[RTC_REGISTER_HOURS]);

  // Convert to 24 hour time for update.
  if (!is_24_hour)
  {
    hours =
      ((binary_mode) ? ((hours - 1) & 0x1F) : (BCDToDecimal(Truncate8(hours - 1)) & 0x1F)) + ((hours & 0x80) ? 12 : 0);
  }
  else
  {
    hours = (binary_mode) ? hours : BCDToDecimal(Truncate8(hours));
  }

  hours += elapsed_hours;
  if (hours >= 24)
  {
    AddClockDays(hours / 24);
    hours %= 24;
  }

  // Convert back to 12 hour time if needed.
  if (!is_24_hour)
    hours = ((hours / 12) * 0x80) | ((binary_mode) ? ((hours % 12) + 1) : DecimalToBCD(Truncate8(hours % 12) + 1));

  m_data[RTC_REGISTER_HOURS] = Truncate8(hours);
}

/// Leap year test.
constexpr bool RTCIsLeapYear(u32 year)
{
  if ((year % 400) == 0)
    return true;
  if ((year % 100) == 0)
    return false;
  return ((year % 4) == 0);
}

/// Assumes one-based month.
static u32 RTCDaysInMonth(u32 month, u32 year)
{
  constexpr u32 days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2)
  {
    // Test for leap year.
    return RTCIsLeapYear(year) ? 29 : 28;
  }

  return (month > 0 && month <= 12) ? days[month - 1] : std::numeric_limits<u32>::max();
}

void DS12887::AddClockDays(const u32 elapsed_days)
{
  u32 day_of_week = ReadClockRegister(RTC_REGISTER_DAY_OF_WEEK);
  day_of_week += elapsed_days;
  if (day_of_week > 7)
    day_of_week = ((day_of_week - 1) % 7) + 1;
  WriteClockRegister(RTC_REGISTER_DAY_OF_WEEK, Truncate8(day_of_week));

  // We need to read the year to update the day.
  // TODO: Daylight savings..
  u32 month = ReadClockRegister(RTC_REGISTER_MONTH);
  u32 year = ReadClockRegister(RTC_REGISTER_YEAR) + (ZeroExtend32(ReadClockRegister(RTC_REGISTER_CENTURY)) * 100);
  u32 date_of_month = ReadClockRegister(RTC_REGISTER_DATE_OF_MONTH);
  date_of_month += elapsed_days;
  for (;;)
  {
    const u32 days_in_month = RTCDaysInMonth(month, year);
    if (date_of_month < days_in_month)
      break;

    date_of_month -= days_in_month;
    month++;

    if (month > 12)
    {
      month -= 12;
      year++;
    }
  }

  WriteClockRegister(RTC_REGISTER_DATE_OF_MONTH, Truncate8(date_of_month));
  WriteClockRegister(RTC_REGISTER_MONTH, Truncate8(month));
  WriteClockRegister(RTC_REGISTER_YEAR, Truncate8(year % 100));
  WriteClockRegister(RTC_REGISTER_CENTURY, Truncate8(year / 100));
}

} // namespace HW
