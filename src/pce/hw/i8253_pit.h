#pragma once
#include "common/bitfield.h"
#include "common/clock.h"
#include "pce/component.h"
#include "pce/types.h"
#include <array>

namespace HW {

// i8253 Programmable Interval Timer
class i8253_PIT : public Component
{
  DECLARE_OBJECT_TYPE_INFO(i8253_PIT, Component);
  DECLARE_OBJECT_NO_FACTORY(i8253_PIT);
  DECLARE_OBJECT_PROPERTY_MAP(i8253_PIT);

public:
  static constexpr float CLOCK_FREQUENCY = 1193181.6666f;
  static constexpr size_t NUM_CHANNELS = 3;

  i8253_PIT(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~i8253_PIT();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  // Gets the gate input state of a channel.
  bool GetChannelGateInput(size_t channel_index);

  // Get the current output state of a channel.
  bool GetChannelOutputState(size_t channel_index);

  // Change the input gate state of a channel.
  void SetChannelGateInput(size_t channel_index, bool value);

  // Sets up a callback for a channel. If not set, the channel will only be readable via IO.
  using ChannelOutputChangeCallback = std::function<void(bool)>;
  void SetChannelOutputChangeCallback(size_t channel_index, ChannelOutputChangeCallback callback);

private:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('8', '2', '5', '3');

  static constexpr u32 IOPORT_CHANNEL_0_DATA = 0x40;
  static constexpr u32 IOPORT_CHANNEL_1_DATA = 0x41;
  static constexpr u32 IOPORT_CHANNEL_2_DATA = 0x42;
  static constexpr u32 IOPORT_COMMAND_REGISTER = 0x43;

  enum ChannelAccessMode : u8
  {
    ChannelAccessModeMSB,
    ChannelAccessModeLSBOnly,
    ChannelAccessModeMSBOnly,
    ChannelAccessModeLSB,
  };

  enum ChannelOperatingMode : u8
  {
    ChannelOperatingModeInterrupt,
    ChannelOperatingModeOneShot,
    ChannelOperatingModeRateGenerator,
    ChannelOperatingModeSquareWaveGenerator,
    ChannelOperatingModeSoftwareTriggeredStrobe,
    ChannelOperatingModeHardwareTriggeredStrobe
  };

  struct Channel
  {
    CycleCount count = 0;
    CycleCount downcount = 0;

    u16 reload_value = 0;

    ChannelOperatingMode operating_mode = ChannelOperatingModeInterrupt;
    ChannelAccessMode read_mode = ChannelAccessModeMSB;
    ChannelAccessMode write_mode = ChannelAccessModeMSB;
    bool bcd_mode = false;

    u16 read_latch_value = 0;
    u16 write_latch_value = 0;
    bool read_latch_needs_update = false;

    bool gate_input = true;
    bool output_state = false;
    bool waiting_for_reload = true;
    bool waiting_for_gate = false;
    bool reload_value_set = false;

    bool square_wave_flip_flop = false;

    ChannelOutputChangeCallback change_callback;

    bool HasCallback() const { return static_cast<bool>(change_callback); }
  };

  void ConnectIOPorts(Bus* bus);

  void ReadDataPort(u32 channel_index, u8* value);
  void WriteDataPort(u32 channel_index, u8 value);
  void WriteCommandRegister(u8 value);

  CycleCount GetDowncount() const;
  CycleCount GetFrequencyFromReloadValue(Channel* channel) const;

  void TickTimers(CycleCount cycles);
  void RescheduleTimerEvent();

  // Internal state changing methods. Probably best not to call these outside.
  void SetChannelMode(size_t channel_index, ChannelOperatingMode mode);
  void SetChannelReloadRegister(size_t channel_index, u16 reload_value);
  void SimulateChannel(size_t channel_index, CycleCount cycles);
  CycleCount UpdateChannelDowncount(size_t channel_index);
  void SetChannelOutputState(size_t channel_index, bool value);
  void UpdateAllChannelsDowncount();

  Clock m_clock;

  std::array<Channel, NUM_CHANNELS> m_channels;

  TimingEvent::Pointer m_tick_event;
};

} // namespace HW
