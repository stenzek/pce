#pragma once
#include "pce/hw/fdc.h"
#include "pce/hw/i8237_dma.h"
#include "pce/hw/i8253_pit.h"
#include "pce/hw/i8259_pic.h"
#include "pce/hw/pcspeaker.h"
#include "pce/hw/xt_ppi.h"
#include "pce/systems/isapc.h"

class ByteStream;

namespace Systems {

class IBMXT : public ISAPC
{
  DECLARE_OBJECT_TYPE_INFO(IBMXT, ISAPC);
  DECLARE_OBJECT_GENERIC_FACTORY(IBMXT);
  DECLARE_OBJECT_PROPERTY_MAP(IBMXT);

public:
  enum class VideoType : u32
  {
    MDA,
    CGA40,
    CGA80,
    Other
  };

  static constexpr u32 PHYSICAL_MEMORY_BITS = 20;
  static constexpr PhysicalMemoryAddress BIOS_ROM_ADDRESS_8K = 0xFE000;
  static constexpr PhysicalMemoryAddress BIOS_ROM_ADDRESS_32K = 0xF8000;

  IBMXT(float cpu_frequency = 4770000.0f, u32 memory_size = 640 * 1024, VideoType video_type = VideoType::Other,
        const ObjectTypeInfo* type_info = &s_type_info);
  ~IBMXT();

  bool Initialize() override;
  void Reset() override;

  auto GetPPI() const { return m_ppi; }
  auto GetFDDController() const { return m_fdd_controller; }
  auto GetDMAController() const { return m_dma_controller; }
  auto GetTimer() const { return m_timer; }

private:
  static constexpr u32 SERIALIZATION_ID = Component::MakeSerializationID('P', 'C', 'X', 'T');
  static constexpr size_t SWITCH_COUNT = 8;

  virtual bool LoadSystemState(BinaryReader& reader) override;
  virtual bool SaveSystemState(BinaryWriter& writer) override;

  void AddComponents();
  void ConnectSystemIOPorts();
  void SetSwitches();

  // IO read/write handlers
  void HandlePortRead(u32 port, u8* value);
  void HandlePortWrite(u32 port, u8 value);

  String m_bios_file_path;

  HW::i8237_DMA* m_dma_controller = nullptr;
  HW::i8253_PIT* m_timer = nullptr;
  HW::i8259_PIC* m_interrupt_controller = nullptr;
  HW::XT_PPI* m_ppi = nullptr;
  HW::PCSpeaker* m_speaker = nullptr;
  HW::FDC* m_fdd_controller = nullptr;

  u32 m_ram_size = 640 * 1024;
  VideoType m_video_type = VideoType::Other;

  // State to save below:
  u8 m_nmi_mask = 0;
};

} // namespace Systems