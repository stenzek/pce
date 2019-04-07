#pragma once

#include "../component.h"
#include "../dma_controller.h"
#include "YBaseLib/PODArray.h"
#include "common/bitfield.h"
#include "common/clock.h"
#include "floppy.h"
#include <array>

class ByteStream;
class InterruptController;

namespace HW {
class Floppy;

class FDC : public Component
{
  DECLARE_OBJECT_TYPE_INFO(FDC, Component);
  DECLARE_OBJECT_NO_FACTORY(FDC);
  DECLARE_OBJECT_PROPERTY_MAP(FDC);

public:
  enum Model : u8
  {
    Model_8272,
    Model_82072,
    Model_82077,
  };

  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('F', 'D', 'C');
  static constexpr u32 SECTOR_SIZE = 512;
  static constexpr u32 MAX_DRIVES = 4;

  // We use 1MHz as the clock frequency for the FDC, so we can specify "cycles" as microseconds.
  static constexpr float CLOCK_FREQUENCY = 1000000;

public:
  FDC(const String& identifier, Model model = Model_8272, const ObjectTypeInfo* type_info = &s_type_info);
  ~FDC();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  Floppy::DriveType GetDriveType_(u32 drive) const;
  bool IsDrivePresent(u32 drive) const;
  bool IsDiskPresent(u32 drive) const;
  u32 GetDriveCount() const;

  bool AttachDrive(u32 number, Floppy* drive);

protected:
  static constexpr u32 IRQ_NUMBER = 6;
  static constexpr u32 DMA_CHANNEL = 2;
  static constexpr u32 MAX_COMMAND_LENGTH = 9;

  void SetActivity(u32 drive_number, bool writing = false);
  void ClearActivity();

  InterruptController* m_interrupt_controller = nullptr;
  DMAController* m_dma = nullptr;
  Clock m_clock;
  Model m_model;
  bool m_fast_transfers = false;

  struct DriveState
  {
    Floppy* floppy = nullptr;

    // TODO: Move some of these to Floppy?
    u32 current_cylinder = 0;
    u32 current_head = 0;
    u32 current_sector = 1;
    u32 current_lba = 0;

    bool write_protect = false;

    bool data_was_read = false;
    bool data_was_written = false;
    bool step_latch = false;

    // Last seek resulted in a forward (greater) track number
    bool direction = false;
  };
  std::array<DriveState, MAX_DRIVES> m_drives;

  static constexpr u32 FIFO_SIZE = 16;

  // 03F2h: Digital Output Register
  union
  {
    u8 bits = 0;

    BitField<u8, u8, 0, 2> drive_select;
    BitField<u8, bool, 2, 1> nreset;
    BitField<u8, bool, 3, 1> ndmagate;
    BitField<u8, bool, 4, 1> drive_0_motor_on;
    BitField<u8, bool, 5, 1> drive_1_motor_on;
    BitField<u8, bool, 6, 1> drive_2_motor_on;
    BitField<u8, bool, 7, 1> drive_3_motor_on;

    bool IsMotorOn(u8 drive) { return ((bits >> (4 + drive)) & 0x01); }
  } m_DOR;

  // 03F4h: Main status register
  union
  {
    u8 bits = 0;

    BitField<u8, bool, 0, 1> drive_0_activity;
    BitField<u8, bool, 1, 1> drive_1_activity;
    BitField<u8, bool, 2, 1> drive_2_activity;
    BitField<u8, bool, 3, 1> drive_3_activity;
    BitField<u8, bool, 4, 1> command_busy;
    BitField<u8, bool, 5, 1> pio_mode;
    BitField<u8, bool, 6, 1> data_direction;     // 1 = FDC->CPU, 0=CPU->FDC
    BitField<u8, bool, 7, 1> request_for_master; // data register ready
  } m_MSR;

  // 03F4h: Data-rate select register.
  // Configuration control register.
  u8 m_data_rate_index = 0;
  bool m_interrupt_pending = false;
  bool m_disk_change_flag = false;
  bool m_specify_lock = false;

  // Reset countdown - must have 4 sense interrupt requests
  u8 m_reset_sense_interrupt_count = 0;
  SimulationTime m_reset_begin_time = 0;

  // 03F5h: FIFO
  u8 m_fifo[FIFO_SIZE] = {};
  u32 m_fifo_result_size = 0;
  u32 m_fifo_result_position = 0;
  std::unique_ptr<TimingEvent> m_command_event;

  // Specify parameters
  u8 m_step_rate_time = 0;
  u8 m_head_load_time = 0;
  u8 m_head_unload_time = 0;
  bool m_pio_mode = false;
  bool m_implied_seeks = false;
  bool m_polling_disabled = false;
  bool m_fifo_disabled = true;
  u8 m_fifo_threshold = 0;
  u8 m_precompensation_start_track = 0;
  u8 m_perpendicular_mode = 0;

  // Transfer operation state
  struct CurrentCommand
  {
    union
    {
      struct
      {
        union
        {
          BitField<u8, u8, 0, 5> command;
          BitField<u8, bool, 5, 1> sk; // Skip deleted data address mark
          BitField<u8, bool, 6, 1> mf; // MFM mode
          BitField<u8, bool, 7, 1> mt; // Multi track
        };
        u8 params[MAX_COMMAND_LENGTH - 1];
      };
      u8 buf[MAX_COMMAND_LENGTH];
    };
    u8 command_length = 0;

    u8 GetExpectedParameterCount() const;
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
    u8 drive = 0;
    u32 bytes_per_sector = 0;
    u32 sectors_per_track = 0;
    u32 sector_offset = 0;
    u8 sector_buffer[SECTOR_SIZE];

    void Clear();
  } m_current_transfer;

  // Status registers
  u8 m_st0 = 0;
  u8 m_st1 = 0;
  u8 m_st2 = 0;

  bool SeekDrive(u32 drive, u32 cylinder, u32 head, u32 sector);
  bool SeekToNextSector(u32 drive);
  void ReadCurrentSector(u32 drive, void* data);
  void WriteCurrentSector(u32 drive, const void* data);

  bool InReset() const { return !m_DOR.nreset; }
  bool IsDMATransferInProgress() const { return m_current_transfer.active && m_DOR.ndmagate; }
  DriveState* GetCurrentDrive() { return &m_drives[m_DOR.drive_select]; }
  u8 GetCurrentDriveIndex() const { return m_DOR.drive_select; }
  void SetCurrentDrive(u8 index) { m_DOR.drive_select = index; }
  bool IsCurrentDrivePresent() const { return IsDrivePresent(GetCurrentDriveIndex()); }
  bool IsCurrentDiskPresent() const { return IsDiskPresent(GetCurrentDriveIndex()); }

  void Reset(bool software_reset);

  void ClearFIFO();
  void WriteToFIFO(u8 value);

  void BeginCommand();
  void HandleUnsupportedCommand();
  void EndCommand();

  void HangController();
  void TransitionToCommandPhase(); // command - host->fdc
  void TransitionToResultPhase();

  void ConnectIOPorts(Bus* bus);
  void RaiseInterrupt();
  void LowerInterrupt();

  void IOReadStatusRegisterA(u8* value);
  void IOReadStatusRegisterB(u8* value);

  void IOReadDigitalInputRegister(u8* value);
  void IOReadDigitalOutputRegister(u8* value);
  void IOWriteDigitalOutputRegister(u8 value);
  void IOWriteDataRateSelectRegister(u8 value);
  void IOWriteConfigurationControlRegister(u8 value);
  void IOReadFIFO(u8* value);
  void IOWriteFIFO(u8 value);

  void EndTransfer(u32 drive, u8 st0_bits, u8 st1_bits, u8 st2_bits);

  bool MoveToNextTransferSector();
  void DMAReadCallback(IOPortDataSize size, u32* value, u32 remaining_bytes);
  void DMAWriteCallback(IOPortDataSize size, u32 value, u32 remaining_bytes);

  // Status code values
  // http://www.threedee.com/jcm/terak/docs/Intel%208272A%20Floppy%20Controller.pdf
  enum : u8
  {
    ST0_IC_NT = 0x00,
    ST0_IC_AT = 0x40,
    ST0_IC_IC = 0x80,
    ST0_IC_SC = 0xC0,
    ST0_SE = 0x20, // Seek End
    ST0_EC = 0x10  // Equipment Check
  };
  enum : u8
  {
    ST1_EN = 0x80, // End of Cylinder
    ST1_DE = 0x20, // Data Error
    ST1_OR = 0x10, // Overrun
    ST1_ND = 0x04, // No data
    ST1_NW = 0x02, // Not writable
    ST1_MA = 0x01  // Missing address mark
  };
  enum : u8
  {
    ST2_CM = 0x40, // Control Mark
    ST2_DD = 0x20, // Data Error in Data Field
    ST2_WC = 0x10, // Wrong Cylinder
    ST2_BC = 0x02, // Bad Cylinder
    ST2_MD = 0x01  // Missing Data Address Mark
  };
  enum : u8
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
  u8 GetST0(u32 drive, u8 bits) const;
  u8 GetST1(u32 drive, u8 bits) const;
  u8 GetST2(u32 drive, u8 bits) const;
  u8 GetST3(u32 drive, u8 bits) const;

  // Returns the number of microseconds to move the head one or more tracks.
  CycleCount CalculateHeadSeekTime(u32 drive, u32 destination_track) const;
  CycleCount CalculateHeadSeekTime(u32 drive) const;
  CycleCount CalculateSectorReadTime() const;
};

} // namespace HW
