#include "pce/hw/fdc.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "pce/bus.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cstring>
Log_SetChannel(HW::FDC);

// Data rates in kb/s.
static const uint32 data_rates[4] = {500, 300, 250, 1000};

namespace HW {
DEFINE_OBJECT_TYPE_INFO(FDC);
BEGIN_OBJECT_PROPERTY_MAP(FDC)
END_OBJECT_PROPERTY_MAP()

FDC::FDC(const String& identifier, Model model /* = Model_8272 */, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_clock("Floppy Controller", CLOCK_FREQUENCY), m_model(model)
{
}

FDC::~FDC() = default;

bool FDC::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus))
    return false;

  m_clock.SetManager(system->GetTimingManager());
  m_dma = system->GetComponentByType<DMAController>();
  if (!m_dma)
  {
    Log_ErrorPrintf("Failed to find DMA controller for FDC");
    return false;
  }
  m_interrupt_controller = m_system->GetComponentByType<InterruptController>();
  if (!m_interrupt_controller)
  {
    Log_ErrorPrintf("Failed to locate interrupt controller.");
    return false;
  }

  ConnectIOPorts(bus);
  m_command_event = m_clock.NewEvent("Floppy Command", 1, std::bind(&FDC::EndCommand, this), false);
  return true;
}

void FDC::Reset()
{
  Reset(false);
}

void FDC::Reset(bool software_reset)
{
  m_current_command.Clear();
  m_command_event->SetActive(false);
  if (m_current_transfer.active)
  {
    m_current_transfer.Clear();
    m_dma->SetDMAState(DMA_CHANNEL, false);
  }

  m_DOR.bits = 0;
  m_DOR.ndmagate = true;
  m_DOR.nreset = true;
  m_MSR.bits = 0;
  m_MSR.request_for_master = true;
  TransitionToCommandPhase();
  LowerInterrupt();

  if (!software_reset || !m_specify_lock)
  {
    m_data_rate_index = 0;
    m_step_rate_time = 0;
    m_head_load_time = 0;
    m_head_unload_time = 0;
    m_pio_mode = false;
    m_fifo_threshold = 1;
    m_implied_seeks = false;
    m_fifo_disabled = true;
    m_polling_disabled = false;
    m_precompensation_start_track = 0;
    m_perpendicular_mode = 0;
  }

  // Reset disk states
  for (DriveState& drive : m_drives)
  {
    drive.data_was_read = false;
    drive.data_was_written = false;
    drive.direction = false;
    drive.step_latch = false;
  }

  if (software_reset)
  {
    // Require sense interrupt for software reset.
    RaiseInterrupt();
    m_reset_sense_interrupt_count = 4;
  }
  else
  {
    m_specify_lock = false;
  }
}

bool FDC::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    DriveState* drive = &m_drives[i];
    const bool present = reader.ReadBool();
    if (present && !drive->floppy)
    {
      Log_ErrorPrintf("Drive state mismatch");
      return false;
    }

    reader.SafeReadUInt32(&drive->current_cylinder);
    reader.SafeReadUInt32(&drive->current_head);
    reader.SafeReadUInt32(&drive->current_sector);
    reader.SafeReadUInt32(&drive->current_lba);
    reader.SafeReadBool(&drive->data_was_read);
    reader.SafeReadBool(&drive->data_was_written);
    reader.SafeReadBool(&drive->step_latch);
    reader.SafeReadBool(&drive->direction);
  }

  reader.SafeReadUInt8(&m_DOR.bits);
  reader.SafeReadUInt8(&m_MSR.bits);

  reader.SafeReadUInt8(&m_data_rate_index);
  reader.SafeReadBool(&m_interrupt_pending);
  reader.SafeReadBool(&m_disk_change_flag);
  reader.SafeReadBool(&m_specify_lock);

  reader.SafeReadUInt8(&m_reset_sense_interrupt_count);
  reader.SafeReadInt64(&m_reset_begin_time);

  reader.SafeReadBytes(m_fifo, sizeof(m_fifo));
  reader.SafeReadUInt32(&m_fifo_result_size);
  reader.SafeReadUInt32(&m_fifo_result_position);

  reader.SafeReadUInt8(&m_step_rate_time);
  reader.SafeReadUInt8(&m_head_load_time);
  reader.SafeReadUInt8(&m_head_unload_time);
  reader.SafeReadBool(&m_pio_mode);
  reader.SafeReadBool(&m_implied_seeks);
  reader.SafeReadBool(&m_polling_disabled);
  reader.SafeReadBool(&m_fifo_disabled);
  reader.SafeReadUInt8(&m_fifo_threshold);
  reader.SafeReadUInt8(&m_precompensation_start_track);
  reader.SafeReadUInt8(&m_perpendicular_mode);

  reader.SafeReadBytes(m_current_command.buf, sizeof(m_current_command.buf));
  reader.SafeReadUInt8(&m_current_command.command_length);

  reader.SafeReadBool(&m_current_transfer.active);
  reader.SafeReadBool(&m_current_transfer.multi_track);
  reader.SafeReadUInt8(&m_current_transfer.drive);
  reader.SafeReadUInt32(&m_current_transfer.bytes_per_sector);
  reader.SafeReadUInt32(&m_current_transfer.sectors_per_track);
  reader.SafeReadUInt32(&m_current_transfer.sector_offset);
  reader.SafeReadBytes(m_current_transfer.sector_buffer, sizeof(m_current_transfer.sector_buffer));

  reader.SafeReadUInt8(&m_st0);
  reader.SafeReadUInt8(&m_st1);
  reader.SafeReadUInt8(&m_st2);

  return !reader.GetErrorState();
}

bool FDC::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    DriveState* drive = &m_drives[i];
    writer.SafeWriteBool(drive->floppy != nullptr);
    writer.SafeWriteUInt32(drive->current_cylinder);
    writer.SafeWriteUInt32(drive->current_head);
    writer.SafeWriteUInt32(drive->current_sector);
    writer.SafeWriteUInt32(drive->current_lba);
    writer.SafeWriteBool(drive->data_was_read);
    writer.SafeWriteBool(drive->data_was_written);
    writer.SafeWriteBool(drive->step_latch);
    writer.SafeWriteBool(drive->direction);
  }

  writer.SafeWriteUInt8(m_DOR.bits);
  writer.SafeWriteUInt8(m_MSR.bits);
  writer.SafeWriteUInt8(m_data_rate_index);
  writer.SafeWriteBool(m_interrupt_pending);
  writer.SafeWriteBool(m_disk_change_flag);
  writer.SafeWriteBool(m_specify_lock);

  writer.SafeWriteUInt8(m_reset_sense_interrupt_count);
  writer.SafeWriteInt64(m_reset_begin_time);

  writer.SafeWriteBytes(m_fifo, sizeof(m_fifo));
  writer.SafeWriteUInt32(m_fifo_result_size);
  writer.SafeWriteUInt32(m_fifo_result_position);

  writer.SafeWriteUInt8(m_step_rate_time);
  writer.SafeWriteUInt8(m_head_load_time);
  writer.SafeWriteUInt8(m_head_unload_time);
  writer.SafeWriteBool(m_pio_mode);
  writer.SafeWriteBool(m_implied_seeks);
  writer.SafeWriteBool(m_polling_disabled);
  writer.SafeWriteBool(m_fifo_disabled);
  writer.SafeWriteUInt8(m_fifo_threshold);
  writer.SafeWriteUInt8(m_precompensation_start_track);
  writer.SafeWriteUInt8(m_perpendicular_mode);

  writer.SafeWriteBytes(m_current_command.buf, sizeof(m_current_command.buf));
  writer.SafeWriteUInt8(m_current_command.command_length);

  writer.SafeWriteBool(m_current_transfer.active);
  writer.SafeWriteBool(m_current_transfer.multi_track);
  writer.SafeWriteUInt8(m_current_transfer.drive);
  writer.SafeWriteUInt32(m_current_transfer.bytes_per_sector);
  writer.SafeWriteUInt32(m_current_transfer.sectors_per_track);
  writer.SafeWriteUInt32(m_current_transfer.sector_offset);
  writer.SafeWriteBytes(m_current_transfer.sector_buffer, sizeof(m_current_transfer.sector_buffer));
  writer.SafeWriteUInt8(m_st0);
  writer.SafeWriteUInt8(m_st1);
  writer.SafeWriteUInt8(m_st2);

  return true;
}

Floppy::DriveType FDC::GetDriveType_(uint32 drive)
{
  return (drive < MAX_DRIVES && m_drives[drive].floppy) ? m_drives[drive].floppy->GetDriveType_() :
                                                          Floppy::DriveType_None;
}

bool FDC::IsDrivePresent(uint32 drive) const
{
  return (drive < MAX_DRIVES && m_drives[drive].floppy);
}

bool FDC::IsDiskPresent(uint32 drive) const
{
  return (drive < MAX_DRIVES && m_drives[drive].floppy) ? m_drives[drive].floppy->GetDiskType() : Floppy::DiskType_None;
}

uint32 FDC::GetDriveCount() const
{
  uint32 count = 0;
  for (uint32 i = 0; i < MAX_DRIVES; i++)
  {
    if (IsDrivePresent(i))
      count++;
  }
  return count;
}

bool FDC::AttachDrive(uint32 number, Floppy* drive)
{
  if (number >= MAX_DRIVES || m_drives[number].floppy)
    return false;

  m_drives[number].floppy = drive;
  return true;
}

void FDC::ClearFIFO()
{
  m_fifo_result_position = 0;
  m_fifo_result_size = 0;
}

void FDC::WriteToFIFO(uint8 value)
{
  Assert(m_fifo_result_size < FIFO_SIZE);
  m_fifo[m_fifo_result_size++] = value;
}

void FDC::SetActivity(u32 drive_number, bool writing /* = false */)
{
  m_MSR.bits &= 0x0F;
  m_MSR.bits |= u8(0x01) << drive_number;
  for (u32 i = 0; i < MAX_DRIVES; i++)
  {
    if (m_drives[i].floppy)
    {
      if (i == drive_number)
        m_drives[i].floppy->SetActivity(writing);
      else
        m_drives[i].floppy->ClearActivity();
    }
  }
}

void FDC::ClearActivity()
{
  m_MSR.bits &= 0xF0;
  for (u32 i = 0; i < MAX_DRIVES; i++)
  {
    if (m_drives[i].floppy)
      m_drives[i].floppy->ClearActivity();
  }
}

bool FDC::SeekDrive(uint32 drive, uint32 cylinder, uint32 head, uint32 sector)
{
  DriveState* state = &m_drives[drive];
  Floppy* floppy = state->floppy;
  if (cylinder >= floppy->GetNumTracks() || head >= floppy->GetNumHeads() || sector < 1 ||
      sector > floppy->GetSectorsPerTrack())
    return false;

  state->current_cylinder = cylinder;
  state->current_head = head;
  state->current_sector = sector;
  state->current_lba = (cylinder * floppy->GetNumHeads() + head) * floppy->GetSectorsPerTrack() + (sector - 1);
  return true;
}

bool FDC::SeekToNextSector(uint32 drive)
{
  DriveState* state = &m_drives[drive];
  Floppy* floppy = state->floppy;

  // CHS addressing
  uint32 cylinder = state->current_cylinder;
  uint32 head = state->current_head;
  uint32 sector = state->current_sector;

  // move sectors -> heads -> cylinders
  sector++;
  if (sector > floppy->GetSectorsPerTrack())
  {
    sector = 1;
    head++;
    if (head >= floppy->GetNumHeads())
    {
      head = 0;
      cylinder++;
      if (cylinder >= floppy->GetNumTracks())
      {
        // end of disk
        return false;
      }
    }
  }
  state->current_cylinder = cylinder;
  state->current_head = head;
  state->current_sector = sector;

  uint32 old_lba = state->current_lba;
  state->current_lba = (cylinder * floppy->GetNumHeads() + head) * floppy->GetSectorsPerTrack() + (sector - 1);
  DebugAssert((old_lba + 1) == state->current_lba);
  return true;
}

void FDC::ReadCurrentSector(uint32 drive, void* data)
{
  DriveState* state = &m_drives[drive];
  Log_DevPrintf("FDC read lba %u offset %u", state->current_lba, state->current_lba * SECTOR_SIZE);
  state->floppy->Read(data, state->current_lba * SECTOR_SIZE, SECTOR_SIZE);
}

void FDC::WriteCurrentSector(uint32 drive, const void* data)
{
  DriveState* state = &m_drives[drive];
  Log_DevPrintf("FDC write lba %u offset %u", state->current_lba, state->current_lba * SECTOR_SIZE);
  state->floppy->Write(data, state->current_lba * SECTOR_SIZE, SECTOR_SIZE);
}

bool FDC::ReadSector(uint32 drive, uint32 cylinder, uint32 head, uint32 sector, void* data)
{
  if (!SeekDrive(drive, cylinder, head, sector))
    return false;

  ReadCurrentSector(drive, data);
  return true;
}

bool FDC::WriteSector(uint32 drive, uint32 cylinder, uint32 head, uint32 sector, const void* data)
{
  if (!SeekDrive(drive, cylinder, head, sector))
    return false;

  WriteCurrentSector(drive, data);
  return true;
}

uint8 FDC::CurrentCommand::GetExpectedParameterCount() const
{
  switch (command)
  {
    case CMD_SPECIFY:
      return 2;
    case CMD_WRITE_DATA:
    case CMD_READ_DATA:
      return 8;
    case CMD_RECALIBRATE:
      return 1;
    case CMD_SEEK:
      return 2;
    case CMD_SENSE_INTERRUPT:
      return 0;
    case CMD_SENSE_STATUS:
      return 1;
    case CMD_READ_ID:
      return 1;
    case CMD_LOCK:
    case CMD_UNLOCK:
      return 0;
    case CMD_PERPENDICULAR_MODE:
      return 1;
    case CMD_CONFIGURE:
      return 3;
    default:
      return 0;
  }
}

void FDC::BeginCommand()
{
  CurrentCommand& cmd = m_current_command;
  Log_DevPrintf("Floppy command 0x%02X MT=%s,MF=%s,SK=%s", cmd.command.GetValue(), cmd.mt.GetValue() ? "yes" : "no",
                cmd.mf.GetValue() ? "yes" : "no", cmd.sk.GetValue() ? "yes" : "no");

  switch (m_current_command.command)
  {
    case CMD_SPECIFY: // Specify
    {
      m_step_rate_time = (cmd.params[0] >> 4) & 0b1111;
      m_head_unload_time = (cmd.params[0]) & 0b1111;
      m_head_load_time = (cmd.params[1] >> 1) & 0b1111;
      m_pio_mode = !!(cmd.params[1] & 0b1);

      // No result bytes or interrupt
      TransitionToCommandPhase();
    }
    break;

    case CMD_RECALIBRATE: // Recalibrate
    {
      const uint8 drive_number = cmd.params[0] & 0x03;
      Log_DevPrintf("Recalibrate drive %u", ZeroExtend32(drive_number));

      // Calculate seek time.
      CycleCount seek_time = 1;
      if (IsDiskPresent(drive_number))
      {
        seek_time = CalculateHeadSeekTime(m_drives[drive_number].current_cylinder, 0);

        // We actually seek the drive here, rather than in the End handler.
        if (!SeekDrive(drive_number, 0, 0, 1))
        {
          Panic("Recalibrate host seek failed.");
          HangController();
          return;
        }
      }

      m_MSR.request_for_master = false;
      SetActivity(drive_number);
      m_command_event->Queue(seek_time);
    }
    break;

    case CMD_SEEK: // Seek
    {
      const uint8 drive_number = (cmd.params[0] & 0b11);
      const uint8 head_number = (cmd.params[0] >> 2);
      const uint8 cylinder_number = (cmd.params[1]);
      Log_DevPrintf("Floppy seek drive %u head %u cylinder %u", drive_number, head_number, cylinder_number);

      if (!IsDiskPresent(drive_number))
      {
        HangController();
        return;
      }

      // Ensure the cylinder/head is in range.
      DriveState* drive = &m_drives[drive_number];
      if (cylinder_number >= drive->floppy->GetNumTracks() || head_number >= drive->floppy->GetNumHeads())
      {
        EndTransfer(drive_number, ST0_IC_AT, ST1_ND, 0);
        return;
      }

      // Calculate time to seek.
      CycleCount seek_time = CalculateHeadSeekTime(drive->current_cylinder, cylinder_number);

      // We actually seek the drive here, rather than in the End handler.
      if (!SeekDrive(drive_number, cylinder_number, head_number, 1))
      {
        Panic("Host seek failed.");
        HangController();
        return;
      }

      // The End handler just pretty much has to send the result now.
      m_MSR.request_for_master = false;
      SetActivity(drive_number);
      m_command_event->Queue(seek_time);
    }
    break;

    case CMD_WRITE_DATA: // Write Data
    case CMD_READ_DATA:  // Read Data
    {
      const bool is_write = (cmd.command == CMD_WRITE_DATA);
      const uint8 drive_number = (cmd.params[0] & 0x03);
      const uint8 head_number2 = (cmd.params[0] >> 2);
      const uint8 cylinder_number = (cmd.params[1]);
      const uint8 head_number = (cmd.params[2]);
      const uint8 sector_number = (cmd.params[3]);
      const uint8 sector_type = (cmd.params[4]);
      const uint8 end_of_track = (cmd.params[5]);
      // const uint8 gap1_size = (cmd.params[6]);
      const uint8 sector_type2 = (cmd.params[7]);

      if (!IsDiskPresent(drive_number))
      {
        Log_DevPrintf("Write: Medium not present");
        HangController();
        return;
      }

      // Header number in bit2 should equal head_number
      if (head_number2 != head_number)
      {
        EndTransfer(drive_number, ST0_IC_AT, ST1_ND, 0);
        return;
      }

      // Check for out-of-range reads.
      DriveState* drive = &m_drives[drive_number];
      if (cylinder_number >= drive->floppy->GetNumTracks() || sector_number == 0 ||
          sector_number > drive->floppy->GetSectorsPerTrack())
      {
        EndTransfer(drive_number, ST0_IC_AT, ST1_ND, 0);
        return;
      }

      // TODO: Properly validate ranges
      DebugAssert(sector_type == 0x02);
      if (sector_type == 0)
      {
        // sector type 2 means the number of bytes is specified in the last command byte
        m_current_transfer.bytes_per_sector = sector_type2;
      }
      else
      {
        m_current_transfer.bytes_per_sector = 128 << sector_type;
      }

      m_current_transfer.active = true;
      m_current_transfer.multi_track = cmd.mt;
      m_current_transfer.is_write = is_write;
      m_current_transfer.sectors_per_track = end_of_track;
      m_current_transfer.drive = drive_number;
      m_current_transfer.sector_offset = 0;

      // TODO: Non-dma transfer
      m_MSR.pio_mode = false;

      Log_DevPrintf("Floppy start %s %u/%u/%u", is_write ? "write" : "read", cylinder_number, head_number,
                    sector_number);

      // Clear RFM bit. The direction bit is set after the seek.
      m_MSR.request_for_master = false;
      SetActivity(drive_number, is_write);

      // We need to seek to the correct track/cylinder.
      CycleCount seek_time = CalculateHeadSeekTime(drive->current_cylinder, cylinder_number);
      if (!SeekDrive(drive_number, cylinder_number, head_number, sector_number))
      {
        EndTransfer(drive_number, ST0_IC_AT, ST1_ND, 0);
        return;
      }

      // Calculate time to read sector for reads, immediate for writes.
      CycleCount read_time = 1;
      if (!is_write)
        read_time += CalculateSectorReadTime() * sector_number;

      m_command_event->Queue(seek_time + read_time);
    }
    break;

    case CMD_SENSE_INTERRUPT: // Sense interrupt
    {
      ClearFIFO();

      if (m_reset_sense_interrupt_count > 0)
      {
        const uint8 drive_number = (4 - m_reset_sense_interrupt_count);
        WriteToFIFO(GetST0(drive_number, ST0_IC_SC));
        WriteToFIFO(Truncate8(IsDiskPresent(drive_number) ? m_drives[drive_number].current_cylinder : 0));

        TransitionToResultPhase();
        m_reset_sense_interrupt_count--;
      }
      else if (m_interrupt_pending)
      {
        WriteToFIFO(m_st0);
        WriteToFIFO(Truncate8(GetCurrentDrive()->current_cylinder));

        TransitionToResultPhase();
      }
      else
      {
        m_st0 = GetST0(GetCurrentDriveIndex(), ST0_IC_IC);
        WriteToFIFO(m_st0);
        TransitionToResultPhase();
      }
    }
    break;

    case CMD_SENSE_STATUS: // Get Status
    {
      const uint8 drive_number = cmd.params[0] & 0x03;
      const uint8 head = (cmd.params[0] >> 2) & 0x01;
      SeekDrive(drive_number, m_drives[drive_number].current_cylinder, head, m_drives[drive_number].current_sector);

      // Writes ST3 to the result, but does not raise an interrupt.
      ClearFIFO();
      WriteToFIFO(GetST3(drive_number, 0));
      TransitionToResultPhase();
    }
    break;

    case CMD_READ_ID: // Read ID
    {
      const uint8 drive_number = cmd.params[0] & 0x03;
      const uint8 head = (cmd.params[0] >> 2) & 0x01;

      if (!IsDiskPresent(drive_number) || m_DOR.IsMotorOn(drive_number))
      {
        Log_DevPrintf("Read ID: disk not present or motor not on");
        HangController();
        return;
      }

      // TODO: Check data rate
      DriveState* drive = &m_drives[drive_number];
      if (!SeekDrive(drive_number, drive->current_cylinder, head, drive->current_sector))
      {
        // Head not valid.
        EndTransfer(drive_number, ST0_IC_AT, ST1_MA, 0);
        return;
      }

      m_MSR.request_for_master = false;
      SetActivity(drive_number);
      m_command_event->Queue(CalculateSectorReadTime());
    }
    break;

    case CMD_VERSION:
    {
      if (m_model < Model_82077)
      {
        HandleUnsupportedCommand();
        return;
      }

      ClearFIFO();
      WriteToFIFO(0x90);
      TransitionToResultPhase();
    }
    break;

    case CMD_DUMP_REGISTERS:
    {
      if (m_model < Model_82072)
      {
        HandleUnsupportedCommand();
        return;
      }

      ClearFIFO();
      for (uint32 i = 0; i < MAX_DRIVES; i++)
        WriteToFIFO(Truncate8(m_drives[i].current_cylinder));
      WriteToFIFO((m_step_rate_time << 4) | m_head_unload_time);
      WriteToFIFO((m_head_load_time << 1) | (m_MSR.pio_mode ? 1 : 0));
      WriteToFIFO(m_drives[GetCurrentDriveIndex()].step_latch);                    // EOT
      WriteToFIFO((m_specify_lock ? 0x80 : 0x00) | (m_perpendicular_mode & 0x7F)); // lock << 7 | perp_mode
      WriteToFIFO(m_fifo_threshold | (m_implied_seeks ? 0x10 : 0x00) | (m_fifo_disabled ? 0x20 : 0x00) |
                  (m_polling_disabled ? 0x40 : 0x00)); // config
      WriteToFIFO(m_precompensation_start_track);      // pretrk
      TransitionToResultPhase();
    }
    break;

    case CMD_LOCK:
    case CMD_UNLOCK:
    {
      if (m_model < Model_82077)
      {
        HandleUnsupportedCommand();
        return;
      }

      m_specify_lock = (cmd.command & 0x80) != 0;
      ClearFIFO();
      WriteToFIFO(BoolToUInt8(m_specify_lock) << 4);
      TransitionToResultPhase();
    }
    break;

    case CMD_PERPENDICULAR_MODE:
    {
      if (m_model < Model_82077)
      {
        HandleUnsupportedCommand();
        return;
      }

      // Not supported, so let's just ignore it for now.
      Log_WarningPrintf("Perpendicular mode configure 0x%02X", cmd.params[0]);
      m_perpendicular_mode = cmd.params[0];
      TransitionToCommandPhase();
    }
    break;

    case CMD_CONFIGURE:
    {
      if (m_model < Model_82072)
      {
        HandleUnsupportedCommand();
        return;
      }

      m_fifo_threshold = cmd.params[1] & 0x0F;       // FIFOTHR
      m_implied_seeks = !!(cmd.params[1] & 0x10);    // EIS
      m_fifo_disabled = !!(cmd.params[1] & 0x20);    // EFIFO
      m_polling_disabled = !!(cmd.params[1] & 0x40); // POLL
      m_precompensation_start_track = cmd.params[2]; // PRETRK
      TransitionToCommandPhase();
    }
    break;

    default:
      HandleUnsupportedCommand();
      break;
  }
}

void FDC::HandleUnsupportedCommand()
{
  CurrentCommand& cmd = m_current_command;
  Log_ErrorPrintf("Unknown floppy command 0x%02X MT=%s,MF=%s,SK=%s", cmd.command.GetValue(),
                  cmd.mt.GetValue() ? "yes" : "no", cmd.mf.GetValue() ? "yes" : "no", cmd.sk.GetValue() ? "yes" : "no");

  ClearFIFO();
  WriteToFIFO(ST0_IC_IC);
  TransitionToResultPhase();
}

void FDC::EndCommand()
{
  CurrentCommand& cmd = m_current_command;

  switch (m_current_command.command)
  {
    case CMD_RECALIBRATE:
    {
      const uint8 drive_number = cmd.params[0] & 0x03;

      // Calculate seek status.
      if (!IsDiskPresent(drive_number))
        m_st0 = GetST0(drive_number, ST0_IC_IC | ST0_IC_AT | ST0_SE);
      else
        m_st0 = GetST0(drive_number, ST0_SE);

      // No result bytes, but sends interrupt
      m_command_event->Deactivate();
      TransitionToCommandPhase();
      RaiseInterrupt();
    }
    break;

    case CMD_SEEK: // Seek
    {
      const uint8 drive_number = cmd.params[0] & 0x03;
      m_st0 = GetST0(drive_number, ST0_SE);

      // No result bytes, but sends interrupt
      m_command_event->Deactivate();
      TransitionToCommandPhase();
      RaiseInterrupt();
    }
    break;

    case CMD_READ_ID:
    {
      // EndTransfer will cancel the event.
      const uint8 drive_number = cmd.params[0] & 0x03;
      EndTransfer(drive_number, ST0_IC_NT, 0, 0);
    }
    break;

    case CMD_READ_DATA:
    case CMD_WRITE_DATA:
    {
      // Everything is already set up, we just need to kick the DMA transfer.
      // We leave the event active for the next sector.
      m_dma->SetDMAState(DMA_CHANNEL, true);
      m_command_event->Reschedule(CalculateSectorReadTime());
    }
    break;
  }
}

void FDC::HangController()
{
  // Leave the command in place, this way writes are ignored.
  ClearActivity();
  m_MSR.command_busy = true;
  m_MSR.data_direction = false;
  m_MSR.request_for_master = false;

  if (m_command_event->IsActive())
    m_command_event->Deactivate();

  if (m_current_transfer.active)
  {
    m_current_transfer.active = false;
    m_dma->SetDMAState(DMA_CHANNEL, false);
  }
}

void FDC::TransitionToCommandPhase()
{
  m_current_command.Clear();
  ClearFIFO();
  LowerInterrupt();

  m_MSR.command_busy = false;
  m_MSR.data_direction = false;
  m_MSR.request_for_master = true;
  ClearActivity();
}

void FDC::TransitionToResultPhase()
{
  m_current_command.Clear();
  m_MSR.command_busy = true;
  m_MSR.data_direction = true;
  m_MSR.request_for_master = true;
  ClearActivity();
}

void FDC::ConnectIOPorts(Bus* bus)
{
  bus->ConnectIOPortRead(0x03F0, this, std::bind(&FDC::IOReadStatusRegisterA, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x03F1, this, std::bind(&FDC::IOReadStatusRegisterB, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x03F2, this, std::bind(&FDC::IOReadDigitalOutputRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x03F2, this, std::bind(&FDC::IOWriteDigitalOutputRegister, this, std::placeholders::_2));
  bus->ConnectIOPortReadToPointer(0x03F4, this, &m_MSR.bits);
  bus->ConnectIOPortWrite(0x03F4, this, std::bind(&FDC::IOWriteDataRateSelectRegister, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x03F5, this, std::bind(&FDC::IOReadFIFO, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x03F5, this, std::bind(&FDC::IOWriteFIFO, this, std::placeholders::_2));
  bus->ConnectIOPortRead(0x03F7, this, std::bind(&FDC::IOReadDigitalInputRegister, this, std::placeholders::_2));
  bus->ConnectIOPortWrite(0x03F7, this,
                          std::bind(&FDC::IOWriteConfigurationControlRegister, this, std::placeholders::_2));

  // connect DMA channel
  if (m_dma)
  {
    m_dma->ConnectDMAChannel(
      DMA_CHANNEL,
      std::bind(&FDC::DMAReadCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
      std::bind(&FDC::DMAWriteCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  }
}

void FDC::RaiseInterrupt()
{
  m_interrupt_pending = true;
  m_reset_sense_interrupt_count = 0;
  if (m_DOR.ndmagate)
    m_interrupt_controller->RaiseInterrupt(6);
}

void FDC::LowerInterrupt()
{
  m_interrupt_pending = false;
  m_interrupt_controller->LowerInterrupt(6);
}

void FDC::IOReadStatusRegisterA(uint8* value)
{
  // 0x80 - interrupt pending
  // 0x40 - dma request
  // 0x20 - step latch - set on seek/recalibrate, cleared on digital input register read or reset
  // 0x10 - track 0
  // 0x08 - head 0
  // 0x04 - index - sector == 0
  // 0x02 - write protect
  // 0x01 - direction == 0
  const DriveState* ds = GetCurrentDrive();
  *value = (BoolToUInt8(m_interrupt_pending) << 7) | (BoolToUInt8(m_dma->GetDMAState(DMA_CHANNEL)) << 6) |
           (BoolToUInt8(ds->step_latch) << 5) | (BoolToUInt8(ds->current_cylinder == 0) << 4) |
           (BoolToUInt8(ds->current_head == 0) << 3) | (BoolToUInt8(ds->current_sector == 0) << 2) |
           (BoolToUInt8(ds->write_protect) << 1) | (BoolToUInt8(ds->direction) << 0);
}

void FDC::IOReadStatusRegisterB(uint8* value)
{
  // 0x80 - no second drive
  // 0x40 - not ds1
  // 0x20 - not ds0
  // 0x10 - data written
  // 0x08 - data read
  // 0x04 - data written again
  // 0x02 - not ds3
  // 0x01 - not ds2
  const DriveState* ds = GetCurrentDrive();
  *value = (BoolToUInt8(IsDrivePresent(1)) << 7) | (BoolToUInt8(GetCurrentDriveIndex() != 1) << 6) |
           (BoolToUInt8(GetCurrentDriveIndex() != 0) << 5) | (BoolToUInt8(ds->data_was_written) << 4) |
           (BoolToUInt8(ds->data_was_read) << 3) | (BoolToUInt8(ds->data_was_written) << 2) |
           (BoolToUInt8(GetCurrentDriveIndex() != 3) << 1) | (BoolToUInt8(GetCurrentDriveIndex() != 2) << 0);
}

void FDC::IOReadDigitalInputRegister(uint8* value)
{
  uint8 bits = 0b01111000;
  bits |= BoolToUInt8(m_disk_change_flag) << 7;
  bits |= (m_data_rate_index & 0x03) << 1;
  if (data_rates[m_data_rate_index] >= 500)
    bits |= 0x01; // High density

  // Bit 7 - not disk change
  // Bit 3 - interrupt enable
  // Bit 2 - CCR?
  // Bit 1-0 - Data rate select
  *value = bits;

  // Clear step bit of current drive
  GetCurrentDrive()->step_latch = false;
}

void FDC::IOReadDigitalOutputRegister(uint8* value)
{
  // Bit 7 - Motor on disk 3 enable
  // Bit 6 - Motor on disk 2 enable
  // Bit 5 - Motor on disk 1 enable
  // Bit 4 - Motor on disk 0 enable
  // Bit 3 - Interrupt enable
  // Bit 2 - Reset
  // Bit 1-0 - Drive select
  *value = m_DOR.bits;
}

void FDC::IOWriteDigitalOutputRegister(uint8 value)
{
  decltype(m_DOR) changed_bits = {uint8(m_DOR.bits ^ value)};
  m_DOR.bits = value;

  if (changed_bits.ndmagate && !m_DOR.ndmagate)
    LowerInterrupt();

  // This bit is inverted.
  if (changed_bits.nreset)
  {
    if (!m_DOR.nreset)
    {
      // Queue a reset command. Make sure we can interrupt this.
      Log_DevPrintf("FDC enter reset");
      m_reset_begin_time = m_system->GetTimingManager()->GetTotalEmulatedTime();
      m_command_event->SetActive(false);
    }
    else
    {
      // Reset after 250us.
      // TODO: We should ignore commands until this point.
      Log_DevPrintf("FDC leave reset");

      SimulationTime time_since_reset = m_system->GetTimingManager()->GetEmulatedTimeDifference(m_reset_begin_time);
      if (time_since_reset > 1000)
      {
        m_reset_begin_time = 0;
        Reset(true);
      }
    }
  }

  // Clear FIFO, TODO move data direction to ClearFIFO
  ClearFIFO();
  m_MSR.data_direction = false;
}

void FDC::IOWriteDataRateSelectRegister(uint8 value)
{
  m_data_rate_index = value & 0x03;

  if (value & 0x80)
  {
    // Soft reset, self-clearing.
    Reset(true);
  }
}

void FDC::IOWriteConfigurationControlRegister(uint8 value)
{
  m_data_rate_index = value & 0x03;
}

void FDC::IOReadFIFO(uint8* value)
{
  // Are we in a DMA transfer? Ignore if so.
  if (InReset() || IsDMATransferInProgress())
  {
    *value = 0xFF;
    return;
  }

  // If a transfer is in progress, this is a PIO transfer.
  if (m_current_transfer.active)
  {
    Panic("TODO: Handle PIO writes.");
    *value = 0xFF;
    return;
  }

  // Reading results back.
  if (m_fifo_result_position == m_fifo_result_size)
  {
    // Bad read
    Log_WarningPrintf("Bad floppy data read");
    *value = 0;
    return;
  }

  *value = m_fifo[m_fifo_result_position++];

  // Interrupt is lowered after the first byte is read.
  LowerInterrupt();

  // Flags are cleared after result is read in its entirety.
  if (m_fifo_result_position == m_fifo_result_size)
    TransitionToCommandPhase();
}

void FDC::IOWriteFIFO(uint8 value)
{
  // Are we in a DMA transfer? Ignore if so.
  if (InReset() || IsDMATransferInProgress())
    return;

  // If a transfer is in progress, this is a PIO transfer.
  if (m_current_transfer.active)
  {
    Panic("TODO: Handle PIO writes.");
    return;
  }

  // Append to the command buffer.
  if (!m_current_command.HasAllParameters())
  {
    m_MSR.command_busy = true;
    Assert(m_current_command.command_length <= sizeof(m_current_command.buf));
    m_current_command.buf[m_current_command.command_length++] = value;
    if (m_current_command.HasAllParameters())
      BeginCommand();
  }
}

void FDC::EndTransfer(uint32 drive, uint8 st0_bits, uint8 st1_bits, uint8 st2_bits)
{
  m_current_command.Clear();
  if (m_current_transfer.active)
  {
    m_current_transfer.Clear();
    m_dma->SetDMAState(DMA_CHANNEL, false);
  }
  if (m_command_event->IsActive())
    m_command_event->Deactivate();

  m_st0 = GetST0(drive, st0_bits);
  m_st1 = GetST1(drive, st1_bits);
  m_st2 = GetST2(drive, st2_bits);

  Log_DevPrintf("End transfer, Drive=%u ST0=0x%02X ST1=0x%02X ST2=0x%02X", drive, ZeroExtend32(m_st0),
                ZeroExtend32(m_st1), ZeroExtend32(m_st2));

  ClearFIFO();
  WriteToFIFO(m_st0);
  WriteToFIFO(m_st1);
  WriteToFIFO(m_st2);
  WriteToFIFO(Truncate8(m_drives[drive].current_cylinder));
  WriteToFIFO(Truncate8(m_drives[drive].current_head));
  WriteToFIFO(Truncate8(m_drives[drive].current_sector));
  WriteToFIFO(Truncate8(2)); // from request sector size
  TransitionToResultPhase();
  RaiseInterrupt();
}

bool FDC::MoveToNextTransferSector()
{
  DriveState* drive_state = &m_drives[m_current_transfer.drive];

  // TODO: Handle errors here.
  if ((drive_state->current_sector + 1) > m_current_transfer.sectors_per_track)
  {
    // Move to next head if multi-track mode is on
    if (m_current_transfer.multi_track && drive_state->current_head == 0)
    {
      return SeekDrive(m_current_transfer.drive, drive_state->current_cylinder, 1, 1);
    }
    else
    {
      // Move to next cylinder
      return SeekDrive(m_current_transfer.drive, drive_state->current_cylinder + 1, 0, 1);
    }
  }
  else
  {
    return SeekDrive(m_current_transfer.drive, drive_state->current_cylinder, drive_state->current_head,
                     drive_state->current_sector + 1);
  }
}

void FDC::DMAReadCallback(IOPortDataSize size, uint32* value, uint32 remaining_bytes)
{
  Assert(m_current_transfer.active);

  // TODO: Is it defined what happens when we configure a DMA read with a write command?
  if (m_current_transfer.is_write)
  {
    Log_ErrorPrintf("DMA read with write command");
    EndTransfer(m_current_transfer.drive, ST0_IC_AT, ST1_ND, 0);
    return;
  }

  if (m_current_transfer.sector_offset == 0)
    ReadCurrentSector(m_current_transfer.drive, m_current_transfer.sector_buffer);

  *value = m_current_transfer.sector_buffer[m_current_transfer.sector_offset];

  // Check for early exit of transfer
  if (remaining_bytes == 0)
  {
    EndTransfer(m_current_transfer.drive, ST0_IC_NT, 0, 0);
    return;
  }

  m_current_transfer.sector_offset++;
  if (m_current_transfer.sector_offset >= m_current_transfer.bytes_per_sector)
  {
    m_current_transfer.sector_offset = 0;

    // TODO: Timing for seeking to next cylinder.
    if (!MoveToNextTransferSector())
    {
      EndTransfer(m_current_transfer.drive, ST0_IC_AT, ST1_EN, 0);
      return;
    }

    // Clear the request flag, while we're reading the next sector. EndCommand() will re-enable it.
    m_dma->SetDMAState(DMA_CHANNEL, false);
  }
}

void FDC::DMAWriteCallback(IOPortDataSize size, uint32 value, uint32 remaining_bytes)
{
  Assert(m_current_transfer.active);

  // TODO: Is it defined what happens when we configure a DMA read with a write command?
  if (!m_current_transfer.is_write)
  {
    Log_ErrorPrintf("DMA write with read command");
    EndTransfer(m_current_transfer.drive, ST0_IC_AT, ST1_ND, 0);
    return;
  }

  m_current_transfer.sector_buffer[m_current_transfer.sector_offset++] = Truncate8(value);
  if (m_current_transfer.sector_offset >= m_current_transfer.bytes_per_sector)
  {
    // Data will be lost if the sector size doesn't match..
    if (m_current_transfer.bytes_per_sector != SECTOR_SIZE)
      Log_ErrorPrintf("Incorrect sector size, data will be lost");

    WriteCurrentSector(m_current_transfer.drive, m_current_transfer.sector_buffer);
    m_current_transfer.sector_offset = 0;
  }

  // Check for early exit of transfer.
  if (remaining_bytes == 0)
  {
    if (m_current_transfer.sector_offset != 0)
      Log_ErrorPrintf("Incomplete sector DMA transfer, data will be lost");

    EndTransfer(m_current_transfer.drive, ST0_IC_NT, 0, 0);
    return;
  }

  // Move to next sector if there's still sectors remaining
  if (m_current_transfer.sector_offset == 0)
  {
    if (!MoveToNextTransferSector())
    {
      EndTransfer(m_current_transfer.drive, ST0_IC_AT, ST1_EN, 0);
      return;
    }

    // Clear the request flag, while we're writing the next sector. EndCommand() will re-enable it.
    m_dma->SetDMAState(DMA_CHANNEL, false);
  }
}

uint8 FDC::GetST0(uint32 drive, uint8 bits) const
{
  return (bits & 0xF8) | (Truncate8(m_drives[drive].current_head & 0x01) << 2) | (Truncate8(drive) << 0);
}

uint8 FDC::GetST1(uint32 drive, uint8 bits) const
{
  return bits;
}

uint8 FDC::GetST2(uint32 drive, uint8 bits) const
{
  return bits;
}

uint8 FDC::GetST3(uint32 drive, uint8 bits) const
{
  return (BoolToUInt8(m_drives[drive].write_protect) << 6) | uint8(1 << 5) |
         (BoolToUInt8(m_drives[drive].current_cylinder == 0) << 4) | uint8(1 << 3) |
         (Truncate8(m_drives[drive].current_head & 0x01) << 2) | (Truncate8(drive) << 0);
}

CycleCount FDC::CalculateHeadSeekTime(uint32 current_track, uint32 destination_track) const
{
  uint32 move_count =
    (current_track >= destination_track) ? (current_track - destination_track) : (destination_track - current_track);
  return CalculateHeadSeekTime() * CycleCount(std::max(move_count, 1u));
}

CycleCount FDC::CalculateHeadSeekTime() const
{
  // SRT value = 16 - (milliseconds * data_rate / 500000) (https://wiki.osdev.org/Floppy_Disk_Controller)
  uint32 val = uint32(m_step_rate_time ^ 0x0F) + 1;
  return (val * 500000) / data_rates[m_data_rate_index];
}

CycleCount FDC::CalculateSectorReadTime() const
{
  // to-microseconds, bits-per-sector / (data-rate-in-kbs*1000)
  return 1000000u * (512u * 8u) / (data_rates[m_data_rate_index] * 1000);
}

void FDC::CurrentTransfer::Clear()
{
  active = false;
  multi_track = false;
  is_write = false;
  drive = 0;
  bytes_per_sector = 0;
  sectors_per_track = 0;
  sector_offset = 0;
}

} // namespace HW
