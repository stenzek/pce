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

HDC::HDC(const String& identifier, u32 num_channels /* = 1 */, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_num_channels(num_channels)
{
  Assert(num_channels <= MAX_CHANNELS);
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
  for (u32 i = 0; i < m_num_channels; i++)
    DoReset(i, true);
}

bool HDC::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  const u32 num_channels = reader.ReadUInt32();
  if (num_channels != m_num_channels)
    return false;

  for (u32 channel = 0; channel < m_num_channels; channel++)
  {
    for (u32 i = 0; i < DEVICES_PER_CHANNEL; i++)
    {
      const bool present = reader.ReadBool();
      if (present != IsDevicePresent(channel, i))
      {
        Log_ErrorPrintf("Save state mismatch for channel %u drive %u", channel, i);
        return false;
      }
    }

    m_channels[channel].control_register.bits = reader.ReadUInt8();
    m_channels[channel].drive_select_register.bits = reader.ReadUInt8();
    reader.SafeReadBytes(m_channels[channel].device_interrupt_lines,
                         sizeof(m_channels[channel].device_interrupt_lines));
    UpdateHostInterruptLine(channel);
  }

  return !reader.GetErrorState();
}

bool HDC::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteUInt32(m_num_channels);

  for (u32 i = 0; i < m_num_channels; i++)
  {
    for (u32 j = 0; j < DEVICES_PER_CHANNEL; j++)
      writer.WriteBool(IsDevicePresent(i, j));

    writer.WriteUInt8(m_channels[i].control_register.bits);
    writer.WriteUInt8(m_channels[i].drive_select_register.bits);
    writer.WriteBytes(m_channels[i].device_interrupt_lines, sizeof(m_channels[i].device_interrupt_lines));
  }

  return !writer.InErrorState();
}

bool HDC::IsDevicePresent(u32 channel, u32 number) const
{
  return (channel < m_num_channels && number < DEVICES_PER_CHANNEL && m_channels[channel].devices[number]);
}

u32 HDC::GetDeviceCount(u32 channel) const
{
  u32 count = 0;
  for (u32 i = 0; i < DEVICES_PER_CHANNEL; i++)
  {
    if (m_channels[channel].devices[i])
      count++;
  }
  return count;
}

bool HDC::IsHDDPresent(u32 channel, u32 number) const
{
  return (channel < m_num_channels && number < DEVICES_PER_CHANNEL && m_channels[channel].devices[number]) ?
           m_channels[channel].devices[number]->IsDerived<ATAHDD>() :
           false;
}

u32 HDC::GetHDDCylinders(u32 channel, u32 number) const
{
  const ATAHDD* hdd =
    (channel < m_num_channels && number < DEVICES_PER_CHANNEL && m_channels[channel].devices[number]) ?
      m_channels[channel].devices[number]->SafeCast<ATAHDD>() :
      nullptr;
  return hdd ? hdd->GetNumCylinders() : 0;
}

u32 HDC::GetHDDHeads(u32 channel, u32 number) const
{
  const ATAHDD* hdd =
    (channel < m_num_channels && number < DEVICES_PER_CHANNEL && m_channels[channel].devices[number]) ?
      m_channels[channel].devices[number]->SafeCast<ATAHDD>() :
      nullptr;
  return hdd ? hdd->GetNumHeads() : 0;
}

u32 HDC::GetHDDSectors(u32 channel, u32 number) const
{
  const ATAHDD* hdd =
    (channel < m_num_channels && number < DEVICES_PER_CHANNEL && m_channels[channel].devices[number]) ?
      m_channels[channel].devices[number]->SafeCast<ATAHDD>() :
      nullptr;
  return hdd ? hdd->GetNumSectors() : 0;
}

bool HDC::AttachDevice(u32 channel, u32 number, ATADevice* device)
{
  if (channel >= m_num_channels || number >= DEVICES_PER_CHANNEL || m_channels[channel].devices[number])
    return false;

  m_channels[channel].devices[number] = device;
  return true;
}

void HDC::DetachDevice(u32 channel, u32 number)
{
  if (channel >= m_num_channels || number >= DEVICES_PER_CHANNEL)
    return;

  m_channels[channel].devices[number] = nullptr;
}

void HDC::SetDeviceInterruptLine(u32 channel, u32 number, bool active)
{
  m_channels[channel].device_interrupt_lines[number] = active;
  UpdateHostInterruptLine(channel);
}

void HDC::UpdateHostInterruptLine(u32 channel)
{
  Channel& cdata = m_channels[channel];

  // TODO: Is this correct?
  const bool state =
    (!cdata.control_register.disable_interrupts) && (cdata.device_interrupt_lines[0] | cdata.device_interrupt_lines[1]);
  m_interrupt_controller->SetInterruptState(cdata.irq, state);
}

bool HDC::SupportsDMA() const
{
  return false;
}

bool HDC::IsDMARequested(u32 channel) const
{
  return false;
}

void HDC::SetDMARequest(u32 channel, u32 drive, bool request) {}

u32 HDC::DMATransfer(u32 channel, u32 drive, bool is_write, void* data, u32 size)
{
  return 0;
}

void HDC::ConnectIOPorts(Bus* bus)
{
  for (u32 channel = 0; channel < m_num_channels; channel++)
  {
    u16 BAR0, BAR1;
    u8 irq;
    if (channel == 0)
    {
      // Primary channel
      BAR0 = 0x01F0;
      BAR1 = 0x03F6;
      irq = 14;
    }
    else
    {
      // Secondary channel
      BAR0 = 0x0170;
      BAR1 = 0x0376;
      irq = 15;
    }

    ConnectIOPorts(bus, channel, BAR0, BAR1, irq);
  }
}

void HDC::ConnectIOPorts(Bus* bus, u32 channel, u16 BAR0, u16 BAR1, u8 irq)
{
  // 01F0 - Data register (R/W)
  bus->ConnectIOPortRead(BAR0 + 0, this, std::bind(&HDC::IOReadDataRegisterByte, this, channel, std::placeholders::_2));
  bus->ConnectIOPortReadWord(BAR0 + 0, this,
                             std::bind(&HDC::IOReadDataRegisterWord, this, channel, std::placeholders::_2));
  bus->ConnectIOPortReadDWord(BAR0 + 0, this,
                              std::bind(&HDC::IOReadDataRegisterDWord, this, channel, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 0, this,
                          std::bind(&HDC::IOWriteDataRegisterByte, this, channel, std::placeholders::_2));
  bus->ConnectIOPortWriteWord(BAR0 + 0, this,
                              std::bind(&HDC::IOWriteDataRegisterWord, this, channel, std::placeholders::_2));
  bus->ConnectIOPortWriteDWord(BAR0 + 0, this,
                               std::bind(&HDC::IOWriteDataRegisterDWord, this, channel, std::placeholders::_2));

  // 01F1 - Status register (R)
  // 01F1	w	WPC/4  (Write Precompensation Cylinder divided by 4)
  bus->ConnectIOPortRead(BAR0 + 1, this, std::bind(&HDC::IOReadErrorRegister, this, channel, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 1, this,
                          std::bind(&HDC::IOWriteCommandBlockFeatures, this, channel, std::placeholders::_2));

  // Command block
  // 01F2	r/w	sector count
  // 01F3	r/w	sector number
  // 01F4	r/w	cylinder low
  // 01F5	r/w	cylinder high
  bus->ConnectIOPortRead(BAR0 + 2, this,
                         std::bind(&HDC::IOReadCommandBlockSectorCount, this, channel, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 2, this,
                          std::bind(&HDC::IOWriteCommandBlockSectorCount, this, channel, std::placeholders::_2));
  bus->ConnectIOPortRead(BAR0 + 3, this,
                         std::bind(&HDC::IOReadCommandBlockSectorNumber, this, channel, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 3, this,
                          std::bind(&HDC::IOWriteCommandBlockSectorNumber, this, channel, std::placeholders::_2));
  bus->ConnectIOPortRead(BAR0 + 4, this,
                         std::bind(&HDC::IOReadCommandBlockCylinderLow, this, channel, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 4, this,
                          std::bind(&HDC::IOWriteCommandBlockCylinderLow, this, channel, std::placeholders::_2));
  bus->ConnectIOPortRead(BAR0 + 5, this,
                         std::bind(&HDC::IOReadCommandBlockCylinderHigh, this, channel, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 5, this,
                          std::bind(&HDC::IOWriteCommandBlockCylinderHigh, this, channel, std::placeholders::_2));

  // 01F6: Drive select (R/W)
  bus->ConnectIOPortRead(BAR0 + 6, this,
                         std::bind(&HDC::IOReadDriveSelectRegister, this, channel, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 6, this,
                          std::bind(&HDC::IOWriteDriveSelectRegister, this, channel, std::placeholders::_2));

  // 01F7 - Status register (R) / Command register (W)
  bus->ConnectIOPortRead(BAR0 + 7, this, std::bind(&HDC::IOReadStatusRegister, this, channel, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR0 + 7, this,
                          std::bind(&HDC::IOWriteCommandRegister, this, channel, std::placeholders::_2));

  // 03F7: Alternate status register (R) / Control register (W)
  bus->ConnectIOPortRead(BAR1 + 0, this,
                         std::bind(&HDC::IOReadAltStatusRegister, this, channel, std::placeholders::_2));
  bus->ConnectIOPortWrite(BAR1 + 0, this,
                          std::bind(&HDC::IOWriteControlRegister, this, channel, std::placeholders::_2));

  m_channels[channel].irq = irq;
}

void HDC::DoReset(u32 channel, bool hardware_reset)
{
  Channel& cdata = m_channels[channel];
  for (u32 i = 0; i < DEVICES_PER_CHANNEL; i++)
  {
    if (cdata.devices[i])
      cdata.devices[i]->DoReset(hardware_reset);

    cdata.device_interrupt_lines[i] = false;
  }

  UpdateHostInterruptLine(channel);
}

void HDC::IOReadStatusRegister(u32 channel, u8* value)
{
  // Lower interrupt
  std::fill_n(m_channels[channel].device_interrupt_lines, DEVICES_PER_CHANNEL, false);
  UpdateHostInterruptLine(channel);

  if (m_channels[channel].control_register.software_reset)
  {
    *value = (1 << 7);
    return;
  }

  const ATADevice* device = GetCurrentDevice(channel);
  *value = device ? device->ReadStatusRegister() : 0xFF;
  Log_TracePrintf("ATA read status register %u/%u <- 0x%02X", channel, GetCurrentDeviceIndex(channel),
                  ZeroExtend32(*value));
}

void HDC::IOReadAltStatusRegister(u32 channel, u8* value)
{
  if (m_channels[channel].control_register.software_reset)
  {
    *value = (1 << 7);
    return;
  }

  const ATADevice* device = GetCurrentDevice(channel);
  *value = device ? device->ReadStatusRegister() : 0xFF;
  Log_TracePrintf("ATA read alt status register %u/%u <- 0x%02X", channel, GetCurrentDeviceIndex(channel),
                  ZeroExtend32(*value));
}

void HDC::IOWriteCommandRegister(u32 channel, u8 value)
{
  Log_TracePrintf("ATA write command register %u/%u <- 0x%02X", channel, GetCurrentDeviceIndex(channel),
                  ZeroExtend32(value));

  const u8 index = GetCurrentDeviceIndex(channel);
  if (!m_channels[channel].devices[index])
    return;

  m_channels[channel].device_interrupt_lines[index] = false;
  UpdateHostInterruptLine(channel);

  m_channels[channel].devices[index]->WriteCommandRegister(value);
}

void HDC::IOReadErrorRegister(u32 channel, u8* value)
{
  const ATADevice* device = GetCurrentDevice(channel);
  *value = device ? device->ReadErrorRegister() : 0xFF;
  Log_TracePrintf("ATA read error register %u/%u <- 0x%02X", channel, GetCurrentDeviceIndex(channel),
                  ZeroExtend32(*value));
}

void HDC::IOWriteControlRegister(u32 channel, u8 value)
{
  Log_TracePrintf("ATA write control register %u/%u <- 0x%02X", channel, GetCurrentDeviceIndex(channel),
                  ZeroExtend32(value));

  auto old_value = m_channels[channel].control_register;
  m_channels[channel].control_register.bits = value;

  if (!m_channels[channel].control_register.software_reset && old_value.software_reset)
  {
    // Software reset
    Log_DevPrintf("ATA channel %u software reset", channel);
    DoReset(channel, false);
  }
}

void HDC::IOReadDriveSelectRegister(u32 channel, u8* value)
{
  const ATADevice* device = GetCurrentDevice(channel);
  *value = device ? device->ReadCommandBlockDriveSelect() : m_channels[channel].drive_select_register.bits;
}

void HDC::IOWriteDriveSelectRegister(u32 channel, u8 value)
{
  for (u32 i = 0; i < DEVICES_PER_CHANNEL; i++)
  {
    if (m_channels[channel].devices[i])
      m_channels[channel].devices[i]->WriteCommandBlockDriveSelect(value);
  }

  m_channels[channel].drive_select_register.bits = value;
  Log_TracePrintf("ATA write drive select 0x%02X (drive %u)", value,
                  m_channels[channel].drive_select_register.drive.GetValue());
}

void HDC::IOReadDataRegisterByte(u32 channel, u8* value)
{
  ATADevice* device = GetCurrentDevice(channel);
  if (device)
    device->ReadDataPort(value, sizeof(*value));
  else
    *value = 0xFF;
}

void HDC::IOReadDataRegisterWord(u32 channel, u16* value)
{
  ATADevice* device = GetCurrentDevice(channel);
  if (device)
    device->ReadDataPort(value, sizeof(*value));
  else
    *value = 0xFFFF;
}

void HDC::IOReadDataRegisterDWord(u32 channel, u32* value)
{
  ATADevice* device = GetCurrentDevice(channel);
  if (device)
    device->ReadDataPort(value, sizeof(*value));
  else
    *value = UINT32_C(0xFFFFFFFF);
}

void HDC::IOWriteDataRegisterByte(u32 channel, u8 value)
{
  ATADevice* device = GetCurrentDevice(channel);
  if (device)
    device->WriteDataPort(&value, sizeof(value));
}

void HDC::IOWriteDataRegisterWord(u32 channel, u16 value)
{
  ATADevice* device = GetCurrentDevice(channel);
  if (device)
    device->WriteDataPort(&value, sizeof(value));
}

void HDC::IOWriteDataRegisterDWord(u32 channel, u32 value)
{
  ATADevice* device = GetCurrentDevice(channel);
  if (device)
    device->WriteDataPort(&value, sizeof(value));
}

void HDC::IOReadCommandBlockSectorCount(u32 channel, u8* value)
{
  const ATADevice* device = GetCurrentDevice(channel);
  *value =
    device ? device->ReadCommandBlockSectorCount(m_channels[channel].control_register.high_order_byte_readback) : 0xFF;
  Log_TracePrintf("ATA read sector count %u/%u <- 0x%02X", channel, GetCurrentDeviceIndex(channel),
                  ZeroExtend32(*value));
}

void HDC::IOReadCommandBlockSectorNumber(u32 channel, u8* value)
{
  const ATADevice* device = GetCurrentDevice(channel);
  *value =
    device ? device->ReadCommandBlockSectorNumber(m_channels[channel].control_register.high_order_byte_readback) : 0xFF;
  Log_TracePrintf("ATA read sector number %u/%u <- 0x%02X", channel, GetCurrentDeviceIndex(channel),
                  ZeroExtend32(*value));
}

void HDC::IOReadCommandBlockCylinderLow(u32 channel, u8* value)
{
  const ATADevice* device = GetCurrentDevice(channel);
  *value =
    device ? device->ReadCommandBlockCylinderLow(m_channels[channel].control_register.high_order_byte_readback) : 0xFF;
  Log_TracePrintf("ATA read cylinder low %u/%u <- 0x%02X", channel, GetCurrentDeviceIndex(channel),
                  ZeroExtend32(*value));
}

void HDC::IOReadCommandBlockCylinderHigh(u32 channel, u8* value)
{
  const ATADevice* device = GetCurrentDevice(channel);
  *value =
    device ? device->ReadCommandBlockCylinderHigh(m_channels[channel].control_register.high_order_byte_readback) : 0xFF;
  Log_TracePrintf("ATA read cylinder high %u/%u <- 0x%02X", channel, GetCurrentDeviceIndex(channel),
                  ZeroExtend32(*value));
}

void HDC::IOWriteCommandBlockFeatures(u32 channel, u8 value)
{
  Log_TracePrintf("ATA write command block features %u/%u 0x%02X", channel, GetCurrentDeviceIndex(channel), value);
  for (u32 i = 0; i < DEVICES_PER_CHANNEL; i++)
  {
    if (m_channels[channel].devices[i])
      m_channels[channel].devices[i]->WriteFeatureSelect(value);
  }
}

void HDC::IOWriteCommandBlockSectorCount(u32 channel, u8 value)
{
  Log_TracePrintf("ATA write command block %u/%u sector count 0x%02X", channel, GetCurrentDeviceIndex(channel), value);
  for (u32 i = 0; i < DEVICES_PER_CHANNEL; i++)
  {
    if (m_channels[channel].devices[i])
      m_channels[channel].devices[i]->WriteCommandBlockSectorCount(value);
  }
}

void HDC::IOWriteCommandBlockSectorNumber(u32 channel, u8 value)
{
  Log_TracePrintf("ATA write command block %u/%u sector number 0x%02X", channel, GetCurrentDeviceIndex(channel), value);
  for (u32 i = 0; i < DEVICES_PER_CHANNEL; i++)
  {
    if (m_channels[channel].devices[i])
      m_channels[channel].devices[i]->WriteCommandBlockSectorNumber(value);
  }
}

void HDC::IOWriteCommandBlockCylinderLow(u32 channel, u8 value)
{
  Log_TracePrintf("ATA write command block %u/%u cylinder low 0x%02X", channel, GetCurrentDeviceIndex(channel), value);
  for (u32 i = 0; i < DEVICES_PER_CHANNEL; i++)
  {
    if (m_channels[channel].devices[i])
      m_channels[channel].devices[i]->WriteCommandBlockSectorCylinderLow(value);
  }
}

void HDC::IOWriteCommandBlockCylinderHigh(u32 channel, u8 value)
{
  Log_TracePrintf("ATA write command block %u/%u cylinder high 0x%02X", channel, GetCurrentDeviceIndex(channel), value);
  for (u32 i = 0; i < DEVICES_PER_CHANNEL; i++)
  {
    if (m_channels[channel].devices[i])
      m_channels[channel].devices[i]->WriteCommandBlockSectorCylinderHigh(value);
  }
}

} // namespace HW
