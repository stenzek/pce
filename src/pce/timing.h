#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "YBaseLib/String.h"

#include "pce/types.h"

class TimingManager;
class TimingEvent;

// Event callback type. Second parameter is the number of cycles the event was executed "late".
using TimingEventCallback = std::function<void(TimingEvent* event, CycleCount cycles, CycleCount cycles_late)>;

class TimingManager
{
public:
  TimingManager();
  ~TimingManager();

  SimulationTime GetPendingTime() const { return m_pending_time; }
  SimulationTime GetNextEventTime() const { return m_next_event_time; }
  SimulationTime GetTotalEmulatedTime() const { return m_total_emulated_time; }
  void ResetTotalEmulatedTime() { m_total_emulated_time = 0; }

  // Adds pending cycles, and invokes events if necessary.
  void AddPendingTime(SimulationTime time);

  // Active event management
  void AddActiveEvent(TimingEvent* event);
  void RemoveActiveEvent(TimingEvent* event);
  void SortEvents();
  void RunEvents();

  // Event lookup, use with care.
  // If you modify an event, call SortEvents afterwards.
  TimingEvent* FindActiveEvent(const char* name);

  // Event enumeration, use with care.
  // Don't remove an event while enumerating the list, as it will invalidate the iterator.
  template<typename T>
  void EnumerateActiveEvents(T callback) const
  {
    for (const TimingEvent* ev : m_events)
      callback(ev);
  }

  // Create an event that executes x times a second.
  std::unique_ptr<TimingEvent> CreateFrequencyEvent(const char* name, float frequency, TimingEventCallback callback,
                                                    bool active = true);

private:
  void UpdateNextEventTime();

  std::vector<TimingEvent*> m_events;
  SimulationTime m_pending_time = 0;
  SimulationTime m_next_event_time = 0;
  SimulationTime m_total_emulated_time = 0;
  bool m_running_events = false;
  bool m_needs_sort = false;
};

class TimingEvent
{
  friend TimingManager;
  friend class Clock;

public:
  // Managed pointer type
  using Pointer = std::unique_ptr<TimingEvent>;

  TimingEvent(TimingManager* manager, const char* name, float frequency, SimulationTime cycle_period,
              CycleCount interval, TimingEventCallback callback);
  ~TimingEvent();

  TimingManager* GetManager() const { return m_manager; }
  const String& GetName() const { return m_name; }
  bool IsActive() const { return m_active; }

  // Returns the frequency of a single cycle for this event.
  float GetFrequency() const { return m_frequency; }
  SimulationTime GetCyclePeriod() const { return m_cycle_period; }

  // Returns the number of cycles between each event.
  CycleCount GetInterval() const { return m_interval; }

  SimulationTime GetDownCount() const { return m_downcount; }

  // Includes pending time.
  SimulationTime GetTimeSinceLastExecution() const;
  CycleCount GetCyclesSinceLastExecution() const;

  void Reschedule(CycleCount cycles);

  void Reset();

  // Services the event with the current accmulated time. If force is set, when not enough time is pending to
  // simulate a single cycle, the callback will still be invoked, otherwise it won't be.
  void InvokeEarly(bool force = false);

  // Schedules the event after the specified number of cycles.
  // cycles is in clock cycles, not source cycles.
  // Do not call within a callback, use Reschedule instead.
  void Activate();
  void Queue(CycleCount cycles);

  // Deactivates the event, preventing it from firing again.
  // Do not call within a callback, return Deactivate instead.
  void Deactivate();

  // Changing frequency of hz events.
  void SetFrequency(float new_frequency, uint32 interval = 1);

  // Changing active state.
  void SetActive(bool active);

private:
  TimingManager* m_manager;
  String m_name;

  float m_frequency;
  SimulationTime m_cycle_period;
  CycleCount m_interval;

  SimulationTime m_downcount;
  SimulationTime m_time_since_last_run;

  TimingEventCallback m_callback;
  bool m_active;
};
