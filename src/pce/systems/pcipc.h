#pragma once
#include "common/bitfield.h"
#include "pce/component.h"
#include "pce/hw/pci_device.h"
#include "pce/system.h"
#include "pce/systems/isapc.h"
#include <list>
#include <memory>

namespace Systems {

class PCIPC : public Systems::ISAPC
{
public:
  enum class PCIConfigSpaceAccessType
  {
    Type1,
    Type2
  };

  PCIPC(HostInterface* host_interface, PCIConfigSpaceAccessType config_access_type);
  virtual ~PCIPC();

  // TODO: Needs a better interface, probably with RTTI and AddComponent().
  bool AddPCIDevice(PCIDevice* dev);
  bool AddPCIDeviceToLocation(PCIDevice* dev, uint32 bus_number, int32 device_number);

protected:
  // NOTE: Assumes there is only a single PCI bus, and all devices are attached to it.
  static constexpr uint8 NUM_PCI_BUSES = 1;
  static constexpr uint8 NUM_PCI_DEVICES_PER_BUS = 16;

  virtual bool Initialize() override;
  virtual void Reset() override;

private:
  void ConnectPCIBusIOPorts();
  bool InitializePCIDevices();

  void IOReadPCIType1ConfigDataByte(uint32 port, uint8* value);
  void IOWritePCIType1ConfigDataByte(uint32 port, uint8 value);

  void IOReadPCIType2ConfigData(uint32 port, uint8* value);
  void IOWritePCIType2ConfigData(uint32 port, uint8 value);

  union PCIConfigType1Address
  {
    BitField<uint32, uint8, 31, 1> enable;
    BitField<uint32, uint8, 16, 8> bus;
    BitField<uint32, uint8, 11, 5> device;
    BitField<uint32, uint8, 8, 3> function;
    BitField<uint32, uint8, 2, 6> reg;
    uint32 bits;
  };

  union PCIConfigType2Address
  {
    BitField<uint8, uint8, 4, 4> key;
    BitField<uint8, uint8, 1, 3> function;
    BitField<uint8, bool, 0, 1> special_cycle;
    uint8 bits;
  };

  PCIDevice* m_pci_devices[NUM_PCI_BUSES][NUM_PCI_DEVICES_PER_BUS] = {};

  PCIConfigSpaceAccessType m_config_access_type;

  PCIConfigType1Address m_pci_config_type1_address{};

  PCIConfigType2Address m_pci_config_type2_address{};
  uint8 m_pci_config_type2_bus = 0;
};

} // namespace Systems