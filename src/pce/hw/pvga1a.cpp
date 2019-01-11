#include "pce/hw/pvga1a.h"
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
Log_SetChannel(HW::PVGA1A);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(PVGA1A);
DEFINE_GENERIC_COMPONENT_FACTORY(PVGA1A);
BEGIN_OBJECT_PROPERTY_MAP(PVGA1A)
PROPERTY_TABLE_MEMBER_UINT("VRAMSize", 0, offsetof(PVGA1A, m_vram_size), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("BIOSImage", 0, offsetof(PVGA1A, m_bios_file_path), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

PVGA1A::PVGA1A(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_bios_file_path("romimages\\paradisepvga1a.bin")
{
  m_vram_size = MAX_VRAM_SIZE;
}

PVGA1A::~PVGA1A()
{
  SAFE_RELEASE(m_vram_mmio);
}

bool PVGA1A::Initialize(System* system, Bus* bus)
{
  if (m_vram_size > MAX_VRAM_SIZE)
  {
    Log_ErrorPrintf("VRAM size exceeds maximum (%u vs %u)", m_vram_size, MAX_VRAM_SIZE);
    return false;
  }

  if (!BaseClass::Initialize(system, bus))
    return false;

  if (!LoadBIOSROM())
    return false;

  // Paradise registers.
  m_graphics_register_mask[PR0A] = 0xFF;
  m_graphics_register_mask[PR0B] = 0xFF;
  m_graphics_register_mask[PR1] = 0xFF;
  m_graphics_register_mask[PR2] = 0xFF;
  m_graphics_register_mask[PR3] = 0xFF;
  m_graphics_register_mask[PR4] = 0xFF;
  m_graphics_register_mask[PR5] = 0xFF;

  return true;
}

void PVGA1A::Reset()
{
  BaseClass::Reset();

  if (m_vram_size >= 1024 * 1024)
    m_graphics_registers[PR1] = 0b11000000;
  else if (m_vram_size >= 512 * 1024)
    m_graphics_registers[PR1] = 0b10000000;
  else if (m_vram_size >= 256 * 1024)
    m_graphics_registers[PR1] = 0b01000000;
  else
    m_graphics_registers[PR1] = 0b00000000;
}

bool PVGA1A::LoadState(BinaryReader& reader)
{
  if (!BaseClass::LoadState(reader) || reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  return !reader.GetErrorState();
}

bool PVGA1A::SaveState(BinaryWriter& writer)
{
  if (!BaseClass::SaveState(writer))
    return false;

  writer.WriteUInt32(SERIALIZATION_ID);

  return !writer.InErrorState();
}

void PVGA1A::ConnectIOPorts()
{
  BaseClass::ConnectIOPorts();
}

bool PVGA1A::LoadBIOSROM()
{
  const PhysicalMemoryAddress bios_load_location = 0xC0000;
  return m_bus->CreateROMRegionFromFile(m_bios_file_path, 0, bios_load_location);
}

void PVGA1A::UpdateVGAMemoryMapping()
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

void PVGA1A::IOGraphicsRegisterWrite(u8 value)
{
  BaseClass::IOSequencerDataRegisterWrite(value);

  switch (m_graphics_index_register)
  {
    case PR3:   // CRTC Control
      {
        const u8 pr3 = m_graphics_registers[PR3];

        const u8 group0_mask = (pr3 & (1u << 5)) ? 0x00 : 0xFF;
        m_crtc_register_mask[CRTC_REGISTER_HORIZONTAL_TOTAL] = group0_mask;
        m_crtc_register_mask[CRTC_REGISTER_HORIZONTAL_DISPLAY_END] = group0_mask;
        m_crtc_register_mask[CRTC_REGISTER_HORIZONTAL_BLANKING_START] = group0_mask;
        m_crtc_register_mask[CRTC_REGISTER_HORIZONTAL_BLANKING_END] = group0_mask;
        m_crtc_register_mask[CRTC_REGISTER_HORIZONTAL_SYNC_START] = group0_mask;
        m_crtc_register_mask[CRTC_REGISTER_HORIZONTAL_SYNC_END] = group0_mask;

        //if (pr3 & (1u << 1))
      }
      break;

    case PR5:   // 
  }
}

} // namespace HW