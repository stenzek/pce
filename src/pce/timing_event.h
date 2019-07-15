#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "YBaseLib/String.h"

#include "pce/types.h"

class System;
class TimingEvent;

// Event callback type. Second parameter is the number of cycles the event was executed "late".
using TimingEventCallback = std::function<void(TimingEvent* event, CycleCount cycles, CycleCount cycles_late)>;

class TimingEvent
{
  friend System;
  friend class Clock;

public:
  TimingEvent(System* system, const char* name, float frequency, SimulationTime cycle_period, CycleCount interval,
              TimingEventCallback callback);
  ~TimingEvent();

  System* GetSystem() const { return m_system; }
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
  void SetFrequency(float new_frequency, u32 interval = 1);

  // Changing active state.
  void SetActive(bool active);

private:
  System* m_system;
  String m_name;

  float m_frequency;
  SimulationTime m_cycle_period;
  CycleCount m_interval;

  SimulationTime m_downcount;
  SimulationTime m_time_since_last_run;

  TimingEventCallback m_callback;
  bool m_active;
};
