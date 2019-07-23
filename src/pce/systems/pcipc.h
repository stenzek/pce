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
  static constexpr u8 NUM_PCI_BUSES = 1;
  static constexpr u8 NUM_PCI_DEVICES_PER_BUS = 16;

  virtual bool Initialize() override;
  virtual void Reset() override;

  virtual bool LoadSystemState(BinaryReader& reader) override;
  virtual bool SaveSystemState(BinaryWriter& writer) override;

private:
  void ConnectPCIBusIOPorts();

  u8 IOReadPCIType1ConfigDataByte(u16 port);
  void IOWritePCIType1ConfigDataByte(u16 port, u8 value);

  u8 IOReadPCIType2ConfigData(u16 port);
  void IOWritePCIType2ConfigData(u16 port, u8 value);

  union PCIConfigType1Address
  {
    BitField<u32, u8, 31, 1> enable;
    BitField<u32, u8, 16, 8> bus;
    BitField<u32, u8, 11, 5> device;
    BitField<u32, u8, 8, 3> function;
    BitField<u32, u8, 2, 6> reg;
    u32 bits;
  };

  union PCIConfigType2Address
  {
    BitField<u8, u8, 4, 4> key;
    BitField<u8, u8, 1, 3> function;
    BitField<u8, bool, 0, 1> special_cycle;
    u8 bits;
  };

  PCIConfigSpaceAccessType m_config_access_type;

  PCIConfigType1Address m_pci_config_type1_address{};

  PCIConfigType2Address m_pci_config_type2_address{};
  u8 m_pci_config_type2_bus = 0;
};

template<typename T, typename... Args>
T* PCIPC::CreatePCIDevice(u32 pci_bus_number, u32 pci_device_number, const String& identifier, Args... args)
{
  T* component = new T(identifier, args...);
  component->SetPCILocation(pci_bus_number, pci_device_number);
  AddComponent(component);
  return component;
}

} // namespace Systems