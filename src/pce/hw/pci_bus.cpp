#include "pce/hw/pci_bus.h"
#include "YBaseLib/Log.h"
Log_SetChannel(PCIBus);

DEFINE_OBJECT_TYPE_INFO(PCIBus);

PCIBus::PCIBus(u32 memory_address_bits /* = 32 */, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(memory_address_bits, type_info)
{
}

PCIBus::~PCIBus() = default;

bool PCIBus::GetNextFreePCIDeviceNumber(u32* pci_bus_number, u32* pci_device_number) const
{
  for (u32 i = 0; i < NUM_PCI_BUSES; i++)
  {
    for (u32 j = 0; j < NUM_PCI_DEVICES_PER_BUS; j++)
    {
      if (!m_pci_devices[i][j])
      {
        *pci_bus_number = i;
        *pci_device_number = j;
        return true;
      }
    }
  }

  return false;
}

PCIDevice* PCIBus::GetPCIDevice(u32 pci_bus_number, u32 pci_device_number) const
{
  return (pci_bus_number < NUM_PCI_BUSES && pci_device_number < NUM_PCI_DEVICES_PER_BUS) ?
           m_pci_devices[pci_bus_number][pci_device_number] :
           nullptr;
}

bool PCIBus::AssignPCIDevice(u32 pci_bus_number, u32 pci_device_number, PCIDevice* device)
{
  if (pci_bus_number >= NUM_PCI_BUSES || pci_device_number >= NUM_PCI_DEVICES_PER_BUS)
    return false;

  Log_DevPrintf("Assigning device '%s' (%s) to bus number %u, device number %u", device->GetIdentifier().GetCharArray(),
                device->GetTypeInfo()->GetTypeName(), pci_bus_number, pci_device_number);
  m_pci_devices[pci_bus_number][pci_device_number] = device;
  return true;
}

bool PCIBus::Initialize(System* system)
{
  if (!BaseClass::Initialize(system))
    return false;

  return true;
}

void PCIBus::Reset()
{
  BaseClass::Reset();
}
