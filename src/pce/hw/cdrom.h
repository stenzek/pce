#pragma once
#include "YBaseLib/String.h"
#include "common/bitfield.h"
#include "common/clock.h"
#include "pce/component.h"
#include <vector>

class ByteStream;

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
  uint8 GetSenseKey() const { return static_cast<uint8>(m_sense.key); }

  const byte* GetDataBuffer() const { return m_data_buffer.data(); }
  size_t GetDataResponseSize() const { return m_data_response_size; }
  void ClearDataBuffer()
  {
    m_data_buffer.clear();
    m_data_response_size = 0;
  }
  void ClearErrorFlag() { m_error = false; }

  using CommandCompletedCallback = std::function<void()>;
  void SetCommandCompletedCallback(CommandCompletedCallback callback);

  // Returns true if the command is completed.
  bool WriteCommandBuffer(const void* data, size_t data_len);
  void ClearCommandBuffer() { m_command_buffer.clear(); }

  // Media changing.
  bool InsertMedia(const char* filename);

  // Transfer next sector for multiple sector transfers.
  uint32 GetRemainingSectors() const { return m_remaining_sectors; }
  bool TransferNextSector();

private:
  static const uint32 SECTOR_SIZE = 2048;
  static const uint32 AUDIO_SECTOR_SIZE = 2352;
  static const uint32 SERIALIZATION_ID = MakeSerializationID('C', 'D', 'R');

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

  enum SENSE_KEY : uint8
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
  void UpdateSenseInfo(SENSE_KEY key, uint8 asc);
  void AllocateData(uint32 reserve_length, uint32 response_length);

  uint8 ReadCommandBufferByte(uint32 offset) const;
  uint16 ReadCommandBufferWord(uint32 offset) const;
  uint32 ReadCommandBufferDWord(uint32 offset) const;
  uint32 ReadCommandBufferLBA24(uint32 offset) const;
  uint64 ReadCommandBufferLBA48(uint32 offset) const;

  void WriteDataBufferByte(uint32 offset, uint8 value);
  void WriteDataBufferWord(uint32 offset, uint16 value);
  void WriteDataBufferDWord(uint32 offset, uint32 value);
  void WriteDataBufferLBA24(uint32 offset, uint32 value);
  void WriteDataBufferLBA48(uint32 offset, uint64 value);

  bool BeginCommand();
  void QueueCommand(CycleCount time_in_microseconds);
  void ExecuteCommand();
  void CompleteCommand();
  void AbortCommand(SENSE_KEY key, uint8 asc);

  // Returns the time in microseconds to move the head/laser to the specified LBA.
  CycleCount CalculateSeekTime(uint64 current_lba, uint64 destination_lba) const;
  CycleCount CalculateReadTime(uint64 lba, uint32 sector_count) const;

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

  Clock m_clock;
  SmallString m_vendor_id_string;
  SmallString m_model_id_string;
  SmallString m_firmware_version_string;

  CommandBuffer m_command_buffer;
  DataBuffer m_data_buffer;
  uint32 m_data_response_size = 0;

  std::unique_ptr<TimingEvent> m_command_event;
  CommandCompletedCallback m_command_completed_callback;

  bool m_busy = false;
  bool m_error = false;

  // Sense information
  struct
  {
    SENSE_KEY key = SENSE_NO_STATUS;
    uint8 information[4] = {};
    uint8 specific_information[4] = {};
    uint8 key_spec[3] = {};
    uint8 fruc = 0;
    uint8 asc = 0;
    uint8 ascq = 0;
  } m_sense;

  // Media information
  struct
  {
    String filename;
    ByteStream* stream = nullptr;
    uint64 total_sectors = 0;
  } m_media;

  // Current head position
  uint64 m_current_lba = 0;
  uint32 m_remaining_sectors = 0;
  bool m_tray_locked = false;
};

} // namespace HW
