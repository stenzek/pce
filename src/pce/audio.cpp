#include "pce/audio.h"
#include "YBaseLib/Log.h"
#include "samplerate.h"
#include <cmath>
#include <cstring>
Log_SetChannel(Audio);

namespace Audio {

size_t GetBytesPerSample(SampleFormat format)
{
  switch (format)
  {
    case SampleFormat::Signed8:
    case SampleFormat::Unsigned8:
      return sizeof(uint8);

    case SampleFormat::Signed16:
    case SampleFormat::Unsigned16:
      return sizeof(uint16);

    case SampleFormat::Signed32:
      return sizeof(int32);

    case SampleFormat::Float32:
      return sizeof(float);
  }

  Panic("Unhandled format");
  return 1;
}

Mixer::Mixer(uint32 output_sample_rate, uint32 output_buffer_size)
  : m_output_sample_rate(output_sample_rate), m_output_buffer_size(output_buffer_size)
{
}

Mixer::~Mixer() {}

Channel* Mixer::CreateChannel(const char* name, float sample_rate, SampleFormat format, size_t channels,
                              size_t buffer_count /* = DefaultBufferCount */)
{
  Assert(!GetChannelByName(name));

  float sample_ratio = sample_rate / float(m_output_sample_rate);
  size_t buffer_size = size_t(std::ceil(m_output_buffer_size * sample_ratio));

  std::unique_ptr<Channel> channel =
    std::make_unique<Channel>(name, buffer_size, buffer_count, m_output_sample_rate, sample_rate, format, channels);
  m_channels.push_back(std::move(channel));
  return m_channels.back().get();
}

void Mixer::RemoveChannel(Channel* channel)
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

Channel* Mixer::GetChannelByName(const char* name)
{
  for (auto& channel : m_channels)
  {
    if (channel->GetName().Compare(name))
      return channel.get();
  }

  return nullptr;
}

void Mixer::ClearBuffers()
{
  for (const auto& channel : m_channels)
    channel->ClearBuffer();
}

void Mixer::CheckRenderBufferSize(size_t num_samples, size_t num_channels)
{
  size_t buffer_size = num_samples * num_channels;
  if (m_render_buffer.size() < buffer_size)
    m_render_buffer.resize(buffer_size);
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

Channel::Channel(const char* name, size_t buffer_size, size_t buffer_count, uint32 output_sample_rate,
                 float input_sample_rate, SampleFormat format, size_t channels)
  : m_name(name), m_buffer_size(buffer_size), m_buffer_count(buffer_count), m_input_sample_rate(input_sample_rate),
    m_output_sample_rate(output_sample_rate), m_format(format), m_channels(channels), m_enabled(true),
    m_frame_size(GetBytesPerSample(format) * channels), m_output_frame_size(sizeof(float) * channels),
    m_resample_buffer(buffer_size * channels), m_resampler_state(src_new(SRC_SINC_FASTEST, int(channels), nullptr)),
    m_resample_ratio(double(output_sample_rate) / double(input_sample_rate))
{
  Assert(m_resampler_state != nullptr);
  AllocateBuffers(buffer_count);
}

Channel::~Channel()
{
  src_delete(reinterpret_cast<SRC_STATE*>(m_resampler_state));
}

void Channel::BeginWrite(void** buffer_ptr, size_t* num_frames)
{
  m_lock.Lock();

  if (m_num_free_buffers == 0)
    DropBuffer();

  Buffer& buffer = m_buffers[m_first_free_buffer];
  *buffer_ptr = &buffer.data[m_frame_size * buffer.write_position];
  *num_frames = m_buffer_size - buffer.write_position;
}

void Channel::WriteFrames(const void* samples, size_t num_frames)
{
  m_lock.Lock();
  size_t remaining_samples = num_frames;

  const byte* samples_ptr = reinterpret_cast<const byte*>(samples);
  while (remaining_samples > 0)
  {
    if (m_num_free_buffers == 0)
      DropBuffer();

    Buffer& buffer = m_buffers[m_first_free_buffer];
    const size_t to_this_buffer = std::min(m_buffer_size - buffer.write_position, remaining_samples);

    const size_t copy_size = to_this_buffer * m_frame_size;
    std::memcpy(&buffer.data[buffer.write_position * m_frame_size], samples_ptr, copy_size);
    samples_ptr += copy_size;

    remaining_samples -= to_this_buffer;
    buffer.write_position += to_this_buffer;

    // End of the buffer?
    if (buffer.write_position == m_buffer_size)
    {
      // Reset it back to the start, and enqueue it.
      buffer.write_position = 0;
      m_num_free_buffers--;
      m_first_free_buffer = (m_first_free_buffer + 1) % m_buffers.size();
      m_num_available_buffers++;
    }
  }

  m_lock.Unlock();
}

void Channel::EndWrite(size_t num_frames)
{
  Buffer& buffer = m_buffers[m_first_free_buffer];
  DebugAssert((buffer.write_position + num_frames) <= m_buffer_size);
  buffer.write_position += num_frames;

  // End of the buffer?
  if (buffer.write_position == m_buffer_size)
  {
    // Reset it back to the start, and enqueue it.
    // Log_DevPrintf("Enqueue buffer %u", m_first_free_buffer);
    buffer.write_position = 0;
    m_num_free_buffers--;
    m_first_free_buffer = (m_first_free_buffer + 1) % m_buffers.size();
    m_num_available_buffers++;
  }

  m_lock.Unlock();
}

void Channel::ChangeSampleRate(float new_sample_rate)
{
  MutexLock lock(m_lock);
  InternalClearBuffer();

  // Calculate the new ratio.
  m_input_sample_rate = new_sample_rate;
  m_resample_ratio = double(m_output_sample_rate) / double(new_sample_rate);
  m_buffer_size = size_t(std::ceil(m_buffer_size / m_resample_ratio));
  m_resample_buffer.resize(m_buffer_size * m_channels);
  AllocateBuffers(m_buffer_count);
}

void Channel::ClearBuffer()
{
  MutexLock lock(m_lock);
  InternalClearBuffer();
}

void Channel::AllocateBuffers(size_t buffer_count)
{
  m_buffers.resize(buffer_count);
  for (size_t i = 0; i < buffer_count; i++)
  {
    Buffer& buffer = m_buffers[i];
    buffer.data.resize(m_buffer_size * m_frame_size);
    buffer.read_position = 0;
    buffer.write_position = 0;
  }

  m_first_available_buffer = 0;
  m_num_available_buffers = 0;
  m_first_free_buffer = 0;
  m_num_free_buffers = buffer_count;
}

void Channel::DropBuffer()
{
  DebugAssert(m_num_available_buffers > 0);
  // Log_DevPrintf("Dropping buffer %u", m_first_free_buffer);

  // Out of space. We'll overwrite the oldest buffer with the new data.
  // At the same time, we shift the available buffer forward one.
  m_first_available_buffer = (m_first_available_buffer + 1) % m_buffers.size();
  m_num_available_buffers--;

  m_buffers[m_first_free_buffer].read_position = 0;
  m_buffers[m_first_free_buffer].write_position = 0;
  m_num_free_buffers++;
}

void Channel::InternalClearBuffer()
{
  for (Buffer& buffer : m_buffers)
  {
    buffer.read_position = 0;
    buffer.write_position = 0;
  }

  m_first_free_buffer = 0;
  m_num_free_buffers = m_buffers.size();
  m_first_available_buffer = 0;
  m_num_available_buffers = 0;

  src_reset(reinterpret_cast<SRC_STATE*>(m_resampler_state));
}

size_t Channel::ReadFrames(float* destination, size_t num_frames)
{
  m_lock.Lock();

  size_t remaining_frames = num_frames;
  while (remaining_frames > 0 && m_num_available_buffers > 0)
  {
    // TODO: Instead of converting all frames, we could just do the first n*ratio.
    Buffer& buffer = m_buffers[m_first_available_buffer];
    const byte* in_data = &buffer.data[buffer.read_position * m_frame_size];
    const size_t in_num_frames = m_buffer_size - buffer.read_position;
    const size_t in_num_samples = in_num_frames * m_channels;

    // Set up resampling.
    SRC_DATA resample_data;
    resample_data.input_frames = static_cast<long>(in_num_frames);
    resample_data.input_frames_used = 0;
    resample_data.data_out = destination;
    resample_data.output_frames = static_cast<long>(remaining_frames);
    resample_data.output_frames_gen = 0;
    resample_data.end_of_input = 0;
    resample_data.src_ratio = m_resample_ratio;

    // Convert from whatever format the input is in to float.
    switch (m_format)
    {
      case SampleFormat::Signed8:
      {
        const int8* in_samples_typed = reinterpret_cast<const int8*>(in_data);
        for (size_t i = 0; i < in_num_samples; i++)
          m_resample_buffer[i] = float(in_samples_typed[i]) / float(0x80);

        resample_data.data_in = m_resample_buffer.data();
      }
      break;
      case SampleFormat::Unsigned8:
      {
        const int8* in_samples_typed = reinterpret_cast<const int8*>(in_data);
        for (size_t i = 0; i < in_num_samples; i++)
          m_resample_buffer[i] = float(int(in_samples_typed[i]) - 128) / float(0x80);

        resample_data.data_in = m_resample_buffer.data();
      }
      break;
      case SampleFormat::Signed16:
        src_short_to_float_array(reinterpret_cast<const short*>(in_data), m_resample_buffer.data(),
                                 int(in_num_samples));
        resample_data.data_in = m_resample_buffer.data();
        break;

      case SampleFormat::Unsigned16:
      {
        const uint16* in_samples_typed = reinterpret_cast<const uint16*>(in_data);
        for (size_t i = 0; i < in_num_samples; i++)
          m_resample_buffer[i] = float(int(in_samples_typed[i]) - 32768) / float(0x8000);

        resample_data.data_in = m_resample_buffer.data();
      }
      break;

      case SampleFormat::Signed32:
        src_int_to_float_array(reinterpret_cast<const int*>(in_data), m_resample_buffer.data(), int(in_num_samples));
        resample_data.data_in = m_resample_buffer.data();
        break;

      case SampleFormat::Float32:
      default:
        resample_data.data_in = reinterpret_cast<const float*>(in_data);
        break;
    }

    // Actually perform the resampling.
    int process_result = src_process(reinterpret_cast<SRC_STATE*>(m_resampler_state), &resample_data);
    Assert(process_result == 0);

    // Update buffer pointers.
    buffer.read_position += resample_data.input_frames_used;
    destination += resample_data.output_frames_gen * m_channels;
    remaining_frames -= resample_data.output_frames_gen;

    // End of buffer?
    if (buffer.read_position >= m_buffer_size)
    {
      // End of this buffer.
      DebugAssert(buffer.read_position == m_buffer_size);
      // Log_DevPrintf("Finish dequeing buffer %u", m_first_available_buffer);
      buffer.read_position = 0;
      m_num_available_buffers--;
      m_first_available_buffer = (m_first_available_buffer + 1) % m_buffers.size();
      m_num_free_buffers++;
    }
  }

  m_lock.Unlock();
  return num_frames - remaining_frames;
}

NullMixer::NullMixer() : Mixer(44100, 2048) {}

NullMixer::~NullMixer() {}

std::unique_ptr<Mixer> NullMixer::Create()
{
  // The null mixer never reads frames, they just keep getting overwritten.
  return std::make_unique<NullMixer>();
}

} // namespace Audio
