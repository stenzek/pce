#include "pci_ide.h"
#include "../bus.h"
#include "../interrupt_controller.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "ata_device.h"
#include <cinttypes>
Log_SetChannel(PCIIDE);

// TODO: Implement native mode.

namespace HW {
DEFINE_OBJECT_TYPE_INFO(PCIIDE);
DEFINE_GENERIC_COMPONENT_FACTORY(PCIIDE);
BEGIN_OBJECT_PROPERTY_MAP(PCIIDE)
END_OBJECT_PROPERTY_MAP()

PCIIDE::PCIIDE(const String& identifier, Model model /* = Model::PIIX */,
               const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, 2, type_info), PCIDevice(this, 1), m_model(model)
{
  switch (model)
  {
    case Model::PIIX:
      InitPCIID(0, 0x8086, 0x1230);
      InitPCIClass(0, 0x01, 0x01, 0x80, 0x00);
      break;

    case Model::PIIX3:
      InitPCIID(0, 0x8086, 0x7010);
      InitPCIClass(0, 0x01, 0x01, 0x80, 0x00);
      break;
  }

  InitPCIMemoryRegion(0, PCIDevice::MemoryRegion_BAR4, 0, 16, true);
}

PCIIDE::~PCIIDE() = default;

bool PCIIDE::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus) || !PCIDevice::Initialize())
    return false;

  return true;
}

void PCIIDE::Reset()
{
  BaseClass::Reset();
  PCIDevice::Reset();
}

bool PCIIDE::LoadState(BinaryReader& reader)
{
  if (!BaseClass::LoadState(reader) || !PCIDevice::LoadState(reader))
    return false;

  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  for (u32 i = 0; i < MAX_CHANNELS; i++)
  {
    DMAState& ds = m_dma_state[i];
    ds.command.bits = reader.ReadUInt8();
    ds.status.bits = reader.ReadUInt8();
    ds.prdt_address = reader.ReadUInt32();
    ds.active_drive_number = reader.ReadUInt32();
    ds.next_prdt_entry_index = reader.ReadUInt32();
    ds.current_physical_address = reader.ReadUInt32();
    ds.remaining_byte_count = reader.ReadUInt32();
    ds.eot = reader.ReadBool();
  }

  return !reader.GetErrorState();
}

bool PCIIDE::SaveState(BinaryWriter& writer)
{
  if (!BaseClass::SaveState(writer) || !PCIDevice::SaveState(writer))
    return false;

  writer.WriteUInt32(SERIALIZATION_ID);

  for (u32 i = 0; i < MAX_CHANNELS; i++)
  {
    DMAState& ds = m_dma_state[i];
    writer.WriteUInt8(ds.command.bits);
    writer.WriteUInt8(ds.status.bits);
    writer.WriteUInt32(ds.prdt_address);
    writer.WriteUInt32(ds.active_drive_number);
    writer.WriteUInt32(ds.next_prdt_entry_index);
    writer.WriteUInt32(ds.current_physical_address);
    writer.WriteUInt32(ds.remaining_byte_count);
    writer.WriteBool(ds.eot);
  }

  return !writer.InErrorState();
}

bool PCIIDE::IsChannelEnabled(u32 channel) const
{
  return (GetConfigSpaceWord(0, 0x40 + (static_cast<u8>(channel) * 2)) & 0x8000) != 0;
}

void PCIIDE::ConnectIOPorts(Bus* bus)
{
  BaseClass* base_class_ptr = this;
  bus->DisconnectIOPorts(base_class_ptr);

  // TODO: Native mode
  if (IsChannelEnabled(0))
    BaseClass::ConnectIOPorts(bus, 0, 0x01F0, 0x03F6, 14);
  if (IsChannelEnabled(1))
    BaseClass::ConnectIOPorts(bus, 1, 0x0170, 0x0376, 15);

  if (m_config_space[0].header.command.enable_io_space)
  {
    const u16 bm_base = static_cast<u16>(GetMemoryRegionBaseAddress(0, PCIDevice::MemoryRegion_BAR4));
    for (u8 channel = 0; channel < 2; channel++)
    {
      bus->ConnectIOPortRead(bm_base + (channel * 8) + 0, base_class_ptr,
                             std::bind(&PCIIDE::IOReadBusMasterCommandRegister, this, channel));
      bus->ConnectIOPortWrite(
        bm_base + (channel * 8) + 0, base_class_ptr,
        std::bind(&PCIIDE::IOWriteBusMasterCommandRegister, this, channel, std::placeholders::_2));
      bus->ConnectIOPortRead(bm_base + (channel * 8) + 2, base_class_ptr,
                             std::bind(&PCIIDE::IOReadBusMasterStatusRegister, this, channel));
      bus->ConnectIOPortWrite(bm_base + (channel * 8) + 2, base_class_ptr,
                              std::bind(&PCIIDE::IOWriteBusMasterStatusRegister, this, channel, std::placeholders::_2));
      for (u8 offset = 0; offset < 4; offset++)
      {
        bus->ConnectIOPortRead(bm_base + (channel * 8) + 4 + offset, base_class_ptr,
                               std::bind(&PCIIDE::IOReadBusMasterPRDTAddress, this, channel, offset));
        bus->ConnectIOPortWrite(
          bm_base + (channel * 8) + 4 + offset, base_class_ptr,
          std::bind(&PCIIDE::IOWriteBusMasterPRDTAddress, this, channel, offset, std::placeholders::_2));
      }
    }
  }
}

void PCIIDE::DoReset(u32 channel, bool hardware_reset)
{
  for (u32 i = 0; i < MAX_CHANNELS; i++)
  {
    DMAState& ds = m_dma_state[i];
    ds.command.bits = 0;
    ds.status.bits = 0;
    ds.prdt_address = 0;
    ds.active_drive_number = DEVICES_PER_CHANNEL;
    ds.next_prdt_entry_index = INVALID_PRDT_INDEX;
    ds.current_physical_address = 0;
    ds.remaining_byte_count = 0;
    ds.eot = true;

    if (hardware_reset)
    {
      for (u32 j = 0; j < 2; j++)
      {
        if (HDC::m_channels[i].devices[j] && HDC::m_channels[i].devices[j]->SupportsDMA())
          ds.status.user = ds.status.user | (0x01 << j);
      }
    }
  }

  BaseClass::DoReset(channel, hardware_reset);
}

void PCIIDE::ResetConfigSpace(u8 function)
{
  PCIDevice::ResetConfigSpace(function);
  if (function > 0)
    return;

  m_config_space[0].header.command.enable_io_space = true;
  m_config_space[0].header.command.enable_bus_master = true;
  m_config_space[0].words[0x20] = 0x8000; // IDE0 Enabled
  m_config_space[0].words[0x21] = 0x8000; // IDE1 Enabled

  ConnectIOPorts(BaseClass::GetBus());
}

void PCIIDE::OnCommandRegisterChanged(u8 function)
{
  ConnectIOPorts(BaseClass::m_bus);
}

void PCIIDE::OnMemoryRegionChanged(u8 function, MemoryRegion region, bool active)
{
  ConnectIOPorts(BaseClass::m_bus);
}

u8 PCIIDE::IOReadBusMasterCommandRegister(u8 channel)
{
  return m_dma_state[channel].command.bits;
}

u8 PCIIDE::IOReadBusMasterStatusRegister(u8 channel)
{
  return m_dma_state[channel].status.bits;
}

u8 PCIIDE::IOReadBusMasterPRDTAddress(u8 channel, u8 offset)
{
  return Truncate8(m_dma_state[channel].prdt_address >> (offset * 8));
}

void PCIIDE::IOWriteBusMasterCommandRegister(u8 channel, u8 value)
{
  DMAState& ds = m_dma_state[channel];
  DMAState::CommandRegister new_value = {value};

  // TODO: Should changing R/W during a transfer be allowed?
  ds.command.is_write = new_value.is_write.GetValue();

  if (new_value.transfer_start != ds.command.transfer_start.GetValue())
  {
    ds.command.transfer_start = new_value.transfer_start.GetValue();
    OnDMAStateChanged(channel);
  }
}

void PCIIDE::IOWriteBusMasterStatusRegister(u8 channel, u8 value)
{
  DMAState& ds = m_dma_state[channel];
  DMAState::StatusRegister new_value = {value};

  // Writing 1s to these bits clears the flags.
  if (new_value.transfer_failed)
    ds.status.transfer_failed = false;
  if (new_value.irq_requested)
    ds.status.irq_requested = false;

  ds.status.user = new_value.user.GetValue();
}

void PCIIDE::IOWriteBusMasterPRDTAddress(u8 channel, u8 offset, u8 value)
{
  m_dma_state[channel].prdt_address &= ~u32(u32(0xFF) << (offset * 8));
  m_dma_state[channel].prdt_address |= ZeroExtend32(value) << (offset * 8);
}

u8 PCIIDE::ReadConfigSpace(u8 function, u8 offset)
{
  u8 val = PCIDevice::ReadConfigSpace(function, offset);
  Log_DebugPrintf("PCIIDE config read reg %02x: %02x", offset, val);
  return val;
}

void PCIIDE::WriteConfigSpace(u8 function, u8 offset, u8 value)
{
  switch (offset)
  {
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    {
      if (m_config_space[function].bytes[offset] == value)
        return;

      m_config_space[function].bytes[offset] = value;
      const u8 channel = (offset & 0x03) >> 1;
      Log_DevPrintf("IDE channel %u %s", channel, IsChannelEnabled(channel) ? "enabled" : "disabled");
      ConnectIOPorts(BaseClass::m_bus);
    }
    break;
  }
  Log_DebugPrintf("PCIIDE config reg %02x <- %02x", offset, value);
  PCIDevice::WriteConfigSpace(function, offset, value);
}

void PCIIDE::UpdateHostInterruptLine(u32 channel)
{
  auto& ds = m_dma_state[channel];
  auto& chan = m_channels[channel];
  const bool irq_state =
    (chan.device_interrupt_lines[0] | chan.device_interrupt_lines[1]) & (ds.remaining_byte_count == 0);
  ds.status.irq_requested = irq_state;
  BaseClass::m_interrupt_controller->SetInterruptState(chan.irq, irq_state);
}

bool PCIIDE::SupportsDMA() const
{
  return true;
}

bool PCIIDE::IsDMARequested(u32 channel) const
{
  return m_dma_state[channel].active_drive_number != DEVICES_PER_CHANNEL;
}

void PCIIDE::SetDMARequest(u32 channel, u32 drive, bool request)
{
  Log_DebugPrintf("DMARQ=%s for channel %u drive %u", request ? "active" : "inactive", channel, drive);

  DMAState& ds = m_dma_state[channel];
  if (request)
  {
    if (ds.active_drive_number != drive && ds.active_drive_number != DEVICES_PER_CHANNEL)
    {
      Log_WarningPrintf("DMARQ on already-active channel");
      return;
    }

    ds.active_drive_number = drive;
  }
  else
  {
    if (ds.active_drive_number != drive)
      return;

    ds.active_drive_number = DEVICES_PER_CHANNEL;
  }

  // ACK it if the DMA has been started already.
  if (ds.command.transfer_start)
    BaseClass::m_channels[channel].devices[drive]->SetDMACK(request);
}

void PCIIDE::OnDMAStateChanged(u32 channel)
{
  DMAState& ds = m_dma_state[channel];
  if (!ds.command.transfer_start)
  {
    // Transfer aborted.
    if (ds.active_drive_number != DEVICES_PER_CHANNEL)
      HDC::m_channels[channel].devices[ds.active_drive_number]->SetDMACK(false);

    ds.status.bus_dma_mode = false;
  }
  else
  {
    // TODO: Does this happen before or after the device requests DMA?
    ds.next_prdt_entry_index = 0;
    ds.remaining_byte_count = 0;
    ds.eot = false;

    // Transfer started.
    if (ds.active_drive_number != DEVICES_PER_CHANNEL)
      HDC::m_channels[channel].devices[ds.active_drive_number]->SetDMACK(true);

    ds.status.bus_dma_mode = true;
  }
}

void PCIIDE::ReadNextPRDT(u32 channel)
{
#pragma pack(push, 1)
  union PRDT_ENTRY
  {
    struct
    {
      u32 physical_base_address; // bit 0 is ignored
      union
      {
        BitField<u32, u32, 0, 16> byte_count; // bit 0 is ignored
        BitField<u32, bool, 31, 1> eot;
      };
    };
    u32 bits32[2];
    u64 bits64;
  };
#pragma pack(pop)

  DMAState& ds = m_dma_state[channel];
  if (ds.eot)
    return;

  const PhysicalMemoryAddress table_address = ds.prdt_address + (ds.next_prdt_entry_index * sizeof(PRDT_ENTRY));
  DebugAssert((table_address & 0x03) == 0);
  PRDT_ENTRY entry;
  entry.bits64 = BaseClass::m_bus->ReadMemoryQWord(table_address & UINT32_C(0xFFFFFFFC));
  Log_DebugPrintf("Channel %u PRDT %u: %" PRIX64 " (base address 0x%08X, size %u, eot %s)", channel,
                  ds.next_prdt_entry_index, entry.bits64, entry.physical_base_address, entry.byte_count.GetValue(),
                  entry.eot ? "true" : "false");
  ds.next_prdt_entry_index++;

  ds.current_physical_address = entry.physical_base_address & UINT32_C(0xFFFFFFFE);
  ds.remaining_byte_count = entry.byte_count & UINT32_C(0xFFFE);
  ds.eot = entry.eot;
}

u32 PCIIDE::DMATransfer(u32 channel, u32 drive, bool is_write, void* data, u32 size)
{
  DMAState& ds = m_dma_state[channel];

  // Mismatched read/write?
  if (!ds.command.transfer_start || is_write != ds.command.is_write)
  {
    Log_WarningPrintf("Mismatched read/write bits");
    return 0;
  }

  byte* data_ptr = static_cast<byte*>(data);
  u32 remaining = size;
  while (remaining > 0)
  {
    if (ds.remaining_byte_count == 0)
      ReadNextPRDT(channel);

    u32 transfer_size = std::min(remaining, ds.remaining_byte_count);
    if (transfer_size == 0)
    {
      // End of PRDT.
      break;
    }

    Log_DebugPrintf("DMA %s %u bytes at 0x%08X for %u/%u", is_write ? "write" : "read", transfer_size,
                    ds.current_physical_address, channel, drive);

    if (is_write)
      BaseClass::m_bus->WriteMemoryBlock(ds.current_physical_address, transfer_size, data_ptr);
    else
      BaseClass::m_bus->ReadMemoryBlock(ds.current_physical_address, transfer_size, data_ptr);

    ds.current_physical_address += transfer_size;
    ds.remaining_byte_count -= transfer_size;
    data_ptr += transfer_size;
    remaining -= transfer_size;

    // TODO: Stall CPU.
  }

  if (ds.eot)
    ds.status.bus_dma_mode = false;

  UpdateHostInterruptLine(channel);
  return size - remaining;
}

} // namespace HW