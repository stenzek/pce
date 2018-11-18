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
  static constexpr u32 DEVICES_PER_CHANNEL = 2;
  static constexpr u32 MAX_CHANNELS = 2;

  HDC(const String& identifier, u32 num_channels = 1, const ObjectTypeInfo* type_info = &s_type_info);
  ~HDC();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  bool IsDevicePresent(u32 channel, u32 number) const;
  u32 GetDeviceCount(u32 channel) const;

  // For populating CMOS.
  bool IsHDDPresent(u32 channel, u32 number) const;
  u32 GetHDDCylinders(u32 channel, u32 number) const;
  u32 GetHDDHeads(u32 channel, u32 number) const;
  u32 GetHDDSectors(u32 channel, u32 number) const;

  bool AttachDevice(u32 channel, u32 number, ATADevice* device);
  void DetachDevice(u32 channel, u32 number);

  void SetDeviceInterruptLine(u32 channel, u32 number, bool active);

  // Bus-mastering DMA support.
  virtual bool SupportsDMA() const;
  virtual bool IsDMARequested(u32 channel) const;
  virtual void SetDMARequest(u32 channel, u32 drive, bool request);
  virtual u32 DMATransfer(u32 channel, u32 drive, bool is_write, void* data, u32 size);

protected:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('H', 'D', 'C');

  InterruptController* m_interrupt_controller = nullptr;

  struct Channel
  {
    ATADevice* devices[DEVICES_PER_CHANNEL] = {};
    u8 irq = 0;

    union ATAControlRegister
    {
      u8 bits;
      BitField<u8, bool, 1, 1> disable_interrupts;       // nIEN
      BitField<u8, bool, 2, 1> software_reset;           // SRST
      BitField<u8, bool, 7, 1> high_order_byte_readback; // HOB
    } control_register = {};

    union ATADriveSelectRegister
    {
      u8 bits;
      BitField<u8, u8, 0, 4> head;
      BitField<u8, u8, 4, 1> drive;
      BitField<u8, bool, 6, 1> lba_enable;
    } drive_select_register = {};

    bool device_interrupt_lines[DEVICES_PER_CHANNEL] = {};
  };

  Channel m_channels[MAX_CHANNELS];
  u32 m_num_channels = 1;

  u8 GetCurrentDeviceIndex(u32 channel) const { return m_channels[channel].drive_select_register.drive; }
  ATADevice* GetCurrentDevice(u32 channel) const { return m_channels[channel].devices[GetCurrentDeviceIndex(channel)]; }

  virtual void ConnectIOPorts(Bus* bus);
  virtual void UpdateHostInterruptLine(u32 channel);
  virtual void DoReset(u32 channel, bool hardware_reset);

  void ConnectIOPorts(Bus* bus, u32 channel, u16 BAR0, u16 BAR1, u8 irq);

  void IOReadStatusRegister(u32 channel, u8* value);
  void IOReadAltStatusRegister(u32 channel, u8* value);
  void IOWriteCommandRegister(u32 channel, u8 value);
  void IOReadErrorRegister(u32 channel, u8* value);

  void IOWriteControlRegister(u32 channel, u8 value);
  void IOReadDriveSelectRegister(u32 channel, u8* value);
  void IOWriteDriveSelectRegister(u32 channel, u8 value);

  void IOReadDataRegisterByte(u32 channel, u8* value);
  void IOReadDataRegisterWord(u32 channel, u16* value);
  void IOReadDataRegisterDWord(u32 channel, u32* value);
  void IOWriteDataRegisterByte(u32 channel, u8 value);
  void IOWriteDataRegisterWord(u32 channel, u16 value);
  void IOWriteDataRegisterDWord(u32 channel, u32 value);

  void IOReadCommandBlockSectorCount(u32 channel, u8* value);
  void IOReadCommandBlockSectorNumber(u32 channel, u8* value);
  void IOReadCommandBlockCylinderLow(u32 channel, u8* value);
  void IOReadCommandBlockCylinderHigh(u32 channel, u8* value);

  void IOWriteCommandBlockFeatures(u32 channel, u8 value);
  void IOWriteCommandBlockSectorCount(u32 channel, u8 value);
  void IOWriteCommandBlockSectorNumber(u32 channel, u8 value);
  void IOWriteCommandBlockCylinderLow(u32 channel, u8 value);
  void IOWriteCommandBlockCylinderHigh(u32 channel, u8 value);
};

} // namespace HW
