#pragma once
#include "YBaseLib/String.h"
#include "common/bitfield.h"
#include "../component.h"
#include <memory>
#include <vector>

class ByteStream;
class TimingEvent;

namespace HW {

// TODO: "SCSI Device" base class
class CDROM : public Component
{
  DECLARE_OBJECT_TYPE_INFO(CDROM, Component);
  DECLARE_OBJECT_NO_FACTORY(CDROM);
  DECLARE_OBJECT_PROPERTY_MAP(CDROM);

  friend class ATACDROM;

public:
  CDROM(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~CDROM() override;

  virtual bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  const String& GetVendorIDString() const { return m_vendor_id_string; }
  const String& GetModelIDString() const { return m_model_id_string; }
  const String& GetFirmwareVersionString() const { return m_firmware_version_string; }

  bool IsBusy() const { return m_busy; }
  bool HasError() const { return m_error; }
  bool HasMedia() const { return (m_media.stream != nullptr); }
  u8 GetSenseKey() const { return static_cast<u8>(m_sense.key); }

  const byte* GetDataBuffer() const { return m_data_buffer.data(); }
  size_t GetDataResponseSize() const { return m_data_response_size; }
  void ClearDataBuffer()
  {
    m_data_buffer.clear();
    m_data_response_size = 0;
  }
  void ClearErrorFlag() { m_error = false; }

  using InterruptCallback = std::function<void()>;
  void SetInterruptCallback(InterruptCallback callback);

  // Returns true if the command is completed.
  bool WriteCommandBuffer(const void* data, size_t data_len);
  void ClearCommandBuffer() { m_command_buffer.clear(); }

  // Media changing.
  bool InsertMedia(const char* filename);

  // Transfer next sector for multiple sector transfers.
  u32 GetRemainingSectors() const { return m_remaining_sectors; }
  bool TransferNextSector();

private:
  static constexpr u32 SECTOR_SIZE = 2048;
  static constexpr u32 AUDIO_SECTOR_SIZE = 2352;
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('C', 'D', 'R');

  using CommandBuffer = std::vector<byte>;
  using DataBuffer = std::vector<byte>;

  enum SCSI_CMD
  {
    SCSI_CMD_TEST_UNIT_READY = 0x00,
    SCSI_CMD_REQUEST_SENSE = 0x03,
    SCSI_CMD_FORMAT_UNIT = 0x04,
    SCSI_CMD_INQUIRY = 0x12,
    SCSI_CMD_START_STOP_UNIT = 0x1B,
    SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E,
    SCSI_CMD_READ_FORMAT_CAPACITIES = 0x23,
    SCSI_CMD_READ_CAPACITY = 0x25,
    SCSI_CMD_READ_10 = 0x28,
    SCSI_CMD_WRITE_10 = 0x2A,
    SCSI_CMD_SEEK_10 = 0x2B,
    SCSI_CMD_WRITE_AND_VERIFY_10 = 0x2E,
    SCSI_CMD_VERIFY_10 = 0x2F,
    SCSI_CMD_SYNCHRONIZE_CACHE = 0x35,
    SCSI_CMD_WRITE_BUFFER = 0x3B,
    SCSI_CMD_READ_BUFFER = 0x3C,
    SCSI_CMD_READ_SUB_CHANNEL = 0x42,
    SCSI_CMD_READ_TOC = 0x43,
    SCSI_CMD_GET_CONFIGURATION = 0x46,
    SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION = 0x4A,
    SCSI_CMD_READ_DISC_INFORMATION = 0x51,
    SCSI_CMD_READ_TRACK_INFORMATION = 0x52,
    SCSI_CMD_RESERVE_TRACK = 0x53,
    SCSI_CMD_SEND_OPC_INFORMATION = 0x54,
    SCSI_CMD_MODE_SELECT_10 = 0x55,
    SCSI_CMD_REPAIR_TRACK = 0x58,
    SCSI_CMD_MODE_SENSE_6 = 0x1A,
    SCSI_CMD_MODE_SENSE_10 = 0x5A,
    SCSI_CMD_CLOSE_TRACK_SESSION = 0x5B,
    SCSI_CMD_READ_BUFFER_CAPACITY = 0x5C,
    SCSI_CMD_SEND_CUE_SHEET = 0x5D,
    SCSI_CMD_REPORT_LUNS = 0xA0,
    SCSI_CMD_BLANK = 0xA1,
    SCSI_CMD_SECURITY_PROTOCOL_IN = 0xA2,
    SCSI_CMD_SEND_KEY = 0xA3,
    SCSI_CMD_REPORT_KEY = 0xA4,
    SCSI_CMD_LOAD_UNLOAD_MEDIUM = 0xA6,
    SCSI_CMD_SET_READ_AHEAD = 0xA7,
    SCSI_CMD_READ_12 = 0xA8,
    SCSI_CMD_WRITE_12 = 0xAA,
    SCSI_CMD_READ_MEDIA_SERIAL_NUMBER = 0xAB,
    SCSI_CMD_SERVICE_ACTION_IN_12 = 0x01,
    SCSI_CMD_GET_PERFORMANCE = 0xAC,
    SCSI_CMD_READ_DISC_STRUCTURE = 0xAD,
    SCSI_CMD_SECURITY_PROTOCOL_OUT = 0xB5,
    SCSI_CMD_SET_STREAMING = 0xB6,
    SCSI_CMD_READ_CD_MSF = 0xB9,
    SCSI_CMD_SET_CD_SPEED = 0xBB,
    SCSI_CMD_MECHANISM_STATUS = 0xBD,
    SCSI_CMD_READ_CD = 0xBE,
    SCSI_CMD_SEND_DISC_STRUCTURE = 0xBF
  };

  enum SENSE_KEY : u8
  {
    SENSE_NO_STATUS = 0x00,
    SENSE_NOT_READY = 0x02,
    SENSE_ILLEGAL_REQUEST = 0x03,
    SENSE_UNIT_ATTENTION = 0x06
  };

  enum ADDITIONAL_SENSE_CODE
  {
    ASC_ILLEGAL_OPCODE = 0x20,
    ASC_LOGICAL_BLOCK_OUT_OF_RANGE = 0x21,
    ASC_INVALID_FIELD_IN_CMD_PACKET = 0x24,
    ASC_MEDIUM_MAY_HAVE_CHANGED = 0x28,
    ASC_SAVING_PARAMETERS_NOT_SUPPORTED = 0x39,
    ASC_MEDIUM_NOT_PRESENT = 0x3A
  };

  void EjectMedia();
  void UpdateSenseInfo(SENSE_KEY key, u8 asc);
  void AllocateData(u32 reserve_length, u32 response_length);

  u8 ReadCommandBufferByte(u32 offset) const;
  u16 ReadCommandBufferWord(u32 offset) const;
  u32 ReadCommandBufferDWord(u32 offset) const;
  u32 ReadCommandBufferLBA24(u32 offset) const;
  u64 ReadCommandBufferLBA48(u32 offset) const;

  void WriteDataBufferByte(u32 offset, u8 value);
  void WriteDataBufferWord(u32 offset, u16 value);
  void WriteDataBufferDWord(u32 offset, u32 value);
  void WriteDataBufferLBA24(u32 offset, u32 value);
  void WriteDataBufferLBA48(u32 offset, u64 value);

  bool BeginCommand();
  void QueueCommand(CycleCount time_in_microseconds);
  void ExecuteCommand();
  void CompleteCommand();
  void AbortCommand(SENSE_KEY key, u8 asc);
  void RaiseInterrupt();
  void SetIndicator();
  void ClearIndicator();

  // Returns the time in microseconds to move the head/laser to the specified LBA.
  CycleCount CalculateSeekTime(u64 current_lba, u64 destination_lba) const;
  CycleCount CalculateReadTime(u64 lba, u32 sector_count) const;

  void HandleTestUnitReadyCommand();
  void HandlePreventMediumRemovalCommand();
  void HandleRequestSenseCommand();
  void HandleInquiryCommand();
  void HandleReadCapacityCommand();
  void HandleReadTOCCommand();
  void HandleReadSubChannelCommand();
  void HandleModeSenseCommand();
  void HandleReadCommand();
  void HandleMechanismStatusCommand();
  void HandleSeekCommand();

  SmallString m_vendor_id_string;
  SmallString m_model_id_string;
  SmallString m_firmware_version_string;

  CommandBuffer m_command_buffer;
  DataBuffer m_data_buffer;
  u32 m_data_response_size = 0;

  std::unique_ptr<TimingEvent> m_command_event;
  InterruptCallback m_interrupt_callback;

  bool m_busy = false;
  bool m_error = false;

  // Sense information
  struct
  {
    SENSE_KEY key = SENSE_NO_STATUS;
    u8 information[4] = {};
    u8 specific_information[4] = {};
    u8 key_spec[3] = {};
    u8 fruc = 0;
    u8 asc = 0;
    u8 ascq = 0;
  } m_sense;

  // Media information
  struct
  {
    String filename;
    ByteStream* stream = nullptr;
    u64 total_sectors = 0;
  } m_media;

  // Current head position
  u64 m_current_lba = 0;
  u32 m_remaining_sectors = 0;
  bool m_tray_locked = false;
};

} // namespace HW
