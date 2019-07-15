#include "timing_event.h"
#include "YBaseLib/Assert.h"
#include "system.h"

TimingEvent::TimingEvent(System* system, const char* name, float frequency, SimulationTime cycle_period,
                         CycleCount interval, TimingEventCallback callback)
  : m_system(system), m_name(name), m_frequency(frequency), m_cycle_period(cycle_period), m_interval(interval),
    m_downcount(m_cycle_period * interval), m_time_since_last_run(0), m_callback(std::move(callback)), m_active(false)
{
  Assert(m_cycle_period > 0);
}

TimingEvent::~TimingEvent()
{
  if (m_active)
    m_system->RemoveActiveEvent(this);
}

SimulationTime TimingEvent::GetTimeSinceLastExecution() const
{
  return m_system->GetPendingEventTime() + m_time_since_last_run;
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
  m_system->SortEvents();
}

void TimingEvent::Reset()
{
  if (m_active)
  {
    m_downcount = m_interval * m_cycle_period;
    m_time_since_last_run = 0;
    m_system->SortEvents();
  }
}

void TimingEvent::InvokeEarly(bool force /* = false */)
{
  if (!m_active)
    return;

  // Remove the pending time, since we want this to be included in the cycles we pass through.
  // We could just force an event sync here, but this would mean that InvokeEarly could be
  // called recursively, which would be a bad thing.
  const u64 pending_time = m_system->GetPendingEventTime();
  m_downcount -= pending_time;
  m_time_since_last_run += pending_time;

  // Try to maintain partial cycles as best as possible.
  CycleCount cycles_to_execute = m_time_since_last_run / m_cycle_period;
  CycleCount partial_time = m_time_since_last_run % m_cycle_period;
  m_time_since_last_run -= cycles_to_execute * m_cycle_period;
  m_downcount = (m_interval * m_cycle_period) - partial_time;

  // Re-add the pending time. Since we're re-scheduling, we want the event to occur after
  // the current time (which includes pending time).
  m_downcount += pending_time;
  m_time_since_last_run -= pending_time;

  // Since we've changed the downcount, we need to re-sort the events.
  m_system->SortEvents();

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
  const u64 pending_time = m_system->GetPendingEventTime();
  m_downcount += pending_time;
  m_time_since_last_run -= pending_time;

  m_system->AddActiveEvent(this);
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

  m_system->RemoveActiveEvent(this);
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
