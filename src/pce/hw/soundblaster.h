#pragma once

#include "../component.h"
#include "common/audio.h"
#include "common/bitfield.h"
#include "ymf262.h"
#include <array>
#include <deque>

class InterruptController;
class DMAController;
class TimingEvent;

namespace HW {

class SoundBlaster final : public Component
{
  DECLARE_OBJECT_TYPE_INFO(SoundBlaster, Component);
  DECLARE_GENERIC_COMPONENT_FACTORY(SoundBlaster);
  DECLARE_OBJECT_PROPERTY_MAP(SoundBlaster);

public:
  enum class Type : u32
  {
    SoundBlaster10,
    SoundBlaster20,
    SoundBlasterPro,
    SoundBlasterPro2,
    SoundBlaster16
  };

  SoundBlaster(const String& identifier, Type type = Type::SoundBlaster10, u32 iobase = 0x220, u32 irq = 7, u32 dma = 1,
               u32 dma16 = 5, const ObjectTypeInfo* type_info = &s_type_info);
  ~SoundBlaster();

  bool Initialize(System* system, Bus* bus) override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;
  void Reset() override;

private:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('C', 'L', 'S', 'B');
  static constexpr Audio::SampleFormat DSP_OUTPUT_FORMAT = Audio::SampleFormat::Signed16;

  enum : u32
  {
    DSP_VERSION_SB10 = 0x0105,
    DSP_VERSION_SB20 = 0x0202,
    DSP_VERSION_SBPRO = 0x0300,
    DSP_VERSION_SBPRO2 = 0x0302,
    DSP_VERSION_SB16 = 0x0404
  };

  enum DSP_COMMAND : u8
  {
    DSP_COMMAND_IDENTIFY = 0xE0,
    DSP_COMMAND_VERSION = 0xE1,
    DSP_COMMAND_PAUSE_DMA = 0xD0,
    DSP_COMMAND_RESUME_DMA = 0xD4,
    DSP_COMMAND_SET_TIME_CONSTANT = 0x40,
    DSP_COMMAND_DIRECT_SAMPLE = 0x10,
    DSP_COMMAND_DMA_8_BIT_PCM_OUTPUT = 0x14,
    DSP_COMMAND_DMA_8_BIT_PCM_INPUT = 0x24,
    DSP_COMMAND_DMA_4_BIT_ADPCM_OUTPUT = 0x74,
    DSP_COMMAND_DMA_4_BIT_ADPCM_OUTPUT_WITH_REF = 0x75,
    DSP_COMMAND_DMA_3_BIT_ADPCM_OUTPUT = 0x76,
    DSP_COMMAND_DMA_3_BIT_ADPCM_OUTPUT_WITH_REF = 0x77,
    DSP_COMMAND_DMA_2_BIT_ADPCM_OUTPUT = 0x16,
    DSP_COMMAND_DMA_2_BIT_ADPCM_OUTPUT_WITH_REF = 0x17,
    DSP_COMMAND_SILENCE = 0x80,
    DSP_COMMAND_ENABLE_SPEAKER = 0xD1,
    DSP_COMMAND_DISABLE_SPEAKER = 0xD3,
    DSP_COMMAND_INTERRUPT_REQUEST = 0xF2,
    DSP_COMMAND_READ_TEST_REGISTER = 0xE8,
    DSP_COMMAND_WRITE_TEST_REGISTER = 0xE4,
    DSP_COMMAND_MPU_401_RESET = 0xFF,
    DSP_COMMAND_UNKNOWN_E2 = 0xE2,
    DSP_COMMAND_UNKNOWN_E3 = 0xE3,
    DSP_COMMAND_UNKNOWN_F8 = 0xF8,

    // DSP2.0+ only
    DSP_COMMAND_DMA_8_BIT_PCM_OUTPUT_AUTOINIT = 0x1C,
    DSP_COMMAND_DMA_8_BIT_PCM_INPUT_AUTOINIT = 0x2C,
    DSP_COMMAND_DMA_4_BIT_ADPCM_OUTPUT_AUTOINIT = 0x7D,
    DSP_COMMAND_DMA_3_BIT_ADPCM_OUTPUT_AUTOINIT = 0x7E,
    DSP_COMMAND_DMA_2_BIT_ADPCM_OUTPUT_AUTOINIT = 0x1F,
    DSP_COMMAND_STOP_AUTOINIT_DMA = 0xDA,
    DSP_COMMAND_DMA_BLOCK_TRANSFER_SIZE = 0x48,
    DSP_COMMAND_SPEAKER_STATUS = 0xD8,

    // DSP2.1+ only
    DSP_COMMAND_HIGH_SPEED_DMA_8_BIT_PCM_OUTPUT = 0x90,
    DSP_COMMAND_HIGH_SPEED_DMA_8_BIT_PCM_INPUT = 0x98,
    DSP_COMMAND_HIGH_SPEED_DMA_8_BIT_PCM_OUTPUT_AUTOINIT = 0x91,
    DSP_COMMAND_HIGH_SPEED_DMA_8_BIT_PCM_INPUT_AUTOINIT = 0x99,

    // DSP3.0+ only
    DSP_COMMAND_SET_INPUT_MONO = 0xA0,
    DSP_COMMAND_SET_INPUT_STEREO = 0xA8,
    DSP_COMMAND_SET_OUTPUT_SAMPLE_RATE = 0x41,
    DSP_COMMAND_SET_INPUT_SAMPLE_RATE = 0x42,

    // DSP4.0+ only
    DSP_COMMAND_PROGRAM_DMA_8 = 0xC0,  // 0xB0-0xBF
    DSP_COMMAND_PROGRAM_DMA_16 = 0xB0, // 0xC0-0xCF
    DSP_COMMAND_PAUSE_DMA_16 = 0xD5,
    DSP_COMMAND_CONTINUE_DMA_16 = 0xD6,
    DSP_COMMAND_STOP_AUTOINIT_DMA_16 = 0xD6,
    DSP_COMMAND_INTERRUPT_REQUEST_16 = 0xF3,
  };

  enum DSP_SAMPLE_FORMAT : u32
  {
    DSP_SAMPLE_FORMAT_U8_PCM,
    DSP_SAMPLE_FORMAT_S8_PCM,
    DSP_SAMPLE_FORMAT_U4_ADPCM,
    DSP_SAMPLE_FORMAT_U3_ADPCM,
    DSP_SAMPLE_FORMAT_U2_ADPCM,
    DSP_SAMPLE_FORMAT_S16_PCM,
    DSP_SAMPLE_FORMAT_U16_PCM
  };

  bool IsStereo() const { return (m_dsp_version >= DSP_VERSION_SBPRO); }
  bool Has16BitDMA() const { return (m_dsp_version >= DSP_VERSION_SB16); }

  u8 IOPortRead(u16 port);
  void IOPortWrite(u16 port, u8 value);

  void RaiseInterrupt(bool is_16_bit);
  void LowerInterrupt(bool is_16_bit);

  InterruptController* m_interrupt_controller = nullptr;
  DMAController* m_dma_controller = nullptr;
  Type m_type;
  u32 m_io_base;
  u32 m_irq;
  u32 m_dma_channel;
  u32 m_dma_channel_16;
  bool m_interrupt_pending = false;
  bool m_interrupt_pending_16 = false;

  // Yamaha FM Synth
  YMF262 m_ymf262;

  //////////////////////////////////////////////////////////////////////////
  // DSP
  //////////////////////////////////////////////////////////////////////////
  u32 m_dsp_version = 0;

  std::deque<u8> m_dsp_input_buffer;
  std::deque<u8> m_dsp_output_buffer;

  bool m_dsp_reset = true;
  u8 m_dsp_test_register = 0;

  static u32 GetDSPVersion(Type type);
  static YMF262::Mode GetOPLMode(Type type);
  static size_t GetBytesPerSample(DSP_SAMPLE_FORMAT format);
  static u32 GetSamplesPerDMATransfer(DSP_SAMPLE_FORMAT format);

  void ResetDSP(bool soft_reset);
  void UpdateDSPAudioOutput();

  u8 ReadDSPDataPort();
  u8 ReadDSPDataWriteStatusPort();
  u8 ReadDSPDataAvailableStatusPort();
  void WriteDSPResetPort(u8 value);
  void WriteDSPCommandDataPort(u8 value);

  size_t GetDSPInputBufferLength() const { return m_dsp_input_buffer.size(); }
  void ClearDSPInputBuffer() { m_dsp_input_buffer.clear(); }

  u8 GetDSPCommand() const
  {
    DebugAssert(m_dsp_input_buffer.size() > 0);
    return m_dsp_input_buffer[0];
  }
  u8 GetDSPCommandParameterByte() const
  {
    DebugAssert(m_dsp_input_buffer.size() > 1);
    return m_dsp_input_buffer[1];
  }
  u16 GetDSPCommandParameterWord() const
  {
    DebugAssert(m_dsp_input_buffer.size() > 2);
    return ZeroExtend16(m_dsp_input_buffer[1]) | (ZeroExtend16(m_dsp_input_buffer[2]) << 8);
  }

  void ClearDSPOutputBuffer();
  void WriteDSPOutputBuffer(u8 value);
  void ClearAndWriteDSPOutputBuffer(u8 value);

  struct DACState
  {
    Audio::Channel* output_channel = nullptr;
    std::unique_ptr<TimingEvent> sample_event;

    bool enable_speaker = false;
    float frequency = 1000000.0f;
    u32 silence_samples = 0;
    u32 dma_block_size = 0;
    bool dma_paused = false;
    bool dma_active = false;
    bool dma_16 = false;
    bool fifo_enable = false;
    bool stereo = false;

    // FIFO for DSP, contains samples in the format that they are submitted
    std::deque<u8> fifo;
    std::array<s16, 2> last_sample = {};
    DSP_SAMPLE_FORMAT sample_format;

    // Subsample for ADPCM samples
    u32 adpcm_subsample = 0;

    // Reference byte for ADPCM samples
    s32 adpcm_scale = 0;
    u8 adpcm_reference = 0;
    bool adpcm_reference_update_pending = 0;
  };
  DACState m_dac_state;

  void HandleDSPCommand();
  void DACSampleEvent(CycleCount cycles);
  void UpdateDACSampleEventState();
  void SetDACSampleRate(float frequency);

  s16 DecodeDACOutputSample(s16 last_sample);
  bool IsDACFIFOFull() const;
  void StartDACDMA(bool dma16, DSP_SAMPLE_FORMAT format, bool stereo, bool update_reference_byte, bool autoinit,
                   bool fifo_enable, u32 length_in_bytes);
  void StopDACDMA();

  struct ADCState
  {
    // float frequency = 1000000.0f;
    s32 e2_value = 0xAA;
    u32 e2_count = 0;
    s16 last_sample = 0;
    bool dma_paused = false;
    bool dma_active = false;
    bool dma_16 = false;

    std::deque<u8> fifo;
  };
  ADCState m_adc_state;

  void StartADCDMA(bool dma16, u32 length_in_bytes);
  void StopADCDMA();

  struct DMAState
  {
    u32 length = 0;
    u32 remaining_bytes = 0;
    bool dma_to_host = false;
    bool autoinit = false;
    bool active = false;
    bool request = false;
  };
  DMAState m_dma_state;
  DMAState m_dma_16_state;

  bool IsDMAActive(bool dma16) const;
  void StartDMA(bool dma16, bool dma_to_host, bool autoinit, u32 length, bool request = true);
  void SetDMARequest(bool dma16, bool request);
  void StopDMA(bool dma16);

  void DMAReadCallback(IOPortDataSize size, u32* value, u32 remaining_bytes, bool is_16_bit);
  void DMAWriteCallback(IOPortDataSize size, u32 value, u32 remaining_bytes, bool is_16_bit);

  struct MixerState
  {
    std::array<float, 2> master_volume;
    std::array<float, 2> voice_volume;
  };
  MixerState m_mixer_state = {};
  u8 m_mixer_index_register = 0;

  u8 ReadMixerIndexPort();
  u8 ReadMixerDataPort();
  void WriteMixerIndexPort(u8 value);
  void WriteMixerDataPort(u8 value);

  u8 ReadMixerDataPortCT1335();            // SB2.0
  u8 ReadMixerDataPortCT1345();            // SBPro
  u8 ReadMixerDataPortCT1745();            // SB16
  void WriteMixerDataPortCT1335(u8 value); // SB2.0
  void WriteMixerDataPortCT1345(u8 value); // SBPro
  void WriteMixerDataPortCT1745(u8 value); // SB16

  void ResetMixer();
};

} // namespace HW