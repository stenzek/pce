#include "pce/systems/ali1429.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu.h"
Log_SetChannel(Systems::ALi1429);

namespace Systems {
DEFINE_OBJECT_TYPE_INFO(ALi1429);
DEFINE_OBJECT_GENERIC_FACTORY(ALi1429);
BEGIN_OBJECT_PROPERTY_MAP(ALi1429)
END_OBJECT_PROPERTY_MAP()

ALi1429::ALi1429(CPU_X86::Model model /* = CPU_X86::MODEL_486 */, float cpu_frequency /* = 2000000.0f */,
                 uint32 memory_size /* = 16 * 1024 * 1024 */, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(type_info), m_bios_file_path("romimages/4alp001.bin")
{
  m_bus = new Bus(PHYSICAL_MEMORY_BITS);
  m_cpu = new CPU_X86::CPU("CPU", model, cpu_frequency);
  AllocatePhysicalMemory(memory_size, false, false);
  AddComponents();
}

ALi1429::~ALi1429() {}

bool ALi1429::Initialize()
{
  if (!BaseClass::Initialize())
    return false;

  if (!m_bus->CreateROMRegionFromFile(m_bios_file_path.c_str(), BIOS_ROM_ADDRESS, BIOS_ROM_SIZE))
    return false;

  m_bus->MirrorRegion(BIOS_ROM_ADDRESS, BIOS_ROM_SIZE, BIOS_ROM_MIRROR_ADDRESS);

  ConnectSystemIOPorts();
  SetCMOSVariables();
  return true;
}

void ALi1429::Reset()
{
  ISAPC::Reset();

  m_cmos_lock = false;
  m_refresh_bit = false;

  std::fill(m_ali1429_registers.begin(), m_ali1429_registers.end(), uint8(0));
  m_ali1429_index_register = 0;

  // Set keyboard controller input port up.
  // b7 = Keyboard not inhibited, b5 = POST loop inactive
  m_keyboard_controller->SetInputPort(0xA0);

  // Start with A20 line on
  SetA20State(true);
  UpdateKeyboardControllerOutputPort();
  UpdateShadowRAM();
}

bool ALi1429::LoadSystemState(BinaryReader& reader)
{
  if (!ISAPC::LoadSystemState(reader))
    return false;

  reader.SafeReadBool(&m_cmos_lock);
  reader.SafeReadBool(&m_refresh_bit);
  return !reader.GetErrorState();
}

bool ALi1429::SaveSystemState(BinaryWriter& writer)
{
  if (!ISAPC::SaveSystemState(writer))
    return false;

  writer.SafeWriteBool(m_cmos_lock);
  writer.SafeWriteBool(m_refresh_bit);
  return !writer.InErrorState();
}

void ALi1429::ConnectSystemIOPorts()
{
  m_bus->ConnectIOPortRead(0x0022, this, std::bind(&ALi1429::IOReadALI1429IndexRegister, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0022, this,
                            std::bind(&ALi1429::IOWriteALI1429IndexRegister, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x0023, this, std::bind(&ALi1429::IOReadALI1429DataRegister, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0023, this, std::bind(&ALi1429::IOWriteALI1429DataRegister, this, std::placeholders::_2));
  // System control ports
  m_bus->ConnectIOPortRead(0x0092, this, std::bind(&ALi1429::IOReadSystemControlPortA, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0092, this, std::bind(&ALi1429::IOWriteSystemControlPortA, this, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x0061, this, std::bind(&ALi1429::IOReadSystemControlPortB, this, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0061, this, std::bind(&ALi1429::IOWriteSystemControlPortB, this, std::placeholders::_2));

  // Connect the keyboard controller output port to the lower 2 bits of system control port A.
  m_keyboard_controller->SetOutputPortWrittenCallback([this](uint8 value, uint8 old_value, bool pulse) {
    if (!pulse)
      value &= ~uint8(0x01);
    IOWriteSystemControlPortA(value & 0x03);
    IOReadSystemControlPortA(&value);
    m_keyboard_controller->SetOutputPort(value);
  });
}

void ALi1429::IOReadALI1429IndexRegister(uint8* value)
{
  *value = m_ali1429_index_register;
}

void ALi1429::IOWriteALI1429IndexRegister(uint8 value)
{
  m_ali1429_index_register = value;
}

void ALi1429::IOReadALI1429DataRegister(uint8* value)
{
  *value = m_ali1429_registers[m_ali1429_index_register];
}

void ALi1429::IOWriteALI1429DataRegister(uint8 value)
{
  m_ali1429_registers[m_ali1429_index_register] = value;

  if (m_ali1429_index_register == 0x13 || m_ali1429_index_register == 0x14)
  {
    // Recalculate shadowing.
    UpdateShadowRAM();
  }
}

void ALi1429::UpdateShadowRAM()
{
  for (uint32 i = 0; i < 8; i++)
  {
    uint32 base = UINT32_C(0xC0000) + (i << 15);
    if (m_ali1429_registers[0x13] & (1 << i))
    {
      // Shadowing enabled for this region.
      const uint32 flag = m_ali1429_registers[0x14] & 0x03;
      const bool readable_memory = !!(flag & 1);
      const bool writable_memory = !!(flag & 2);
      Log_DevPrintf("Shadowing ENABLED for 0x%08X-0x%08X (type %u, readable=%s, writable=%s)", base,
                    base + SHADOW_REGION_SIZE - 1, flag, readable_memory ? "yes" : "no",
                    writable_memory ? "yes" : "no");
      m_bus->SetPagesRAMState(base, SHADOW_REGION_SIZE, readable_memory, writable_memory);
    }
    else
    {
      Log_DevPrintf("Shadowing DISABLED for 0x%08X-0x%08X", base, base + SHADOW_REGION_SIZE - 1);
      m_bus->SetPagesRAMState(base, SHADOW_REGION_SIZE, false, false);
    }
  }
}

void ALi1429::IOReadSystemControlPortA(uint8* value)
{
  *value = (BoolToUInt8(m_cmos_lock) << 3) | (BoolToUInt8(GetA20State()) << 1);
}

void ALi1429::IOWriteSystemControlPortA(uint8 value)
{
  Log_DevPrintf("Write system control port A: 0x%02X", ZeroExtend32(value));

  // b7-6 - Activity Lights
  // b5 - Reserved
  // b4 - Watchdog Timeout
  // b3 - CMOS Security Lock
  // b2 - Reserved
  // b1 - A20 Active
  // b0 - System Reset

  bool cmos_security_lock = !!(value & (1 << 2));
  bool new_a20_state = !!(value & (1 << 1));
  bool system_reset = !!(value & (1 << 0));

  // Update A20 state
  if (GetA20State() != new_a20_state)
  {
    SetA20State(new_a20_state);
    UpdateKeyboardControllerOutputPort();
  }

  m_cmos_lock = cmos_security_lock;

  // System reset?
  // We do this last as it's going to destroy everything.
  // TODO: We should probably put it on an event though..
  if (system_reset)
  {
    Log_WarningPrintf("CPU reset via system control port");
    m_cpu->Reset();
  }
}

void ALi1429::IOReadSystemControlPortB(uint8* value)
{
  *value = (BoolToUInt8(m_timer->GetChannelGateInput(2)) << 0) |  // Timer 2 gate input
           (BoolToUInt8(m_speaker->IsOutputEnabled()) << 1) |     // Speaker data status
           (BoolToUInt8(m_refresh_bit) << 4) |                    // Triggers with each memory refresh
           (BoolToUInt8(m_timer->GetChannelOutputState(2)) << 5); // Raw timer 2 output

  // Seems that we can get away with faking this every read.
  // The refresh controller steps one refresh address every 15 microseconds. Each refresh cycle
  // requires eight clock cycles to refresh all of the system's dynamic memory; 256 refresh cycles
  // are required every 4 milliseconds, but the system hardware refreshes every 3.84ms.
  m_refresh_bit ^= true;
}

void ALi1429::IOWriteSystemControlPortB(uint8 value)
{
  Log_DevPrintf("Write system control port B: 0x%02X", ZeroExtend32(value));

  m_timer->SetChannelGateInput(2, !!(value & (1 << 0))); // Timer 2 gate input
  m_speaker->SetOutputEnabled(!!(value & (1 << 1)));     // Speaker data enable
}

void ALi1429::UpdateKeyboardControllerOutputPort()
{
  uint8 value = m_keyboard_controller->GetOutputPort();
  value &= ~uint8(0x03);
  value |= (BoolToUInt8(GetA20State()) << 1);
  m_keyboard_controller->SetOutputPort(value);
}

void ALi1429::AddComponents()
{
  m_interrupt_controller = CreateComponent<HW::i8259_PIC>("InterruptController");
  m_dma_controller = CreateComponent<HW::i8237_DMA>("DMAController");
  m_timer = CreateComponent<HW::i8253_PIT>("PIT");
  m_keyboard_controller = CreateComponent<HW::i8042_PS2>("KeyboardController");
  m_cmos = CreateComponent<HW::CMOS>("CMOS");
  m_speaker = CreateComponent<HW::PCSpeaker>("Speaker");

  m_fdd_controller = CreateComponent<HW::FDC>("FDC", HW::FDC::Model_8272);
  m_hdd_controller = CreateComponent<HW::HDC>("HDC", 1);

  // Connect channel 0 of the PIT to the interrupt controller
  m_timer->SetChannelOutputChangeCallback(0,
                                          [this](bool value) { m_interrupt_controller->SetInterruptState(0, value); });

  // Connect channel 2 of the PIT to the speaker
  m_timer->SetChannelOutputChangeCallback(2, [this](bool value) { m_speaker->SetLevel(value); });
}

void ALi1429::SetCMOSVariables() {}

} // namespace Systems