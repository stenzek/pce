#pragma once
#include "common/bitfield.h"
#include "pce/bus.h"
#include "pce/hw/pci_device.h"

class PCIBus : public Bus
{
  DECLARE_OBJECT_TYPE_INFO(PCIBus, Bus);
  DECLARE_OBJECT_NO_FACTORY(PCIBus);
  DECLARE_OBJECT_NO_PROPERTIES(PCIBus);

public:
  PCIBus(u32 memory_address_bits = 32, const ObjectTypeInfo* type_info = &s_type_info);
  virtual ~PCIBus();

  virtual bool Initialize(System* system) override;
  virtual void Reset() override;

  bool GetNextFreePCIDeviceNumber(u32* pci_bus_number, u32* pci_device_number) const;

  PCIDevice* GetPCIDevice(u32 pci_bus_number, u32 pci_device_number) const;
  bool AssignPCIDevice(u32 pci_bus_number, u32 pci_device_number, PCIDevice* device);

protected:
  // NOTE: Assumes there is only a single PCI bus, and all devices are attached to it.
  static constexpr uint8 NUM_PCI_BUSES = 1;
  static constexpr uint8 NUM_PCI_DEVICES_PER_BUS = 16;

  PCIDevice* m_pci_devices[NUM_PCI_BUSES][NUM_PCI_DEVICES_PER_BUS] = {};
};
