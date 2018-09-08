#pragma once
#include "common/audio.h"
#include <SDL_audio.h>

class Mixer_SDL : public Audio::Mixer
{
public:
  Mixer_SDL(SDL_AudioDeviceID device_id, float output_sample_rate);
  virtual ~Mixer_SDL();

  static std::unique_ptr<Mixer_SDL> Create();

protected:
  void RenderSamples(Audio::OutputFormatType* buf, size_t num_samples);
  static void RenderCallback(void* userdata, Uint8* stream, int len);

private:
  SDL_AudioDeviceID m_device_id;
};
