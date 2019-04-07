#pragma once
#include "pce/cpu_x86/cpu_x86.h"
#include "pce/hw/ds12887.h"
#include "pce/hw/fdc.h"
#include "pce/hw/i8042_ps2.h"
#include "pce/hw/i8237_dma.h"
#include "pce/hw/i82437fx.h"
#include "pce/hw/i8253_pit.h"
#include "pce/hw/i8259_pic.h"
#include "pce/hw/pci_ide.h"
#include "pce/hw/pcspeaker.h"
#include "pce/systems/pcipc.h"

class ByteStream;

namespace Systems {

class i430FX : public PCIPC
{
  DECLARE_OBJECT_TYPE_INFO(i430FX, PCIPC);
  DECLARE_OBJECT_GENERIC_FACTORY(i430FX);
  DECLARE_OBJECT_PROPERTY_MAP(i430FX);

public:
  i430FX(CPU_X86::Model model = CPU_X86::MODEL_PENTIUM, float cpu_frequency = 75000000.0f,
         u32 memory_size = 16 * 1024 * 1024, const ObjectTypeInfo* type_info = &s_type_info);
  ~i430FX();

  virtual bool Initialize() override;
  virtual void Reset() override;

  virtual bool LoadSystemState(BinaryReader& reader) override;
  virtual bool SaveSystemState(BinaryWriter& writer) override;

  void SetBIOSFilePath(const String& path) { m_bios_file_path = path; }

  auto GetFDDController() const { return m_fdd_controller; }
  auto GetHDDController() const { return m_hdd_controller; }
  auto GetKeyboardController() const { return m_keyboard_controller; }
  auto GetDMAController() const { return m_dma_controller; }
  auto GetTimer() const { return m_timer; }
  auto GetCMOS() const { return m_cmos; }

protected:
  static constexpr u32 PHYSICAL_MEMORY_BITS = 32;
  static constexpr PhysicalMemoryAddress BIOS_ROM_ADDRESS = 0xE0000;
  static constexpr u32 BIOS_ROM_SIZE = 131072;
  static constexpr PhysicalMemoryAddress BIOS_ROM_MIRROR_ADDRESS = 0xFFFF0000;
  static constexpr u32 BIOS_ROM_MIRROR_START = 0x10000;
  static constexpr u32 BIOS_ROM_MIRROR_SIZE = 65536;

  void ConnectSystemIOPorts();
  void AddComponents();
  void SetCMOSVariables();

  void IOReadSystemControlPortA(u8* value);
  void IOWriteSystemControlPortA(u8 value);
  void IOReadSystemControlPortB(u8* value);
  void IOWriteSystemControlPortB(u8 value);
  void UpdateKeyboardControllerOutputPort();

  String m_bios_file_path;
  u32 m_ram_size = 16 * 1024 * 1024;

  HW::i82437FX* m_sb82437 = nullptr;

  HW::i8042_PS2* m_keyboard_controller = nullptr;
  HW::i8237_DMA* m_dma_controller = nullptr;
  HW::i8253_PIT* m_timer = nullptr;
  HW::i8259_PIC* m_interrupt_controller = nullptr;

  HW::DS12887* m_cmos = nullptr;

  HW::PCSpeaker* m_speaker = nullptr;

  HW::FDC* m_fdd_controller = nullptr;
  HW::PCIIDE* m_hdd_controller = nullptr;

  bool m_cmos_lock = false;
};

} // namespace Systems