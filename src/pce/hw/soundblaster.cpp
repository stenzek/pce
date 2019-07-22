#include "pce/hw/soundblaster.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/dma_controller.h"
#include "pce/host_interface.h"
#include "pce/hw/soundblaster_adpcm.inl"
#include "pce/interrupt_controller.h"
Log_SetChannel(HW::SoundBlaster);

// https://courses.engr.illinois.edu/ece390/resources/sound/sbdsp.txt.html
// http://flint.cs.yale.edu/cs422/readings/hardware/SoundBlaster.pdf

namespace HW {
DEFINE_OBJECT_TYPE_INFO(SoundBlaster);
DEFINE_GENERIC_COMPONENT_FACTORY(SoundBlaster);
BEGIN_OBJECT_PROPERTY_MAP(SoundBlaster)
PROPERTY_TABLE_MEMBER_UINT("IOBase", 0, offsetof(SoundBlaster, m_io_base), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("IRQ", 0, offsetof(SoundBlaster, m_irq), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("DMA", 0, offsetof(SoundBlaster, m_dma_channel), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Board", 0, offsetof(SoundBlaster, m_type), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

u32 SoundBlaster::GetDSPVersion(Type type)
{
  switch (type)
  {
    case Type::SoundBlaster10:
      return DSP_VERSION_SB10;
    case Type::SoundBlaster20:
      return DSP_VERSION_SB20;
    case Type::SoundBlasterPro:
      return DSP_VERSION_SBPRO;
    case Type::SoundBlasterPro2:
      return DSP_VERSION_SBPRO2;
    case Type::SoundBlaster16:
      return DSP_VERSION_SB16;
    default:
      return DSP_VERSION_SB10;
  }
}

YMF262::Mode SoundBlaster::GetOPLMode(Type type)
{
  switch (type)
  {
    case Type::SoundBlaster10:
      return YMF262::Mode::OPL2;
    case Type::SoundBlaster20:
      return YMF262::Mode::OPL2;
    case Type::SoundBlasterPro:
      return YMF262::Mode::DualOPL2;
    case Type::SoundBlasterPro2:
      return YMF262::Mode::OPL3;
    case Type::SoundBlaster16:
      return YMF262::Mode::OPL3;
    default:
      return YMF262::Mode::OPL2;
  }
}

SoundBlaster::SoundBlaster(const String& identifier, Type type /* = Type::SoundBlaster10 */, u32 iobase /* = 0x220 */,
                           u32 irq /* = 7 */, u32 dma /* = 1 */, u32 dma16 /* = 5 */,
                           const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_type(type), m_io_base(iobase), m_irq(irq), m_dma_channel(dma),
    m_dma_channel_16(dma16), m_ymf262(GetOPLMode(type)), m_dsp_version(GetDSPVersion(type))
{
}

SoundBlaster::~SoundBlaster() {}

bool SoundBlaster::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus) || !m_ymf262.Initialize(system))
    return false;

  if (m_type < Type::SoundBlaster10 || m_type > Type::SoundBlaster16)
  {
    Log_ErrorPrintf("Invalid board type");
    return false;
  }

  m_interrupt_controller = system->GetComponentByType<InterruptController>();
  if (!m_interrupt_controller)
  {
    Log_ErrorPrintf("Failed to locate interrupt controller");
    return false;
  }

  // IO port connections
  // Present in all sound blaster models
  for (u32 port_offset : {0x06, 0x08, 0x09, 0x0A, 0x0C, 0x0E, 0x0F})
    bus->ConnectIOPortRead(Truncate16(m_io_base + port_offset), this,
                           std::bind(&SoundBlaster::IOPortRead, this, std::placeholders::_1, std::placeholders::_2));
  for (u32 port_offset : {0x06, 0x08, 0x09, 0x0C})
    bus->ConnectIOPortWrite(Truncate16(m_io_base + port_offset), this,
                            std::bind(&SoundBlaster::IOPortWrite, this, std::placeholders::_1, std::placeholders::_2));

  // Mixer ports are in SB2.0+
  if (m_type >= Type::SoundBlasterPro)
  {
    for (u32 port_offset : {0x04, 0x05})
      bus->ConnectIOPortRead(Truncate16(m_io_base + port_offset), this,
                             std::bind(&SoundBlaster::IOPortRead, this, std::placeholders::_1, std::placeholders::_2));
    for (u32 port_offset : {0x04, 0x05})
      bus->ConnectIOPortWrite(
        Truncate16(m_io_base + port_offset), this,
        std::bind(&SoundBlaster::IOPortWrite, this, std::placeholders::_1, std::placeholders::_2));
  }
  // Stereo FM ports on SBPro+
  if (m_type >= Type::SoundBlasterPro)
  {
    for (u32 port_offset : {0x00, 0x01, 0x02, 0x03})
      bus->ConnectIOPortRead(Truncate16(m_io_base + port_offset), this,
                             std::bind(&SoundBlaster::IOPortRead, this, std::placeholders::_1, std::placeholders::_2));
    for (u32 port_offset : {0x00, 0x01, 0x02, 0x03})
      bus->ConnectIOPortWrite(
        Truncate16(m_io_base + port_offset), this,
        std::bind(&SoundBlaster::IOPortWrite, this, std::placeholders::_1, std::placeholders::_2));
  }

  // Also connect up the adlib ports at the hardwired locations
  bus->ConnectIOPortRead(0x0388, this, [this](u16, u8* value) { *value = m_ymf262.ReadAddressPort(0); });
  bus->ConnectIOPortRead(0x0389, this, [this](u16, u8* value) { *value = m_ymf262.ReadDataPort(0); });
  bus->ConnectIOPortWrite(0x0388, this, [this](u16, u8 value) { m_ymf262.WriteAddressPort(0, value); });
  bus->ConnectIOPortWrite(0x0389, this, [this](u16, u8 value) { m_ymf262.WriteDataPort(0, value); });

  // DSP output channel
  m_dac_state.output_channel = m_system->GetHostInterface()->GetAudioMixer()->CreateChannel(
    "Sound Blaster DSP", 44100, Audio::SampleFormat::Signed16, IsStereo() ? 2 : 1);
  if (!m_dac_state.output_channel)
    Panic("Failed to create Sound blaster output channel");

  m_dac_state.sample_event =
    m_system->CreateClockedEvent("Sound Blaster DSP Render", 44100.0f, 1,
                                 std::bind(&SoundBlaster::DACSampleEvent, this, std::placeholders::_2), false);

  // DMA channel connections
  m_dma_controller = system->GetComponentByType<DMAController>();
  if (!m_dma_controller)
  {
    Log_ErrorPrintf("Failed to find DMA controller.");
    return false;
  }
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

  return true;
}

void SoundBlaster::Reset()
{
  m_dac_state.output_channel->ClearBuffer();
  m_ymf262.Reset();
  ResetDSP(false);
  ResetMixer();
}

bool SoundBlaster::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  if (!m_ymf262.LoadState(reader))
    return false;

  m_interrupt_pending = reader.ReadBool();
  m_interrupt_pending_16 = reader.ReadBool();

  u32 size = reader.ReadUInt32();
  m_dsp_input_buffer.clear();
  for (u32 i = 0; i < size; i++)
    m_dsp_input_buffer.push_back(reader.ReadUInt8());

  size = reader.ReadUInt32();
  m_dsp_output_buffer.clear();
  for (u32 i = 0; i < size; i++)
    m_dsp_output_buffer.push_back(reader.ReadUInt8());

  m_dsp_reset = reader.ReadBool();
  m_dsp_test_register = reader.ReadUInt8();

  m_dac_state.enable_speaker = reader.ReadBool();
  m_dac_state.frequency = reader.ReadFloat();
  m_dac_state.silence_samples = reader.ReadUInt32();
  m_dac_state.dma_block_size = reader.ReadUInt32();
  m_dac_state.dma_paused = reader.ReadBool();
  m_dac_state.dma_active = reader.ReadBool();
  m_dac_state.dma_16 = reader.ReadBool();
  m_dac_state.fifo_enable = reader.ReadBool();
  m_dac_state.stereo = reader.ReadBool();
  size = reader.ReadUInt32();
  m_dac_state.fifo.clear();
  for (u32 i = 0; i < size; i++)
    m_dac_state.fifo.push_back(reader.ReadUInt8());
  for (size_t i = 0; i < m_dac_state.last_sample.size(); i++)
    m_dac_state.last_sample[i] = reader.ReadInt16();
  m_dac_state.sample_format = static_cast<DSP_SAMPLE_FORMAT>(reader.ReadUInt8());
  m_dac_state.adpcm_subsample = reader.ReadUInt32();
  m_dac_state.adpcm_scale = reader.ReadInt32();
  m_dac_state.adpcm_reference = reader.ReadUInt8();
  m_dac_state.adpcm_reference_update_pending = reader.ReadBool();

  m_adc_state.e2_value = reader.ReadInt32();
  m_adc_state.e2_count = reader.ReadUInt32();
  m_adc_state.last_sample = reader.ReadInt16();
  m_adc_state.dma_paused = reader.ReadBool();
  m_adc_state.dma_active = reader.ReadBool();
  m_adc_state.dma_16 = reader.ReadBool();
  size = reader.ReadUInt32();
  m_adc_state.fifo.clear();
  for (u32 i = 0; i < size; i++)
    m_adc_state.fifo.push_back(reader.ReadUInt8());

  for (DMAState* dma_state : {&m_dma_state, &m_dma_16_state})
  {
    dma_state->length = reader.ReadUInt32();
    dma_state->remaining_bytes = reader.ReadUInt32();
    dma_state->dma_to_host = reader.ReadBool();
    dma_state->autoinit = reader.ReadBool();
    dma_state->active = reader.ReadBool();
    dma_state->request = reader.ReadBool();
  }

  for (size_t i = 0; i < m_mixer_state.master_volume.size(); i++)
    m_mixer_state.master_volume[i] = reader.ReadFloat();
  for (size_t i = 0; i < m_mixer_state.voice_volume.size(); i++)
    m_mixer_state.voice_volume[i] = reader.ReadFloat();
  m_mixer_index_register = reader.ReadUInt8();

  if (reader.GetErrorState())
    return false;

  m_dac_state.output_channel->ClearBuffer();
  m_dac_state.output_channel->ChangeSampleRate(m_dac_state.frequency);
  UpdateDACSampleEventState();
  return true;
}

bool SoundBlaster::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  if (!m_ymf262.SaveState(writer))
    return false;

  writer.WriteBool(m_interrupt_pending);
  writer.WriteBool(m_interrupt_pending_16);

  writer.WriteUInt32(static_cast<u32>(m_dsp_input_buffer.size()));
  for (size_t i = 0; i < m_dsp_input_buffer.size(); i++)
    writer.WriteByte(m_dsp_input_buffer[i]);

  writer.WriteUInt32(static_cast<u32>(m_dsp_output_buffer.size()));
  for (size_t i = 0; i < m_dsp_output_buffer.size(); i++)
    writer.WriteByte(m_dsp_output_buffer[i]);

  writer.WriteBool(m_dsp_reset);
  writer.WriteUInt8(m_dsp_test_register);

  writer.WriteBool(m_dac_state.enable_speaker);
  writer.WriteFloat(m_dac_state.frequency);
  writer.WriteUInt32(m_dac_state.silence_samples);
  writer.WriteUInt32(m_dac_state.dma_block_size);
  writer.WriteBool(m_dac_state.dma_paused);
  writer.WriteBool(m_dac_state.dma_active);
  writer.WriteBool(m_dac_state.dma_16);
  writer.WriteBool(m_dac_state.fifo_enable);
  writer.WriteBool(m_dac_state.stereo);
  writer.WriteUInt32(static_cast<u32>(m_dac_state.fifo.size()));
  for (size_t i = 0; i < m_dac_state.fifo.size(); i++)
    writer.WriteByte(m_dac_state.fifo[i]);
  for (size_t i = 0; i < m_dac_state.last_sample.size(); i++)
    writer.WriteInt16(m_dac_state.last_sample[i]);
  writer.WriteUInt8(static_cast<u8>(m_dac_state.sample_format));
  writer.WriteUInt32(m_dac_state.adpcm_subsample);
  writer.WriteInt32(m_dac_state.adpcm_scale);
  writer.WriteUInt8(m_dac_state.adpcm_reference);
  writer.WriteBool(m_dac_state.adpcm_reference_update_pending);

  writer.WriteInt32(m_adc_state.e2_value);
  writer.WriteUInt32(m_adc_state.e2_count);
  writer.WriteInt16(m_adc_state.last_sample);
  writer.WriteBool(m_adc_state.dma_paused);
  writer.WriteBool(m_adc_state.dma_active);
  writer.WriteBool(m_adc_state.dma_16);
  writer.WriteUInt32(static_cast<u32>(m_adc_state.fifo.size()));
  for (size_t i = 0; i < m_adc_state.fifo.size(); i++)
    writer.WriteByte(m_adc_state.fifo[i]);

  for (const DMAState* dma_state : {&m_dma_state, &m_dma_16_state})
  {
    writer.WriteUInt32(dma_state->length);
    writer.WriteUInt32(dma_state->remaining_bytes);
    writer.WriteBool(dma_state->dma_to_host);
    writer.WriteBool(dma_state->autoinit);
    writer.WriteBool(dma_state->active);
    writer.WriteBool(dma_state->request);
  }

  for (size_t i = 0; i < m_mixer_state.master_volume.size(); i++)
    writer.WriteFloat(m_mixer_state.master_volume[i]);
  for (size_t i = 0; i < m_mixer_state.voice_volume.size(); i++)
    writer.WriteFloat(m_mixer_state.voice_volume[i]);
  writer.WriteUInt8(m_mixer_index_register);

  return !writer.InErrorState();
}

void SoundBlaster::IOPortRead(u16 port, u8* value)
{
  switch (port & 0x0F)
  {
      // mixer index
    case 0x04:
      *value = ReadMixerIndexPort();
      break;

      // mixer data
    case 0x05:
      *value = ReadMixerDataPort();
      break;

      // reset
    case 0x06:
      *value = 0xFF;
      break;

      // Adlib status
    case 0x08:
      *value = m_ymf262.ReadAddressPort(0);
      break;

      // DSP read data
    case 0x0A:
      *value = ReadDSPDataPort();
      break;

      // DSP write data status, 0xff - not ready to write, 0x7f - ready to write
    case 0x0C:
      *value = ReadDSPDataWriteStatusPort();
      break;

      // DSP data available status, and lower IRQ. 0xff - has data, 0x7f - no data
    case 0x0E:
      *value = ReadDSPDataAvailableStatusPort();
      break;

      // ACK 16-bit interrupt
    case 0x0F:
      LowerInterrupt(true);
      *value = 0xFF;
      break;

    default:
      Log_ErrorPrintf("Unknown read port offset: 0x%X", port & 0x0F);
      *value = 0xFF;
      break;
  }
}

void SoundBlaster::IOPortWrite(u16 port, u8 value)
{
  switch (port & 0x0F)
  {
      // mixer index
    case 0x04:
      WriteMixerIndexPort(value);
      break;

      // mixer data
    case 0x05:
      WriteMixerDataPort(value);
      break;

      // reset state
    case 0x06:
      WriteDSPResetPort(value);
      break;

      // adlib address port
    case 0x08:
      Log_DebugPrintf("Adlib address port: 0x%02X", ZeroExtend32(value));
      m_ymf262.WriteAddressPort(0, value);
      break;

      // adlib data port
    case 0x09:
      Log_DebugPrintf("Adlib data port: 0x%02X", ZeroExtend32(value));
      m_ymf262.WriteDataPort(0, value);
      break;

      // DSP data/command
    case 0x0C:
      WriteDSPCommandDataPort(value);
      break;

    default:
      Log_ErrorPrintf("Unknown write port offset: 0x%X : 0x%02X", port & 0x0F, ZeroExtend32(value));
      break;
  }
}

void SoundBlaster::RaiseInterrupt(bool is_16_bit)
{
  // TODO: Is this going to be an issue if both get raised?
  bool& interrupt_flag = is_16_bit ? m_interrupt_pending_16 : m_interrupt_pending;
  if (!interrupt_flag)
  {
    m_interrupt_controller->RaiseInterrupt(m_irq);
    interrupt_flag = true;
  }
}

void SoundBlaster::LowerInterrupt(bool is_16_bit)
{
  bool& interrupt_flag = is_16_bit ? m_interrupt_pending_16 : m_interrupt_pending;
  if (interrupt_flag)
  {
    m_interrupt_controller->LowerInterrupt(m_irq);
    interrupt_flag = false;
  }
}

u8 SoundBlaster::ReadDSPDataPort()
{
  u8 value;

  if (!m_dsp_output_buffer.empty())
  {
    value = m_dsp_output_buffer.front();
    m_dsp_output_buffer.pop_front();
    Log_DebugPrintf("Read DSP output byte: 0x%02X", ZeroExtend32(value));
  }
  else
  {
    value = 0;
    Log_WarningPrintf("DSP output buffer is empty on read");
  }

  return value;
}

u8 SoundBlaster::ReadDSPDataWriteStatusPort()
{
  // DSP write data status, 0xff - not ready to write, 0x7f - ready to write
  return 0x7F;
}

u8 SoundBlaster::ReadDSPDataAvailableStatusPort()
{
  // DSP data available status, and lower IRQ. 0xff - has data, 0x7f - no data
  u8 value = (!m_dsp_output_buffer.empty()) ? 0xFF : 0x7F;
  LowerInterrupt(false);
  return value;
}

void SoundBlaster::WriteDSPResetPort(u8 value)
{
  UpdateDSPAudioOutput();

  bool is_reset = (value & 1) != 0;
  bool do_soft_reset = m_dsp_reset && !is_reset;
  Log_DevPrintf("Reset: %s -> %s", m_dsp_reset ? "yes" : "no", is_reset ? "yes" : "no");
  m_dsp_reset = is_reset;
  if (do_soft_reset)
    ResetDSP(true);
}

void SoundBlaster::WriteDSPCommandDataPort(u8 value)
{
  Log_DebugPrintf("DSP command/data: 0x%02X", ZeroExtend32(value));
  m_dsp_input_buffer.push_back(value);
  HandleDSPCommand();
}

void SoundBlaster::ResetDSP(bool soft_reset)
{
  ClearDSPInputBuffer();
  ClearDSPOutputBuffer();
  if (m_interrupt_pending || m_interrupt_pending_16)
    m_interrupt_controller->LowerInterrupt(m_irq);
  m_interrupt_pending = false;
  m_interrupt_pending_16 = false;
  m_dsp_test_register = 0;

  // DAC state
  m_dac_state.enable_speaker = true;
  m_dac_state.frequency = 1000000.0f;
  m_dac_state.silence_samples = 0;
  m_dac_state.last_sample.fill(0);
  m_dac_state.dma_active = false;
  m_dac_state.dma_paused = false;
  m_dac_state.dma_16 = false;
  m_dac_state.fifo.clear();

  // ADC state
  m_adc_state.e2_value = 0xAA;
  m_adc_state.e2_count = 0;
  m_adc_state.last_sample = 0;
  m_adc_state.dma_active = false;
  m_adc_state.dma_paused = false;
  m_adc_state.dma_16 = false;
  m_adc_state.fifo.clear();

  // Clear DMA state
  m_dma_state.length = 0;
  m_dma_state.remaining_bytes = 0;
  m_dma_state.dma_to_host = false;
  m_dma_state.active = false;
  m_dma_state.request = false;
  m_dma_state.autoinit = false;
  m_dma_controller->SetDMAState(m_dma_channel, false);
  if (Has16BitDMA())
  {
    m_dma_16_state.length = 0;
    m_dma_16_state.remaining_bytes = 0;
    m_dma_16_state.dma_to_host = false;
    m_dma_16_state.active = false;
    m_dma_16_state.request = false;
    m_dma_16_state.autoinit = false;
    m_dma_controller->SetDMAState(m_dma_channel, false);
  }

  UpdateDACSampleEventState();

  // Start with reset high.
  if (!soft_reset)
    m_dsp_reset = true;
  else
    WriteDSPOutputBuffer(0xAA);
}

void SoundBlaster::UpdateDSPAudioOutput()
{
  m_dac_state.sample_event->InvokeEarly();
}

void SoundBlaster::ClearDSPOutputBuffer()
{
  m_dsp_output_buffer.clear();
}

void SoundBlaster::WriteDSPOutputBuffer(u8 value)
{
  m_dsp_output_buffer.push_back(value);
}

void SoundBlaster::ClearAndWriteDSPOutputBuffer(u8 value)
{
  ClearDSPOutputBuffer();
  WriteDSPOutputBuffer(value);
}

void SoundBlaster::HandleDSPCommand()
{
  u8 command = GetDSPCommand();
  bool has_byte_param = (GetDSPInputBufferLength() > 1);
  bool has_word_param = (GetDSPInputBufferLength() > 2);
  if (has_word_param)
    Log_TracePrintf("DSP command %02X %04X", ZeroExtend32(GetDSPCommand()), ZeroExtend32(GetDSPCommandParameterWord()));
  if (has_byte_param)
    Log_TracePrintf("DSP command %02X %02X", ZeroExtend32(GetDSPCommand()), ZeroExtend32(GetDSPCommandParameterByte()));
  else
    Log_TracePrintf("DSP command %02X", ZeroExtend32(GetDSPCommand()));

  switch (command)
  {
    case DSP_COMMAND_IDENTIFY:
    {
      if (!has_byte_param)
        return;

      u8 param = GetDSPCommandParameterByte();
      Log_DevPrintf("DAC identify 0x%02X", ZeroExtend32(param));
      ClearAndWriteDSPOutputBuffer(~param);
    }
    break;

    case DSP_COMMAND_VERSION:
    {
      Log_DevPrintf("DSP version 0x%04X", m_dsp_version);

      ClearDSPOutputBuffer();
      WriteDSPOutputBuffer(Truncate8(m_dsp_version >> 8));
      WriteDSPOutputBuffer(Truncate8(m_dsp_version));
    }
    break;

    case DSP_COMMAND_PAUSE_DMA:
    {
      Log_DevPrintf("DAC pause DMA");
      m_dac_state.dma_paused = true;
      if (m_dac_state.dma_active)
      {
        SetDMARequest(m_adc_state.dma_16, false);
        UpdateDACSampleEventState();
      }
    }
    break;

    case DSP_COMMAND_RESUME_DMA:
    {
      Log_DevPrintf("DAC resume DMA");
      m_dac_state.dma_paused = false;
      UpdateDACSampleEventState();
      if (m_dac_state.dma_active && !IsDACFIFOFull())
        SetDMARequest(m_dac_state.dma_16, true);
    }
    break;

    case DSP_COMMAND_SET_TIME_CONSTANT:
    {
      if (!has_byte_param)
        return;

      u8 param = GetDSPCommandParameterByte();
      float frequency = 1000000.0f / float(256 - ZeroExtend32(param));
      Log_DevPrintf("DAC set frequency 0x%02X -> %.4f hz", ZeroExtend32(param), frequency);
      SetDACSampleRate(frequency);
    }
    break;

    case DSP_COMMAND_SET_OUTPUT_SAMPLE_RATE:
    {
      if (!has_word_param)
        return;

      u16 param = GetDSPCommandParameterWord();
      Log_DevPrintf("DAC set frequency %u hz", ZeroExtend32(param));
      SetDACSampleRate(static_cast<float>(param));
    }
    break;

    case DSP_COMMAND_DIRECT_SAMPLE:
    {
      if (!has_byte_param)
        return;

      u8 param = GetDSPCommandParameterByte();
      Log_DevPrintf("DAC direct sample 0x%02X", ZeroExtend32(param));

      // Ensure our output is in sync.
      UpdateDSPAudioOutput();

      // DMA shouldn't be in progress.
      StopADCDMA();
      m_dac_state.sample_format = DSP_SAMPLE_FORMAT_U8_PCM;

      // Push a single byte to the fifo, and schedule the event to pick it up at the appropriate time.
      m_dac_state.fifo.clear();
      m_dac_state.fifo.push_back(param);
      if (m_dac_state.stereo)
        m_dac_state.fifo.push_back(param);

      UpdateDACSampleEventState();
    }
    break;

    case DSP_COMMAND_DMA_8_BIT_PCM_OUTPUT:
    case DSP_COMMAND_DMA_4_BIT_ADPCM_OUTPUT:
    case DSP_COMMAND_DMA_4_BIT_ADPCM_OUTPUT_WITH_REF:
    case DSP_COMMAND_DMA_3_BIT_ADPCM_OUTPUT:
    case DSP_COMMAND_DMA_3_BIT_ADPCM_OUTPUT_WITH_REF:
    case DSP_COMMAND_DMA_2_BIT_ADPCM_OUTPUT:
    case DSP_COMMAND_DMA_2_BIT_ADPCM_OUTPUT_WITH_REF:
    {
      if (!has_word_param)
        return;

      DSP_SAMPLE_FORMAT sample_format = DSP_SAMPLE_FORMAT_U8_PCM;
      switch (command)
      {
        case DSP_COMMAND_DMA_8_BIT_PCM_OUTPUT:
          sample_format = DSP_SAMPLE_FORMAT_U8_PCM;
          break;
        case DSP_COMMAND_DMA_4_BIT_ADPCM_OUTPUT:
          sample_format = DSP_SAMPLE_FORMAT_U4_ADPCM;
          break;
        case DSP_COMMAND_DMA_4_BIT_ADPCM_OUTPUT_WITH_REF:
          sample_format = DSP_SAMPLE_FORMAT_U4_ADPCM;
          break;
        case DSP_COMMAND_DMA_3_BIT_ADPCM_OUTPUT:
          sample_format = DSP_SAMPLE_FORMAT_U3_ADPCM;
          break;
        case DSP_COMMAND_DMA_3_BIT_ADPCM_OUTPUT_WITH_REF:
          sample_format = DSP_SAMPLE_FORMAT_U3_ADPCM;
          break;
        case DSP_COMMAND_DMA_2_BIT_ADPCM_OUTPUT:
          sample_format = DSP_SAMPLE_FORMAT_U2_ADPCM;
          break;
        case DSP_COMMAND_DMA_2_BIT_ADPCM_OUTPUT_WITH_REF:
          sample_format = DSP_SAMPLE_FORMAT_U2_ADPCM;
          break;
      }

      u32 length = ZeroExtend32(GetDSPCommandParameterWord()) + 1;
      bool update_reference_byte = (command != DSP_COMMAND_DMA_8_BIT_PCM_OUTPUT) ? ((command & 0x01) != 0) : false;
      Log_DevPrintf("Single cycle 8-bit%s DMA %u bytes", (command != DSP_COMMAND_DMA_8_BIT_PCM_OUTPUT) ? " ADPCM" : "",
                    length);
      StartDACDMA(false, sample_format, false, update_reference_byte, false, false, length);
    }
    break;

    case DSP_COMMAND_DMA_8_BIT_PCM_OUTPUT_AUTOINIT:
    case DSP_COMMAND_DMA_4_BIT_ADPCM_OUTPUT_AUTOINIT:
    case DSP_COMMAND_DMA_3_BIT_ADPCM_OUTPUT_AUTOINIT:
    case DSP_COMMAND_DMA_2_BIT_ADPCM_OUTPUT_AUTOINIT:
    {
      DSP_SAMPLE_FORMAT sample_format = DSP_SAMPLE_FORMAT_U8_PCM;
      switch (command)
      {
        case DSP_COMMAND_DMA_8_BIT_PCM_OUTPUT_AUTOINIT:
          sample_format = DSP_SAMPLE_FORMAT_U8_PCM;
          break;
        case DSP_COMMAND_DMA_4_BIT_ADPCM_OUTPUT_AUTOINIT:
          sample_format = DSP_SAMPLE_FORMAT_U4_ADPCM;
          break;
        case DSP_COMMAND_DMA_3_BIT_ADPCM_OUTPUT_AUTOINIT:
          sample_format = DSP_SAMPLE_FORMAT_U3_ADPCM;
          break;
        case DSP_COMMAND_DMA_2_BIT_ADPCM_OUTPUT_AUTOINIT:
          sample_format = DSP_SAMPLE_FORMAT_U2_ADPCM;
          break;
      }

      Log_DevPrintf("Auto-init 8-bit%s DMA %u bytes",
                    (command != DSP_COMMAND_DMA_8_BIT_PCM_OUTPUT_AUTOINIT) ? " ADPCM" : "", m_dac_state.dma_block_size);
      StartDACDMA(false, sample_format, false, false, true, false, m_dac_state.dma_block_size);
    }
    break;

    case DSP_COMMAND_HIGH_SPEED_DMA_8_BIT_PCM_OUTPUT:
    {
      Log_DevPrintf("Single-cycle highspeed 8-bit DMA %u bytes", m_dac_state.dma_block_size);
      StartDACDMA(false, DSP_SAMPLE_FORMAT_U8_PCM, false, false, false, false, m_dac_state.dma_block_size);
    }
    break;

    case DSP_COMMAND_HIGH_SPEED_DMA_8_BIT_PCM_OUTPUT_AUTOINIT:
    {
      Log_DevPrintf("Auto-init highspeed 8-bit DMA %u bytes", m_dac_state.dma_block_size);
      StartDACDMA(false, DSP_SAMPLE_FORMAT_U8_PCM, false, false, true, false, m_dac_state.dma_block_size);
    }
    break;

    case DSP_COMMAND_DMA_BLOCK_TRANSFER_SIZE:
    {
      if (!has_word_param)
        return;

      u32 block_size = ZeroExtend32(GetDSPCommandParameterWord()) + 1;
      Log_DevPrintf("DAC autoinit DMA block size %u", block_size);
      // UpdateOutput();

      m_dac_state.dma_block_size = block_size;
    }
    break;

    case DSP_COMMAND_PROGRAM_DMA_8 + 0x00:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x01:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x02:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x03:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x04:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x05:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x06:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x07:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x08:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x09:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x0A:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x0B:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x0C:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x0D:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x0E:
    case DSP_COMMAND_PROGRAM_DMA_8 + 0x0F:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x00:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x01:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x02:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x03:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x04:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x05:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x06:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x07:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x08:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x09:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x0A:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x0B:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x0C:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x0D:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x0E:
    case DSP_COMMAND_PROGRAM_DMA_16 + 0x0F:
    {
      if (m_dsp_input_buffer.size() < 4)
        return;

      u8 mode = m_dsp_input_buffer[1];
      u32 length = (ZeroExtend32(m_dsp_input_buffer[2]) | (ZeroExtend32(m_dsp_input_buffer[3]) << 8)) + 1;
      bool is_adc = !!(command & (1 << 3));
      bool autoinit = !!(command & (1 << 2));
      bool fifo_enable = !!(command & (1 << 1));
      bool is_stereo = !!(mode & (1 << 5));
      bool is_signed = !!(mode & (1 << 4));
      bool is_dma16 = ((command & 0xF0) == DSP_COMMAND_PROGRAM_DMA_16);

      DSP_SAMPLE_FORMAT sample_format;
      if (is_dma16)
        sample_format = is_signed ? DSP_SAMPLE_FORMAT_S16_PCM : DSP_SAMPLE_FORMAT_U16_PCM;
      else
        sample_format = is_signed ? DSP_SAMPLE_FORMAT_S8_PCM : DSP_SAMPLE_FORMAT_U8_PCM;

      // Length here seems to be divided by 2 for 16-bit modes?
      u32 length_in_bytes = length;
      if (is_dma16)
        length_in_bytes *= 2;

      UpdateDSPAudioOutput();
      Log_DevPrintf("DAC program %u-bit %s%s%s %s %s %u samples", is_dma16 ? 16 : 8, is_adc ? "ADC" : "DAC",
                    autoinit ? " autoinit" : "", fifo_enable ? " fifo" : "", is_stereo ? "stereo" : "mono",
                    is_signed ? "signed" : "unsigned", length);

      StartDACDMA(is_dma16, sample_format, is_stereo, false, autoinit, fifo_enable, length_in_bytes);
    }
    break;

    case DSP_COMMAND_SILENCE:
    {
      if (!has_word_param)
        return;

      u32 samples = ZeroExtend32(GetDSPCommandParameterWord()) + 1;
      Log_DevPrintf("DAC silence for %u samples", samples);
      UpdateDSPAudioOutput();

      // Wipe the FIFO. Is this correct?
      StopDACDMA();
      m_dac_state.silence_samples = samples;
      UpdateDACSampleEventState();
    }
    break;

    case DSP_COMMAND_INTERRUPT_REQUEST:
    {
      Log_DevPrintf("DSP interrupt request");
      RaiseInterrupt(false);
    }
    break;

    case DSP_COMMAND_INTERRUPT_REQUEST_16:
    {
      Log_DevPrintf("DSP interrupt request 16");
      RaiseInterrupt(true);
    }
    break;

    case DSP_COMMAND_READ_TEST_REGISTER:
    {
      Log_DevPrintf("DSP read test register");
      ClearAndWriteDSPOutputBuffer(m_dsp_test_register);
    }
    break;

    case DSP_COMMAND_WRITE_TEST_REGISTER:
    {
      if (!has_byte_param)
        return;

      u8 param = GetDSPCommandParameterByte();
      Log_DevPrintf("DSP write test register 0x%02X", ZeroExtend32(param));
      m_dsp_test_register = param;
    }
    break;

    case DSP_COMMAND_ENABLE_SPEAKER:
    {
      Log_DevPrintf("DAC enable speaker");
      UpdateDSPAudioOutput();
      m_dac_state.enable_speaker = true;
    }
    break;

    case DSP_COMMAND_DISABLE_SPEAKER:
    {
      Log_DevPrintf("DAC disable speaker");
      UpdateDSPAudioOutput();
      m_dac_state.enable_speaker = false;
    }
    break;

    case DSP_COMMAND_MPU_401_RESET:
    {
      // Just pretend it doesn't exist and don't send an acknowledge byte.
      Log_WarningPrintf("Unimplemented MPU-401 reset");
    }
    break;

    case DSP_COMMAND_UNKNOWN_E2:
    {
      if (!has_byte_param)
        return;

      // From dosbox sblaster.cpp
      // Some sort of DMA test?
      static const s32 E2_incr_table[4][9] = {{0x01, -0x02, -0x04, 0x08, -0x10, 0x20, 0x40, -0x80, -106},
                                              {-0x01, 0x02, -0x04, 0x08, 0x10, -0x20, 0x40, -0x80, 165},
                                              {-0x01, 0x02, 0x04, -0x08, 0x10, -0x20, -0x40, 0x80, -151},
                                              {0x01, -0x02, 0x04, -0x08, -0x10, 0x20, -0x40, 0x80, 90}};

      u8 param = GetDSPCommandParameterByte();
      Log_DevPrintf("DSP E2 command 0x%02X", ZeroExtend32(param));
      for (u8 i = 0; i < 8; i++)
      {
        if (param & (1 << i))
          m_adc_state.e2_value += E2_incr_table[m_adc_state.e2_count % countof(E2_incr_table)][i];
      }
      m_adc_state.e2_value += E2_incr_table[m_adc_state.e2_count % countof(E2_incr_table)][8];
      m_adc_state.e2_count++;

      // Place the value in the ADC fifo for sending to the host
      m_adc_state.fifo.clear();
      m_adc_state.fifo.push_back(Truncate8(m_adc_state.e2_value));
      StartADCDMA(false, 1);
    }
    break;

    case DSP_COMMAND_UNKNOWN_E3:
    {
      Log_DevPrintf("DSP E3 command");
      ClearDSPOutputBuffer();

      // Apparently the null terminator must be included here.
      static const char copyright_string[] = "COPYRIGHT (C) CREATIVE TECHNOLOGY LTD, 1992.";
      for (size_t i = 0; i < countof(copyright_string); i++)
        WriteDSPOutputBuffer(u8(copyright_string[i]));
    }
    break;

    case DSP_COMMAND_UNKNOWN_F8:
      ClearAndWriteDSPOutputBuffer(0x00);
      break;

    default:
      Log_ErrorPrintf("Unhandled DSP command 0x%02X", ZeroExtend32(command));
      break;
  }

  ClearDSPInputBuffer();
}

size_t SoundBlaster::GetBytesPerSample(DSP_SAMPLE_FORMAT format)
{
  switch (format)
  {
    case DSP_SAMPLE_FORMAT_U2_ADPCM:
    case DSP_SAMPLE_FORMAT_U3_ADPCM:
    case DSP_SAMPLE_FORMAT_U4_ADPCM:
      return sizeof(u8);
    case DSP_SAMPLE_FORMAT_S8_PCM:
    case DSP_SAMPLE_FORMAT_U8_PCM:
      return sizeof(u8);
    case DSP_SAMPLE_FORMAT_S16_PCM:
    case DSP_SAMPLE_FORMAT_U16_PCM:
    default:
      return sizeof(s16);
  }
}

u32 SoundBlaster::GetSamplesPerDMATransfer(DSP_SAMPLE_FORMAT format)
{
  switch (format)
  {
    case DSP_SAMPLE_FORMAT_U2_ADPCM:
      return 4;
    case DSP_SAMPLE_FORMAT_U3_ADPCM:
      return 3;
    case DSP_SAMPLE_FORMAT_U4_ADPCM:
      return 2;
    case DSP_SAMPLE_FORMAT_S8_PCM:
    case DSP_SAMPLE_FORMAT_U8_PCM:
    case DSP_SAMPLE_FORMAT_S16_PCM:
    case DSP_SAMPLE_FORMAT_U16_PCM:
    default:
      return 1;
  }
}

void SoundBlaster::DACSampleEvent(CycleCount cycles)
{
  size_t num_samples = size_t(cycles);
  s16* obuf = reinterpret_cast<s16*>(m_dac_state.output_channel->ReserveInputSamples(num_samples));

  // TODO: Invert loop
  for (size_t sample_index = 0; sample_index < num_samples; sample_index++)
  {
    if (m_dac_state.silence_samples > 0)
    {
      m_dac_state.silence_samples--;
      if (m_dac_state.silence_samples == 0)
        RaiseInterrupt(m_dac_state.dma_16);

      if (m_dac_state.stereo)
      {
        obuf[sample_index * 2 + 0] = m_dac_state.last_sample[0] = 0;
        obuf[sample_index * 2 + 1] = m_dac_state.last_sample[1] = 0;
      }
    }
    else
    {
      m_dac_state.last_sample[0] = DecodeDACOutputSample(m_dac_state.last_sample[0]);
      if (m_dac_state.stereo)
        m_dac_state.last_sample[1] = DecodeDACOutputSample(m_dac_state.last_sample[1]);
      else
        m_dac_state.last_sample[1] = m_dac_state.last_sample[0];

      obuf[sample_index * 2 + 0] = m_dac_state.last_sample[0];
      obuf[sample_index * 2 + 1] = m_dac_state.last_sample[1];
    }
  }

  m_dac_state.output_channel->CommitInputSamples(num_samples);

  // If a DMA is not active, we're just going to be repeating the same value anyway, so set a higher interval
  UpdateDACSampleEventState();
}

void SoundBlaster::UpdateDACSampleEventState()
{
  // If a DMA is not active, we're just going to be repeating the same value anyway, so set a higher interval
  if (m_dac_state.silence_samples == 0 && m_dac_state.fifo.empty() &&
      !(m_dac_state.dma_active && !m_dac_state.dma_paused))
  {
    if (m_dac_state.sample_event->IsActive())
      m_dac_state.sample_event->Deactivate();
  }
  else
  {
    // Batch samples together in ADPCM modes.
    CycleCount interval = CycleCount(GetSamplesPerDMATransfer(m_dac_state.sample_format));
    if (!m_dac_state.sample_event->IsActive())
      m_dac_state.sample_event->Queue(interval);
    else if (m_dac_state.sample_event->GetInterval() != interval)
      m_dac_state.sample_event->Reschedule(interval);
  }
}

void SoundBlaster::SetDACSampleRate(float frequency)
{
  if (frequency != m_dac_state.frequency)
  {
    UpdateDSPAudioOutput();
    m_dac_state.frequency = frequency;
    m_dac_state.sample_event->SetFrequency(frequency);
    m_dac_state.output_channel->ChangeSampleRate(frequency);
    UpdateDACSampleEventState();
  }
}

s16 SoundBlaster::DecodeDACOutputSample(s16 last_sample)
{
  // If the fifo is empty, warn, then return the last sample without change.
  size_t required_bytes = GetBytesPerSample(m_dac_state.sample_format);
  if (m_dac_state.adpcm_reference_update_pending)
    required_bytes += sizeof(u8);
  if (m_dac_state.fifo.size() < required_bytes)
  {
    Log_DevPrintf("FIFO empty and sample requested");
    return last_sample;
  }

  // Update ADPCM reference byte?
  if (m_dac_state.adpcm_reference_update_pending)
  {
    m_dac_state.adpcm_reference = m_dac_state.fifo.front();
    m_dac_state.fifo.pop_front();
    m_dac_state.adpcm_scale = 0;
    m_dac_state.adpcm_reference_update_pending = false;
  }

  // Something something depending on format
  s16 sample;

  switch (m_dac_state.sample_format)
  {
      // 8-bit unsigned -> 16-bit unsigned -> 16-bit signed
    case DSP_SAMPLE_FORMAT_U8_PCM:
    {
      u8 input_sample = m_dac_state.fifo.front();
      m_dac_state.fifo.pop_front();
      u16 to16 = (ZeroExtend16(input_sample) << 8) | ZeroExtend16(input_sample);
      sample = Truncate16(s32(ZeroExtend32(to16)) - 32768);
    }
    break;

    case DSP_SAMPLE_FORMAT_S8_PCM:
    {
      u8 input_sample = m_dac_state.fifo.front();
      m_dac_state.fifo.pop_front();
      sample = s16(ZeroExtend16(input_sample) << 8) | ZeroExtend16(input_sample);
    }
    break;

    case DSP_SAMPLE_FORMAT_U16_PCM:
    {
      u16 input_sample = (ZeroExtend16(m_dac_state.fifo[1]) << 8) | ZeroExtend16(m_dac_state.fifo[0]);
      m_dac_state.fifo.pop_front();
      m_dac_state.fifo.pop_front();
      sample = Truncate16(s32(ZeroExtend32(input_sample)) - 32768);
    }
    break;

    case DSP_SAMPLE_FORMAT_S16_PCM:
    {
      u16 input_sample = (ZeroExtend16(m_dac_state.fifo[1]) << 8) | ZeroExtend16(m_dac_state.fifo[0]);
      m_dac_state.fifo.pop_front();
      m_dac_state.fifo.pop_front();
      sample = s16(input_sample);
    }
    break;

      // High bits -> low bits for ADPCM formats
      // TODO: These are only supported for mono, not stereo.
    case DSP_SAMPLE_FORMAT_U2_ADPCM:
    {
      u8 input_sample = m_dac_state.fifo.front();
      input_sample = (input_sample >> (6 - (m_dac_state.adpcm_subsample * 2))) & 3;
      sample = DecodeADPCM_2(input_sample, m_dac_state.adpcm_reference, m_dac_state.adpcm_scale);
      m_dac_state.adpcm_subsample = (m_dac_state.adpcm_subsample + 1) % 4;
      if (m_dac_state.adpcm_subsample == 0)
        m_dac_state.fifo.pop_front();
    }
    break;

    case DSP_SAMPLE_FORMAT_U3_ADPCM:
    {
      u8 input_sample = m_dac_state.fifo.front();
      switch (m_dac_state.adpcm_subsample)
      {
        case 0:
          input_sample = (input_sample >> 5) & 7;
          m_dac_state.adpcm_subsample++;
          break;
        case 1:
          input_sample = (input_sample >> 2) & 7;
          m_dac_state.adpcm_subsample++;
          break;
        case 2:
          input_sample = (input_sample & 3) << 1;
          m_dac_state.adpcm_subsample = 0;
          m_dac_state.fifo.pop_front();
          break;
      }

      sample = DecodeADPCM_3(input_sample, m_dac_state.adpcm_reference, m_dac_state.adpcm_scale);
    }
    break;

    case DSP_SAMPLE_FORMAT_U4_ADPCM:
    {
      u8 input_sample = m_dac_state.fifo.front();
      input_sample = (input_sample >> (4 - (m_dac_state.adpcm_subsample * 4))) & 15;
      sample = DecodeADPCM_4(input_sample, m_dac_state.adpcm_reference, m_dac_state.adpcm_scale);
      m_dac_state.adpcm_subsample = (m_dac_state.adpcm_subsample + 1) % 2;
      if (m_dac_state.adpcm_subsample == 0)
        m_dac_state.fifo.pop_front();
    }
    break;

    default:
      // Huh?
      sample = 0;
      break;
  }

  // If the FIFO is empty, we need a new byte from DMA
  if (!IsDACFIFOFull())
  {
    if (m_dac_state.dma_active && !m_dac_state.dma_paused)
      SetDMARequest(m_dac_state.dma_16, true);
  }

#if 0
    static FILE* fp = nullptr;
    if (!fp)
        fp = fopen("D:\\sbdspraw.raw", "wb");
    if (fp)
    {
        fwrite(&sample, sizeof(int16), 1, fp);
        fflush(fp);
        }
#endif

  // Log_DevPrintf("DAC sample: %d", SignExtend32(sample));
  return sample;
}

bool SoundBlaster::IsDACFIFOFull() const
{
  // 16-bit formats should be at least 2 bytes, 8-bit should be at least 1.
  size_t required_bytes = GetBytesPerSample(m_dac_state.sample_format);
  if (m_dac_state.stereo)
    required_bytes *= 2;
  if (m_dac_state.fifo_enable)
    required_bytes *= 16;
  if (m_dac_state.adpcm_reference_update_pending)
    required_bytes++;

  return (m_dac_state.fifo.size() >= required_bytes);
}

void SoundBlaster::StartDACDMA(bool dma16, DSP_SAMPLE_FORMAT format, bool stereo, bool update_reference_byte,
                               bool autoinit, bool fifo_enable, u32 length_in_bytes)
{
  if (length_in_bytes == 0)
  {
    Log_WarningPrintf("Zero-byte DMA attempted, ignoring");
    return;
  }

  // Ensure everything is rendered for the time.
  UpdateDSPAudioOutput();

  // Stop any active DMA in progress.
  StopDACDMA();

  // Ensure the DMA channel is clear.
  StopDMA(dma16);

  // Configure the DMA.
  m_dac_state.dma_16 = dma16;
  m_dac_state.dma_active = true;
  m_dac_state.dma_paused = false;
  m_dac_state.fifo_enable = fifo_enable;
  m_dac_state.sample_format = format;
  m_dac_state.stereo = stereo;
  m_dac_state.adpcm_subsample = 0;
  m_dac_state.adpcm_reference_update_pending = update_reference_byte;
  StartDMA(dma16, false, autoinit, length_in_bytes, !IsDACFIFOFull());
  UpdateDACSampleEventState();
}

void SoundBlaster::StopDACDMA()
{
  if (!m_dac_state.dma_active)
    return;

  StopDMA(m_dac_state.dma_16);

  // This should get set by StopDMA()
  DebugAssert(!m_dac_state.dma_active);
  m_dac_state.dma_active = false;
}

void SoundBlaster::StartADCDMA(bool dma16, u32 length_in_bytes)
{
  if (length_in_bytes == 0)
  {
    Log_WarningPrintf("Zero-byte DMA attempted, ignoring");
    return;
  }

  // Ensure everything is rendered for the time.
  UpdateDSPAudioOutput();

  // Stop any active DMA in progress.
  StopADCDMA();

  // Ensure the DMA channel is clear.
  StopDMA(dma16);

  // Configure the DMA.
  m_adc_state.dma_16 = dma16;
  m_adc_state.dma_active = true;
  m_adc_state.dma_paused = false;
  StartDMA(dma16, true, false, length_in_bytes, !m_adc_state.fifo.empty());
}

void SoundBlaster::StopADCDMA()
{
  if (!m_adc_state.dma_active)
    return;

  StopDMA(m_adc_state.dma_16);

  // This should get set by StopDMA()
  DebugAssert(!m_adc_state.dma_active);
  m_adc_state.dma_active = false;
}

bool SoundBlaster::IsDMAActive(bool dma16) const
{
  const DMAState& state = dma16 ? m_dma_16_state : m_dma_state;
  return state.active;
}

void SoundBlaster::StartDMA(bool dma16, bool dma_to_host, bool autoinit, u32 length, bool request /* = true */)
{
  DebugAssert(!m_dma_state.active);

  DMAState& state = dma16 ? m_dma_16_state : m_dma_state;
  state.length = length;
  state.remaining_bytes = length;
  state.dma_to_host = dma_to_host;
  state.autoinit = autoinit;
  state.active = true;
  state.request = request;

  m_dma_controller->SetDMAState(dma16 ? m_dma_channel_16 : m_dma_channel, request);
}

void SoundBlaster::SetDMARequest(bool dma16, bool request)
{
  DMAState& state = dma16 ? m_dma_16_state : m_dma_state;
  DebugAssert(state.active);

  state.request = request;

  m_dma_controller->SetDMAState(dma16 ? m_dma_channel_16 : m_dma_channel, request);
}

void SoundBlaster::StopDMA(bool dma16)
{
  DMAState& state = dma16 ? m_dma_16_state : m_dma_state;
  if (!state.active)
    return;

  Log_DebugPrintf("Stop DMA");
  state.active = false;
  state.request = false;
  state.length = 0;
  state.remaining_bytes = 0;
  m_dma_controller->SetDMAState(dma16 ? m_dma_channel_16 : m_dma_channel, false);

  // Notify whatever set it up
  if (state.dma_to_host)
  {
    m_adc_state.dma_active = false;
  }
  else
  {
    m_dac_state.dma_active = false;
    UpdateDACSampleEventState();
  }
}

void SoundBlaster::DMAReadCallback(IOPortDataSize size, u32* value, u32 remaining_bytes, bool is_16_bit)
{
  DMAState& state = is_16_bit ? m_dma_16_state : m_dma_state;
  if (!state.dma_to_host)
  {
    Log_WarningPrintf("Incorrect DMA direction configured");
    *value = 0;
    return;
  }

  u32 data_size = is_16_bit ? sizeof(u16) : sizeof(u8);
  if (m_adc_state.fifo.size() >= data_size)
  {
    *value = ZeroExtend32(m_adc_state.fifo.front());
    m_adc_state.fifo.pop_front();
    if (is_16_bit)
    {
      *value |= (ZeroExtend32(m_adc_state.fifo.front()) << 8);
      m_adc_state.fifo.pop_front();
    }
  }
  else
  {
    Log_WarningPrintf("Insufficient data in ADC fifo");
    m_adc_state.fifo.clear();
  }

  // End of block?
  if (data_size >= state.remaining_bytes)
  {
    // Apparently we don't raise interrupts for ADC?
    state.remaining_bytes = 0;

    // Not autoinit?
    if (!state.autoinit)
    {
      m_adc_state.dma_active = false;
      StopDMA(is_16_bit);
      return;
    }

    // Autoinit
    state.remaining_bytes = state.length;
  }
  else
  {
    state.remaining_bytes -= data_size;
  }

  // Update request state if fifo is empty
  if (m_adc_state.fifo.empty())
    SetDMARequest(is_16_bit, false);
}

void SoundBlaster::DMAWriteCallback(IOPortDataSize size, u32 value, u32 remaining_bytes, bool is_16_bit)
{
  DMAState& state = is_16_bit ? m_dma_16_state : m_dma_state;
  if (state.dma_to_host)
  {
    Log_WarningPrintf("Incorrect DMA direction configured");
    return;
  }

  // Append to FIFO
  m_dac_state.fifo.push_back(Truncate8(value));
  if (is_16_bit)
    m_dac_state.fifo.push_back(Truncate8(value >> 8));

  // End of block?
  u32 data_size = is_16_bit ? sizeof(u16) : sizeof(u8);
  if (data_size >= state.remaining_bytes)
  {
    state.remaining_bytes = 0;
    RaiseInterrupt(is_16_bit);

    // Not autoinit?
    if (!state.autoinit)
    {
      m_dac_state.dma_active = false;
      StopDMA(is_16_bit);
      return;
    }

    // Autoinit
    state.remaining_bytes = state.length;
  }
  else
  {
    state.remaining_bytes -= data_size;
  }

  // FIFO full?
  if (IsDACFIFOFull())
  {
    // Set DMA to inactive until we need a new byte
    SetDMARequest(is_16_bit, false);
  }
}

u8 SoundBlaster::ReadMixerIndexPort()
{
  return m_mixer_index_register;
}

u8 SoundBlaster::ReadMixerDataPort()
{
  switch (m_type)
  {
    case Type::SoundBlaster20:
      return ReadMixerDataPortCT1335();
    case Type::SoundBlasterPro:
      return ReadMixerDataPortCT1345();
    case Type::SoundBlaster16:
      return ReadMixerDataPortCT1745();
    default:
      return 0;
  }
}

void SoundBlaster::WriteMixerIndexPort(u8 value)
{
  m_mixer_index_register = value;
}

void SoundBlaster::WriteMixerDataPort(u8 value)
{
  switch (m_type)
  {
    case Type::SoundBlaster20:
      WriteMixerDataPortCT1335(value);
      break;
    case Type::SoundBlasterPro:
      WriteMixerDataPortCT1345(value);
      break;
    case Type::SoundBlaster16:
      WriteMixerDataPortCT1745(value);
      break;
    default:
      break;
  }
}

u8 SoundBlaster::ReadMixerDataPortCT1335()
{
  switch (m_mixer_index_register)
  {
    default:
      Log_ErrorPrintf("Read unknown mixer register 0x%02X", ZeroExtend32(m_mixer_index_register));
      return 0xAA;
  }
}

void SoundBlaster::WriteMixerDataPortCT1335(u8 value)
{
  switch (m_mixer_index_register)
  {
    default:
      Log_ErrorPrintf("Write unknown mixer register 0x%02X (value 0x%02X)", ZeroExtend32(m_mixer_index_register),
                      ZeroExtend32(value));
      break;
  }
}

u8 SoundBlaster::ReadMixerDataPortCT1345()
{
  switch (m_mixer_index_register)
  {
    case 0x04: // Voice L/R
    case 0x22: // Master L/R
    {
      float left;
      float right;

      switch (m_mixer_index_register)
      {
        case 0x04: // Voice L/R
          left = m_mixer_state.voice_volume[0];
          right = m_mixer_state.voice_volume[1];
          break;

        case 0x22: // Master L/R
          left = m_mixer_state.master_volume[0];
          right = m_mixer_state.master_volume[1];
          break;

        default:
          left = 0.0f;
          right = 0.0f;
          break;
      }

      return ((u8(left * 7.0f) << 1) | (u8(right * 7.0f) << 5));
    }
    break;

    default:
      Log_ErrorPrintf("Read unknown mixer register 0x%02X", ZeroExtend32(m_mixer_index_register));
      return 0xAA;
  }
}

void SoundBlaster::WriteMixerDataPortCT1345(u8 value)
{
  switch (m_mixer_index_register)
  {
    case 0x00: // Mixer reset
      UpdateDSPAudioOutput();
      ResetMixer();
      break;

    case 0x04: // Voice L/R
    case 0x22: // Master L/R
    {
      float left = float((value >> 1) & 7) / 7.0f;
      float right = float((value >> 5) & 7) / 7.0f;

      switch (m_mixer_index_register)
      {
        case 0x04: // Voice L/R
          m_mixer_state.voice_volume[0] = left;
          m_mixer_state.voice_volume[1] = right;
          Log_ErrorPrintf("Voice volume <- %f %f", left, right);
          break;

        case 0x22: // Master L/R
          m_mixer_state.master_volume[0] = left;
          m_mixer_state.master_volume[1] = right;
          Log_ErrorPrintf("Master volume <- %f %f", left, right);
          break;
      }
    }
    break;

    default:
      Log_ErrorPrintf("Write unknown mixer register 0x%02X (value 0x%02X)", ZeroExtend32(m_mixer_index_register),
                      ZeroExtend32(value));
      break;
  }
}

u8 SoundBlaster::ReadMixerDataPortCT1745()
{
  switch (m_mixer_index_register)
  {
    case 0x80: // IRQ select
    {
      switch (m_irq)
      {
        case 2:
          return 0x01;
        case 5:
          return 0x02;
        case 7:
          return 0x04;
        case 10:
          return 0x08;
        default:
          return 0x00;
      }
    }
    break;

    case 0x81: // DMA select
    {
      u8 ret = 0;
      switch (m_dma_channel)
      {
        case 0:
          ret |= 0x01;
          break;
        case 1:
          ret |= 0x02;
          break;
        case 3:
          ret |= 0x03;
          break;
      }
      switch (m_dma_channel_16)
      {
        case 5:
          ret |= 0x20;
          break;
        case 6:
          ret |= 0x40;
          break;
        case 7:
          ret |= 0x80;
          break;
      }
      return ret;
    }
    break;

    case 0x82: // IRQ status
    {
      u8 val = (BoolToUInt8(m_interrupt_pending)) | (BoolToUInt8(m_interrupt_pending_16) << 1) |
               (BoolToUInt8(m_type >= Type::SoundBlaster16) << 5);
      return val;
    }
    break;

    default:
      Log_ErrorPrintf("Read unknown mixer register 0x%02X", ZeroExtend32(m_mixer_index_register));
      return 0xAA;
  }
}

void SoundBlaster::WriteMixerDataPortCT1745(u8 value)
{
  switch (m_mixer_index_register)
  {
    default:
      Log_ErrorPrintf("Write unknown mixer register 0x%02X (value 0x%02X)", ZeroExtend32(m_mixer_index_register),
                      ZeroExtend32(value));
      break;
  }
}

void SoundBlaster::ResetMixer()
{
  m_mixer_state.master_volume.fill(1.0f);
}

} // namespace HW