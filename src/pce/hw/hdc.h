#pragma once

#include "common/bitfield.h"
#include "common/timing.h"
#include "pce/component.h"
#include <YBaseLib/Assert.h>
#include <array>
#include <memory>
#include <string>
#include <vector>

class InterruptController;

namespace HW {

class ATADevice;

class HDC : public Component
{
  DECLARE_OBJECT_TYPE_INFO(HDC, Component);
  DECLARE_OBJECT_NO_FACTORY(HDC);
  DECLARE_OBJECT_PROPERTY_MAP(HDC);

public:
  static const u32 SERIALIZATION_ID = MakeSerializationID('H', 'D', 'C');
  static const u32 NUM_DEVICES = 2;

  // TODO: Flag class
  enum class Channel : u32
  {
    Primary,
    Secondary
  };

  HDC(const String& identifier, Channel channel = Channel::Primary, const ObjectTypeInfo* type_info = &s_type_info);
  ~HDC();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  bool IsDevicePresent(u32 number) const { return (number < NUM_DEVICES && m_devices[number]); }
  u32 GetDeviceCount() const;

  // For populating CMOS.
  bool IsHDDPresent(u32 number) const;
  u32 GetHDDCylinders(u32 number) const;
  u32 GetHDDHeads(u32 number) const;
  u32 GetHDDSectors(u32 number) const;

  bool AttachDevice(u32 number, ATADevice* device);
  void DetachDevice(u32 number);

  void SetDeviceInterruptLine(u32 number, bool active);
  void UpdateHostInterruptLine();

protected:
  InterruptController* m_interrupt_controller = nullptr;
  ATADevice* m_devices[NUM_DEVICES] = {};
  Channel m_channel;
  uint32 m_irq_number = 0;

  union ATAControlRegister
  {
    u8 bits;
    BitField<u8, bool, 1, 1> disable_interrupts;       // nIEN
    BitField<u8, bool, 2, 1> software_reset;           // SRST
    BitField<u8, bool, 7, 1> high_order_byte_readback; // HOB
  } m_control_register = {};

  union ATADriveSelectRegister
  {
    u8 bits;
    BitField<u8, u8, 0, 4> head;
    BitField<u8, u8, 4, 1> drive;
    BitField<u8, bool, 6, 1> lba_enable;
  } m_drive_select_register = {};

  bool m_device_interrupt_lines[NUM_DEVICES] = {};

  u8 GetCurrentDeviceIndex() const { return m_drive_select_register.drive; }
  ATADevice* GetCurrentDevice() const { return m_devices[GetCurrentDeviceIndex()]; }

  void ConnectIOPorts(Bus* bus);

  void SoftReset();

  void IOReadStatusRegister(u8* value);
  void IOReadAltStatusRegister(u8* value);
  void IOWriteCommandRegister(u8 value);
  void IOReadErrorRegister(u8* value);

  void IOWriteControlRegister(u8 value);
  void IOReadDriveSelectRegister(u8* value);
  void IOWriteDriveSelectRegister(u8 value);

  void IOReadDataRegisterByte(u8* value);
  void IOReadDataRegisterWord(u16* value);
  void IOReadDataRegisterDWord(u32* value);
  void IOWriteDataRegisterByte(u8 value);
  void IOWriteDataRegisterWord(u16 value);
  void IOWriteDataRegisterDWord(u32 value);

  void IOReadCommandBlockSectorCount(u8* value);
  void IOReadCommandBlockSectorNumber(u8* value);
  void IOReadCommandBlockCylinderLow(u8* value);
  void IOReadCommandBlockCylinderHigh(u8* value);

  void IOWriteCommandBlockFeatures(u8 value);
  void IOWriteCommandBlockSectorCount(u8 value);
  void IOWriteCommandBlockSectorNumber(u8 value);
  void IOWriteCommandBlockCylinderLow(u8 value);
  void IOWriteCommandBlockCylinderHigh(u8 value);
};

} // namespace HW
