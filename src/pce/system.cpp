#include "system.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "bus.h"
#include "common/state_wrapper.h"
#include "component.h"
#include "cpu.h"
#include "host_interface.h"
#include "save_state_version.h"
Log_SetChannel(System);

DEFINE_OBJECT_TYPE_INFO(System);
BEGIN_OBJECT_PROPERTY_MAP(System)
END_OBJECT_PROPERTY_MAP()

System::System(const ObjectTypeInfo* type_info /* = &s_type_info */) : BaseClass(type_info) {}

System::~System()
{
  // We should be stopped first.
  Assert(m_state == State::Initializing || m_state == State::Stopped);
  for (Component* component : m_components)
    delete component;

  delete m_bus;
}

void System::SetState(State state)
{
  m_state = state;
  if (state != State::Running)
    m_cpu->SetExecutionDowncount(0);
}

void System::AddComponent(Component* component)
{
  m_components.push_back(component);
}

bool System::Initialize()
{
  Assert(m_state == State::Initializing);

  m_bus->Initialize(this);
  for (Component* component : m_components)
  {
    if (!component->Initialize(this, m_bus))
    {
      Log_ErrorPrintf("Component failed to initialize.");
      return false;
    }
  }

  return true;
}

void System::Reset()
{
  m_simulation_time = 0;
  m_last_event_run_time = 0;
  for (TimingEvent* ev : m_events)
    ev->Reset();

  m_bus->Reset();

  for (Component* component : m_components)
    component->Reset();

  UpdateCPUDowncount();
}

bool System::LoadState(ByteStream* stream)
{
  // TODO: Save old state before loading, instead of resetting.
  StateWrapper sw(stream, StateWrapper::Mode::Read);
  return DoAllState(sw);
}

bool System::SaveState(ByteStream* stream)
{
  StateWrapper sw(stream, StateWrapper::Mode::Write);
  return DoAllState(sw);
}

bool System::DoAllState(StateWrapper& sw)
{
  if (!sw.DoMarker("HEADER"))
    return false;

  u32 version = SAVE_STATE_VERSION;
  sw.Do(&version);
  if (version != SAVE_STATE_VERSION)
  {
    Log_ErrorPrintf("Incorrect save state version");
    return false;
  }

  SmallString system_class_name(m_type_info->GetTypeName());
  sw.Do(&system_class_name);
  if (system_class_name != m_type_info->GetTypeName())
  {
    Log_ErrorPrintf("System class mismatch, we are '%s' and the save state is '%s'", m_type_info->GetTypeName(),
                    system_class_name.GetCharArray());
    return false;
  }

  // Global state
  sw.Do(&m_simulation_time);
  sw.Do(&m_last_event_run_time);
  if (sw.HasError())
    return false;

  // Load system (this class) state, then the bus state
  if (!sw.DoMarker("SYSTEM") || !DoState(sw))
    return false;

  // Load bus state next
  if (!sw.DoMarker("BUS") || !m_bus->DoState(sw))
    return false;

  // Then the components
  if (!sw.DoMarker("COMPONENTS") || !DoComponentsState(sw))
    return false;

  // Finally, load events
  if (!sw.DoMarker("EVENTS") || !DoEventsState(sw))
    return false;

  return !sw.HasError();
}

bool System::DoComponentsState(StateWrapper& sw)
{
  u32 num_components = static_cast<u32>(m_components.size());
  sw.Do(&num_components);
  if (sw.HasError() || num_components != m_components.size())
  {
    Log_ErrorPrintf("Incorrect number of components");
    return false;
  }
  for (Component* component : m_components)
  {
    if (!component->DoState(sw))
      return false;
  }

  return !sw.HasError();
}

bool System::LoadSystemState(BinaryReader& reader)
{
  return true;
}

bool System::SaveSystemState(BinaryWriter& writer)
{
  return true;
}

bool System::DoState(StateWrapper& sw)
{
  if (sw.IsReading())
  {
    BinaryReader bw(sw.GetStream());
    return LoadSystemState(bw);
  }
  else
  {
    BinaryWriter bw(sw.GetStream());
    return SaveSystemState(bw);
  }
}

void System::Run()
{
  m_cpu->Execute();
  m_interrupt_execution.store(false);
}

void System::InterruptRunLoop()
{
  m_interrupt_execution.store(true);
}

std::pair<std::unique_ptr<byte[]>, u32> System::ReadFileToBuffer(const char* filename, u32 offset, u32 expected_size)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &stream))
  {
    Log_ErrorPrintf("Failed to open ROM file: %s", filename);
    return std::make_pair(std::unique_ptr<byte[]>(), 0);
  }

  const u32 size = Truncate32(stream->GetSize());
  if (expected_size != 0 && (offset >= size || (size - offset) != expected_size))
  {
    Log_ErrorPrintf("ROM file %s mismatch - expected %u bytes, got %u bytes", filename, expected_size, size);
    stream->Release();
    return std::make_pair(std::unique_ptr<byte[]>(), 0);
  }

  std::unique_ptr<byte[]> data = std::make_unique<byte[]>(size - offset);
  if ((offset > 0 && !stream->SeekAbsolute(offset)) || !stream->Read2(data.get(), size - offset))
  {
    Log_ErrorPrintf("Failed to read %u bytes from ROM file %s", size - offset, filename);
    stream->Release();
    return std::make_pair(std::unique_ptr<byte[]>(), 0);
  }

  stream->Release();
  return std::make_pair(std::move(data), size - offset);
}

String System::GetMiscDataFilename(const char* suffix) const
{
  return suffix ? String::FromFormat("%s%s", m_base_path.GetCharArray(), suffix) : m_base_path;
}

std::unique_ptr<TimingEvent> System::CreateClockedEvent(const char* name, float frequency, CycleCount interval,
                                                        TimingEventCallback callback, bool activate)
{
  SimulationTime cycle_period = SimulationTime(double(1000000000.0) / double(frequency));
  auto evt = std::make_unique<TimingEvent>(this, name, frequency, cycle_period, interval, std::move(callback));
  if (activate)
    evt->Activate();

  return evt;
}

std::unique_ptr<TimingEvent> System::CreateFrequencyEvent(const char* name, float frequency,
                                                          TimingEventCallback callback, bool activate)
{
  SimulationTime cycle_period = SimulationTime(double(1000000000.0) / double(frequency));
  auto evt = std::make_unique<TimingEvent>(this, name, frequency, cycle_period, 1, std::move(callback));
  if (activate)
    evt->Activate();

  return evt;
}

std::unique_ptr<TimingEvent> System::CreateMillisecondEvent(const char* name, CycleCount ms,
                                                            TimingEventCallback callback, bool activate)
{
  auto evt = std::make_unique<TimingEvent>(this, name, 1000.0f, 1000000000 / 1000, ms, std::move(callback));
  if (activate)
    evt->Activate();

  return evt;
}

std::unique_ptr<TimingEvent> System::CreateMicrosecondEvent(const char* name, CycleCount us,
                                                            TimingEventCallback callback, bool activate)
{
  auto evt = std::make_unique<TimingEvent>(this, name, 1000000.0f, 1000000000 / 1000000, us, std::move(callback));
  if (activate)
    evt->Activate();

  return evt;
}

std::unique_ptr<TimingEvent> System::CreateNanosecondEvent(const char* name, CycleCount ns,
                                                           TimingEventCallback callback, bool activate)
{
  auto evt = std::make_unique<TimingEvent>(this, name, 1000.0f, 1000000000 / 1000000000, ns, std::move(callback));
  if (activate)
    evt->Activate();

  return evt;
}

static bool CompareEvents(const TimingEvent* lhs, const TimingEvent* rhs)
{
  return lhs->GetDownCount() > rhs->GetDownCount();
}

void System::AddActiveEvent(TimingEvent* event)
{
  m_events.push_back(event);
  if (!m_running_events)
  {
    std::push_heap(m_events.begin(), m_events.end(), CompareEvents);
    UpdateCPUDowncount();
  }
  else
  {
    m_events_need_sorting = true;
  }
}

void System::RemoveActiveEvent(TimingEvent* event)
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
    UpdateCPUDowncount();
  }
  else
  {
    m_events_need_sorting = true;
  }
}

TimingEvent* System::FindActiveEvent(const char* name)
{
  auto iter = std::find_if(m_events.begin(), m_events.end(), [&name](auto& ev) { return ev->GetName().Compare(name); });

  return (iter != m_events.end()) ? *iter : nullptr;
}

void System::SortEvents()
{
  if (!m_running_events)
  {
    std::make_heap(m_events.begin(), m_events.end(), CompareEvents);
    UpdateCPUDowncount();
  }
  else
  {
    m_events_need_sorting = true;
  }
}

void System::RunEvents()
{
  DebugAssert(!m_running_events);
  if (m_events.empty())
  {
    // on the off chance that we don't have any events...
    m_last_event_run_time = m_simulation_time;
    return;
  }

  SimulationTime remaining_time = GetSimulationTimeSince(m_last_event_run_time);
  if (remaining_time < m_events.front()->GetDownCount())
  {
    // no need to run events yet.
    m_cpu->SetExecutionDowncount(m_events.front()->GetDownCount() - remaining_time);
    return;
  }

  m_running_events = true;

  while (remaining_time > 0)
  {
    // To avoid issues where two events are related to each other from becoming desynced,
    // we run at a slice that is the length of the lowest next event time.
    SimulationTime time = std::min(remaining_time, m_events.front()->GetDownCount());
    remaining_time -= time;
    m_last_event_run_time += time;

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
      DebugAssert(cycles_to_execute >= 0);

      // Calculate and set the new downcount for periodic events, taking into account late time.
      CycleCount cycles_late = time_late / evt->m_cycle_period;

      // Factor late time into the time for the next invocation.
      evt->m_downcount += (evt->m_cycle_period * evt->m_interval);
      evt->m_time_since_last_run -= cycles_to_execute * evt->m_cycle_period;

      // The cycles_late is only an indicator, it doesn't modify the cycles to execute.
      evt->m_callback(evt, cycles_to_execute, cycles_late);

      // Place it in the appropriate position in the queue.
      if (m_events_need_sorting)
      {
        // Another event may have been changed by this event, or the interval/downcount changed.
        std::make_heap(m_events.begin(), m_events.end(), CompareEvents);
        m_events_need_sorting = false;
      }
      else
      {
        // Keep the event list in a heap. The event we just serviced will be in the last place,
        // so we can use push_here instead of make_heap, which should be faster.
        std::push_heap(m_events.begin(), m_events.end(), CompareEvents);
      }
    }

    // Run until next event, or 100ms.
    UpdateCPUDowncount();
  }

  DebugAssert(m_last_event_run_time == m_simulation_time);
  m_running_events = false;
}

void System::UpdateCPUDowncount()
{
  SimulationTime next_event_time = m_events.empty() ? POLL_FREQUENCY : m_events.front()->GetDownCount();
  next_event_time -= GetSimulationTimeSince(m_last_event_run_time);
  m_cpu->SetExecutionDowncount(std::max(next_event_time, SimulationTime(0)));
}

bool System::DoEventsState(StateWrapper& sw)
{
  if (sw.IsReading())
  {
    // Load timestamps for the clock events.
    // Any oneshot events should be recreated by the load state method, so we can fix up their times here.
    u32 event_count = 0;
    sw.Do(&event_count);

    for (u32 i = 0; i < event_count; i++)
    {
      SmallString event_name;
      float frequency;
      SimulationTime cycle_period;
      CycleCount interval;
      SimulationTime downcount, time_since_last_run;
      sw.Do(&event_name);
      sw.Do(&frequency);
      sw.Do(&cycle_period);
      sw.Do(&interval);
      sw.Do(&downcount);
      sw.Do(&time_since_last_run);
      if (sw.HasError())
        return false;

      TimingEvent* event = FindActiveEvent(event_name);
      if (!event)
      {
        Log_WarningPrintf("Save state has event '%s', but couldn't find this event when loading.",
                          event_name.GetCharArray());
        continue;
      }

      // Using reschedule is safe here since we call sort afterwards.
      event->m_frequency = frequency;
      event->m_cycle_period = cycle_period;
      event->m_interval = interval;
      event->m_downcount = downcount;
      event->m_time_since_last_run = time_since_last_run;
    }

    Log_DevPrintf("Loaded %u events from save state.", event_count);
    SortEvents();
  }
  else
  {
    u32 event_count = static_cast<u32>(m_events.size());
    sw.Do(&event_count);

    for (TimingEvent* evt : m_events)
    {
      sw.Do(&evt->m_name);
      sw.Do(&evt->m_frequency);
      sw.Do(&evt->m_cycle_period);
      sw.Do(&evt->m_interval);
      sw.Do(&evt->m_downcount);
      sw.Do(&evt->m_time_since_last_run);
    }

    Log_DevPrintf("Wrote %u events to save state.", event_count);
  }

  return !sw.HasError();
}
