#pragma once
#include "common/bitfield.h"
#include "common/clock.h"
#include "pce/component.h"
#include "pce/dma_controller.h"
#include "pce/types.h"

namespace HW {

// i8237 DMA Controller
class i8237_DMA : public DMAController
{
  DECLARE_OBJECT_TYPE_INFO(i8237_DMA, DMAController);
  DECLARE_OBJECT_NO_FACTORY(i8237_DMA);
  DECLARE_OBJECT_PROPERTY_MAP(i8237_DMA);

public:
  enum DMATransferType
  {
    DMATransferType_Verify,
    DMATransferType_DeviceToMemory,
    DMATransferType_MemoryToDevice
  };

  enum DMAMode
  {
    DMAMode_Demand,
    DMAMode_Single,
    DMAMode_Block,
    DMAMode_Cascade
  };

  i8237_DMA(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  ~i8237_DMA();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  bool ConnectDMAChannel(u32 channel_index, DMAReadCallback&& read_callback,
                         DMAWriteCallback&& write_callback) override;
  bool GetDMAState(u32 channel_index) override;
  void SetDMAState(u32 channel_index, bool request) override;

private:
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('8', '2', '3', '7');
  static constexpr u32 NUM_CHANNELS = 8;
  static constexpr u32 NUM_CHANNELS_PER_CONTROLLER = 4;

  struct Channel
  {
    // count refers to the number of bytes in the transfer
    // bytes_remaining is the count that is written to the io port
    u16 start_address = 0;
    u16 bytes_remaining = 0;
    u16 address = 0;
    u16 count = 0;
    u8 page_address = 0;
    DMATransferType transfer_type = DMATransferType_Verify;
    DMAMode mode = DMAMode_Single;
    bool decrement = false;
    bool auto_reset = false;
    bool masked = true;
    bool request = false;
    bool transfer_complete = false;

    DMAReadCallback read_callback;
    DMAWriteCallback write_callback;

    bool HasCallbacks() const { return (read_callback || write_callback); }

    bool IsActive() const
    {
      // We don't simulate cascade mode.
      if (mode == DMAMode_Cascade)
        return false;

      return !masked && request;
    }
  };

  void ConnectIOPorts();

  bool HasActiveTransfer() const;

  void RescheduleTickEvent();

  void Transfer(u32 channel_index, size_t count);

  void IOReadStartAddress(u32 channel_index, u8* value);
  void IOWriteStartAddress(u32 channel_index, u8 value);
  void IOReadCount(u32 channel_index, u8* value);
  void IOWriteCount(u32 channel_index, u8 value);
  void IOReadPageAddress(u32 channel_index, u8* value);
  void IOWritePageAddress(u32 channel_index, u8 value);
  void IOReadStatus(u32 base_channel, u8* value);
  void IOWriteMode(u32 base_channel, u8 value);
  void IOWriteSingleMask(u32 base_channel, u8 value);
  void IOReadMultiMask(u32 base_channel, u8* value);
  void IOWriteMultiMask(u32 base_channel, u8 value);
  void IOWriteFlipFlopReset(u32 base_channel, u8 value);
  void IOWriteMasterReset(u32 base_channel, u8 value);
  void IOWriteMaskReset(u32 base_channel, u8 value);

  Clock m_clock;

  Channel m_channels[NUM_CHANNELS];
  bool m_flipflops[2] = {};

  u8 m_unused_page_registers[9] = {};

  TimingEvent::Pointer m_tick_event;
  bool m_tick_in_progress = false;
};

} // namespace HW
