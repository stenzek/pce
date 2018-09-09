#include "clock.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"

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
