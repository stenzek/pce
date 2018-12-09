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

  m_attribute_register_flipflop = false;

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
  reader.SafeReadUInt8(&m_cursor_counter);
  reader.SafeReadBool(&m_cursor_state);

  CRTCTimingChanged();

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

void VGABase::IOCRTCDataRegisterRead(u8* value)
{
  if (m_crtc_index_register > MAX_VGA_CRTC_REGISTER)
  {
    Log_ErrorPrintf("Out-of-range CRTC register read: %u", u32(m_crtc_index_register));
    *value = 0;
    return;
  }

  *value = m_crtc_registers_ptr[m_crtc_index_register];
  Log_TracePrintf("CRTC register read: %u -> 0x%02X", u32(m_crtc_index_register),
                  u32(m_crtc_registers_ptr[m_crtc_index_register]));
}

void VGABase::IOCRTCDataRegisterWrite(u8 value)
{
  if (m_crtc_index_register > MAX_VGA_CRTC_REGISTER)
  {
    Log_ErrorPrintf("Out-of-range CRTC register write: %u", u32(m_crtc_index_register));
    return;
  }

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
      CRTCTimingChanged();
      break;
  }
}

void VGABase::CRTCTimingChanged()
{
  if (!m_display_event->IsActive())
  {
    m_display_event->SetFrequency(60.0f);
    m_display_event->Activate();
  }

  m_crtc_timing_changed = true;
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
  CRTCTimingChanged();
}

void VGABase::IOReadStatusRegister0(u8* value)
{
  union
  {
    u8 bits;
    BitField<u8, bool, 4, 1> switch_sense;
  } val = {};

  val.switch_sense = true;

  *value = val.bits;
}

void VGABase::IOReadStatusRegister1(u8* value)
{
  union
  {
    u8 bits;
    BitField<u8, bool, 3, 1> vertical_retrace;
    BitField<u8, bool, 0, 1> display_disabled;
  } val = {};

  const DisplayTiming::Snapshot ss = m_display_timing.GetSnapshot(m_system->GetTimingManager()->GetTotalEmulatedTime());
  val.display_disabled = !ss.display_active;
  val.vertical_retrace = ss.vsync_active;

  *value = val.bits;

  m_attribute_register_flipflop = false;
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

  Log_TracePrintf("Sequencer register write: %u <- 0x%02X", u32(m_sequencer_address_register), value);

  if (m_sequencer_address_register == SEQUENCER_REGISTER_CLOCKING_MODE)
    CRTCTimingChanged();
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
    std::memcpy(&m_latch, &m_vram_ptr[(segment_base + offset) & ~u32(3)], sizeof(m_latch));
  }
  else
  {
    u32 latch_planar_address;
    if (!GRAPHICS_REGISTER_MISCELLANEOUS_CHAIN_ODD_EVEN_ENABLE(
          m_graphics_registers_ptr[GRAPHICS_REGISTER_MISCELLANEOUS]))
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
    std::memcpy(&m_latch, &m_vram_ptr[latch_planar_address * 4], sizeof(m_latch));
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
      m_vram_ptr[segment_base + offset] = value;
  }
  else if (!SEQUENCER_REGISTER_MEMORY_MODE_HOST_ODD_EVEN(m_sequencer_register_ptr[SEQUENCER_REGISTER_MEMORY_MODE]))
  {
    u8 plane = Truncate8(offset & 1);
    if (m_sequencer_register_ptr[SEQUENCER_REGISTER_PLANE_MASK] & (1 << plane))
      m_vram_ptr[((segment_base + (offset & ~u32(1))) * 4) | plane] = value;
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
    std::memcpy(&current_value, &m_vram_ptr[(segment_base + offset) * 4], sizeof(current_value));
    all_planes_value = (all_planes_value & write_mask) | (current_value & ~write_mask);
    std::memcpy(&m_vram_ptr[(segment_base + offset) * 4], &all_planes_value, sizeof(current_value));
  }
}

void VGABase::SetOutputPalette16()
{
  const u8 color_select = m_attribute_register_ptr[ATTRIBUTE_REGISTER_COLOR_SELECT];

  for (u32 i = 0; i < 16; i++)
  {
    u32 index = ZeroExtend32(m_attribute_register_ptr[i]);

    // Control whether the color select controls the high bits or the palette index.
    if (m_attribute_register_ptr[ATTRIBUTE_REGISTER_MODE] & ATTRIBUTE_REGISTER_MODE_PALETTE_BITS_5_4)
      index = ((color_select & 0x0F) << 4) | (index & 0x0F);
    else
      index = ((color_select & 0x0C) << 4) | (index & 0x3F);

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

void VGABase::UpdateDisplayTiming()
{
  DisplayTiming new_timing;
  u32 render_width, render_height;
  GetDisplayTiming(new_timing, &render_width, &render_height);

  if (m_display_timing.FrequenciesMatch(new_timing) && m_display->GetFramebufferWidth() == render_width &&
      m_display->GetFramebufferHeight() == render_height)
  {
    return;
  }

  m_display_timing = new_timing;
  if (!m_display_timing.IsValid())
    return;

  Log_DevPrintf("VGA: %ux%u @ %.2f hz (%.2f KHz)", render_width, render_height, m_display_timing.GetVerticalFrequency(),
                m_display_timing.GetHorizontalFrequency() / 1000.0);

  m_display_timing.SetClockEnable(true);
  m_display_timing.ResetClock(m_system->GetTimingManager()->GetTotalEmulatedTime());
  m_display->UpdateFramebuffer(render_width, render_height, m_display->GetFramebufferFormat(),
                               static_cast<float>(m_display_timing.GetVerticalFrequency()));
  m_display->ResizeDisplay();

  m_display_event->SetFrequency(static_cast<float>(m_display_timing.GetVerticalFrequency()));
}

void VGABase::GetDisplayTiming(DisplayTiming& timing, u32* render_width, u32* render_height)
{
  const bool dot_clock_div2 =
    SEQUENCER_REGISTER_CLOCKING_MODE_DOT_CLOCK_DIV2(m_sequencer_register_ptr[SEQUENCER_REGISTER_CLOCKING_MODE]);

  // Pixels clocks. 0 - 25MHz, 1 - 28Mhz, 2/3 - undefined
  static constexpr std::array<u32, 4> pixel_clocks = {{25175000, 28322000, 25175000, 25175000}};
  timing.SetPixelClock(pixel_clocks[m_misc_output_register.clock_select]);

  u32 horizontal_visible = ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_HORIZONTAL_DISPLAY_END]) + 1u;
  u32 horizontal_total = ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_HORIZONTAL_TOTAL]) + 5u;
  u32 horizontal_sync_start = ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_HORIZONTAL_SYNC_START]);
  u32 horizontal_sync_end = ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_HORIZONTAL_SYNC_END] & 0x1F);

  // No idea if this is correct, but it seems to be the only way to get a correct sync length in 40x25 modes..
  if (dot_clock_div2)
  {
    horizontal_visible *= 2;
    horizontal_total *= 2;
    horizontal_sync_start *= 2;
    horizontal_sync_end *= 2;
  }

  u32 horizontal_sync_length = ((horizontal_sync_end - (horizontal_sync_start & 0x1F)) & 0x1F);

  u32 character_width =
    SEQUENCER_REGISTER_CLOCKING_MODE_DOT8(m_sequencer_register_ptr[SEQUENCER_REGISTER_CLOCKING_MODE]) ? 8 : 9;

  timing.SetHorizontalVisible(horizontal_visible * character_width);
  timing.SetHorizontalSyncLength(horizontal_sync_start * character_width, horizontal_sync_length * character_width);
  timing.SetHorizontalTotal(horizontal_total * character_width);

  const u32 overflow = ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_OVERFLOW]);
  const u32 vertical_visible = (ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_VERTICAL_DISPLAY_END]) |
                                (((overflow >> 6) & 1u) << 9) | (((overflow >> 1u) & 1u) << 8)) +
                               1u;
  const u32 vertical_sync_start = (ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_VERTICAL_SYNC_START]) |
                                   (((overflow >> 7) & 1u) << 9) | (((overflow >> 2u) & 1u) << 8));
  const u32 vertical_sync_length = ZeroExtend32(((m_crtc_registers_ptr[CRTC_REGISTER_VERTICAL_SYNC_END] & 0x0F) -
                                                 (m_crtc_registers_ptr[CRTC_REGISTER_VERTICAL_SYNC_START] & 0x0F)) &
                                                0x0F);
  const u32 vertical_total = (ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_VERTICAL_TOTAL]) |
                              (((overflow >> 5) & 1u) << 9) | ((overflow & 1u) << 8)) +
                             2u;

  timing.SetVerticalVisible(vertical_visible);
  timing.SetVerticalSyncLength(vertical_sync_start, vertical_sync_length);
  timing.SetVerticalTotal(vertical_total);

  *render_width = horizontal_visible * character_width;
  *render_height = vertical_visible;

  if (dot_clock_div2)
    *render_width /= 2;

  // the actual dimensions we render don't include double-scanning.
  if (m_crtc_registers_ptr[CRTC_REGISTER_CHARACTER_CELL_HEIGHT] & 0x80)
    *render_height /= 2;
}

void VGABase::LatchStartAddress()
{
  m_render_latch.start_address = (ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_START_ADDRESS_HIGH]) << 8) |
                                 (ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_START_ADDRESS_LOW]));
  m_render_latch.start_address += ZeroExtend32((m_crtc_registers_ptr[CRTC_REGISTER_PRESET_ROW_SCAN] >> 5) & 0x03);

  m_render_latch.character_width =
    SEQUENCER_REGISTER_CLOCKING_MODE_DOT8(m_sequencer_register_ptr[SEQUENCER_REGISTER_CLOCKING_MODE]) ? 8 : 9;
  m_render_latch.character_height = (m_crtc_registers_ptr[CRTC_REGISTER_CHARACTER_CELL_HEIGHT] & 0x1F) + 1;

  m_render_latch.pitch = ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_OFFSET]) * 2;
  m_render_latch.line_compare = (ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_LINE_COMPARE])) |
                                (ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_OVERFLOW] & 0x10) << 4) |
                                (ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_CHARACTER_CELL_HEIGHT] & 0x40) << 3);
  m_render_latch.row_scan_counter = (m_crtc_registers_ptr[CRTC_REGISTER_PRESET_ROW_SCAN] & 0x1F);

  m_render_latch.cursor_address = (ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_TEXT_CURSOR_ADDRESS_HIGH]) << 8) |
                                  (ZeroExtend32(m_crtc_registers_ptr[CRTC_REGISTER_TEXT_CURSOR_ADDRESS_LOW]));
  m_render_latch.cursor_start_line = std::min(
    static_cast<u8>(m_crtc_registers_ptr[CRTC_REGISTER_TEXT_CURSOR_START] & 0x1F), m_render_latch.character_height);
  m_render_latch.cursor_end_line = std::min(
    static_cast<u8>((m_crtc_registers_ptr[CRTC_REGISTER_TEXT_CURSOR_END] & 0x1F) + 1), m_render_latch.character_height);

  // If the cursor is disabled, set the address to something that will never be equal
  if (m_crtc_registers_ptr[CRTC_REGISTER_TEXT_CURSOR_START] & (1 << 5) || !m_cursor_state)
    m_render_latch.cursor_address = m_vram_size;
}

u32 VGABase::ReadVRAMPlanes(u32 base_address, u32 address_counter, u32 row_scan_counter) const
{
  u32 address = CRTCWrapAddress(base_address, address_counter, row_scan_counter);
  u32 vram_offset = (address * 4) & m_vram_mask;
  u32 all_planes;
  std::memcpy(&all_planes, &m_vram_ptr[vram_offset], sizeof(all_planes));

  u32 plane_mask = mask16[m_attribute_register_ptr[ATTRIBUTE_REGISTER_COLOR_PLANE_ENABLE] &
                          ATTRIBUTE_REGISTER_COLOR_PLANE_ENABLE_MASK];

  return all_planes & plane_mask;
}

u32 VGABase::CRTCWrapAddress(u32 base_address, u32 address_counter, u32 row_scan_counter) const
{
  const u8 mode_ctrl = m_crtc_registers_ptr[CRTC_REGISTER_MODE_CONTROL];
  if (m_crtc_registers_ptr[CRTC_REGISTER_UNDERLINE_ROW_SCANLINE] & 0x20)
  {
    // Count by 4
    address_counter /= 4;
  }
  else if (mode_ctrl & CRTC_REGISTER_MODE_CONTROL_COUNTBY2)
  {
    // Count by 2
    address_counter /= 2;
  }

  u32 address;
  if (m_crtc_registers_ptr[CRTC_REGISTER_UNDERLINE_ROW_SCANLINE] & 0x40)
  {
    // Double-word mode
    address = (((address_counter << 2) | ((address_counter >> 14) & 0x3)));
  }
  else if (!(mode_ctrl & CRTC_REGISTER_MODE_CONTROL_BYTE_MODE))
  {
    // Word mode
    if (mode_ctrl & CRTC_REGISTER_MODE_CONTROL_ALTERNATE_MA00)
      address = ((address_counter << 1) | ((address_counter >> 15) & 0x1));
    else
      address = ((address_counter << 1) | ((address_counter >> 13) & 0x1));
  }
  else
  {
    // Byte mode
    address = address_counter;
  }

  // TODO: Should this be before or after?
  address += base_address;

  // This bit selects the source of bit 13 of the output multiplexer. When this bit is set to 0, bit 0 of the row scan
  // counter is the source, and when this bit is set to 1, bit 13 of the address counter is the source.
  if (!(mode_ctrl & CRTC_REGISTER_MODE_CONTROL_ALTERNATE_LA13))
    address = (address & ~u32(1 << 13)) | ((row_scan_counter & 1) << 13);

  // This bit selects the source of bit 14 of the output multiplexer. When this bit is set to 0, bit 1 of the row scan
  // counter is the source, and when this bit is set to 1, bit 13 of the address counter is the source.
  if (!(mode_ctrl & CRTC_REGISTER_MODE_CONTROL_ALTERNATE_LA14))
    address = (address & ~u32(1 << 14)) | ((row_scan_counter & 2) << 13);

  return address;
}

void VGABase::Render()
{
  // On the standard VGA, the blink rate is dependent on the vertical frame rate. The on/off state of the cursor
  // changes every 16 vertical frames, which amounts to 1.875 blinks per second at 60 vertical frames per second. The
  // cursor blink rate is thus fixed and cannot be software controlled on the standard VGA. Some SVGA chipsets
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

  if (!m_display_timing.IsValid())
  {
    m_display_event->Deactivate();
    m_display->ClearFramebuffer();
    return;
  }

  if (!m_display->IsActive())
    return;

  LatchStartAddress();

  if (GRAPHICS_REGISTER_MISCELLANEOUS_GRAPHICS_MODE(m_graphics_registers_ptr[GRAPHICS_REGISTER_MISCELLANEOUS]))
    RenderGraphicsMode();
  else
    RenderTextMode();
}

void VGABase::RenderTextMode()
{
  const u32 character_columns = m_display->GetFramebufferWidth() / m_render_latch.character_width;
  const u32 character_rows = m_display->GetFramebufferHeight() / m_render_latch.character_height;

  // Determine base address of the fonts
  u32 font_base_address[2];
  const u8* font_base_pointers[2];
  for (u32 i = 0; i < 2; i++)
  {
    const u8 cmselect = m_sequencer_register_ptr[SEQUENCER_REGISTER_CHARACTER_MAP_SELECT];
    const u32 field = (i == 0) ? SEQUENCER_REGISTER_CHARACTER_MAP_SELECT_B(cmselect) :
                                 SEQUENCER_REGISTER_CHARACTER_MAP_SELECT_A(cmselect);
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

    font_base_pointers[i] = &m_vram_ptr[font_base_address[i] * 4];
  }

  // Get text palette colors
  SetOutputPalette16();

  u32 row_scan_counter = m_render_latch.row_scan_counter;
  u32 fb_x = 0, fb_y = 0;

  for (u32 row = 0; row < character_rows; row++)
  {
    u32 address_counter = (m_render_latch.pitch * row);
    fb_x = 0;

    for (u32 col = 0; col < character_columns; col++)
    {
      // Read as dwords, with each byte representing one plane
      u32 current_address = address_counter++;
      u32 all_planes = ReadVRAMPlanes(m_render_latch.start_address, current_address, row_scan_counter);

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
      switch (m_render_latch.character_width)
      {
        default:
        case 8:
          DrawTextGlyph8(fb_x, fb_y, glyph, m_render_latch.character_height, foreground_color, background_color, -1);
          break;

        case 9:
          DrawTextGlyph8(fb_x, fb_y, glyph, m_render_latch.character_height, foreground_color, background_color, dup9);
          break;

        case 16:
          DrawTextGlyph16(fb_x, fb_y, glyph, m_render_latch.character_height, foreground_color, background_color);
          break;
      }

      // To draw the cursor, we simply overwrite the pixels. Easier than branching in the character draw routine.
      if (current_address == m_render_latch.cursor_address)
      {
        // On the standard VGA, the cursor color is obtained from the foreground color of the character that the
        // cursor is superimposing. On the standard VGA there is no way to modify this behavior.
        // TODO: How is dup9 handled here?
        for (u8 cursor_line = m_render_latch.cursor_start_line; cursor_line < m_render_latch.cursor_end_line;
             cursor_line++)
        {
          for (u32 i = 0; i < m_render_latch.character_width; i++)
            m_display->SetPixel(fb_x + i, fb_y + cursor_line, foreground_color);
        }
      }

      fb_x += m_render_latch.character_width;
    }

    fb_y += m_render_latch.character_height;
  }

  m_display->SwapFramebuffer();
}

void VGABase::DrawTextGlyph8(u32 fb_x, u32 fb_y, const u8* glyph, u32 rows, u32 fg_color, u32 bg_color, s32 dup9)
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
  const bool shift_256 = GRAPHICS_REGISTER_MODE_SHIFT_256(m_graphics_registers_ptr[GRAPHICS_REGISTER_MODE]);
  const bool shift_reg = GRAPHICS_REGISTER_MODE_SHIFT_REG(m_graphics_registers_ptr[GRAPHICS_REGISTER_MODE]);
  const u32 screen_width = m_display->GetFramebufferWidth();
  const u32 screen_height = m_display->GetFramebufferHeight();
  const u32 scanlines_per_row = m_render_latch.character_height;
  const u32 line_compare = m_render_latch.line_compare;
  const u32 pitch = m_render_latch.pitch;
  u32 start_address = m_render_latch.start_address;
  u32 horizontal_pan = m_attribute_register_ptr[ATTRIBUTE_REGISTER_PIXEL_PANNING] & 0x07;

  // 4 or 16 color mode?
  if (!shift_256)
  {
    // This initializes 16 colours when we only need 4, but whatever.
    SetOutputPalette16();
  }
  else
  {
    // Initialize all palette colours beforehand.
    SetOutputPalette256();
  }

  // preset_row_scan[4:0] contains the starting row scan number, cleared when it hits max.
  u32 row_counter = 0;
  u32 row_scan_counter = m_render_latch.row_scan_counter;

  // Draw lines
  for (u32 scanline = 0; scanline < screen_height; scanline++)
  {
    if (scanline == line_compare)
    {
      // TODO: pixel_panning_mode determines whether to reset horizontal_pan
      start_address = 0;
      row_counter = 0;
      row_scan_counter = 0;
      horizontal_pan = 0;
    }

    u32 address_counter = pitch * row_counter;

    // 4 or 16 color mode?
    if (!shift_256)
    {
      if (shift_reg)
      {
        // CGA mode - Shift register in interleaved mode, odd bits from odd maps and even bits from even maps
        for (u32 col = 0; col < screen_width;)
        {
          u32 all_planes = ReadVRAMPlanes(start_address, address_counter, row_scan_counter);
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
          u32 all_planes = ReadVRAMPlanes(start_address, address_counter, row_scan_counter);
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
      s32 col = -s32(pan_pixels * 2);
      while (col < 0)
      {
        u32 indices = ReadVRAMPlanes(start_address, address_counter, row_scan_counter);
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
      while ((col + 4) <= static_cast<s32>(screen_width))
      {
        // Load 4 pixels, one from each plane
        // Duplicate horizontally twice, this is the shift_256 stuff
        u32 indices = ReadVRAMPlanes(start_address, address_counter, row_scan_counter);
        address_counter++;

        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 0) & 0xFF]);
        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 8) & 0xFF]);
        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 16) & 0xFF]);
        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 24) & 0xFF]);
      }

      // Slow loop to handle misaligned buffer when panning
      while (col < static_cast<s32>(screen_width))
      {
        u32 indices = ReadVRAMPlanes(start_address, address_counter, row_scan_counter);
        address_counter++;

        for (u32 i = 0; i < 4; i++)
        {
          u8 index = Truncate8(indices);
          u32 color = m_output_palette[index];
          indices >>= 8;

          if (col < static_cast<s32>(screen_width))
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