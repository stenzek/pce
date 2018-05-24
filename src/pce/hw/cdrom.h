#pragma once
#include "pce/clock.h"
#include "pce/bitfield.h"
#include "pce/component.h"
#include <vector>

namespace HW
{
// TODO: "SCSI Device" base class
class CDROM final : public Component
{
public:
  CDROM();
  ~CDROM() override;

  void Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  bool IsBusy() const { return m_busy; }
  bool HasError() const { return m_error; }

  const byte* GetDataBuffer() const { return m_data_buffer.data(); }
  size_t GetDataSize() const { return m_data_buffer.size(); }
  void ClearDataBuffer() { m_data_buffer.clear(); }
  void ClearErrorFlag() { m_error = false; }

  using CommandCompletedCallback = std::function<void()>;
  void SetCommandCompletedCallback(CommandCompletedCallback callback);

  // Returns true if the command is completed.
  bool WriteCommandBuffer(const void* data, size_t data_len);

private:
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
    SCSI_CMD_READ_TOC = 0x43,
    SCSI_CMD_GET_CONFIGURATION = 0x46,
    SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION = 0x4A,
    SCSI_CMD_READ_DISC_INFORMATION = 0x51,
    SCSI_CMD_READ_TRACK_INFORMATION = 0x52,
    SCSI_CMD_RESERVE_TRACK = 0x53,
    SCSI_CMD_SEND_OPC_INFORMATION = 0x54,
    SCSI_CMD_MODE_SELECT_10 = 0x55,
    SCSI_CMD_REPAIR_TRACK = 0x58,
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

  bool BeginCommand();
  void QueueCommand(uint32 time_in_microseconds);
  void CompleteCommand();

  Clock m_clock;

  CommandBuffer m_command_buffer;
  DataBuffer m_data_buffer;

  std::unique_ptr<TimingEvent> m_command_event;
  CommandCompletedCallback m_command_completed_callback;

  bool m_busy = false;
  bool m_error = false;
};
}
