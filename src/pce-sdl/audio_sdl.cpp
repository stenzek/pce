#include "pce-sdl/audio_sdl.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Timer.h"
#include <SDL_audio.h>
Log_SetChannel(Audio_SDL);

using namespace Audio;

inline SDL_AudioFormat GetSDLAudioFormat(SampleFormat format)
{
  switch (format)
  {
    case SampleFormat::Signed8:
      return AUDIO_S8;

    case SampleFormat::Unsigned8:
      return AUDIO_U8;

    case SampleFormat::Signed16:
      return AUDIO_S16SYS;

    case SampleFormat::Unsigned16:
      return AUDIO_U16SYS;

    case SampleFormat::Signed32:
      return AUDIO_S32SYS;

    case SampleFormat::Float32:
      return AUDIO_F32;
  }

  Panic("Unhandled format");
  return AUDIO_U8;
}

Mixer_SDL::Mixer_SDL(SDL_AudioDeviceID device_id, uint32 output_sample_rate, uint32 output_buffer_size)
  : Mixer(output_sample_rate, output_buffer_size), m_device_id(device_id)
{
}

Mixer_SDL::~Mixer_SDL()
{
  SDL_CloseAudioDevice(m_device_id);
}

std::unique_ptr<Mixer> Mixer_SDL::Create()
{
  auto mixer = std::make_unique<Mixer_SDL>(0, DefaultOutputSampleRate, DefaultBufferSize);
  SDL_AudioSpec spec = {static_cast<int>(mixer->m_output_sample_rate),
                        AUDIO_F32,
                        static_cast<Uint8>(NumOutputChannels),
                        0,
                        static_cast<Uint16>(mixer->m_output_buffer_size),
                        0,
                        0,
                        RenderCallback,
                        mixer.get()};
  SDL_AudioSpec obtained_spec;
  SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(nullptr, 0, &spec, &obtained_spec, 0);
  if (device_id == 0)
    return nullptr;

  mixer->m_device_id = device_id;

  SDL_PauseAudioDevice(device_id, SDL_FALSE);

  return mixer;
}

void Mixer_SDL::RenderSamples(Audio::OutputFormatType* buf, size_t num_frames)
{
  std::fill_n(buf, num_frames * NumOutputChannels, 0.0f);

  for (auto& channel : m_channels)
  {
    CheckRenderBufferSize(num_frames, channel->GetChannels());
    size_t frames_read = channel->ReadFrames(m_render_buffer.data(), num_frames);

    // Don't bother mixing it if we're muted.
    if (frames_read == 0 || m_muted)
      continue;

    // If the format is the same, we can just copy it as-is..
    if (channel->GetChannels() == 1 && NumOutputChannels == 2)
    {
      // Mono -> stereo
      for (size_t i = 0; i < frames_read; i++)
      {
        buf[i * 2 + 0] += m_render_buffer[i];
        buf[i * 2 + 1] += m_render_buffer[i];
      }

      continue;
    }
    else if (channel->GetChannels() != NumOutputChannels)
    {
      SDL_AudioCVT cvt;
      int err = SDL_BuildAudioCVT(&cvt, AUDIO_F32, Truncate8(channel->GetChannels()), int(m_output_sample_rate),
                                  AUDIO_F32, Truncate8(NumOutputChannels), int(m_output_sample_rate));
      if (err != 1)
        Panic("Failed to set up audio conversion");

      cvt.len = int(channel->GetChannels() * sizeof(OutputFormatType) * frames_read);
      cvt.buf = reinterpret_cast<Uint8*>(m_render_buffer.data());
      err = SDL_ConvertAudio(&cvt);
      if (err != 0)
        Panic("Failed to convert audio");
    }

    // Mix channels together.
    const Audio::OutputFormatType* mix_src = m_render_buffer.data();
    Audio::OutputFormatType* mix_dst = buf;
    for (size_t i = 0; i < frames_read; i++)
    {
      // TODO: Saturation/clamping here
      *(mix_dst++) += *(mix_src++);
    }
  }

#if 0
  static FILE* fp = nullptr;
  if (!fp)
    fp = fopen("D:\\mixed.raw", "wb");
  if (fp)
  {
    fwrite(buf, sizeof(float), num_samples * NumOutputChannels, fp);
    fflush(fp);
  }
#endif
}

void Mixer_SDL::RenderCallback(void* userdata, Uint8* stream, int len)
{
  Mixer_SDL* mixer = static_cast<Mixer_SDL*>(userdata);
  Audio::OutputFormatType* buf = reinterpret_cast<Audio::OutputFormatType*>(stream);
  size_t num_samples = size_t(len) / NumOutputChannels / sizeof(Audio::OutputFormatType);
  if (num_samples > 0)
    mixer->RenderSamples(buf, num_samples);
}
