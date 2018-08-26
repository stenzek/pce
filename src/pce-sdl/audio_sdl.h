#pragma once
#include "pce/audio.h"
#include <SDL_audio.h>

class Mixer_SDL : public Audio::Mixer
{
public:
  Mixer_SDL(SDL_AudioDeviceID device_id, uint32 output_sample_rate, uint32 output_buffer_size);
  virtual ~Mixer_SDL();

  static std::unique_ptr<Mixer> Create();

protected:
  void RenderSamples(Audio::OutputFormatType* buf, size_t num_frames);
  static void RenderCallback(void* userdata, Uint8* stream, int len);

private:
  SDL_AudioDeviceID m_device_id;
};
