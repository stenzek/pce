#pragma once

#include "YBaseLib/Assert.h"
#include "common/clock.h"
#include "pce/component.h"
#include <array>
#include <memory>
#include <vector>

class ByteStream;
class InterruptController;

namespace HW {

class DS12887 : public Component
{
  DECLARE_OBJECT_TYPE_INFO(DS12887, Component);
  DECLARE_OBJECT_PROPERTY_MAP(DS12887);
  DECLARE_OBJECT_NO_FACTORY(DS12887);

public:
  DS12887(const String& identifier, u32 size = 128, u32 irq = 8, const ObjectTypeInfo* type_info = &s_type_info);
  ~DS12887();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  /// Synchronizes the RTC with the real time of the host.
  void SynchronizeTimeWithHost();

  uint8 GetConfigVariable(uint8 index) const { return m_data[index]; }
  void SetConfigVariable(uint8 index, uint8 value) { m_data[index] = value; }

  uint16 GetConfigWordVariable(uint8 base_index) const;
  void SetConfigWordVariable(uint8 base_index, uint16 value);

  void SetConfigFloppyType(uint32 index, uint32 type);
  void SetConfigFloppyCount(uint32 count);

protected:
  static const uint32 SERIALIZATION_ID = MakeSerializationID('D', 'S', '1', '7');
  static const uint32 IOPORT_INDEX_REGISTER = 0x70;
  static const uint32 IOPORT_DATA_PORT = 0x71;

  enum RTC_REGISTERS
  {
    RTC_REGISTER_SECONDS = 0x00,
    RTC_REGISTER_SECOND_ALARM = 0x01,
    RTC_REGISTER_MINUTES = 0x02,
    RTC_REGISTER_MINUTE_ALARM = 0x03,
    RTC_REGISTER_HOURS = 0x04,
    RTC_REGISTER_HOUR_ALARM = 0x05,
    RTC_REGISTER_DAY_OF_WEEK = 0x06,
    RTC_REGISTER_DATE_OF_MONTH = 0x07,
    RTC_REGISTER_MONTH = 0x08,
    RTC_REGISTER_YEAR = 0x09,
    RTC_REGISTER_STATUS_REGISTER_A = 0x0A,
    RTC_REGISTER_STATUS_REGISTER_B = 0x0B,
    RTC_REGISTER_STATUS_REGISTER_C = 0x0C,
    RTC_REGISTER_STATUS_REGISTER_D = 0x0D,
    RTC_REGISTER_CENTURY = 0x32
  };
  enum RTC_SRA : u8
  {
    RTC_SRA_UPDATE_IN_PROGRESS = (1 << 7),
    RTC_SRA_DV_SHIFT = 4,
    RTC_SRA_DV_MASK = 0x07,
    RTC_SRA_RS_SHIFT = 0,
    RTC_SRA_RS_MASK = 0x0F,
  };
  enum RTC_SRB : u8
  {
    RTC_SRB_SET = (1 << 7),
    RTC_SRB_PERIODIC_INTERRUPT_ENABLE = (1 << 6),
    RTC_SRB_ALARM_INTERRUPT_ENABLE = (1 << 5),
    RTC_SRB_UPDATE_ENDED_INTERRUPT_ENABLE = (1 << 4),
    RTC_SRB_SQUARE_WAVE_ENABLE = (1 << 3),
    RTC_SRB_BINARY_MODE = (1 << 2),
    RTC_SRB_24_HOUR_MODE = (1 << 1),
    RTC_SRB_DAYLIGHT_SAVINGS_MODE = (1 << 0)
  };
  enum RTC_SRC : u8
  {
    RTC_SRC_INTERRUPT_REQUEST = (1 << 7),
    RTC_SRC_PERIODIC_INTERRUPT = (1 << 6),
    RTC_SRC_ALARM_INTERRUPT = (1 << 5),
    RTC_SRC_UPDATE_ENDED_INTERRUPT = (1 << 4),
  };
  enum RTC_SRD : u8
  {
    RTC_SRD_RAM_VALID = (1 << 7)
  };

  void ConnectIOPorts(Bus* bus);
  void IOReadDataPort(uint8* value);
  void IOWriteDataPort(uint8 value);

  void UpdateRTCFrequency();
  void RTCInterruptEvent(CycleCount cycles);

  void UpdateInterruptState();
  void UpdateClock();

  // Reads/writes the clock register in binary/BCD mode.
  u8 ReadClockRegister(u8 index) const;
  void WriteClockRegister(u8 index, u8 value);

  // Clock increment helpers. seconds can be greater than 60, which will be added to minutes, etc.
  void AddClockSeconds(const u32 elapsed_seconds);
  void AddClockMinutes(const u32 elapsed_minutes);
  void AddClockHours(const u32 elapsed_hours);
  void AddClockDays(const u32 elapsed_days);

  InterruptController* m_interrupt_controller = nullptr;
  u32 m_size = 256;
  u32 m_irq = 8;

  std::vector<u8> m_data;
  u8 m_index_register = 0;
  u8 m_index_register_mask = 0xFF;

  TimingEvent::Pointer m_rtc_interrupt_event{};

  SimulationTime m_last_clock_update_time = 0;

  /// Contains the fraction of a second time "left over" from the previous update.
  SimulationTime m_clock_partial_time = 0;
};

} // namespace HW