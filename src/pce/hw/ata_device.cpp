#include "ata_device.h"

namespace HW {

ATADevice::ATADevice(const ObjectTypeInfo* type_info /*= &s_type_info*/) {}

ATADevice::~ATADevice() {}

bool ATADevice::Initialize(System* system, Bus* bus) {}

void ATADevice::Reset()
{
  BaseClass::Reset();
  DoReset(true);
}

bool ATADevice::LoadState(BinaryReader& reader)
{
}

bool ATADevice::SaveState(BinaryWriter& writer)
{
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

void ATADevice::DoReset(bool is_hardware_reset)
{

}

}