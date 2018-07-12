#pragma once
#include "pce/component.h"
#include <vector>

class PCIDevice : public Component
{
public:
  static constexpr uint32 NUM_CONFIG_REGISTERS = 64;

  PCIDevice(uint16 vendor_id, uint16 device_id, uint32 num_functions = 1);
  ~PCIDevice();

  virtual bool InitializePCIDevice(uint32 pci_bus_number, uint32 pci_device_number);
  virtual void Reset();

  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

  uint8 ReadConfigRegister(uint32 function, uint8 reg, uint8 index);
  void WriteConfigRegister(uint32 function, uint8 reg, uint8 index, uint8 value);

protected:
  virtual uint8 HandleReadConfigRegister(uint32 function, uint8 offset);
  virtual void HandleWriteConfigRegister(uint32 function, uint8 offset, uint8 value);

  uint32 m_num_functions = 0;
  uint32 m_pci_bus_number = 0;
  uint32 m_pci_device_number = 0;

  union ConfigSpace
  {
    uint32 dwords[NUM_CONFIG_REGISTERS];
    uint16 words[NUM_CONFIG_REGISTERS * 2];
    uint8 bytes[NUM_CONFIG_REGISTERS * 4];
  };

  std::vector<ConfigSpace> m_config_space;

private:
  static const uint32 SERIALIZATION_ID = MakeSerializationID('P', 'C', 'I', '-');
};
