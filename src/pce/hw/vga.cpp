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
  : BaseClass(identifier, type_info), m_bios_file_path("romimages\\VGABIOS-lgpl-latest")
{
  m_vram_size = VRAM_SIZE;
  m_crtc_registers_ptr = m_crtc_registers.data();
  m_graphics_registers_ptr = m_graphics_registers.data();
  m_attribute_register_ptr = m_attribute_registers.data();
  m_sequencer_register_ptr = m_sequencer_registers.data();
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

  RegisterVRAMMMIO();
  return true;
}

void VGA::Reset()
{
  BaseClass::Reset();
}

bool VGA::LoadState(BinaryReader& reader)
{
  if (!BaseClass::LoadState(reader) || reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.SafeReadBytes(m_vram.data(), m_vram_size);
  reader.SafeReadBytes(m_crtc_registers.data(), static_cast<u32>(m_crtc_registers.size()));
  reader.SafeReadBytes(m_graphics_registers.data(), static_cast<u32>(m_graphics_registers.size()));
  reader.SafeReadBytes(m_attribute_registers.data(), static_cast<u32>(m_attribute_registers.size()));
  reader.SafeReadBytes(m_sequencer_registers.data(), static_cast<u32>(m_sequencer_registers.size()));
  reader.SafeReadUInt8(&m_vga_adapter_enable.bits);

  return !reader.GetErrorState();
}

bool VGA::SaveState(BinaryWriter& writer)
{
  if (!BaseClass::SaveState(writer))
    return false;

  writer.WriteUInt32(SERIALIZATION_ID);

  writer.WriteBytes(m_vram.data(), m_vram_size);
  writer.WriteBytes(m_crtc_registers.data(), static_cast<u32>(m_crtc_registers.size()));
  writer.WriteBytes(m_graphics_registers.data(), static_cast<u32>(m_graphics_registers.size()));
  writer.WriteBytes(m_attribute_registers.data(), static_cast<u32>(m_attribute_registers.size()));
  writer.WriteBytes(m_sequencer_registers.data(), static_cast<u32>(m_sequencer_registers.size()));
  writer.WriteUInt8(m_vga_adapter_enable.bits);

  return !writer.InErrorState();
}

void VGA::ConnectIOPorts()
{
  BaseClass::ConnectIOPorts();

  m_bus->ConnectIOPortReadToPointer(0x46E8, this, &m_vga_adapter_enable.bits);
  m_bus->ConnectIOPortWriteToPointer(0x46E8, this, &m_vga_adapter_enable.bits);
  m_bus->ConnectIOPortReadToPointer(0x03C3, this, &m_vga_adapter_enable.bits);
  m_bus->ConnectIOPortWriteToPointer(0x03C3, this, &m_vga_adapter_enable.bits);
}

bool VGA::LoadBIOSROM()
{
  const PhysicalMemoryAddress bios_load_location = 0xC0000;
  return m_bus->CreateROMRegionFromFile(m_bios_file_path, 0, bios_load_location);
}

void VGA::RegisterVRAMMMIO()
{
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

  // Map the entire range (0xA0000 - 0xCFFFF), then throw the writes out in the handler.
  m_vram_mmio = MMIO::CreateComplex(0xA0000, 0x20000, std::move(handlers));
  m_bus->ConnectMMIO(m_vram_mmio);

  // Log_DevPrintf("Mapped %u bytes of VRAM at 0x%08X-0x%08X", size, base, base + size - 1);
}
} // namespace HW