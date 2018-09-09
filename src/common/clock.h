#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "YBaseLib/String.h"

#include "timing.h"
#include "types.h"

// Notes:
// - TODO: A negative frequency here indicates a multiplier rather than divider.
// - Events will only be signaled at the clock source frequency.
// - TODO: Rename Source to something better..
// - TODO: floats?
// - TODO: Conversion helpers from time to cycles for time-based events
// - TODO: Allow CPU to run on a different divider/multiplier
// - TODO: Alter event interval while maintaining partial cycles, but with new interval

class Clock
{
public:
  Clock(const char* name, float frequency);
  ~Clock();

  TimingManager* GetManager() const { return m_manager; }
  void SetManager(TimingManager* manager) { m_manager = manager; }

  const String& GetName() const { return m_name; }
  float GetFrequency() const { return m_frequency; }

  // Named NewEvent because CreateEvent conflicts with windows.h.
  std::unique_ptr<TimingEvent> NewEvent(const char* name, CycleCount cycles, TimingEventCallback callback,
                                        bool active = true);

  // Frequency events don't use clock cycles, but instead execute hz times a second.
  std::unique_ptr<TimingEvent> NewFrequencyEvent(const char* name, float frequency, TimingEventCallback callback,
                                                 bool active = true);

private:
  TimingManager* m_manager = nullptr;
  String m_name;
  float m_frequency;
};
