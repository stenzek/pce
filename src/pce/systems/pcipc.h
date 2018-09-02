#pragma once
#include "common/bitfield.h"
#include "pce/systems/isapc.h"

class PCIBus;

namespace Systems {

class PCIPC : public ISAPC
{
  DECLARE_OBJECT_TYPE_INFO(PCIPC, ISAPC);
  DECLARE_OBJECT_NO_FACTORY(PCIPC);
  DECLARE_OBJECT_NO_PROPERTIES(PCIPC);

public:
  enum class PCIConfigSpaceAccessType
  {
    Type1,
    Type2
  };

  PCIPC(PCIConfigSpaceAccessType config_access_type, const ObjectTypeInfo* type_info = &s_type_info);
  virtual ~PCIPC();

  PCIBus* GetPCIBus() const;

  // Creates a PCI device, and adds it to the specified location.
  template<typename T, typename... Args>
  T* CreatePCIDevice(u32 pci_bus_number, u32 pci_device_number, const String& identifier, Args...);

protected:
  // NOTE: Assumes there is only a single PCI bus, and all devices are attached to it.
  static constexpr uint8 NUM_PCI_BUSES = 1;
  static constexpr uint8 NUM_PCI_DEVICES_PER_BUS = 16;

  virtual bool Initialize() override;
  virtual void Reset() override;

private:
  void ConnectPCIBusIOPorts();

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

  PCIConfigSpaceAccessType m_config_access_type;

  PCIConfigType1Address m_pci_config_type1_address{};

  PCIConfigType2Address m_pci_config_type2_address{};
  uint8 m_pci_config_type2_bus = 0;
};

template<typename T, typename... Args>
T* PCIPC::CreatePCIDevice(u32 pci_bus_number, u32 pci_device_number, const String& identifier, Args... args)
{
  T* component = new T(identifier, args...);
  component->SetLocation(pci_bus_number, pci_device_number);
  AddComponent(component);
  return component;
}

} // namespace Systems