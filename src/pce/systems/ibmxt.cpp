#include "pce/systems/ibmxt.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu_x86/cpu.h"
Log_SetChannel(Systems::ISAPC);

namespace Systems {

IBMXT::IBMXT(HostInterface* host_interface, float cpu_frequency /* = 1000000.0f */,
             uint32 memory_size /* = 640 * 1024 */, VideoType video_type /* = VideoType::Other */)
  : ISAPC(host_interface), m_bios_file_path("romimages/PCXTBIOS.BIN"), m_video_type(video_type)
{
  m_cpu = new CPU_X86::CPU(CPU_X86::MODEL_8086, cpu_frequency);
  m_bus = new Bus(PHYSICAL_MEMORY_BITS);
  AllocatePhysicalMemory(memory_size, true, true);
  AddComponents();
}

IBMXT::~IBMXT() {}

bool IBMXT::Initialize()
{
  if (!ISAPC::Initialize())
    return false;

  if (!m_bus->CreateROMRegionFromFile(m_bios_file_path.c_str(), BIOS_ROM_ADDRESS_8K, 8192))
    return false;

  ConnectSystemIOPorts();
  SetSwitches();
  return true;
}

void IBMXT::Reset()
{
  ISAPC::Reset();

  m_nmi_mask = 0;

  // We can save some CPU by not simulating the timer that triggers the DMA refresh.
  m_dma_controller->SetDMAState(0, true);
}

bool IBMXT::LoadSystemState(BinaryReader& reader)
{
  if (!ISAPC::LoadSystemState(reader))
    return false;

  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  m_nmi_mask = reader.ReadUInt8();

  return !reader.GetErrorState();
}

bool IBMXT::SaveSystemState(BinaryWriter& writer)
{
  if (!ISAPC::SaveSystemState(writer))
    return false;

  writer.WriteUInt32(SERIALIZATION_ID);

  writer.WriteUInt8(m_nmi_mask);

  return !writer.InErrorState();
}

void IBMXT::AddComponents()
{
  m_dma_controller = new HW::i8237_DMA();
  m_timer = new HW::i8253_PIT();
  m_interrupt_controller = new HW::i8259_PIC();
  m_ppi = new HW::XT_PPI();
  m_speaker = new HW::PCSpeaker();
  m_fdd_controller = new HW::FDC(HW::FDC::Model_8272, m_dma_controller);

  AddComponent(m_interrupt_controller);
  AddComponent(m_dma_controller);
  AddComponent(m_timer);
  AddComponent(m_ppi);
  AddComponent(m_speaker);
  AddComponent(m_fdd_controller);
}

void IBMXT::ConnectSystemIOPorts()
{
  // Connect channel 0 of the PIT to the interrupt controller
  m_timer->SetChannelOutputChangeCallback(0,
                                          [this](bool value) { m_interrupt_controller->SetInterruptState(0, value); });

  // Connect channel 2 of the PIT to the speaker
  m_timer->SetChannelOutputChangeCallback(2, [this](bool value) { m_speaker->SetLevel(value); });

  // Connect PPI to speaker
  m_ppi->SetSpeakerGateCallback([this](bool enabled) { m_timer->SetChannelGateInput(2, enabled); });
  m_ppi->SetSpeakerEnableCallback([this](bool enabled) { m_speaker->SetOutputEnabled(enabled); });
  m_ppi->SetSpeakerOutputCallback([this]() -> bool { return m_timer->GetChannelOutputState(2); });

  // The XT has no second interrupt controller.
  m_bus->ConnectIOPortReadToPointer(0x00A0, this, &m_nmi_mask);
  m_bus->ConnectIOPortWriteToPointer(0x00A0, this, &m_nmi_mask);

  // We need to set up a fake DMA channel for memory refresh.
  m_dma_controller->ConnectDMAChannel(0, [](IOPortDataSize, uint32*, uint32) {}, [](IOPortDataSize, uint32, uint32) {});

  // Connect channel 1 of the PIT to trigger memory refresh.
  // m_timer->SetChannelOutputChangeCallback(1, [this](bool value) { m_dma_controller->SetDMAState(0, value); });
}

void IBMXT::SetSwitches()
{
  // Switch settings.
  bool boot_loop = false;
  bool numeric_processor_installed = false;
  PhysicalMemoryAddress base_memory = GetBaseMemorySize();
  uint32 num_disk_drives = m_fdd_controller->GetDriveCount();

  // From http://www.rci.rutgers.edu/~preid/pcxtsw.htm
  m_ppi->SetSwitch(1 - 1, !boot_loop);
  m_ppi->SetSwitch(2 - 1, !numeric_processor_installed);
  if (base_memory >= 640 * 1024)
  {
    m_ppi->SetSwitch(3 - 1, false);
    m_ppi->SetSwitch(4 - 1, false);
  }
  else if (base_memory >= 576 * 1024)
  {
    m_ppi->SetSwitch(3 - 1, true);
    m_ppi->SetSwitch(4 - 1, false);
  }
  else if (base_memory >= 512 * 1024)
  {
    m_ppi->SetSwitch(3 - 1, false);
    m_ppi->SetSwitch(4 - 1, true);
  }
  else
  {
    m_ppi->SetSwitch(3 - 1, true);
    m_ppi->SetSwitch(4 - 1, true);
  }

  switch (m_video_type)
  {
    case VideoType::MDA:
      m_ppi->SetSwitch(5 - 1, false);
      m_ppi->SetSwitch(6 - 1, false);
      break;
    case VideoType::CGA80:
      m_ppi->SetSwitch(5 - 1, false);
      m_ppi->SetSwitch(6 - 1, true);
      break;
    case VideoType::CGA40:
      m_ppi->SetSwitch(5 - 1, true);
      m_ppi->SetSwitch(6 - 1, false);
      break;
    case VideoType::Other:
    default:
      m_ppi->SetSwitch(5 - 1, true);
      m_ppi->SetSwitch(6 - 1, true);
      break;
  }

  switch (num_disk_drives)
  {
    case 4:
      m_ppi->SetSwitch(7 - 1, false);
      m_ppi->SetSwitch(8 - 1, false);
      break;
    case 3:
      m_ppi->SetSwitch(7 - 1, true);
      m_ppi->SetSwitch(8 - 1, false);
      break;
    case 2:
      m_ppi->SetSwitch(7 - 1, false);
      m_ppi->SetSwitch(8 - 1, true);
      break;
    case 1:
    case 0:
    default:
      m_ppi->SetSwitch(7 - 1, true);
      m_ppi->SetSwitch(8 - 1, true);
      break;
  }
}

void IBMXT::HandlePortRead(uint32 port, uint8* value)
{
  switch (port)
  {
      // NMI Mask
    case 0xA0:
      *value = m_nmi_mask;
      break;
  }
}

void IBMXT::HandlePortWrite(uint32 port, uint8 value)
{
  switch (port)
  {
    case 0xA0:
      Log_WarningPrintf("NMI Mask <- 0x%02X", ZeroExtend32(value));
      m_nmi_mask = value;
      break;
  }
}

} // namespace Systems