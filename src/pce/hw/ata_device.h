#pragma once
#include "../component.h"
#include "../types.h"
#include "common/bitfield.h"

namespace HW {

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

class ATADevice : public Component
{
  DECLARE_OBJECT_TYPE_INFO(ATADevice, Component);
  DECLARE_OBJECT_NO_FACTORY(ATADevice);
  DECLARE_OBJECT_NO_PROPERTIES(ATADevice);

public:
  ATADevice(const ObjectTypeInfo* type_info = &s_type_info);
  virtual ~ATADevice();

  virtual bool Initialize(System* system, Bus* bus) override;
  virtual void Reset() override;

  virtual bool LoadState(BinaryReader& reader) override;
  virtual bool SaveState(BinaryWriter& writer) override;

  // Status/error registers.
  ATAStatusRegister ReadGetStatusRegister() const { return m_registers.status; }
  ATA_ERR ReadErrorRegister() const { return m_registers.error; }

  // Command block.
  u8 ReadCommandBlockSectorCount(bool hob) const;
  u8 ReadCommandBlockSectorNumber(bool hob) const;
  u8 ReadCommandBlockCylinderLow(bool hob) const;
  u8 ReadCommandBlockCylinderHigh(bool hob) const;
  void WriteFeatureSelect(u8 value);
  void WriteCommandBlockSectorCount(u8 value);
  void WriteCommandBlockSectorNumber(u8 value);
  void WriteCommandBlockSectorCylinderLow(u8 value);
  void WriteCommandBlockSectorCylinderHigh(u8 value);

  // Command register.
  virtual void WriteCommandRegister(u8 value) = 0;

protected:
  virtual void DoReset(bool is_hardware_reset);

  struct
  {
    ATAStatusRegister status;
    ATA_ERR error;
    u8 feature_select;

    u16 sector_count;
    u16 sector_number;
    u16 cylinder_low;
    u16 cylinder_high;
  } m_registers = {};
};

} // namespace HW