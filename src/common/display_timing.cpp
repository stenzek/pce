#include "display_timing.h"
#include "YBaseLib/Log.h"
Log_SetChannel(DisplayTiming);

DisplayTiming::DisplayTiming() = default;

void DisplayTiming::ResetClock(SimulationTime start_time)
{
  m_clock_start_time = start_time;
}

void DisplayTiming::SetHorizontalRange(u32 visible, u32 blank_start, u32 blank_end, u32 total)
{
  m_horizontal_visible = visible;
  m_horizontal_blank_start = blank_start;
  m_horizontal_blank_end = blank_end;
  m_horizontal_total = total;
  UpdateHorizontalFrequency();
}

void DisplayTiming::SetVerticalRange(u32 visible, u32 blank_start, u32 blank_end, u32 total)
{
  m_vertical_visible = visible;
  m_vertical_blank_start = blank_start;
  m_vertical_blank_end = blank_end;
  m_vertical_total = total;
  UpdateVerticalFrequency();
}

DisplayTiming::Snapshot DisplayTiming::GetSnapshot(SimulationTime time) const
{
  Snapshot ss;
  if (m_clock_enable && IsValid())
  {
    const u32 time_in_frame = static_cast<u32>(time % m_vertical_total_duration);
    const u32 line_number = static_cast<u32>(time_in_frame / m_horizontal_total_duration);
    const u32 time_in_line = static_cast<u32>(time_in_frame % m_horizontal_total_duration);
    ss.current_line = line_number;
    ss.current_pixel = static_cast<u32>(time_in_line / m_horizontal_pixel_duration);
    ss.hsync_active = (time_in_line >= m_horizontal_blank_start_time && time_in_line < m_horizontal_blank_end_time);
    ss.in_horizontal_blank = (time_in_line >= m_horizontal_active_duration);
    ss.vsync_active = (line_number >= m_vertical_blank_start && line_number < m_vertical_blank_end);
    ss.in_vertical_blank = (line_number >= m_vertical_visible);
    ss.display_active = !(ss.in_horizontal_blank | ss.in_vertical_blank);
  }
  else
  {
    ss.current_line = 0;
    ss.current_pixel = 0;
    ss.display_active = false;
    ss.in_horizontal_blank = false;
    ss.in_vertical_blank = false;
    ss.hsync_active = false;
    ss.vsync_active = false;
  }
  return ss;
}

bool DisplayTiming::IsDisplayActive(SimulationTime time) const
{
  if (!m_clock_enable || !IsValid())
    return false;

  const u32 time_in_frame = static_cast<u32>(time % m_vertical_total_duration);
  const u32 line_number = static_cast<u32>(time_in_frame / m_horizontal_total_duration);
  const u32 time_in_line = static_cast<u32>(time_in_frame % m_horizontal_total_duration);
  return (line_number < m_vertical_visible && time_in_line < m_horizontal_active_duration);
}

bool DisplayTiming::InVerticalBlank(SimulationTime time) const
{
  if (!m_clock_enable || !IsValid())
    return false;

  const u32 time_in_frame = static_cast<u32>(time % m_vertical_total_duration);
  const u32 line_number = static_cast<u32>(time_in_frame / m_horizontal_total_duration);
  return (line_number >= m_vertical_visible);
}

u32 DisplayTiming::GetCurrentLine(SimulationTime time) const
{
  if (!m_clock_enable || !IsValid())
    return 0;

  const u32 time_in_frame = static_cast<u32>(time % m_vertical_total_duration);
  return static_cast<u32>(time_in_frame / m_horizontal_total_duration);
}

void DisplayTiming::LogFrequencies(const char* what) const
{
  Log_InfoPrintf("%s: horizontal frequency %.3f Khz, vertical frequency %.3f hz", what, m_horizontal_frequency / 1000.0,
                 m_vertical_frequency);
}

void DisplayTiming::UpdateHorizontalFrequency()
{
  if (m_pixel_clock == 0.0 || m_horizontal_visible == 0 || m_horizontal_total == 0 || m_horizontal_blank_start == 0 ||
      m_horizontal_blank_end == 0 || m_horizontal_visible > m_horizontal_total)
  {
    m_horizontal_frequency = 0.0;
    m_horizontal_active_duration = 0;
    m_horizontal_blank_start_time = 0;
    m_horizontal_blank_end_time = 0;
    m_horizontal_total_duration = 0;
    UpdateVerticalFrequency();
    return;
  }

  const double pixel_period = 1.0 / m_pixel_clock;
  const double active_duration_s = pixel_period * static_cast<double>(m_horizontal_visible);
  const double start_time_s = pixel_period * static_cast<double>(m_horizontal_blank_start);
  const double end_time_s = pixel_period * static_cast<double>(m_horizontal_blank_end);
  const double total_duration_s = pixel_period * static_cast<double>(m_horizontal_total);

  m_horizontal_frequency = m_pixel_clock / static_cast<double>(m_horizontal_total);
  m_horizontal_pixel_duration = static_cast<u32>(1000000000.0 * pixel_period);
  m_horizontal_active_duration = static_cast<u32>(1000000000.0 * active_duration_s);
  m_horizontal_blank_start_time = static_cast<u32>(1000000000.0 * start_time_s);
  m_horizontal_blank_end_time = static_cast<u32>(1000000000.0 * end_time_s);
  m_horizontal_total_duration = static_cast<u32>(1000000000.0 * active_duration_s);
  UpdateVerticalFrequency();
}

void DisplayTiming::UpdateVerticalFrequency()
{
  if (m_horizontal_total_duration == 0 || m_vertical_visible == 0 || m_vertical_blank_start == 0 ||
      m_vertical_blank_end == 0 || m_vertical_visible > m_vertical_total)
  {
    m_vertical_frequency = 0;
    m_vertical_active_duration = 0;
    m_vertical_blank_start_time = 0;
    m_vertical_blank_end_time = 0;
    m_vertical_total_duration = 0;
    UpdateValid();
    return;
  }

  // TODO: Handle vblank at top properly..
  u32 vblank_start = m_vertical_blank_start;
  u32 vblank_end = m_vertical_blank_end;
  if (vblank_start < m_vertical_visible)
    vblank_start += m_vertical_total;
  if (vblank_end < m_vertical_blank_start)
    vblank_end += m_vertical_total;

  m_vertical_frequency = m_horizontal_frequency / static_cast<double>(m_vertical_total);
  m_vertical_active_duration = m_horizontal_total_duration * m_vertical_visible;
  m_vertical_blank_start_time = m_horizontal_total_duration * vblank_start;
  m_vertical_blank_end_time = m_horizontal_total_duration * vblank_end;
  m_vertical_total_duration = m_horizontal_total_duration * m_vertical_total;
  UpdateValid();
}

void DisplayTiming::UpdateValid()
{
  m_valid = (m_horizontal_total_duration > 0 && m_vertical_total_duration > 0);
}
