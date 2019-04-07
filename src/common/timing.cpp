#include "timing.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
Log_SetChannel(TimingManager);

constexpr SimulationTime POLL_FREQUENCY = INT64_C(100000000);

static bool CompareEvents(const TimingEvent* lhs, const TimingEvent* rhs)
{
  return lhs->GetDownCount() > rhs->GetDownCount();
}

TimingManager::TimingManager() {}

TimingManager::~TimingManager()
{
  // Pointers will clean themselves up.
}

SimulationTime TimingManager::GetEmulatedTimeDifference(SimulationTime timestamp)
{
  return GetEmulatedTimeDifference(timestamp, m_total_emulated_time);
}

SimulationTime TimingManager::GetEmulatedTimeDifference(SimulationTime prev, SimulationTime now)
{
  if (prev <= now)
    return now - prev;
  else
    return (std::numeric_limits<SimulationTime>::max() - prev) + now;
}

void TimingManager::AddPendingTime(SimulationTime time)
{
  // TODO: Accuracy of total while running events..
  m_pending_time += time;
  m_total_emulated_time += time;
  if (m_pending_time >= m_next_event_time)
    RunEvents();
}

void TimingManager::AddActiveEvent(TimingEvent* event)
{
  m_events.push_back(event);
  if (!m_running_events)
  {
    std::push_heap(m_events.begin(), m_events.end(), CompareEvents);
    UpdateNextEventTime();
  }
  else
  {
    m_needs_sort = true;
  }
}

void TimingManager::RemoveActiveEvent(TimingEvent* event)
{
  auto iter = std::find_if(m_events.begin(), m_events.end(), [event](const auto& it) { return event == it; });
  if (iter == m_events.end())
  {
    Panic("Attempt to remove inactive event");
    return;
  }

  m_events.erase(iter);
  if (!m_running_events)
  {
    std::make_heap(m_events.begin(), m_events.end(), CompareEvents);
    UpdateNextEventTime();
  }
  else
  {
    m_needs_sort = true;
  }
}

TimingEvent* TimingManager::FindActiveEvent(const char* name)
{
  auto iter = std::find_if(m_events.begin(), m_events.end(), [&name](auto& ev) { return ev->GetName().Compare(name); });

  return (iter != m_events.end()) ? *iter : nullptr;
}

std::unique_ptr<TimingEvent> TimingManager::CreateFrequencyEvent(const char* name, float frequency,
                                                                 TimingEventCallback callback, bool active)
{
  SimulationTime cycle_period = SimulationTime(double(1000000000.0) / double(frequency));
  auto evt = std::make_unique<TimingEvent>(this, name, frequency, cycle_period, 1, std::move(callback));
  if (active)
    evt->Activate();

  return evt;
}

std::unique_ptr<TimingEvent> TimingManager::CreateMillisecondIntervalEvent(const char* name, CycleCount ms,
                                                                           TimingEventCallback callback,
                                                                           bool active /* = true */)
{
  auto evt = std::make_unique<TimingEvent>(this, name, 1000.0f, 1000000000 / 1000, ms, std::move(callback));
  if (active)
    evt->Activate();

  return evt;
}

std::unique_ptr<TimingEvent> TimingManager::CreateMicrosecondIntervalEvent(const char* name, CycleCount us,
                                                                           TimingEventCallback callback,
                                                                           bool active /* = true */)
{
  auto evt = std::make_unique<TimingEvent>(this, name, 1000000.0f, 1000000000 / 1000000, us, std::move(callback));
  if (active)
    evt->Activate();

  return evt;
}

std::unique_ptr<TimingEvent> TimingManager::CreateNanosecondIntervalEvent(const char* name, CycleCount ns,
                                                                          TimingEventCallback callback,
                                                                          bool active /* = true */)
{
  auto evt = std::make_unique<TimingEvent>(this, name, 1000.0f, 1000000000 / 1000000000, ns, std::move(callback));
  if (active)
    evt->Activate();

  return evt;
}

void TimingManager::UpdateNextEventTime()
{
  m_next_event_time =
    !m_events.empty() ? std::max(m_events.front()->GetDownCount(), SimulationTime(0)) : POLL_FREQUENCY;
}

void TimingManager::SortEvents()
{
  if (!m_running_events)
  {
    std::make_heap(m_events.begin(), m_events.end(), CompareEvents);
    UpdateNextEventTime();
  }
  else
  {
    m_needs_sort = true;
  }
}

void TimingManager::RunEvents()
{
  Assert(!m_running_events);

  SimulationTime remaining_time = m_pending_time;
  m_pending_time = 0;
  m_running_events = true;

  while (remaining_time > 0)
  {
    // To avoid issues where two events are related to each other from becoming desynced,
    // we run at a slice that is the length of the lowest next event time.
    SimulationTime time = std::min(remaining_time, m_next_event_time);
    remaining_time -= time;

    // Apply downcount to all events.
    // This will result in a negative downcount for those events which are late.
    for (TimingEvent* evt : m_events)
    {
      evt->m_downcount = evt->m_downcount - time;
      evt->m_time_since_last_run += time;
    }

    // Now we can actually run the callbacks.
    while (!m_events.empty() && m_events.front()->GetDownCount() <= 0)
    {
      TimingEvent* evt = m_events.front();
      SimulationTime time_late = -evt->m_downcount;
      std::pop_heap(m_events.begin(), m_events.end(), CompareEvents);

      // Don't include overrun cycles in the execution.
      // If the late time is greater than (period * interval), we'll re-place us at the front (or near)
      // the front of the queue again, and submit the next iteration then. This should reduce issues where
      // multiple events are dependent on one another, that may be caused if all cycles were executed at once.
      CycleCount cycles_to_execute = (evt->m_time_since_last_run - time_late) / evt->m_cycle_period;

      // Calculate and set the new downcount for periodic events, taking into account late time.
      CycleCount cycles_late = time_late / evt->m_cycle_period;

      // Factor late time into the time for the next invocation.
      evt->m_downcount += (evt->m_cycle_period * evt->m_interval);
      evt->m_time_since_last_run -= cycles_to_execute * evt->m_cycle_period;

      // The cycles_late is only an indicator, it doesn't modify the cycles to execute.
      evt->m_callback(evt, cycles_to_execute, cycles_late);

      // Place it in the appropriate position in the queue.
      if (m_needs_sort)
      {
        // Another event may have been changed by this event, or the interval/downcount changed.
        std::make_heap(m_events.begin(), m_events.end(), CompareEvents);
        m_needs_sort = false;
      }
      else
      {
        // Keep the event list in a heap. The event we just serviced will be in the last place,
        // so we can use push_here instead of make_heap, which should be faster.
        std::push_heap(m_events.begin(), m_events.end(), CompareEvents);
      }
    }

    // Run until next event, or 100ms.
    UpdateNextEventTime();
  }

  m_running_events = false;
}

bool TimingManager::LoadState(BinaryReader& reader)
{
  u32 signature;
  if (!reader.SafeReadUInt32(&signature) || signature != SAVE_STATE_SIGNATURE)
  {
    Log_ErrorPrintf("Invalid signature.");
    return false;
  }

  if (!reader.SafeReadInt64(&m_pending_time) || !reader.SafeReadInt64(&m_next_event_time) ||
      !reader.SafeReadInt64(&m_total_emulated_time))
  {
    return false;
  }

  // Load timestamps for the clock events.
  // Any oneshot events should be recreated by the load state method, so we can fix up their times here.
  u32 event_count;
  if (!reader.SafeReadUInt32(&event_count))
    return false;

  SmallString event_name;
  for (u32 i = 0; i < event_count; i++)
  {
    if (!reader.SafeReadCString(&event_name))
      return false;

    CycleCount downcount, time_since_last_run;
    if (!reader.SafeReadInt64(&downcount) || !reader.SafeReadInt64(&time_since_last_run))
      return false;

    TimingEvent* event = FindActiveEvent(event_name);
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

  SortEvents();

  Log_DevPrintf("Loaded %u events from save state.", event_count);
  return true;
}

bool TimingManager::SaveState(BinaryWriter& writer)
{
  if (!writer.SafeWriteUInt32(SAVE_STATE_SIGNATURE))
    return false;

  if (!writer.SafeWriteInt64(m_pending_time) || !writer.SafeWriteInt64(m_next_event_time) ||
      !writer.SafeWriteInt64(m_total_emulated_time))
  {
    return false;
  }

  // Event count placeholder.
  u64 count_offset = writer.GetStreamPosition();
  if (!writer.SafeWriteUInt32(0))
    return false;

  u32 event_count = 0;
  for (const TimingEvent* evt : m_events)
  {
    if (!writer.SafeWriteCString(evt->GetName()))
      return false;

    if (!writer.SafeWriteInt64(evt->m_downcount) || !writer.SafeWriteInt64(evt->m_time_since_last_run))
      return false;

    event_count++;
  }

  u64 end_offset = writer.GetStreamPosition();
  if (!writer.SafeSeekAbsolute(count_offset) || !writer.SafeWriteUInt32(event_count) ||
      !writer.SafeSeekAbsolute(end_offset))
    return false;

  Log_DevPrintf("Wrote %u events to save state.", event_count);
  return true;
}

TimingEvent::TimingEvent(TimingManager* manager, const char* name, float frequency, SimulationTime cycle_period,
                         CycleCount interval, TimingEventCallback callback)
  : m_manager(manager), m_name(name), m_frequency(frequency), m_cycle_period(cycle_period), m_interval(interval),
    m_downcount(m_cycle_period * interval), m_time_since_last_run(0), m_callback(std::move(callback)), m_active(false)
{
  Assert(m_cycle_period > 0);
}

TimingEvent::~TimingEvent()
{
  if (m_active)
    m_manager->RemoveActiveEvent(this);
}

SimulationTime TimingEvent::GetTimeSinceLastExecution() const
{
  return m_manager->GetPendingTime() + m_time_since_last_run;
}

CycleCount TimingEvent::GetCyclesSinceLastExecution() const
{
  return GetTimeSinceLastExecution() / m_cycle_period;
}

void TimingEvent::Reschedule(CycleCount cycles)
{
  // Should only be called when active and in the callback.
  DebugAssert(m_active);

  // We should really be up to date already in terms of cycles, so only take the partial cycles.
  CycleCount partial_cycles_nodiv = (m_downcount < 0) ? -m_downcount : (m_downcount % m_cycle_period);

  // Update the interval and new downcount, subtracting any partial cycles.
  m_interval = cycles;
  m_downcount = (cycles * m_cycle_period) - partial_cycles_nodiv;

  // If this is a call from an IO handler for example, re-sort the event queue.
  m_manager->SortEvents();
}

void TimingEvent::Reset()
{
  if (m_active)
  {
    m_downcount = m_interval * m_cycle_period;
    m_time_since_last_run = 0;
    m_manager->SortEvents();
  }
}

void TimingEvent::InvokeEarly(bool force /* = false */)
{
  if (!m_active)
    return;

  // Remove the pending time, since we want this to be included in the cycles we pass through.
  // We could just force an event sync here, but this would mean that InvokeEarly could be
  // called recursively, which would be a bad thing.
  m_downcount -= m_manager->GetPendingTime();
  m_time_since_last_run += m_manager->GetPendingTime();

  // Try to maintain partial cycles as best as possible.
  CycleCount cycles_to_execute = m_time_since_last_run / m_cycle_period;
  CycleCount partial_time = m_time_since_last_run % m_cycle_period;
  m_time_since_last_run -= cycles_to_execute * m_cycle_period;
  m_downcount = (m_interval * m_cycle_period) - partial_time;

  // Re-add the pending time. Since we're re-scheduling, we want the event to occur after
  // the current time (which includes pending time).
  m_downcount += m_manager->GetPendingTime();
  m_time_since_last_run -= m_manager->GetPendingTime();

  // Since we've changed the downcount, we need to re-sort the events.
  m_manager->SortEvents();

  // Run any pending cycles.
  if (force || cycles_to_execute > 0)
    m_callback(this, cycles_to_execute, 0);
}

void TimingEvent::Activate()
{
  Assert(!m_active);
  m_downcount = m_interval * m_cycle_period;
  m_time_since_last_run = 0;
  m_active = true;

  // Since we can be running behind, if we want to trigger this event on the correct
  // number of cycles, not immediately (and many times).
  m_downcount += m_manager->GetPendingTime();
  m_time_since_last_run -= m_manager->GetPendingTime();

  m_manager->AddActiveEvent(this);
}

void TimingEvent::Queue(CycleCount cycles)
{
  m_interval = cycles;
  Activate();
}

void TimingEvent::Deactivate()
{
  Assert(m_active);
  m_active = false;

  m_manager->RemoveActiveEvent(this);
}

void TimingEvent::SetFrequency(float new_frequency, u32 interval /* = 1 */)
{
  SimulationTime new_cycle_period = SimulationTime(double(1000000000.0) / double(new_frequency));

  // Adjust downcount if active.
  if (m_active)
  {
    SimulationTime diff = new_cycle_period - m_cycle_period;
    m_downcount += diff;
  }

  m_frequency = new_frequency;
  m_cycle_period = new_cycle_period;
  m_interval = interval;
}

void TimingEvent::SetActive(bool active)
{
  if (active)
  {
    if (!m_active)
      Activate();
  }
  else
  {
    if (m_active)
      Deactivate();
  }
}
