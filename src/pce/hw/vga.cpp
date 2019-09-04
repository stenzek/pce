#include "pce/hw/vga.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "common/display.h"
#include "pce/bus.h"
#include "pce/mmio.h"
#include "pce/system.h"
Log_SetChannel(HW::VGA);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(VGA);
DEFINE_GENERIC_COMPONENT_FACTORY(VGA);
BEGIN_OBJECT_PROPERTY_MAP(VGA)
PROPERTY_TABLE_MEMBER_STRING("BIOSImage", 0, offsetof(VGA, m_bios_file_path), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

VGA::VGA(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_bios_file_path("romimages/VGABIOS-lgpl-latest")
{
  m_vram_size = VRAM_SIZE;
}

VGA::~VGA()
{
  SAFE_RELEASE(m_vram_mmio);
}

bool VGA::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  if (!LoadBIOSROM())
    return false;

  ConnectIOPorts();
  UpdateVGAMemoryMapping();
  return true;
}

void VGA::Reset()
{
  BaseClass::Reset();

  CRTCTimingChanged();
  UpdateVGAMemoryMapping();
}

bool VGA::LoadState(BinaryReader& reader)
{
  if (!BaseClass::LoadState(reader) || reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.SafeReadUInt8(&m_vga_adapter_enable.bits);

  if (reader.GetErrorState())
    return false;

  CRTCTimingChanged();
  UpdateVGAMemoryMapping();
  return true;
}

bool VGA::SaveState(BinaryWriter& writer)
{
  if (!BaseClass::SaveState(writer))
    return false;

  writer.WriteUInt32(SERIALIZATION_ID);

  writer.WriteUInt8(m_vga_adapter_enable.bits);

  return !writer.InErrorState();
}

bool VGA::LoadBIOSROM()
{
  const PhysicalMemoryAddress bios_load_location = 0xC0000;
  return m_bus->CreateROMRegionFromFile(m_bios_file_path, 0, bios_load_location);
}

void VGA::UpdateVGAMemoryMapping()
{
  u32 start_address;
  u32 size;
  GetVGAMemoryMapping(&start_address, &size);

  if (m_vram_mmio)
  {
    m_bus->DisconnectMMIO(m_vram_mmio);
    m_vram_mmio->Release();
  }

  MMIO::Handlers handlers;
  handlers.read_byte = [this](u32 offset) { return HandleVGAVRAMRead(0, offset); };
  handlers.write_byte = [this](u32 offset, u8 value) { HandleVGAVRAMWrite(0, offset, value); };

  m_vram_mmio = MMIO::CreateComplex(start_address, size, std::move(handlers), false);
  m_bus->ConnectMMIO(m_vram_mmio);
}

} // namespace HW