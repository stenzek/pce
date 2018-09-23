#include "ata_device.h"
#include "../system.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "hdc.h"
Log_SetChannel(HW::ATADevice);

namespace HW {

DEFINE_OBJECT_TYPE_INFO(ATADevice);
BEGIN_OBJECT_PROPERTY_MAP(ATADevice)
PROPERTY_TABLE_MEMBER_UINT("Channel", 0, offsetof(ATADevice, m_ata_channel_number), nullptr, 0)
PROPERTY_TABLE_MEMBER_UINT("Drive", 0, offsetof(ATADevice, m_ata_drive_number), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

ATADevice::ATADevice(const String& identifier, u32 ata_channel, u32 ata_drive,
                     const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_ata_channel_number(ata_channel), m_ata_drive_number(ata_drive)
{
}

ATADevice::~ATADevice() {}

bool ATADevice::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_ata_controller = system->GetComponentByType<HDC>(m_ata_channel_number % 2);
  if (!m_ata_controller)
  {
    Log_ErrorPrintf("Failed to find IDE controller");
    return false;
  }
  else if (m_ata_controller->IsDevicePresent(m_ata_channel_number, m_ata_drive_number))
  {
    Log_ErrorPrintf("ATA channel %u already has a drive %u attached", m_ata_channel_number, m_ata_drive_number);
    return false;
  }

  if (!m_ata_controller->AttachDevice(m_ata_channel_number, m_ata_drive_number, this))
  {
    Log_ErrorPrintf("Failed to attach device to ATA channel %u device %u", m_ata_channel_number, m_ata_drive_number);
    return false;
  }

  return true;
}

void ATADevice::Reset()
{
  BaseClass::Reset();
  DoReset(true);
}

bool ATADevice::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  if (reader.ReadUInt32() != m_ata_channel_number || reader.ReadUInt32() != m_ata_drive_number)
  {
    Log_ErrorPrintf("Save state channel/drive mismatch");
    return false;
  }

  m_registers.status.bits = reader.ReadUInt8();
  m_registers.drive_select.bits = reader.ReadUInt8();
  m_registers.error = reader.ReadUInt8();
  m_registers.feature_select = reader.ReadUInt8();
  m_registers.sector_count = reader.ReadUInt16();
  m_registers.sector_number = reader.ReadUInt16();
  m_registers.cylinder_low = reader.ReadUInt16();
  m_registers.cylinder_high = reader.ReadUInt16();

  m_buffer.size = reader.ReadUInt32();
  m_buffer.position = reader.ReadUInt32();
  m_buffer.is_write = reader.ReadBool();
  m_buffer.valid = reader.ReadBool();
  if (m_buffer.size > 0)
  {
    m_buffer.data.resize(m_buffer.size);
    reader.ReadBytes(m_buffer.data.data(), m_buffer.size);
  }

  return !reader.GetErrorState();
}

bool ATADevice::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);
  writer.WriteUInt32(m_ata_channel_number);
  writer.WriteUInt32(m_ata_drive_number);

  writer.WriteUInt8(m_registers.status.bits);
  writer.WriteUInt8(m_registers.drive_select.bits);
  writer.WriteUInt8(m_registers.error);
  writer.WriteUInt8(m_registers.feature_select);
  writer.WriteUInt16(m_registers.sector_count);
  writer.WriteUInt16(m_registers.sector_number);
  writer.WriteUInt16(m_registers.cylinder_low);
  writer.WriteUInt16(m_registers.cylinder_high);

  writer.WriteUInt32(m_buffer.size);
  writer.WriteUInt32(m_buffer.position);
  writer.WriteBool(m_buffer.is_write);
  writer.WriteBool(m_buffer.valid);
  if (m_buffer.size > 0)
    writer.WriteBytes(m_buffer.data.data(), m_buffer.size);

  return !writer.InErrorState();
}

u8 ATADevice::ReadCommandBlockSectorCount(bool hob) const
{
  return Truncate8(hob ? (m_registers.sector_count >> 8) : (m_registers.sector_count));
}

u8 ATADevice::ReadCommandBlockSectorNumber(bool hob) const
{
  return Truncate8(hob ? (m_registers.sector_number >> 8) : (m_registers.sector_number));
}

u8 ATADevice::ReadCommandBlockCylinderLow(bool hob) const
{
  return Truncate8(hob ? (m_registers.cylinder_low >> 8) : (m_registers.cylinder_low));
}

u8 ATADevice::ReadCommandBlockCylinderHigh(bool hob) const
{
  return Truncate8(hob ? (m_registers.cylinder_high >> 8) : (m_registers.cylinder_high));
}

u8 ATADevice::ReadCommandBlockDriveSelect() const
{
  return m_registers.drive_select.bits;
}

void ATADevice::WriteFeatureSelect(u8 value)
{
  m_registers.feature_select = value;
}

void ATADevice::WriteCommandBlockSectorCount(u8 value)
{
  m_registers.sector_count <<= 8;
  m_registers.sector_count |= ZeroExtend16(value);
}

void ATADevice::WriteCommandBlockSectorNumber(u8 value)
{
  m_registers.sector_number <<= 8;
  m_registers.sector_number |= ZeroExtend16(value);
}

void ATADevice::WriteCommandBlockSectorCylinderLow(u8 value)
{
  m_registers.cylinder_low <<= 8;
  m_registers.cylinder_low |= ZeroExtend16(value);
}

void ATADevice::WriteCommandBlockSectorCylinderHigh(u8 value)
{
  m_registers.cylinder_high <<= 8;
  m_registers.cylinder_high |= ZeroExtend16(value);
}

void ATADevice::WriteCommandBlockDriveSelect(u8 value)
{
  m_registers.drive_select.bits = value;
}

void ATADevice::DoReset(bool is_hardware_reset)
{
  // The 430FX bios seems to require that the error register be 1 after soft reset.
  // The IDE spec agrees that the initial value is 1.
  m_registers.status.Reset();
  m_registers.error = 1;
  m_ata_controller->SetDeviceInterruptLine(m_ata_channel_number, m_ata_drive_number, false);
  m_dmack = false;
  ResetBuffer();
}

void ATADevice::PutIdentifyString(char* buffer, uint32 buffer_size, const char* str)
{
  Assert(buffer_size <= 40);
  char temp_buffer[40 + 1];
  size_t len = std::strlen(str);
  Y_strncpy(temp_buffer, countof(temp_buffer), str);
  for (size_t i = len; i < countof(temp_buffer); i++)
    temp_buffer[i] = ' ';

  u16* word_buffer = reinterpret_cast<u16*>(buffer);
  for (u32 i = 0; i < (buffer_size / sizeof(u16)); i++)
    word_buffer[i] = ZeroExtend16(temp_buffer[i * 2] << 8) | ZeroExtend16(temp_buffer[i * 2 + 1]);
}

void ATADevice::RaiseInterrupt()
{
  Log_TracePrintf("Raising ATA interrupt line %u/%u", m_ata_channel_number, m_ata_drive_number);
  m_ata_controller->SetDeviceInterruptLine(m_ata_channel_number, m_ata_drive_number, true);
}

void ATADevice::ReadDataPort(void* buffer, u32 size)
{
  if (!m_buffer.valid || m_buffer.is_write || m_buffer.is_dma)
    return;

  const u32 bytes_to_copy = std::min(size, m_buffer.size - m_buffer.position);
  std::memcpy(buffer, &m_buffer.data[m_buffer.position], bytes_to_copy);
  m_buffer.position += bytes_to_copy;

  if (m_buffer.position == m_buffer.size)
    OnBufferEnd();
}

void ATADevice::WriteDataPort(const void* buffer, u32 size)
{
  if (!m_buffer.valid || !m_buffer.is_write || m_buffer.is_dma)
    return;

  const u32 bytes_to_copy = std::min(size, m_buffer.size - m_buffer.position);
  std::memcpy(&m_buffer.data[m_buffer.position], buffer, bytes_to_copy);
  m_buffer.position += bytes_to_copy;

  if (m_buffer.position == m_buffer.size)
    OnBufferEnd();
}

void ATADevice::SetupBuffer(u32 size, bool is_write, bool dma)
{
  m_buffer.size = size;
  m_buffer.position = 0;
  if (m_buffer.data.size() < m_buffer.size)
    m_buffer.data.resize(m_buffer.size);
  m_buffer.is_write = is_write;
  m_buffer.is_dma = dma;
}

void ATADevice::ResetBuffer()
{
  if (m_buffer.is_dma)
  {
    m_ata_controller->SetDMARequest(m_ata_channel_number, m_ata_drive_number, false);
    m_buffer.is_dma = false;
  }

  m_buffer.size = 0;
  m_buffer.position = 0;
  m_buffer.is_write = false;
  m_buffer.valid = false;
}

void ATADevice::BufferReady(bool raise_interrupt)
{
  m_buffer.valid = true;
  if (!m_buffer.is_dma)
  {
    m_registers.status.SetDRQ();
    if (raise_interrupt)
      RaiseInterrupt();
  }
  else
  {
    if (m_dmack)
      DoDMATransfer();
    else
      m_ata_controller->SetDMARequest(m_ata_channel_number, m_ata_drive_number, true);
  }
}

void ATADevice::DoDMATransfer()
{
  if (!m_buffer.valid)
    return;

  // If this is a read, we need to copy from the buffer to memory.
  // If it's a write, we copy from memory to the buffer.
  // This is why we invert the is_write, because the bus master's point of view is reversed.
  const u32 remaining = m_buffer.size - m_buffer.position;
  const u32 written = m_ata_controller->DMATransfer(m_ata_channel_number, m_ata_drive_number, !m_buffer.is_write,
                                                    m_buffer.data.data(), remaining);
  m_buffer.position += written;
  if (m_buffer.position >= m_buffer.size)
    OnBufferEnd();
}

bool ATADevice::SupportsDMA() const
{
  return false;
}

void ATADevice::SetDMACK(bool active)
{
  Log_DevPrintf("ATA device %u/%u DMACK=%s", m_ata_channel_number, m_ata_drive_number, active ? "active" : "inactive");
  m_dmack = active;
  if (active)
  {
    // Transfer any bytes which we have available.
    DoDMATransfer();
  }
}

} // namespace HW