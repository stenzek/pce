#pragma once
#include "pce/bitfield.h"
#include "pce/cpu_x86/cpu.h"
#include "pce/hw/cmos.h"
#include "pce/hw/fdc.h"
#include "pce/hw/hdc.h"
#include "pce/hw/i8042_ps2.h"
#include "pce/hw/i8237_dma.h"
#include "pce/hw/i8253_pit.h"
#include "pce/hw/i8259_pic.h"
#include "pce/hw/pcspeaker.h"
#include "pce/systems/pcipc.h"

class ByteStream;

namespace Systems {

class i430FX : public PCIPC
{
public:
  i430FX(HostInterface* host_interface, CPU_X86::Model model = CPU_X86::MODEL_PENTIUM, float cpu_frequency = 2000000.0f,
         uint32 memory_size = 16 * 1024 * 1024);
  ~i430FX();

  const char* GetSystemName() const override { return "Intel 430FX"; }
  InterruptController* GetInterruptController() const override { return m_interrupt_controller; }
  bool Initialize() override;
  void Reset() override;

  void SetBIOSFilePath(const std::string& path) { m_bios_file_path = path; }

  auto GetFDDController() const { return m_fdd_controller; }
  auto GetHDDController() const { return m_hdd_controller; }
  auto GetKeyboardController() const { return m_keyboard_controller; }
  auto GetDMAController() const { return m_dma_controller; }
  auto GetTimer() const { return m_timer; }
  auto GetCMOS() const { return m_cmos; }

private:
  static const uint32 PHYSICAL_MEMORY_BITS = 32;
  static const PhysicalMemoryAddress BIOS_ROM_ADDRESS = 0xE0000;
  static const PhysicalMemoryAddress BIOS_ROM_MIRROR_ADDRESS = 0xFFFE0000;
  static const uint32 BIOS_ROM_SIZE = 131072;
  static const uint32 SHADOW_REGION_BASE = 0xC0000;
  static const uint32 SHADOW_REGION_SIZE = 0x8000;
  static const uint32 SHADOW_REGION_COUNT = 8;

  class SB82437FX : public PCIDevice
  {
  public:
    SB82437FX(i430FX* system, Bus* bus);
    ~SB82437FX();

    bool InitializePCIDevice(uint32 pci_bus_number, uint32 pci_device_number) override;
    void Reset() override;

  protected:
    uint8 HandleReadConfigRegister(uint32 function, uint8 offset);
    void HandleWriteConfigRegister(uint32 function, uint8 offset, uint8 value);

  private:
    static constexpr uint8 NUM_PAM_REGISTERS = 7;
    static constexpr uint8 PAM_BASE_OFFSET = 0x59;

    void SetPAMMapping(uint32 base, uint32 size, uint8 flag);
    void UpdatePAMMapping(uint8 offset);

    i430FX* m_system;
    Bus* m_bus;
  };

  virtual bool LoadSystemState(BinaryReader& reader) override;
  virtual bool SaveSystemState(BinaryWriter& writer) override;

  void ConnectSystemIOPorts();
  void AddComponents();
  void SetCMOSVariables();

  void IOReadSystemControlPortA(uint8* value);
  void IOWriteSystemControlPortA(uint8 value);
  void IOReadSystemControlPortB(uint8* value);
  void IOWriteSystemControlPortB(uint8 value);
  void UpdateKeyboardControllerOutputPort();

  std::string m_bios_file_path;

  SB82437FX* m_sb82437fx = nullptr;

  HW::i8042_PS2* m_keyboard_controller = nullptr;
  HW::i8237_DMA* m_dma_controller = nullptr;
  HW::i8253_PIT* m_timer = nullptr;
  HW::i8259_PIC* m_interrupt_controller = nullptr;

  HW::CMOS* m_cmos = nullptr;

  HW::PCSpeaker* m_speaker = nullptr;

  HW::FDC* m_fdd_controller = nullptr;
  HW::HDC* m_hdd_controller = nullptr;

  bool m_cmos_lock = false;
  bool m_refresh_bit = false;
};

} // namespace Systems