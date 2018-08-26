#pragma once
#include "pce/hw/pci_device.h"

namespace Systems {
class PCIPC;
}

namespace HW {

class i82437FX : public PCIDevice
{
public:
  i82437FX(Systems::PCIPC* system, Bus* bus);
  ~i82437FX();

  bool InitializePCIDevice(uint32 pci_bus_number, uint32 pci_device_number) override;
  void Reset() override;

  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

protected:
  uint8 HandleReadConfigRegister(uint32 function, uint8 offset);
  void HandleWriteConfigRegister(uint32 function, uint8 offset, uint8 value);

private:
  static constexpr uint8 NUM_PAM_REGISTERS = 7;
  static constexpr uint8 PAM_BASE_OFFSET = 0x59;

  void SetPAMMapping(uint32 base, uint32 size, uint8 flag);
  void UpdatePAMMapping(uint8 offset);

  Systems::PCIPC* m_system;
  Bus* m_bus;
};

} // namespace HW