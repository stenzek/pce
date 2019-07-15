#include "pce/hw/ymf262.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Timer.h"
#include "pce/host_interface.h"
#include "pce/thirdparty/dosbox/dbopl.h"
#include <cmath>

namespace HW {

YMF262::YMF262(Mode mode) : m_mode(mode)
{
  DBOPL::InitTables();

  m_num_chips = (m_mode == Mode::DualOPL2 ? 2 : 1);
  for (size_t i = 0; i < m_num_chips; i++)
    m_chips[i].dbopl = std::make_unique<DBOPL::Chip>();
}

YMF262::~YMF262()
{
#ifdef YMF262_USE_THREAD
  m_worker_thread.ExitWorkers();
#endif
  if (m_output_channel)
    m_system->GetHostInterface()->GetAudioMixer()->RemoveChannel(m_output_channel);
}

bool YMF262::Initialize(System* system)
{
  m_system = system;

  m_output_channel = m_system->GetHostInterface()->GetAudioMixer()->CreateChannel(
    "YMF262", OUTPUT_FREQUENCY, Audio::SampleFormat::Signed16, IsStereo() ? 2 : 1);
  if (!m_output_channel)
  {
    Panic("Failed to create YMF262 output channel");
    return false;
  }

  // Render samples every 100ms, or when the level changes.
  m_render_sample_event =
    m_system->CreateClockedEvent("YMF262 Render", OUTPUT_FREQUENCY, CycleCount(OUTPUT_FREQUENCY / Audio::MixFrequency),
                                 std::bind(&YMF262::RenderSampleEvent, this, std::placeholders::_2), true);

  // Timer events, start disabled
  for (size_t i = 0; i < m_num_chips; i++)
  {
    ChipState& chip = m_chips[i];
    chip.timers[0].event =
      m_system->CreateClockedEvent("YMF262 Timer 1 Expire", OUTPUT_FREQUENCY, 1,
                                   std::bind(&YMF262::TimerExpiredEvent, this, i, 0, std::placeholders::_2), false);
    chip.timers[1].event =
      m_system->CreateClockedEvent("YMF262 Timer 2 Expire", OUTPUT_FREQUENCY, 1,
                                   std::bind(&YMF262::TimerExpiredEvent, this, i, 1, std::placeholders::_2), false);
  }

#ifdef YMF262_USE_THREAD
  m_worker_thread.Initialize(TaskQueue::DefaultQueueSize, 1);
#endif

  return true;
}

void YMF262::Reset()
{
  FlushWorkerThread();
  m_output_channel->ClearBuffer();

  for (size_t i = 0; i < m_num_chips; i++)
  {
    ChipState& chip = m_chips[i];
    chip.address_register = 0;
    chip.volume = 1.0f;

    for (ChipState::Timer& timer : chip.timers)
    {
      timer.reload_value = 256;
      timer.value = 0;
      timer.masked = false;
      timer.expired = false;
      timer.active = false;
      if (timer.event->IsActive())
        timer.event->Deactivate();
    }

    // Hopefully this clears out everything.
    chip.dbopl->Setup(static_cast<Bit32u>(OUTPUT_FREQUENCY));
  }
}

bool YMF262::LoadChipState(size_t chip_index, BinaryReader& reader)
{
  ChipState* chip = &m_chips[chip_index];
  DebugAssert(chip->dbopl);
  FlushWorkerThread();

  reader.SafeReadUInt32(&chip->address_register);
  reader.SafeReadFloat(&chip->volume);

  for (ChipState::Timer& timer : chip->timers)
  {
    reader.SafeReadUInt16(&timer.reload_value);
    reader.SafeReadUInt16(&timer.value);
    reader.SafeReadBool(&timer.masked);
    reader.SafeReadBool(&timer.expired);
    reader.SafeReadBool(&timer.active);
  }

  reader.SafeReadUInt32(&chip->dbopl->lfoCounter);
  reader.SafeReadUInt32(&chip->dbopl->lfoAdd);
  reader.SafeReadUInt32(&chip->dbopl->noiseCounter);
  reader.SafeReadUInt32(&chip->dbopl->noiseAdd);
  reader.SafeReadUInt32(&chip->dbopl->noiseValue);
  for (size_t i = 0; i < countof(chip->dbopl->freqMul); i++)
    reader.SafeReadUInt32(&chip->dbopl->freqMul[i]);
  for (size_t i = 0; i < countof(chip->dbopl->linearRates); i++)
    reader.SafeReadUInt32(&chip->dbopl->linearRates[i]);
  for (size_t i = 0; i < countof(chip->dbopl->attackRates); i++)
    reader.SafeReadUInt32(&chip->dbopl->attackRates[i]);

  for (size_t chan_idx = 0; chan_idx < countof(chip->dbopl->chan) && !reader.GetErrorState(); chan_idx++)
  {
    DBOPL::Channel* chan = &chip->dbopl->chan[chan_idx];
    for (size_t op_index = 0; op_index < countof(chan->op); op_index++)
    {
      DBOPL::Operator* op = &chan->op[op_index];

      u8 state = 0;
      u8 regE0 = 0;
      reader.SafeReadUInt8(&state);
      reader.SafeReadUInt8(&regE0);

      // Use state to set volumeHandler.
      op->SetState(state);

      // Use E0 to set waveBase.
      op->regE0 = 0;
      op->WriteE0(chip->dbopl.get(), regE0);

#if (DBOPL_WAVE != WAVE_HANDLER)
      reader.SafeReadUInt32(&op->waveMask);
      reader.SafeReadUInt32(&op->waveStart);
#endif

      reader.SafeReadUInt32(&op->waveIndex);
      reader.SafeReadUInt32(&op->waveAdd);
      reader.SafeReadUInt32(&op->waveCurrent);

      reader.SafeReadUInt32(&op->chanData);
      reader.SafeReadUInt32(&op->freqMul);
      reader.SafeReadUInt32(&op->vibrato);
      reader.SafeReadInt32(&op->sustainLevel);
      reader.SafeReadInt32(&op->totalLevel);
      reader.SafeReadUInt32(&op->currentLevel);
      reader.SafeReadInt32(&op->volume);

      reader.SafeReadUInt32(&op->attackAdd);
      reader.SafeReadUInt32(&op->decayAdd);
      reader.SafeReadUInt32(&op->releaseAdd);
      reader.SafeReadUInt32(&op->rateIndex);

      reader.SafeReadUInt8(&op->rateZero);
      reader.SafeReadUInt8(&op->keyOn);
      reader.SafeReadUInt8(&op->reg20);
      reader.SafeReadUInt8(&op->reg40);
      reader.SafeReadUInt8(&op->reg60);
      reader.SafeReadUInt8(&op->reg80);
      reader.SafeReadUInt8(&op->tremoloMask);
      reader.SafeReadUInt8(&op->vibStrength);
      reader.SafeReadUInt8(&op->ksr);
    }

    reader.SafeReadUInt32(&chan->chanData);
    for (size_t i = 0; i < countof(chan->old); i++)
      reader.SafeReadInt32(&chan->old[i]);

    reader.SafeReadUInt8(&chan->feedback);
    reader.SafeReadUInt8(&chan->regB0);
    reader.SafeReadUInt8(&chan->regC0);
    reader.SafeReadUInt8(&chan->fourMask);
    reader.SafeReadInt8(&chan->maskLeft);
    reader.SafeReadInt8(&chan->maskRight);
  }

  if (reader.GetErrorState())
    return false;

  // Use regC0 to set synthHandler for each channel.
  // This is done down here because this each channel can depend on another.
  for (size_t chan_idx = 0; chan_idx < countof(chip->dbopl->chan); chan_idx++)
  {
    DBOPL::Channel* chan = &chip->dbopl->chan[chan_idx];
    u8 regC0 = chan->regC0;
    chan->regC0 = 0;
    chan->WriteC0(chip->dbopl.get(), regC0);
  }

  // Fix up timer events as best we can for now.
  // The exact downcount will be loaded with the other timing events.
  for (size_t i = 0; i < NUM_TIMERS; i++)
    UpdateTimerEvent(chip_index, i);

  return true;
}

bool YMF262::SaveChipState(size_t chip_index, BinaryWriter& writer) const
{
  const ChipState* chip = &m_chips[chip_index];
  DebugAssert(chip->dbopl);

  writer.SafeWriteUInt32(chip->address_register);
  writer.SafeWriteFloat(chip->volume);

  for (const ChipState::Timer& timer : chip->timers)
  {
    writer.SafeWriteUInt16(timer.reload_value);
    writer.SafeWriteUInt16(timer.value);
    writer.SafeWriteBool(timer.masked);
    writer.SafeWriteBool(timer.expired);
    writer.SafeWriteBool(timer.active);
  }

  writer.SafeWriteUInt32(chip->dbopl->lfoCounter);
  writer.SafeWriteUInt32(chip->dbopl->lfoAdd);
  writer.SafeWriteUInt32(chip->dbopl->noiseCounter);
  writer.SafeWriteUInt32(chip->dbopl->noiseAdd);
  writer.SafeWriteUInt32(chip->dbopl->noiseValue);
  for (size_t i = 0; i < countof(chip->dbopl->freqMul); i++)
    writer.SafeWriteUInt32(chip->dbopl->freqMul[i]);
  for (size_t i = 0; i < countof(chip->dbopl->linearRates); i++)
    writer.SafeWriteUInt32(chip->dbopl->linearRates[i]);
  for (size_t i = 0; i < countof(chip->dbopl->attackRates); i++)
    writer.SafeWriteUInt32(chip->dbopl->attackRates[i]);

  for (size_t chan_idx = 0; chan_idx < countof(chip->dbopl->chan) && !writer.InErrorState(); chan_idx++)
  {
    const DBOPL::Channel* chan = &chip->dbopl->chan[chan_idx];
    for (size_t op_index = 0; op_index < countof(chan->op); op_index++)
    {
      const DBOPL::Operator* op = &chan->op[op_index];

      writer.SafeWriteUInt8(op->state);
      writer.SafeWriteUInt8(op->regE0);

#if (DBOPL_WAVE != WAVE_HANDLER)
      writer.SafeWriteUInt32(op->waveMask);
      writer.SafeWriteUInt32(op->waveStart);
#endif

      writer.SafeWriteUInt32(op->waveIndex);
      writer.SafeWriteUInt32(op->waveAdd);
      writer.SafeWriteUInt32(op->waveCurrent);

      writer.SafeWriteUInt32(op->chanData);
      writer.SafeWriteUInt32(op->freqMul);
      writer.SafeWriteUInt32(op->vibrato);
      writer.SafeWriteInt32(op->sustainLevel);
      writer.SafeWriteInt32(op->totalLevel);
      writer.SafeWriteUInt32(op->currentLevel);
      writer.SafeWriteInt32(op->volume);

      writer.SafeWriteUInt32(op->attackAdd);
      writer.SafeWriteUInt32(op->decayAdd);
      writer.SafeWriteUInt32(op->releaseAdd);
      writer.SafeWriteUInt32(op->rateIndex);

      writer.SafeWriteUInt8(op->rateZero);
      writer.SafeWriteUInt8(op->keyOn);
      writer.SafeWriteUInt8(op->reg20);
      writer.SafeWriteUInt8(op->reg40);
      writer.SafeWriteUInt8(op->reg60);
      writer.SafeWriteUInt8(op->reg80);
      writer.SafeWriteUInt8(op->tremoloMask);
      writer.SafeWriteUInt8(op->vibStrength);
      writer.SafeWriteUInt8(op->ksr);
    }

    writer.SafeWriteUInt32(chan->chanData);
    for (size_t i = 0; i < countof(chan->old); i++)
      writer.SafeWriteInt32(chan->old[i]);

    writer.SafeWriteUInt8(chan->feedback);
    writer.SafeWriteUInt8(chan->regB0);
    writer.SafeWriteUInt8(chan->regC0);
    writer.SafeWriteUInt8(chan->fourMask);
    writer.SafeWriteInt8(chan->maskLeft);
    writer.SafeWriteInt8(chan->maskRight);
  }

  return !writer.InErrorState();
}

bool YMF262::LoadState(BinaryReader& reader)
{
  FlushWorkerThread();
  m_output_channel->ClearBuffer();

  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  u32 mode_uint;
  if (!reader.SafeReadUInt32(&mode_uint) || mode_uint != static_cast<u32>(m_mode))
    return false;

  for (size_t i = 0; i < m_num_chips; i++)
  {
    if (!LoadChipState(i, reader))
      return false;
  }

  return !reader.GetErrorState();
}

bool YMF262::SaveState(BinaryWriter& writer) const
{
  FlushWorkerThread();

  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteUInt32(static_cast<u32>(m_mode));

  for (size_t i = 0; i < m_num_chips; i++)
  {
    if (!SaveChipState(i, writer))
      return false;
  }

  return !writer.InErrorState();
}

u8 YMF262::ReadAddressPort(size_t chip_index)
{
  ChipState& chip = m_chips[chip_index];
  DebugAssert(chip_index < m_num_chips);

  for (ChipState::Timer& timer : chip.timers)
    timer.event->InvokeEarly();

  // Report timer state.
  u8 out_value = 0;
  if (chip.timers[0].expired & !chip.timers[0].masked)
    out_value |= 0x40 | 0x80;
  if (chip.timers[1].expired & !chip.timers[1].masked)
    out_value |= 0x20 | 0x80;

  return out_value;
}

u8 YMF262::ReadDataPort(size_t chip_index)
{
  // ChipState& chip = m_chips[chip_index];
  // DebugAssert(chip_index < m_num_chips);

  return 0xFF;
}

void YMF262::WriteAddressPort(size_t chip_index, u8 value)
{
  ChipState& chip = m_chips[chip_index];
  DebugAssert(chip_index < m_num_chips);

  chip.address_register = chip.dbopl->WriteAddr(0, value);
}

void YMF262::WriteDataPort(size_t chip_index, u8 value)
{
  ChipState& chip = m_chips[chip_index];
  DebugAssert(chip_index < m_num_chips);

  m_render_sample_event->InvokeEarly();

  const u32 address_register = chip.address_register;
#ifdef YMF262_USE_THREAD
  m_worker_thread.QueueLambdaTask(
    [this, chip_index, address_register, value]() { m_chips[chip_index].dbopl->WriteReg(address_register, value); });
#else
  chip.dbopl->WriteReg(address_register, value);
#endif

  // We control timer registers ourselves.
  switch (address_register)
  {
      // Timer 0 value
    case 0x02:
      chip.timers[0].reload_value = 256 - ZeroExtend16(value);
      break;

      // Timer 1 value
    case 0x03:
      chip.timers[1].reload_value = 256 - ZeroExtend16(value);
      break;

      // Timer control
    case 0x04:
    {
      if (value & 0x80)
      {
        // Reset timers
        for (size_t i = 0; i < NUM_TIMERS; i++)
        {
          chip.timers[0].value = chip.timers[0].reload_value;
          chip.timers[i].expired = false;
          UpdateTimerEvent(chip_index, i);
        }
      }
      else
      {
        for (size_t i = 0; i < NUM_TIMERS; i++)
        {
          ChipState::Timer& timer = chip.timers[i];
          timer.event->InvokeEarly();

          bool new_active = ((value & (1 << i)) != 0);
          if (new_active != timer.active)
          {
            if (new_active)
              timer.value = timer.reload_value;
            timer.active = new_active;
            UpdateTimerEvent(chip_index, i);
          }
        }

        // TODO: Raise late interrupt if masked when it expired?
        chip.timers[0].masked = (value & 0x40) != 0;
        if (chip.timers[0].masked)
          chip.timers[0].expired = false;
        chip.timers[1].masked = (value & 0x20) != 0;
        if (chip.timers[1].masked)
          chip.timers[1].expired = false;
      }
    }
    break;
  }
}

void YMF262::DualWriteAddressPort(u8 value)
{
  WriteAddressPort(0, value);
  WriteAddressPort(1, value);
}

void YMF262::DualWriteDataPort(u8 value)
{
  WriteDataPort(0, value);
  WriteDataPort(1, value);
}

void YMF262::SetVolume(size_t chip_index, float volume)
{
  DebugAssert(chip_index < m_num_chips);
  m_render_sample_event->InvokeEarly();

#ifdef YMF262_USE_THREAD
  m_worker_thread.QueueLambdaTask([this, chip_index, volume]() { m_chips[chip_index].volume = volume; });
#else
  m_chips[chip_index].volume = volume;
#endif
}

void YMF262::RenderSampleEvent(CycleCount cycles)
{
  u32 num_samples = u32(cycles);

#ifdef YMF262_USE_THREAD
  m_worker_thread.QueueLambdaTask([this, num_samples]() { RenderSamples(num_samples); });
#else
  RenderSamples(num_samples);
#endif
}

inline s16 ConvertSample(s32 in_sample, float factor)
{
  // TODO: Beware of clipping here.
  // return static_cast<int16>(in_sample);
  // return static_cast<int16>(std::max(INT32_C(-32768), std::min(INT32_C(32767), in_sample * 8)));
  return static_cast<s16>(std::max(INT32_C(-32768), std::min(INT32_C(32767), s32(float(in_sample) * factor))));
}

void YMF262::RenderSamples(u32 num_samples)
{
  float gain_factor = float(std::pow(10.0f, (GAIN / 10.0f)));

  switch (m_mode)
  {
    case Mode::OPL2:
    {
      ChipState& chip = m_chips[0];
      if (num_samples > chip.temp_buffer.size())
        chip.temp_buffer.resize(num_samples);

      chip.dbopl->GenerateBlock2(Truncate32(num_samples), chip.temp_buffer.data());

      s16* output_samples = reinterpret_cast<s16*>(m_output_channel->ReserveInputSamples(num_samples));
      float factor = gain_factor * m_chips[0].volume;

      for (size_t i = 0; i < num_samples; i++)
        output_samples[i] = ConvertSample(chip.temp_buffer[i], factor);

      m_output_channel->CommitInputSamples(num_samples);
    }
    break;

    case Mode::DualOPL2:
    {
      for (ChipState& chip : m_chips)
      {
        if (num_samples > chip.temp_buffer.size())
          chip.temp_buffer.resize(num_samples);

        chip.dbopl->GenerateBlock2(Truncate32(num_samples), chip.temp_buffer.data());
      }

      s16* output_samples = reinterpret_cast<s16*>(m_output_channel->ReserveInputSamples(num_samples));
      float factor0 = gain_factor * m_chips[0].volume;
      float factor1 = gain_factor * m_chips[1].volume;

      for (size_t i = 0; i < num_samples; i++)
      {
        output_samples[i * 2 + 0] = ConvertSample(m_chips[0].temp_buffer[i], factor0);
        output_samples[i * 2 + 1] = ConvertSample(m_chips[1].temp_buffer[i], factor1);
      }

      m_output_channel->CommitInputSamples(num_samples);
    }
    break;

    case Mode::OPL3:
    {
      ChipState& chip = m_chips[0];
      if ((num_samples * 2) > chip.temp_buffer.size())
        chip.temp_buffer.resize(num_samples * 2);

      s16* output_samples = reinterpret_cast<s16*>(m_output_channel->ReserveInputSamples(num_samples));
      float factor = gain_factor * m_chips[0].volume;

      if (chip.dbopl->opl3Active != 0)
      {
        // Stereo samples
        chip.dbopl->GenerateBlock3(Truncate32(num_samples), chip.temp_buffer.data());
        for (size_t i = 0; i < num_samples * 2; i++)
          output_samples[i] = ConvertSample(chip.temp_buffer[i], factor);
      }
      else
      {
        // Mono samples, so duplicate across both channels.
        chip.dbopl->GenerateBlock2(Truncate32(num_samples), chip.temp_buffer.data());
        for (size_t i = 0; i < num_samples; i++)
          output_samples[i * 2 + 0] = output_samples[i * 2 + 1] = ConvertSample(chip.temp_buffer[i], factor);
      }

      m_output_channel->CommitInputSamples(num_samples);
    }
    break;
  }
}

void YMF262::TimerExpiredEvent(size_t chip_index, size_t timer_index, CycleCount cycles)
{
  ChipState::Timer& timer = m_chips[chip_index].timers[timer_index];
  if (cycles >= static_cast<CycleCount>(timer.value))
  {
    // Timer expired.
    timer.expired = true;
    timer.event->Deactivate();
    return;
  }

  // Still has time remaining.
  timer.value -= u8(cycles);
  timer.event->Reschedule(CycleCount(timer.value));
}

void YMF262::UpdateTimerEvent(size_t chip_index, size_t timer_index)
{
  ChipState::Timer& timer = m_chips[chip_index].timers[timer_index];
  if (timer.expired || !timer.active)
  {
    if (timer.event->IsActive())
      timer.event->Deactivate();
  }
  else
  {
    // If the timer has a count of zero, it would fire immediately..
    if (timer.value == 0)
    {
      timer.expired = true;
      if (timer.event->IsActive())
        timer.event->Deactivate();
      return;
    }

    if (timer.event->IsActive())
      timer.event->Reschedule(CycleCount(timer.value));
    else
      timer.event->Queue(CycleCount(timer.value));
  }
}

void YMF262::FlushWorkerThread() const
{
#ifdef YMF262_USE_THREAD
  m_worker_thread.QueueBlockingLambdaTask([]() {});
#endif
}

} // namespace HW