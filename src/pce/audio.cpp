#include "audio.h"
#include "YBaseLib/Assert.h"
#include "host_interface.h"
#include "samplerate.h"
#include <cmath>
#include <cstring>
#include <retro2/retro2_session.h>

size_t GetBytesPerSample(AudioSampleFormat format)
{
  switch (format)
  {
    case AudioSampleFormat::Signed8:
    case AudioSampleFormat::Unsigned8:
      return sizeof(u8);

    case AudioSampleFormat::Signed16:
    case AudioSampleFormat::Unsigned16:
      return sizeof(u16);

    case AudioSampleFormat::Signed32:
      return sizeof(s32);

    case AudioSampleFormat::Float32:
      return sizeof(float);
  }

  Panic("Unhandled format");
  return 1;
}

AudioMixer::AudioMixer()
{
  m_output_stream = R2Session.CreateAudioStream(static_cast<u32>(OUTPUT_SAMPLE_RATE), NumOutputChannels,
                                                RETRO2_AUDIO_FORMAT_S16, true, nullptr);
}

AudioMixer::~AudioMixer()
{
  R2Session.DestroyAudioStream(m_output_stream);
}

AudioChannel* AudioMixer::CreateChannel(float sample_rate, AudioSampleFormat format, size_t channels)
{
  std::unique_ptr<AudioChannel> channel =
    std::make_unique<AudioChannel>(OUTPUT_SAMPLE_RATE, sample_rate, format, channels);
  m_channels.push_back(std::move(channel));
  return m_channels.back().get();
}

void AudioMixer::RemoveChannel(AudioChannel* channel)
{
  for (auto iter = m_channels.begin(); iter != m_channels.end(); iter++)
  {
    if (iter->get() == channel)
    {
      m_channels.erase(iter);
      return;
    }
  }

  Panic("Removing unknown channel.");
}

void AudioMixer::ClearBuffers(SimulationTime new_time)
{
  for (const auto& channel : m_channels)
    channel->ClearBuffer();
  m_last_mix_time = new_time;
}

void AudioMixer::Mix(SimulationTime time)
{
  const SimulationTime delta_time = GetSimulationTimeDifference(m_last_mix_time, time);
  const size_t frames_to_generate = static_cast<size_t>(delta_time / OUTPUT_FRAME_SIM_PERIOD);
  if (frames_to_generate == 0)
    return;

  m_last_mix_time += frames_to_generate * OUTPUT_FRAME_SIM_PERIOD;

  const size_t samples_to_generate = frames_to_generate * NumOutputChannels;
  const size_t mix_buffer_size = samples_to_generate * 2;
  if (m_mix_buffer.size() < mix_buffer_size)
    m_mix_buffer.resize(mix_buffer_size);

  float* const mix_buffer = m_mix_buffer.data();
  float* const render_buffer = mix_buffer + samples_to_generate;
  bool first = true;

  for (std::unique_ptr<AudioChannel>& channel : m_channels)
  {
    // Read samples from the channel.
    channel->ReadSamples(render_buffer, frames_to_generate);

    if (channel->GetChannels() == 1)
    {
      // Mono -> stereo
      for (ssize_t idx = ssize_t(frames_to_generate) - 1; idx >= 0; idx--)
      {
        float sample = render_buffer[idx];
        render_buffer[idx * 2 + 0] = sample;
        render_buffer[idx * 2 + 1] = sample;
      }
    }

    float* mixbuf_ptr = mix_buffer;
    float* renderbuf_ptr = render_buffer;
    if (first)
    {
      first = false;
      for (size_t samples_remaining = samples_to_generate; samples_remaining > 0; samples_remaining--)
        *(mixbuf_ptr++) = *(renderbuf_ptr++);
    }
    else
    {
      for (size_t samples_remaining = samples_to_generate; samples_remaining > 0; samples_remaining--)
        *(mixbuf_ptr++) += *(renderbuf_ptr++);
    }
  }

  float* mixbuf_ptr = mix_buffer;
  for (size_t frames_remaining = frames_to_generate; frames_remaining > 0;)
  {
    s16* outsamples_ptr;
    size_t max_frames;
    R2Session.BeginWriteAudioFrames(m_output_stream, reinterpret_cast<void**>(&outsamples_ptr), &max_frames);

    const size_t batch_frames = std::min(frames_remaining, max_frames);
    for (size_t batch_remaining = batch_frames * NumOutputChannels; batch_remaining > 0; batch_remaining--)
    {
      *(outsamples_ptr++) = static_cast<s16>(std::clamp(*(mixbuf_ptr++) * 32767.0f, -32768.0f, 32767.0f));
    }

    frames_remaining -= batch_frames;
    R2Session.EndWriteAudioFrames(m_output_stream, batch_frames);
  }
}

AudioBuffer::AudioBuffer(size_t size) : m_buffer(size) {}

size_t AudioBuffer::GetBufferUsed() const
{
  return m_used;
}

size_t AudioBuffer::GetContiguousBufferSpace() const
{
  return m_buffer.size() - m_used;
}

void AudioBuffer::Clear()
{
  m_used = 0;
}

bool AudioBuffer::Read(void* dst, size_t len)
{
  if (len > m_used)
    return false;

  std::memcpy(dst, m_buffer.data(), len);
  m_used -= len;
  if (m_used > 0)
    std::memmove(m_buffer.data(), m_buffer.data() + len, m_used);

  return true;
}

bool AudioBuffer::GetWritePointer(void** ptr, size_t* len)
{
  size_t free = GetContiguousBufferSpace();
  if (*len > free)
    return false;

  *len = free;
  *ptr = m_buffer.data() + m_used;
  return true;
}

void AudioBuffer::MoveWritePointer(size_t len)
{
  DebugAssert(m_used + len <= m_buffer.size());
  m_used += len;
}

bool AudioBuffer::GetReadPointer(const void** ppReadPointer, size_t* pByteCount) const
{
  if (m_used == 0)
    return false;

  *ppReadPointer = m_buffer.data();
  *pByteCount = m_used;
  return true;
}

void AudioBuffer::MoveReadPointer(size_t byteCount)
{
  DebugAssert(byteCount <= m_used);
  m_used -= byteCount;
  if (m_used > 0)
    std::memmove(m_buffer.data(), m_buffer.data() + byteCount, m_used);
}

AudioChannel::AudioChannel(float output_sample_rate, float input_sample_rate, AudioSampleFormat format, size_t channels)
  : m_input_sample_rate(input_sample_rate), m_output_sample_rate(output_sample_rate), m_format(format),
    m_channels(channels), m_enabled(true), m_input_sample_size(GetBytesPerSample(format)),
    m_input_frame_size(GetBytesPerSample(format) * channels), m_output_frame_size(sizeof(float) * channels),
    m_input_buffer(u32(float(InputBufferLengthInSeconds * input_sample_rate)) * channels * m_input_sample_size),
    m_output_buffer(u32(float(InputBufferLengthInSeconds * output_sample_rate)) * channels * sizeof(OutputFormatType)),
    m_resample_buffer(u32(float(InputBufferLengthInSeconds * output_sample_rate)) * channels),
    m_resample_ratio(double(output_sample_rate) / double(input_sample_rate)),
    m_resampler_state(src_new(SRC_SINC_FASTEST, int(channels), nullptr))
{
  Assert(m_resampler_state != nullptr);
}

AudioChannel::~AudioChannel()
{
  src_delete(reinterpret_cast<SRC_STATE*>(m_resampler_state));
}

size_t AudioChannel::GetFreeInputSamples()
{
  return m_input_buffer.GetContiguousBufferSpace() / m_input_frame_size;
}

void* AudioChannel::ReserveInputSamples(size_t sample_count)
{
  void* write_ptr;
  size_t byte_count = sample_count * m_input_frame_size;

  // When the speed limiter is off, we can easily exceed the audio buffer length.
  // In this case, just destroy the oldest samples, wrapping around.
  while (!m_input_buffer.GetWritePointer(&write_ptr, &byte_count))
  {
    size_t bytes_to_remove = byte_count - m_input_buffer.GetContiguousBufferSpace();
    m_input_buffer.MoveReadPointer(bytes_to_remove);
  }

  return write_ptr;
}

void AudioChannel::CommitInputSamples(size_t sample_count)
{
  size_t byte_count = sample_count * m_input_frame_size;
  m_input_buffer.MoveWritePointer(byte_count);
}

void AudioChannel::ReadSamples(float* destination, size_t num_samples)
{
  while (num_samples > 0)
  {
    // Can we use what we have buffered?
    size_t currently_buffered = m_output_buffer.GetBufferUsed() / m_output_frame_size;
    if (currently_buffered > 0)
    {
      size_t to_read = std::min(num_samples, currently_buffered);
      m_output_buffer.Read(destination, to_read * m_output_frame_size);
      destination += to_read;
      num_samples -= to_read;
      if (num_samples == 0)
        break;
    }

    // Resample num_samples samples
    if (m_input_buffer.GetBufferUsed() > 0)
    {
      if (ResampleInput(num_samples))
        continue;
    }

    // If we hit here, it's because we're out of input data.
    std::memset(destination, 0, num_samples * m_output_frame_size);
    break;
  }
}

void AudioChannel::ChangeSampleRate(float new_sample_rate)
{
  InternalClearBuffer();

  // Calculate the new ratio.
  m_input_sample_rate = new_sample_rate;
  m_resample_ratio = double(m_output_sample_rate) / double(new_sample_rate);
}

void AudioChannel::ClearBuffer()
{
  InternalClearBuffer();
}

void AudioChannel::InternalClearBuffer()
{
  m_input_buffer.Clear();
  src_reset(reinterpret_cast<SRC_STATE*>(m_resampler_state));
  m_output_buffer.Clear();
}

bool AudioChannel::ResampleInput(size_t num_output_samples)
{
  const void* in_buf;
  size_t in_bufsize;
  size_t in_num_frames;
  size_t in_num_samples;
  if (!m_input_buffer.GetReadPointer(&in_buf, &in_bufsize))
    return false;

  in_num_frames = in_bufsize / m_input_frame_size;
  in_num_samples = in_num_frames * m_channels;
  if (in_num_frames == 0)
    return false;

  // Cap output samples at buffer size.
  num_output_samples = std::min(num_output_samples, m_output_buffer.GetContiguousBufferSpace() / m_output_frame_size);
  Assert((num_output_samples * m_channels) <= m_resample_buffer.size());

  // Only use as many input samples as needed.
  void* out_buf;
  size_t out_bufsize = num_output_samples * m_output_frame_size;
  if (!m_output_buffer.GetWritePointer(&out_buf, &out_bufsize))
    return false;

  // Set up resampling.
  SRC_DATA resample_data;
  resample_data.data_out = reinterpret_cast<float*>(out_buf);
  resample_data.output_frames = static_cast<long>(num_output_samples);
  resample_data.input_frames_used = 0;
  resample_data.output_frames_gen = 0;
  resample_data.end_of_input = 0;
  resample_data.src_ratio = m_resample_ratio;

  // Convert from whatever format the input is in to float.
  switch (m_format)
  {
    case AudioSampleFormat::Signed8: {
      const s8* in_samples_typed = reinterpret_cast<const s8*>(in_buf);
      for (size_t i = 0; i < in_num_samples; i++)
        m_resample_buffer[i] = float(in_samples_typed[i]) / float(0x80);

      resample_data.input_frames = long(in_num_frames);
      resample_data.data_in = m_resample_buffer.data();
    }
    break;
    case AudioSampleFormat::Unsigned8: {
      const s8* in_samples_typed = reinterpret_cast<const s8*>(in_buf);
      for (size_t i = 0; i < in_num_samples; i++)
        m_resample_buffer[i] = float(int(in_samples_typed[i]) - 128) / float(0x80);

      resample_data.input_frames = long(in_num_frames);
      resample_data.data_in = m_resample_buffer.data();
    }
    break;
    case AudioSampleFormat::Signed16:
      src_short_to_float_array(reinterpret_cast<const short*>(in_buf), m_resample_buffer.data(), int(in_num_samples));
      resample_data.input_frames = long(in_num_frames);
      resample_data.data_in = m_resample_buffer.data();
      break;

    case AudioSampleFormat::Unsigned16: {
      const u16* in_samples_typed = reinterpret_cast<const u16*>(in_buf);
      for (size_t i = 0; i < in_num_samples; i++)
        m_resample_buffer[i] = float(int(in_samples_typed[i]) - 32768) / float(0x8000);

      resample_data.input_frames = long(in_num_frames);
      resample_data.data_in = m_resample_buffer.data();
    }
    break;

    case AudioSampleFormat::Signed32:
      src_int_to_float_array(reinterpret_cast<const int*>(in_buf), m_resample_buffer.data(), int(in_num_samples));
      resample_data.input_frames = long(in_num_frames);
      resample_data.data_in = m_resample_buffer.data();
      break;

    case AudioSampleFormat::Float32:
    default:
      resample_data.input_frames = long(in_num_frames);
      resample_data.data_in = reinterpret_cast<const float*>(in_buf);
      break;
  }

  // Actually perform the resampling.
  int process_result = src_process(reinterpret_cast<SRC_STATE*>(m_resampler_state), &resample_data);
  Assert(process_result == 0);

  // Update buffer pointers.
  m_input_buffer.MoveReadPointer(size_t(resample_data.input_frames_used) * m_input_frame_size);
  m_output_buffer.MoveWritePointer(size_t(resample_data.output_frames_gen) * m_output_frame_size);
  return true;
}
