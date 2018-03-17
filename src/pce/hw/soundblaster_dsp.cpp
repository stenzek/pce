#include "pce/hw/soundblaster_dsp.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Timer.h"
#include "pce/dma_controller.h"
#include "pce/host_interface.h"
#include "pce/hw/soundblaster_dsp_adpcm.inl"
#include "pce/interrupt_controller.h"
#include <cmath>
Log_SetChannel(HW::SoundBlaster);

// https://courses.engr.illinois.edu/ece390/resources/sound/sbdsp.txt.html
// http://flint.cs.yale.edu/cs422/readings/hardware/SoundBlaster.pdf

namespace HW {

SoundBlaster::SoundBlaster(DMAController* dma_controller, uint32 version, uint32 irq, uint32 dma, uint32 dma16)
  : m_clock("Sound Blaster DSP", 44100), m_dma_controller(dma_controller), m_version(version), m_irq(irq),
    m_dma_channel(dma), m_dma_channel_16(dma16)
{
}

SoundBlaster::~SoundBlaster()
{
  if (m_output_channel)
    m_system->GetHostInterface()->GetAudioMixer()->RemoveChannel(m_output_channel);
}

void SoundBlaster::Initialize(System* system)
{
  m_system = system;
  m_clock.SetManager(system->GetTimingManager());

  m_output_channel = m_system->GetHostInterface()->GetAudioMixer()->CreateChannel(
    "Sound Blaster", 44100, Audio::SampleFormat::Signed16, IsStereo() ? 2 : 1);
  if (!m_output_channel)
    Panic("Failed to create Sound blaster output channel");

  m_dac_sample_event =
    m_clock.NewEvent("DAC Sample", 1, std::bind(&SoundBlaster::DACSampleEvent, this, std::placeholders::_2), false);

  // DMA channel connections
  m_dma_controller->ConnectDMAChannel(m_dma_channel,
                                      std::bind(&SoundBlaster::DMAReadCallback, this, std::placeholders::_1,
                                                std::placeholders::_2, std::placeholders::_3, false),
                                      std::bind(&SoundBlaster::DMAWriteCallback, this, std::placeholders::_1,
                                                std::placeholders::_2, std::placeholders::_3, false));
  if (Has16BitDMA())
  {
    m_dma_controller->ConnectDMAChannel(m_dma_channel_16,
                                        std::bind(&SoundBlaster::DMAReadCallback, this, std::placeholders::_1,
                                                  std::placeholders::_2, std::placeholders::_3, true),
                                        std::bind(&SoundBlaster::DMAWriteCallback, this, std::placeholders::_1,
                                                  std::placeholders::_2, std::placeholders::_3, true));
  }
}

bool SoundBlaster::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  ResetDSP(false);

  return !reader.GetErrorState();
}

bool SoundBlaster::SaveState(BinaryWriter& writer) const
{
  writer.WriteUInt32(SERIALIZATION_ID);

  return !writer.InErrorState();
}

} // namespace HW