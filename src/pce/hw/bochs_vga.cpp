#include "pce/hw/bochs_vga.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "common/display.h"
#include "pce/bus.h"
#include "pce/mmio.h"
#include "pce/system.h"
Log_SetChannel(HW::BochsVGA);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(BochsVGA);
DEFINE_GENERIC_COMPONENT_FACTORY(BochsVGA);
BEGIN_OBJECT_PROPERTY_MAP(BochsVGA)
PROPERTY_TABLE_MEMBER_STRING("BIOSImage", 0, offsetof(BochsVGA, m_bios_file_path), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

BochsVGA::BochsVGA(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), PCIDevice(identifier, 1, type_info),
    m_bios_file_path("romimages\\VGABIOS-lgpl-latest")
{
  m_vram_size = VBE_DISPI_TOTAL_VIDEO_MEMORY_BYTES;

  InitPCIID(0, 0x1234, 0x1111);
  InitPCIClass(0, 0x03, 0x00, 0x00, 0x00);
  InitPCIMemoryRegion(0, PCIDevice::MemoryRegion_BAR0, VBE_DISPI_DEFAULT_LFB_PHYSICAL_ADDRESS, m_vram_size, false,
                      false);
}

BochsVGA::~BochsVGA()
{
  SAFE_RELEASE(m_lfb_mmio);
  SAFE_RELEASE(m_vga_mmio);
}

bool BochsVGA::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus) || !PCIDevice::Initialize(system, bus))
    return false;

  if (!LoadBIOSROM())
    return false;

  ConnectIOPorts();
  UpdateVGAMemoryMapping();
  return true;
}

void BochsVGA::Reset()
{
  BaseClass::Reset();
  PCIDevice::Reset();

  m_vbe_index_register = 0;
  m_vbe_width = 640;
  m_vbe_height = 480;
  m_vbe_bpp = 32;
  m_vbe_offset_x = 0;
  m_vbe_offset_y = 0;
  m_vbe_virt_width = 0;
  m_vbe_virt_height = 0;

  CRTCTimingChanged();
  UpdateVGAMemoryMapping();
  UpdateFramebufferFormat();
}

bool BochsVGA::LoadState(BinaryReader& reader)
{
  if (!BaseClass::LoadState(reader) || !PCIDevice::LoadState(reader) || reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.SafeReadUInt16(&m_vbe_index_register);
  reader.SafeReadUInt16(&m_vbe_enable.bits);
  reader.SafeReadUInt16(&m_vbe_id);
  reader.SafeReadUInt16(&m_vbe_bank);
  reader.SafeReadUInt16(&m_vbe_width);
  reader.SafeReadUInt16(&m_vbe_height);
  reader.SafeReadUInt16(&m_vbe_bpp);
  reader.SafeReadUInt16(&m_vbe_offset_x);
  reader.SafeReadUInt16(&m_vbe_offset_y);
  reader.SafeReadUInt16(&m_vbe_virt_width);
  reader.SafeReadUInt16(&m_vbe_virt_height);

  if (reader.GetErrorState())
    return false;

  CRTCTimingChanged();
  UpdateVGAMemoryMapping();
  UpdateFramebufferFormat();
  return true;
}

bool BochsVGA::SaveState(BinaryWriter& writer)
{
  if (!BaseClass::SaveState(writer) || !PCIDevice::SaveState(writer))
    return false;

  writer.WriteUInt32(SERIALIZATION_ID);

  writer.WriteUInt16(m_vbe_index_register);
  writer.WriteUInt16(m_vbe_enable.bits);
  writer.WriteUInt16(m_vbe_id);
  writer.WriteUInt16(m_vbe_bank);
  writer.WriteUInt16(m_vbe_width);
  writer.WriteUInt16(m_vbe_height);
  writer.WriteUInt16(m_vbe_bpp);
  writer.WriteUInt16(m_vbe_offset_x);
  writer.WriteUInt16(m_vbe_offset_y);
  writer.WriteUInt16(m_vbe_virt_width);
  writer.WriteUInt16(m_vbe_virt_height);

  return !writer.InErrorState();
}

bool BochsVGA::LoadBIOSROM()
{
  const PhysicalMemoryAddress bios_load_location = 0xC0000;
  return BaseClass::m_bus->CreateROMRegionFromFile(m_bios_file_path, 0, bios_load_location);
}

void BochsVGA::ConnectIOPorts()
{
  BaseClass::ConnectIOPorts();

  BaseClass::m_bus->ConnectIOPortReadWord(VBE_DISPI_IOPORT_INDEX, this,
                                          [this](u16, u16* value) { *value = m_vbe_index_register; });
  BaseClass::m_bus->ConnectIOPortWriteWord(VBE_DISPI_IOPORT_INDEX, this,
                                           [this](u16, u16 value) { m_vbe_index_register = value; });
  BaseClass::m_bus->ConnectIOPortReadWord(VBE_DISPI_IOPORT_DATA, this,
                                          [this](u16, u16* value) { *value = IOReadVBEDataRegister(); });
  BaseClass::m_bus->ConnectIOPortWriteWord(VBE_DISPI_IOPORT_DATA, this,
                                           [this](u16, u16 value) { IOWriteVBEDataRegister(value); });
}

void BochsVGA::UpdateVGAMemoryMapping()
{
  if (m_lfb_mmio)
  {
    BaseClass::m_bus->DisconnectMMIO(m_lfb_mmio);
    m_lfb_mmio->Release();
    m_lfb_mmio = nullptr;
  }

  if (m_vga_mmio)
  {
    BaseClass::m_bus->DisconnectMMIO(m_vga_mmio);
    m_vga_mmio->Release();
    m_vga_mmio = nullptr;
  }

  if (IsLFBEnabled() && IsPCIMemoryActive(0))
  {
    const PhysicalMemoryAddress base_address = GetMemoryRegionBaseAddress(0, PCIDevice::MemoryRegion_BAR0);
    m_lfb_mmio = MMIO::CreateDirect(base_address, m_vram_size, m_vram.data(), true, true, false);
    BaseClass::m_bus->ConnectMMIO(m_lfb_mmio);
    Log_DebugPrintf("LFB is enabled at %08X", base_address);
  }

  if (m_vbe_enable.enable)
  {
    MMIO::Handlers handlers;
    if (m_vbe_bpp <= 4)
    {
      // 4bpp modes use the regular VGA latches.
      handlers.read_byte = [this](u32 offset, u8* value) {
        HandleVGAVRAMRead(ZeroExtend32(m_vbe_bank) * VBE_DISPI_BANK_SIZE, offset, value);
      };
      handlers.write_byte = [this](u32 offset, u8 value) {
        HandleVGAVRAMWrite(ZeroExtend32(m_vbe_bank) * VBE_DISPI_BANK_SIZE, offset, value);
      };
    }
    else
    {
      // Other modes are mapped directly to VRAM.
      handlers.read_byte = [this](u32 offset, u8* value) {
        std::memcpy(value, &m_vram[ZeroExtend32(m_vbe_bank) * VBE_DISPI_BANK_SIZE + offset], sizeof(*value));
      };
      handlers.read_word = [this](u32 offset, u16* value) {
        std::memcpy(value, &m_vram[ZeroExtend32(m_vbe_bank) * VBE_DISPI_BANK_SIZE + offset], sizeof(*value));
      };
      handlers.read_dword = [this](u32 offset, u32* value) {
        std::memcpy(value, &m_vram[ZeroExtend32(m_vbe_bank) * VBE_DISPI_BANK_SIZE + offset], sizeof(*value));
      };
      handlers.write_byte = [this](u32 offset, u8 value) {
        std::memcpy(&m_vram[ZeroExtend32(m_vbe_bank) * VBE_DISPI_BANK_SIZE + offset], &value, sizeof(value));
      };
      handlers.write_word = [this](u32 offset, u16 value) {
        std::memcpy(&m_vram[ZeroExtend32(m_vbe_bank) * VBE_DISPI_BANK_SIZE + offset], &value, sizeof(value));
      };
      handlers.write_dword = [this](u32 offset, u32 value) {
        std::memcpy(&m_vram[ZeroExtend32(m_vbe_bank) * VBE_DISPI_BANK_SIZE + offset], &value, sizeof(value));
      };
    }

    // VBE banked modes are always mapped to A0000 and 64KB in size.
    m_vga_mmio = MMIO::CreateComplex(VBE_DISPI_BANK_ADDRESS, VBE_DISPI_BANK_SIZE, std::move(handlers), false);
    BaseClass::m_bus->ConnectMMIO(m_vga_mmio);
  }
  else
  {
    PhysicalMemoryAddress start_address;
    u32 size;
    GetVGAMemoryMapping(&start_address, &size);

    MMIO::Handlers handlers;
    handlers.read_byte = [this](u32 offset, u8* value) { HandleVGAVRAMRead(0, offset, value); };
    handlers.write_byte = [this](u32 offset, u8 value) { HandleVGAVRAMWrite(0, offset, value); };

    m_vga_mmio = MMIO::CreateComplex(start_address, size, std::move(handlers), false);
    BaseClass::m_bus->ConnectMMIO(m_vga_mmio);
  }
}

bool BochsVGA::IsValidBPP(u16 bpp)
{
  return bpp == 4 || bpp == 8 || bpp == 15 || bpp == 16 || bpp == 24 || bpp == 32;
}

u16 BochsVGA::IOReadVBEDataRegister()
{
  switch (m_vbe_index_register)
  {
    case VBE_DISPI_INDEX_ID:
      return m_vbe_id;

    case VBE_DISPI_INDEX_XRES:
      return m_vbe_enable.read_capabilities ? VBE_DISPI_MAX_XRES : m_vbe_width;

    case VBE_DISPI_INDEX_YRES:
      return m_vbe_enable.read_capabilities ? VBE_DISPI_MAX_YRES : m_vbe_height;

    case VBE_DISPI_INDEX_BPP:
      return m_vbe_enable.read_capabilities ? VBE_DISPI_MAX_BPP : m_vbe_bpp;

    case VBE_DISPI_INDEX_ENABLE:
      return m_vbe_enable.bits;

    case VBE_DISPI_INDEX_BANK:
      return m_vbe_bank;

    case VBE_DISPI_INDEX_VIRT_WIDTH:
      return m_vbe_virt_width;

    case VBE_DISPI_INDEX_VIRT_HEIGHT:
      return m_vbe_virt_height;

    case VBE_DISPI_INDEX_X_OFFSET:
      return m_vbe_offset_x;

    case VBE_DISPI_INDEX_Y_OFFSET:
      return m_vbe_offset_y;

    case VBE_DISPI_INDEX_VIDEO_MEMORY_64K:
      return static_cast<u16>(m_vram_size / 65536);

    case VBE_DISPI_INDEX_DDC:
      return static_cast<u16>(0x000F);

    default:
      return UINT16_C(0xFFFF);
  }
}

void BochsVGA::IOWriteVBEDataRegister(u16 value)
{
  switch (m_vbe_index_register)
  {
    case VBE_DISPI_INDEX_ID:
    {
      if (value < VBE_DISPI_ID0 || value > VBE_DISPI_ID5)
        Log_WarningPrintf("Invalid ID: %08X", ZeroExtend32(value));
      else
        m_vbe_id = value;
    }
    break;

    case VBE_DISPI_INDEX_XRES:
      Log_DebugPrintf("X Resolution = %u", ZeroExtend32(value));
      m_vbe_width = value;
      break;

    case VBE_DISPI_INDEX_YRES:
      Log_DebugPrintf("Y Resolution = %u", ZeroExtend32(value));
      m_vbe_height = value;
      break;

    case VBE_DISPI_INDEX_BPP:
    {
      if (!IsValidBPP(value))
      {
        Log_WarningPrintf("BPP %u is invalid", ZeroExtend32(value));
        return;
      }

      Log_DebugPrintf("BPP = %u", ZeroExtend32(value));
      m_vbe_bpp = value;
      if (m_vbe_enable.enable)
      {
        // BPP can change the mapping (if switching to/from 4bpp).
        UpdateVGAMemoryMapping();
        UpdateFramebufferFormat();
      }
    }
    break;

    case VBE_DISPI_INDEX_ENABLE:
    {
      const bool old_enable = m_vbe_enable.enable;
      const bool old_lfb_enable = m_vbe_enable.lfb_enable;
      const bool old_dac_8bit = m_vbe_enable.dac_8bit;

      m_vbe_enable.bits = value;

      if (m_vbe_enable.enable != old_enable)
      {
        CRTCTimingChanged();
        UpdateVGAMemoryMapping();
        UpdateFramebufferFormat();

        if (m_vbe_enable.enable)
        {
          // Virtual resolution is reset on enable.
          m_vbe_virt_width = m_vbe_width;
          m_vbe_virt_height = m_vbe_height;
          m_vbe_offset_x = 0;
          m_vbe_offset_y = 0;

          if (!m_vbe_enable.no_clear_mem)
          {
            Log_DebugPrintf("Zeroing VRAM");
            Y_memzero(m_vram.data(), m_vram_size);
          }
        }
      }
    }
    break;

    case VBE_DISPI_INDEX_BANK:
    {
      if ((ZeroExtend64(value + 1) * VBE_DISPI_BANK_SIZE) > ZeroExtend64(m_vram_size))
        Log_WarningPrintf("VBE bank 0x%04X is invalid", ZeroExtend32(value));
      else
        m_vbe_bank = value;
    }
    break;

    case VBE_DISPI_INDEX_VIRT_WIDTH:
      Log_DebugPrintf("Virtual Width = %u", ZeroExtend32(value));
      m_vbe_virt_width = value;
      break;

    case VBE_DISPI_INDEX_VIRT_HEIGHT:
      Log_DebugPrintf("Virtual Height = %u", ZeroExtend32(value));
      m_vbe_virt_height = value;
      break;

    case VBE_DISPI_INDEX_X_OFFSET:
      Log_DebugPrintf("Offset X = %u", ZeroExtend32(value));
      m_vbe_offset_x = value;
      break;

    case VBE_DISPI_INDEX_Y_OFFSET:
      Log_DebugPrintf("Offset Y = %u", ZeroExtend32(value));
      m_vbe_offset_y = value;
      break;

    case VBE_DISPI_INDEX_VIDEO_MEMORY_64K:
      Log_WarningPrintf("Write to 64K memory size %08X", value);
      break;

    case VBE_DISPI_INDEX_DDC:
      Log_WarningPrintf("DDC write %08X", value);
      break;

    default:
      break;
  }
}

void BochsVGA::OnMemoryRegionChanged(u8 function, MemoryRegion region, bool active)
{
  PCIDevice::OnMemoryRegionChanged(function, region, active);
  if (function != 0x00 || region != PCIDevice::MemoryRegion_BAR0)
    return;

  if (IsLFBEnabled())
    UpdateVGAMemoryMapping();
}

bool BochsVGA::IsLFBEnabled() const
{
  return m_vbe_enable.enable && m_vbe_enable.lfb_enable;
}

void BochsVGA::UpdateFramebufferFormat()
{
  if (!m_vbe_enable.enable)
  {
    m_display->ChangeFramebufferFormat(BASE_FRAMEBUFFER_FORMAT);
    return;
  }

  if (m_vbe_bpp <= 4)
    m_display->ChangeFramebufferFormat(BASE_FRAMEBUFFER_FORMAT);
  else if (m_vbe_bpp <= 8)
    m_display->ChangeFramebufferFormat(Display::FramebufferFormat::RGBX8);
  else if (m_vbe_bpp <= 16)
    m_display->ChangeFramebufferFormat(Display::FramebufferFormat::RGB565);
  else if (m_vbe_bpp <= 24)
    m_display->ChangeFramebufferFormat(Display::FramebufferFormat::RGB8);
  else // if (m_vbe_bpp <= 32)
    m_display->ChangeFramebufferFormat(Display::FramebufferFormat::RGBX8);
}

void BochsVGA::GetDisplayTiming(DisplayTiming& timing) const
{
  if (!m_vbe_enable.enable)
  {
    BaseClass::GetDisplayTiming(timing);
    return;
  }

  // Valid?
  if (m_vbe_width == 0 || m_vbe_width >= VBE_DISPI_MAX_XRES || m_vbe_height == 0 || m_vbe_height >= VBE_DISPI_MAX_YRES)
    return;

  // We fake this by setting the timings for VGA 640x480 @ 60hz, and override the size in the latch.
  Log_DebugPrintf("Returning VGA timings for %ux%ux%u", ZeroExtend32(m_vbe_width), ZeroExtend32(m_vbe_height),
                  ZeroExtend32(m_vbe_bpp));

  timing.SetPixelClock(25.175 * 1000000.0);
  timing.SetHorizontalVisible(640);
  timing.SetHorizontalSyncLength(640 + 16, 96);
  timing.SetHorizontalTotal(800);
  timing.SetVerticalVisible(480);
  timing.SetVerticalSyncLength(480 + 10, 2);
  timing.SetVerticalTotal(525);
}

void BochsVGA::LatchStartAddress()
{
  if (!m_vbe_enable.enable)
  {
    BaseClass::LatchStartAddress();
    return;
  }

  const u32 bpp = ZeroExtend32(m_vbe_bpp);
  const u32 virt_width = ZeroExtend32(m_vbe_virt_width);

  m_render_latch = {};
  m_render_latch.graphics_mode = true;
  m_render_latch.render_width = ZeroExtend32(m_vbe_width);
  m_render_latch.render_height = ZeroExtend32(m_vbe_height);
  m_render_latch.pitch = (bpp * virt_width) / 8;
  m_render_latch.start_address =
    ZeroExtend32(m_vbe_offset_y) * ((bpp * virt_width) / 8) + ((ZeroExtend32(m_vbe_offset_x) * bpp) / 8);

  if ((m_render_latch.start_address + (m_render_latch.pitch * m_render_latch.render_height)) > m_vram_size)
  {
    Log_WarningPrintf("VBE start address 0x%08X out-of-range", m_render_latch.start_address);
    m_render_latch.start_address = m_vram_size;
  }
}

void BochsVGA::RenderGraphicsMode()
{
  if (!m_vbe_enable.enable)
  {
    BaseClass::RenderGraphicsMode();
    return;
  }

  if (m_render_latch.start_address == m_vram_size)
  {
    // invalid, so skip the frame
    return;
  }

  switch (m_vbe_bpp)
  {
    case 4:
      BaseClass::RenderGraphicsMode();
      break;
    case 8:
      Render8BPP();
      break;
    case 15:
    case 16:
    case 24:
    case 32:
      RenderDirect();
      break;
    default:
      break;
  }
}

void BochsVGA::Render8BPP()
{
  // Use DAC palette directly if in 8-bit mode.
  const u32* palette = m_dac_palette.data();
  if (!m_vbe_enable.dac_8bit)
  {
    SetOutputPalette256();
    palette = m_output_palette.data();
  }

  const u32 fb_stride = m_display->GetFramebufferStride();
  byte* fb_ptr = m_display->GetFramebufferPointer();

  const u8* vram_ptr = m_vram.data() + m_render_latch.start_address;
  for (u32 row = 0; row < m_render_latch.render_height; row++)
  {
    byte* fb_row_ptr = fb_ptr;
    const u8* vram_row_ptr = vram_ptr;
    for (u32 col = 0; col < m_render_latch.render_width; col++)
    {
      const u32 palette_entry = palette[*vram_row_ptr++];
      std::memcpy(fb_row_ptr, &palette_entry, sizeof(palette_entry));
      fb_row_ptr += sizeof(palette_entry);
    }

    fb_ptr += fb_stride;
    vram_ptr += m_render_latch.pitch;
  }

  m_display->SwapFramebuffer();
}

void BochsVGA::RenderDirect()
{
  const u32 fb_stride = m_display->GetFramebufferStride();
  const u32 copy_stride = std::min(fb_stride, m_render_latch.pitch);

  // If the pitch matches, just copy it directly.
  if (m_render_latch.pitch == fb_stride)
  {
    std::memcpy(m_display->GetFramebufferPointer(), &m_vram[m_render_latch.start_address],
                copy_stride * m_render_latch.render_height);
  }
  else
  {
    // Copy the lines direct to the output framebuffer.
    byte* fb_ptr = m_display->GetFramebufferPointer();
    u32 current_address = m_render_latch.start_address;
    for (u32 row = 0; row < m_render_latch.render_height; row++)
    {
      std::memcpy(fb_ptr, &m_vram[current_address], copy_stride);
      current_address += m_render_latch.pitch;
      fb_ptr += fb_stride;
    }
  }

  m_display->SwapFramebuffer();
}

} // namespace HW
