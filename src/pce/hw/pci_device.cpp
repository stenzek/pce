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

PCIDevice::PCIDevice(const String& identifier, u8 num_functions /* = 1 */,
                     const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_num_functions(num_functions)
{
  m_config_space.resize(num_functions);
  for (u8 i = 0; i < num_functions; i++)
    std::memset(m_config_space[i].dwords, 0, sizeof(m_config_space[i].dwords));
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
  for (u8 i = 0; i < m_num_functions; i++)
  {
    auto& cs = m_config_space[i];
    for (u8 j = 0; j < NumMemoryRegions; j++)
    {
      const auto& mr = cs.memory_regions[j];
      if (mr.size > 0)
      {
        const u32 base = (j == MemoryRegion_ExpansionROM) ? (0x30 / 4) : ((0x10 / 4) + static_cast<u32>(j));
        if (!mr.is_io)
          cs.dwords[base] = (mr.default_address & UINT32_C(0xFFFFFFF0) | (cs.dwords[base] & 0xF));
        else
          cs.dwords[base] = (mr.default_address & UINT32_C(0xFFFFFFFC) | (cs.dwords[base] & 0x3));

        OnMemoryRegionChanged(i, static_cast<MemoryRegion>(j));
      }
    }
  }
}

bool PCIDevice::LoadState(BinaryReader& reader)
{
  u32 serialization_id;
  if (!reader.SafeReadUInt32(&serialization_id) || serialization_id != SERIALIZATION_ID)
    return false;

  u32 num_functions;
  if (!reader.SafeReadUInt32(&num_functions) || num_functions != m_num_functions)
    return false;

  bool result = true;
  for (u8 i = 0; i < m_num_functions; i++)
  {
    result &= reader.SafeReadBytes(m_config_space[i].bytes, sizeof(m_config_space[i].bytes));
    if (result)
    {
      for (u32 j = 0; j < NumMemoryRegions; j++)
      {
        if (m_config_space[i].memory_regions[j].size > 0)
          OnMemoryRegionChanged(i, static_cast<MemoryRegion>(j));
      }
    }
  }

  return result;
}

bool PCIDevice::SaveState(BinaryWriter& writer)
{
  bool result = true;
  result &= writer.SafeWriteUInt32(SERIALIZATION_ID);
  result &= writer.SafeWriteUInt32(m_num_functions);

  for (u32 i = 0; i < m_num_functions; i++)
    result &= writer.SafeWriteBytes(m_config_space[i].bytes, sizeof(m_config_space[i].bytes));

  return result;
}

void PCIDevice::InitPCIID(u8 function, u16 vendor_id, u16 device_id)
{
  DebugAssert(function < m_num_functions);
  m_config_space[function].dwords[0] = ZeroExtend32(vendor_id) | (ZeroExtend32(device_id) << 16);
}

void PCIDevice::InitPCIClass(u8 function, u8 class_code, u8 subclass_code, u8 prog_if, u8 rev_id)
{
  DebugAssert(function < m_num_functions);
  m_config_space[function].dwords[2] = (ZeroExtend32(rev_id) | (ZeroExtend32(prog_if) << 8) |
                                        (ZeroExtend32(subclass_code) << 16) | (ZeroExtend32(class_code) << 24));
}

void PCIDevice::InitPCIMemoryRegion(u8 function, MemoryRegion region, PhysicalMemoryAddress default_address, u32 size,
                                    bool io, bool prefetchable)
{
  DebugAssert(function < m_num_functions);
  DebugAssert(!io || region != MemoryRegion_ExpansionROM);

  m_config_space[function].memory_regions[region].default_address = default_address;
  m_config_space[function].memory_regions[region].size = size;
  m_config_space[function].memory_regions[region].is_io = io;
  m_config_space[function].memory_regions[region].is_prefetchable = prefetchable;

  const u32 base = (region == MemoryRegion_ExpansionROM) ? 0x30 : (0x10 + (static_cast<u32>(region) * 4));
  if (!io)
  {
    m_config_space[function].dwords[base / 4] =
      (default_address & UINT32_C(0xFFFFFFF0)) | (BoolToUInt32(prefetchable) << 3);
  }
  else
  {
    m_config_space[function].dwords[base / 4] = (default_address & UINT32_C(0xFFFFFFFC) | 0x01);
  }
}

u8 PCIDevice::GetConfigSpaceByte(u8 function, u8 byte_offset) const
{
  DebugAssert(function < m_num_functions);
  return m_config_space[function].bytes[byte_offset];
}

u16 PCIDevice::GetConfigSpaceWord(u8 function, u8 byte_offset) const
{
  DebugAssert(function < m_num_functions);
  return m_config_space[function].words[byte_offset / 2];
}

u32 PCIDevice::GetConfigSpaceDWord(u8 function, u8 byte_offset) const
{
  DebugAssert(function < m_num_functions);
  return m_config_space[function].dwords[byte_offset / 4];
}

void PCIDevice::SetConfigSpaceByte(u8 function, u8 byte_offset, u8 value)
{
  DebugAssert(function < m_num_functions);
  m_config_space[function].bytes[byte_offset] = value;
}

void PCIDevice::SetConfigSpaceWord(u8 function, u8 byte_offset, u16 value)
{
  DebugAssert(function < m_num_functions);
  m_config_space[function].words[byte_offset / 2] = value;
}

void PCIDevice::SetConfigSpaceDWord(u8 function, u8 byte_offset, u32 value)
{
  DebugAssert(function < m_num_functions);
  m_config_space[function].dwords[byte_offset / 4] = value;
}

PhysicalMemoryAddress PCIDevice::GetMemoryRegionBaseAddress(u8 function, MemoryRegion region) const
{
  DebugAssert(function < m_num_functions);

  const auto& cs = m_config_space[function];
  const auto& mr = cs.memory_regions[region];

  const u32 base = (region == MemoryRegion_ExpansionROM) ? (0x30 / 4) : ((0x10 / 4) + static_cast<u32>(region));
  if (!mr.is_io)
    return cs.dwords[base] & UINT32_C(0xFFFFFFF0);
  else
    return cs.dwords[base] & UINT32_C(0xFFFFFFFC);
}

void PCIDevice::OnCommandRegisterChanged(u8 function) {}

void PCIDevice::OnMemoryRegionChanged(u8 function, MemoryRegion region) {}

u8 PCIDevice::ReadConfigSpace(u8 function, u8 offset)
{
  DebugAssert(function < m_num_functions);
  return m_config_space[function].bytes[offset];
}

void PCIDevice::WriteConfigSpace(u8 function, u8 offset, u8 value)
{
  // TODO: Perhaps make this DWORD-based...
  DebugAssert(function < m_num_functions);
  auto& cs = m_config_space[function];
  switch (offset)
  {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D:
    case 0x0E:
    case 0x0F:
    case 0x2C:
    case 0x2D:
    case 0x2E:
    case 0x2F:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
    case 0x3A:
    case 0x3B:
    case 0x3E:
    case 0x3F:
      // Can't override VID/DID/Class/Rev.
      break;

    case 0x04: // Command Register
      cs.bytes[offset] = (value & 0x02) | 0x04;
      OnCommandRegisterChanged(function);
      break;
    case 0x05:
      cs.bytes[offset] = 0;
      OnCommandRegisterChanged(function);
      break;

    case 0x06: // Status Register
      cs.bytes[offset] = 0;
      break;
    case 0x07:
      cs.bytes[offset] = 0x02;
      break;

    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    {
      // Memory region registers.
      const MemoryRegion region =
        (offset >= 0x30) ? MemoryRegion_ExpansionROM : static_cast<MemoryRegion>((offset - 0x10) / 4);
      const auto& mr = cs.memory_regions[region];
      if (mr.size == 0)
      {
        // No memory region exists at this BAR.
        return;
      }

      if (!mr.is_io)
      {
        // Memory space.
        cs.bytes[offset] = ((offset & 3) == 0) ? ((cs.bytes[offset] & 0x0F) | (value & 0xF0)) : (value);

        // Mask away the bits, so that the address is aligned to the size.
        PhysicalMemoryAddress base_address = cs.dwords[offset / 4] & UINT32_C(0xFFFFFFF0);
        base_address = Common::AlignDown(base_address, mr.size);
        cs.dwords[offset / 4] = ((base_address & UINT32_C(0xFFFFFFF0)) | (cs.dwords[offset / 4] & 0x3));
      }
      else
      {
        // I/O space.
        cs.bytes[offset] = ((offset & 3) == 0) ? ((cs.bytes[offset] & 0x03) | (value & 0xFC)) : (value);
        PhysicalMemoryAddress base_address = cs.dwords[offset / 4] & UINT32_C(0xFFFFFFFC);
        base_address = Common::AlignDown(base_address, mr.size);
        cs.dwords[offset / 4] = ((base_address & UINT32_C(0xFFFFFFFC)) | (cs.dwords[offset / 4] & 0xF));
      }
    }
    break;

    default:
      Log_DevPrintf("Unknown PCI config space write for '%s': %02X <- %02X", m_identifier.GetCharArray(), offset,
                    value);
      cs.bytes[offset] = value;
      break;
  }
}
