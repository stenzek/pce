#include "pce/hw/pci_device.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "pce/hw/pci_bus.h"
Log_SetChannel(PCIDevice);

DEFINE_OBJECT_TYPE_INFO(PCIDevice);
BEGIN_OBJECT_PROPERTY_MAP(PCIDevice)
PROPERTY_TABLE_MEMBER_UINT("PCIBusNumber", 0, offsetof(PCIDevice, m_pci_bus_number), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("PCIDeviceNumber", 0, offsetof(PCIDevice, m_pci_device_number), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

PCIDevice::PCIDevice(const String& identifier, uint16 vendor_id, uint16 device_id, uint32 num_functions /* = 1 */,
                     const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_num_functions(num_functions)
{
  m_config_space.resize(num_functions);
  for (uint32 i = 0; i < num_functions; i++)
  {
    std::memset(m_config_space[i].dwords, 0, sizeof(m_config_space[i].dwords));
    m_config_space[i].dwords[0] = ZeroExtend32(vendor_id) | (ZeroExtend32(device_id) << 16);
  }
}

PCIDevice::~PCIDevice() = default;

void PCIDevice::SetLocation(u32 pci_bus_number, u32 pci_device_number)
{
  m_pci_bus_number = pci_bus_number;
  m_pci_device_number = pci_device_number;
}

PCIBus* PCIDevice::GetPCIBus() const
{
  return static_cast<PCIBus*>(m_bus);
}

bool PCIDevice::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  PCIBus* pci_bus = bus->SafeCast<PCIBus>();
  if (!pci_bus)
  {
    Log_ErrorPrintf("Attempting to initialize PCI device '%s' (%s) on non-PCI bus (%s)", m_identifier.GetCharArray(),
                    m_type_info->GetTypeName(), bus->GetTypeInfo()->GetTypeName());
    return false;
  }

  // Auto device number?
  if (m_pci_bus_number == 0xFFFFFFFFu || m_pci_device_number == 0xFFFFFFFFu)
  {
    if (!pci_bus->GetNextFreePCIDeviceNumber(&m_pci_bus_number, &m_pci_device_number))
    {
      Log_ErrorPrintf("No free PCI slots in bus for '%s' (%s)", m_identifier.GetCharArray(),
                      m_type_info->GetTypeName());
      return false;
    }
  }

  if (!pci_bus->AssignPCIDevice(m_pci_bus_number, m_pci_device_number, this))
  {
    Log_ErrorPrintf("Failed to assign PCI device '%s' (%s) to bus %u/device %u", m_identifier.GetCharArray(),
                    m_type_info->GetTypeName(), m_pci_bus_number, m_pci_device_number);
    return false;
  }

  return true;
}

void PCIDevice::Reset()
{
  // TODO: Reset the config space...
  for (uint32 i = 0; i < m_num_functions; i++)
  {
    for (uint32 j = 1; j < NUM_CONFIG_REGISTERS; j++)
      m_config_space[i].dwords[j] = 0;
  }
}

bool PCIDevice::LoadState(BinaryReader& reader)
{
  uint32 serialization_id;
  if (!reader.SafeReadUInt32(&serialization_id) || serialization_id != SERIALIZATION_ID)
    return false;

  uint32 num_functions;
  if (!reader.SafeReadUInt32(&num_functions) || num_functions != m_num_functions)
    return false;

  bool result = true;
  for (uint32 i = 0; i < m_num_functions; i++)
    result &= reader.SafeReadBytes(m_config_space[i].bytes, sizeof(m_config_space[i].bytes));

  return result;
}

bool PCIDevice::SaveState(BinaryWriter& writer)
{
  bool result = true;
  result &= writer.SafeWriteUInt32(SERIALIZATION_ID);
  result &= writer.SafeWriteUInt32(m_num_functions);

  for (uint32 i = 0; i < m_num_functions; i++)
    result &= writer.SafeWriteBytes(m_config_space[i].bytes, sizeof(m_config_space[i].bytes));

  return result;
}

uint8 PCIDevice::ReadConfigRegister(uint32 function, uint8 reg, uint8 index)
{
  if (function >= m_num_functions)
    return 0xFF;

  return HandleReadConfigRegister(function, reg * 4 + index);
}

void PCIDevice::WriteConfigRegister(uint32 function, uint8 reg, uint8 index, uint8 value)
{
  if (function >= m_num_functions)
    return;

  HandleWriteConfigRegister(function, reg * 4 + index, value);
}

uint8 PCIDevice::HandleReadConfigRegister(uint32 function, uint8 offset)
{
  return m_config_space[function].bytes[offset];
}

void PCIDevice::HandleWriteConfigRegister(uint32 function, uint8 offset, uint8 value)
{
  switch (offset)
  {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
      // Can't override VID/DID.
      break;

    case 0x04: // Command Register
      m_config_space[0].bytes[offset] = (value & 0x02) | 0x04;
      break;
    case 0x05:
      m_config_space[0].bytes[offset] = 0;
      break;

    case 0x06: // Status Register
      m_config_space[0].bytes[offset] = 0;
      break;
    case 0x07:
      m_config_space[0].bytes[offset] = 0x02;
      break;

    default:
      m_config_space[function].bytes[offset] = value;
      break;
  }
}
