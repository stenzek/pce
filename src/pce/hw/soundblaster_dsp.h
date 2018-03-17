#pragma once

#include "pce/audio.h"
#include "pce/bitfield.h"
#include "pce/clock.h"
#include "pce/component.h"
#include "pce/system.h"
#include <array>
#include <deque>

class DMAController;

namespace HW {

class SoundBlasterDSP final
{
public:
public:
  SoundBlasterDSP(DMAController* dma_controller, uint32 version, uint32 irq, uint32 dma, uint32 dma16);
  ~SoundBlasterDSP();

  void Initialize(System* system);

  bool LoadState(BinaryReader& reader);
  bool SaveState(BinaryWriter& writer) const;

private:
  static constexpr uint32 SERIALIZATION_ID = Component::MakeSerializationID('S', 'D', 'S', 'P');

  Clock m_clock;
  System* m_system = nullptr;
  Audio::Channel* m_output_channel = nullptr;
  TimingEvent::Pointer m_dac_sample_event;

  DMAController* m_dma_controller;
  uint32 m_version;
  uint32 m_irq;
  uint32 m_dma_channel;
  uint32 m_dma_channel_16;

  bool m_reset = true;
  bool m_interrupt_pending = false;

  uint8 m_test_register = 0;

  std::deque<uint8> m_input_buffer;
  std::deque<uint8> m_output_buffer;

  size_t GetInputBufferLength() const { return m_input_buffer.size(); }
  void ClearInputBuffer() { m_input_buffer.clear(); }

  uint8 GetCommand() const
  {
    DebugAssert(m_input_buffer.size() > 0);
    return m_input_buffer[0];
  }
  uint8 GetCommandParameterByte() const
  {
    DebugAssert(m_input_buffer.size() > 1);
    return m_input_buffer[1];
  }
  uint16 GetCommandParameterWord() const
  {
    DebugAssert(m_input_buffer.size() > 2);
    return ZeroExtend16(m_input_buffer[1]) | (ZeroExtend16(m_input_buffer[2]) << 8);
  }

  void ClearOutputBuffer();
  void WriteOutputBuffer(uint8 value);
  void ClearAndWriteOutputBuffer(uint8 value);

  struct DACState
  {
    bool enable_speaker = false;
    float frequency = 1000000.0f;
    uint32 silence_samples = 0;
    uint32 dma_block_size = 0;
    bool dma_paused = false;
    bool dma_active = false;
    bool dma_16 = false;
    bool fifo_enable = false;
    bool stereo = false;

    // FIFO for DSP, contains samples in the format that they are submitted
    // One fifo per channel makes it easier to pull samples out of this buffer (especially ADPCM)
    uint8 dma_fifo_index = 0;
    std::array<std::deque<uint8>, 2> fifos;
    std::array<int16, 2> last_sample = {};
    DSP_SAMPLE_FORMAT sample_format;

    // Subsample for ADPCM samples
    uint32 adpcm_subsample = 0;

    // Reference byte for ADPCM samples
    int32 adpcm_scale = 0;
    uint8 adpcm_reference = 0;
    bool adpcm_reference_update_pending = 0;
  };
  DACState m_dac_state;

  void HandleDSPCommand();
  void DACSampleEvent(CycleCount cycles);

  void UpdateDACSampleEventState();
  int16 DecodeDACOutputSample(uint8 channel);
  bool IsDACFIFOFull() const;
  void StartDACDMA(bool dma16, DSP_SAMPLE_FORMAT format, bool stereo, bool update_reference_byte, bool autoinit,
                   bool fifo_enable, uint32 length_in_bytes);
  void StopDACDMA();

  struct ADCState
  {
    // float frequency = 1000000.0f;
    int32 e2_value = 0xAA;
    uint32 e2_count = 0;
    int16 last_sample = 0;
    bool dma_paused = false;
    bool dma_active = false;
    bool dma_16 = false;

    std::deque<uint8> fifo;
  };
  ADCState m_adc_state;

  void StartADCDMA(bool dma16, uint32 length_in_bytes);
  void StopADCDMA();

  struct DMAState
  {
    uint32 length = 0;
    uint32 remaining_bytes = 0;
    bool dma_to_host = false;
    bool autoinit = false;
    bool active = false;
    bool request = false;
  };
  DMAState m_dma_state;
  DMAState m_dma_16_state;

  bool IsDMAActive(bool dma16) const;
  void StartDMA(bool dma16, bool dma_to_host, bool autoinit, uint32 length, bool request = true);
  void SetDMARequest(bool dma16, bool request);
  void StopDMA(bool dma16);

  void DMAReadCallback(IOPortDataSize size, uint32* value, uint32 remaining_bytes, bool is_16_bit);
  void DMAWriteCallback(IOPortDataSize size, uint32 value, uint32 remaining_bytes, bool is_16_bit);
};

} // namespace HW