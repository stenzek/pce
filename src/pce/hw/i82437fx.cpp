#include "pce/hw/i82437fx.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
Log_SetChannel(HW::i82437FX);

namespace HW {

i82437FX::i82437FX(Systems::PCIPC* system, Bus* bus) : PCIDevice(0x8086, 0x122D), m_system(system), m_bus(bus) {}

i82437FX::~i82437FX() {}

bool i82437FX::InitializePCIDevice(uint32 pci_bus_number, uint32 pci_device_number)
{
  if (!PCIDevice::InitializePCIDevice(pci_bus_number, pci_device_number))
    return false;

  return true;
}

void i82437FX::Reset()
{
  PCIDevice::Reset();

  // Default values from pcem.
  m_config_space[0].bytes[0x04] = 0x06;
  m_config_space[0].bytes[0x05] = 0x00;
  m_config_space[0].bytes[0x06] = 0x00;
  m_config_space[0].bytes[0x07] = 0x82;
  m_config_space[0].bytes[0x08] = 0x00;
  m_config_space[0].bytes[0x09] = 0x00;
  m_config_space[0].bytes[0x0A] = 0x00;
  m_config_space[0].bytes[0x0B] = 0x06;
  m_config_space[0].bytes[0x52] = 0x40;
  // m_config_space[0].bytes[0x53] = 0x14;
  // m_config_space[0].bytes[0x56] = 0x52;
  m_config_space[0].bytes[0x57] = 0x01;
  m_config_space[0].bytes[0x60] = 0x02;
  m_config_space[0].bytes[0x61] = 0x02;
  m_config_space[0].bytes[0x62] = 0x02;
  m_config_space[0].bytes[0x63] = 0x02;
  m_config_space[0].bytes[0x64] = 0x02;
  // m_config_space[0].bytes[0x67] = 0x11;
  // m_config_space[0].bytes[0x69] = 0x03;
  // m_config_space[0].bytes[0x70] = 0x20;
  m_config_space[0].bytes[0x72] = 0x02;
  // m_config_space[0].bytes[0x74] = 0x0E;
  // m_config_space[0].bytes[0x78] = 0x23;

  for (uint8 i = 0; i < NUM_PAM_REGISTERS; i++)
    UpdatePAMMapping(PAM_BASE_OFFSET + i);
}

bool i82437FX::LoadState(BinaryReader& reader)
{
  if (!PCIDevice::LoadState(reader))
    return false;

  for (uint8 i = 0; i < NUM_PAM_REGISTERS; i++)
    UpdatePAMMapping(PAM_BASE_OFFSET + i);

  return true;
}

bool i82437FX::SaveState(BinaryWriter& writer)
{
  return PCIDevice::SaveState(writer);
}

uint8 i82437FX::HandleReadConfigRegister(uint32 function, uint8 offset)
{
  return PCIDevice::HandleReadConfigRegister(function, offset);
}

void i82437FX::HandleWriteConfigRegister(uint32 function, uint8 offset, uint8 value)
{
  if (offset >= 0x10 && offset < 0x4F)
    return;

  Log_DevPrintf("i82437FX: 0x%08X 0x%02X", offset, value);

  switch (offset)
  {
    case 0x59: // PAM0
    case 0x5A: // PAM1
    case 0x5B: // PAM2
    case 0x5C: // PAM3
    case 0x5D: // PAM4
    case 0x5E: // PAM5
    case 0x5F: // PAM6
    {
      if (m_config_space[0].bytes[offset] != value)
      {
        m_config_space[0].bytes[offset] = value;
        UpdatePAMMapping(offset);
      }
    }
    break;

    default:
      PCIDevice::HandleWriteConfigRegister(function, offset, value);
      break;
  }
}

void i82437FX::SetPAMMapping(uint32 base, uint32 size, uint8 flag)
{
  const bool readable_memory = !!(flag & 1);
  const bool writable_memory = !!(flag & 2);

  Log_DevPrintf("Shadowing for 0x%08X-0x%08X - type %u, readable=%s, writable=%s", base, base + size - 1, flag,
                readable_memory ? "yes" : "no", writable_memory ? "yes" : "no");

  m_bus->SetPagesRAMState(base, size, readable_memory, writable_memory);
}

void i82437FX::UpdatePAMMapping(uint8 offset)
{
  const uint8 value = m_config_space[0].bytes[offset];

  switch (offset)
  {
    case 0x59:
      SetPAMMapping(0xF0000, 0x10000, value >> 4);
      break;
    case 0x5A:
      SetPAMMapping(0xC0000, 0x04000, value & 0x0F);
      SetPAMMapping(0xC4000, 0x04000, value >> 4);
      break;
    case 0x5B:
      SetPAMMapping(0xC8000, 0x04000, value & 0x0F);
      SetPAMMapping(0xCC000, 0x04000, value >> 4);
      break;
    case 0x5C:
      SetPAMMapping(0xD0000, 0x04000, value & 0x0F);
      SetPAMMapping(0xD4000, 0x04000, value >> 4);
      break;
    case 0x5D:
      SetPAMMapping(0xD8000, 0x04000, value & 0x0F);
      SetPAMMapping(0xDC000, 0x04000, value >> 4);
      break;
    case 0x5E:
      SetPAMMapping(0xE0000, 0x04000, value & 0x0F);
      SetPAMMapping(0xE4000, 0x04000, value >> 4);
      break;
    case 0x5F:
      SetPAMMapping(0xE8000, 0x04000, value & 0x0F);
      SetPAMMapping(0xEC000, 0x04000, value >> 4);
      break;
  }
}

} // namespace HW