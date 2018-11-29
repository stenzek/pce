#include "vgabase.h"
#include "../bus.h"
#include "../host_interface.h"
#include "../mmio.h"
#include "../system.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "common/display.h"
#include "vgahelpers.h"
Log_SetChannel(HW::VGABase);

namespace HW {

DEFINE_OBJECT_TYPE_INFO(VGABase);
DEFINE_GENERIC_COMPONENT_FACTORY(VGABase);
BEGIN_OBJECT_PROPERTY_MAP(VGABase)
END_OBJECT_PROPERTY_MAP()

VGABase::VGABase(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info)
{
}

VGABase::~VGABase() = default;

bool VGABase::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_display = system->GetHostInterface()->CreateDisplay(
    SmallString::FromFormat("%s (%s)", m_identifier.GetCharArray(), m_type_info->GetTypeName()),
    Display::Type::Primary);
  if (!m_display)
    return false;
  m_display->SetDisplayAspectRatio(4, 3);

  ConnectIOPorts();

  m_display_event =
    m_system->GetTimingManager()->CreateFrequencyEvent("VGA Render", 60.0f, std::bind(&VGABase::Render, this), true);
  return true;
}

void VGABase::Reset()
{
  for (u32 i = 0; i < MAX_VGA_CRTC_REGISTER; i++)
    m_crtc_registers_ptr[i] = 0;
  for (u32 i = 0; i < MAX_VGA_GRAPHICS_REGISTER; i++)
    m_graphics_registers_ptr[i] = 0;
  for (u32 i = 0; i < MAX_VGA_SEQUENCER_REGISTER; i++)
    m_sequencer_register_ptr[i] = 0;
  for (u32 i = 0; i < MAX_VGA_ATTRIBUTE_REGISTER; i++)
    m_attribute_register_ptr[i] = 0;

  m_crtc_timing_changed = true;

  m_misc_output_register.io_address_select = false;
  m_misc_output_register.ram_enable = true;
  m_misc_output_register.odd_even_page = false;
  m_misc_output_register.clock_select = 0;
  m_misc_output_register.hsync_polarity = true;
  m_misc_output_register.vsync_polarity = true;

  m_dac_state_register = 0;
  m_dac_read_address = 0;
  m_dac_write_address = 0;
  m_dac_color_index = 0;
  for (size_t i = 0; i < m_dac_palette.size(); i++)
    m_dac_palette[i] = 0xFFFFFFFF;

  m_cursor_counter = 0;
  m_cursor_state = false;

  UpdateDisplayTiming();
}

bool VGABase::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.SafeReadUInt8(&m_crtc_index_register);
  reader.SafeReadUInt8(&m_graphics_address_register);
  reader.SafeReadUInt8(&m_misc_output_register.bits);
  reader.SafeReadUInt8(&m_feature_control_register.bits);
  reader.SafeReadUInt8(&m_attribute_address_register);
  reader.SafeReadBool(&m_attribute_register_flipflop);
  reader.SafeReadUInt8(&m_sequencer_address_register);
  reader.SafeReadBytes(m_dac_palette.data(), Truncate32(sizeof(u32) * m_dac_palette.size()));
  reader.SafeReadUInt8(&m_dac_state_register);
  reader.SafeReadUInt8(&m_dac_write_address);
  reader.SafeReadUInt8(&m_dac_read_address);
  reader.SafeReadUInt8(&m_dac_color_index);
  reader.SafeReadUInt32(&m_latch);
  reader.SafeReadBytes(m_output_palette.data(), Truncate32(sizeof(u32) * m_output_palette.size()));
  reader.SafeReadUInt8(&m_character_width);
  reader.SafeReadUInt8(&m_cursor_counter);
  reader.SafeReadBool(&m_cursor_state);

  m_crtc_timing_changed = true;

  return !reader.GetErrorState();
}

bool VGABase::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  writer.WriteUInt8(m_crtc_index_register);
  writer.WriteUInt8(m_graphics_address_register);
  writer.WriteUInt8(m_misc_output_register.bits);
  writer.WriteUInt8(m_feature_control_register.bits);
  writer.WriteUInt8(m_attribute_address_register);
  writer.WriteBool(m_attribute_register_flipflop);
  writer.WriteUInt8(m_sequencer_address_register);
  writer.WriteBytes(m_dac_palette.data(), Truncate32(sizeof(u32) * m_dac_palette.size()));
  writer.WriteUInt8(m_dac_state_register);
  writer.WriteUInt8(m_dac_write_address);
  writer.WriteUInt8(m_dac_read_address);
  writer.WriteUInt8(m_dac_color_index);
  writer.WriteUInt32(m_latch);
  writer.WriteBytes(m_output_palette.data(), Truncate32(sizeof(u32) * m_output_palette.size()));
  writer.WriteUInt8(m_character_width);
  writer.WriteUInt8(m_cursor_counter);
  writer.WriteBool(m_cursor_state);

  return !writer.InErrorState();
}

void VGABase::ConnectIOPorts()
{
  m_bus->ConnectIOPortReadToPointer(0x03B0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03B0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03B2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03B2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03B4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03B4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortRead(0x03B1, this, [this](u16, u8* value) { IOCRTCDataRegisterRead(value); });
  m_bus->ConnectIOPortWrite(0x03B1, this, [this](u16, u8 value) { IOCRTCDataRegisterWrite(value); });
  m_bus->ConnectIOPortRead(0x03B3, this, [this](u16, u8* value) { IOCRTCDataRegisterRead(value); });
  m_bus->ConnectIOPortWrite(0x03B3, this, [this](u16, u8 value) { IOCRTCDataRegisterWrite(value); });
  m_bus->ConnectIOPortRead(0x03B5, this, [this](u16, u8* value) { IOCRTCDataRegisterRead(value); });
  m_bus->ConnectIOPortWrite(0x03B5, this, [this](u16, u8 value) { IOCRTCDataRegisterWrite(value); });
  m_bus->ConnectIOPortReadToPointer(0x03D0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03D0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03D2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03D2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03D4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03D4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortRead(0x03D1, this, [this](u16, u8* value) { IOCRTCDataRegisterRead(value); });
  m_bus->ConnectIOPortWrite(0x03D1, this, [this](u16, u8 value) { IOCRTCDataRegisterWrite(value); });
  m_bus->ConnectIOPortRead(0x03D3, this, [this](u16, u8* value) { IOCRTCDataRegisterRead(value); });
  m_bus->ConnectIOPortWrite(0x03D3, this, [this](u16, u8 value) { IOCRTCDataRegisterWrite(value); });
  m_bus->ConnectIOPortRead(0x03D5, this, [this](u16, u8* value) { IOCRTCDataRegisterRead(value); });
  m_bus->ConnectIOPortWrite(0x03D5, this, [this](u16, u8 value) { IOCRTCDataRegisterWrite(value); });
  m_bus->ConnectIOPortRead(0x03C2, this, [this](u16, u8* value) { IOReadStatusRegister0(value); });
  m_bus->ConnectIOPortRead(0x03BA, this, [this](u16, u8* value) { IOReadStatusRegister1(value); });
  m_bus->ConnectIOPortRead(0x03DA, this, [this](u16, u8* value) { IOReadStatusRegister1(value); });
  m_bus->ConnectIOPortReadToPointer(0x03CE, this, &m_graphics_address_register);
  m_bus->ConnectIOPortWriteToPointer(0x03CE, this, &m_graphics_address_register);
  m_bus->ConnectIOPortRead(0x03CF, this, [this](u16, u8* value) { IOGraphicsRegisterRead(value); });
  m_bus->ConnectIOPortWrite(0x03CF, this, [this](u16, u8 value) { IOGraphicsRegisterWrite(value); });
  m_bus->ConnectIOPortReadToPointer(0x03CC, this, &m_misc_output_register.bits);
  m_bus->ConnectIOPortWrite(0x03C2, this, [this](u16, u8 value) { IOMiscOutputRegisterWrite(value); });
  m_bus->ConnectIOPortReadToPointer(0x03CA, this, &m_feature_control_register.bits);
  m_bus->ConnectIOPortWriteToPointer(0x03BA, this, &m_feature_control_register.bits);
  m_bus->ConnectIOPortWriteToPointer(0x03DA, this, &m_feature_control_register.bits);
  m_bus->ConnectIOPortReadToPointer(0x03C0, this, &m_attribute_address_register);
  m_bus->ConnectIOPortWrite(0x03C0, this, [this](u16, u8 value) { IOAttributeAddressDataWrite(value); });
  m_bus->ConnectIOPortRead(0x03C1, this, [this](u16, u8* value) { IOAttributeDataRead(value); });
  m_bus->ConnectIOPortReadToPointer(0x03C4, this, &m_sequencer_address_register);
  m_bus->ConnectIOPortWriteToPointer(0x03C4, this, &m_sequencer_address_register);
  m_bus->ConnectIOPortRead(0x03C5, this, [this](u16, u8* value) { IOSequencerDataRegisterRead(value); });
  m_bus->ConnectIOPortWrite(0x03C5, this, [this](u16, u8 value) { IOSequencerDataRegisterWrite(value); });
  m_bus->ConnectIOPortRead(0x03C7, this, [this](u16, u8* value) { IODACStateRegisterRead(value); });
  m_bus->ConnectIOPortWrite(0x03C7, this, [this](u16, u8 value) { IODACReadAddressWrite(value); });
  m_bus->ConnectIOPortRead(0x03C8, this, [this](u16, u8* value) { IODACWriteAddressRead(value); });
  m_bus->ConnectIOPortWrite(0x03C8, this, [this](u16, u8 value) { IODACWriteAddressWrite(value); });
  m_bus->ConnectIOPortRead(0x03C9, this, [this](u16, u8* value) { IODACDataRegisterRead(value); });
  m_bus->ConnectIOPortWrite(0x03C9, this, [this](u16, u8 value) { IODACDataRegisterWrite(value); });
}

void VGABase::Render()
{
  // On the standard ET4000, the blink rate is dependent on the vertical frame rate. The on/off state of the cursor
  // changes every 16 vertical frames, which amounts to 1.875 blinks per second at 60 vertical frames per second. The
  // cursor blink rate is thus fixed and cannot be software controlled on the standard ET4000. Some SVGA chipsets
  // provide non-standard means for changing the blink rate of the text-mode cursor.
  // TODO: Should this tick in only text mode, and only when the cursor is enabled?
  if ((++m_cursor_counter) == 16)
  {
    m_cursor_counter = 0;
    m_cursor_state ^= true;
  }

  if (m_crtc_timing_changed)
  {
    m_crtc_timing_changed = false;
    UpdateDisplayTiming();
  }

  if (!m_display->IsActive())
    return;

  if (m_display_timing.IsValid())
  {
    m_display->ClearFramebuffer();
    return;
  }

  if (GRAPHICS_REGISTER_MISCELLANEOUS_GRAPHICS_MODE(m_graphics_registers_ptr[GRAPHICS_REGISTER_MISCELLANEOUS]))
    RenderGraphicsMode();
  else
    RenderTextMode();
}

void VGABase::IOCRTCDataRegisterRead(u8* value)
{
  if (m_crtc_index_register > MAX_VGA_CRTC_REGISTER)
  {
    Log_ErrorPrintf("Out-of-range CRTC register read: %u", u32(m_crtc_index_register));
    *value = 0;
    return;
  }

  *value = m_crtc_registers_ptr[m_crtc_index_register];
  Log_TracePrintf("CRTC register read: %u -> 0x%02X", u32(register_index),
                  u32(m_crtc_registers_ptr[m_crtc_index_register]));
}

void VGABase::IOCRTCDataRegisterWrite(u8 value)
{
  if (m_crtc_index_register > MAX_VGA_CRTC_REGISTER)
  {
    Log_ErrorPrintf("Out-of-range CRTC register write: %u", u32(m_crtc_index_register));
    return;
  }

  u32 register_index = m_crtc_index_register;
  m_crtc_registers_ptr[m_crtc_index_register] = value;

  Log_TracePrintf("CRTC register write: %u <- 0x%02X", u32(m_crtc_index_register), u32(value));

  switch (m_crtc_index_register)
  {
    case CRTC_REGISTER_HORIZONTAL_TOTAL:
    case CRTC_REGISTER_HORIZONTAL_DISPLAY_END:
    case CRTC_REGISTER_HORIZONTAL_BLANKING_START:
    case CRTC_REGISTER_HORIZONTAL_BLANKING_END:
    case CRTC_REGISTER_HORIZONTAL_SYNC_START:
    case CRTC_REGISTER_HORIZONTAL_SYNC_END:
    case CRTC_REGISTER_VERTICAL_TOTAL:
    case CRTC_REGISTER_OVERFLOW:
    case CRTC_REGISTER_VERTICAL_SYNC_START:
    case CRTC_REGISTER_VERTICAL_SYNC_END:
    case CRTC_REGISTER_VERTICAL_DISPLAY_END:
    case CRTC_REGISTER_VERTICAL_BLANK_START:
    case CRTC_REGISTER_VERTICAL_BLANK_END:
      m_crtc_timing_changed = true;
      break;
  }
}

void VGABase::IOGraphicsRegisterRead(u8* value)
{
  if (m_graphics_address_register > MAX_VGA_GRAPHICS_REGISTER)
  {
    Log_ErrorPrintf("Out-of-range graphics register read: %u", u32(m_graphics_address_register));
    *value = 0;
    return;
  }

  *value = m_graphics_registers_ptr[m_graphics_address_register];

  Log_TracePrintf("Graphics register read: %u -> 0x%02X", u32(m_graphics_address_register),
                  u32(m_graphics_registers_ptr[m_graphics_address_register]));
}

void VGABase::IOGraphicsRegisterWrite(u8 value)
{
  static const u8 gr_mask[MAX_VGA_GRAPHICS_REGISTER + 1] = {
    0x0f, /* 0x00 */
    0x0f, /* 0x01 */
    0x0f, /* 0x02 */
    0x1f, /* 0x03 */
    0x03, /* 0x04 */
    0x7b, /* 0x05 */
    0x0f, /* 0x06 */
    0x0f, /* 0x07 */
    0xff, /* 0x08 */
  };

  if (m_graphics_address_register > MAX_VGA_GRAPHICS_REGISTER)
  {
    Log_ErrorPrintf("Out-of-range graphics register write: %u", u32(m_graphics_address_register));
    return;
  }

  m_graphics_registers_ptr[m_graphics_address_register] = value & gr_mask[m_graphics_address_register];

  Log_TracePrintf("Graphics register write: %u <- 0x%02X", u32(m_graphics_address_register), u32(value));
}

void VGABase::IOMiscOutputRegisterWrite(u8 value)
{
  Log_TracePrintf("Misc output register write: 0x%02X", u32(value));
  m_misc_output_register.bits = value;
  m_crtc_timing_changed = true;
}

void VGABase::IOAttributeAddressRead(u8* value)
{
  *value = m_attribute_address_register;
}

void VGABase::IOAttributeDataRead(u8* value)
{
  if (m_attribute_address_register > MAX_VGA_ATTRIBUTE_REGISTER)
  {
    Log_ErrorPrintf("Out-of-range attribute register read: %u", u32(m_attribute_address_register));
    *value = 0;
    return;
  }

  u8 register_index = m_attribute_address_register;
  *value = m_attribute_register_ptr[register_index];

  Log_TracePrintf("Attribute register read: %u -> 0x%02X", u32(register_index),
                  u32(m_attribute_register_ptr[register_index]));
}

void VGABase::IOAttributeAddressDataWrite(u8 value)
{
  if (!m_attribute_register_flipflop)
  {
    // This write is the address
    m_attribute_address_register = (value & 0x1F);
    m_attribute_register_flipflop = true;
    return;
  }

  // This write is the data
  m_attribute_register_flipflop = false;

  if (m_attribute_address_register > MAX_VGA_ATTRIBUTE_REGISTER)
  {
    Log_ErrorPrintf("Out-of-range attribute register write: %u", u32(m_attribute_address_register));
    return;
  }

  m_attribute_register_ptr[m_attribute_address_register] = value;

  Log_TracePrintf("Attribute register write: %u <- 0x%02X", u32(m_attribute_address_register), u32(value));

  // Mask text-mode palette indices to 6 bits
  if (m_attribute_address_register < 16)
    m_attribute_register_ptr[m_attribute_address_register] &= 0x3F;
}

void VGABase::IOSequencerDataRegisterRead(u8* value)
{
  if (m_sequencer_address_register > MAX_VGA_SEQUENCER_REGISTER)
  {
    Log_ErrorPrintf("Out-of-range sequencer register read: %u", u32(m_sequencer_address_register));
    *value = 0;
    return;
  }

  *value = m_sequencer_register_ptr[m_sequencer_address_register];

  Log_TracePrintf("Sequencer register read: %u -> 0x%02X", u32(m_sequencer_address_register),
                  u32(m_sequencer_register_ptr[m_sequencer_address_register]));
}

void VGABase::IOSequencerDataRegisterWrite(u8 value)
{
  /* force some bits to zero */
  const u8 sr_mask[8] = {
    0x03, 0x3d, 0x0f, 0x3f, 0x0e, 0x00, 0x00, 0xff,
  };

  if (m_sequencer_address_register > MAX_VGA_SEQUENCER_REGISTER)
  {
    Log_ErrorPrintf("Out-of-range sequencer register write: %u", u32(m_sequencer_address_register));
    return;
  }

  m_sequencer_register_ptr[m_sequencer_address_register] = value & sr_mask[m_sequencer_address_register];

  Log_TracePrintf("Sequencer register write: %u <- 0x%02X", u32(m_sequencer_address_register),
                  u32(m_sequencer_register_ptr[register_index]));

  if (m_sequencer_address_register == SEQUENCER_REGISTER_CLOCKING_MODE)
    m_crtc_timing_changed = true;
}

void VGABase::IODACStateRegisterRead(u8* value) // 3c7
{
  *value = m_dac_state_register;
  m_dac_state_register = 0;
}

void VGABase::IODACReadAddressWrite(u8 value) // 3c7
{
  Log_TracePrintf("DAC read address write: %u", value);
  m_dac_read_address = value;
  m_dac_state_register &= 0b00;
}

void VGABase::IODACWriteAddressRead(u8* value) // 3c8
{
  *value = m_dac_write_address;
  m_dac_state_register = 0;
}

void VGABase::IODACWriteAddressWrite(u8 value) // 3c8
{
  Log_TracePrintf("DAC write address write: %u", value);
  m_dac_write_address = value;
  m_dac_color_index = 0;
  m_dac_state_register |= 0b11;
}

void VGABase::IODACDataRegisterRead(u8* value) // 3c9
{
  u32 color_value = m_dac_palette[m_dac_read_address];
  u8 shift = m_dac_color_index * 8;
  *value = u8((color_value >> shift) & 0xFF);

  Log_TracePrintf("DAC palette read %u/%u: %u", u32(m_dac_read_address), u32(m_dac_color_index), u32(*value));

  m_dac_color_index++;
  if (m_dac_color_index >= 3)
  {
    m_dac_color_index = 0;
    m_dac_read_address = (m_dac_read_address + 1) % m_dac_palette.size();
  }

  m_dac_state_register = 0;
}

void VGABase::IODACDataRegisterWrite(u8 value) // 3c9
{
  Log_TracePrintf("DAC palette write %u/%u: %u", u32(m_dac_write_address), u32(m_dac_color_index), u32(value));

  // Mask away higher bits
  value &= 0x3F;

  u32 color_value = m_dac_palette[m_dac_write_address];
  u8 shift = m_dac_color_index * 8;
  color_value &= ~u32(0xFF << shift);
  color_value |= (u32(value) << shift);
  m_dac_palette[m_dac_write_address] = color_value;

  m_dac_color_index++;
  if (m_dac_color_index >= 3)
  {
    m_dac_color_index = 0;
    m_dac_write_address = (m_dac_write_address + 1) % m_dac_palette.size();
  }

  m_dac_state_register = 0;
}

// Values of 4-bit registers containing the plane mask expanded to 8 bits per plane.
static constexpr std::array<u32, 16> mask16 = {
  0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff, 0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff,
  0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff, 0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff,
};

bool VGABase::MapToVGAVRAMOffset(u32* offset_ptr)
{
  const u32 offset = *offset_ptr;
  switch (GRAPHICS_REGISTER_MISCELLANEOUS_MEMORY_MAP_SELECT(m_graphics_registers_ptr[GRAPHICS_REGISTER_MISCELLANEOUS]))
  {
    case 0: // A0000-BFFFF (128K)
    {
      return true;
    }

    case 1: // A0000-AFFFF (64K)
    {
      if (offset >= 0x10000)
        return false;

      *offset_ptr = offset;
      return true;
    }

    case 2: // B0000-B7FFF (32K)
    {
      if (offset < 0x10000 || offset >= 0x18000)
        return false;

      *offset_ptr = (offset - 0x10000);
      return true;
    }

    case 3: // B8000-BFFFF (32K)
    default:
    {
      if (offset < 0x18000)
        return false;

      *offset_ptr = (offset - 0x18000);
      return true;
    }
  }
}

void VGABase::HandleVGAVRAMRead(u32 segment_base, u32 offset, u8* value)
{
  if (!MapToVGAVRAMOffset(&offset))
  {
    // Out-of-range for the current mapping mode
    *value = 0xFF;
    return;
  }

  u8 read_plane;
  if (SEQUENCER_REGISTER_MEMORY_MODE_CHAIN_4(m_sequencer_register_ptr[SEQUENCER_REGISTER_MEMORY_MODE]))
  {
    // Chain4 mode - access all four planes as a series of linear bytes
    read_plane = Truncate8(offset & 3);
    std::memcpy(&m_latch, &m_vram[(segment_base + offset) & ~u32(3)], sizeof(m_latch));
  }
  else
  {
    u32 latch_planar_address;
    if (SEQUENCER_REGISTER_MEMORY_MODE_HOST_ODD_EVEN(m_sequencer_register_ptr[SEQUENCER_REGISTER_MEMORY_MODE]))
    {
      // By default we use the read map select register for the plane to return.
      read_plane = m_graphics_registers_ptr[GRAPHICS_REGISTER_READ_MAP_SELECT] & GRAPHICS_REGISTER_READ_MAP_SELECT_MASK;
      latch_planar_address = segment_base + offset;
    }
    else
    {
      // Except for odd/even addressing, only access planes 0/1.
      read_plane = (m_graphics_registers_ptr[GRAPHICS_REGISTER_READ_MAP_SELECT] & 0x02) | Truncate8(offset & 0x01);
      latch_planar_address = segment_base + (offset & ~u32(1));
    }

    // Use the offset to load the latches with all 4 planes.
    std::memcpy(&m_latch, &m_vram[latch_planar_address * 4], sizeof(m_latch));
  }

  // Compare value/mask mode?
  if (GRAPHICS_REGISTER_MODE_READ_MODE(m_graphics_registers_ptr[GRAPHICS_REGISTER_MODE]) != 0)
  {
    // Read mode 1 - compare value/mask
    u32 compare_result =
      (m_latch ^
       mask16[m_graphics_registers_ptr[GRAPHICS_REGISTER_COLOR_COMPARE] & GRAPHICS_REGISTER_COLOR_COMPARE_MASK]) &
      mask16[m_graphics_registers_ptr[GRAPHICS_REGISTER_COLOR_DONT_CARE] & GRAPHICS_REGISTER_COLOR_DONT_CARE_MASK];
    u8 ret = Truncate8(compare_result) | Truncate8(compare_result >> 8) | Truncate8(compare_result >> 16) |
             Truncate8(compare_result >> 24);
    *value = ~ret;
  }
  else
  {
    // Read mode 0 - return specified plane
    *value = Truncate8(m_latch >> (8 * read_plane));
  }
}

inline u32 VGALogicOp(u8 logic_op, u32 latch, u32 value)
{
  switch (logic_op)
  {
    case 0:
      return value;
    case 1:
      return value & latch;
    case 2:
      return value | latch;
    case 3:
      return value ^ latch;
    default:
      return value;
  }
}

constexpr u32 VGAExpandMask(u8 mask)
{
  return ZeroExtend32(mask) | (ZeroExtend32(mask) << 8) | (ZeroExtend32(mask) << 16) | (ZeroExtend32(mask) << 24);
}

void VGABase::HandleVGAVRAMWrite(u32 segment_base, u32 offset, u8 value)
{
  if (!MapToVGAVRAMOffset(&offset))
  {
    // Out-of-range for the current mapping mode
    return;
  }

  if (SEQUENCER_REGISTER_MEMORY_MODE_CHAIN_4(m_sequencer_register_ptr[SEQUENCER_REGISTER_MEMORY_MODE]))
  {
    // ET4000 differs from other SVGA hardware - chained write addresses go direct to memory addresses.
    u8 plane = Truncate8(offset & 3);
    if (m_sequencer_register_ptr[SEQUENCER_REGISTER_PLANE_MASK] & (1 << plane))
      m_vram[segment_base + offset] = value;
  }
  else if (SEQUENCER_REGISTER_MEMORY_MODE_HOST_ODD_EVEN(m_sequencer_register_ptr[SEQUENCER_REGISTER_MEMORY_MODE]))
  {
    u8 plane = Truncate8(offset & 1);
    if (m_sequencer_register_ptr[SEQUENCER_REGISTER_PLANE_MASK] & (1 << plane))
      m_vram[((segment_base + (offset & ~u32(1))) * 4) | plane] = value;
  }
  else
  {
    const u8 set_reset = m_graphics_registers_ptr[GRAPHICS_REGISTER_SET_RESET] & GRAPHICS_REGISTER_SET_RESET_MASK;
    const u8 enable_set_reset =
      m_graphics_registers_ptr[GRAPHICS_REGISTER_SET_RESET_ENABLE] & GRAPHICS_REGISTER_SET_RESET_ENABLE_MASK;
    const u8 bit_mask_index = m_graphics_registers_ptr[GRAPHICS_REGISTER_BIT_MASK];
    const u8 rotate_count =
      GRAPHICS_REGISTER_DATA_ROTATE_COUNT(m_graphics_registers_ptr[GRAPHICS_REGISTER_DATA_ROTATE]);
    const u8 logic_op = GRAPHICS_REGISTER_DATA_ROTATE_LOGIC_OP(m_graphics_registers_ptr[GRAPHICS_REGISTER_DATA_ROTATE]);

    u32 all_planes_value = 0;
    switch (GRAPHICS_REGISTER_MODE_WRITE_MODE(m_graphics_registers_ptr[GRAPHICS_REGISTER_MODE]))
    {
      case 0:
      {
        // The input byte is rotated right by the amount specified in Rotate Count, with all bits shifted off being
        // fed into bit 7
        u8 rotated = (value >> rotate_count) | (value << (8 - rotate_count));

        // The resulting byte is distributed over 4 separate paths, one for each plane of memory
        all_planes_value = VGAExpandMask(rotated);

        // If a bit in the Enable Set/Reset register is clear, the corresponding byte is left unmodified. Otherwise
        // the byte is replaced by all 0s if the corresponding bit in Set/Reset Value is clear, or all 1s if the bit
        // is one.
        all_planes_value =
          (all_planes_value & ~mask16[enable_set_reset]) | (mask16[set_reset] & mask16[enable_set_reset]);

        // The resulting value and the latch value are passed to the ALU
        all_planes_value = VGALogicOp(logic_op, m_latch, all_planes_value);

        // The Bit Mask Register is checked, for each set bit the corresponding bit from the ALU is forwarded. If the
        // bit is clear the bit is taken directly from the Latch.
        u32 bit_mask = VGAExpandMask(bit_mask_index);
        all_planes_value = (all_planes_value & bit_mask) | (m_latch & ~bit_mask);
      }
      break;

      case 1:
      {
        // In this mode, data is transferred directly from the 32 bit latch register to display memory, affected only
        // by the Memory Plane Write Enable field. The host data is not used in this mode.
        all_planes_value = m_latch;
      }
      break;

      case 2:
      {
        // In this mode, the bits 3-0 of the host data are replicated across all 8 bits of their respective planes.
        all_planes_value = mask16[value & 0x0F];

        // Then the selected Logical Operation is performed on the resulting data and the data in the latch register.
        all_planes_value = VGALogicOp(logic_op, m_latch, all_planes_value);

        // Then the Bit Mask field is used to select which bits come from the resulting data and which come from the
        // latch register.
        u32 bit_mask = VGAExpandMask(bit_mask_index);
        all_planes_value = (all_planes_value & bit_mask) | (m_latch & ~bit_mask);
      }
      break;

      case 3:
      {
        // In this mode, the data in the Set/Reset field is used as if the Enable Set/Reset field were set to 1111b.
        u32 set_reset_data = mask16[set_reset];

        // Then the host data is first rotated as per the Rotate Count field, then logical ANDed with the value of the
        // Bit Mask field.
        u8 rotated = (value >> rotate_count) | (value << (8 - rotate_count));
        u8 temp_bit_mask = bit_mask_index & rotated;

        // Apply logical operation.
        all_planes_value = VGALogicOp(logic_op, m_latch, set_reset_data);

        // The resulting value is used on the data obtained from the Set/Reset field in the same way that the Bit Mask
        // field would ordinarily be used to select which bits come from the expansion of the Set/Reset field and
        // which come from the latch register.
        u32 bit_mask = VGAExpandMask(temp_bit_mask);
        all_planes_value = (all_planes_value & bit_mask) | (m_latch & ~bit_mask);
      }
      break;
    }

    // Finally, only the bit planes enabled by the Memory Plane Write Enable field are written to memory.
    u32 write_mask = mask16[m_sequencer_register_ptr[SEQUENCER_REGISTER_PLANE_MASK] & 0xF];
    u32 current_value;
    std::memcpy(&current_value, &m_vram[(segment_base + offset) * 4], sizeof(current_value));
    all_planes_value = (all_planes_value & write_mask) | (current_value & ~write_mask);
    std::memcpy(&m_vram[(segment_base + offset) * 4], &all_planes_value, sizeof(current_value));
  }
}

void VGABase::SetOutputPalette16()
{
  for (u32 i = 0; i < 16; i++)
  {
    u32 index = ZeroExtend32(m_attribute_register_ptr[i]);

    // Control whether the color select controls the high bits or the palette index.
    if (m_attribute_registers.attribute_mode_control.palette_bits_5_4_select)
      index = ((m_attribute_registers.color_select & 0x0F) << 4) | (index & 0x0F);
    else
      index = ((m_attribute_registers.color_select & 0x0C) << 4) | (index & 0x3F);

    m_output_palette[i] = Convert6BitColorTo8Bit(m_dac_palette[index]);
  }
}

void VGABase::SetOutputPalette256()
{
  for (u32 i = 0; i < 256; i++)
  {
    m_output_palette[i] = Convert6BitColorTo8Bit(m_dac_palette[i]);
  }
}

u32 VGABase::CRTCReadVRAMPlanes(u32 address_counter, u32 row_scan_counter) const
{
  u32 address = CRTCWrapAddress(address_counter, row_scan_counter);
  u32 vram_offset = (address * 4) & m_vram_mask;
  u32 all_planes;
  std::memcpy(&all_planes, &m_vram[vram_offset], sizeof(all_planes));

  u32 plane_mask = mask16[m_attribute_register_ptr[ATTRIBUTE_REGISTER_COLOR_PLANE_ENABLE] &
                          ATTRIBUTE_REGISTER_COLOR_PLANE_ENABLE_MASK];

  return all_planes & plane_mask;
}

u32 VGABase::CRTCWrapAddress(u32 address_counter, u32 row_scan_counter) const
{
  u32 address;
  if (m_crtc_registers.underline_location & 0x40)
  {
    // Double-word mode
    // address = ((address_counter << 2) | ((address_counter >> 14) & 0x3)) & (VRAM_SIZE - 1);
    address = address_counter;
  }
  else if (!m_crtc_registers.crtc_mode_control.byte_mode)
  {
    // Word mode
    if (m_crtc_registers.crtc_mode_control.alternate_ma00_output)
      address = ((address_counter << 1) | ((address_counter >> 15) & 0x1)) & VRAM_MASK_PER_PLANE;
    else
      address = ((address_counter << 1) | ((address_counter >> 13) & 0x1)) & VRAM_MASK_PER_PLANE;
  }
  else
  {
    // Byte mode
    address = address_counter & VRAM_MASK_PER_PLANE;
  }

  // This bit selects the source of bit 13 of the output multiplexer. When this bit is set to 0, bit 0 of the row scan
  // counter is the source, and when this bit is set to 1, bit 13 of the address counter is the source.
  if (!m_crtc_registers.crtc_mode_control.alternate_la13)
    address = (address & ~u32(1 << 13)) | ((row_scan_counter & 1) << 13);

  // This bit selects the source of bit 14 of the output multiplexer. When this bit is set to 0, bit 1 of the row scan
  // counter is the source, and when this bit is set to 1, bit 13 of the address counter is the source.
  if (!m_crtc_registers.crtc_mode_control.alternate_la14)
    address = (address & ~u32(1 << 14)) | ((row_scan_counter & 2) << 13);

  return address;
}

void VGABase::RenderTextMode()
{
  const u32 character_height = m_crtc_registers.GetScanlinesPerRow();
  const u32 character_width = m_sequencer_registers.GetCharacterWidth();
  m_character_width u32 character_columns = m_crtc_registers.GetHorizontalDisplayed();
  u32 character_rows = m_crtc_registers.GetVerticalDisplayed();
  character_rows = (character_rows + 1) / character_height;
  if (m_crtc_registers.vertical_total == 100)
    character_rows = 100;

  if (character_columns == 0 || character_rows == 0)
    return;

  u32 screen_width = character_columns * character_width;
  u32 screen_height = character_rows * character_height;
  if (screen_width == 0 || screen_height == 0)
    return;

  if (m_display->GetFramebufferWidth() != screen_width || m_display->GetFramebufferHeight() != screen_height ||
      m_last_rendered_vertical_frequency != m_timing.vertical_frequency)
  {
    m_system->GetHostInterface()->ReportFormattedMessage("Screen format changed: %ux%u @ %.1f hz", screen_width,
                                                         screen_height, m_timing.vertical_frequency);

    m_last_rendered_vertical_frequency = m_timing.vertical_frequency;
    m_display->ResizeFramebuffer(screen_width, screen_height);
    m_display->ResizeDisplay();
  }

  // preset_row_scan[4:0] contains the starting row scan number, cleared when it hits max.
  // u32 row_counter = 0;
  u32 row_scan_counter = ZeroExtend32(m_crtc_registers.preset_row_scan & 0x1F);

  // Determine base address of the fonts
  u32 font_base_address[2];
  const u8* font_base_pointers[2];
  for (u32 i = 0; i < 2; i++)
  {
    u32 field;
    if (i == 0)
    {
      field = m_sequencer_registers.character_set_b_select_01 | (m_sequencer_registers.character_set_b_select_2 << 2);
    }
    else
    {
      field = m_sequencer_registers.character_set_a_select_01 | (m_sequencer_registers.character_set_a_select_2 << 2);
    }

    switch (field)
    {
      case 0b000:
        font_base_address[i] = 0x0000;
        break;
      case 0b001:
        font_base_address[i] = 0x4000;
        break;
      case 0b010:
        font_base_address[i] = 0x8000;
        break;
      case 0b011:
        font_base_address[i] = 0xC000;
        break;
      case 0b100:
        font_base_address[i] = 0x2000;
        break;
      case 0b101:
        font_base_address[i] = 0x6000;
        break;
      case 0b110:
        font_base_address[i] = 0xA000;
        break;
      case 0b111:
        font_base_address[i] = 0xE000;
        break;
    }

    font_base_pointers[i] = &m_vram[font_base_address[i] * 4];
  }

  // Get text palette colours
  SetOutputPalette16();

  // Determine the starting address in VRAM of the text/attribute data from the CRTC registers
  u32 data_base_address =
    (ZeroExtend32(m_crtc_registers.start_address_high) << 8) | (ZeroExtend32(m_crtc_registers.start_address_low));
  //     u32 line_compare = (ZeroExtend32(m_crtc_registers.line_compare)) |
  //                           (ZeroExtend32(m_crtc_registers.overflow_register & 0x10) << 4) |
  //                           (ZeroExtend32(m_crtc_registers.maximum_scan_lines & 0x40) << 3);
  u32 row_pitch = ZeroExtend32(m_crtc_registers.offset) * 2;

  // Cursor setup
  u32 cursor_address =
    (ZeroExtend32(m_crtc_registers.cursor_location_high) << 8) | (ZeroExtend32(m_crtc_registers.cursor_location_low));
  u32 cursor_start_line = ZeroExtend32(m_crtc_registers.cursor_start & 0x1F);
  u32 cursor_end_line = ZeroExtend32(m_crtc_registers.cursor_end & 0x1F) + 1;

  // If the cursor is disabled, set the address to something that will never be equal
  if (m_crtc_registers.cursor_start & (1 << 5) || !m_cursor_state)
    cursor_address = m_vram_size;

  // Draw
  for (u32 row = 0; row < character_rows; row++)
  {
    // DebugAssert((vram_offset + (character_columns * 2)) <= VRAM_SIZE);

    u32 address_counter = data_base_address + (row_pitch * row);
    for (u32 col = 0; col < character_columns; col++)
    {
      // Read as dwords, with each byte representing one plane
      u32 current_address = address_counter++;
      u32 all_planes = CRTCReadVRAMPlanes(current_address, row_scan_counter);

      u8 character = Truncate8(all_planes >> 0);
      u8 attribute = Truncate8(all_planes >> 8);

      // Grab foreground and background colours
      u32 foreground_color = m_output_palette[(attribute & 0xF)];
      u32 background_color = m_output_palette[(attribute >> 4) & 0xF];

      // Offset into font table to get glyph, bit 4 determines the font to use
      // 32 bytes per character in the font bitmap, 4 bytes per plane, data in plane 2.
      const u8* glyph = font_base_pointers[(attribute >> 3) & 0x01] + (character * 32 * 4) + 2;

      // Actually draw the character
      int32 dup9 = (character >= 0xC0 && character <= 0xDF) ? 1 : 0;
      switch (character_width)
      {
        default:
        case 8:
          DrawTextGlyph8(col * character_width, row * character_height, glyph, character_height, foreground_color,
                         background_color, -1);
          break;

        case 9:
          DrawTextGlyph8(col * character_width, row * character_height, glyph, character_height, foreground_color,
                         background_color, dup9);
          break;

        case 16:
          DrawTextGlyph16(col * character_width, row * character_height, glyph, character_height, foreground_color,
                          background_color);
          break;
      }

      // To draw the cursor, we simply overwrite the pixels. Easier than branching in the character draw routine.
      if (current_address == cursor_address)
      {
        // On the standard ET4000, the cursor color is obtained from the foreground color of the character that the
        // cursor is superimposing. On the standard VGA there is no way to modify this behavior.
        // TODO: How is dup9 handled here?
        cursor_start_line = std::min(cursor_start_line, character_height);
        cursor_end_line = std::min(cursor_end_line, character_height);
        for (u32 cursor_line = cursor_start_line; cursor_line < cursor_end_line; cursor_line++)
        {
          for (u32 i = 0; i < character_width; i++)
            m_display->SetPixel(col * character_width + i, row * character_height + cursor_line, foreground_color);
        }
      }
    }
  }

  m_display->SwapFramebuffer();
}

void VGABase::DrawTextGlyph8(u32 fb_x, u32 fb_y, const u8* glyph, u32 rows, u32 fg_color, u32 bg_color, int32 dup9)
{
  const u32 colors[2] = {bg_color, fg_color};

  for (u32 row = 0; row < rows; row++)
  {
    u8 source_row = *glyph;
    m_display->SetPixel(fb_x + 0, fb_y + row, colors[(source_row >> 7) & 1]);
    m_display->SetPixel(fb_x + 1, fb_y + row, colors[(source_row >> 6) & 1]);
    m_display->SetPixel(fb_x + 2, fb_y + row, colors[(source_row >> 5) & 1]);
    m_display->SetPixel(fb_x + 3, fb_y + row, colors[(source_row >> 4) & 1]);
    m_display->SetPixel(fb_x + 4, fb_y + row, colors[(source_row >> 3) & 1]);
    m_display->SetPixel(fb_x + 5, fb_y + row, colors[(source_row >> 2) & 1]);
    m_display->SetPixel(fb_x + 6, fb_y + row, colors[(source_row >> 1) & 1]);
    m_display->SetPixel(fb_x + 7, fb_y + row, colors[(source_row >> 0) & 1]);

    if (dup9 == 0)
      m_display->SetPixel(fb_x + 8, fb_y + row, bg_color);
    else if (dup9 > 0)
      m_display->SetPixel(fb_x + 8, fb_y + row, colors[(source_row >> 0) & 1]);

    // Have to read the second plane, so offset by 4
    glyph += 4;
  }
}

void VGABase::DrawTextGlyph16(u32 fb_x, u32 fb_y, const u8* glyph, u32 rows, u32 fg_color, u32 bg_color)
{
  const u32 colors[2] = {bg_color, fg_color};

  for (u32 row = 0; row < rows; row++)
  {
    u8 source_row = *glyph;
    m_display->SetPixel(fb_x + 0, fb_y + row, colors[(source_row >> 7) & 1]);
    m_display->SetPixel(fb_x + 1, fb_y + row, colors[(source_row >> 7) & 1]);
    m_display->SetPixel(fb_x + 2, fb_y + row, colors[(source_row >> 6) & 1]);
    m_display->SetPixel(fb_x + 3, fb_y + row, colors[(source_row >> 6) & 1]);
    m_display->SetPixel(fb_x + 4, fb_y + row, colors[(source_row >> 5) & 1]);
    m_display->SetPixel(fb_x + 5, fb_y + row, colors[(source_row >> 5) & 1]);
    m_display->SetPixel(fb_x + 6, fb_y + row, colors[(source_row >> 4) & 1]);
    m_display->SetPixel(fb_x + 7, fb_y + row, colors[(source_row >> 4) & 1]);
    m_display->SetPixel(fb_x + 8, fb_y + row, colors[(source_row >> 3) & 1]);
    m_display->SetPixel(fb_x + 9, fb_y + row, colors[(source_row >> 3) & 1]);
    m_display->SetPixel(fb_x + 10, fb_y + row, colors[(source_row >> 2) & 1]);
    m_display->SetPixel(fb_x + 11, fb_y + row, colors[(source_row >> 2) & 1]);
    m_display->SetPixel(fb_x + 12, fb_y + row, colors[(source_row >> 1) & 1]);
    m_display->SetPixel(fb_x + 13, fb_y + row, colors[(source_row >> 1) & 1]);
    m_display->SetPixel(fb_x + 14, fb_y + row, colors[(source_row >> 0) & 1]);
    m_display->SetPixel(fb_x + 15, fb_y + row, colors[(source_row >> 0) & 1]);

    // Have to read the second plane, so offset by 4
    glyph += 4;
  }
}

void VGABase::RenderGraphicsMode()
{
  u32 screen_width = m_crtc_registers.GetHorizontalDisplayed() * m_sequencer_registers.GetCharacterWidth();
  u32 screen_height = m_crtc_registers.GetVerticalDisplayed();
  u32 scanlines_per_row = m_crtc_registers.GetScanlinesPerRow();
  u32 line_compare = m_crtc_registers.GetLineCompare();
  if (screen_width == 0 || screen_height == 0)
    return;

  // 200-line EGA/VGA modes set scanlines_per_row to 2, creating an effective 400 lines.
  // We can speed things up by only rendering one of these lines, if the only muxes which
  // use the scanline counter are enabled (alternative LA13/14).
  if (scanlines_per_row == 2 && (line_compare > screen_height || (line_compare & 1) == 0) &&
      m_crtc_registers.preset_row_scan == 0 && m_crtc_registers.crtc_mode_control.alternate_la13 &&
      m_crtc_registers.crtc_mode_control.alternate_la14)
  {
    scanlines_per_row = 1;
    screen_height /= 2;
    line_compare /= 2;
  }

  // Scan doubling, used for CGA modes
  // This causes the row scan counter to increment at half the rate, so when scanlines_per_row = 1,
  // address counter is always divided by two as well (for CGA modes).
  if (m_crtc_registers.scan_doubling)
    screen_height /= 2;

  // Halving the pixel clock halves the effective resolution. This is used by mode 13h.
  if (m_attribute_registers.attribute_mode_control.pelclock_div2)
    screen_width /= 2;

  // To get VGA modes, we divide the main clock by 2. To get hicolor mode, it runs full speed.
  if (!m_sequencer_registers.mclk_div2)
    screen_width /= 2;

  // Update framebuffer size before drawing to it
  if (m_display->GetFramebufferWidth() != screen_width || m_display->GetFramebufferHeight() != screen_height ||
      m_last_rendered_vertical_frequency != m_timing.vertical_frequency)
  {
    m_system->GetHostInterface()->ReportFormattedMessage("Screen format changed: %ux%u @ %.1f hz", screen_width,
                                                         screen_height, m_timing.vertical_frequency);

    m_last_rendered_vertical_frequency = m_timing.vertical_frequency;
    m_display->ResizeFramebuffer(screen_width, screen_height);
    m_display->ResizeDisplay();
  }

  // 4 or 16 color mode?
  if (!m_graphics_registers.mode.shift_256)
  {
    // This initializes 16 colours when we only need 4, but whatever.
    SetOutputPalette16();
  }
  else
  {
    // Initialize all palette colours beforehand.
    SetOutputPalette256();
  }

  // Determine the starting address in VRAM of the data from the CRTC registers
  // This should be multiplied by 4 when accessing because we store interleave all planes.
  u32 data_base_address = m_crtc_registers.GetStartAddress();
  data_base_address += m_crtc_registers.byte_panning;

  // Determine the pitch of each line
  u32 row_pitch = ZeroExtend32(m_crtc_registers.offset) * 2;

  u32 horizontal_pan = m_attribute_registers.horizontal_pixel_panning;
  if (horizontal_pan >= 8)
    horizontal_pan = 0;

  // preset_row_scan[4:0] contains the starting row scan number, cleared when it hits max.
  u32 row_counter = 0;
  u32 row_scan_counter = m_crtc_registers.preset_row_scan;

  // Draw lines
  for (u32 scanline = 0; scanline < screen_height; scanline++)
  {
    if (scanline == line_compare)
    {
      // TODO: pixel_panning_mode determines whether to reset horizontal_pan
      data_base_address = 0;
      row_counter = 0;
      row_scan_counter = 0;
      horizontal_pan = 0;
    }

    u32 address_counter = (data_base_address + (row_pitch * row_counter));

    // High colour modes
    // This is a hack. The palette should be disabled, and we should read like shift mode?
    if ((m_dac_ctrl & 0xc0) != 0)
    {
      const bool is_16bpp = !!(m_dac_ctrl & 0x40);
      for (u32 col = 0; col < screen_width;)
      {
        // Two pixels per dword
        u32 all_planes = CRTCReadVRAMPlanes(address_counter++, row_scan_counter);
        u32 color1, color2;
        if (is_16bpp)
        {
          color1 = ConvertBGR565ToRGB24(Truncate16(all_planes));
          color2 = ConvertBGR565ToRGB24(Truncate16(all_planes >> 16));
        }
        else
        {
          color1 = ConvertBGR555ToRGB24(Truncate16(all_planes));
          color2 = ConvertBGR555ToRGB24(Truncate16(all_planes >> 16));
        }

        m_display->SetPixel(col++, scanline, color1);
        m_display->SetPixel(col++, scanline, color2);
      }
    }
    // 4 or 16 color mode?
    else if (!m_graphics_registers.mode.shift_256)
    {
      if (m_graphics_registers.mode.shift_reg)
      {
        // CGA mode - Shift register in interleaved mode, odd bits from odd maps and even bits from even maps
        for (u32 col = 0; col < screen_width;)
        {
          u32 all_planes = CRTCReadVRAMPlanes(address_counter, row_scan_counter);
          address_counter++;

          u8 pl0 = Truncate8((all_planes >> 0) & 0xFF);
          u8 pl1 = Truncate8((all_planes >> 8) & 0xFF);
          u8 pl2 = Truncate8((all_planes >> 16) & 0xFF);
          u8 pl3 = Truncate8((all_planes >> 24) & 0xFF);
          u8 index;

          // One pixel per input pixel
          index = ((pl0 >> 6) & 3) | (((pl2 >> 6) & 3) << 2);
          m_display->SetPixel(col++, scanline, m_output_palette[index]);
          index = ((pl0 >> 4) & 3) | (((pl2 >> 6) & 3) << 2);
          m_display->SetPixel(col++, scanline, m_output_palette[index]);
          index = ((pl0 >> 2) & 3) | (((pl2 >> 6) & 3) << 2);
          m_display->SetPixel(col++, scanline, m_output_palette[index]);
          index = ((pl0 >> 0) & 3) | (((pl2 >> 6) & 3) << 2);
          m_display->SetPixel(col++, scanline, m_output_palette[index]);

          index = ((pl1 >> 6) & 3) | (((pl3 >> 6) & 3) << 2);
          m_display->SetPixel(col++, scanline, m_output_palette[index]);
          index = ((pl1 >> 4) & 3) | (((pl3 >> 6) & 3) << 2);
          m_display->SetPixel(col++, scanline, m_output_palette[index]);
          index = ((pl1 >> 2) & 3) | (((pl3 >> 6) & 3) << 2);
          m_display->SetPixel(col++, scanline, m_output_palette[index]);
          index = ((pl1 >> 0) & 3) | (((pl3 >> 6) & 3) << 2);
          m_display->SetPixel(col++, scanline, m_output_palette[index]);
        }
      }
      else
      {
        // 16 color mode.
        // Output 8 pixels for one dword
        for (int32 col = -(int32)horizontal_pan; col < (int32)screen_width;)
        {
          u32 all_planes = CRTCReadVRAMPlanes(address_counter, row_scan_counter);
          address_counter++;

          u8 pl0 = Truncate8((all_planes >> 0) & 0xFF);
          u8 pl1 = Truncate8((all_planes >> 8) & 0xFF);
          u8 pl2 = Truncate8((all_planes >> 16) & 0xFF);
          u8 pl3 = Truncate8((all_planes >> 24) & 0xFF);

          u8 indices[8] = {
            u8(((pl0 >> 7) & 1u) | (((pl1 >> 7) & 1u) << 1) | (((pl2 >> 7) & 1u) << 2) | (((pl3 >> 7) & 1u) << 3)),
            u8(((pl0 >> 6) & 1u) | (((pl1 >> 6) & 1u) << 1) | (((pl2 >> 6) & 1u) << 2) | (((pl3 >> 6) & 1u) << 3)),
            u8(((pl0 >> 5) & 1u) | (((pl1 >> 5) & 1u) << 1) | (((pl2 >> 5) & 1u) << 2) | (((pl3 >> 5) & 1u) << 3)),
            u8(((pl0 >> 4) & 1u) | (((pl1 >> 4) & 1u) << 1) | (((pl2 >> 4) & 1u) << 2) | (((pl3 >> 4) & 1u) << 3)),
            u8(((pl0 >> 3) & 1u) | (((pl1 >> 3) & 1u) << 1) | (((pl2 >> 3) & 1u) << 2) | (((pl3 >> 3) & 1u) << 3)),
            u8(((pl0 >> 2) & 1u) | (((pl1 >> 2) & 1u) << 1) | (((pl2 >> 2) & 1u) << 2) | (((pl3 >> 2) & 1u) << 3)),
            u8(((pl0 >> 1) & 1u) | (((pl1 >> 1) & 1u) << 1) | (((pl2 >> 1) & 1u) << 2) | (((pl3 >> 1) & 1u) << 3)),
            u8(((pl0 >> 0) & 1u) | (((pl1 >> 0) & 1u) << 1) | (((pl2 >> 0) & 1u) << 2) | (((pl3 >> 0) & 1u) << 3))};

          for (u32 subindex = 0; col < (int32)screen_width && subindex < 8;)
          {
            if (col >= 0 && col < (int32)screen_width)
              m_display->SetPixel(col, scanline, m_output_palette[indices[subindex]]);

            col++;
            subindex++;
          }
        }
      }
    }
    else
    {
      u32 pan_pixels = (horizontal_pan & 7) / 2;

      // Slow loop with panning part
      int32 col = -int32(pan_pixels * 2);
      int32 screen_width_div2 = int32(screen_width);
      while (col < 0)
      {
        u32 indices = CRTCReadVRAMPlanes(address_counter, row_scan_counter);
        address_counter++;

        for (u32 i = 0; i < 4; i++)
        {
          u8 index = Truncate8(indices);
          u32 color = m_output_palette[index];
          indices >>= 8;

          if (col >= 0)
            m_display->SetPixel(col, scanline, color);

          col++;
        }
      }

      // Fast loop without partial panning
      while ((col + 4) <= screen_width_div2)
      {
        // Load 4 pixels, one from each plane
        // Duplicate horizontally twice, this is the shift_256 stuff
        u32 indices = CRTCReadVRAMPlanes(address_counter, row_scan_counter);
        address_counter++;

        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 0) & 0xFF]);
        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 8) & 0xFF]);
        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 16) & 0xFF]);
        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 24) & 0xFF]);
      }

      // Slow loop to handle misaligned buffer when panning
      while (col < screen_width_div2)
      {
        u32 indices = CRTCReadVRAMPlanes(address_counter, row_scan_counter);
        address_counter++;

        for (u32 i = 0; i < 4; i++)
        {
          u8 index = Truncate8(indices);
          u32 color = m_output_palette[index];
          indices >>= 8;

          if (col < screen_width_div2)
            m_display->SetPixel(col, scanline, color);
          else
            break;

          col++;
        }
      }
    }

    row_scan_counter++;
    if (row_scan_counter == scanlines_per_row)
    {
      row_scan_counter = 0;
      row_counter++;
    }
  }

  m_display->SwapFramebuffer();
}
} // namespace HW