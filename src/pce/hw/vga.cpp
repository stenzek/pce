#include "pce/hw/vga.h"
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
Log_SetChannel(HW::VGA);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(VGA);
DEFINE_GENERIC_COMPONENT_FACTORY(VGA);
BEGIN_OBJECT_PROPERTY_MAP(VGA)
PROPERTY_TABLE_MEMBER_STRING("BIOSImage", 0, offsetof(VGA, m_bios_file_path), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

VGA::VGA(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_clock("VGA Retrace", 25175000),
    m_bios_file_path("romimages\\VGABIOS-lgpl-latest")
{
}

VGA::~VGA()
{
  SAFE_RELEASE(m_vram_mmio);
}

bool VGA::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_display = system->GetHostInterface()->CreateDisplay(
    SmallString::FromFormat("%s (VGA)", m_identifier.GetCharArray()), Display::Type::Primary);
  if (!m_display)
    return false;

  m_display->SetDisplayAspectRatio(4, 3);

  m_clock.SetManager(system->GetTimingManager());

  if (!LoadBIOSROM())
    return false;

  ConnectIOPorts();
  RegisterVRAMMMIO();

  // Retrace event will be scheduled after timing is calculated.
  m_retrace_event = m_clock.NewEvent("Retrace", 1, std::bind(&VGA::Render, this), false);
  return true;
}

void VGA::Reset()
{
  m_st1.display_disabled = false;
  m_st1.vblank = false;

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
  m_attribute_registers.attribute_mode_control.eight_bit_mode = false;
  m_attribute_registers.attribute_mode_control.palette_bits_5_4_select = false;

  m_sequencer_registers.clocking_mode.dot_mode = false;
  m_sequencer_registers.clocking_mode.dot_clock_rate = false;

  m_crtc_registers.horizontal_total = 95;
  m_crtc_registers.end_horizontal_display = 79;
  m_crtc_registers.start_horizontal_blanking = 80;
  m_crtc_registers.end_horizontal_blanking = 130;
  m_crtc_registers.start_horizontal_retrace = 85;
  m_crtc_registers.end_horizontal_retrace = 129;
  m_crtc_registers.vertical_total = 191;
  m_crtc_registers.overflow_register = 31;
  m_crtc_registers.preset_row_scan = 0;
  m_crtc_registers.maximum_scan_lines = 79;
  m_crtc_registers.cursor_start = 0;
  m_crtc_registers.cursor_end = 0;
  m_crtc_registers.start_address_low = 0;
  m_crtc_registers.start_address_high = 0;
  m_crtc_registers.cursor_location_low = 0;
  m_crtc_registers.cursor_location_high = 0;
  m_crtc_registers.vertical_retrace_start = 156;
  m_crtc_registers.vertical_retrace_end = 142;
  m_crtc_registers.vertical_display_end = 143;
  m_crtc_registers.offset = 40;
  m_crtc_registers.underline_location = 31;
  m_crtc_registers.start_vertical_blanking = 150;
  m_crtc_registers.end_vertical_blanking = 185;
  m_crtc_registers.crtc_mode_control = 163;
  m_crtc_registers.line_compare = 255;

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

bool VGA::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  reader.SafeReadUInt8(&m_st0);
  reader.SafeReadUInt8(&m_st1.bits);
  reader.SafeReadBytes(m_crtc_registers.index, sizeof(m_crtc_registers.index));
  reader.SafeReadUInt8(&m_crtc_index_register);
  reader.SafeReadBytes(m_graphics_registers.index, sizeof(m_graphics_registers.index));
  reader.SafeReadUInt8(&m_graphics_address_register);
  reader.SafeReadUInt8(&m_misc_output_register.bits);
  reader.SafeReadUInt8(&m_feature_control_register);
  reader.SafeReadUInt8(&m_vga_adapter_enable.bits);
  reader.SafeReadBytes(m_attribute_registers.index, sizeof(m_attribute_registers.index));
  reader.SafeReadUInt8(&m_attribute_address_register);
  reader.SafeReadBool(&m_attribute_video_enabled);
  reader.SafeReadBytes(m_sequencer_registers.index, sizeof(m_sequencer_registers.index));
  reader.SafeReadUInt8(&m_sequencer_address_register);
  reader.SafeReadBytes(m_dac_palette.data(), Truncate32(sizeof(uint32) * m_dac_palette.size()));
  reader.SafeReadUInt8(&m_dac_state_register);
  reader.SafeReadUInt8(&m_dac_write_address);
  reader.SafeReadUInt8(&m_dac_read_address);
  reader.SafeReadUInt8(&m_dac_color_index);
  reader.SafeReadBytes(m_vram, sizeof(m_vram));
  reader.SafeReadUInt32(&m_latch);
  reader.SafeReadBytes(m_output_palette.data(), Truncate32(sizeof(uint32) * m_output_palette.size()));
  reader.SafeReadUInt8(&m_cursor_counter);
  reader.SafeReadBool(&m_cursor_state);

  // Force re-render after loading state
  RecalculateEventTiming();
  Render();

  return !reader.GetErrorState();
}

bool VGA::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  writer.WriteUInt8(m_st0);
  writer.WriteUInt8(m_st1.bits);
  writer.WriteBytes(m_crtc_registers.index, sizeof(m_crtc_registers.index));
  writer.WriteUInt8(m_crtc_index_register);
  writer.WriteBytes(m_graphics_registers.index, sizeof(m_graphics_registers.index));
  writer.WriteUInt8(m_graphics_address_register);
  writer.WriteUInt8(m_misc_output_register.bits);
  writer.WriteUInt8(m_feature_control_register);
  writer.WriteUInt8(m_vga_adapter_enable.bits);
  writer.WriteBytes(m_attribute_registers.index, sizeof(m_attribute_registers.index));
  writer.WriteUInt8(m_attribute_address_register);
  writer.WriteBool(m_attribute_video_enabled);
  writer.WriteBytes(m_sequencer_registers.index, sizeof(m_sequencer_registers.index));
  writer.WriteUInt8(m_sequencer_address_register);
  writer.WriteBytes(m_dac_palette.data(), Truncate32(sizeof(uint32) * m_dac_palette.size()));
  writer.WriteUInt8(m_dac_state_register);
  writer.WriteUInt8(m_dac_write_address);
  writer.WriteUInt8(m_dac_read_address);
  writer.WriteUInt8(m_dac_color_index);
  writer.WriteBytes(m_vram, sizeof(m_vram));
  writer.WriteUInt32(m_latch);
  writer.WriteBytes(m_output_palette.data(), Truncate32(sizeof(uint32) * m_output_palette.size()));
  writer.WriteUInt8(m_cursor_counter);
  writer.WriteBool(m_cursor_state);

  return !writer.InErrorState();
}

void VGA::ConnectIOPorts()
{
  m_bus->ConnectIOPortReadToPointer(0x03B0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03B0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03B2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03B2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03B4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03B4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortRead(0x03B1, this, std::bind(&VGA::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03B1, this, std::bind(&VGA::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03B3, this, std::bind(&VGA::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03B3, this, std::bind(&VGA::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03B5, this, std::bind(&VGA::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03B5, this, std::bind(&VGA::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03D0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03D0, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03D2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03D2, this, &m_crtc_index_register);
  m_bus->ConnectIOPortReadToPointer(0x03D4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortWriteToPointer(0x03D4, this, &m_crtc_index_register);
  m_bus->ConnectIOPortRead(0x03D1, this, std::bind(&VGA::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03D1, this, std::bind(&VGA::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03D3, this, std::bind(&VGA::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03D3, this, std::bind(&VGA::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03D5, this, std::bind(&VGA::IOCRTCDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03D5, this, std::bind(&VGA::IOCRTCDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03C2, this, &m_st0);
  m_bus->ConnectIOPortRead(0x03BA, this, std::bind(&VGA::IOReadStatusRegister1, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03DA, this, std::bind(&VGA::IOReadStatusRegister1, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03CE, this, &m_graphics_address_register);
  m_bus->ConnectIOPortWriteToPointer(0x03CE, this, &m_graphics_address_register);
  m_bus->ConnectIOPortRead(0x03CF, this, std::bind(&VGA::IOGraphicsDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03CF, this, std::bind(&VGA::IOGraphicsDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03CC, this, &m_misc_output_register.bits);
  m_bus->ConnectIOPortWrite(0x03C2, this, std::bind(&VGA::IOMiscOutputRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03CA, this, &m_feature_control_register);
  m_bus->ConnectIOPortWriteToPointer(0x03BA, this, &m_feature_control_register);
  m_bus->ConnectIOPortWriteToPointer(0x03DA, this, &m_feature_control_register);
  m_bus->ConnectIOPortReadToPointer(0x46E8, this, &m_vga_adapter_enable.bits);
  m_bus->ConnectIOPortWriteToPointer(0x46E8, this, &m_vga_adapter_enable.bits);
  m_bus->ConnectIOPortReadToPointer(0x03C3, this, &m_vga_adapter_enable.bits);
  m_bus->ConnectIOPortWriteToPointer(0x03C3, this, &m_vga_adapter_enable.bits);
  m_bus->ConnectIOPortRead(0x03C0, this, std::bind(&VGA::IOAttributeAddressRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03C0, this, std::bind(&VGA::IOAttributeAddressDataWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03C1, this, std::bind(&VGA::IOAttributeDataRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03C4, this, &m_sequencer_address_register);
  m_bus->ConnectIOPortWriteToPointer(0x03C4, this, &m_sequencer_address_register);
  m_bus->ConnectIOPortRead(0x03C5, this, std::bind(&VGA::IOSequencerDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03C5, this, std::bind(&VGA::IOSequencerDataRegisterWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03C7, this, &m_dac_state_register);
  m_bus->ConnectIOPortWrite(0x03C7, this, std::bind(&VGA::IODACReadAddressWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortReadToPointer(0x03C8, this, &m_dac_write_address);
  m_bus->ConnectIOPortWrite(0x03C8, this, std::bind(&VGA::IODACWriteAddressWrite, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03C9, this, std::bind(&VGA::IODACDataRegisterRead, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03C9, this, std::bind(&VGA::IODACDataRegisterWrite, this, std::placeholders::_2));
}

bool VGA::LoadBIOSROM()
{
  const PhysicalMemoryAddress bios_load_location = 0xC0000;
  return m_bus->CreateROMRegionFromFile(m_bios_file_path.c_str(), 0, bios_load_location);
}

void VGA::Render()
{
  // On the standard VGA, the blink rate is dependent on the vertical frame rate. The on/off state of the cursor changes
  // every 16 vertical frames, which amounts to 1.875 blinks per second at 60 vertical frames per second. The cursor
  // blink rate is thus fixed and cannot be software controlled on the standard VGA. Some SVGA chipsets provide
  // non-standard means for changing the blink rate of the text-mode cursor.
  // TODO: Should this tick in only text mode, and only when the cursor is enabled?
  if ((++m_cursor_counter) == 16)
  {
    m_cursor_counter = 0;
    m_cursor_state ^= true;
  }

  if (m_graphics_registers.misc.text_mode_disable)
    RenderGraphicsMode();
  else
    RenderTextMode();
}

void VGA::IOReadStatusRegister1(uint8* value)
{
  ScanoutInfo si = GetScanoutInfo();
  m_st1.vblank = si.in_vertical_blank;
  m_st1.display_disabled = !si.display_active;
  *value = m_st1.bits;

  // Reset the attribute register flip-flop
  m_crtc_registers.attribute_register_flipflop = false;
}

void VGA::IOCRTCDataRegisterRead(uint8* value)
{
  if (m_crtc_index_register >= countof(m_crtc_registers.index))
  {
    Log_ErrorPrintf("Out-of-range CRTC register read: %u", uint32(m_crtc_index_register));
    *value = 0;
    return;
  }

  // Always a byte value
  uint32 register_index = m_crtc_index_register;

  // Can only read C-F
  // Not true?
  // if (register_index >= 0xC && register_index <= 0xF)
  *value = m_crtc_registers.index[register_index];
  // else
  //*value = 0;

  Log_TracePrintf("CRTC register read: %u -> 0x%02X", uint32(register_index),
                  uint32(m_graphics_registers.index[register_index]));
}

void VGA::IOCRTCDataRegisterWrite(uint8 value)
{
  if (m_crtc_index_register >= countof(m_crtc_registers.index))
  {
    Log_ErrorPrintf("Out-of-range CRTC register write: %u", uint32(m_crtc_index_register));
    return;
  }

  uint32 register_index = m_crtc_index_register;
  m_crtc_registers.index[register_index] = value;

  Log_TracePrintf("CRTC register write: %u <- 0x%02X", uint32(register_index), uint32(value));

  if (register_index == 0x00 || register_index == 0x06 || register_index == 0x07)
    RecalculateEventTiming();
}

void VGA::IOGraphicsDataRegisterRead(uint8* value)
{
  if (m_graphics_address_register >= countof(m_graphics_registers.index))
  {
    Log_ErrorPrintf("Out-of-range graphics register read: %u", uint32(m_graphics_address_register));
    *value = 0;
    return;
  }

  uint8 register_index = m_graphics_address_register;
  *value = m_graphics_registers.index[register_index];

  Log_TracePrintf("Graphics register read: %u -> 0x%02X", uint32(register_index),
                  uint32(m_graphics_registers.index[register_index]));
}

void VGA::IOGraphicsDataRegisterWrite(uint8 value)
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
    Log_ErrorPrintf("Out-of-range graphics register write: %u", uint32(m_graphics_address_register));
    return;
  }

  uint8 register_index = m_graphics_address_register & 0x0F;
  m_graphics_registers.index[register_index] = value & gr_mask[register_index];

  Log_TracePrintf("Graphics register write: %u <- 0x%02X", uint32(register_index), uint32(value));
}

void VGA::IOMiscOutputRegisterWrite(uint8 value)
{
  Log_TracePrintf("Misc output register write: 0x%02X", uint32(value));
  m_misc_output_register.bits = value;
  RecalculateEventTiming();
}

void VGA::IOAttributeAddressRead(uint8* value)
{
  *value = m_attribute_address_register;
}

void VGA::IOAttributeDataRead(uint8* value)
{
  if (m_attribute_address_register >= countof(m_attribute_registers.index))
  {
    Log_ErrorPrintf("Out-of-range attribute register read: %u", uint32(m_attribute_address_register));
    *value = 0;
    return;
  }

  uint8 register_index = m_attribute_address_register;
  *value = m_attribute_registers.index[register_index];

  Log_TracePrintf("Attribute register read: %u -> 0x%02X", uint32(register_index),
                  uint32(m_attribute_registers.index[register_index]));
}

void VGA::IOAttributeAddressDataWrite(uint8 value)
{
  if (!m_crtc_registers.attribute_register_flipflop)
  {
    bool video_enable = !!(value & 0x20);
    if (video_enable != m_attribute_video_enabled)
    {
      if (m_attribute_video_enabled)
        Log_WarningPrintf("Video disable - we should clear the displayed framebuffer");
      else
        Log_WarningPrintf("Video enable - we should set a full dirty region");

      m_attribute_video_enabled = video_enable;
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
    Log_ErrorPrintf("Out-of-range attribute register write: %u", uint32(m_attribute_address_register));
    return;
  }

  uint8 register_index = m_attribute_address_register;
  m_attribute_registers.index[m_attribute_address_register] = value;

  Log_TracePrintf("Attribute register write: %u <- 0x%02X", uint32(register_index), uint32(value));

  // Mask text-mode palette indices to 6 bits
  if (register_index < 16)
    m_attribute_registers.index[register_index] &= 0x3F;
}

void VGA::IOSequencerDataRegisterRead(uint8* value)
{
  if (m_sequencer_address_register >= countof(m_sequencer_registers.index))
  {
    Log_ErrorPrintf("Out-of-range sequencer register read: %u", uint32(m_sequencer_address_register));
    *value = 0;
    return;
  }

  uint8 register_index = m_sequencer_address_register;
  *value = m_sequencer_registers.index[register_index];

  Log_TracePrintf("Sequencer register read: %u -> 0x%02X", uint32(register_index),
                  uint32(m_sequencer_registers.index[register_index]));
}

void VGA::IOSequencerDataRegisterWrite(uint8 value)
{
  /* force some bits to zero */
  const uint8_t sr_mask[8] = {
    0x03, 0x3d, 0x0f, 0x3f, 0x0e, 0x00, 0x00, 0xff,
  };

  if (m_sequencer_address_register >= countof(m_sequencer_registers.index))
    Log_ErrorPrintf("Out-of-range sequencer register write: %u", uint32(m_sequencer_address_register));

  uint8 register_index = m_sequencer_address_register % countof(m_sequencer_registers.index);
  m_sequencer_registers.index[register_index] = value & sr_mask[register_index];

  Log_TracePrintf("Sequencer register write: %u <- 0x%02X", uint32(register_index),
                  uint32(m_sequencer_registers.index[register_index]));

  if (register_index == 0x01)
    RecalculateEventTiming();
}

void VGA::IODACReadAddressWrite(uint8 value)
{
  Log_DebugPrintf("DAC read address write: %u", value);
  m_dac_read_address = value;
  m_dac_state_register &= 0b00;
}

void VGA::IODACWriteAddressWrite(uint8 value)
{
  Log_DebugPrintf("DAC write address write: %u", value);
  m_dac_write_address = value;
  m_dac_state_register |= 0b11;
}

void VGA::IODACDataRegisterRead(uint8* value)
{
  uint32 color_value = m_dac_palette[m_dac_read_address];
  uint8 shift = m_dac_color_index * 8;
  *value = uint8((color_value >> shift) & 0xFF);

  Log_TracePrintf("DAC palette read %u/%u: %u", uint32(m_dac_read_address), uint32(m_dac_color_index), uint32(*value));

  m_dac_color_index++;
  if (m_dac_color_index >= 3)
  {
    m_dac_color_index = 0;
    m_dac_read_address = (m_dac_read_address + 1) % m_dac_palette.size();
  }
}

void VGA::IODACDataRegisterWrite(uint8 value)
{
  Log_TracePrintf("DAC palette write %u/%u: %u", uint32(m_dac_write_address), uint32(m_dac_color_index), uint32(value));

  // Mask away higher bits
  value &= 0x3F;

  uint32 color_value = m_dac_palette[m_dac_write_address];
  uint8 shift = m_dac_color_index * 8;
  color_value &= ~uint32(0xFF << shift);
  color_value |= (uint32(value) << shift);
  m_dac_palette[m_dac_write_address] = color_value;

  m_dac_color_index++;
  if (m_dac_color_index >= 3)
  {
    m_dac_color_index = 0;
    m_dac_write_address = (m_dac_write_address + 1) % m_dac_palette.size();
  }
}

// Values of 4-bit registers containing the plane mask expanded to 8 bits per plane.
static constexpr std::array<uint32, 16> mask16 = {
  0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff, 0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff,
  0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff, 0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff,
};

bool VGA::MapToVRAMOffset(uint32* offset)
{
  uint32 value = *offset;
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

      return true;
    }

    case 2: // B0000-B7FFF (32K)
    {
      if (value < 0x10000 || value >= 0x18000)
        return false;

      *offset = value - 0x10000;
      return true;
    }

    case 3: // B8000-BFFFF (32K)
    default:
    {
      if (value < 0x18000)
        return false;

      *offset = value - 0x18000;
      return true;
    }
  }
}

void VGA::HandleVRAMRead(uint32 offset, uint8* value)
{
  if (!MapToVRAMOffset(&offset))
  {
    // Out-of-range for the current mapping mode
    *value = 0xFF;
    return;
  }

  uint8 read_plane;
  uint32 latch_linear_address;

  if (m_sequencer_registers.sequencer_memory_mode.chain_4_enable)
  {
    // Chain4 mode - access all four planes as a series of linear bytes
    read_plane = Truncate8(offset & 3);
    latch_linear_address = ((offset & ~uint32(3)) << 2) & (VRAM_SIZE - 1);
    DebugAssert(latch_linear_address < VRAM_SIZE);
    std::memcpy(&m_latch, &m_vram[latch_linear_address], sizeof(m_latch));
    *value = Truncate8(m_latch >> (8 * read_plane));
    return;
  }

  // Use the offset to load the latches with all 4 planes.
  latch_linear_address = (offset << 2) & (VRAM_SIZE - 1);
  std::memcpy(&m_latch, &m_vram[latch_linear_address], sizeof(m_latch));

  // By default we use the read map select register for the plane to return.
  read_plane = m_graphics_registers.read_map_select;
  uint32 read_linear_address = (offset << 2) & (VRAM_SIZE - 1);
  if (m_graphics_registers.mode.host_odd_even)
  {
    // Except for odd/even addressing, only access planes 0/1.
    read_plane = (m_graphics_registers.read_map_select & 0x02) | Truncate8(offset & 0x01);
    read_linear_address = (offset & ~uint32(1)) * 4;
  }

  // Compare value/mask mode?
  if (m_graphics_registers.mode.read_mode != 0)
  {
    // Read mode 1 - compare value/mask
    uint32 compare_result =
      (m_latch ^ mask16[m_graphics_registers.color_compare]) & mask16[m_graphics_registers.color_dont_care];
    uint8 ret = Truncate8(compare_result) | Truncate8(compare_result >> 8) | Truncate8(compare_result >> 16) |
                Truncate8(compare_result >> 24);
    *value = ~ret;
  }
  else
  {
    // Read mode 0 - return specified plane
    *value = m_vram[read_linear_address | read_plane];
  }
}

inline uint32 VGALogicOp(uint8 logic_op, uint32 latch, uint32 value)
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

constexpr uint32 VGAExpandMask(uint8 mask)
{
  return ZeroExtend32(mask) | (ZeroExtend32(mask) << 8) | (ZeroExtend32(mask) << 16) | (ZeroExtend32(mask) << 24);
}

void VGA::HandleVRAMWrite(uint32 offset, uint8 value)
{
  if (!MapToVRAMOffset(&offset))
  {
    // Out-of-range for the current mapping mode
    return;
  }

  if (m_sequencer_registers.sequencer_memory_mode.chain_4_enable)
  {
    uint8 plane = Truncate8(offset & 3);
    if (m_sequencer_registers.memory_plane_write_enable & (1 << plane))
    {
      // Offset | Plane | Byte within plane | VRAM Address
      // -------------------------------------------------
      //      0 |     0 |                 0 |            0
      //      1 |     1 |                 0 |            1
      //      2 |     2 |                 0 |            2
      //      3 |     3 |                 0 |            3
      //      4 |     0 |                 4 |           16
      //      5 |     1 |                 4 |           17
      //      6 |     2 |                 4 |           18
      //      7 |     3 |                 4 |           19
      offset = ((offset & ~uint32(3)) * 4) | ZeroExtend32(plane);
      DebugAssert(offset < VRAM_SIZE);
      m_vram[offset] = value;
    }
  }
  else if (!m_sequencer_registers.sequencer_memory_mode.odd_even_host_memory)
  {
    uint8 plane = Truncate8(offset & 1);
    if (m_sequencer_registers.memory_plane_write_enable & (1 << plane))
    {
      // Similar to chain4, write to planes 0/1, but keep offset.
      offset = ((offset & ~uint32(1)) * 4) | ZeroExtend32(plane);
      DebugAssert(offset < VRAM_SIZE);
      m_vram[offset] = value;
    }
  }
  else
  {
    uint32 all_planes_value = 0;
    switch (m_graphics_registers.mode.write_mode)
    {
      case 0:
      {
        // The input byte is rotated right by the amount specified in Rotate Count, with all bits shifted off being fed
        // into bit 7
        uint8 rotated =
          (value >> m_graphics_registers.rotate_count) | (value << (8 - m_graphics_registers.rotate_count));

        // The resulting byte is distributed over 4 separate paths, one for each plane of memory
        all_planes_value = VGAExpandMask(rotated);

        // If a bit in the Enable Set/Reset register is clear, the corresponding byte is left unmodified. Otherwise the
        // byte is replaced by all 0s if the corresponding bit in Set/Reset Value is clear, or all 1s if the bit is one.
        all_planes_value = (all_planes_value & ~mask16[m_graphics_registers.enable_set_reset]) |
                           (mask16[m_graphics_registers.set_reset] & mask16[m_graphics_registers.enable_set_reset]);

        // The resulting value and the latch value are passed to the ALU
        all_planes_value = VGALogicOp(m_graphics_registers.logic_op, m_latch, all_planes_value);

        // The Bit Mask Register is checked, for each set bit the corresponding bit from the ALU is forwarded. If the
        // bit is clear the bit is taken directly from the Latch.
        uint32 bit_mask = VGAExpandMask(m_graphics_registers.bit_mask);
        all_planes_value = (all_planes_value & bit_mask) | (m_latch & ~bit_mask);
      }
      break;

      case 1:
      {
        // In this mode, data is transferred directly from the 32 bit latch register to display memory, affected only by
        // the Memory Plane Write Enable field. The host data is not used in this mode.
        all_planes_value = m_latch;
      }
      break;

      case 2:
      {
        // In this mode, the bits 3-0 of the host data are replicated across all 8 bits of their respective planes.
        all_planes_value = mask16[value & 0x0F];

        // Then the selected Logical Operation is performed on the resulting data and the data in the latch register.
        all_planes_value = VGALogicOp(m_graphics_registers.logic_op, m_latch, all_planes_value);

        // Then the Bit Mask field is used to select which bits come from the resulting data and which come from the
        // latch register.
        uint32 bit_mask = VGAExpandMask(m_graphics_registers.bit_mask);
        all_planes_value = (all_planes_value & bit_mask) | (m_latch & ~bit_mask);
      }
      break;

      case 3:
      {
        // In this mode, the data in the Set/Reset field is used as if the Enable Set/Reset field were set to 1111b.
        uint32 set_reset_data = mask16[m_graphics_registers.set_reset];

        // Then the host data is first rotated as per the Rotate Count field, then logical ANDed with the value of the
        // Bit Mask field.
        uint8 rotated =
          (value >> m_graphics_registers.rotate_count) | (value << (8 - m_graphics_registers.rotate_count));
        uint8 temp_bit_mask = m_graphics_registers.bit_mask & rotated;

        // Apply logical operation.
        all_planes_value = VGALogicOp(m_graphics_registers.logic_op, m_latch, set_reset_data);

        // The resulting value is used on the data obtained from the Set/Reset field in the same way that the Bit Mask
        // field would ordinarily be used to select which bits come from the expansion of the Set/Reset field and which
        // come from the latch register.
        uint32 bit_mask = VGAExpandMask(temp_bit_mask);
        all_planes_value = (all_planes_value & bit_mask) | (m_latch & ~bit_mask);
      }
      break;
    }

    // Finally, only the bit planes enabled by the Memory Plane Write Enable field are written to memory.
    uint32 write_mask = mask16[m_sequencer_registers.memory_plane_write_enable & 0xF];
    uint32 current_value;
    std::memcpy(&current_value, &m_vram[offset * 4], sizeof(current_value));
    all_planes_value = (all_planes_value & write_mask) | (current_value & ~write_mask);
    std::memcpy(&m_vram[offset * 4], &all_planes_value, sizeof(current_value));
  }
}

void VGA::RegisterVRAMMMIO()
{
  auto read_byte_handler = [this](uint32 base, uint32 offset, uint8* value) { HandleVRAMRead(base + offset, value); };
  auto read_word_handler = [this](uint32 base, uint32 offset, uint16* value) {
    uint8 b0, b1;
    HandleVRAMRead(base + offset + 0, &b0);
    HandleVRAMRead(base + offset + 1, &b1);
    *value = (uint16(b1) << 8) | (uint16(b0));
  };
  auto read_dword_handler = [this](uint32 base, uint32 offset, uint32* value) {
    uint8 b0, b1, b2, b3;
    HandleVRAMRead(base + offset + 0, &b0);
    HandleVRAMRead(base + offset + 1, &b1);
    HandleVRAMRead(base + offset + 2, &b2);
    HandleVRAMRead(base + offset + 3, &b3);
    *value = (uint32(b3) << 24) | (uint32(b2) << 16) | (uint32(b1) << 8) | (uint32(b0));
  };
  auto write_byte_handler = [this](uint32 base, uint32 offset, uint8 value) { HandleVRAMWrite(base + offset, value); };
  auto write_word_handler = [this](uint32 base, uint32 offset, uint16 value) {
    HandleVRAMWrite(base + offset + 0, uint8(value & 0xFF));
    HandleVRAMWrite(base + offset + 1, uint8((value >> 8) & 0xFF));
  };
  auto write_dword_handler = [this](uint32 base, uint32 offset, uint32 value) {
    HandleVRAMWrite(base + offset + 0, uint8(value & 0xFF));
    HandleVRAMWrite(base + offset + 1, uint8((value >> 8) & 0xFF));
    HandleVRAMWrite(base + offset + 2, uint8((value >> 16) & 0xFF));
    HandleVRAMWrite(base + offset + 3, uint8((value >> 24) & 0xFF));
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

  // Log_DevPrintf("Mapped %u bytes of VRAM at 0x%08X-0x%08X", size, base, base + size - 1);
}

inline uint32 Convert6BitColorTo8Bit(uint32 color)
{
  uint8 r = Truncate8(color);
  uint8 g = Truncate8(color >> 8);
  uint8 b = Truncate8(color >> 16);

  // Convert 6-bit color to 8-bit color by shifting low bits to high bits (00123456 -> 12345612).
  r = (r << 2) | (r >> 4);
  g = (g << 2) | (g >> 4);
  b = (b << 2) | (b >> 4);

  return (color & 0xFF000000) | ZeroExtend32(r) | (ZeroExtend32(g) << 8) | (ZeroExtend32(b) << 16);
}

void VGA::SetOutputPalette16()
{
  for (uint32 i = 0; i < 16; i++)
  {
    uint32 index = ZeroExtend32(m_attribute_registers.palette[i]);

    // Control whether the color select controls the high bits or the palette index.
    if (m_attribute_registers.attribute_mode_control.palette_bits_5_4_select)
      index = ((m_attribute_registers.color_select & 0x0F) << 4) | (index & 0x0F);
    else
      index = ((m_attribute_registers.color_select & 0x0C) << 4) | (index & 0x3F);

    m_output_palette[i] = Convert6BitColorTo8Bit(m_dac_palette[index]);
  }
}

void VGA::SetOutputPalette256()
{
  for (uint32 i = 0; i < 256; i++)
  {
    m_output_palette[i] = Convert6BitColorTo8Bit(m_dac_palette[i]);
  }
}

void VGA::RecalculateEventTiming()
{
  // Pixels clocks. 0 - 25MHz, 1 - 28Mhz, 2/3 - undefined
  static constexpr std::array<uint32, 4> pixel_clocks = {{25175000, 28322000, 25175000, 25175000}};
  uint32 pixel_clock = pixel_clocks[m_misc_output_register.clock_select];

  // Character width depending on dot clock - is this correct for graphics modes? What about dot clock rate?
  uint32 character_width = m_sequencer_registers.clocking_mode.dot_mode ? 8 : 9;

  // Due to timing factors of the VGA hardware (which, for compatibility purposes has been emulated by VGA compatible
  // chipsets), the actual horizontal total is 5 character clocks more than the value stored in this field.
  uint32 horizontal_total_pixels = (ZeroExtend32(m_crtc_registers.horizontal_total) + 5) * character_width;
  if (m_sequencer_registers.clocking_mode.dot_clock_rate)
    horizontal_total_pixels *= 2;

  // If we need hblank timing, use start_horizontal_blanking
  double horizontal_frequency = double(pixel_clock) / double(horizontal_total_pixels);

  // This field contains the value of the scanline counter at the beginning of the last scanline in the vertical period.
  uint32 vertical_total_lines =
    (ZeroExtend32(m_crtc_registers.vertical_total) | (ZeroExtend32(m_crtc_registers.overflow_register & 0x01) << 8) |
     ZeroExtend32(m_crtc_registers.overflow_register & 0x20) << 4) +
    1;

  // Doublescanning - should we have line repeat here too?
  if (m_crtc_registers.maximum_scan_lines & 0x80)
    vertical_total_lines *= 2;

  double vertical_frequency = horizontal_frequency / double(vertical_total_lines);
  Log_DebugPrintf("VGA: Horizontal frequency: %.4f kHz, vertical frequency: %.4f hz", horizontal_frequency / 1000.0,
                  vertical_frequency);

  m_timing.horizontal_frequency = std::max(float(horizontal_frequency), 1.0f);
  m_timing.vertical_frequency = std::max(float(vertical_frequency), 1.0f);

  // This register should be programmed with the number of character clocks in the active display - 1.
  double horizontal_active_duration =
    (1000000000.0 * (ZeroExtend64(m_crtc_registers.end_horizontal_display) + 1) * character_width) /
    double(pixel_clock);
  double horizontal_total_duration = double(1000000000.0) / horizontal_frequency;

  // The field contains the value of the vertical scanline counter at the beggining of the scanline immediately after
  // the last scanline of active display.
  uint32 vertical_active_lines = (ZeroExtend32(m_crtc_registers.vertical_display_end) |
                                  (ZeroExtend32(m_crtc_registers.overflow_register & 0x02) << 7) |
                                  ZeroExtend32(m_crtc_registers.overflow_register & 0x40) << 3) +
                                 1;
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
  if (vertical_frequency < 35.0 || vertical_frequency > 75.0)
  {
    Log_DebugPrintf("VGA: Horizontal frequency: %.4f kHz, vertical frequency: %.4f hz out of range.",
                    horizontal_frequency / 1000.0, vertical_frequency);

    // Clear the screen
    m_display->ClearFramebuffer();

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

VGA::ScanoutInfo VGA::GetScanoutInfo()
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
  si.current_line = uint32(time_since_retrace / m_timing.horizontal_total_duration);

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

uint32 VGA::CRTCReadVRAMPlanes(uint32 address_counter, uint32 row_scan_counter) const
{
  uint32 address = CRTCWrapAddress(address_counter, row_scan_counter);
  uint32 vram_offset = address * 4;
  uint32 all_planes;
  std::memcpy(&all_planes, &m_vram[vram_offset], sizeof(all_planes));

  uint32 plane_mask = mask16[m_attribute_registers.color_plane_enable];

  return all_planes & plane_mask;
}

uint32 VGA::CRTCWrapAddress(uint32 address_counter, uint32 row_scan_counter) const
{
  uint32 address;
  if (m_crtc_registers.underline_location & 0x40)
  {
    // Double-word mode
    address = ((address_counter << 2) | ((address_counter >> 14) & 0x3)) & 0xFFFF;
  }
  else if (!(m_crtc_registers.crtc_mode_control & 0x40))
  {
    // Word mode
    if (m_crtc_registers.crtc_mode_control & 0x20)
      address = ((address_counter << 1) | ((address_counter >> 15) & 0x1)) & 0xFFFF;
    else
      address = ((address_counter << 1) | ((address_counter >> 13) & 0x1)) & 0xFFFF;
  }
  else
  {
    // Byte mode
    address = address_counter & 0xFFFF;
  }

  // This bit selects the source of bit 13 of the output multiplexer. When this bit is set to 0, bit 0 of the row scan
  // counter is the source, and when this bit is set to 1, bit 13 of the address counter is the source.
  if ((m_crtc_registers.crtc_mode_control & 0x01) == 0)
    address = (address & ~uint32(1 << 13)) | ((row_scan_counter & 1) << 13);

  // This bit selects the source of bit 14 of the output multiplexer. When this bit is set to 0, bit 1 of the row scan
  // counter is the source, and when this bit is set to 1, bit 13 of the address counter is the source.
  if ((m_crtc_registers.crtc_mode_control & 0x02) == 0)
    address = (address & ~uint32(1 << 14)) | ((row_scan_counter & 2) << 13);

  return address;
}

void VGA::RenderTextMode()
{
  uint32 character_height = (m_crtc_registers.maximum_scan_lines & 0x1F) + 1;
  uint32 character_width = 8;
  if (!m_sequencer_registers.clocking_mode.dot_mode)
    character_width = 9;
  if (m_sequencer_registers.clocking_mode.dot_clock_rate)
    character_width = 16;

  uint32 character_columns = (uint32(m_crtc_registers.end_horizontal_display) + 1);
  uint32 character_rows =
    (m_crtc_registers.vertical_display_end | (uint32(m_crtc_registers.overflow_register & 0x02) << 7) |
     (uint32(m_crtc_registers.overflow_register & 0x40) << 3));
  character_rows = (character_rows + 1) / character_height;
  if (m_crtc_registers.vertical_total == 100)
    character_rows = 100;

  if (character_columns == 0 || character_rows == 0)
    return;

  // uint32 screen_width = (uint32(m_crtc_registers.end_horizontal_display) + 1) * 8;
  // uint32 screen_height = (m_crtc_registers.vertical_display_end | (uint32(m_crtc_registers.overflow_register & 0x02)
  // << 7) | (uint32(m_crtc_registers.overflow_register & 0x40) << 3)) + 1;
  uint32 screen_width = character_columns * character_width;
  uint32 screen_height = character_rows * character_height;
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
  uint32 row_scan_counter = ZeroExtend32(m_crtc_registers.preset_row_scan & 0x1F);

  // Determine base address of the fonts
  uint32 font_base_address[2];
  const uint8* font_base_pointers[2];
  for (uint32 i = 0; i < 2; i++)
  {
    uint32 field;
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
  uint32 data_base_address =
    (ZeroExtend32(m_crtc_registers.start_address_high) << 8) | (ZeroExtend32(m_crtc_registers.start_address_low));
  //     uint32 line_compare = (ZeroExtend32(m_crtc_registers.line_compare)) |
  //                           (ZeroExtend32(m_crtc_registers.overflow_register & 0x10) << 4) |
  //                           (ZeroExtend32(m_crtc_registers.maximum_scan_lines & 0x40) << 3);
  uint32 row_pitch = ZeroExtend32(m_crtc_registers.offset) * 2;

  // Cursor setup
  uint32 cursor_address =
    (ZeroExtend32(m_crtc_registers.cursor_location_high) << 8) | (ZeroExtend32(m_crtc_registers.cursor_location_low));
  uint32 cursor_start_line = ZeroExtend32(m_crtc_registers.cursor_start & 0x1F);
  uint32 cursor_end_line = ZeroExtend32(m_crtc_registers.cursor_end & 0x1F) + 1;

  // If the cursor is disabled, set the address to something that will never be equal
  if (m_crtc_registers.cursor_start & (1 << 5) || !m_cursor_state)
    cursor_address = VRAM_SIZE;

  // Draw
  for (uint32 row = 0; row < character_rows; row++)
  {
    // DebugAssert((vram_offset + (character_columns * 2)) <= VRAM_SIZE);

    uint32 address_counter = data_base_address + (row_pitch * row);
    for (uint32 col = 0; col < character_columns; col++)
    {
      // Read as dwords, with each byte representing one plane
      uint32 current_address = address_counter++;
      uint32 all_planes = CRTCReadVRAMPlanes(current_address, row_scan_counter);

      uint8 character = Truncate8(all_planes >> 0);
      uint8 attribute = Truncate8(all_planes >> 8);

      // Grab foreground and background colours
      uint32 foreground_color = m_output_palette[(attribute & 0xF)];
      uint32 background_color = m_output_palette[(attribute >> 4) & 0xF];

      // Offset into font table to get glyph, bit 4 determines the font to use
      // 32 bytes per character in the font bitmap, 4 bytes per plane, data in plane 2.
      const uint8* glyph = font_base_pointers[(attribute >> 3) & 0x01] + (character * 32 * 4) + 2;

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
        // On the standard VGA, the cursor color is obtained from the foreground color of the character that the cursor
        // is superimposing. On the standard VGA there is no way to modify this behavior.
        // TODO: How is dup9 handled here?
        cursor_start_line = std::min(cursor_start_line, character_height);
        cursor_end_line = std::min(cursor_end_line, character_height);
        for (uint32 cursor_line = cursor_start_line; cursor_line < cursor_end_line; cursor_line++)
        {
          for (uint32 i = 0; i < character_width; i++)
            m_display->SetPixel(col * character_width + i, row * character_height + cursor_line, foreground_color);
        }
      }
    }
  }

  m_display->SwapFramebuffer();
}

void VGA::DrawTextGlyph8(uint32 fb_x, uint32 fb_y, const uint8* glyph, uint32 rows, uint32 fg_color, uint32 bg_color,
                         int32 dup9)
{
  const uint32 colors[2] = {bg_color, fg_color};

  for (uint32 row = 0; row < rows; row++)
  {
    uint8 source_row = *glyph;
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

void VGA::DrawTextGlyph16(uint32 fb_x, uint32 fb_y, const uint8* glyph, uint32 rows, uint32 fg_color, uint32 bg_color)
{
  const uint32 colors[2] = {bg_color, fg_color};

  for (uint32 row = 0; row < rows; row++)
  {
    uint8 source_row = *glyph;
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

//#define FAST_VGA_RENDER 1

// https://ia801809.us.archive.org/11/items/bitsavers_ibmpccardseferenceManualMay92_1756350/IBM_VGA_XGA_Technical_Reference_Manual_May92.pdf

void VGA::RenderGraphicsMode()
{
  uint32 screen_width = (uint32(m_crtc_registers.end_horizontal_display) + 1) * 8;
  uint32 screen_height =
    (m_crtc_registers.vertical_display_end | (uint32(m_crtc_registers.overflow_register & 0x02) << 7) |
     (uint32(m_crtc_registers.overflow_register & 0x40) << 3)) +
    1;
  if (screen_width == 0 || screen_height == 0)
    return;

  // Horizontal resolution must always be divisible by 8, this is an assumption made by the line rendering code.
  Assert((screen_width % 8) == 0);

  // Maximum scan line determines the number of scanlines per row of bytes
  uint32 scanlines_per_row = ZeroExtend32(m_crtc_registers.maximum_scan_lines & 0x1F) + 1;

  // Scan doubling, used for CGA modes
  // This causes the row scan counter to increment at half the rate, so when scanlines_per_row = 1,
  // address counter is always divided by two as well (for CGA modes).
  bool double_scan = !!(m_crtc_registers.maximum_scan_lines & 0x80);

#ifdef FAST_VGA_RENDER
  // We can skip rendering the doubled scanlines by dividing the height by 2.
  if (double_scan)
    screen_height /= 2;
  if (m_graphics_registers.mode.shift_256)
    screen_width /= 2;
#endif

  // Non-divided clock multiplies by 2
  uint32 pixels_per_col = 1;
  if (!m_graphics_registers.mode.shift_256 && m_sequencer_registers.clocking_mode.dot_clock_rate)
  {
    pixels_per_col = 2;
    screen_width *= 2;
  }
  // attribute register used here too

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
  uint32 data_base_address =
    (ZeroExtend32(m_crtc_registers.start_address_high) << 8) | (ZeroExtend32(m_crtc_registers.start_address_low));
  data_base_address += (m_crtc_registers.preset_row_scan >> 5) & 0x03;

  // Line compare, address resets to zero at this line
  uint32 line_compare = (ZeroExtend32(m_crtc_registers.line_compare)) |
                        (ZeroExtend32(m_crtc_registers.overflow_register & 0x10) << 4) |
                        (ZeroExtend32(m_crtc_registers.maximum_scan_lines & 0x40) << 3);

  // Determine the pitch of each line
  uint32 row_pitch = ZeroExtend32(m_crtc_registers.offset) * 2;

  // Divide memory address clock by 2 - not sure if this is correct, should it increase the horizontal resolution?
  // if (m_crtc_registers.crtc_mode_control & 0x08)
  // data_width /= 2;

  uint32 horizontal_pan = m_attribute_registers.horizontal_pixel_panning;
  if (horizontal_pan >= 8)
    horizontal_pan = 0;
  horizontal_pan *= pixels_per_col;

  // preset_row_scan[4:0] contains the starting row scan number, cleared when it hits max.
  uint32 row_counter = 0;
  uint32 row_scan_counter = ZeroExtend32(m_crtc_registers.preset_row_scan & 0x1F);

  // Draw lines
  for (uint32 scanline = 0; scanline < screen_height; scanline++)
  {
    if (scanline == line_compare)
    {
      // TODO: pixel_panning_mode determines whether to reset horizontal_pan
      data_base_address = 0;
      row_counter = 0;
      row_scan_counter = 0;
      horizontal_pan = 0;
    }

    uint32 address_counter = (data_base_address + (row_pitch * row_counter));

    // 4 or 16 color mode?
    if (!m_graphics_registers.mode.shift_256)
    {
      if (m_graphics_registers.mode.shift_reg)
      {
        // CGA mode - Shift register in interleaved mode, odd bits from odd maps and even bits from even maps
        for (uint32 col = 0; col < screen_width;)
        {
          uint32 all_planes = CRTCReadVRAMPlanes(address_counter, row_scan_counter);
          address_counter++;

          uint8 pl0 = Truncate8((all_planes >> 0) & 0xFF);
          uint8 pl1 = Truncate8((all_planes >> 8) & 0xFF);
          uint8 pl2 = Truncate8((all_planes >> 16) & 0xFF);
          uint8 pl3 = Truncate8((all_planes >> 24) & 0xFF);
          uint8 index;

          // Low-resolution graphics, 2 bits per pixel
          if (!m_sequencer_registers.clocking_mode.dot_clock_rate)
          {
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
          else
          {
            // Two pixels per input pixel
            index = ((pl0 >> 6) & 3) | (((pl2 >> 6) & 3) << 2);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            index = ((pl0 >> 4) & 3) | (((pl2 >> 6) & 3) << 2);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            index = ((pl0 >> 2) & 3) | (((pl2 >> 6) & 3) << 2);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            index = ((pl0 >> 0) & 3) | (((pl2 >> 6) & 3) << 2);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);

            index = ((pl1 >> 6) & 3) | (((pl3 >> 6) & 3) << 2);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            index = ((pl1 >> 4) & 3) | (((pl3 >> 6) & 3) << 2);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            index = ((pl1 >> 2) & 3) | (((pl3 >> 6) & 3) << 2);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            index = ((pl1 >> 0) & 3) | (((pl3 >> 6) & 3) << 2);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
            m_display->SetPixel(col++, scanline, m_output_palette[index]);
          }
        }
      }
      else
      {
        // 16 color mode.
        // Output 8 pixels for one dword
        for (int32 col = -(int32)horizontal_pan; col < (int32)screen_width;)
        {
          uint32 all_planes = CRTCReadVRAMPlanes(address_counter, row_scan_counter);
          address_counter++;

          uint8 pl0 = Truncate8((all_planes >> 0) & 0xFF);
          uint8 pl1 = Truncate8((all_planes >> 8) & 0xFF);
          uint8 pl2 = Truncate8((all_planes >> 16) & 0xFF);
          uint8 pl3 = Truncate8((all_planes >> 24) & 0xFF);

          uint8 indices[8] = {
            uint8(((pl0 >> 7) & 1u) | (((pl1 >> 7) & 1u) << 1) | (((pl2 >> 7) & 1u) << 2) | (((pl3 >> 7) & 1u) << 3)),
            uint8(((pl0 >> 6) & 1u) | (((pl1 >> 6) & 1u) << 1) | (((pl2 >> 6) & 1u) << 2) | (((pl3 >> 6) & 1u) << 3)),
            uint8(((pl0 >> 5) & 1u) | (((pl1 >> 5) & 1u) << 1) | (((pl2 >> 5) & 1u) << 2) | (((pl3 >> 5) & 1u) << 3)),
            uint8(((pl0 >> 4) & 1u) | (((pl1 >> 4) & 1u) << 1) | (((pl2 >> 4) & 1u) << 2) | (((pl3 >> 4) & 1u) << 3)),
            uint8(((pl0 >> 3) & 1u) | (((pl1 >> 3) & 1u) << 1) | (((pl2 >> 3) & 1u) << 2) | (((pl3 >> 3) & 1u) << 3)),
            uint8(((pl0 >> 2) & 1u) | (((pl1 >> 2) & 1u) << 1) | (((pl2 >> 2) & 1u) << 2) | (((pl3 >> 2) & 1u) << 3)),
            uint8(((pl0 >> 1) & 1u) | (((pl1 >> 1) & 1u) << 1) | (((pl2 >> 1) & 1u) << 2) | (((pl3 >> 1) & 1u) << 3)),
            uint8(((pl0 >> 0) & 1u) | (((pl1 >> 0) & 1u) << 1) | (((pl2 >> 0) & 1u) << 2) | (((pl3 >> 0) & 1u) << 3))};

          for (uint32 subcol = 0, subindex = 0; col < (int32)screen_width && subindex < 8;)
          {
            if (col >= 0 && col < (int32)screen_width)
              m_display->SetPixel(col, scanline, m_output_palette[indices[subindex]]);

            col++;
            subcol++;
            if (subcol == pixels_per_col)
            {
              subindex++;
              subcol = 0;
            }
          }
        }
      }
    }
    else
    {
      uint32 pan_pixels = (horizontal_pan & 7) / 2;

      // Slow loop with panning part
#ifdef FAST_VGA_RENDER
      int32 col = -int32(pan_pixels * 2);
      int32 screen_width_div2 = int32(screen_width);
#else
      int32 col = -int32(pan_pixels);
      int32 screen_width_div2 = int32(screen_width / 2);
#endif
      while (col < 0)
      {
        uint32 indices = CRTCReadVRAMPlanes(address_counter, row_scan_counter);
        address_counter++;

        for (uint32 i = 0; i < 4; i++)
        {
          uint8 index = Truncate8(indices);
          uint32 color = m_output_palette[index];
          indices >>= 8;

          if (col >= 0)
          {
#ifdef FAST_VGA_RENDER
            m_display->SetPixel(col, scanline, color);
#else
            m_display->SetPixel(col * 2 + 0, scanline, color);
            m_display->SetPixel(col * 2 + 1, scanline, color);
#endif
          }

          col++;
        }
      }

      // Fast loop without partial panning
      while ((col + 4) <= screen_width_div2)
      {
        // Load 4 pixels, one from each plane
        // Duplicate horizontally twice, this is the shift_256 stuff
        uint32 indices = CRTCReadVRAMPlanes(address_counter, row_scan_counter);
        address_counter++;

#ifdef FAST_VGA_RENDER
        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 0) & 0xFF]);
        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 8) & 0xFF]);
        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 16) & 0xFF]);
        m_display->SetPixel(col++, scanline, m_output_palette[(indices >> 24) & 0xFF]);
#else
        m_display->SetPixel(col * 2 + 0, scanline, m_output_palette[(indices >> 0) & 0xFF]);
        m_display->SetPixel(col * 2 + 1, scanline, m_output_palette[(indices >> 0) & 0xFF]);
        col++;

        m_display->SetPixel(col * 2 + 0, scanline, m_output_palette[(indices >> 8) & 0xFF]);
        m_display->SetPixel(col * 2 + 1, scanline, m_output_palette[(indices >> 8) & 0xFF]);
        col++;

        m_display->SetPixel(col * 2 + 0, scanline, m_output_palette[(indices >> 16) & 0xFF]);
        m_display->SetPixel(col * 2 + 1, scanline, m_output_palette[(indices >> 16) & 0xFF]);
        col++;

        m_display->SetPixel(col * 2 + 0, scanline, m_output_palette[(indices >> 24) & 0xFF]);
        m_display->SetPixel(col * 2 + 1, scanline, m_output_palette[(indices >> 24) & 0xFF]);
        col++;
#endif
      }

      // Slow loop to handle misaligned buffer when panning
      while (col < screen_width_div2)
      {
        uint32 indices = CRTCReadVRAMPlanes(address_counter, row_scan_counter);
        address_counter++;

        for (uint32 i = 0; i < 4; i++)
        {
          uint8 index = Truncate8(indices);
          uint32 color = m_output_palette[index];
          indices >>= 8;

          if (col < screen_width_div2)
          {
#ifdef FAST_VGA_RENDER
            m_display->SetPixel(col, scanline, color);
#else
            m_display->SetPixel(col * 2 + 0, scanline, color);
            m_display->SetPixel(col * 2 + 1, scanline, color);
#endif
          }
          else
          {
            break;
          }

          col++;
        }
      }
    }

#ifndef FAST_VGA_RENDER
    if (!double_scan || (scanline & 1) != 0)
#endif
    {
      row_scan_counter++;
      if (row_scan_counter == scanlines_per_row)
      {
        row_scan_counter = 0;
        row_counter++;
      }
    }
  }

  m_display->SwapFramebuffer();
}
} // namespace HW