#pragma once
#include "ata_device.h"
#include "common/timing.h"

class HDDImage;

namespace HW {

// TODO: Move common stuff to "base HDD" class, like CDROM.

class ATAHDD final : public ATADevice
{
  DECLARE_OBJECT_TYPE_INFO(ATAHDD, ATADevice);
  DECLARE_GENERIC_COMPONENT_FACTORY(ATAHDD);
  DECLARE_OBJECT_PROPERTY_MAP(ATAHDD);

public:
  ATAHDD(const String& identifier, const char* image_filename = "", u32 cylinders = 0, u32 heads = 0, u32 sectors = 0,
         u32 ide_channel = 0, u32 ide_device = 0, const ObjectTypeInfo* type_info = &s_type_info);

  // Pass in 0/0/0 for complete autodetection, otherwise heads/sectors are used to compute cylinders.
  static bool ComputeCHSGeometry(const u64 size, u32& cylinders, u32& heads, u32& sectors_per_track);

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;

  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  void WriteCommandRegister(u8 value) override;

  HDDImage* GetImage() const { return m_image.get(); }

  u64 GetNumLBAs() const { return m_lbas; }
  u32 GetNumCylinders() const { return m_cylinders; }
  u32 GetNumHeads() const { return m_heads; }
  u32 GetNumSectors() const { return m_sectors_per_track; }

protected:
  void DoReset(bool is_hardware_reset) override;

private:
  static const u32 SERIALIZATION_ID = MakeSerializationID('A', 'T', 'A', 'H');
  static const u32 SECTOR_SIZE = 512;
  static const u16 INVALID_COMMAND = 0x100;

  void SetSignature();
  void ClearActivity();
  void FlushImage();

  void TranslateLBAToCHS(const u64 lba, u32* cylinder, u32* head, u32* sector) const;
  u64 TranslateCHSToLBA(const u32 cylinder, const u32 head, const u32 sector) const;

  bool SeekCHS(const u32 cylinder, const u32 head, const u32 sector);
  bool SeekLBA(const u64 lba);

  void CompleteCommand(bool seek_complete = false, bool raise_interrupt = true);
  void AbortCommand(ATA_ERR error = ATA_ERR_ABRT, bool device_fault = false);

  void SetupTransfer(u32 num_sectors, u32 block_size, bool is_write);
  void SetupReadWriteEvent(CycleCount seek_time, u32 num_sectors);
  void FillReadBuffer();
  void FlushWriteBuffer();
  void OnBufferEnd() override;
  void ExecutePendingReadWrite();
  void OnReadWriteEnd();

  CycleCount CalculateCommandTime(u8 command) const;
  CycleCount CalculateSeekTime(u64 from_lba, u64 to_lba) const;
  CycleCount CalculateReadWriteTime(u32 num_sectors) const;
  bool IsWriteCommand(u8 command) const;
  bool HasPendingCommand() const;
  void ExecutePendingCommand();

  void HandleATADeviceReset();
  void HandleATAIdentify();
  void HandleATARecalibrate();
  void HandleATATransferPIO(bool write, bool extended, bool multiple);
  void HandleATAReadVerifySectors(bool extended, bool with_retry);
  void HandleATASetMultipleMode();
  void HandleATAExecuteDriveDiagnostic();
  void HandleATAInitializeDriveParameters();
  void HandleATASetFeatures();

  String m_image_filename;
  std::unique_ptr<HDDImage> m_image;

  TimingEvent::Pointer m_flush_event;
  TimingEvent::Pointer m_command_event;
  TimingEvent::Pointer m_read_write_event;

  u64 m_lbas;
  u32 m_cylinders;
  u32 m_heads;
  u32 m_sectors_per_track;

  u32 m_ata_channel_number;
  u32 m_ata_drive_number;

  // parameters in current translation mode
  u32 m_current_num_cylinders = 0;
  u32 m_current_num_heads = 0;
  u32 m_current_num_sectors_per_track = 0;
  u32 m_multiple_sectors = 0;

  // current seek position
  // sector is one-based as per convention
  u64 m_current_lba = 0;

  // Current command being executed.
  u16 m_current_command = INVALID_COMMAND;

  // Current buffer being transferred.
  u32 m_transfer_remaining_sectors = 0;
  u32 m_transfer_block_size = 0;
};

} // namespace HW
