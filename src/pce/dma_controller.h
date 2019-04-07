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
  DECLARE_OBJECT_TYPE_INFO(DMAController, Component);
  DECLARE_OBJECT_NO_FACTORY(DMAController);
  DECLARE_OBJECT_NO_PROPERTIES(DMAController);

public:
  DMAController(const String& identifier, const ObjectTypeInfo* type_info = &s_type_info);
  virtual ~DMAController();

  using DMAReadCallback = std::function<void(IOPortDataSize size, u32* value, u32 remaining_bytes)>;
  using DMAWriteCallback = std::function<void(IOPortDataSize size, u32 value, u32 remaining_bytes)>;

  // Connect DMA channel to a device
  virtual bool ConnectDMAChannel(u32 channel_index, DMAReadCallback&& read_callback,
                                 DMAWriteCallback&& write_callback) = 0;

  // Sends DREQ signal from device
  virtual bool GetDMAState(u32 channel_index) = 0;
  virtual void SetDMAState(u32 channel_index, bool request) = 0;
};
