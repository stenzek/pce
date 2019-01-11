#include "pce/hw/t8900d.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "common/display.h"
#include "pce/bus.h"
#include "pce/mmio.h"
#include "pce/system.h"
#include "pce/hw/vgahelpers.h"
Log_SetChannel(HW::T8900D);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(T8900D);
DEFINE_GENERIC_COMPONENT_FACTORY(T8900D);
BEGIN_OBJECT_PROPERTY_MAP(T8900D)
PROPERTY_TABLE_MEMBER_UINT("VRAMSize", 0, offsetof(T8900D, m_vram_size), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("BIOSImage", 0, offsetof(T8900D, m_bios_file_path), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

T8900D::T8900D(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_bios_file_path("romimages\\trident.bin")
{
  m_vram_size = MAX_VRAM_SIZE;
}

T8900D::~T8900D()
{
  SAFE_RELEASE(m_vram_mmio);
}

bool T8900D::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  if (!LoadBIOSROM())
    return false;

  return true;
}

void T8900D::Reset()
{
  BaseClass::Reset();
}

bool T8900D::LoadState(BinaryReader& reader)
{
  if (!BaseClass::LoadState(reader) || reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  return !reader.GetErrorState();
}

bool T8900D::SaveState(BinaryWriter& writer)
{
  if (!BaseClass::SaveState(writer))
    return false;

  writer.WriteUInt32(SERIALIZATION_ID);

  return !writer.InErrorState();
}

void T8900D::ConnectIOPorts()
{
  BaseClass::ConnectIOPorts();
}

bool T8900D::LoadBIOSROM()
{
  const PhysicalMemoryAddress bios_load_location = 0xC0000;
  return m_bus->CreateROMRegionFromFile(m_bios_file_path, 0, bios_load_location);
}

void T8900D::UpdateVGAMemoryMapping()
{
  u32 memory_base, memory_size;
  switch (GRAPHICS_REGISTER_MISCELLANEOUS_MEMORY_MAP_SELECT(m_graphics_registers[GRAPHICS_REGISTER_MISCELLANEOUS]))
  {
    case 0: // A0000-BFFFF (128K)
      memory_base = 0xA0000;
      memory_size = 0x20000;
      break;

    case 1: // A0000-AFFFF (64K)
      memory_base = 0xA0000;
      memory_size = 0x10000;
      break;

    case 2: // B0000-B7FFF (32K)
      memory_base = 0xB0000;
      memory_size = 0x08000;
      break;

    case 3: // B8000-BFFFF (32K)
    default:
      memory_base = 0xB8000;
      memory_size = 0x08000;
      break;
  }

  MMIO::Handlers handlers;
  handlers.read_byte = [this](u32 offset, u8* value) { HandleVGAVRAMRead(0, offset, value); };
  handlers.read_word = [this](u32 offset, u16* value) {
    u8 b0, b1;
    HandleVGAVRAMRead(0, offset + 0, &b0);
    HandleVGAVRAMRead(0, offset + 1, &b1);
    *value = (u16(b1) << 8) | (u16(b0));
  };
  handlers.read_dword = [this](u32 offset, u32* value) {
    u8 b0, b1, b2, b3;
    HandleVGAVRAMRead(0, offset + 0, &b0);
    HandleVGAVRAMRead(0, offset + 1, &b1);
    HandleVGAVRAMRead(0, offset + 2, &b2);
    HandleVGAVRAMRead(0, offset + 3, &b3);
    *value = (u32(b3) << 24) | (u32(b2) << 16) | (u32(b1) << 8) | (u32(b0));
  };
  handlers.write_byte = [this](u32 offset, u8 value) { HandleVGAVRAMWrite(0, offset, value); };
  handlers.write_word = [this](u32 offset, u16 value) {
    HandleVGAVRAMWrite(0, offset + 0, u8(value & 0xFF));
    HandleVGAVRAMWrite(0, offset + 1, u8((value >> 8) & 0xFF));
  };
  handlers.write_dword = [this](u32 offset, u32 value) {
    HandleVGAVRAMWrite(0, offset + 0, u8(value & 0xFF));
    HandleVGAVRAMWrite(0, offset + 1, u8((value >> 8) & 0xFF));
    HandleVGAVRAMWrite(0, offset + 2, u8((value >> 16) & 0xFF));
    HandleVGAVRAMWrite(0, offset + 3, u8((value >> 24) & 0xFF));
  };

  if (m_vram_mmio)
  {
    m_bus->DisconnectMMIO(m_vram_mmio);
    m_vram_mmio->Release();
  }
  m_vram_mmio = MMIO::CreateComplex(memory_base, memory_size, std::move(handlers));
  m_bus->ConnectMMIO(m_vram_mmio);

  // Log_DevPrintf("Mapped %u bytes of VRAM at 0x%08X-0x%08X", size, base, base + size - 1);
}

void T8900D::IOSequencerDataRegisterWrite(u8 value)
{
  if (m_sequencer_index_register == 0x0E)
    __debugbreak();

  BaseClass::IOSequencerDataRegisterWrite(value);
}

} // namespace HW