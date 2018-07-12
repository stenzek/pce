#include "pce/systems/pcipc.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "pce/bus.h"
#include "pce/mmio.h"
Log_SetChannel(Systems::PCIPC);

namespace Systems {

PCIPC::PCIPC(HostInterface* host_interface, PCIConfigSpaceAccessType config_access_type)
  : Systems::ISAPC(host_interface), m_config_access_type(config_access_type)
{
}

PCIPC::~PCIPC() {}

bool PCIPC::AddPCIDevice(PCIDevice* dev)
{
  // Find a free location
  for (uint32 bus = 0; bus < NUM_PCI_BUSES; bus++)
  {
    for (uint32 device = 0; device < NUM_PCI_DEVICES_PER_BUS; device++)
    {
      if (!m_pci_devices[bus][device])
        return AddPCIDeviceToLocation(dev, bus, device);
    }
  }

  return false;
}

bool PCIPC::AddPCIDeviceToLocation(PCIDevice* dev, uint32 bus_number, int32 device_number)
{
  if (bus_number >= NUM_PCI_BUSES || device_number >= NUM_PCI_DEVICES_PER_BUS ||
      m_pci_devices[bus_number][device_number] != nullptr)
  {
    return false;
  }

  m_pci_devices[bus_number][device_number] = dev;
  AddComponent(dev);
  return true;
}

bool PCIPC::Initialize()
{
  if (!ISAPC::Initialize())
    return false;

  ConnectPCIBusIOPorts();
  if (!InitializePCIDevices())
    return false;

  return true;
}

void PCIPC::Reset()
{
  m_pci_config_type1_address.bits = 0;
  m_pci_config_type2_bus = 0;
  m_pci_config_type2_address.bits = 0;
}

void PCIPC::ConnectPCIBusIOPorts()
{
  if (m_config_access_type == PCIConfigSpaceAccessType::Type1)
  {
    m_bus->ConnectIOPortReadDWord(0x0CF8, this,
                                  [this](uint32, uint32* value) { *value = m_pci_config_type1_address.bits; });
    m_bus->ConnectIOPortWriteDWord(0x0CF8, this,
                                   [this](uint32, uint32 value) { m_pci_config_type1_address.bits = value; });

    auto read_func =
      std::bind(&PCIPC::IOReadPCIType1ConfigDataByte, this, std::placeholders::_1, std::placeholders::_2);
    auto write_func =
      std::bind(&PCIPC::IOWritePCIType1ConfigDataByte, this, std::placeholders::_1, std::placeholders::_2);
    for (uint32 i = 0x0CFC; i <= 0x0CFF; i++)
    {
      m_bus->ConnectIOPortRead(i, this, read_func);
      m_bus->ConnectIOPortWrite(i, this, write_func);
    }
  }
  else if (m_config_access_type == PCIConfigSpaceAccessType::Type2)
  {
    m_bus->ConnectIOPortReadToPointer(0x0CF8, this, &m_pci_config_type2_address.bits);
    m_bus->ConnectIOPortWriteToPointer(0x0CF8, this, &m_pci_config_type2_address.bits);
    m_bus->ConnectIOPortReadToPointer(0x0CFA, this, &m_pci_config_type2_bus);
    m_bus->ConnectIOPortWriteToPointer(0x0CFA, this, &m_pci_config_type2_bus);

    // Accessors.
    auto read_func = std::bind(&PCIPC::IOReadPCIType2ConfigData, this, std::placeholders::_1, std::placeholders::_2);
    auto write_func = std::bind(&PCIPC::IOWritePCIType2ConfigData, this, std::placeholders::_1, std::placeholders::_2);
    for (uint32 i = 0; i < 0x1000; i++)
    {
      // Top 4 bits must be 1100, and bottom two bits must be 00.
      if ((i >> 12) != 0b1100)
        continue;

      m_bus->ConnectIOPortRead(i, this, read_func);
      m_bus->ConnectIOPortWrite(i, this, write_func);
    }
  }
}

bool PCIPC::InitializePCIDevices()
{
  for (uint32 bus = 0; bus < NUM_PCI_BUSES; bus++)
  {
    for (uint32 device = 0; device < NUM_PCI_DEVICES_PER_BUS; device++)
    {
      PCIDevice* dev = m_pci_devices[bus][device];
      if (dev && !dev->InitializePCIDevice(bus, device))
        return false;
    }
  }

  return true;
}

void PCIPC::IOReadPCIType1ConfigDataByte(uint32 port, uint8* value)
{
  if (!m_pci_config_type1_address.enable)
  {
    *value = 0xFF;
    return;
  }

  const uint8 function = m_pci_config_type1_address.function;
  const uint8 bus = m_pci_config_type1_address.bus;
  const uint8 device = m_pci_config_type1_address.device;
  const uint8 reg = m_pci_config_type1_address.reg;
  const uint8 idx = Truncate8(port & 3);

  if (bus >= NUM_PCI_BUSES || device >= NUM_PCI_DEVICES_PER_BUS || !m_pci_devices[bus][device])
  {
    Log_DevPrintf("Missing bus %u device %u function %u (%u/%u/%u)", bus, device, function, reg, idx, (reg * 4) + idx);
    *value = 0xFF;
    return;
  }

  *value = m_pci_devices[bus][device]->ReadConfigRegister(function, reg, idx);
}

void PCIPC::IOWritePCIType1ConfigDataByte(uint32 port, uint8 value)
{
  if (!m_pci_config_type1_address.enable)
    return;

  const uint8 function = m_pci_config_type1_address.function;
  const uint8 bus = m_pci_config_type1_address.bus;
  const uint8 device = m_pci_config_type1_address.device;
  const uint8 reg = m_pci_config_type1_address.reg;
  const uint8 idx = Truncate8(port & 3);

  if (bus >= NUM_PCI_BUSES || device >= NUM_PCI_DEVICES_PER_BUS || !m_pci_devices[bus][device])
  {
    Log_DevPrintf("Missing bus %u device %u function %u (%u/%u/%u) <- 0x%02X", bus, device, function, reg, idx,
                  (reg * 4) + idx, value);
    return;
  }

  m_pci_devices[bus][device]->WriteConfigRegister(function, reg, idx, value);
}

void PCIPC::IOReadPCIType2ConfigData(uint32 port, uint8* value)
{
  if (!m_pci_config_type2_address.key)
  {
    *value = 0xFF;
    return;
  }

  const uint8 function = m_pci_config_type2_address.function;
  const uint8 bus = m_pci_config_type2_bus;
  const uint8 device = Truncate8((port >> 8) & 15);
  const uint8 reg = Truncate8((port >> 2) & 63);
  const uint8 idx = Truncate8(port & 3);

  if (bus >= NUM_PCI_BUSES || device >= NUM_PCI_DEVICES_PER_BUS || !m_pci_devices[bus][device])
  {
    Log_DevPrintf("Missing bus %u device %u function %u (%u/%u/%u)", bus, device, function, reg, idx, (reg * 4) + idx);
    *value = 0xFF;
    return;
  }

  *value = m_pci_devices[bus][device]->ReadConfigRegister(function, reg, idx);
}

void PCIPC::IOWritePCIType2ConfigData(uint32 port, uint8 value)
{
  if (!m_pci_config_type2_address.key)
    return;

  const uint8 function = m_pci_config_type2_address.function;
  const uint8 bus = m_pci_config_type2_bus;
  const uint8 device = Truncate8((port >> 8) & 15);
  const uint8 reg = Truncate8((port >> 2) & 63);
  const uint8 idx = Truncate8(port & 3);

  if (bus >= NUM_PCI_BUSES || device >= NUM_PCI_DEVICES_PER_BUS || !m_pci_devices[bus][device])
  {
    Log_DevPrintf("Missing bus %u device %u function %u (%u/%u/%u) <- 0x%02X", bus, device, function, reg, idx,
                  (reg * 4) + idx, value);
    return;
  }

  m_pci_devices[bus][device]->WriteConfigRegister(function, reg, idx, value);
}

} // namespace Systems
