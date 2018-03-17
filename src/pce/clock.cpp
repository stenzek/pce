#include "pce/clock.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "pce/timing.h"
Log_SetChannel(Clock);

Clock::Clock(const char* name, float frequency) : m_name(name), m_frequency(frequency) {}

Clock::~Clock() {}

std::unique_ptr<TimingEvent> Clock::NewEvent(const char* name, CycleCount cycles, TimingEventCallback callback,
                                             bool active)
{
  SmallString event_name;
  event_name.Format("%s: %s", m_name.GetCharArray(), name);

  SimulationTime cycle_period = SimulationTime(double(1000000000.0) / double(m_frequency));
  auto event =
    std::make_unique<TimingEvent>(m_manager, event_name, m_frequency, cycle_period, cycles, std::move(callback));
  if (active)
    event->Activate();

  return std::move(event);
}

std::unique_ptr<TimingEvent> Clock::NewFrequencyEvent(const char* name, float frequency, TimingEventCallback callback,
                                                      bool active /*= true*/)
{
  SmallString event_name;
  event_name.Format("%s: %s", m_name.GetCharArray(), name);

  SimulationTime cycle_period = SimulationTime(double(1000000000.0) / double(frequency));
  auto event = std::make_unique<TimingEvent>(m_manager, event_name, frequency, cycle_period, 1, std::move(callback));
  if (active)
    event->Activate();

  return std::move(event);
}

bool Clock::LoadTimingState(TimingManager* manager, BinaryReader& reader)
{
  // Load timestamps for the clock events.
  // Any oneshot events should be recreated by the load state method, so we can fix up their times here.
  uint32 event_count;
  if (!reader.SafeReadUInt32(&event_count))
    return false;

  SmallString event_name;
  for (uint32 i = 0; i < event_count; i++)
  {
    if (!reader.SafeReadCString(&event_name))
      return false;

    CycleCount downcount, time_since_last_run;
    if (!reader.SafeReadInt64(&downcount) || !reader.SafeReadInt64(&time_since_last_run))
      return false;

    TimingEvent* event = manager->FindActiveEvent(event_name);
    if (!event)
    {
      Log_WarningPrintf("Save state has event '%s', but couldn't find this event when loading.",
                        event_name.GetCharArray());
      continue;
    }

    // Using reschedule is safe here since we call sort afterwards.
    event->m_downcount = downcount;
    event->m_time_since_last_run = time_since_last_run;
  }

  manager->SortEvents();

  Log_DevPrintf("Loaded %u events from save state.", event_count);
  return true;
}

bool Clock::SaveTimingState(TimingManager* manager, BinaryWriter& writer)
{
  uint64 count_offset = writer.GetStreamPosition();
  if (!writer.SafeWriteUInt32(0))
    return false;

  uint32 event_count = 0;
  manager->EnumerateActiveEvents([&writer, &event_count](const TimingEvent* evt) {
    if (!writer.SafeWriteCString(evt->GetName()))
      return;

    if (!writer.SafeWriteInt64(evt->m_downcount) || !writer.SafeWriteInt64(evt->m_time_since_last_run))
      return;

    event_count++;
  });

  if (writer.InErrorState())
    return false;

  uint64 end_offset = writer.GetStreamPosition();
  if (!writer.SafeSeekAbsolute(count_offset) || !writer.SafeWriteUInt32(event_count) ||
      !writer.SafeSeekAbsolute(end_offset))
    return false;

  Log_DevPrintf("Wrote %u events to save state.", event_count);
  return true;
}
