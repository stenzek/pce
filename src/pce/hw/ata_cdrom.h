#pragma once
#include "ata_device.h"
#include "cdrom.h"
#include <memory>

class TimingEvent;

namespace HW {

class ATACDROM final : public ATADevice
{
  DECLARE_OBJECT_TYPE_INFO(ATACDROM, ATADevice);
  DECLARE_GENERIC_COMPONENT_FACTORY(ATACDROM);
  DECLARE_OBJECT_PROPERTY_MAP(ATACDROM);

public:
  ATACDROM(const String& identifier, u32 ata_channel = 0, u32 ata_device = 0,
           const ObjectTypeInfo* type_info = &s_type_info);

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;

  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  void WriteCommandRegister(u8 value) override;

protected:
  void DoReset(bool is_hardware_reset) override;
  void OnBufferEnd() override;

private:
  static constexpr u16 INVALID_COMMAND = 0x100;

  void SetSignature();
  void SetInterruptReason(bool is_command, bool data_from_device, bool release);

  void CompleteCommand(bool raise_interrupt);
  void AbortCommand(u8 error = ATA_ERR_ABRT);

  CycleCount CalculateCommandTime(u8 command) const;
  bool HasPendingCommand() const;
  void ExecutePendingCommand();

  void HandleATADeviceReset();
  void HandleATAIdentify();

  void HandleATAPIIdentify();
  void HandleATAPIPacket();

  void InterruptCallback();

  CDROM m_cdrom;

  std::unique_ptr<TimingEvent> m_command_event;

  // Size of all ATAPI packets.
  u32 m_packet_size = 12;

  // Current command being executed.
  u16 m_current_command = INVALID_COMMAND;
};

} // namespace HW
