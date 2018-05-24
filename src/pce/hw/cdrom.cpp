#include <functional>
#include "pce/hw/cdrom.h"
#include "pce/system.h"
#include "YBaseLib/Log.h"
Log_SetChannel(HW::CDROM);

namespace HW {

CDROM::CDROM()
  : m_clock("CDROM", 1000000.0f)
{

}

CDROM::~CDROM() {}

void CDROM::Initialize(System* system, Bus* bus)
{
  m_clock.SetManager(system->GetTimingManager());
  m_command_event = m_clock.NewEvent("CDROM Command Event", 1, std::bind(&CDROM::CompleteCommand, this), false);
}

void CDROM::Reset() {}

bool CDROM::LoadState(BinaryReader& reader)
{
  return false;
}

bool CDROM::SaveState(BinaryWriter& writer)
{
  return false;
}

void CDROM::SetCommandCompletedCallback(CommandCompletedCallback callback)
{
  m_command_completed_callback = std::move(callback);
}

bool CDROM::WriteCommandBuffer(const void* data, size_t data_len)
{
  if (m_busy)
    return true;

  const byte* data_ptr = reinterpret_cast<const byte*>(data);
  for (size_t i = 0; i < data_len; i++)
  {
    m_command_buffer.push_back(*(data_ptr++));
    if (BeginCommand())
      return true;
  }

  return false;
}

bool CDROM::BeginCommand()
{
  uint8 opcode = uint8(m_command_buffer[0]);
//   switch (opcode)
//   {
//     
//   }

  // Unknown command.
  Log_ErrorPrintf("Unhandled SCSI command 0x%02X", ZeroExtend32(opcode));
  m_error = true;
  return true;
}

void CDROM::QueueCommand(uint32 time_in_microseconds)
{
  DebugAssert(!m_command_event->IsActive());
  m_error = false;
  m_busy = true;
  m_command_event->Queue(CycleCount(time_in_microseconds));
}

void CDROM::CompleteCommand()
{

}

} // namespace HW