#pragma once

#include "YBaseLib/PODArray.h"
#include "common/bitfield.h"
#include "common/clock.h"
#include "pce/component.h"
#include "pce/dma_controller.h"
#include <array>

class ByteStream;

namespace HW {

class FDC : public Component
{
  DECLARE_OBJECT_TYPE_INFO(FDC, Component);
  DECLARE_OBJECT_NO_FACTORY(FDC);
  DECLARE_OBJECT_PROPERTY_MAP(FDC);

public:
  enum DriveType : uint8
  {
    DriveType_None,
    DriveType_5_25,
    DriveType_3_5,
    DriveType_Count
  };

  enum DiskType : uint8
  {
    DiskType_None,

    DiskType_160K,  // 5.25-inch, single-sided, 40 tracks, 8 sectors per track
    DiskType_180K,  // 5.25-inch, single-sided, 40 tracks, 9 sectors per track
    DiskType_320K,  // 5.25-inch, double-sided, 40 tracks, 8 sectors per track
    DiskType_360K,  // 5.25-inch, double-sided, 40 tracks, 9 sectors per track
    DiskType_640K,  // 5.25-inch, double-sided, 80 tracks, 8 sectors per track
    DiskType_1220K, // 5.25-inch, double-sided, 80 tracks, 15 sectors per track
    DiskType_720K,  // 3.5-inch, double-sided, 80 tracks, 9 sectors per track
    DiskType_1440K, // 3.5-inch, double-sided, 80 tracks, 18 sectors per track
    DiskType_1680K, // 3.5-inch, double-sided, 80 tracks, 21 sectors per track, "DMF"
    DiskType_2880K, // 3.5-inch, double-sided, 80 tracks, 36 sectors per track

    DiskType_AutoDetect
  };

  enum Model : uint8
  {
    Model_8272,
    Model_82072,
    Model_82077,
  };

  static const uint32 SERIALIZATION_ID = MakeSerializationID('F', 'D', 'C');
  static const uint32 SECTOR_SIZE = 512;
  static const uint32 MAX_DRIVES = 4;

  // We use 1MHz as the clock frequency for the FDC, so we can specify "cycles" as microseconds.
  static constexpr float CLOCK_FREQUENCY = 1000000;

  static DiskType DetectDiskType(ByteStream* pStream);
  static DriveType GetDriveTypeForDiskType(DiskType type);

public:
  FDC(Model model, DMAController* dma);
  ~FDC();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  // Renamed due to winapi conflicts
  DriveType GetDriveType_(uint32 drive) { return m_drives[drive].drive_type; }
  DiskType GetDiskType(uint32 drive) { return m_drives[drive].disk_type; }

  bool IsDrivePresent(uint32 drive) const
  {
    return (drive < MAX_DRIVES && m_drives[drive].drive_type != DriveType_None);
  }
  bool IsDiskPresent(uint32 drive) const { return (drive < MAX_DRIVES && m_drives[drive].disk_type != DiskType_None); }
  void SetDriveType(uint32 drive, DriveType type);
  uint32 GetDriveCount() const;

  uint32 GetDiskSize(uint32 drive) { return static_cast<uint32>(m_drives[drive].data.GetSize()); }
  uint32 GetDiskCylinders(uint32 drive) { return m_drives[drive].num_cylinders; }
  uint32 GetDiskHeads(uint32 drive) { return m_drives[drive].num_heads; }
  uint32 GetDiskSectors(uint32 drive) { return m_drives[drive].num_sectors; }
  uint32 GetDriveCurrentCylinder(uint32 drive) const { return m_drives[drive].current_cylinder; }
  uint32 GetDriveCurrentHead(uint32 drive) const { return m_drives[drive].current_head; }
  uint32 GetDriveCurrentSector(uint32 drive) const { return m_drives[drive].current_sector; }
  uint32 GetDriveCurrentLBA(uint32 drive) const { return m_drives[drive].current_sector; }

  bool InsertDisk(uint32 drive, DiskType type, ByteStream* pStream);

  // For HLE bios
  bool SeekDrive(uint32 drive, uint32 cylinder, uint32 head, uint32 sector);
  bool SeekToNextSector(uint32 drive);
  void ReadCurrentSector(uint32 drive, void* data);
  void WriteCurrentSector(uint32 drive, const void* data);
  bool ReadSector(uint32 drive, uint32 cylinder, uint32 head, uint32 sector, void* data);
  bool WriteSector(uint32 drive, uint32 cylinder, uint32 head, uint32 sector, const void* data);

protected:
  static constexpr uint32 IRQ_NUMBER = 6;
  static constexpr uint32 DMA_CHANNEL = 2;
  static constexpr uint32 MAX_COMMAND_LENGTH = 9;

  System* m_system = nullptr;
  DMAController* m_dma = nullptr;
  Clock m_clock;
  Model m_model;

  struct DriveState
  {
    DriveType drive_type = DriveType_None;
    DiskType disk_type = DiskType_None;

    uint32 num_cylinders = 0;
    uint32 num_heads = 0;
    uint32 num_sectors = 0;

    uint32 current_cylinder = 0;
    uint32 current_head = 0;
    uint32 current_sector = 0;
    uint32 current_lba = 0;

    PODArray<uint8> data;
    bool write_protect = false;

    bool data_was_read = false;
    bool data_was_written = false;
    bool step_latch = false;

    // Last seek resulted in a forward (greater) track number
    bool direction = false;
  };
  std::array<DriveState, MAX_DRIVES> m_drives;

  static const uint32 FIFO_SIZE = 16;

  // 03F2h: Digital Output Register
  union
  {
    uint8 bits = 0;

    BitField<uint8, uint8, 0, 2> drive_select;
    BitField<uint8, bool, 2, 1> nreset;
    BitField<uint8, bool, 3, 1> ndmagate;
    BitField<uint8, bool, 4, 1> drive_0_motor_on;
    BitField<uint8, bool, 5, 1> drive_1_motor_on;
    BitField<uint8, bool, 6, 1> drive_2_motor_on;
    BitField<uint8, bool, 7, 1> drive_3_motor_on;

    bool IsMotorOn(uint8 drive) { return ((bits >> (4 + drive)) & 0x01); }
  } m_DOR;

  // 03F4h: Main status register
  union
  {
    uint8 bits = 0;

    BitField<uint8, bool, 0, 1> drive_0_activity;
    BitField<uint8, bool, 1, 1> drive_1_activity;
    BitField<uint8, bool, 2, 1> drive_2_activity;
    BitField<uint8, bool, 3, 1> drive_3_activity;
    BitField<uint8, bool, 4, 1> command_busy;
    BitField<uint8, bool, 5, 1> pio_mode;
    BitField<uint8, bool, 6, 1> data_direction;     // 1 = FDC->CPU, 0=CPU->FDC
    BitField<uint8, bool, 7, 1> request_for_master; // data register ready

    void ClearActivity()
    {
      drive_0_activity = false;
      drive_1_activity = false;
      drive_2_activity = false;
      drive_3_activity = false;
    }
    void SetActivity(uint8 drive)
    {
      ClearActivity();
      bits |= uint8(1 << drive);
    }
  } m_MSR;

  // 03F4h: Data-rate select register.
  // Configuration control register.
  uint8 m_data_rate_index = 0;
  bool m_interrupt_pending = false;
  bool m_disk_change_flag = false;
  bool m_specify_lock = false;

  // Reset countdown - must have 4 sense interrupt requests
  uint8 m_reset_sense_interrupt_count = 0;
  SimulationTime m_reset_begin_time = 0;

  // 03F5h: FIFO
  uint8 m_fifo[FIFO_SIZE] = {};
  uint32 m_fifo_result_size = 0;
  uint32 m_fifo_result_position = 0;
  std::unique_ptr<TimingEvent> m_command_event;

  // Specify parameters
  uint8 m_step_rate_time = 0;
  uint8 m_head_load_time = 0;
  uint8 m_head_unload_time = 0;
  bool m_pio_mode = false;
  bool m_implied_seeks = false;
  bool m_polling_disabled = false;
  bool m_fifo_disabled = true;
  uint8 m_fifo_threshold = 0;
  uint8 m_precompensation_start_track = 0;
  uint8 m_perpendicular_mode = 0;

  // Transfer operation state
  struct CurrentCommand
  {
    union
    {
      struct
      {
        union
        {
          BitField<uint8, uint8, 0, 5> command;
          BitField<uint8, bool, 5, 1> sk; // Skip deleted data address mark
          BitField<uint8, bool, 6, 1> mf; // MFM mode
          BitField<uint8, bool, 7, 1> mt; // Multi track
        };
        uint8 params[MAX_COMMAND_LENGTH - 1];
      };
      uint8 buf[MAX_COMMAND_LENGTH];
    };
    uint8 command_length = 0;

    uint8 GetExpectedParameterCount() const;
    bool HasCommand() const { return (command_length > 0); }
    bool HasAllParameters() const
    {
      return (command_length > 0 && (command_length - 1) == GetExpectedParameterCount());
    }

    void Clear() { command_length = 0; }
  } m_current_command;

  struct CurrentTransfer
  {
    bool active = false;
    bool multi_track = false;
    bool is_write = false;
    uint8 drive = 0;
    uint32 bytes_per_sector = 0;
    uint32 sectors_per_track = 0;
    uint32 sector_offset = 0;
    uint8 sector_buffer[SECTOR_SIZE];

    void Clear();
  } m_current_transfer;

  // Status registers
  uint8 m_st0 = 0;
  uint8 m_st1 = 0;
  uint8 m_st2 = 0;

  bool InReset() const { return !m_DOR.nreset; }
  bool IsDMATransferInProgress() const { return m_current_transfer.active && m_DOR.ndmagate; }
  DriveState* GetCurrentDrive() { return &m_drives[m_DOR.drive_select]; }
  uint8 GetCurrentDriveIndex() const { return m_DOR.drive_select; }
  void SetCurrentDrive(uint8 index) { m_DOR.drive_select = index; }
  bool IsCurrentDrivePresent() const { return IsDrivePresent(GetCurrentDriveIndex()); }
  bool IsCurrentDiskPresent() const { return IsDiskPresent(GetCurrentDriveIndex()); }

  void Reset(bool software_reset);
  void RemoveDisk(uint32 drive);

  void ClearFIFO();
  void WriteToFIFO(uint8 value);

  void BeginCommand();
  void HandleUnsupportedCommand();
  void EndCommand();

  void HangController();
  void TransitionToCommandPhase(); // command - host->fdc
  void TransitionToResultPhase();

  void ConnectIOPorts(Bus* bus);
  void RaiseInterrupt();
  void LowerInterrupt();

  void IOReadStatusRegisterA(uint8* value);
  void IOReadStatusRegisterB(uint8* value);

  void IOReadDigitalInputRegister(uint8* value);
  void IOReadDigitalOutputRegister(uint8* value);
  void IOWriteDigitalOutputRegister(uint8 value);
  void IOWriteDataRateSelectRegister(uint8 value);
  void IOWriteConfigurationControlRegister(uint8 value);
  void IOReadFIFO(uint8* value);
  void IOWriteFIFO(uint8 value);

  void EndTransfer(uint32 drive, uint8 st0_bits, uint8 st1_bits, uint8 st2_bits);

  bool MoveToNextTransferSector();
  void DMAReadCallback(IOPortDataSize size, uint32* value, uint32 remaining_bytes);
  void DMAWriteCallback(IOPortDataSize size, uint32 value, uint32 remaining_bytes);

  // Status code values
  // http://www.threedee.com/jcm/terak/docs/Intel%208272A%20Floppy%20Controller.pdf
  enum : uint8
  {
    ST0_IC_NT = 0x00,
    ST0_IC_AT = 0x40,
    ST0_IC_IC = 0x80,
    ST0_IC_SC = 0xC0,
    ST0_SE = 0x20, // Seek End
    ST0_EC = 0x10  // Equipment Check
  };
  enum : uint8
  {
    ST1_EN = 0x80, // End of Cylinder
    ST1_DE = 0x20, // Data Error
    ST1_OR = 0x10, // Overrun
    ST1_ND = 0x04, // No data
    ST1_NW = 0x02, // Not writable
    ST1_MA = 0x01  // Missing address mark
  };
  enum : uint8
  {
    ST2_CM = 0x40, // Control Mark
    ST2_DD = 0x20, // Data Error in Data Field
    ST2_WC = 0x10, // Wrong Cylinder
    ST2_BC = 0x02, // Bad Cylinder
    ST2_MD = 0x01  // Missing Data Address Mark
  };
  enum : uint8
  {
    CMD_SPECIFY = 0x03,
    CMD_SENSE_STATUS = 0x04,
    CMD_WRITE_DATA = 0x05,
    CMD_READ_DATA = 0x06,
    CMD_RECALIBRATE = 0x07,
    CMD_SENSE_INTERRUPT = 0x08,
    CMD_READ_ID = 0x0A,
    CMD_SEEK = 0x0F,

    // Enhanced drive
    CMD_DUMP_REGISTERS = 0x0E,
    CMD_VERSION = 0x10,
    CMD_UNLOCK = 0x14,
    CMD_LOCK = 0x94,
    CMD_PERPENDICULAR_MODE = 0x12,
    CMD_CONFIGURE = 0x13,
  };

  // Assumes the drive being referenced is m_current_drive.
  uint8 GetST0(uint32 drive, uint8 bits) const;
  uint8 GetST1(uint32 drive, uint8 bits) const;
  uint8 GetST2(uint32 drive, uint8 bits) const;
  uint8 GetST3(uint32 drive, uint8 bits) const;

  // Returns the number of microseconds to move the head one or more tracks.
  CycleCount CalculateHeadSeekTime(uint32 current_track, uint32 destination_track) const;
  CycleCount CalculateHeadSeekTime() const;
  CycleCount CalculateSectorReadTime() const;
};

} // namespace HW
