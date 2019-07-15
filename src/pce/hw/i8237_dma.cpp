#include "pce/hw/i8237_dma.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <algorithm>
Log_SetChannel(i8253_PIT);

namespace HW {
DEFINE_OBJECT_TYPE_INFO(i8237_DMA);
BEGIN_OBJECT_PROPERTY_MAP(i8237_DMA)
END_OBJECT_PROPERTY_MAP()

i8237_DMA::i8237_DMA(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info)
{
}

i8237_DMA::~i8237_DMA() = default;

bool i8237_DMA::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  ConnectIOPorts();

  // As we're only using a single cycle here, we'll always be up to date, so no need to sync in IO handlers.
  auto tick_callback = [this](TimingEvent*, CycleCount cycles, CycleCount) {
    m_tick_in_progress = true;

    for (u32 i = 0; i < NUM_CHANNELS; i++)
    {
      Channel* channel = &m_channels[i];

      // Request active?
      if (!channel->IsActive())
        continue;

      Transfer(i, size_t(cycles));
    }

    m_tick_in_progress = false;
    RescheduleTickEvent();
  };
  m_tick_event =
    m_system->CreateClockedEvent("i8237 DMA Tick", CLOCK_FREQUENCY, 1, std::move(tick_callback), HasActiveTransfer());
  return true;
}

void i8237_DMA::Reset()
{
  for (u32 i = 0; i < NUM_CHANNELS; i++)
  {
    Channel* channel = &m_channels[i];
    channel->decrement = false;
    channel->auto_reset = false;
    channel->transfer_type = DMATransferType_Verify;
    channel->mode = DMAMode_Demand;
    channel->masked = false;
    channel->request = false;
  }

  Y_memzero(m_flipflops, sizeof(m_flipflops));
  RescheduleTickEvent();
}

bool i8237_DMA::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  for (u32 i = 0; i < NUM_CHANNELS; i++)
  {
    Channel* channel = &m_channels[i];
    reader.SafeReadUInt16(&channel->start_address);
    reader.SafeReadUInt16(&channel->bytes_remaining);
    reader.SafeReadUInt16(&channel->address);
    reader.SafeReadUInt16(&channel->count);
    reader.SafeReadUInt8(&channel->page_address);

    u8 transfer_type, mode;
    reader.SafeReadUInt8(&transfer_type);
    reader.SafeReadUInt8(&mode);
    channel->transfer_type = static_cast<DMATransferType>(transfer_type);
    channel->mode = static_cast<DMAMode>(mode);

    reader.SafeReadBool(&channel->decrement);
    reader.SafeReadBool(&channel->auto_reset);
    reader.SafeReadBool(&channel->masked);
    reader.SafeReadBool(&channel->request);
    reader.SafeReadBool(&channel->transfer_complete);
  }

  reader.SafeReadBytes(m_flipflops, sizeof(m_flipflops));
  reader.SafeReadBytes(m_unused_page_registers, sizeof(m_unused_page_registers));
  RescheduleTickEvent();
  return true;
}

bool i8237_DMA::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  for (u32 i = 0; i < NUM_CHANNELS; i++)
  {
    Channel* channel = &m_channels[i];
    writer.WriteUInt16(channel->start_address);
    writer.WriteUInt16(channel->bytes_remaining);
    writer.WriteUInt16(channel->address);
    writer.WriteUInt16(channel->count);
    writer.WriteUInt8(channel->page_address);
    writer.WriteUInt8(static_cast<u8>(channel->transfer_type));
    writer.WriteUInt8(static_cast<u8>(channel->mode));
    writer.WriteBool(channel->decrement);
    writer.WriteBool(channel->auto_reset);
    writer.WriteBool(channel->masked);
    writer.WriteBool(channel->request);
    writer.WriteBool(channel->transfer_complete);
  }

  writer.WriteBytes(m_flipflops, sizeof(m_flipflops));
  writer.WriteBytes(m_unused_page_registers, sizeof(m_unused_page_registers));
  return true;
}

bool i8237_DMA::ConnectDMAChannel(u32 channel_index, DMAReadCallback&& read_callback, DMAWriteCallback&& write_callback)
{
  if (channel_index >= NUM_CHANNELS || m_channels[channel_index].read_callback)
    return false;

  Channel* channel = &m_channels[channel_index];
  channel->read_callback = read_callback;
  channel->write_callback = write_callback;
  return true;
}

bool i8237_DMA::GetDMAState(u32 channel_index)
{
  // Prevent recursive calls to update, since we can go tick -> callback -> setstate -> tick.
  if (!m_tick_in_progress)
    m_tick_event->InvokeEarly();

  Channel* channel = &m_channels[channel_index];
  DebugAssert(channel_index < NUM_CHANNELS);

  if (!m_tick_in_progress)
    RescheduleTickEvent();

  return channel->request;
}

void i8237_DMA::SetDMAState(u32 channel_index, bool request)
{
  // Prevent recursive calls to update, since we can go tick -> callback -> setstate -> tick.
  if (!m_tick_in_progress)
    m_tick_event->InvokeEarly();

  Channel* channel = &m_channels[channel_index];
  DebugAssert(channel_index < NUM_CHANNELS);
  channel->request = request;

  if (!m_tick_in_progress)
    RescheduleTickEvent();
}

void i8237_DMA::ConnectIOPorts()
{
  // What a mess
  m_bus->ConnectIOPortRead(0x00, this, std::bind(&i8237_DMA::IOReadStartAddress, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x00, this, std::bind(&i8237_DMA::IOWriteStartAddress, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x87, this, std::bind(&i8237_DMA::IOReadPageAddress, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x87, this, std::bind(&i8237_DMA::IOWritePageAddress, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x01, this, std::bind(&i8237_DMA::IOReadCount, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x01, this, std::bind(&i8237_DMA::IOWriteCount, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x02, this, std::bind(&i8237_DMA::IOReadStartAddress, this, 1, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x02, this, std::bind(&i8237_DMA::IOWriteStartAddress, this, 1, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x83, this, std::bind(&i8237_DMA::IOReadPageAddress, this, 1, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x83, this, std::bind(&i8237_DMA::IOWritePageAddress, this, 1, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x03, this, std::bind(&i8237_DMA::IOReadCount, this, 1, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x03, this, std::bind(&i8237_DMA::IOWriteCount, this, 1, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x04, this, std::bind(&i8237_DMA::IOReadStartAddress, this, 2, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x04, this, std::bind(&i8237_DMA::IOWriteStartAddress, this, 2, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x81, this, std::bind(&i8237_DMA::IOReadPageAddress, this, 2, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x81, this, std::bind(&i8237_DMA::IOWritePageAddress, this, 2, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x05, this, std::bind(&i8237_DMA::IOReadCount, this, 2, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x05, this, std::bind(&i8237_DMA::IOWriteCount, this, 2, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x06, this, std::bind(&i8237_DMA::IOReadStartAddress, this, 3, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x06, this, std::bind(&i8237_DMA::IOWriteStartAddress, this, 3, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x82, this, std::bind(&i8237_DMA::IOReadPageAddress, this, 3, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x82, this, std::bind(&i8237_DMA::IOWritePageAddress, this, 3, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x07, this, std::bind(&i8237_DMA::IOReadCount, this, 3, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x07, this, std::bind(&i8237_DMA::IOWriteCount, this, 3, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0xC0, this, std::bind(&i8237_DMA::IOReadStartAddress, this, 4, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xC0, this, std::bind(&i8237_DMA::IOWriteStartAddress, this, 4, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x8F, this, std::bind(&i8237_DMA::IOReadPageAddress, this, 4, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x8F, this, std::bind(&i8237_DMA::IOWritePageAddress, this, 4, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0xC2, this, std::bind(&i8237_DMA::IOReadCount, this, 4, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xC2, this, std::bind(&i8237_DMA::IOWriteCount, this, 4, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0xC4, this, std::bind(&i8237_DMA::IOReadStartAddress, this, 5, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xC4, this, std::bind(&i8237_DMA::IOWriteStartAddress, this, 5, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x8B, this, std::bind(&i8237_DMA::IOReadPageAddress, this, 5, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x8B, this, std::bind(&i8237_DMA::IOWritePageAddress, this, 5, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0xC6, this, std::bind(&i8237_DMA::IOReadCount, this, 5, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xC6, this, std::bind(&i8237_DMA::IOWriteCount, this, 5, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0xC8, this, std::bind(&i8237_DMA::IOReadStartAddress, this, 6, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xC8, this, std::bind(&i8237_DMA::IOWriteStartAddress, this, 6, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x89, this, std::bind(&i8237_DMA::IOReadPageAddress, this, 6, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x89, this, std::bind(&i8237_DMA::IOWritePageAddress, this, 6, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0xCA, this, std::bind(&i8237_DMA::IOReadCount, this, 6, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xCA, this, std::bind(&i8237_DMA::IOWriteCount, this, 6, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0xCC, this, std::bind(&i8237_DMA::IOReadStartAddress, this, 7, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xCC, this, std::bind(&i8237_DMA::IOWriteStartAddress, this, 7, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0x8A, this, std::bind(&i8237_DMA::IOReadPageAddress, this, 7, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x8A, this, std::bind(&i8237_DMA::IOWritePageAddress, this, 7, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0xCE, this, std::bind(&i8237_DMA::IOReadCount, this, 7, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xCE, this, std::bind(&i8237_DMA::IOWriteCount, this, 7, std::placeholders::_2));

  // Command/reset registers
  m_bus->ConnectIOPortRead(0x08, this, std::bind(&i8237_DMA::IOReadStatus, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortRead(0xD0, this, std::bind(&i8237_DMA::IOReadStatus, this, 4, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0A, this, std::bind(&i8237_DMA::IOWriteSingleMask, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xD4, this, std::bind(&i8237_DMA::IOWriteSingleMask, this, 4, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0B, this, std::bind(&i8237_DMA::IOWriteMode, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xD6, this, std::bind(&i8237_DMA::IOWriteMode, this, 4, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0C, this, std::bind(&i8237_DMA::IOWriteFlipFlopReset, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xD8, this, std::bind(&i8237_DMA::IOWriteFlipFlopReset, this, 4, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0D, this, std::bind(&i8237_DMA::IOWriteMasterReset, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xDA, this, std::bind(&i8237_DMA::IOWriteMasterReset, this, 4, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0x0E, this, std::bind(&i8237_DMA::IOWriteMaskReset, this, 0, std::placeholders::_2));
  m_bus->ConnectIOPortWrite(0xDC, this, std::bind(&i8237_DMA::IOWriteMaskReset, this, 4, std::placeholders::_2));

  // Unused page registers, but some bioses probe them
  m_bus->ConnectIOPortReadToPointer(0x80, this, &m_unused_page_registers[0]);
  m_bus->ConnectIOPortWriteToPointer(0x80, this, &m_unused_page_registers[0]);
  m_bus->ConnectIOPortReadToPointer(0x84, this, &m_unused_page_registers[1]);
  m_bus->ConnectIOPortWriteToPointer(0x84, this, &m_unused_page_registers[1]);
  m_bus->ConnectIOPortReadToPointer(0x85, this, &m_unused_page_registers[2]);
  m_bus->ConnectIOPortWriteToPointer(0x85, this, &m_unused_page_registers[2]);
  m_bus->ConnectIOPortReadToPointer(0x86, this, &m_unused_page_registers[3]);
  m_bus->ConnectIOPortWriteToPointer(0x86, this, &m_unused_page_registers[3]);
  m_bus->ConnectIOPortReadToPointer(0x88, this, &m_unused_page_registers[4]);
  m_bus->ConnectIOPortWriteToPointer(0x88, this, &m_unused_page_registers[4]);
  m_bus->ConnectIOPortReadToPointer(0x8C, this, &m_unused_page_registers[5]);
  m_bus->ConnectIOPortWriteToPointer(0x8C, this, &m_unused_page_registers[5]);
  m_bus->ConnectIOPortReadToPointer(0x8D, this, &m_unused_page_registers[6]);
  m_bus->ConnectIOPortWriteToPointer(0x8D, this, &m_unused_page_registers[6]);
  m_bus->ConnectIOPortReadToPointer(0x8E, this, &m_unused_page_registers[7]);
  m_bus->ConnectIOPortWriteToPointer(0x8E, this, &m_unused_page_registers[7]);
  m_bus->ConnectIOPortReadToPointer(0x8F, this, &m_unused_page_registers[8]);
  m_bus->ConnectIOPortWriteToPointer(0x8F, this, &m_unused_page_registers[8]);
}

bool i8237_DMA::HasActiveTransfer() const
{
  for (u32 i = 0; i < NUM_CHANNELS; i++)
  {
    const Channel* channel = &m_channels[i];
    if (channel->IsActive())
      return true;
  }

  return false;
}

void i8237_DMA::RescheduleTickEvent()
{
  // Work out the downcount.
  // HACK: For DMA channel 0 we batch it all together as one transfer.
  CycleCount downcount = std::numeric_limits<CycleCount>::max();
  for (Channel& channel : m_channels)
  {
    if (channel.IsActive())
    {
      if (channel.HasCallbacks())
      {
        downcount = 1;
        break;
      }

      // Batch transfers without callbacks together.
      downcount = std::min(downcount, CycleCount(channel.bytes_remaining));
    }
  }

  if (downcount != std::numeric_limits<CycleCount>::max())
  {
    if (!m_tick_event->IsActive())
      m_tick_event->Queue(downcount);
    else
      m_tick_event->Reschedule(downcount);
  }
  else
  {
    if (m_tick_event->IsActive())
      m_tick_event->Deactivate();
  }
}

void i8237_DMA::Transfer(u32 channel_index, size_t count)
{
  Channel* channel = &m_channels[channel_index];
  bool use_word_transfers = (channel_index >= NUM_CHANNELS_PER_CONTROLLER);

  PhysicalMemoryAddress actual_address;
  u32 actual_bytes_remaining;
  if (use_word_transfers)
  {
    // TODO: Should bytes_remaining be multiplied by two?
    actual_address = (u32(channel->page_address) << 16) | u32(u16(channel->address << 1));
    actual_bytes_remaining = channel->bytes_remaining;
  }
  else
  {
    actual_address = (u32(channel->page_address) << 16) | u32(channel->address);
    actual_bytes_remaining = channel->bytes_remaining;
  }

  //     // HACK: Skip logic when simulating the memory refresh DMA.
  //     if (channel_index == 0)
  //     {
  //     }

  // Transfer the entire block if possible in block mode.
  if (channel->mode == DMAMode_Demand || channel->mode == DMAMode_Block)
    count = size_t(channel->bytes_remaining + 1);

  for (size_t i = 0; i < count && channel->request; i++)
  {
    if (use_word_transfers)
    {
      if (channel->transfer_type == DMATransferType_MemoryToDevice)
      {
        u32 value = ZeroExtend32(m_bus->ReadMemoryWord(actual_address));
        channel->write_callback(IOPortDataSize_16, value, actual_bytes_remaining);
      }
      else if (channel->transfer_type == DMATransferType_DeviceToMemory)
      {
        u32 value = 0;
        channel->read_callback(IOPortDataSize_16, &value, actual_bytes_remaining);
        m_bus->WriteMemoryWord(actual_address, Truncate16(value));
      }
      else if (channel->transfer_type == DMATransferType_Verify)
      {
        u32 value = 0;
        channel->read_callback(IOPortDataSize_16, &value, actual_bytes_remaining);
      }
    }
    else
    {
      if (channel->transfer_type == DMATransferType_MemoryToDevice)
      {
        u8 value = m_bus->ReadMemoryByte(actual_address);
        channel->write_callback(IOPortDataSize_8, value, actual_bytes_remaining);
      }
      else if (channel->transfer_type == DMATransferType_DeviceToMemory)
      {
        u32 value = 0;
        channel->read_callback(IOPortDataSize_8, &value, actual_bytes_remaining);
        m_bus->WriteMemoryByte(actual_address, Truncate8(value));
      }
      else if (channel->transfer_type == DMATransferType_Verify)
      {
        u32 value = 0;
        channel->read_callback(IOPortDataSize_8, &value, actual_bytes_remaining);
      }
    }

    if (!channel->decrement)
    {
      channel->address++;
      if (use_word_transfers)
        actual_address += 2;
      else
        actual_address++;
    }
    else
    {
      channel->address--;
      if (use_word_transfers)
        actual_address -= 2;
      else
        actual_address--;
    }

    if (actual_bytes_remaining > 0)
    {
      channel->bytes_remaining--;
      actual_bytes_remaining--;
    }
    else
    {
      channel->transfer_complete = true;
      if (channel->auto_reset)
      {
        channel->address = channel->start_address;
        channel->bytes_remaining = channel->count;
      }
      else
      {
        break;
      }
    }
  }
}

void i8237_DMA::IOReadStartAddress(u32 channel_index, u8* value)
{
  Channel* channel = &m_channels[channel_index];
  u32 controller_index = channel_index / NUM_CHANNELS_PER_CONTROLLER;

  if (m_flipflops[controller_index])
    *value = ((channel->address >> 8) & 0xFF);
  else
    *value = (channel->address & 0xFF);

  m_flipflops[controller_index] ^= true;
}

void i8237_DMA::IOWriteStartAddress(u32 channel_index, u8 value)
{
  Channel* channel = &m_channels[channel_index];
  u32 controller_index = channel_index / NUM_CHANNELS_PER_CONTROLLER;
  m_tick_event->InvokeEarly();

  u16 bits = u16(value);
  if (m_flipflops[controller_index])
    channel->address = (channel->address & 0x00FF) | (bits << 8);
  else
    channel->address = (channel->address & 0xFF00) | bits;

  channel->start_address = channel->address;
  m_flipflops[controller_index] ^= true;

  Log_DebugPrintf("DMA channel %u start address = 0x%04X", channel_index, channel->start_address);
}

void i8237_DMA::IOReadCount(u32 channel_index, u8* value)
{
  Channel* channel = &m_channels[channel_index];
  u32 controller_index = channel_index / NUM_CHANNELS_PER_CONTROLLER;

  if (m_flipflops[controller_index])
    *value = ((channel->bytes_remaining >> 8) & 0xFF);
  else
    *value = (channel->bytes_remaining & 0xFF);

  m_flipflops[controller_index] ^= true;
}

void i8237_DMA::IOWriteCount(u32 channel_index, u8 value)
{
  Channel* channel = &m_channels[channel_index];
  u32 controller_index = channel_index / NUM_CHANNELS_PER_CONTROLLER;
  m_tick_event->InvokeEarly();

  u16 bits = u16(value);
  if (m_flipflops[controller_index])
    channel->bytes_remaining = (channel->bytes_remaining & 0x00FF) | (bits << 8);
  else
    channel->bytes_remaining = (channel->bytes_remaining & 0xFF00) | bits;

  channel->count = channel->bytes_remaining;
  m_flipflops[controller_index] ^= true;

  Log_DebugPrintf("DMA channel %u count = %u", channel_index, channel->count);
  RescheduleTickEvent();
}

void i8237_DMA::IOReadPageAddress(u32 channel_index, u8* value)
{
  *value = m_channels[channel_index].page_address;
}

void i8237_DMA::IOWritePageAddress(u32 channel_index, u8 value)
{
  Log_DebugPrintf("DMA channel %u page address = %02X", channel_index, value);
  m_tick_event->InvokeEarly();

  m_channels[channel_index].page_address = u8(value & 0xFF);
}

void i8237_DMA::IOReadStatus(u32 base_channel, u8* value)
{
  m_tick_event->InvokeEarly();

  u8 bitmask = 0;
  for (u32 i = 0; i < NUM_CHANNELS_PER_CONTROLLER; i++)
  {
    if (m_channels[base_channel + i].transfer_complete)
      bitmask |= (1 << i);
  }

  *value = bitmask;
}

void i8237_DMA::IOWriteMode(u32 base_channel, u8 value)
{
  m_tick_event->InvokeEarly();

  u32 channel_index = base_channel + ZeroExtend32(value & 0b11);
  DMATransferType transfer_type = DMATransferType((value >> 2) & 0b11);
  bool auto_reset = !!((value >> 4) & 0b1);
  bool decrement = !!((value >> 5) & 0b1);
  DMAMode mode = DMAMode((value >> 6) & 0b11);
  Log_DebugPrintf("DMA channel %u mode = 0x%02X", channel_index, value);

  Channel* channel = &m_channels[channel_index];
  channel->transfer_type = transfer_type;
  channel->auto_reset = auto_reset;
  channel->decrement = decrement;
  channel->mode = mode;

  //     // HACK: Set channel 0 to self-test so that it doesn't do memory reads.
  //     if (channel_index == 0)
  //         channel->transfer_type = DMATransferType_SelfTest;
}

void i8237_DMA::IOWriteSingleMask(u32 base_channel, u8 value)
{
  m_tick_event->InvokeEarly();

  u8 offset = value & 0b11;
  bool on = ((value & 0b100) != 0);
  m_channels[base_channel + offset].masked = on;

  RescheduleTickEvent();
}

void i8237_DMA::IOReadMultiMask(u32 base_channel, u8* value)
{
  u8 bitmask = 0;
  for (u32 i = 0; i < NUM_CHANNELS_PER_CONTROLLER; i++)
  {
    if (m_channels[base_channel + i].masked)
      bitmask |= (1 << i);
  }

  *value = bitmask;
}

void i8237_DMA::IOWriteMultiMask(u32 base_channel, u8 value)
{
  m_tick_event->InvokeEarly();

  for (u32 i = 0; i < NUM_CHANNELS_PER_CONTROLLER; i++)
    m_channels[base_channel + i].masked = ((value & (1 << i)) != 0);

  RescheduleTickEvent();
}

void i8237_DMA::IOWriteFlipFlopReset(u32 base_channel, u8 value)
{
  m_flipflops[base_channel / NUM_CHANNELS_PER_CONTROLLER] = false;
}

void i8237_DMA::IOWriteMasterReset(u32 base_channel, u8 value)
{
  m_tick_event->InvokeEarly();

  for (u32 i = 0; i < NUM_CHANNELS_PER_CONTROLLER; i++)
  {
    Channel* channel = &m_channels[base_channel + i];
    channel->decrement = false;
    channel->auto_reset = false;
    channel->transfer_type = DMATransferType_Verify;
    channel->mode = DMAMode_Single;
    channel->masked = true;
  }

  m_flipflops[base_channel / NUM_CHANNELS_PER_CONTROLLER] = false;
  RescheduleTickEvent();
}

void i8237_DMA::IOWriteMaskReset(u32 base_channel, u8 value)
{
  m_tick_event->InvokeEarly();

  for (u32 i = 0; i < NUM_CHANNELS_PER_CONTROLLER; i++)
  {
    Channel* channel = &m_channels[base_channel + i];
    channel->masked = false;
  }

  RescheduleTickEvent();
}

} // namespace HW
