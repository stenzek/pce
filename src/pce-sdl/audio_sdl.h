#pragma once
#include "pce/audio.h"
#include <SDL_audio.h>

class Mixer_SDL : public Audio::Mixer
{
public:
  Mixer_SDL(SDL_AudioDeviceID device_id, float output_sample_rate);
  virtual ~Mixer_SDL();

  static std::unique_ptr<Mixer> Create();

protected:
  virtual void RenderSamples(size_t output_samples) override;

private:
  SDL_AudioDeviceID m_device_id;
};
