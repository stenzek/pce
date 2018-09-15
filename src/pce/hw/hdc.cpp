#include "pce/hw/hdc.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/hw/ata_device.h"
#include "pce/hw/ata_hdd.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cstring>
Log_SetChannel(HW::HDC);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(HDC);
BEGIN_OBJECT_PROPERTY_MAP(HDC)
END_OBJECT_PROPERTY_MAP()

HDC::HDC(const String& identifier, Channel channel /* = Channel::Primary */,
         const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_channel(channel)
{
}

HDC::~HDC() = default;

bool HDC::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_interrupt_controller = m_system->GetComponentByType<InterruptController>();
  if (!m_interrupt_controller)
  {
    Log_ErrorPrintf("Failed to locate interrupt controller.");
    return false;
  }

  ConnectIOPorts(bus);
  return true;
}

void HDC::Reset()
{
  BaseClass::Reset();

  std::fill_n(m_device_interrupt_lines, NUM_DEVICES, false);
  UpdateHostInterruptLine();
}

bool HDC::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  uint32 channel = reader.ReadUInt32();
  if (channel != static_cast<uint32>(m_channel))
    return false;

  for (uint32 i = 0; i < NUM_DEVICES; i++)
  {
    const bool present = reader.ReadBool();
    if (present != IsDevicePresent(i))
    {
      Log_ErrorPrintf("Save state mismatch for drive %u", i);
      return false;
    }
  }

  m_control_register.bits = reader.ReadUInt8();
  m_drive_select_register.bits = reader.ReadUInt8();
  return !reader.GetErrorState();
}

bool HDC::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteUInt32(static_cast<uint32>(m_channel));

  for (uint32 i = 0; i < NUM_DEVICES; i++)
    writer.WriteBool(IsDevicePresent(i));

  writer.WriteUInt8(m_control_register.bits);
  writer.WriteUInt8(m_drive_select_register.bits);
  return !writer.InErrorState();
}

u32 HDC::GetDeviceCount() const
{
  u32 count = 0;
  for (u32 i = 0; i < NUM_DEVICES; i++)
  {
    if (m_devices[i])
      count++;
  }
  return count;
}

bool HDC::IsHDDPresent(u32 number) const
{
  return (number < NUM_DEVICES && m_devices[number]) ? m_devices[number]->IsDerived<ATAHDD>() : false;
}

u32 HDC::GetHDDCylinders(u32 number) const
{
  const ATAHDD* hdd = (number < NUM_DEVICES && m_devices[number]) ? m_devices[number]->SafeCast<ATAHDD>() : nullptr;
  return hdd ? hdd->GetNumCylinders() : 0;
}

u32 HDC::GetHDDHeads(u32 number) const
{
  const ATAHDD* hdd = (number < NUM_DEVICES && m_devices[number]) ? m_devices[number]->SafeCast<ATAHDD>() : nullptr;
  return hdd ? hdd->GetNumHeads() : 0;
}

u32 HDC::GetHDDSectors(u32 number) const
{
  const ATAHDD* hdd = (number < NUM_DEVICES && m_devices[number]) ? m_devices[number]->SafeCast<ATAHDD>() : nullptr;
  return hdd ? hdd->GetNumSectors() : 0;
}

bool HDC::AttachDevice(u32 number, ATADevice* device)
{
  if (number >= NUM_DEVICES || m_devices[number])
    return false;

  m_devices[number] = device;
  return true;
}

void HDC::DetachDevice(u32 number)
{
  if (number >= NUM_DEVICES)
    return;

  m_devices[NUM_DEVICES] = nullptr;
}

void HDC::SetDeviceInterruptLine(u32 number, bool active)
{
  m_device_interrupt_lines[number] = active;
  UpdateHostInterruptLine();
}

void HDC::UpdateHostInterruptLine()
{
  // TODO: Is this correct?
  const bool state =
    (!m_control_register.disable_interrupts) && (m_device_interrupt_lines[0] | m_device_interrupt_lines[1]);
  m_interrupt_controller->SetInterruptState(m_irq_number, state);
}

void HDC::ConnectIOPorts(Bus* bus)
{
  u16 BAR0, BAR1;
  if (m_channel == Channel::Primary)
  {
    // Primary channel
    BAR0 = 0x01F0;
    BAR1 = 0x03F6;
    m_irq_number = 14;
  }
  else
  {
    // Secondary channel
    BAR0 = 0x0170;
    BAR1 = 0x0376;
    m_irq_number = 15;
  }

  // 01F0 - Data register (R/W)
  bus->ConnectIOPortRead(BAR0 + 0, this, std::bind(&HDC::IOReadDataRegisterByte, this, std::placeholders::_2));
  bus->ConnectIOPortReadWord(BAR0 + 0, this, std::bind(&HDC::IOReadDataRegisterWord, this, std::placeholders::_2));
  bus->ConnectIOPortReadDWord(BAR0 + 0, this, std::bind(&HDC::IOReadDataRegisterDWord, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 0, this, std::bind(&HDC::IOWriteDataRegisterByte, this, std::placeholders::_2));
  bus->ConnectIOPortWriteWord(BAR0 + 0, this, std::bind(&HDC::IOWriteDataRegisterWord, this, std::placeholders::_2));
  bus->ConnectIOPortWriteDWord(BAR0 + 0, this, std::bind(&HDC::IOWriteDataRegisterDWord, this, std::placeholders::_2));

  // 01F1 - Status register (R)
  // 01F1	w	WPC/4  (Write Precompensation Cylinder divided by 4)
  bus->ConnectIOPortRead(BAR0 + 1, this, std::bind(&HDC::IOReadErrorRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 1, this, std::bind(&HDC::IOWriteCommandBlockFeatures, this, std::placeholders::_2));

  // Command block
  // 01F2	r/w	sector count
  // 01F3	r/w	sector number
  // 01F4	r/w	cylinder low
  // 01F5	r/w	cylinder high
  bus->ConnectIOPortRead(BAR0 + 2, this, std::bind(&HDC::IOReadCommandBlockSectorCount, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 2, this, std::bind(&HDC::IOWriteCommandBlockSectorCount, this, std::placeholders::_2));
  bus->ConnectIOPortRead(BAR0 + 3, this, std::bind(&HDC::IOReadCommandBlockSectorNumber, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 3, this,
                          std::bind(&HDC::IOWriteCommandBlockSectorNumber, this, std::placeholders::_2));
  bus->ConnectIOPortRead(BAR0 + 4, this, std::bind(&HDC::IOReadCommandBlockCylinderLow, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 4, this, std::bind(&HDC::IOWriteCommandBlockCylinderLow, this, std::placeholders::_2));
  bus->ConnectIOPortRead(BAR0 + 5, this, std::bind(&HDC::IOReadCommandBlockCylinderHigh, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 5, this,
                          std::bind(&HDC::IOWriteCommandBlockCylinderHigh, this, std::placeholders::_2));

  // 01F6: Drive select (R/W)
  bus->ConnectIOPortRead(BAR0 + 6, this, std::bind(&HDC::IOReadDriveSelectRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 6, this, std::bind(&HDC::IOWriteDriveSelectRegister, this, std::placeholders::_2));

  // 01F7 - Status register (R) / Command register (W)
  bus->ConnectIOPortRead(BAR0 + 7, this, std::bind(&HDC::IOReadStatusRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 7, this, std::bind(&HDC::IOWriteCommandRegister, this, std::placeholders::_2));

  // 03F7: Alternate status register (R) / Control register (W)
  bus->ConnectIOPortRead(BAR1 + 0, this, std::bind(&HDC::IOReadAltStatusRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR1 + 0, this, std::bind(&HDC::IOWriteControlRegister, this, std::placeholders::_2));
}

void HDC::SoftReset()
{
  for (u32 i = 0; i < NUM_DEVICES; i++)
  {
    if (m_devices[i])
      m_devices[i]->DoReset(false);

    m_device_interrupt_lines[i] = false;
  }

  UpdateHostInterruptLine();
}

void HDC::IOReadStatusRegister(u8* value)
{
  // Lower interrupt
  std::fill_n(m_device_interrupt_lines, NUM_DEVICES, false);
  UpdateHostInterruptLine();

  if (m_control_register.software_reset)
  {
    *value = (1 << 7);
    return;
  }

  const ATADevice* device = GetCurrentDevice();
  *value = device ? device->ReadStatusRegister() : 0xFF;
}

void HDC::IOReadAltStatusRegister(u8* value)
{
  if (m_control_register.software_reset)
  {
    *value = (1 << 7);
    return;
  }

  const ATADevice* device = GetCurrentDevice();
  *value = device ? device->ReadStatusRegister() : 0xFF;
}

void HDC::IOWriteCommandRegister(u8 value)
{
  Log_TracePrintf("ATA write command register <- 0x%02X", ZeroExtend32(value));

  const u8 index = GetCurrentDeviceIndex();
  if (!m_devices[index])
    return;

  m_device_interrupt_lines[index] = false;
  UpdateHostInterruptLine();

  m_devices[index]->WriteCommandRegister(value);
}

void HDC::IOReadErrorRegister(u8* value)
{
  const ATADevice* device = GetCurrentDevice();
  *value = device ? device->ReadErrorRegister() : 0xFF;
}

void HDC::IOWriteControlRegister(u8 value)
{
  Log_TracePrintf("ATA write control register <- 0x%02X", ZeroExtend32(value));

  decltype(m_control_register) old_value = m_control_register;
  m_control_register.bits = value;

  if (!m_control_register.software_reset && old_value.software_reset)
  {
    // Software reset
    Log_DevPrintf("ATA controller software reset");
    SoftReset();
  }
}

void HDC::IOReadDriveSelectRegister(u8* value)
{
  const ATADevice* device = GetCurrentDevice();
  *value = device ? device->ReadCommandBlockDriveSelect() : m_drive_select_register.bits;
}

void HDC::IOWriteDriveSelectRegister(u8 value)
{
  for (u32 i = 0; i < NUM_DEVICES; i++)
  {
    if (m_devices[i])
      m_devices[i]->WriteCommandBlockDriveSelect(value);
  }

  m_drive_select_register.bits = value;
  Log_TracePrintf("ATA write drive select 0x%02X (drive %u)", value, m_drive_select_register.drive.GetValue());
}

void HDC::IOReadDataRegisterByte(u8* value)
{
  ATADevice* device = GetCurrentDevice();
  if (device)
    device->ReadDataPort(value, sizeof(*value));
  else
    *value = 0xFF;
}

void HDC::IOReadDataRegisterWord(u16* value)
{
  ATADevice* device = GetCurrentDevice();
  if (device)
    device->ReadDataPort(value, sizeof(*value));
  else
    *value = 0xFFFF;
}

void HDC::IOReadDataRegisterDWord(u32* value)
{
  ATADevice* device = GetCurrentDevice();
  if (device)
    device->ReadDataPort(value, sizeof(*value));
  else
    *value = UINT32_C(0xFFFFFFFF);
}

void HDC::IOWriteDataRegisterByte(u8 value)
{
  ATADevice* device = GetCurrentDevice();
  if (device)
    device->WriteDataPort(&value, sizeof(value));
}

void HDC::IOWriteDataRegisterWord(u16 value)
{
  ATADevice* device = GetCurrentDevice();
  if (device)
    device->WriteDataPort(&value, sizeof(value));
}

void HDC::IOWriteDataRegisterDWord(u32 value)
{
  ATADevice* device = GetCurrentDevice();
  if (device)
    device->WriteDataPort(&value, sizeof(value));
}

void HDC::IOReadCommandBlockSectorCount(u8* value)
{
  const ATADevice* device = GetCurrentDevice();
  *value = device ? device->ReadCommandBlockSectorCount(m_control_register.high_order_byte_readback) : 0xFF;
}

void HDC::IOReadCommandBlockSectorNumber(u8* value)
{
  const ATADevice* device = GetCurrentDevice();
  *value = device ? device->ReadCommandBlockSectorNumber(m_control_register.high_order_byte_readback) : 0xFF;
}

void HDC::IOReadCommandBlockCylinderLow(u8* value)
{
  const ATADevice* device = GetCurrentDevice();
  *value = device ? device->ReadCommandBlockCylinderLow(m_control_register.high_order_byte_readback) : 0xFF;
}

void HDC::IOReadCommandBlockCylinderHigh(u8* value)
{
  const ATADevice* device = GetCurrentDevice();
  *value = device ? device->ReadCommandBlockCylinderHigh(m_control_register.high_order_byte_readback) : 0xFF;
}

void HDC::IOWriteCommandBlockFeatures(u8 value)
{
  Log_TracePrintf("ATA write command block features 0x%02X", value);
  for (u32 i = 0; i < NUM_DEVICES; i++)
  {
    if (m_devices[i])
      m_devices[i]->WriteFeatureSelect(value);
  }
}

void HDC::IOWriteCommandBlockSectorCount(u8 value)
{
  Log_TracePrintf("ATA write command block sector count 0x%02X", value);
  for (u32 i = 0; i < NUM_DEVICES; i++)
  {
    if (m_devices[i])
      m_devices[i]->WriteCommandBlockSectorCount(value);
  }
}

void HDC::IOWriteCommandBlockSectorNumber(u8 value)
{
  Log_TracePrintf("ATA write command block sector number 0x%02X", value);
  for (u32 i = 0; i < NUM_DEVICES; i++)
  {
    if (m_devices[i])
      m_devices[i]->WriteCommandBlockSectorNumber(value);
  }
}

void HDC::IOWriteCommandBlockCylinderLow(u8 value)
{
  Log_TracePrintf("ATA write command block cylinder low 0x%02X", value);
  for (u32 i = 0; i < NUM_DEVICES; i++)
  {
    if (m_devices[i])
      m_devices[i]->WriteCommandBlockSectorCylinderLow(value);
  }
}

void HDC::IOWriteCommandBlockCylinderHigh(u8 value)
{
  Log_TracePrintf("ATA write command block cylinder high 0x%02X", value);
  for (u32 i = 0; i < NUM_DEVICES; i++)
  {
    if (m_devices[i])
      m_devices[i]->WriteCommandBlockSectorCylinderHigh(value);
  }
}

} // namespace HW