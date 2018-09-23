#include "ata_cdrom.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "hdc.h"
#include "pce/system.h"
Log_SetChannel(HW::ATACDROM);

namespace HW {

DEFINE_OBJECT_TYPE_INFO(ATACDROM);
DEFINE_GENERIC_COMPONENT_FACTORY(ATACDROM);
BEGIN_OBJECT_PROPERTY_MAP(ATACDROM)
PROPERTY_TABLE_MEMBER_STRING("VendorID", 0, offsetof(ATACDROM, m_cdrom.m_vendor_id_string), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("ModelID", 0, offsetof(ATACDROM, m_cdrom.m_model_id_string), nullptr, 0)
PROPERTY_TABLE_MEMBER_STRING("FirmwareVersion", 0, offsetof(ATACDROM, m_cdrom.m_firmware_version_string), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

ATACDROM::ATACDROM(const String& identifier, u32 ata_channel /* = 0 */, u32 ata_device /* = 0 */,
                   const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, ata_channel, ata_device, type_info), m_cdrom(identifier)
{
}

bool ATACDROM::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  if (!m_cdrom.Initialize(system, bus))
    return false;

  m_command_event = m_system->GetTimingManager()->CreateMicrosecondIntervalEvent(
    "ATAPI Command", 1, std::bind(&ATACDROM::ExecutePendingCommand, this), false);
  m_cdrom.SetInterruptCallback(std::bind(&ATACDROM::InterruptCallback, this));

  return true;
}

void ATACDROM::Reset()
{
  BaseClass::Reset();
  DoReset(true);
}

bool ATACDROM::LoadState(BinaryReader& reader)
{
  if (!BaseClass::LoadState(reader) || !m_cdrom.LoadState(reader))
    return false;

  m_current_command = reader.ReadUInt16();
  return !reader.GetErrorState();
}

bool ATACDROM::SaveState(BinaryWriter& writer)
{
  if (!BaseClass::SaveState(writer) || !m_cdrom.SaveState(writer))
    return false;

  writer.WriteUInt16(m_current_command);
  return !writer.InErrorState();
}

void ATACDROM::WriteCommandRegister(u8 value)
{
  Log_TracePrintf("ATA drive %u/%u command register <- 0x%02X", m_ata_channel_number, m_ata_drive_number, value);

  // Ignore writes to the command register when busy.
  if (!m_registers.status.IsReady() || HasPendingCommand())
  {
    Log_WarningPrintf("ATA drive %u/%u is not ready", m_ata_channel_number, m_ata_drive_number);
    return;
  }

  // We're busy now, and can't accept other commands.
  m_registers.status.SetBusy();

  // Determine how long the command will take to execute.
  const CycleCount cycles = CalculateCommandTime(value);
  m_current_command = ZeroExtend16(value);
  if (cycles > 0)
  {
    Log_DevPrintf("Queueing ATAPI command 0x%02X in %u us", value, unsigned(cycles));
    m_command_event->Queue(cycles);
  }
  else
  {
    // Some commands, e.g. PACKET, we execute immediately.
    ExecutePendingCommand();
  }
}

void ATACDROM::DoReset(bool is_hardware_reset)
{
  BaseClass::DoReset(is_hardware_reset);

  m_current_command = INVALID_COMMAND;
  m_command_event->SetActive(false);

  // Set the ATAPI signature.
  SetSignature();
}

void ATACDROM::SetSignature()
{
  m_registers.sector_count = 1;
  m_registers.sector_number = 1;
  m_registers.cylinder_low = 0x14;
  m_registers.cylinder_high = 0xEB;
}

void ATACDROM::SetInterruptReason(bool is_command, bool data_from_device, bool release)
{
  // Bit 0 - CoD, Bit 1 - I/O, Bit 2- RELEASE
  m_registers.sector_count =
    BoolToUInt8(is_command) | (BoolToUInt8(data_from_device) << 1) | (BoolToUInt8(release) << 2);
}

void ATACDROM::CompleteCommand(bool raise_interrupt /* = true */)
{
  ResetBuffer();
  m_current_command = INVALID_COMMAND;
  m_registers.status.SetReady();
  m_registers.error = static_cast<ATA_ERR>(0x00);

  if (raise_interrupt)
    RaiseInterrupt();
}

void ATACDROM::AbortCommand(u8 error /* = ATA_ERR_ABRT */)
{
  ResetBuffer();
  m_current_command = INVALID_COMMAND;
  m_registers.status.SetError(false);
  m_registers.error = error;
  RaiseInterrupt();
}

CycleCount ATACDROM::CalculateCommandTime(u8 command) const
{
  // Respond instantly to PACKET.
  if (command == ATAPI_CMD_PACKET)
    return 0;
  else
    return 1;
}

bool ATACDROM::HasPendingCommand() const
{
  return (m_current_command != INVALID_COMMAND);
}

void ATACDROM::ExecutePendingCommand()
{
  const u8 command = Truncate8(m_current_command);
  m_command_event->SetActive(false);

  Log_TracePrintf("Executing ATAPI command 0x%02X on drive %u/%u", command, m_ata_channel_number, m_ata_drive_number);

  switch (command)
  {
    case ATA_CMD_IDENTIFY:
      HandleATAIdentify();
      break;

    case ATA_CMD_IDENTIFY_PACKET:
      HandleATAPIIdentify();
      break;

    case ATA_CMD_DEVICE_RESET:
      HandleATADeviceReset();
      break;

    case ATAPI_CMD_PACKET:
      HandleATAPIPacket();
      break;

    default:
    {
      // Unknown command - abort is correct?
      Log_ErrorPrintf("Unknown ATAPI command 0x%02X", ZeroExtend32(command));
      AbortCommand();
    }
    break;
  }
}

void ATACDROM::HandleATADeviceReset()
{
  Log_DevPrintf("ATAPI device reset");
  DoReset(false);
}

void ATACDROM::HandleATAIdentify()
{
  Log_DevPrintf("ATAPI identify");

  // Executing normal identify on a packet device sets the signature and aborts.
  SetSignature();
  AbortCommand(ATA_ERR_ABRT);
}

void ATACDROM::HandleATAPIIdentify()
{
  Log_DevPrintf("ATAPI identify packet device");

  ATA_IDENTIFY_RESPONSE response = {};
  response.flags |= (2 << 5);
  response.flags |= (1 << 7);  // removable
  response.flags |= (5 << 8);  // cdrom
  response.flags |= (2 << 14); // atapi device
  PutIdentifyString(response.serial_number, sizeof(response.serial_number), "DERP123");
  PutIdentifyString(response.firmware_revision, sizeof(response.firmware_revision), "HURR101");
  response.dword_io_supported = 1;
  response.support = (1 << 9);
  response.pio_timing_mode = 0x200;
  PutIdentifyString(response.model, sizeof(response.model), m_cdrom.GetModelIDString());
  for (size_t i = 0; i < countof(response.pio_cycle_time); i++)
    response.pio_cycle_time[i] = 120;
  response.word_80 = (1 << 4);
  response.minor_version_number = 0x0017;
  response.word_82 = (1 << 14) | (1 << 9) | (1 << 4);

  // 512 bytes total
  SetupBuffer(sizeof(response), false, false);
  std::memcpy(m_buffer.data.data(), &response, sizeof(response));
  BufferReady(true);

  // Signature is reset
  SetSignature();
}

void ATACDROM::HandleATAPIPacket()
{
  if (m_registers.feature_select != 0)
  {
    Log_ErrorPrintf("ATAPI DMA requested");
    AbortCommand();
    return;
  }
  else if (m_cdrom.IsBusy())
  {
    Log_WarningPrintf("ATAPI device busy on packet command");
    AbortCommand();
    return;
  }

  // Setup the buffer for receiving the packet from the host.
  SetupBuffer(m_packet_size, true, false);
  SetInterruptReason(true, false, false);

  // No interrupt is raised.
  BufferReady(false);
}

void ATACDROM::OnBufferEnd()
{
  // If we're here and it's a write, it means the packet was written by the host.
  if (m_buffer.is_write)
  {
    Log_TracePrintf("ATAPI PACKET received from host of %u bytes", m_buffer.size);
    m_buffer.valid = false;

    // Command now in progress.
    m_registers.status.SetBusy();

    // Send it to the device.
    if (!m_cdrom.WriteCommandBuffer(m_buffer.data.data(), m_buffer.size))
    {
      // Invalid command?
      m_cdrom.ClearCommandBuffer();
      AbortCommand();
    }

    // The device will call back to us later.
    return;
  }

  // If we're here, it means we read the entire response from the device.
  // Any remaining sectors in the read?
  if (m_cdrom.GetRemainingSectors() > 0)
  {
    m_registers.status.SetBusy();
    if (!m_cdrom.TransferNextSector())
      AbortCommand();

    return;
  }

  // Command is fully complete now.
  SetInterruptReason(true, true, false);
  CompleteCommand(true);
}

void ATACDROM::InterruptCallback()
{
  // Empty out the buffer (since we used it when writing).
  ResetBuffer();

  // Did this command fail?
  if (m_cdrom.HasError())
  {
    SetInterruptReason(true, true, false);
    AbortCommand(m_cdrom.GetSenseKey() << 4);
    m_cdrom.ClearErrorFlag();
    return;
  }

  // Any response bytes?
  if (m_cdrom.GetDataResponseSize() == 0)
  {
    SetInterruptReason(true, true, false);
    CompleteCommand(true);
    return;
  }

  // Set up the buffer.
  SetupBuffer(Truncate16(m_cdrom.GetDataResponseSize()), false, false);
  std::memcpy(m_buffer.data.data(), m_cdrom.GetDataBuffer(), m_buffer.size);

  // Update the command block with the response size.
  m_registers.cylinder_low = Truncate8(m_cdrom.GetDataResponseSize());
  m_registers.cylinder_high = Truncate8(m_cdrom.GetDataResponseSize() >> 8);
  m_cdrom.ClearDataBuffer();

  // Notify host.
  SetInterruptReason(false, true, false);
  BufferReady(true);

  // RDY bit is cleared if there is further packets remaining.
  m_registers.status.ready = (m_cdrom.GetRemainingSectors() == 0);
}

} // namespace HW
