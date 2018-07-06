#include "pce/hw/i8253_pit.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <algorithm>
Log_SetChannel(i8253_PIT);

// NOTE: Most of the documentation here is based on http://wiki.osdev.org/Programmable_Interval_Timer

namespace HW {

i8253_PIT::i8253_PIT() : m_clock("i8253 PIT", CLOCK_FREQUENCY) {}

i8253_PIT::~i8253_PIT() {}

bool i8253_PIT::Initialize(System* system, Bus* bus)
{
  m_system = system;
  m_clock.SetManager(system->GetTimingManager());
  ConnectIOPorts(bus);

  // Create the tick event.
  m_tick_event =
    m_clock.NewEvent("Tick Event", GetDowncount(), std::bind(&i8253_PIT::TickTimers, this, std::placeholders::_2));

  return true;
}

void i8253_PIT::TickTimers(CycleCount cycles)
{
  if (cycles > 0)
  {
    for (size_t i = 0; i < NUM_CHANNELS; i++)
    {
      SimulateChannel(i, cycles);
      UpdateChannelDowncount(i);
    }
  }

  RescheduleTimerEvent();
}

void i8253_PIT::RescheduleTimerEvent()
{
  // Re-schedule the event with the new downcount.
  m_tick_event->Reschedule(GetDowncount());
  // m_tick_event->Reschedule(1);
}

void i8253_PIT::Reset()
{
  for (uint32 i = 0; i < NUM_CHANNELS; i++)
  {
    Channel* channel = &m_channels[i];
    channel->count = 0;
    channel->monitor_count = 0;
    channel->downcount = 0;

    channel->reload_value = 0;
    channel->operating_mode = ChannelOperatingModeInterrupt;
    channel->read_mode = ChannelAccessModeMSB;
    channel->write_mode = ChannelAccessModeMSB;
    channel->bcd_mode = false;

    channel->read_latch_value = 0;
    channel->write_latch_value = 0;
    channel->read_latch_needs_update = false;

    channel->gate_input = true;
    channel->output_state = false;

    channel->waiting_for_reload = true;
    channel->waiting_for_gate = false;
    channel->reload_value_set = false;

    channel->square_wave_flip_flop = false;
  }
}

bool i8253_PIT::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  for (uint32 i = 0; i < NUM_CHANNELS; i++)
  {
    Channel* channel = &m_channels[i];
    reader.SafeReadInt64(&channel->count);
    reader.SafeReadInt64(&channel->monitor_count);
    reader.SafeReadInt64(&channel->downcount);
    reader.SafeReadUInt16(&channel->reload_value);

    uint8 operating_mode = 0, read_mode = 0, write_mode = 0;
    reader.SafeReadUInt8(&operating_mode);
    reader.SafeReadUInt8(&read_mode);
    reader.SafeReadUInt8(&write_mode);
    channel->operating_mode = static_cast<ChannelOperatingMode>(operating_mode);
    channel->read_mode = static_cast<ChannelAccessMode>(read_mode);
    channel->write_mode = static_cast<ChannelAccessMode>(write_mode);
    reader.SafeReadBool(&channel->bcd_mode);

    reader.SafeReadUInt16(&channel->read_latch_value);
    reader.SafeReadUInt16(&channel->write_latch_value);
    reader.SafeReadBool(&channel->read_latch_needs_update);

    reader.SafeReadBool(&channel->gate_input);
    reader.SafeReadBool(&channel->output_state);

    reader.SafeReadBool(&channel->waiting_for_reload);
    reader.SafeReadBool(&channel->waiting_for_gate);
    reader.SafeReadBool(&channel->reload_value_set);

    reader.SafeReadBool(&channel->square_wave_flip_flop);
  }

  RescheduleTimerEvent();
  return !reader.GetErrorState();
}

bool i8253_PIT::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  for (uint32 i = 0; i < NUM_CHANNELS; i++)
  {
    Channel* channel = &m_channels[i];
    writer.WriteInt64(channel->count);
    writer.WriteInt64(channel->monitor_count);
    writer.WriteInt64(channel->downcount);
    writer.WriteUInt16(channel->reload_value);
    writer.WriteUInt8(static_cast<uint8>(channel->operating_mode));
    writer.WriteUInt8(static_cast<uint8>(channel->read_mode));
    writer.WriteUInt8(static_cast<uint8>(channel->write_mode));
    writer.WriteBool(channel->bcd_mode);

    writer.WriteUInt16(channel->read_latch_value);
    writer.WriteUInt16(channel->write_latch_value);
    writer.WriteBool(channel->read_latch_needs_update);

    writer.WriteBool(channel->gate_input);
    writer.WriteBool(channel->output_state);

    writer.WriteBool(channel->waiting_for_reload);
    writer.WriteBool(channel->waiting_for_gate);
    writer.WriteBool(channel->reload_value_set);

    writer.WriteBool(channel->square_wave_flip_flop);
  }

  return !writer.InErrorState();
}

bool i8253_PIT::GetChannelGateInput(size_t channel_index)
{
  DebugAssert(channel_index < m_channels.size());
  return m_channels[channel_index].gate_input;
}

bool i8253_PIT::GetChannelOutputState(size_t channel_index)
{
  // Channel should be up to date before reading the value.
  m_tick_event->InvokeEarly();
  UpdateAllChannelsDowncount();

  DebugAssert(channel_index < m_channels.size());
  return m_channels[channel_index].output_state;
}

void i8253_PIT::SetChannelOutputChangeCallback(size_t channel_index, ChannelOutputChangeCallback callback)
{
  m_channels[channel_index].change_callback = std::move(callback);
}

void i8253_PIT::SetChannelMonitorCallback(size_t channel_index, ChannelMonitorCallback callback)
{
  m_channels[channel_index].monitor_callback = std::move(callback);
}

void i8253_PIT::ConnectIOPorts(Bus* bus)
{
  bus->ConnectIOPortRead(IOPORT_CHANNEL_0_DATA, this,
                         std::bind(&i8253_PIT::ReadDataPort, this, 0, std::placeholders::_2));
  bus->ConnectIOPortWrite(IOPORT_CHANNEL_0_DATA, this,
                          std::bind(&i8253_PIT::WriteDataPort, this, 0, std::placeholders::_2));
  bus->ConnectIOPortRead(IOPORT_CHANNEL_1_DATA, this,
                         std::bind(&i8253_PIT::ReadDataPort, this, 1, std::placeholders::_2));
  bus->ConnectIOPortWrite(IOPORT_CHANNEL_1_DATA, this,
                          std::bind(&i8253_PIT::WriteDataPort, this, 1, std::placeholders::_2));
  bus->ConnectIOPortRead(IOPORT_CHANNEL_2_DATA, this,
                         std::bind(&i8253_PIT::ReadDataPort, this, 2, std::placeholders::_2));
  bus->ConnectIOPortWrite(IOPORT_CHANNEL_2_DATA, this,
                          std::bind(&i8253_PIT::WriteDataPort, this, 2, std::placeholders::_2));
  bus->ConnectIOPortWrite(IOPORT_COMMAND_REGISTER, this,
                          std::bind(&i8253_PIT::WriteCommandRegister, this, std::placeholders::_2));
}

CycleCount i8253_PIT::GetDowncount() const
{
  CycleCount downcount = 0;
  for (const auto& channel : m_channels)
  {
    if (channel.downcount != 0)
      downcount = (downcount != 0) ? std::min(downcount, channel.downcount) : channel.downcount;
  }

  // Just use some stupidly high number of cycles, since we don't need to force sync.
  if (downcount == 0)
  {
    // Every 100ms should do..
    downcount = CycleCount(CLOCK_FREQUENCY / 10);
  }

  return downcount;
}

CycleCount i8253_PIT::GetFrequencyFromReloadValue(Channel* channel) const
{
  if (channel->reload_value == 0)
    return 65536;
  else
    return channel->reload_value;
}

void i8253_PIT::ReadDataPort(uint32 channel_index, uint8* value)
{
  Channel* channel = &m_channels[channel_index];
  DebugAssert(channel_index < NUM_CHANNELS);

  if (channel->read_latch_needs_update)
  {
    m_tick_event->InvokeEarly();
    channel->read_latch_value = Truncate16(channel->count);
    channel->read_latch_needs_update = false;
  }

  switch (channel->read_mode)
  {
    case ChannelAccessModeLSBOnly:
      *value = Truncate8(channel->read_latch_value);
      break;

    case ChannelAccessModeMSBOnly:
      *value = Truncate8(channel->read_latch_value >> 8);
      break;

    case ChannelAccessModeMSB:
      *value = Truncate8(channel->read_latch_value >> 8);
      channel->read_latch_needs_update = true;
      channel->read_mode = ChannelAccessModeLSB;
      break;

    case ChannelAccessModeLSB:
    default:
      *value = Truncate8(channel->read_latch_value);
      channel->read_mode = ChannelAccessModeMSB;
      break;
  }
}

void i8253_PIT::WriteDataPort(uint32 channel_index, uint8 value)
{
  Channel* channel = &m_channels[channel_index];
  DebugAssert(channel_index < NUM_CHANNELS);

  // Ensure we're up-to-date.
  m_tick_event->InvokeEarly();

  // In "lobyte/hibyte" access mode counting will stop when the first byte of the reload value is set.
  if (channel->write_mode == ChannelAccessModeMSB)
    channel->waiting_for_reload = true;

  switch (channel->write_mode)
  {
    case ChannelAccessModeLSBOnly:
      channel->reload_value = (channel->reload_value & 0xFF00) | ZeroExtend16(value);
      SetChannelReloadRegister(channel_index, channel->reload_value);
      break;

    case ChannelAccessModeMSBOnly:
      channel->reload_value = (channel->reload_value & 0x00FF) | (ZeroExtend16(value) << 8);
      SetChannelReloadRegister(channel_index, channel->reload_value);
      break;

    case ChannelAccessModeMSB:
      channel->reload_value = (channel->reload_value & 0x00FF) | (ZeroExtend16(value) << 8);
      channel->write_mode = ChannelAccessModeLSB;
      SetChannelReloadRegister(channel_index, channel->reload_value);
      break;

    case ChannelAccessModeLSB:
    default:
      channel->reload_value = (channel->reload_value & 0xFF00) | ZeroExtend16(value);
      channel->write_mode = ChannelAccessModeMSB;
      break;
  }

  UpdateAllChannelsDowncount();
}

void i8253_PIT::WriteCommandRegister(uint8 value)
{
  uint8 channel_index = (value >> 6) & 0b11;
  ChannelAccessMode access_mode = ChannelAccessMode((value >> 4) & 0b11);
  ChannelOperatingMode operating_mode = ChannelOperatingMode((value >> 1) & 0b111);
  bool bcd_mode = !!(value & 0b1);

  // Ensure we're up-to-date.
  m_tick_event->InvokeEarly();

  if (channel_index == 0b11)
  {
    // TODO: Read-back mode
    Log_ErrorPrintf("PIT readback mode not fully implemented");
    for (uint32 i = 0; i < NUM_CHANNELS; i++)
    {
      Channel* channel = &m_channels[i];
      channel->read_latch_value = 0;
      channel->read_latch_value |= (uint8(channel->bcd_mode ? 1 : 0) << 0);
      channel->read_latch_value |= (uint8(channel->operating_mode) << 1);
      channel->read_latch_value |= (uint8(channel->write_mode) << 4);
      channel->read_latch_value |= (uint8(channel->waiting_for_reload ? 1 : 0) << 6);
      channel->read_latch_value |= (uint8(0) << 7); // Current output pin state
    }

    return;
  }

  Channel* channel = &m_channels[channel_index];

  // Access mode == 0 is latch immediately
  if ((value & 0x30) == 0)
  {
    channel->read_latch_value = Truncate16(channel->count);
    channel->read_latch_needs_update = false;
    return;
  }

  // Access mode is updated on write
  channel->read_mode = access_mode;
  channel->write_mode = access_mode;

  // Value isn't latched until read time
  channel->read_latch_needs_update = true;

  // Update mode
  channel->bcd_mode = bcd_mode;
  SetChannelMode(channel_index, operating_mode);
}

void i8253_PIT::SetChannelMode(size_t channel_index, ChannelOperatingMode mode)
{
  Channel* channel = &m_channels[channel_index];
  Log_DevPrintf("Set PIC channel %u to mode %u%s", Truncate32(channel_index), ZeroExtend32(mode),
                channel->bcd_mode ? " (BCD)" : "");

  // ChannelOperatingMode old_mode = channel->operating_mode;
  channel->operating_mode = mode;

  switch (mode)
  {
    case ChannelOperatingModeInterrupt:
    {
      // For this mode, when the mode/command register is written the output signal goes low and the PIT waits for the
      // reload register to be set by software, to begin the countdown.
      SetChannelOutputState(channel_index, false);
      channel->waiting_for_reload = true;
      channel->waiting_for_gate = false;
      channel->reload_value_set = false;
    }
    break;

    case ChannelOperatingModeOneShot:
    {
      // When the mode/command register is written the output signal goes high and the PIT waits for the reload register
      // to be set by software.
      SetChannelOutputState(channel_index, true);
      channel->waiting_for_reload = true;
      channel->reload_value_set = false;

      // After the reload register has been set the PIT will wait for the next rising edge of the gate input.
      channel->waiting_for_gate = true;
    }
    break;

    case ChannelOperatingModeRateGenerator:
    case ChannelOperatingModeSquareWaveGenerator:
    {
      // This mode operates as a frequency divider.
      // When the mode/command register is written the output signal goes high and the PIT waits for the reload register
      // to be set by software.
      SetChannelOutputState(channel_index, true);
      channel->waiting_for_reload = true;
      channel->waiting_for_gate = false;
      channel->reload_value_set = false;
    }
    break;

    case ChannelOperatingModeSoftwareTriggeredStrobe:
    {
      // Mode four operates as a retriggerable delay, and generates a pulse when the current count reaches zero.
      // When the mode/command register is written the output signal goes high and the PIT waits for the reload register
      // to be set by software.
      SetChannelOutputState(channel_index, true);
      channel->waiting_for_reload = true;
      channel->waiting_for_gate = false;
      channel->reload_value_set = false;
    }
    break;

    case ChannelOperatingModeHardwareTriggeredStrobe:
    {
      // Mode 5 is similar to mode 4, except that it waits for the rising edge of the gate input to trigger (or
      // re-trigger) the delay period (like mode 1). When the mode/command register is written the output signal goes
      // high and the PIT waits for the reload register to be set by software. After the reload register has been set
      // the PIT will wait for the next rising edge of the gate input.
      SetChannelOutputState(channel_index, true);
      channel->waiting_for_reload = true;
      channel->waiting_for_gate = true;
      channel->reload_value_set = false;
    }
    break;

    default:
      Log_ErrorPrintf("Unknown PIC operating mode %u", ZeroExtend32(mode));
      Panic("Unhandled PIC operating mode");
      break;
  }

  UpdateAllChannelsDowncount();
}

void i8253_PIT::SetChannelReloadRegister(size_t channel_index, uint16 reload_value)
{
  Channel* channel = &m_channels[channel_index];
  Log_DevPrintf("Set PIC channel %u reload register: %u", Truncate32(channel_index), ZeroExtend32(reload_value));

  // Ensure we're up-to-date.
  m_tick_event->InvokeEarly();

  switch (channel->operating_mode)
  {
    case ChannelOperatingModeInterrupt:
    {
      // TODO: Check writing the reload value resets the output state to low?
      SetChannelOutputState(channel_index, false);
      channel->reload_value = reload_value;
      channel->reload_value_set = true;

      // The reload value can be changed at any time. In "lobyte/hibyte" access mode counting
      // will stop when the first byte of the reload value is set.
      channel->waiting_for_reload = false;
    }
    break;

    case ChannelOperatingModeOneShot:
    {
      channel->reload_value = reload_value;
      channel->waiting_for_reload = false;
    }
    break;

    case ChannelOperatingModeRateGenerator:
    case ChannelOperatingModeSquareWaveGenerator:
    {
      // After the reload register has been set, the current count will be set to the reload value on the next falling
      // edge of the (1.193182 MHz) input signal. Subsequent falling edges of the input signal will decrement the
      // current count (if the gate input is high on the preceding rising edge of the input signal).
      channel->reload_value = reload_value;

      // The reload value can be changed at any time, however the new value will not effect the current count until the
      // current count is reloaded (when it is decreased from two to one, or the gate input going low then high).
      channel->reload_value_set = channel->waiting_for_reload;
      channel->waiting_for_reload = false;
    }
    break;

    case ChannelOperatingModeSoftwareTriggeredStrobe:
    {
      // The reload value can be changed at any time. When the new value has been set (both bytes for "lobyte/hibyte"
      // access mode) it will be loaded into the current count on the next falling edge of the (1.193182 MHz) input
      // signal, and counting will continue using the new reload value.
      channel->reload_value = reload_value;
      channel->waiting_for_reload = false;
    }
    break;

    case ChannelOperatingModeHardwareTriggeredStrobe:
    {
      // The reload value can be changed at any time, however the new value will not affect the current count until the
      // current count is reloaded (on the next rising edge of the gate input). When this occurs counting will continue
      // using the new reload value.
      channel->reload_value = reload_value;
      channel->waiting_for_reload = false;
    }
    break;

    default:
      break;
  }

  UpdateAllChannelsDowncount();
}

void i8253_PIT::SetChannelGateInput(size_t channel_index, bool value)
{
  Channel* channel = &m_channels[channel_index];
  if (channel->gate_input == value)
    return;

  Log_DevPrintf("Set PIC channel %u gate input %s->%s", Truncate32(channel_index), channel->gate_input ? "high" : "low",
                value ? "high" : "low");

  // Ensure we're up-to-date.
  m_tick_event->InvokeEarly();

  bool rising = !channel->gate_input;
  bool falling = channel->gate_input;
  channel->gate_input = value;

  switch (channel->operating_mode)
  {
    case ChannelOperatingModeOneShot:
    {
      // After the reload register has been set the PIT will wait for the next rising edge of the gate input. Once this
      // occurs, the output signal will go low and the current count will be set to the reload value on the next falling
      // edge of the (1.193182 MHz) input signal.
      if (!channel->waiting_for_reload && rising)
      {
        SetChannelOutputState(channel_index, false);
        channel->waiting_for_gate = false;

        // However, if the gate input goes high again it will cause the current count to be reloaded from the reload
        // register on the next falling edge of the input signal, and restart the count again.

        // We leave the reload value as-is, and set reload_value_set, which causes it to be copied on the next clock
        // cycle.
        channel->reload_value_set = true;
      }
    }
    break;

    case ChannelOperatingModeRateGenerator:
    case ChannelOperatingModeSquareWaveGenerator:
    {
      // If the gate input goes low, counting stops and the output goes high immediately. Once the gate input has
      // returned high, the next falling edge on input signal will cause the current count to be set to the reload value
      // and operation will continue.
      if (falling)
        SetChannelOutputState(channel_index, true);
      else
        channel->reload_value_set = true;
    }
    break;

    case ChannelOperatingModeHardwareTriggeredStrobe:
    {
      // After the reload register has been set the PIT will wait for the next rising edge of the gate input.
      if (!channel->waiting_for_reload && rising)
        channel->reload_value_set = true;
    }
    break;

    default:
      break;
  }

  UpdateAllChannelsDowncount();
}

void i8253_PIT::SimulateChannel(size_t channel_index, CycleCount cycles)
{
  Channel* channel = &m_channels[channel_index];
  switch (channel->operating_mode)
  {
    case ChannelOperatingModeInterrupt:
    {
      if (channel->waiting_for_reload)
      {
        channel->monitor_count += cycles;
        break;
      }

      // After the reload register has been set, the current count will be set to the reload value on the next falling
      // edge of the (1.193182 MHz) input signal. Subsequent falling edges of the input signal will decrement the
      // current count (if the gate input is high on the preceding rising edge of the input signal).
      if (channel->reload_value_set)
      {
        channel->count = GetFrequencyFromReloadValue(channel);
        channel->reload_value_set = false;
      }

      while (cycles >= channel->count)
      {
        channel->monitor_count += channel->count;
        cycles -= channel->count;
        channel->count = 0;

        // When the current count decrements from one to zero, the output goes high and remains high until another
        // mode/command register is written or the reload register is set again.
        SetChannelOutputState(channel_index, true);

        // The current count will wrap around to 0xFFFF (or 0x9999 in BCD mode) and continue to decrement until the
        // mode/command register or the reload register are set, however this will not effect the output pin state.
        channel->count = channel->bcd_mode ? 0x9999 : 0xFFFF;
      }

      channel->count -= cycles;
      channel->monitor_count += cycles;
      break;
    }

    case ChannelOperatingModeOneShot:
    {
      // This mode is similar to mode 0 above, however counting doesn't start until a rising edge of the gate input is
      // detected.
      if (channel->waiting_for_reload || channel->waiting_for_gate)
      {
        channel->monitor_count += cycles;
        break;
      }

      if (channel->reload_value_set)
      {
        channel->count = GetFrequencyFromReloadValue(channel);
        channel->reload_value_set = false;
      }

      while (cycles >= channel->count)
      {
        channel->monitor_count += channel->count;
        cycles -= channel->count;
        channel->count = 0;

        // When the current count decrements from one to zero, the output goes high and remains high until another
        // mode/command register is written or the reload register is set again.
        SetChannelOutputState(channel_index, true);

        // The current count will wrap around to 0xFFFF (or 0x9999 in BCD mode) and continue to decrement until the
        // mode/command register or the reload register are set, however this will not effect the output pin state.
        channel->count = channel->bcd_mode ? 0x9999 : 0xFFFF;
      }

      channel->count -= cycles;
      channel->monitor_count += cycles;
      break;
    }

    case ChannelOperatingModeRateGenerator:
    {
      // See note in SetChannelGateInput, the counter is disabled while the gate input is low.
      if (channel->waiting_for_reload || !channel->gate_input)
      {
        channel->monitor_count += cycles;
        break;
      }

      if (channel->reload_value_set)
      {
        channel->count = GetFrequencyFromReloadValue(channel);
        channel->reload_value_set = false;
      }

      // Add one here because we want to trigger when the counter is one as well as zero.
      while (cycles > 0 && (cycles + 1) >= channel->count)
      {
        // When the current count decrements from two to one, the output goes low, and on the next falling edge of the
        // (1.193182 MHz) input signal it will go high again and the current count will be set to the reload value and
        // counting will continue.
        if (channel->count > 1)
        {
          channel->monitor_count += (channel->count - 1);
          cycles -= (channel->count - 1);
          channel->count = 1;
          SetChannelOutputState(channel_index, false);
        }

        // It's possible that we're set to 1 now.
        if (cycles > 0)
        {
          channel->monitor_count++;
          cycles--;
          channel->count = GetFrequencyFromReloadValue(channel);
          SetChannelOutputState(channel_index, true);
        }
      }

      // Ensure output is high when counting.
      if (cycles > 0)
      {
        channel->count -= cycles;
        channel->monitor_count += cycles;
        SetChannelOutputState(channel_index, true);
      }

      break;
    }

    case ChannelOperatingModeSquareWaveGenerator:
    {
      // See note in SetChannelGateInput, the counter is disabled while the gate input is low.
      if (channel->waiting_for_reload || !channel->gate_input)
      {
        channel->monitor_count += cycles;
        break;
      }

      if (channel->reload_value_set)
      {
        channel->count = GetFrequencyFromReloadValue(channel) & ~CycleCount(1);
        channel->reload_value_set = false;
      }

      // For mode 3, the PIT channel operates as a frequency divider like mode 2, however the output signal is fed into
      // an internal "flip flop" to produce a square wave (rather than a short pulse). The flip flop changes its output
      // state each time its input state (or the output of the PIT channel's frequency divider) changes. This causes the
      // actual output to change state half as often, so to compensate for this the current count is decremented twice
      // on each falling edge of the input signal (instead of once), and the current count is set to the reload value
      // twice as often.

      // Rather than subtracting two, we multiply the cycle count by two.
      cycles *= 2;

      while (cycles >= channel->count)
      {
        channel->monitor_count += (channel->count / 2);
        cycles -= channel->count;
        channel->count = GetFrequencyFromReloadValue(channel) & ~CycleCount(1);

        // For even reload values, when the current count decrements from two to zero the output of the flop-flop
        // changes state; the current count will be reset to the reload value and counting will continue.
        channel->square_wave_flip_flop ^= true;
        SetChannelOutputState(channel_index, channel->square_wave_flip_flop);

        // For odd reload values, the current count is always set to one less than the reload value. If the output of
        // the flip flop is low when the current count decrements from two to zero it will behave the same as the
        // equivalent even reload value. However, if the output of the flip flop is high the reload will be delayed for
        // one input signal cycle (0.8381 uS), which causes the "high" pulse to be slightly longer and the duty cycle
        // will not be exactly 50%. Because the reload value is rounded down to the nearest even number anyway, it is
        // recommended that only even reload values be used (which means you should mask the value before sending it to
        // the port).
        if ((channel->reload_value % 2) != 0)
        {
          // We cheat here and offset the count by one, rather than delaying another cycle.
          // Not accurate, but unlikely for the CPU to read the value in between these two PIT cycles.
          if (channel->square_wave_flip_flop)
            channel->count++;
        }
      }

      channel->count -= cycles;
      channel->monitor_count += (cycles / 2);
      break;
    }

    case ChannelOperatingModeSoftwareTriggeredStrobe:
    case ChannelOperatingModeHardwareTriggeredStrobe:
    {
      if (channel->waiting_for_reload ||
          (channel->operating_mode == ChannelOperatingModeHardwareTriggeredStrobe && channel->waiting_for_gate))
      {
        channel->monitor_count += cycles;
        break;
      }

      if (channel->reload_value_set)
      {
        channel->count = GetFrequencyFromReloadValue(channel);
        channel->reload_value_set = false;
      }

      while (cycles >= channel->count)
      {
        // When the current count decrements from one to zero, the output goes low for one cycle of the input signal
        // (0.8381 uS).
        cycles -= channel->count;
        channel->monitor_count += channel->count;
        channel->count = GetFrequencyFromReloadValue(channel);
        SetChannelOutputState(channel_index, false);

        // We simulate the one cycle output by adding one to the frequency.
        // In real-time, this happens early, but in cycles it'll be correct.
        channel->monitor_count++;
        SetChannelOutputState(channel_index, true);
        channel->count++;
      }

      channel->count -= cycles;
      channel->monitor_count += cycles;
      break;
    }

    default:
      break;
  }

  // Fire monitor callback if present.
  if (channel->monitor_callback && channel->monitor_count > 0)
  {
    CycleCount monitor_count = channel->monitor_count;
    channel->monitor_count = 0;
    channel->monitor_callback(monitor_count, channel->output_state);
  }
}

CycleCount i8253_PIT::UpdateChannelDowncount(size_t channel_index)
{
  Channel* channel = &m_channels[channel_index];
  switch (channel->operating_mode)
  {
    case ChannelOperatingModeInterrupt:
    {
      if (!channel->waiting_for_reload && channel->HasCallback() && !channel->output_state)
        channel->downcount = channel->reload_value_set ? 1 : channel->count;
      else
        channel->downcount = 0;
    }
    break;

    case ChannelOperatingModeOneShot:
    {
      if (!channel->waiting_for_reload && !channel->waiting_for_gate && channel->HasCallback() &&
          !channel->output_state)
        channel->downcount = channel->reload_value_set ? 1 : channel->count;
      else
        channel->downcount = 0;
    }
    break;

    case ChannelOperatingModeRateGenerator:
    {
      if (!channel->waiting_for_reload && channel->gate_input && channel->HasCallback())
        channel->downcount = channel->reload_value_set ? 1 : std::max(channel->count - 1, CycleCount(1));
      else
        channel->downcount = 0;
    }
    break;

    case ChannelOperatingModeSquareWaveGenerator:
    {
      if (!channel->waiting_for_reload && channel->gate_input && channel->HasCallback())
        channel->downcount = channel->reload_value_set ? 1 : std::max(channel->count / 2, CycleCount(1));
      else
        channel->downcount = 0;
    }
    break;

    case ChannelOperatingModeSoftwareTriggeredStrobe:
    case ChannelOperatingModeHardwareTriggeredStrobe:
    {
      if (!channel->waiting_for_reload && !channel->waiting_for_gate && channel->HasCallback() &&
          !channel->output_state)
        channel->downcount = channel->reload_value_set ? 1 : channel->count;
      else
        channel->downcount = 0;
    }
    break;
  }

  return channel->downcount;
}

void i8253_PIT::SetChannelOutputState(size_t channel_index, bool value)
{
  Channel* channel = &m_channels[channel_index];
  if (channel->output_state == value)
    return;

  // if (channel_index != 1)
  Log_TracePrintf("Set PIT channel %u output %s->%s after %u cycles", Truncate32(channel_index),
                  channel->output_state ? "high" : "low", value ? "high" : "low", Truncate32(channel->monitor_count));

  CycleCount monitor_count = channel->monitor_count;
  bool monitor_output_state = channel->output_state;
  channel->monitor_count = 0;
  channel->output_state = value;

  if (channel->change_callback)
    channel->change_callback(value);

  if (channel->monitor_callback && monitor_count > 0)
    channel->monitor_callback(monitor_count, monitor_output_state);
}

void i8253_PIT::UpdateAllChannelsDowncount()
{
  // TODO: This could be optimized and do the min() call here.
  for (size_t i = 0; i < NUM_CHANNELS; i++)
    UpdateChannelDowncount(i);

  RescheduleTimerEvent();
}

} // namespace HW
