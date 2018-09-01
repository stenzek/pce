#include "pce/hw/pcspeaker.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Timer.h"
#include "pce/host_interface.h"
Log_SetChannel(HW::PCSpeaker);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(PCSpeaker);
BEGIN_OBJECT_PROPERTY_MAP(PCSpeaker)
END_OBJECT_PROPERTY_MAP()

PCSpeaker::PCSpeaker() : m_clock("PC Speaker", OUTPUT_FREQUENCY) {}

PCSpeaker::~PCSpeaker()
{
  if (m_output_channel)
    m_system->GetHostInterface()->GetAudioMixer()->RemoveChannel(m_output_channel);
}

bool PCSpeaker::Initialize(System* system, Bus* bus)
{
  m_system = system;
  m_clock.SetManager(system->GetTimingManager());

  m_output_channel = m_system->GetHostInterface()->GetAudioMixer()->CreateChannel("PC Speaker", OUTPUT_FREQUENCY,
                                                                                  Audio::SampleFormat::Signed16, 1);
  if (!m_output_channel)
    Panic("Failed to create PC speaker output channel");

  // Render samples every 100ms, or when the level changes.
  m_render_sample_event = m_clock.NewEvent("Render Samples", CycleCount(OUTPUT_FREQUENCY / Audio::MixFrequency),
                                           std::bind(&PCSpeaker::RenderSampleEvent, this, std::placeholders::_2));

  return true;
}

bool PCSpeaker::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != 0x12345678)
    return false;

  m_output_channel->ClearBuffer();

  reader.SafeReadBool(&m_output_enabled);
  reader.SafeReadBool(&m_level);

  return !reader.GetErrorState();
}

bool PCSpeaker::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(0x12345678);

  writer.SafeWriteBool(m_output_enabled);
  writer.SafeWriteBool(m_level);

  return !writer.InErrorState();
}

void PCSpeaker::Reset()
{
  m_output_channel->ClearBuffer();
  m_output_enabled = false;
  m_level = false;
}

void PCSpeaker::SetOutputEnabled(bool enabled)
{
  if (m_output_enabled == enabled)
    return;

  // Ensure output is up to date first.
  m_render_sample_event->InvokeEarly();
  m_output_enabled = enabled;
}

void PCSpeaker::SetLevel(bool level)
{
  if (m_level == level)
    return;

  m_render_sample_event->InvokeEarly();
  m_level = level;
}

void PCSpeaker::RenderSampleEvent(CycleCount cycles)
{
  size_t num_samples = size_t(cycles);

  //     static SimulationTime total_audio_time;
  //     total_audio_time += cycles * m_render_sample_event->GetCyclePeriod();
  //     Log_ErrorPrintf("TOTAL SPEAKER RENDER TIME: %.2f ms", total_audio_time / 1000000.0);

  // This functions as a low-pass filter, preventing noise from low frequencies.
  // TODO: Calculate a better value here.
  bool sound_on = m_output_enabled;
  if (num_samples < 2)
    sound_on = false;

  int16* samples = reinterpret_cast<int16*>(m_output_channel->ReserveInputSamples(num_samples));
  int16 value = sound_on ? (m_level ? HIGH_SAMPLE_VALUE : LOW_SAMPLE_VALUE) : 0;
  for (size_t i = 0; i < num_samples; i++)
    samples[i] = value;

#if 0
  static FILE* fp = nullptr;
  if (!fp)
    fp = fopen("D:\\speaker.raw", "wb");
  if (fp)
  {
    fwrite(samples, sizeof(int16), num_samples, fp);
    fflush(fp);
  }
#endif

  Log_TracePrintf("Speaker: rendered %u samples at %s", Truncate32(num_samples),
                  sound_on ? (m_level ? "high" : "low") : "off");
  m_output_channel->CommitInputSamples(num_samples);

#if 0
  static Timer t;
  static size_t sc = 0;
  sc += num_samples;
  if (t.GetTimeSeconds() >= 1.0)
  {
    Log_WarningPrintf("Samples per second: %.4f", (float)(sc / t.GetTimeSeconds()));
    t.Reset();
    sc = 0;
  }
#endif
}

} // namespace HW