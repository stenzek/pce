#ifdef ENABLE_VOODOO
#include "voodoo.h"
#include "../bus.h"
#include "../host_interface.h"
#include "../mmio.h"
#include "../thirdparty/mame/voodoo.h"
#include "YBaseLib/Log.h"
#include "common/display.h"
#include "common/timing.h"
Log_SetChannel(Voodoo);

namespace HW {

DEFINE_OBJECT_TYPE_INFO(Voodoo);
DEFINE_GENERIC_COMPONENT_FACTORY(Voodoo);
BEGIN_OBJECT_PROPERTY_MAP(Voodoo)
PROPERTY_TABLE_MEMBER_UINT("Type", 0, offsetof(Voodoo, m_type), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("FBMemoryMB", 0, offsetof(Voodoo, m_fb_mem_size), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("TMUMemoryMB", 0, offsetof(Voodoo, m_tmu_mem_size), nullptr, 0)
PROPERTY_TABLE_MEMBER_BOOL("PrimaryDisplay", 0, offsetof(Voodoo, m_primary_display), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

Voodoo::Voodoo(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, 1, type_info)
{
}

Voodoo::~Voodoo() {}

bool Voodoo::Initialize(System* system, Bus* bus)
{
  // PCI Init
  u8 type;
  u32 freq;
  switch (m_type)
  {
    case Type::Voodoo1:
      InitPCIID(0, 0x121A, 0x0001);
      InitPCIClass(0, 0x04, 0x00, 0x00, 0x00);
      InitPCIMemoryRegion(0, PCIDevice::MemoryRegion_BAR0, DEFAULT_MEMORY_REGION_ADDRESS, MEMORY_REGION_SIZE, false,
                          false);
      type = TYPE_VOODOO_1;
      freq = (m_clock_frequency != 0) ? m_clock_frequency : STD_VOODOO_1_CLOCK;
      break;
    case Type::Voodoo2:
      InitPCIID(0, 0x121A, 0x0002);
      InitPCIClass(0, 0x04, 0x80, 0x00, 0x00);
      InitPCIMemoryRegion(0, PCIDevice::MemoryRegion_BAR0, DEFAULT_MEMORY_REGION_ADDRESS, MEMORY_REGION_SIZE, false,
                          false);
      m_config_space[0].bytes[0x40] = 0x00;
      m_config_space[0].bytes[0x41] = 0x40; // Voodoo2 revision
      m_config_space[0].bytes[0x42] = 0x01;
      m_config_space[0].bytes[0x43] = 0x00;
      type = TYPE_VOODOO_2;
      freq = (m_clock_frequency != 0) ? m_clock_frequency : STD_VOODOO_2_CLOCK;
      break;
    default:
      Panic("Unknown voodoo card");
      return false;
  }

  if (!BaseClass::Initialize(system, bus))
    return false;

  m_display = system->GetHostInterface()->CreateDisplay(
    SmallString::FromFormat("%s (Voodoo)", m_identifier.GetCharArray()),
    m_primary_display ? Display::Type::Primary : Display::Type::Secondary, Display::DEFAULT_PRIORITY + 1);
  if (!m_display)
    return false;

  // Ensure the display is initially inactive.
  m_display->SetEnable(false);

  m_device = std::make_unique<voodoo_device>(freq, type);
  m_device->initialize(bus, system->GetTimingManager(), m_display.get());
  return true;
}

void Voodoo::Reset()
{
  BaseClass::Reset();

  // TODO:
  m_device->reset();
}

bool Voodoo::LoadState(BinaryReader& reader)
{
  return BaseClass::LoadState(reader);
}

bool Voodoo::SaveState(BinaryWriter& writer)
{
  return BaseClass::SaveState(writer);
}

u8 Voodoo::ReadConfigSpace(u8 function, u8 offset)
{
  if (function >= 1)
    return 0xFF;

  u8 value;
  switch (offset)
  {
    case 0x40: // initEnable
      return m_config_space[0].bytes[0x40];
    case 0x41:
      return (((m_type >= Type::Voodoo2) ? 0x50 : 0x00) | (m_config_space[0].bytes[0x41] & 0x0F));
    case 0x42:
      return m_config_space[0].bytes[0x42];
    case 0x43:
      return m_config_space[0].bytes[0x43];

    case 0x4C: // cfgStatus
    case 0x4D:
    case 0x4E:
    case 0x4F:
    {
      // Return the same value as reading the memory-mapped status register.
      value = Truncate8(m_device->voodoo_r(0) >> ((offset - 0x4C) * 8));
    }
    break;

    case 0x54: // siProcess
      value = 0x02;
      break;
    case 0x55:
      value = 0x60;
      break;
    case 0x56:
      value = 0x00;
      break;
    case 0x57:
      value = 0x00;
      break;

    default:
      value = BaseClass::ReadConfigSpace(function, offset);
      break;
  }

  Log_DebugPrintf("Voodoo config space %02X -> %02X", offset, value);
  return value;
}

void Voodoo::WriteConfigSpace(u8 function, u8 offset, u8 value)
{
  if (function >= 1)
    return;

  Log_DebugPrintf("Voodoo config space %02X <- %02X", offset, value);
  switch (offset)
  {
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
      m_config_space[0].bytes[offset] = value;
      Log_DebugPrintf("voodoo pcienable %08X", m_config_space[0].dwords[0x40 / 4]);
      m_device->voodoo_set_init_enable(m_config_space[0].dwords[0x40 / 4]);
      break;

    case 0x4C: // cfgStatus
    case 0x4D:
    case 0x4E:
    case 0x4F:
      break;

    case 0xc0:
      Log_DebugPrintf("voodoo clock enable");
      // voodoo_set_clock_enable(v, true);
      break;

    case 0xe0:
      Log_DebugPrintf("voodoo clock disable");
      // voodoo_set_clock_enable(v, false);
      break;

    default:
      BaseClass::WriteConfigSpace(function, offset, value);
      break;
  }
}

void Voodoo::OnMemoryRegionChanged(u8 function, MemoryRegion region, bool active)
{
  BaseClass::OnMemoryRegionChanged(function, region, active);

  switch (region)
  {
    case PCIDevice::MemoryRegion_BAR0:
    {
      if (active)
      {
        PhysicalMemoryAddress base_address = GetMemoryRegionBaseAddress(function, region);
        Log_DebugPrintf("Voodoo base address: %08X", base_address);

        if (m_mmio_mapping)
        {
          // Unchanged?
          if (m_mmio_mapping->GetStartAddress() == base_address)
            return;

          // Changed.
          m_bus->DisconnectMMIO(m_mmio_mapping);
          m_mmio_mapping->Release();
        }

        // 32-bit MMIO. TODO: Handle other widths.
        static constexpr u32 OFFSET_MASK = (MEMORY_REGION_SIZE / 4) - 1;
        MMIO::Handlers handlers;
        handlers.read_byte = std::bind(&Voodoo::HandleBusByteRead, this, std::placeholders::_1, std::placeholders::_2);
        handlers.read_word = std::bind(&Voodoo::HandleBusWordRead, this, std::placeholders::_1, std::placeholders::_2);
        handlers.read_dword =
          std::bind(&Voodoo::HandleBusDWordRead, this, std::placeholders::_1, std::placeholders::_2);
        handlers.write_byte =
          std::bind(&Voodoo::HandleBusByteWrite, this, std::placeholders::_1, std::placeholders::_2);
        handlers.write_word =
          std::bind(&Voodoo::HandleBusWordWrite, this, std::placeholders::_1, std::placeholders::_2);
        handlers.write_dword =
          std::bind(&Voodoo::HandleBusDWordWrite, this, std::placeholders::_1, std::placeholders::_2);
        m_mmio_mapping = MMIO::CreateComplex(base_address, MEMORY_REGION_SIZE, std::move(handlers), false);
        m_bus->ConnectMMIO(m_mmio_mapping);
      }
      else
      {
        Log_DebugPrintf("Voodoo unmapped");
        if (m_mmio_mapping)
        {
          m_bus->DisconnectMMIO(m_mmio_mapping);
          m_mmio_mapping->Release();
          m_mmio_mapping = nullptr;
        }
      }
    }
    break;

    default:
      break;
  }
}

void Voodoo::HandleBusByteRead(u32 offset, u8* val)
{
  const u32 dwval = m_device->voodoo_r((offset >> 2) & MEMORY_REGION_DWORD_MASK);
  *val = Truncate8(dwval >> ((offset & 3) * 8));
}

void Voodoo::HandleBusByteWrite(u32 offset, u8 val)
{
  m_device->voodoo_w((offset >> 2) & MEMORY_REGION_DWORD_MASK, ZeroExtend32(val),
                     UINT32_C(0x000000FF) << ((offset & 3) * 8));
}

void Voodoo::HandleBusWordRead(u32 offset, u16* val)
{
  if ((offset & 1) == 0)
  {
    const u32 dwval = m_device->voodoo_r((offset >> 2) & MEMORY_REGION_DWORD_MASK);
    *val = ((offset & 3) == 0) ? Truncate16(dwval) : Truncate16(dwval >> 16);
  }
  else
  {
    // byte access unsupported?
    Log_WarningPrintf("unaligned word access %08X", offset);
    *val = UINT16_C(0xFFFF);
  }
}

void Voodoo::HandleBusWordWrite(u32 offset, u16 val)
{
  if ((offset & 3) == 0)
    m_device->voodoo_w((offset >> 2) & MEMORY_REGION_DWORD_MASK, ZeroExtend32(val), UINT32_C(0x0000FFFF));
  else
    m_device->voodoo_w((offset >> 2) & MEMORY_REGION_DWORD_MASK, ZeroExtend32(val) << 16, UINT32_C(0xFFFF0000));
}

void Voodoo::HandleBusDWordRead(u32 offset, u32* val)
{
  if ((offset & 3) == 0)
  {
    // aligned access.
    *val = m_device->voodoo_r((offset >> 2) & MEMORY_REGION_DWORD_MASK);
  }
  else if ((offset & 1) == 0)
  {
    // word-aligned access.
    const u32 low = m_device->voodoo_r((offset >> 2) & MEMORY_REGION_DWORD_MASK);
    const u32 high = m_device->voodoo_r(((offset >> 2) + 1) & MEMORY_REGION_DWORD_MASK);
    *val = (low >> 16) | (high << 16);
  }
  else
  {
    // byte access unsupported?
    Log_WarningPrintf("unaligned access %08X", offset);
    *val = UINT32_C(0xFFFFFFFF);
  }
}

void Voodoo::HandleBusDWordWrite(u32 offset, u32 val)
{
  if ((offset & 3) == 0)
  {
    // dword-aligned access
    m_device->voodoo_w((offset >> 2) & MEMORY_REGION_DWORD_MASK, val, UINT32_C(0xFFFFFFFF));
    return;
  }
  else if ((offset & 1) == 0)
  {
    // word-aligned access
    m_device->voodoo_w((offset >> 2) & MEMORY_REGION_DWORD_MASK, val << 16, UINT32_C(0xFFFF0000));
    m_device->voodoo_w(((offset >> 2) + 1) & MEMORY_REGION_DWORD_MASK, val >> 16, UINT32_C(0x0000FFFF));
  }
  else
  {
    u32 low = m_device->voodoo_r((offset >> 2) & MEMORY_REGION_DWORD_MASK);
    u32 high = m_device->voodoo_r(((offset >> 2) + 1) & MEMORY_REGION_DWORD_MASK);
    if ((offset & 3) == 1)
    {
      low = (low & UINT32_C(0x00FFFFFF)) | (val << 24);
      high = (high & UINT32_C(0xFF000000)) | (val >> 8);
    }
    else if ((offset & 3) == 3)
    {
      low = (low & UINT32_C(0xFF)) | ((val & 0xFFFFFF) << 8);
      high = (high & UINT32_C(0xFFFFFF00)) | (val >> 24);
    }
    m_device->voodoo_w((offset >> 2) & MEMORY_REGION_DWORD_MASK, low, UINT32_C(0xFFFFFFFF));
    m_device->voodoo_w(((offset >> 2) + 1) & MEMORY_REGION_DWORD_MASK, high, UINT32_C(0xFFFFFFFF));
  }
}

} // namespace HW
#endif