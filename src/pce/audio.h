#pragma once

#include <memory>
#include <vector>

#include "common/types.h"

class AudioChannel;
class AudioMixer;

enum class AudioSampleFormat
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

// Output format type.
constexpr AudioSampleFormat OutputFormat = AudioSampleFormat::Float32;
using OutputFormatType = float;

// Get the number of bytes for each element of a sample format.
size_t GetBytesPerSample(AudioSampleFormat format);

// Base audio class, handles mixing/resampling
class AudioMixer
{
public:
  AudioMixer();
  ~AudioMixer();

  // Adds a channel to the audio mixer.
  // This pointer is owned by the audio class.
  AudioChannel* CreateChannel(float sample_rate, AudioSampleFormat format, size_t channels);

  // Drops a channel from the audio mixer.
  void RemoveChannel(AudioChannel* channel);

  // Clears all buffers. Use when changing speed limiter state, or loading state.
  void ClearBuffers(SimulationTime new_time);

  // Generates output audio.
  void Mix(SimulationTime time);

private:
  static constexpr float OUTPUT_SAMPLE_RATE = 44100.0f; // Default output sample rate
  static constexpr SimulationTime OUTPUT_FRAME_SIM_PERIOD =
    static_cast<SimulationTime>(1000000000.0 / OUTPUT_SAMPLE_RATE);

  // Input channels.
  std::vector<std::unique_ptr<AudioChannel>> m_channels;
  std::vector<float> m_mix_buffer;
  SimulationTime m_last_mix_time = 0;
  void* m_output_stream = nullptr;
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
class AudioChannel
{
public:
  AudioChannel(float output_sample_rate, float input_sample_rate, AudioSampleFormat format, size_t channels);
  ~AudioChannel();

  float GetSampleRate() const { return m_input_sample_rate; }
  AudioSampleFormat GetFormat() const { return m_format; }
  size_t GetChannels() const { return m_channels; }

  // When the channel is disabled, adding samples will have no effect, and it won't affect the output.
  bool IsEnabled() const { return m_enabled; }
  void SetEnabled(bool enabled) { m_enabled = enabled; }

  // This sample_count is the number of samples per channel, so two-channel will be half of the total values.
  size_t GetFreeInputSamples();
  void* ReserveInputSamples(size_t sample_count);
  void CommitInputSamples(size_t sample_count);

  // Resamples at most num_output_samples, the actual number can be lower if there isn't enough input data.
  bool ResampleInput(size_t num_output_samples);

  // Render n output samples. If not enough input data is in the buffer, set to zero.
  void ReadSamples(float* destination, size_t num_samples);

  // Changes the frequency of the input data. Flushes the resample buffer.
  void ChangeSampleRate(float new_sample_rate);

  // Clears the buffer. Use when loading state or changing speed limiter.
  void ClearBuffer();

private:
  void InternalClearBuffer();

  float m_input_sample_rate;
  float m_output_sample_rate;
  AudioSampleFormat m_format;
  size_t m_channels;
  bool m_enabled;

  // std::unique_ptr<CircularBuffer> m_input_buffer;
  size_t m_input_sample_size;
  size_t m_input_frame_size;
  size_t m_output_frame_size;
  AudioBuffer m_input_buffer;
  AudioBuffer m_output_buffer;
  std::vector<float> m_resample_buffer;
  double m_resample_ratio;
  void* m_resampler_state;
};
