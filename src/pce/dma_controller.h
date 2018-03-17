#pragma once
#include "pce/component.h"
#include <functional>

// enum class DMAStatus
// {
//     Success,
//     TransferComplete,
//     NotReady
// };

class DMAController : public Component
{
public:
  using DMAReadCallback = std::function<void(IOPortDataSize size, uint32* value, uint32 remaining_bytes)>;
  using DMAWriteCallback = std::function<void(IOPortDataSize size, uint32 value, uint32 remaining_bytes)>;

  // Connect DMA channel to a device
  virtual bool ConnectDMAChannel(uint32 channel_index, DMAReadCallback&& read_callback,
                                 DMAWriteCallback&& write_callback) = 0;

  // Sends DREQ signal from device
  virtual bool GetDMAState(uint32 channel_index) = 0;
  virtual void SetDMAState(uint32 channel_index, bool request) = 0;
};
