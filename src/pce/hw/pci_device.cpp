#include "pce/hw/pci_device.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "pce/hw/pci_bus.h"
Log_SetChannel(PCIDevice);

PCIDevice::PCIDevice(Component* parent_component, u8 num_functions)
  : m_parent_component(parent_component), m_num_pci_functions(num_functions)
{
  m_config_space.resize(num_functions);
  for (u8 i = 0; i < num_functions; i++)
    std::memset(m_config_space[i].dwords, 0, sizeof(m_config_space[i].dwords));
}

PCIDevice::~PCIDevice() = default;

void PCIDevice::SetPCILocation(u32 pci_bus_number, u32 pci_device_number)
{
  m_pci_bus_number = pci_bus_number;
  m_pci_device_number = pci_device_number;
}

PCIBus* PCIDevice::GetPCIBus() const
{
  return m_parent_component->GetBus()->SafeCast<PCIBus>();
}

bool PCIDevice::Initialize()
{
  PCIBus* pci_bus = m_parent_component->GetBus()->SafeCast<PCIBus>();
  if (!pci_bus)
  {
    Log_ErrorPrintf("Attempting to initialize PCI device '%s' (%s) on non-PCI bus (%s)",
                    m_parent_component->GetIdentifier().GetCharArray(),
                    m_parent_component->GetTypeInfo()->GetTypeName(),
                    m_parent_component->GetBus()->GetTypeInfo()->GetTypeName());
    return false;
  }

  // Auto device number?
  if (m_pci_bus_number == 0xFFFFFFFFu || m_pci_device_number == 0xFFFFFFFFu)
  {
    if (!pci_bus->GetNextFreePCIDeviceNumber(&m_pci_bus_number, &m_pci_device_number))
    {
      Log_ErrorPrintf("No free PCI slots in bus for '%s' (%s)", m_parent_component->GetIdentifier().GetCharArray(),
                      m_parent_component->GetTypeInfo()->GetTypeName());
      return false;
    }
  }

  if (!pci_bus->AssignPCIDevice(m_pci_bus_number, m_pci_device_number, this))
  {
    Log_ErrorPrintf("Failed to assign PCI device '%s' (%s) to bus %u/device %u",
                    m_parent_component->GetIdentifier().GetCharArray(),
                    m_parent_component->GetTypeInfo()->GetTypeName(), m_pci_bus_number, m_pci_device_number);
    return false;
  }

  return true;
}

void PCIDevice::Reset()
{
  for (u8 i = 0; i < m_num_pci_functions; i++)
    ResetConfigSpace(i);
}

bool PCIDevice::LoadState(BinaryReader& reader)
{
  u32 serialization_id;
  if (!reader.SafeReadUInt32(&serialization_id) || serialization_id != SERIALIZATION_ID)
    return false;

  u32 num_functions;
  if (!reader.SafeReadUInt32(&num_functions) || num_functions != m_num_pci_functions)
    return false;

  bool result = true;
  for (u8 i = 0; i < m_num_pci_functions; i++)
  {
    result &= reader.SafeReadBytes(m_config_space[i].bytes, sizeof(m_config_space[i].bytes));
    if (result)
    {
      const bool io_active = m_config_space[i].header.command.enable_io_space;
      const bool memory_active = m_config_space[i].header.command.enable_memory_space;
      for (u32 j = 0; j < NumMemoryRegions; j++)
      {
        if (m_config_space[i].memory_regions[j].size > 0)
        {
          OnMemoryRegionChanged(i, static_cast<MemoryRegion>(j),
                                m_config_space[i].memory_regions[j].is_io ? io_active : memory_active);
        }
      }
    }
  }

  return result;
}

bool PCIDevice::SaveState(BinaryWriter& writer)
{
  bool result = true;
  result &= writer.SafeWriteUInt32(SERIALIZATION_ID);
  result &= writer.SafeWriteUInt32(m_num_pci_functions);

  for (u32 i = 0; i < m_num_pci_functions; i++)
    result &= writer.SafeWriteBytes(m_config_space[i].bytes, sizeof(m_config_space[i].bytes));

  return result;
}

void PCIDevice::InitPCIID(u8 function, u16 vendor_id, u16 device_id)
{
  DebugAssert(function < m_num_pci_functions);
  m_config_space[function].dwords[0] = ZeroExtend32(vendor_id) | (ZeroExtend32(device_id) << 16);
}

void PCIDevice::InitPCIClass(u8 function, u8 class_code, u8 subclass_code, u8 prog_if, u8 rev_id)
{
  DebugAssert(function < m_num_pci_functions);
  m_config_space[function].dwords[2] = (ZeroExtend32(rev_id) | (ZeroExtend32(prog_if) << 8) |
                                        (ZeroExtend32(subclass_code) << 16) | (ZeroExtend32(class_code) << 24));
}

void PCIDevice::InitPCIMemoryRegion(u8 function, MemoryRegion region, PhysicalMemoryAddress default_address, u32 size,
                                    bool io, bool prefetchable)
{
  DebugAssert(function < m_num_pci_functions);
  DebugAssert(!io || region != MemoryRegion_ExpansionROM);
  DebugAssert(Common::IsPow2(size));

  m_config_space[function].memory_regions[region].default_address = default_address;
  m_config_space[function].memory_regions[region].size = size;
  m_config_space[function].memory_regions[region].is_io = io;
  m_config_space[function].memory_regions[region].is_prefetchable = prefetchable;
  SetDefaultAddressForRegion(function, region);
}

u8 PCIDevice::GetConfigSpaceByte(u8 function, u8 byte_offset) const
{
  DebugAssert(function < m_num_pci_functions);
  return m_config_space[function].bytes[byte_offset];
}

u16 PCIDevice::GetConfigSpaceWord(u8 function, u8 byte_offset) const
{
  DebugAssert(function < m_num_pci_functions);
  return m_config_space[function].words[byte_offset / 2];
}

u32 PCIDevice::GetConfigSpaceDWord(u8 function, u8 byte_offset) const
{
  DebugAssert(function < m_num_pci_functions);
  return m_config_space[function].dwords[byte_offset / 4];
}

void PCIDevice::SetConfigSpaceByte(u8 function, u8 byte_offset, u8 value)
{
  DebugAssert(function < m_num_pci_functions);
  m_config_space[function].bytes[byte_offset] = value;
}

void PCIDevice::SetConfigSpaceWord(u8 function, u8 byte_offset, u16 value)
{
  DebugAssert(function < m_num_pci_functions);
  m_config_space[function].words[byte_offset / 2] = value;
}

void PCIDevice::SetConfigSpaceDWord(u8 function, u8 byte_offset, u32 value)
{
  DebugAssert(function < m_num_pci_functions);
  m_config_space[function].dwords[byte_offset / 4] = value;
}

PhysicalMemoryAddress PCIDevice::GetMemoryRegionBaseAddress(u8 function, MemoryRegion region) const
{
  DebugAssert(function < m_num_pci_functions);

  const auto& cs = m_config_space[function];
  const auto& mr = cs.memory_regions[region];

  const u32 base = (region == MemoryRegion_ExpansionROM) ? (0x30 / 4) : ((0x10 / 4) + static_cast<u32>(region));
  if (!mr.is_io)
    return cs.dwords[base] & UINT32_C(0xFFFFFFF0);
  else
    return cs.dwords[base] & UINT32_C(0xFFFFFFFC);
}

bool PCIDevice::IsPCIMemoryActive(u8 function) const
{
  DebugAssert(function < m_num_pci_functions);
  return m_config_space[function].header.command.enable_memory_space;
}

bool PCIDevice::IsPCIIOActive(u8 function) const
{
  DebugAssert(function < m_num_pci_functions);
  return m_config_space[function].header.command.enable_io_space;
}

bool PCIDevice::IsPCIExpansionROMActive(u8 function) const
{
  DebugAssert(function < m_num_pci_functions);
  return (m_config_space[function].header.rom_base_address & u32(0x01)) != 0;
}

void PCIDevice::ResetConfigSpace(u8 function)
{
  auto& cs = m_config_space[function];

  // Don't trample over the read-only fields.
  for (u32 byte_offset = 0x04; byte_offset < 0x08; byte_offset++)
    cs.bytes[byte_offset] = 0;
  for (u32 byte_offset = 0x10; byte_offset < 0x2C; byte_offset++)
    cs.bytes[byte_offset] = 0;
  for (u32 byte_offset = 0x30; byte_offset < 0x34; byte_offset++)
    cs.bytes[byte_offset] = 0;
  for (u32 byte_offset = 0x40; byte_offset < countof(cs.bytes); byte_offset++)
    cs.bytes[byte_offset] = 0;

  // Disable memory and IO decoding?
  cs.header.command.enable_io_space = false;
  cs.header.command.enable_memory_space = false;

  for (u8 j = 0; j < NumMemoryRegions; j++)
  {
    const auto& mr = cs.memory_regions[j];
    if (mr.size > 0)
    {
      SetDefaultAddressForRegion(function, static_cast<MemoryRegion>(j));
      OnMemoryRegionChanged(function, static_cast<MemoryRegion>(j), false);
    }
  }
}

void PCIDevice::SetDefaultAddressForRegion(u8 function, MemoryRegion region)
{
  auto& cs = m_config_space[function];
  const auto& mr = cs.memory_regions[region];
  const u32 base = (region == MemoryRegion_ExpansionROM) ? (0x30 / 4) : ((0x10 / 4) + static_cast<u32>(region));
  if (region == MemoryRegion_ExpansionROM)
    cs.dwords[base] = (mr.default_address & UINT32_C(0xFFFFF800));
  else if (!mr.is_io)
    cs.dwords[base] = (mr.default_address & UINT32_C(0xFFFFFFF0)) | BoolToUInt32(mr.is_prefetchable);
  else
    cs.dwords[base] = (mr.default_address & UINT32_C(0xFFFFFFFC)) | 0x01;
}

void PCIDevice::OnCommandRegisterChanged(u8 function) {}

void PCIDevice::OnMemoryRegionChanged(u8 function, MemoryRegion region, bool active) {}

u8 PCIDevice::ReadConfigSpace(u8 function, u8 offset)
{
  DebugAssert(function < m_num_pci_functions);
  return m_config_space[function].bytes[offset];
}

void PCIDevice::WriteConfigSpace(u8 function, u8 offset, u8 value)
{
  // TODO: Perhaps make this DWORD-based...
  DebugAssert(function < m_num_pci_functions);
  auto& cs = m_config_space[function];
  const u8 old_value = cs.bytes[offset];
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
    {
      cs.bytes[offset] = value;
      OnCommandRegisterChanged(function);

      // Least significant bits of command register control memory/IO decoding.
      if ((value & 0x01) != (old_value & 0x01))
      {
        // I/O changed.
        const bool active = m_config_space[function].header.command.enable_io_space;
        for (u8 i = 0; i < NumMemoryRegions; i++)
        {
          if (m_config_space[function].memory_regions[i].size > 0 && m_config_space[function].memory_regions[i].is_io)
            OnMemoryRegionChanged(function, static_cast<MemoryRegion>(i), active);
        }
      }
      if ((value & 0x02) != (old_value & 0x02))
      {
        // Memory changed.
        const bool active = m_config_space[function].header.command.enable_memory_space;
        for (u8 i = 0; i < NumMemoryRegions; i++)
        {
          if (m_config_space[function].memory_regions[i].size > 0 && !m_config_space[function].memory_regions[i].is_io)
            OnMemoryRegionChanged(function, static_cast<MemoryRegion>(i), active);
        }
      }
    }
    break;
    case 0x05:
      cs.bytes[offset] = value;
      OnCommandRegisterChanged(function);
      break;

    case 0x06: // Status Register
      cs.bytes[offset] = value;
      break;
    case 0x07:
      cs.bytes[offset] = value;
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
    {
      // Memory region registers.
      const MemoryRegion region = static_cast<MemoryRegion>((offset - 0x10) / 4);
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
        cs.dwords[offset / 4] = ((base_address & UINT32_C(0xFFFFFFF0)) | (cs.dwords[offset / 4] & 0xF));

        // Last byte? Call the update handler.
        if ((offset & 3) == 3 && IsPCIMemoryActive(function))
        {
          Log_DevPrintf("PCI BAR %u changed to memory 0x%08X", static_cast<u32>(region),
                        GetMemoryRegionBaseAddress(function, region));
          OnMemoryRegionChanged(function, region, true);
        }
      }
      else
      {
        // I/O space.
        cs.bytes[offset] = ((offset & 3) == 0) ? ((cs.bytes[offset] & 0x03) | (value & 0xFC)) : (value);
        PhysicalMemoryAddress base_address = cs.dwords[offset / 4] & UINT32_C(0xFFFFFFFC);
        base_address = Common::AlignDown(base_address, mr.size);
        cs.dwords[offset / 4] = ((base_address & UINT32_C(0xFFFFFFFC)) | (cs.dwords[offset / 4] & 0x3));
        if ((offset & 3) == 3 && IsPCIIOActive(function))
        {
          Log_DevPrintf("PCI BAR %u changed to I/O 0x%08X", static_cast<u32>(region),
                        GetMemoryRegionBaseAddress(function, region));

          OnMemoryRegionChanged(function, region, true);
        }
      }
    }
    break;

    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    {
      // Expansion ROM memory region.
      const auto& mr = cs.memory_regions[MemoryRegion_ExpansionROM];
      if (mr.size == 0)
        return;

      DebugAssert(!mr.is_io);

      // bit 0 - enable, bit 1..10 - reserved, bit 11..31 address
      const u32 old_dword = cs.dwords[0x30 / 4];
      cs.bytes[offset] = value;

      // only allow setting address bits
      u32 new_dword = (old_dword & UINT32_C(0x000007FE)) | (cs.dwords[0x30 / 4] & UINT32_C(0xFFFFF801));

      // Mask away the bits, so that the address is aligned to the size.
      PhysicalMemoryAddress base_address = new_dword & UINT32_C(0xFFFFF800);
      base_address = Common::AlignDown(base_address, mr.size);

      new_dword = base_address | (new_dword & UINT32_C(0x01));
      cs.dwords[0x30 / 4] = new_dword;

      // changed?
      if (offset == 0x33)
        OnMemoryRegionChanged(function, MemoryRegion_ExpansionROM, IsPCIExpansionROMActive(function));
    }
    break;

    default:
      Log_DebugPrintf("Unknown PCI config space write for '%s': %02X <- %02X",
                      m_parent_component->GetIdentifier().GetCharArray(), offset, value);
      cs.bytes[offset] = value;
      break;
  }
}
