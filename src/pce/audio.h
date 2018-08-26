#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "YBaseLib/CircularBuffer.h"
#include "YBaseLib/Mutex.h"
#include "YBaseLib/MutexLock.h"
#include "YBaseLib/String.h"
#include "pce/types.h"

namespace Audio {

class Channel;
class Mixer;

enum class SampleFormat
{
  Signed8,
  Unsigned8,
  Signed16,
  Unsigned16,
  Signed32,
  Float32
};

// Constants for the maximums we support.
constexpr size_t NumOutputChannels = 2;

// We buffer one second of data either way.
constexpr float InputBufferLengthInSeconds = 1.0f;
constexpr float OutputBufferLengthInSeconds = 1.0f;

// Audio render frequency. We render the elapsed simulated time worth of audio at this interval.
// Currently it is every 50ms, or 20hz. For audio channels, it's recommended to render at twice this
// frequency in order to ensure that there is always data ready. This could also be mitigated by buffering.
constexpr uint32 MixFrequency = 20;
constexpr SimulationTime MixInterval = SimulationTime(1000000000) / SimulationTime(MixFrequency);

// We buffer 10ms of input before rendering any samples, that way we don't get buffer underruns.
constexpr SimulationTime ChannelDelayTimeInSimTime = SimulationTime(10000000);

// Output format type.
constexpr SampleFormat OutputFormat = SampleFormat::Float32;
using OutputFormatType = float;

// Get the number of bytes for each element of a sample format.
size_t GetBytesPerSample(SampleFormat format);

// Base audio class, handles mixing/resampling
class Mixer
{
public:
  enum : uint32
  {
    DefaultOutputSampleRate = 44100,
    DefaultBufferSize = 2048,
    DefaultBufferCount = 3,
  };

  Mixer(uint32 output_sample_rate, uint32 output_buffer_size);
  virtual ~Mixer();

  // Disable all outputs.
  // This prevents any samples being written to the device, but still consumes samples.
  bool IsMuted() const { return m_muted; }
  void SetMuted(bool muted) { m_muted = muted; }

  // Adds a channel to the audio mixer.
  // This pointer is owned by the audio class.
  Channel* CreateChannel(const char* name, float sample_rate, SampleFormat format, size_t channels,
                         size_t buffer_count = DefaultBufferCount);

  // Drops a channel from the audio mixer.
  void RemoveChannel(Channel* channel);

  // Looks up channel by name. Shouldn't really be needed.
  Channel* GetChannelByName(const char* name);

  // Clears all buffers. Use when changing speed limiter state, or loading state.
  void ClearBuffers();

protected:
  void CheckRenderBufferSize(size_t num_samples, size_t num_channels);

  uint32 m_output_sample_rate;
  uint32 m_output_buffer_size;
  bool m_muted = false;

  // Input channels.
  std::vector<std::unique_ptr<Channel>> m_channels;

  // Render/resampling buffer.
  std::vector<OutputFormatType> m_render_buffer;
};

class AudioBuffer
{
public:
  AudioBuffer(size_t size);

  size_t GetBufferUsed() const;
  size_t GetContiguousBufferSpace() const;

  void Clear();

  bool Read(void* dst, size_t len);

  bool GetWritePointer(void** ptr, size_t* len);

  void MoveWritePointer(size_t len);

  bool GetReadPointer(const void** ppReadPointer, size_t* pByteCount) const;

  void MoveReadPointer(size_t byteCount);

private:
  std::vector<byte> m_buffer;
  size_t m_used = 0;
};

// A channel, or source of audio for the mixer.
class Channel
{
public:
  Channel(const char* name, size_t buffer_size, size_t buffer_count, uint32 output_sample_rate, float input_sample_rate,
          SampleFormat format, size_t channels);
  ~Channel();

  const String& GetName() const { return m_name; }
  float GetSampleRate() const { return m_input_sample_rate; }
  SampleFormat GetFormat() const { return m_format; }
  size_t GetChannels() const { return m_channels; }

  // When the channel is disabled, adding samples will have no effect, and it won't affect the output.
  bool IsEnabled() const { return m_enabled; }
  void SetEnabled(bool enabled) { m_enabled = enabled; }

  // This sample_count is the number of samples per channel, so two-channel will be half of the total values.
  void BeginWrite(void** buffer_ptr, size_t* num_frames);
  void WriteFrames(const void* samples, size_t num_frames);
  void EndWrite(size_t num_frames);

  // Render n output samples. Returns the number of samples written to destination.
  size_t ReadFrames(float* destination, size_t num_frames);

  // Changes the frequency of the input data. Flushes the resample buffer.
  void ChangeSampleRate(float new_sample_rate);

  // Clears the buffer. Use when loading state or changing speed limiter.
  void ClearBuffer();

private:
  void InternalClearBuffer();
  void AllocateBuffers(size_t buffer_count);
  void DropBuffer();

  struct Buffer
  {
    std::vector<byte> data;
    size_t write_position;
    size_t read_position;
  };

  String m_name;
  size_t m_buffer_size;
  size_t m_buffer_count;
  float m_input_sample_rate;
  uint32 m_output_sample_rate;
  SampleFormat m_format;
  size_t m_channels;
  bool m_enabled;

  Mutex m_lock;
  size_t m_frame_size;
  size_t m_output_frame_size;

  std::vector<Buffer> m_buffers;

  // For input.
  size_t m_first_free_buffer = 0;
  size_t m_num_free_buffers = 0;

  // For output.
  size_t m_num_available_buffers = 0;
  size_t m_first_available_buffer = 0;

  std::vector<float> m_resample_buffer;
  double m_resample_ratio;
  void* m_resampler_state;
};

// Null audio sink/mixer
class NullMixer : public Mixer
{
public:
  NullMixer();
  virtual ~NullMixer();

  static std::unique_ptr<Mixer> Create();
};

} // namespace Audio
