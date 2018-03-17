#pragma once
#include "pce/hw/fdc.h"
#include "pce/hw/i8237_dma.h"
#include "pce/hw/i8253_pit.h"
#include "pce/hw/i8259_pic.h"
#include "pce/hw/pcspeaker.h"
#include "pce/hw/xt_ppi.h"
#include "pce/systems/pcbase.h"

class ByteStream;

namespace Systems {

class PCXT : public PCBase
{
public:
  static const uint32 PHYSICAL_MEMORY_BITS = 20;
  static const PhysicalMemoryAddress BIOS_ROM_ADDRESS_8K = 0xFE000;
  static const PhysicalMemoryAddress BIOS_ROM_ADDRESS_32K = 0xF8000;

  PCXT(HostInterface* host_interface, float cpu_frequency = 1000000.0f, uint32 memory_size = 640 * 1024);
  ~PCXT();

  const char* GetSystemName() const override { return "IBM XT"; }
  InterruptController* GetInterruptController() const override { return m_interrupt_controller; }
  void Initialize() override;
  void Reset() override;

  auto GetPPI() const { return m_ppi; }
  auto GetFDDController() const { return m_fdd_controller; }
  auto GetDMAController() const { return m_dma_controller; }
  auto GetTimer() const { return m_timer; }

private:
  static constexpr uint32 SERIALIZATION_ID = Component::MakeSerializationID('P', 'C', 'X', 'T');
  static constexpr size_t SWITCH_COUNT = 8;
  enum VIDEO_TYPE
  {
    VIDEO_TYPE_MDA,
    VIDEO_TYPE_CGA40,
    VIDEO_TYPE_CGA80,
    VIDEO_TYPE_OTHER
  };

  virtual bool LoadSystemState(BinaryReader& reader) override;
  virtual bool SaveSystemState(BinaryWriter& writer) override;

  void AddComponents();
  void ConnectSystemIOPorts();
  void SetSwitches();

  // IO read/write handlers
  void HandlePortRead(uint32 port, uint8* value);
  void HandlePortWrite(uint32 port, uint8 value);

  HW::i8237_DMA* m_dma_controller = nullptr;
  HW::i8253_PIT* m_timer = nullptr;
  HW::i8259_PIC* m_interrupt_controller = nullptr;
  HW::XT_PPI* m_ppi = nullptr;
  HW::PCSpeaker* m_speaker = nullptr;
  HW::FDC* m_fdd_controller = nullptr;

  // State to save below:
  uint8 m_nmi_mask = 0;
};

} // namespace Systems