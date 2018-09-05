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

class IDEHDD;
class CDROM;

class HDC : public Component
{
  DECLARE_OBJECT_TYPE_INFO(HDC, Component);
  DECLARE_OBJECT_NO_FACTORY(HDC);
  DECLARE_OBJECT_PROPERTY_MAP(HDC);

public:
  static const uint32 SERIALIZATION_ID = MakeSerializationID('H', 'D', 'C');
  static const uint32 SECTOR_SIZE = 512;
  static const uint32 MAX_DRIVES = 2;

  // TODO: Flag class
  enum CHANNEL : u32
  {
    CHANNEL_PRIMARY,
    CHANNEL_SECONDARY,
    ATA_CHANNELS
  };
  enum DRIVE_TYPE
  {
    DRIVE_TYPE_NONE,
    DRIVE_TYPE_HDD,
    DRIVE_TYPE_ATAPI,
    NUM_DRIVE_TYPES
  };
  enum ATA_SR
  {
    ATA_SR_BSY = 0x80,  // Busy
    ATA_SR_DRDY = 0x40, // Device ready
    ATA_SR_DF = 0x20,   // Device write fault
    ATA_SR_DSC = 0x10,  // Drive seek complete
    ATA_SR_DRQ = 0x08,  // Data request ready
    ATA_SR_CORR = 0x04, // Corrected data
    ATA_SR_IDX = 0x02,  // Inlex
    ATA_SR_ERR = 0x01   // Error
  };
  enum ATA_ERR
  {
    ATA_ERR_BBK = 0x80,   // Bad sector
    ATA_ERR_UNC = 0x40,   // Uncorrectable data
    ATA_ERR_MC = 0x20,    // No media
    ATA_ERR_IDNF = 0x10,  // ID mark not found
    ATA_ERR_MCR = 0x08,   // No media
    ATA_ERR_ABRT = 0x04,  // Command aborted
    ATA_ERR_TK0NF = 0x02, // Track 0 not found
    ATA_ERR_AMNF = 0x01   // No address mark
  };
  enum ATA_CMD
  {
    ATA_CMD_RECALIBRATE = 0x10,
    ATA_CMD_READ_PIO = 0x20,
    ATA_CMD_READ_PIO_NO_RETRY = 0x21,
    ATA_CMD_READ_MULTIPLE_PIO = 0xC4,
    ATA_CMD_READ_PIO_EXT = 0x24,
    ATA_CMD_READ_DMA = 0xC8,
    ATA_CMD_READ_DMA_EXT = 0x25,
    ATA_CMD_READ_VERIFY = 0x40,
    ATA_CMD_READ_VERIFY_EXT = 0x41,
    ATA_CMD_WRITE_PIO = 0x30,
    ATA_CMD_WRITE_PIO_NO_RETRY = 0x31,
    ATA_CMD_WRITE_MULTIPLE_PIO = 0xC5,
    ATA_CMD_WRITE_PIO_EXT = 0x34,
    ATA_CMD_WRITE_DMA = 0xCA,
    ATA_CMD_WRITE_DMA_EXT = 0x35,
    ATA_CMD_CACHE_FLUSH = 0xE7,
    ATA_CMD_CACHE_FLUSH_EXT = 0xEA,
    ATA_CMD_IDENTIFY_PACKET = 0xA1,
    ATA_CMD_IDENTIFY = 0xEC,
    ATA_CMD_SET_MULTIPLE_MODE = 0xC6,
    ATA_CMD_EXECUTE_DRIVE_DIAGNOSTIC = 0x90,
    ATA_CMD_INITIALIZE_DRIVE_PARAMETERS = 0x91,
    ATA_CMD_SET_FEATURES = 0xEF,
    ATA_CMD_DEVICE_RESET = 0x08,

    ATAPI_CMD_READ = 0xA8,
    ATAPI_CMD_EJECT = 0x1B,
    ATAPI_CMD_PACKET = 0xA0
  };

  static void CalculateCHSForSize(uint32* cylinders, uint32* heads, uint32* sectors, uint64 disk_size);

  HDC(const String& identifier, CHANNEL channel = CHANNEL_PRIMARY, const ObjectTypeInfo* type_info = &s_type_info);
  ~HDC();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  bool IsDrivePresent(uint32 number) const;
  uint32 GetDriveCount() const;

  uint32 GetDriveCylinders(uint32 number) const;
  uint32 GetDriveHeads(uint32 number) const;
  uint32 GetDriveSectors(uint32 number) const;
  uint64 GetDriveLBAs(uint32 number) const;

  bool AttachHDD(uint32 number, IDEHDD* dev);
  bool AttachATAPIDevice(uint32 number, CDROM* atapi_dev);
  void DetachDrive(uint32 number);

  // For HLE bios
  bool SeekDrive(uint32 drive, uint64 lba);
  bool SeekDrive(uint32 drive, uint32 cylinder, uint32 head, uint32 sector);
  bool SeekToNextSector(uint32 drive);
  void ReadCurrentSector(uint32 drive, void* data);
  void WriteCurrentSector(uint32 drive, const void* data);

protected:
  InterruptController* m_interrupt_controller = nullptr;
  CHANNEL m_channel = CHANNEL_PRIMARY;
  uint32 m_irq_number = 0;

  // TODO: Make this a fixed array, rather than pointer.
  struct DriveState
  {
    DRIVE_TYPE type = DRIVE_TYPE_NONE;

    // parameters in current translation mode
    uint32 current_num_cylinders = 0;
    uint32 current_num_heads = 0;
    uint32 current_num_sectors = 0;

    // current seek position
    // sector is one-based as per convention
    uint32 current_cylinder = 0;
    uint32 current_head = 0;
    uint32 current_sector = 1;
    uint64 current_lba = 0;

    // visible to the guest
    // TODO: These should be shared between drives.
    uint16 ata_sector_count = 0;
    uint16 ata_sector_number = 0;
    uint16 ata_cylinder_low = 0;
    uint16 ata_cylinder_high = 0;
    uint16 multiple_sectors = 0;

    // TODO: Replace with file IO
    IDEHDD* hdd = nullptr;
    CDROM* atapi_dev = nullptr;

    void SetATAPIInterruptReason(bool is_command, bool data_from_device, bool release);
  };
  std::array<DriveState, MAX_DRIVES> m_drives;
  TimingEvent::Pointer m_image_flush_event;

  void ConnectIOPorts(Bus* bus);
  void SetDriveActivity(u32 drive, bool writing);
  void ClearDriveActivity();
  void SoftReset();
  void FlushImagesEvent();

  uint8 GetCurrentDriveIndex() const;
  DriveState* GetCurrentDrive();
  CDROM* GetCurrentATAPIDevice();
  void SetSignature(DriveState* drive);

  union
  {
    BitField<u8, bool, 7, 1> busy;
    BitField<u8, bool, 6, 1> ready;
    BitField<u8, bool, 5, 1> write_fault;
    BitField<u8, bool, 4, 1> seek_complete;
    BitField<u8, bool, 3, 1> data_request_ready;
    BitField<u8, bool, 2, 1> corrected_data;
    BitField<u8, bool, 1, 1> index;
    BitField<u8, bool, 0, 1> error;
    uint8 bits;

    // TODO: We can optimize these to manipulate bits directly.

    bool IsAcceptingData() const { return data_request_ready; }

    bool IsReady() const { return ready; }

    void ClearError()
    {
      error = false;
      write_fault = false;
    }

    void SetReady()
    {
      busy = false;
      ready = true;
      seek_complete = true;
      data_request_ready = false;
      write_fault = false;
      error = false;
    }

    void SetError(bool write_fault_ = false)
    {
      busy = false;
      ready = true;
      seek_complete = false;
      data_request_ready = false;
      write_fault = write_fault;
      error = true;
    }

    void SetBusy()
    {
      busy = true;
      ready = false;
      data_request_ready = false;
      error = false;
      write_fault = false;
    }

    void SetDRQ()
    {
      busy = false;
      ready = true;
      data_request_ready = true;
      write_fault = write_fault;
      error = error;
    }

    void Reset()
    {
      bits = 0;
      ready = true;
      seek_complete = true;
    }

  } m_status_register;
  void IOReadStatusRegister(uint8* value);
  void IOReadAltStatusRegister(uint8* value);
  void IOWriteCommandRegister(uint8 value);

  uint8 m_error_register = 0;
  void IOReadErrorRegister(uint8* value);

  union
  {
    uint8 bits = 0;
    BitField<uint8, bool, 1, 1> disable_interrupts;       // nIEN
    BitField<uint8, bool, 2, 1> software_reset;           // SRST
    BitField<uint8, bool, 7, 1> high_order_byte_readback; // HOB
  } m_control_register;
  void IOWriteControlRegister(uint8 value);

  void IOReadDataRegisterByte(uint8* value);
  void IOReadDataRegisterWord(uint16* value);
  void IOReadDataRegisterDWord(uint32* value);
  void IOWriteDataRegisterByte(uint8 value);
  void IOWriteDataRegisterWord(uint16 value);
  void IOWriteDataRegisterDWord(uint32 value);

  union
  {
    uint8 bits = 0;
    BitField<uint8, uint8, 0, 4> head;
    BitField<uint8, uint8, 4, 1> drive;
    BitField<uint8, bool, 6, 1> lba_enable;
  } m_drive_select;

  uint8 m_feature_select = 0;

  TimingEvent::Pointer m_command_event;
  uint8 m_pending_command = 0;

  // Also: LBA0 = Sector, LBA1 = Cylinder low, LBA2 = Cylinder high, head in DS
  // TODO: Byte read/writes need to use a flip-flop in LBA48 mode
  void IOReadCommandBlock(uint32 port, uint8* value);
  void IOWriteCommandBlock(uint32 port, uint8 value);
  void IOReadDriveSelect(uint8* value);
  void IOWriteDriveSelect(uint8 value);

  bool FillReadBuffer(uint32 drive_index, uint32 sector_count);
  void PrepareWriteBuffer(uint32 drive_index, uint32 sector_count);
  bool FlushWriteBuffer(uint32 drive_index, uint32 sector_count);

  CycleCount CalculateCommandTime(u8 command) const;
  bool IsWriteCommand(u8 command) const;
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

  void HandleATAPIIdentify();
  void HandleATAPIPacket();
  void HandleATAPICommandCompleted(uint32 drive_index);

  void AbortCommand(uint8 error = ATA_ERR_ABRT, bool write_fault = false);
  void CompleteCommand();

  void BeginTransfer(uint32 drive_index, uint32 sectors_per_block, uint32 num_sectors, bool is_write);
  void UpdatePacketCommand(const void* data, size_t data_size);
  void UpdateTransferBuffer();
  void DataRequestReady();

  void RaiseInterrupt();

  void StopTransfer();

  // read/write buffer before it is sent to the backing store
  struct
  {
    std::vector<uint8> buffer;
    size_t buffer_position = 0;
    uint32 drive_index = 0;
    uint32 sectors_per_block = 0;
    uint32 remaining_sectors = 0;
    bool is_write = false;
    bool is_packet_command = false;
    bool is_packet_data = false;
  } m_current_transfer;
};

} // namespace HW
