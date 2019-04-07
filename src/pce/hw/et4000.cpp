#include "pce/hw/et4000.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "common/display.h"
#include "pce/bus.h"
#include "pce/host_interface.h"
#include "pce/mmio.h"
#include "pce/system.h"
Log_SetChannel(HW::ET4000);

namespace HW {

DEFINE_OBJECT_TYPE_INFO(ET4000);
DEFINE_GENERIC_COMPONENT_FACTORY(ET4000);
BEGIN_OBJECT_PROPERTY_MAP(ET4000)
END_OBJECT_PROPERTY_MAP()

ET4000::ET4000(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_clock("ET4000 Retrace", 25175000), m_bios_file_path("romimages/et4000.bin")
{
}

ET4000::~ET4000()
{
  SAFE_RELEASE(m_bios_mmio);
  SAFE_RELEASE(m_vram_mmio);
}

bool ET4000::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_display = system->GetHostInterface()->CreateDisplay(
    SmallString::FromFormat("%s (ET4000)", m_identifier.GetCharArray()), Display::Type::Primary);
  if (!m_display)
    return false;
  m_display->SetDisplayAspectRatio(4, 3);

  m_clock.SetManager(system->GetTimingManager());

  if (!LoadBIOSROM())
    return false;

  ConnectIOPorts();
  RegisterVRAMMMIO();

  // Retrace event will be scheduled after timing is calculated.
  m_retrace_event = m_clock.NewEvent("Retrace", 1, std::bind(&ET4000::Render, this), false);
  return true;
}

void ET4000::Reset()
{
  m_misc_output_register.io_address_select = false;
  m_misc_output_register.ram_enable = true;
  m_misc_output_register.clock_select = 0;
  m_misc_output_register.hsync_polarity = true;
  m_misc_output_register.vsync_polarity = true;

  m_attribute_registers.attribute_mode_control.graphics_enable = false;
  m_attribute_registers.attribute_mode_control.mono_emulation = false;
  m_attribute_registers.attribute_mode_control.line_graphics_enable = true;
  m_attribute_registers.attribute_mode_control.blink_enable = false;
  m_attribute_registers.attribute_mode_control.pixel_panning_mode = false;
  m_attribute_registers.attribute_mode_control.pelclock_div2 = false;
  m_attribute_registers.attribute_mode_control.palette_bits_5_4_select = false;

  m_sequencer_registers.clocking_mode.dot_mode = false;
  m_sequencer_registers.clocking_mode.dot_clock_rate = false;
  m_sequencer_registers.vga_mode = true;
  m_sequencer_registers.bios_rom_address_map_0 = 1;
  m_sequencer_registers.bios_rom_address_map_1 = 1;

  std::fill_n(m_crtc_registers.index, countof(m_crtc_registers.index), u8(0));
  m_crtc_registers.mc6845_compatibility_control.vse_register_port = true;

  m_dac_ctrl = 0;
  m_dac_mask = 0xFF;
  m_dac_status_register = 0;
  m_dac_state_register = 0;
  m_dac_read_address = 0;
  m_dac_write_address = 0;
  m_dac_color_index = 0;
  for (size_t i = 0; i < m_dac_palette.size(); i++)
    m_dac_palette[i] = 0xFFFFFFFF;

  // Map B0000-B7FF by default
  m_misc_output_register.ram_enable = true;
  m_misc_output_register.odd_even_page = false;
  m_graphics_registers.misc.text_mode_disable = false;
  m_graphics_registers.misc.chain_odd_even_enable = false;
  m_graphics_registers.misc.memory_map_select = 2;
  m_graphics_registers.mode.host_odd_even = true;

  m_sequencer_registers.memory_plane_write_enable = 3;

  m_cursor_counter = 0;
  m_cursor_state = false;

  RecalculateEventTiming();
  m_retrace_event->Reset();
}

bool ET4000::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.SafeReadUInt8(&m_st0);
  reader.SafeReadBytes(m_crtc_registers.index, sizeof(m_crtc_registers.index));
  reader.SafeReadUInt8(&m_crtc_index_register);
  reader.SafeReadBytes(m_graphics_registers.index, sizeof(m_graphics_registers.index));
  reader.SafeReadUInt8(&m_graphics_address_register);
  reader.SafeReadUInt8(&m_misc_output_register.bits);
  reader.SafeReadUInt8(&m_feature_control_register);
  reader.SafeReadUInt8(&m_vga_adapter_enable.bits);
  reader.SafeReadBytes(m_attribute_registers.index, sizeof(m_attribute_registers.index));
  reader.SafeReadUInt8(&m_attribute_address_register);
  reader.SafeReadBool(&m_atc_palette_access);
  reader.SafeReadBytes(m_sequencer_registers.index, sizeof(m_sequencer_registers.index));
  reader.SafeReadUInt8(&m_sequencer_address_register);
  reader.SafeReadBytes(m_dac_palette.data(), Truncate32(sizeof(u32) * m_dac_palette.size()));
  reader.SafeReadUInt8(&m_dac_ctrl);
  reader.SafeReadUInt8(&m_dac_mask);
  reader.SafeReadUInt8(&m_dac_status_register);
  reader.SafeReadUInt8(&m_dac_state_register);
  reader.SafeReadUInt8(&m_dac_write_address);
  reader.SafeReadUInt8(&m_dac_read_address);
  reader.SafeReadUInt8(&m_dac_color_index);
  reader.SafeReadBytes(m_vram, sizeof(m_vram));
  reader.SafeReadUInt32(&m_latch);
  reader.SafeReadBytes(m_output_palette.data(), Truncate32(sizeof(u32) * m_output_palette.size()));
  reader.SafeReadUInt8(&m_cursor_counter);
  reader.SafeReadBool(&m_cursor_state);

  // Force re-render after loading state
  RecalculateEventTiming();
  Render();

  return !reader.GetErrorState();
}

bool ET4000::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  writer.WriteUInt8(m_st0);
  writer.WriteBytes(m_crtc_registers.index, sizeof(m_crtc_registers.index));
  writer.WriteUInt8(m_crtc_index_register);
  writer.WriteBytes(m_graphics_registers.index, sizeof(m_graphics_registers.index));
  writer.WriteUInt8(m_graphics_address_register);
  writer.WriteUInt8(m_misc_output_register.bits);
  writer.WriteUInt8(m_feature_control_register);
  writer.WriteUInt8(m_vga_adapter_enable.bits);
  writer.WriteBytes(m_attribute_registers.index, sizeof(m_attribute_registers.index));
  writer.WriteUInt8(m_attribute_address_register);
  writer.WriteBool(m_atc_palette_access);
  writer.WriteBytes(m_sequencer_registers.index, sizeof(m_sequencer_registers.index));
  writer.WriteUInt8(m_sequencer_address_register);
  writer.WriteBytes(m_dac_palette.data(), Truncate32(sizeof(u32) * m_dac_palette.size()));
  writer.WriteUInt8(m_dac_ctrl);
  writer.WriteUInt8(m_dac_mask);
  writer.WriteUInt8(m_dac_status_register);
  writer.WriteUInt8(m_dac_state_register);
  writer.WriteUInt8(m_dac_write_address);
  writer.WriteUInt8(m_dac_read_address);
  writer.WriteUInt8(m_dac_color_index);
  writer.WriteBytes(m_vram, sizeof(m_vram));
  writer.WriteUInt32(m_latch);
  writer.WriteBytes(m_output_palette.data(), Truncate32(sizeof(u32) * m_output_palette.size()));
  writer.WriteUInt8(m_cursor_counter);
  writer.WriteBool(m_cursor_state);

  return !writer.InErrorState();
}

void ET4000::ConnectIOPorts()
{
  m_bus->ConnectIOPortReadToPointer(0x03B0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03B0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03B2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03B2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03B4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03B4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortRead(0x03B1, this, std::bind(&ET4000::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03B1, this, std::bind(&ET4000::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03B3, this, std::bind(&ET4000::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03B3, this, std::bind(&ET4000::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03B5, this, std::bind(&ET4000::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03B5, this, std::bind(&ET4000::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03D0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03D0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03D2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03D2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03D4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03D4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortRead(0x03D1, this, std::bind(&ET4000::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03D1, this, std::bind(&ET4000::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03D3, this, std::bind(&ET4000::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03D3, this, std::bind(&ET4000::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03D5, this, std::bind(&ET4000::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03D5, this, std::bind(&ET4000::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03C2, this, &m_st0);
  m_bus->ConnectIOPortRead(0x03BA, this, std::bind(&ET4000::IOReadStatusRegister1, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03DA, this, std::bind(&ET4000::IOReadStatusRegister1, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03CD, this, &m_segment_select_register.bits);
  m_bus->ConnectIOPortWriteToPointer(0x03CD, this, &m_segment_select_register.bits);
  m_bus->ConnectIOPortReadToPointer(0x03CE, this, &m_graphics_address_register);
  m_bus->ConnectIOPortWriteToPointer(0x03CE, this, &m_graphics_address_register);
  m_bus->ConnectIOPortRead(0x03CF, this, std::bind(&ET4000::IOGraphicsDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03CF, this, std::bind(&ET4000::IOGraphicsDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03CC, this, &m_misc_output_register.bits);
  m_bus->ConnectIOPortWrite(0x03C2, this, std::bind(&ET4000::IOMiscOutputRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03CA, this, &m_feature_control_register);
  m_bus->ConnectIOPortWriteToPointer(0x03BA, this, &m_feature_control_register);
  m_bus->ConnectIOPortWriteToPointer(0x03DA, this, &m_feature_control_register);
  m_bus->ConnectIOPortReadToPointer(0x46E8, this, &m_vga_adapter_enable.bits);
  m_bus->ConnectIOPortWriteToPointer(0x46E8, this, &m_vga_adapter_enable.bits);
  m_bus->ConnectIOPortReadToPointer(0x03C3, this, &m_vga_adapter_enable.bits);
  m_bus->ConnectIOPortWriteToPointer(0x03C3, this, &m_vga_adapter_enable.bits);
  m_bus->ConnectIOPortRead(0x03C0, this, std::bind(&ET4000::IOAttributeAddressRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03C0, this, std::bind(&ET4000::IOAttributeAddressDataWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03C1, this, std::bind(&ET4000::IOAttributeDataRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03C4, this, &m_sequencer_address_register);
  m_bus->ConnectIOPortWriteToPointer(0x03C4, this, &m_sequencer_address_register);
  m_bus->ConnectIOPortRead(0x03C5, this, std::bind(&ET4000::IOSequencerDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03C5, this,
                            std::bind(&ET4000::IOSequencerDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03C6, this, std::bind(&ET4000::IODACMaskRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03C6, this, std::bind(&ET4000::IODACMaskWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03C7, this, std::bind(&ET4000::IODACStateRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03C7, this, std::bind(&ET4000::IODACReadAddressWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03C8, this, std::bind(&ET4000::IODACWriteAddressRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03C8, this, std::bind(&ET4000::IODACWriteAddressWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03C9, this, std::bind(&ET4000::IODACDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03C9, this, std::bind(&ET4000::IODACDataRegisterWrite, this, std::placeholders::_2));
}

void ET4000::Render()
{
  // On the standard ET4000, the blink rate is dependent on the vertical frame rate. The on/off state of the cursor
  // changes every 16 vertical frames, which amounts to 1.875 blinks per second at 60 vertical frames per second. The
  // cursor blink rate is thus fixed and cannot be software controlled on the standard ET4000. Some SET4000 chipsets
  // provide non-standard means for changing the blink rate of the text-mode cursor.
  // TODO: Should this tick in only text mode, and only when the cursor is enabled?
  if ((++m_cursor_counter) == 16)
  {
    m_cursor_counter = 0;
    m_cursor_state ^= true;
  }

  if (!m_display->IsActive())
    return;

  if (m_graphics_registers.misc.text_mode_disable)
    RenderGraphicsMode();
  else
    RenderTextMode();
}

void ET4000::IOReadStatusRegister1(u8* value)
{
  ScanoutInfo si = GetScanoutInfo();

  StatusRegister1 st1 = {};
  st1.display_enable_n = si.in_horizontal_blank || si.in_vertical_blank;
  st1.vertical_blank = si.in_vertical_blank;
  st1.vertical_blank_n = !si.in_vertical_blank;
  st1.display_feedback_test = 0x3; // Connected to output of attribute controller
  *value = st1.bits;

  // Reset the attribute register flip-flop
  m_crtc_registers.attribute_register_flipflop = false;
}

void ET4000::IOCRTCDataRegisterRead(u8* value)
{
  if (m_crtc_index_register >= countof(m_crtc_registers.index))
  {
    Log_ErrorPrintf("Out-of-range CRTC register read: %u", u32(m_crtc_index_register));
    *value = 0;
    return;
  }

  // Always a byte value
  u32 register_index = m_crtc_index_register;

  // Can only read C-F
  // Not true?
  // if (register_index >= 0xC && register_index <= 0xF)
  *value = m_crtc_registers.index[register_index];
  // else
  //*value = 0;

  Log_TracePrintf("CRTC register read: %u -> 0x%02X", u32(register_index),
                  u32(m_graphics_registers.index[register_index]));
}

void ET4000::IOCRTCDataRegisterWrite(u8 value)
{
  if (m_crtc_index_register >= countof(m_crtc_registers.index))
  {
    Log_ErrorPrintf("Out-of-range CRTC register write: %u", u32(m_crtc_index_register));
    return;
  }

  u32 register_index = m_crtc_index_register;
  m_crtc_registers.index[register_index] = value;

  Log_TracePrintf("CRTC register write: %u <- 0x%02X", u32(register_index), u32(value));

  if (register_index == 0x00 || register_index == 0x06 || register_index == 0x07)
    RecalculateEventTiming();
}

void ET4000::IOGraphicsDataRegisterRead(u8* value)
{
  if (m_graphics_address_register >= countof(m_graphics_registers.index))
  {
    Log_ErrorPrintf("Out-of-range graphics register read: %u", u32(m_graphics_address_register));
    *value = 0;
    return;
  }

  u8 register_index = m_graphics_address_register;
  *value = m_graphics_registers.index[register_index];

  Log_TracePrintf("Graphics register read: %u -> 0x%02X", u32(register_index),
                  u32(m_graphics_registers.index[register_index]));
}

void ET4000::IOGraphicsDataRegisterWrite(u8 value)
{
  static const uint8_t gr_mask[16] = {
    0x0f, /* 0x00 */
    0x0f, /* 0x01 */
    0x0f, /* 0x02 */
    0x1f, /* 0x03 */
    0x03, /* 0x04 */
    0x7b, /* 0x05 */
    0x0f, /* 0x06 */
    0x0f, /* 0x07 */
    0xff, /* 0x08 */
    0x00, /* 0x09 */
    0x00, /* 0x0a */
    0x00, /* 0x0b */
    0x00, /* 0x0c */
    0x00, /* 0x0d */
    0x00, /* 0x0e */
    0x00, /* 0x0f */
  };

  if (m_graphics_address_register >= countof(m_graphics_registers.index))
  {
    Log_ErrorPrintf("Out-of-range graphics register write: %u", u32(m_graphics_address_register));
    return;
  }

  u8 register_index = m_graphics_address_register & 0x0F;
  m_graphics_registers.index[register_index] = value & gr_mask[register_index];

  Log_TracePrintf("Graphics register write: %u <- 0x%02X", u32(register_index), u32(value));
}

void ET4000::IOMiscOutputRegisterWrite(u8 value)
{
  Log_TracePrintf("Misc output register write: 0x%02X", u32(value));
  m_misc_output_register.bits = value;
  RecalculateEventTiming();
}

void ET4000::IOAttributeAddressRead(u8* value)
{
  *value = m_attribute_address_register;
}

void ET4000::IOAttributeDataRead(u8* value)
{
  if (m_attribute_address_register >= countof(m_attribute_registers.index))
  {
    Log_ErrorPrintf("Out-of-range attribute register read: %u", u32(m_attribute_address_register));
    *value = 0;
    return;
  }

  u8 register_index = m_attribute_address_register;
  *value = m_attribute_registers.index[register_index];

  Log_TracePrintf("Attribute register read: %u -> 0x%02X", u32(register_index),
                  u32(m_attribute_registers.index[register_index]));
}

void ET4000::IOAttributeAddressDataWrite(u8 value)
{
  if (!m_crtc_registers.attribute_register_flipflop)
  {
    // bit 5/0x20 - disable ATC palette ram access, replace palette with overscan register
    bool atc_palette_access = !!(value & 0x20);
    if (atc_palette_access != m_atc_palette_access)
    {
      m_retrace_event->InvokeEarly();
      m_atc_palette_access = atc_palette_access;
    }

    // This write is the address
    m_attribute_address_register = (value & 0x1F);
    m_crtc_registers.attribute_register_flipflop = true;
    return;
  }

  // This write is the data
  m_crtc_registers.attribute_register_flipflop = false;

  if (m_attribute_address_register >= countof(m_attribute_registers.index))
  {
    Log_ErrorPrintf("Out-of-range attribute register write: %u", u32(m_attribute_address_register));
    return;
  }

  u8 register_index = m_attribute_address_register;
  m_attribute_registers.index[m_attribute_address_register] = value;

  Log_TracePrintf("Attribute register write: %u <- 0x%02X", u32(register_index), u32(value));

  // Mask text-mode palette indices to 6 bits
  if (register_index < 16)
    m_attribute_registers.index[register_index] &= 0x3F;
}

void ET4000::IOSequencerDataRegisterRead(u8* value)
{
  if (m_sequencer_address_register >= countof(m_sequencer_registers.index))
  {
    Log_ErrorPrintf("Out-of-range sequencer register read: %u", u32(m_sequencer_address_register));
    *value = 0;
    return;
  }

  u8 register_index = m_sequencer_address_register;
  *value = m_sequencer_registers.index[register_index];

  Log_TracePrintf("Sequencer register read: %u -> 0x%02X", u32(register_index),
                  u32(m_sequencer_registers.index[register_index]));
}

void ET4000::IOSequencerDataRegisterWrite(u8 value)
{
  /* force some bits to zero */
  const uint8_t sr_mask[8] = {
    0x03, 0x3d, 0x0f, 0x3f, 0x0e, 0x00, 0x00, 0xff,
  };

  if (m_sequencer_address_register >= countof(m_sequencer_registers.index))
    Log_ErrorPrintf("Out-of-range sequencer register write: %u", u32(m_sequencer_address_register));

  u8 register_index = m_sequencer_address_register % countof(m_sequencer_registers.index);
  m_sequencer_registers.index[register_index] = value & sr_mask[register_index];

  Log_TracePrintf("Sequencer register write: %u <- 0x%02X", u32(register_index),
                  u32(m_sequencer_registers.index[register_index]));

  if (register_index == 0x01)
    RecalculateEventTiming();
}

void ET4000::IODACMaskRead(u8* value) // 3c6
{
  if (m_dac_state_register == 4)
  {
    *value = m_dac_ctrl;
    m_dac_state_register = 0;
    return;
  }

  *value = m_dac_mask;
  m_dac_state_register++;
}

void ET4000::IODACMaskWrite(u8 value) // 3c6
{
  if (m_dac_state_register == 4)
  {
    m_dac_state_register = 0;
    m_dac_ctrl = value;
    return;
  }

  m_dac_mask = value;
  m_dac_state_register = 0;
}

void ET4000::IODACStateRegisterRead(u8* value) // 3c7
{
  *value = m_dac_status_register;
  m_dac_state_register = 0;
}

void ET4000::IODACReadAddressWrite(u8 value) // 3c7
{
  Log_TracePrintf("DAC read address write: %u", value);
  m_dac_read_address = value;
  m_dac_state_register = 0;
}

void ET4000::IODACWriteAddressRead(u8* value) // 3c8
{
  *value = m_dac_write_address;
  m_dac_state_register = 0;
}

void ET4000::IODACWriteAddressWrite(u8 value) // 3c8
{
  Log_TracePrintf("DAC write address write: %u", value);
  m_dac_write_address = value;
  m_dac_color_index = 0;
  m_dac_state_register = 0;
}

void ET4000::IODACDataRegisterRead(u8* value) // 3c9
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

  m_dac_status_register = 3;
  m_dac_state_register = 0;
}

void ET4000::IODACDataRegisterWrite(u8 value) // 3c9
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

bool ET4000::MapToVRAMOffset(u32* offset)
{
  u32 value = *offset;
  switch (m_graphics_registers.misc.memory_map_select)
  {
    case 0: // A0000-BFFFF (128K)
    {
      return true;
    }

    case 1: // A0000-AFFFF (64K)
    {
      if (value >= 0x10000)
        return false;

      *offset = value;
      return true;
    }

    case 2: // B0000-B7FFF (32K)
    {
      if (value < 0x10000 || value >= 0x18000)
        return false;

      *offset = (value - 0x10000);
      return true;
    }

    case 3: // B8000-BFFFF (32K)
    default:
    {
      if (value < 0x18000)
        return false;

      *offset = (value - 0x18000);
      return true;
    }
  }
}

void ET4000::HandleVRAMRead(u32 offset, u8* value)
{
  if (!MapToVRAMOffset(&offset))
  {
    // Out-of-range for the current mapping mode
    *value = 0xFF;
    return;
  }

  // 64K segment/bank select.
  const u32 segment = ZeroExtend32(m_segment_select_register.read_segment.GetValue()) * 65536;
  u8 read_plane;

  if (m_sequencer_registers.sequencer_memory_mode.chain_4_enable)
  {
    // Chain4 mode - access all four planes as a series of linear bytes
    read_plane = Truncate8(offset & 3);
    std::memcpy(&m_latch, &m_vram[(segment + offset) & ~u32(3)], sizeof(m_latch));
  }
  else
  {
    u32 latch_planar_address;
    if (!m_graphics_registers.mode.host_odd_even)
    {
      // By default we use the read map select register for the plane to return.
      read_plane = m_graphics_registers.read_map_select;
      latch_planar_address = segment + offset;
    }
    else
    {
      // Except for odd/even addressing, only access planes 0/1.
      read_plane = (m_graphics_registers.read_map_select & 0x02) | Truncate8(offset & 0x01);
      latch_planar_address = segment + (offset & ~u32(1));
    }

    // Use the offset to load the latches with all 4 planes.
    std::memcpy(&m_latch, &m_vram[latch_planar_address * 4], sizeof(m_latch));
  }

  // Compare value/mask mode?
  if (m_graphics_registers.mode.read_mode != 0)
  {
    // Read mode 1 - compare value/mask
    u32 compare_result =
      (m_latch ^ mask16[m_graphics_registers.color_compare]) & mask16[m_graphics_registers.color_dont_care];
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

inline u32 ET4000LogicOp(u8 logic_op, u32 latch, u32 value)
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

constexpr u32 ET4000ExpandMask(u8 mask)
{
  return ZeroExtend32(mask) | (ZeroExtend32(mask) << 8) | (ZeroExtend32(mask) << 16) | (ZeroExtend32(mask) << 24);
}

void ET4000::HandleVRAMWrite(u32 offset, u8 value)
{
  if (!MapToVRAMOffset(&offset))
  {
    // Out-of-range for the current mapping mode
    return;
  }

  // 64K segment/bank select.
  const u32 segment = ZeroExtend32(m_segment_select_register.write_segment.GetValue()) * 65536;

  if (m_sequencer_registers.sequencer_memory_mode.chain_4_enable)
  {
    // ET4000 differs from other SVGA hardware - chained write addresses go direct to memory addresses.
    u8 plane = Truncate8(offset & 3);
    if (m_sequencer_registers.memory_plane_write_enable & (1 << plane))
      m_vram[segment + offset] = value;
  }
  else if (!m_sequencer_registers.sequencer_memory_mode.odd_even_host_memory)
  {
    u8 plane = Truncate8(offset & 1);
    if (m_sequencer_registers.memory_plane_write_enable & (1 << plane))
      m_vram[((segment + (offset & ~u32(1))) * 4) | plane] = value;
  }
  else
  {
    u32 all_planes_value = 0;
    switch (m_graphics_registers.mode.write_mode)
    {
      case 0:
      {
        // The input byte is rotated right by the amount specified in Rotate Count, with all bits shifted off being
        // fed into bit 7
        u8 rotated = (value >> m_graphics_registers.rotate_count) | (value << (8 - m_graphics_registers.rotate_count));

        // The resulting byte is distributed over 4 separate paths, one for each plane of memory
        all_planes_value = ET4000ExpandMask(rotated);

        // If a bit in the Enable Set/Reset register is clear, the corresponding byte is left unmodified. Otherwise
        // the byte is replaced by all 0s if the corresponding bit in Set/Reset Value is clear, or all 1s if the bit
        // is one.
        all_planes_value = (all_planes_value & ~mask16[m_graphics_registers.enable_set_reset]) |
                           (mask16[m_graphics_registers.set_reset] & mask16[m_graphics_registers.enable_set_reset]);

        // The resulting value and the latch value are passed to the ALU
        all_planes_value = ET4000LogicOp(m_graphics_registers.logic_op, m_latch, all_planes_value);

        // The Bit Mask Register is checked, for each set bit the corresponding bit from the ALU is forwarded. If the
        // bit is clear the bit is taken directly from the Latch.
        u32 bit_mask = ET4000ExpandMask(m_graphics_registers.bit_mask);
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
        all_planes_value = ET4000LogicOp(m_graphics_registers.logic_op, m_latch, all_planes_value);

        // Then the Bit Mask field is used to select which bits come from the resulting data and which come from the
        // latch register.
        u32 bit_mask = ET4000ExpandMask(m_graphics_registers.bit_mask);
        all_planes_value = (all_planes_value & bit_mask) | (m_latch & ~bit_mask);
      }
      break;

      case 3:
      {
        // In this mode, the data in the Set/Reset field is used as if the Enable Set/Reset field were set to 1111b.
        u32 set_reset_data = mask16[m_graphics_registers.set_reset];

        // Then the host data is first rotated as per the Rotate Count field, then logical ANDed with the value of the
        // Bit Mask field.
        u8 rotated = (value >> m_graphics_registers.rotate_count) | (value << (8 - m_graphics_registers.rotate_count));
        u8 temp_bit_mask = m_graphics_registers.bit_mask & rotated;

        // Apply logical operation.
        all_planes_value = ET4000LogicOp(m_graphics_registers.logic_op, m_latch, set_reset_data);

        // The resulting value is used on the data obtained from the Set/Reset field in the same way that the Bit Mask
        // field would ordinarily be used to select which bits come from the expansion of the Set/Reset field and
        // which come from the latch register.
        u32 bit_mask = ET4000ExpandMask(temp_bit_mask);
        all_planes_value = (all_planes_value & bit_mask) | (m_latch & ~bit_mask);
      }
      break;
    }

    // Finally, only the bit planes enabled by the Memory Plane Write Enable field are written to memory.
    u32 write_mask = mask16[m_sequencer_registers.memory_plane_write_enable & 0xF];
    u32 current_value;
    std::memcpy(&current_value, &m_vram[(segment + offset) * 4], sizeof(current_value));
    all_planes_value = (all_planes_value & write_mask) | (current_value & ~write_mask);
    std::memcpy(&m_vram[(segment + offset) * 4], &all_planes_value, sizeof(current_value));
  }
}

bool ET4000::IsBIOSAddressMapped(u32 offset, u32 size)
{
  u32 last_byte = (offset + size - 1);
  if (last_byte >= m_bios_size)
    return false;

  // The ET4000 is designed to decode COOOO-CSFFF; C6800-C7FFF (hexl as the EROM address space on power-up, providing
  // 30KB code size for the ET4000 BIOS ROM modules. This address space can be redefined to a full 32KB by programming
  // TS Index Register 7 ITS Auxiliary Registerl bits 5 and 3.
  const u8 rmap = m_sequencer_registers.bios_rom_address_map_0 | (m_sequencer_registers.bios_rom_address_map_1 << 1);
  switch (rmap & 3)
  {
    case 0: // C0000-C3FFF
      // return (last_byte <= 0x3FFF);
      return true;

    case 1: // Disabled
      return false;

    case 2: // C0000-C5FFF+C6800-C7FFF
      return (last_byte <= 0x5FFF || last_byte >= 0x6800);

    case 3: // C0000-C7FFF
    default:
      return true;
  }
}

bool ET4000::LoadBIOSROM()
{
  auto data = System::ReadFileToBuffer(m_bios_file_path.c_str(), 0, MAX_BIOS_SIZE);
  if (!data.first)
    return false;

  m_bios_rom = std::move(data.first);
  m_bios_size = data.second;
  return true;
}

void ET4000::RegisterVRAMMMIO()
{
  auto read_byte_handler = [this](u32 base, u32 offset, u8* value) { HandleVRAMRead(base + offset, value); };
  auto read_word_handler = [this](u32 base, u32 offset, u16* value) {
    u8 b0, b1;
    HandleVRAMRead(base + offset + 0, &b0);
    HandleVRAMRead(base + offset + 1, &b1);
    *value = (u16(b1) << 8) | (u16(b0));
  };
  auto read_dword_handler = [this](u32 base, u32 offset, u32* value) {
    u8 b0, b1, b2, b3;
    HandleVRAMRead(base + offset + 0, &b0);
    HandleVRAMRead(base + offset + 1, &b1);
    HandleVRAMRead(base + offset + 2, &b2);
    HandleVRAMRead(base + offset + 3, &b3);
    *value = (u32(b3) << 24) | (u32(b2) << 16) | (u32(b1) << 8) | (u32(b0));
  };
  auto write_byte_handler = [this](u32 base, u32 offset, u8 value) { HandleVRAMWrite(base + offset, value); };
  auto write_word_handler = [this](u32 base, u32 offset, u16 value) {
    HandleVRAMWrite(base + offset + 0, u8(value & 0xFF));
    HandleVRAMWrite(base + offset + 1, u8((value >> 8) & 0xFF));
  };
  auto write_dword_handler = [this](u32 base, u32 offset, u32 value) {
    HandleVRAMWrite(base + offset + 0, u8(value & 0xFF));
    HandleVRAMWrite(base + offset + 1, u8((value >> 8) & 0xFF));
    HandleVRAMWrite(base + offset + 2, u8((value >> 16) & 0xFF));
    HandleVRAMWrite(base + offset + 3, u8((value >> 24) & 0xFF));
  };

  MMIO::Handlers handlers;
  handlers.read_byte = std::bind(read_byte_handler, 0, std::placeholders::_1, std::placeholders::_2);
  handlers.read_word = std::bind(read_word_handler, 0, std::placeholders::_1, std::placeholders::_2);
  handlers.read_dword = std::bind(read_dword_handler, 0, std::placeholders::_1, std::placeholders::_2);
  handlers.write_byte = std::bind(write_byte_handler, 0, std::placeholders::_1, std::placeholders::_2);
  handlers.write_word = std::bind(write_word_handler, 0, std::placeholders::_1, std::placeholders::_2);
  handlers.write_dword = std::bind(write_dword_handler, 0, std::placeholders::_1, std::placeholders::_2);

  // Map the entire range (0xA0000 - 0xCFFFF), then throw the writes out in the handler.
  m_vram_mmio = MMIO::CreateComplex(0xA0000, 0x20000, std::move(handlers));
  m_bus->ConnectMMIO(m_vram_mmio);

  // BIOS region
  handlers = {};
  handlers.read_byte = [this](u32 offset, u8* value) {
    *value = IsBIOSAddressMapped(offset, 1) ? m_bios_rom[offset] : 0xFF;
  };
  handlers.read_word = [this](u32 offset, u16* value) {
    if (IsBIOSAddressMapped(offset, 2))
    {
      std::memcpy(value, &m_bios_rom[offset], 2);
    }
  };
  handlers.read_dword = [this](u32 offset, u32* value) {
    if (IsBIOSAddressMapped(offset, 4))
    {
      std::memcpy(value, &m_bios_rom[offset], 4);
    }
  };
  handlers.IgnoreWrites();
  m_bios_mmio = MMIO::CreateComplex(0xC0000, 0x8000, std::move(handlers), true);
  m_bus->ConnectMMIO(m_bios_mmio);
}

inline u32 Convert6BitColorTo8Bit(u32 color)
{
  u8 r = Truncate8(color);
  u8 g = Truncate8(color >> 8);
  u8 b = Truncate8(color >> 16);

  // Convert 6-bit color to 8-bit color by shifting low bits to high bits (00123456 -> 12345612).
  r = (r << 2) | (r >> 4);
  g = (g << 2) | (g >> 4);
  b = (b << 2) | (b >> 4);

  return (color & 0xFF000000) | ZeroExtend32(r) | (ZeroExtend32(g) << 8) | (ZeroExtend32(b) << 16);
}

inline u32 ConvertBGR555ToRGB24(u16 color)
{
  u8 b = Truncate8(color & 31);
  u8 g = Truncate8((color >> 5) & 31);
  u8 r = Truncate8((color >> 10) & 31);

  // 00012345 -> 1234545
  b = (b << 3) | (b >> 3);
  g = (g << 3) | (g >> 3);
  r = (r << 3) | (r >> 3);

  return (color & 0xFF000000) | ZeroExtend32(r) | (ZeroExtend32(g) << 8) | (ZeroExtend32(b) << 16);
}

inline u32 ConvertBGR565ToRGB24(u16 color)
{
  u8 b = Truncate8(color & 31);
  u8 g = Truncate8((color >> 5) & 63);
  u8 r = Truncate8((color >> 11) & 31);

  // 00012345 -> 1234545 / 00123456 -> 12345656
  b = (b << 3) | (b >> 3);
  g = (g << 2) | (g >> 4);
  r = (r << 3) | (r >> 3);

  return (color & 0xFF000000) | ZeroExtend32(r) | (ZeroExtend32(g) << 8) | (ZeroExtend32(b) << 16);
}

void ET4000::SetOutputPalette16()
{
  for (u32 i = 0; i < 16; i++)
  {
    u32 index = ZeroExtend32(m_attribute_registers.palette[i]);

    // Control whether the color select controls the high bits or the palette index.
    if (m_attribute_registers.attribute_mode_control.palette_bits_5_4_select)
      index = ((m_attribute_registers.color_select & 0x0F) << 4) | (index & 0x0F);
    else
      index = ((m_attribute_registers.color_select & 0x0C) << 4) | (index & 0x3F);

    m_output_palette[i] = Convert6BitColorTo8Bit(m_dac_palette[index]);
  }
}

void ET4000::SetOutputPalette256()
{
  for (u32 i = 0; i < 256; i++)
  {
    m_output_palette[i] = Convert6BitColorTo8Bit(m_dac_palette[i]);
  }
}

void ET4000::RecalculateEventTiming()
{
  // Pixels clocks. 0 - 25MHz, 1 - 28Mhz, 2/3 - undefined
  static constexpr std::array<u32, 8> pixel_clocks = {
    {25175000, 28322000, 36000000, 40000000, 36000000, 65000000, 36000000, 36000000}};
  const u32 clock_select =
    m_misc_output_register.clock_select | (m_crtc_registers.mc6845_compatibility_control.clock_select_2 << 2);

  // Dot clock and character width are the only modifications which influences timings.
  u32 pixel_clock = pixel_clocks[clock_select];
  if (m_sequencer_registers.clocking_mode.dot_clock_rate)
    pixel_clock /= 2;
  if (!m_sequencer_registers.mclk_div2)
    pixel_clock *= 2;

  const u32 horizontal_total_pixels = m_crtc_registers.GetHorizontalTotal() * m_sequencer_registers.GetCharacterWidth();
  const double horizontal_frequency = double(pixel_clock) / double(horizontal_total_pixels);
  const u32 vertical_total_lines = m_crtc_registers.GetVerticalTotal();
  double vertical_frequency = horizontal_frequency / double(vertical_total_lines);

  Log_DevPrintf("ET4000: Horizontal frequency: %.4f kHz, vertical frequency: %.4f hz", horizontal_frequency / 1000.0,
                vertical_frequency);

  m_timing.horizontal_frequency = std::max(float(horizontal_frequency), 1.0f);
  m_timing.vertical_frequency = std::max(float(vertical_frequency), 1.0f);

  // This register should be programmed with the number of character clocks in the active display - 1.
  double horizontal_active_duration = (1000000000.0 * (ZeroExtend64(m_crtc_registers.GetHorizontalDisplayed()) *
                                                       m_sequencer_registers.GetCharacterWidth())) /
                                      double(pixel_clock);
  double horizontal_total_duration = double(1000000000.0) / horizontal_frequency;

  // The field contains the value of the vertical scanline counter at the beggining of the scanline immediately after
  // the last scanline of active display.
  u32 vertical_active_lines = m_crtc_registers.GetVerticalDisplayed();
  double vertical_active_duration = horizontal_total_duration * double(vertical_active_lines);
  double vertical_total_duration = double(1000000000.0) / vertical_frequency;

  // The active duration can be programmed so that there is no blanking period?
  if (horizontal_active_duration > horizontal_total_duration)
    horizontal_active_duration = horizontal_total_duration;
  m_timing.horizontal_active_duration = SimulationTime(horizontal_active_duration);
  m_timing.horizontal_total_duration = std::max(SimulationTime(horizontal_total_duration), SimulationTime(1));
  m_timing.vertical_active_duration = SimulationTime(vertical_active_duration);
  m_timing.vertical_total_duration = std::max(SimulationTime(vertical_total_duration), SimulationTime(1));

  // Vertical frequency must be between 35-75hz?
  if (vertical_frequency < 50.0 || vertical_frequency > 75.0)
  {
    Log_ErrorPrintf("ET4000: Horizontal frequency: %.4f kHz, vertical frequency: %.4f hz out of range.",
                    horizontal_frequency / 1000.0, vertical_frequency);

    // And prevent it from refreshing
    if (m_retrace_event->IsActive())
      m_retrace_event->Deactivate();

    return;
  }

  // TODO: Per-scanline rendering
  if (m_timing.vertical_frequency != m_retrace_event->GetInterval())
    m_retrace_event->SetFrequency(m_timing.vertical_frequency);
  if (!m_retrace_event->IsActive())
    m_retrace_event->Activate();
}

ET4000::ScanoutInfo ET4000::GetScanoutInfo()
{
  ScanoutInfo si;
  if (!m_retrace_event->IsActive())
  {
    // Display off, so let's just say we're in vblank
    si.current_line = 0;
    si.in_horizontal_blank = false;
    si.in_vertical_blank = false;
    si.display_active = false;
    return si;
  }

  SimulationTime time_since_retrace = m_retrace_event->GetTimeSinceLastExecution();
  si.current_line = u32(time_since_retrace / m_timing.horizontal_total_duration);

  // Check if we're in vertical retrace.
  si.in_vertical_blank = (time_since_retrace > m_timing.vertical_active_duration);
  if (si.in_vertical_blank)
  {
    si.in_horizontal_blank = false;
  }
  else
  {
    // Check if we're in horizontal retrace.
    SimulationTime scanline_time = (time_since_retrace % m_timing.horizontal_total_duration);
    si.in_horizontal_blank = (scanline_time >= m_timing.horizontal_active_duration);
    si.display_active = !si.in_horizontal_blank;
  }

  si.display_active = !si.in_horizontal_blank && !si.in_vertical_blank;
  return si;
}

u32 ET4000::CRTCReadVRAMPlanes(u32 address_counter, u32 row_scan_counter) const
{
  u32 address = CRTCWrapAddress(address_counter, row_scan_counter);
  u32 vram_offset = (address * 4) & (VRAM_SIZE - 1);
  u32 all_planes;
  std::memcpy(&all_planes, &m_vram[vram_offset], sizeof(all_planes));

  u32 plane_mask = mask16[m_attribute_registers.color_plane_enable];

  return all_planes & plane_mask;
}

u32 ET4000::CRTCWrapAddress(u32 address_counter, u32 row_scan_counter) const
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

void ET4000::RenderTextMode()
{
  const u32 character_height = m_crtc_registers.GetScanlinesPerRow();
  const u32 character_width = m_sequencer_registers.GetCharacterWidth();

  u32 character_columns = m_crtc_registers.GetHorizontalDisplayed();
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
  // uint32 row_counter = 0;
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
  //     uint32 line_compare = (ZeroExtend32(m_crtc_registers.line_compare)) |
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
    cursor_address = VRAM_SIZE;

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
      s32 dup9 = (character >= 0xC0 && character <= 0xDF) ? 1 : 0;
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
        // cursor is superimposing. On the standard ET4000 there is no way to modify this behavior.
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

void ET4000::DrawTextGlyph8(u32 fb_x, u32 fb_y, const u8* glyph, u32 rows, u32 fg_color, u32 bg_color, s32 dup9)
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

void ET4000::DrawTextGlyph16(u32 fb_x, u32 fb_y, const u8* glyph, u32 rows, u32 fg_color, u32 bg_color)
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

void ET4000::RenderGraphicsMode()
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
        for (s32 col = -(s32)horizontal_pan; col < (s32)screen_width;)
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

          for (u32 subindex = 0; col < (s32)screen_width && subindex < 8;)
          {
            if (col >= 0 && col < (s32)screen_width)
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
      s32 screen_width_div2 = s32(screen_width);
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