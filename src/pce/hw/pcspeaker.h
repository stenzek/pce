#pragma once

#include "pce/audio.h"
#include "pce/bitfield.h"
#include "pce/clock.h"
#include "pce/component.h"
#include "pce/system.h"

namespace HW {

// This is kinda a garbage implementation of a PC speaker, considering it can't render amplitudes other than -1/+1.
class PCSpeaker final : public Component
{
public:
  static constexpr float OUTPUT_FREQUENCY = 22050.0f;
  static constexpr float VOLUME = 0.1f;

public:
  PCSpeaker();
  ~PCSpeaker();

  void Initialize(System* system, Bus* bus) override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;
  void Reset() override;

  bool IsOutputEnabled() const { return m_output_enabled; }
  void SetOutputEnabled(bool enabled);

  void SetLevel(bool level);

private:
  static constexpr int16 LOW_SAMPLE_VALUE = int16(VOLUME * -32768);
  static constexpr int16 HIGH_SAMPLE_VALUE = int16(VOLUME * 32767);

  void RenderSampleEvent(CycleCount cycles);

  Clock m_clock;
  System* m_system = nullptr;
  Audio::Channel* m_output_channel = nullptr;

  bool m_output_enabled = false;
  bool m_level = false;

  TimingEvent::Pointer m_render_sample_event;
};

} // namespace HW