#pragma once
#include "../component.h"
#include "../types.h"
#include "common/bitfield.h"

namespace HW {

class HDC;

class ATADevice : public Component
{
  DECLARE_OBJECT_TYPE_INFO(ATADevice, Component);
  DECLARE_OBJECT_NO_FACTORY(ATADevice);
  DECLARE_OBJECT_PROPERTY_MAP(ATADevice);

public:
  // TODO: Move this crap to a namespace
  enum ATA_ERR : u8
  {
    ATA_ERR_BBK = 0x80,   // Bad sector
    ATA_ERR_UNC = 0x40,   // Uncorrectable data
    ATA_ERR_MC = 0x20,    // No media
    ATA_ERR_IDNF = 0x10,  // ID mark not found
    ATA_ERR_MCR = 0x08,   // No media
    ATA_ERR_ABRT = 0x04,  // Command aborted
    ATA_ERR_TK0NF = 0x02, // Track 0 not found
    ATA_ERR_AMNF = 0x01,  // No address mark
    ATA_ERR_NONE = 0x00
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
    ATA_CMD_READ_NATIVE_MAX_ADDRESS = 0x27,
    ATA_CMD_READ_NATIVE_MAX_ADDRESS_EXT = 0xF8,

    ATAPI_CMD_READ = 0xA8,
    ATAPI_CMD_EJECT = 0x1B,
    ATAPI_CMD_PACKET = 0xA0
  };

  union ATAStatusRegister
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
  };

#pragma pack(push, 1)
  struct ATA_IDENTIFY_RESPONSE
  {
    u16 flags;                        // 0
    u16 cylinders;                    // 1
    u16 unused_0;                     // 2
    u16 heads;                        // 3
    u16 unformatted_bytes_per_track;  // 4
    u16 unformatted_bytes_per_sector; // 5
    u16 sectors_per_track;            // 6
    u16 unused_1[3];                  // 7-9
    char serial_number[20];           // 10-19
    u16 buffer_type;                  // 20
    u16 buffer_size;                  // 21 - in 512kb increments
    u16 ecc_bytes;                    // 22 - should be 4
    char firmware_revision[8];        // 23-26
    char model[40];                   // 27-46
    u16 readwrite_multiple_supported; // 47
    u16 dword_io_supported;           // 48
    u16 support;                      // 49
    u16 unused_2;                     // 50
    u16 pio_timing_mode;              // 51
    u16 dma_timing_mode;              // 52
    u16 user_fields_valid;            // 53
    u16 user_cylinders;               // 54
    u16 user_heads;                   // 55
    u16 user_sectors_per_track;       // 56
    u32 user_total_sectors;           // 57-58
    u16 multiple_sector_setting;      // 59
    u32 lba_sectors;                  // 60-61
    u16 singleword_dma_modes;         // 62
    u16 multiword_dma_modes;          // 63
    u16 pio_modes_supported;          // 64
    u16 pio_cycle_time[4];            // 65-68
    u16 unused_4[11];                 // 69-79
    u16 word_80;                      // 80
    u16 minor_version_number;         // 81
    u16 word_82;                      // 82
    u16 supports_lba48;               // 83 set (1 << 10) here
    u16 word_84;                      // 84
    u16 word_85;                      // 85
    u16 word_86;                      // 86
    u16 word_87;                      // 87
    u16 word_88;                      // 88
    u16 word_89;                      // 89
    u16 word_90;                      // 90
    u16 word_91;                      // 91
    u16 word_92;                      // 92
    u16 word_93;                      // 93
    u16 unused_5[6];                  // 94-99
    u64 lba48_sectors;                // 100-103
    u16 unused_6[152];                // 104-255
  };
  static_assert(sizeof(ATA_IDENTIFY_RESPONSE) == 512, "ATA identify response is 512 bytes");
#pragma pack(pop)

  union ATADriveSelectRegister
  {
    u8 bits;
    BitField<u8, u8, 0, 4> head;
    BitField<u8, u8, 4, 1> drive;
    BitField<u8, bool, 6, 1> lba_enable;
  };

public:
  ATADevice(const String& identifier, u32 ata_channel, u32 ata_drive, const ObjectTypeInfo* type_info = &s_type_info);
  virtual ~ATADevice();

  virtual bool Initialize(System* system, Bus* bus) override;
  virtual void Reset() override;

  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

  u32 GetATAChannelNumber() const { return m_ata_channel_number; }
  u32 GetATADriveNumber() const { return m_ata_drive_number; }

  // Status/error registers.
  u8 ReadStatusRegister() const { return m_registers.status.bits; }
  u8 ReadErrorRegister() const { return static_cast<u8>(m_registers.error); }

  // Command block.
  u8 ReadCommandBlockSectorCount(bool hob) const;
  u8 ReadCommandBlockSectorNumber(bool hob) const;
  u8 ReadCommandBlockCylinderLow(bool hob) const;
  u8 ReadCommandBlockCylinderHigh(bool hob) const;
  u8 ReadCommandBlockDriveSelect() const;
  void WriteFeatureSelect(u8 value);
  void WriteCommandBlockSectorCount(u8 value);
  void WriteCommandBlockSectorNumber(u8 value);
  void WriteCommandBlockSectorCylinderLow(u8 value);
  void WriteCommandBlockSectorCylinderHigh(u8 value);
  void WriteCommandBlockDriveSelect(u8 value);
  void WriteCommandBlockFeatureSelect(u8 value);

  // Software reset.
  virtual void DoReset(bool is_hardware_reset);

  // Command register.
  virtual void WriteCommandRegister(u8 value) = 0;

  // Data port. TODO: Make specific size overloads.
  void ReadDataPort(void* buffer, u32 size);
  void WriteDataPort(const void* buffer, u32 size);

protected:
  static const u32 SERIALIZATION_ID = MakeSerializationID('A', 'T', 'A', 'D');

  static void PutIdentifyString(char* buffer, uint32 buffer_size, const char* str);

  void RaiseInterrupt();

  void SetupBuffer(u32 size, bool is_write);
  void ResetBuffer();
  void BufferReady(bool raise_interrupt);
  virtual void OnBufferEnd() = 0;

  HDC* m_ata_controller = nullptr;
  u32 m_ata_channel_number;
  u32 m_ata_drive_number;

  struct
  {
    ATAStatusRegister status;
    ATADriveSelectRegister drive_select;
    u8 error;
    u8 feature_select;

    u16 sector_count;
    u16 sector_number;
    u16 cylinder_low;
    u16 cylinder_high;
  } m_registers = {};

  // Current buffer being transferred.
  struct
  {
    std::vector<byte> data;
    u32 size;
    u32 position;
    bool is_write;
    bool valid;
  } m_buffer = {};
};

} // namespace HW