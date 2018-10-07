#pragma once
#include "types.h"

class DisplayRenderer;

class DisplayTiming
{
public:
  DisplayTiming();

  // H/V frequencies are valid?
  bool IsValid() const { return m_valid; }

  // Enables the clock at the specified start time.
  void SetClockEnable(bool enable) { m_clock_enable = enable; }
  void ResetClock(SimulationTime start_time);

  // Accessors.
  u32 GetHorizontalVisible() const { return m_horizontal_visible; }
  u32 GetHorizontalBlankStart() const { return m_horizontal_blank_start; }
  u32 GetHorizontalBlankEnd() const { return m_horizontal_blank_end; }
  u32 GetHorizontalTotal() const { return m_horizontal_total; }
  u32 GetVerticalVisible() const { return m_vertical_visible; }
  u32 GetVerticalBlankStart() const { return m_vertical_blank_start; }
  u32 GetVerticalBlankEnd() const { return m_vertical_blank_end; }
  u32 GetVerticalTotal() const { return m_vertical_total; }
  double GetPixelClock() const { return m_pixel_clock; }
  double GetHorizontalFrequency() const { return m_horizontal_frequency; }
  double GetVerticalFrequency() const { return m_vertical_frequency; }
  u32 GetHorizontalPixelDuration() const { return m_horizontal_pixel_duration; }
  u32 GetHorizontalActiveDuration() const { return m_horizontal_active_duration; }
  u32 GetHorizontalBlankStartTime() const { return m_horizontal_blank_start_time; }
  u32 GetHorizontalBlankEndTime() const { return m_horizontal_blank_end_time; }
  u32 GetHorizontalTotalDuration() const { return m_horizontal_total_duration; }
  u32 GetVerticalActiveDuration() const { return m_vertical_active_duration; }
  u32 GetVerticalBlankStartTime() const { return m_vertical_blank_start_time; }
  u32 GetVerticalBlankEndTime() const { return m_vertical_blank_end_time; }
  u32 GetVerticalTotalDuration() const { return m_vertical_total_duration; }

  // Setting horizontal timing based on pixels and clock.
  void SetPixelClock(double clock) { m_pixel_clock = clock; }
  void SetHorizontalRange(u32 visible, u32 blank_start, u32 blank_end, u32 total);
  void SetVerticalRange(u32 visible, u32 blank_start, u32 blank_end, u32 total);

  // Gets the timing state for the specified time point.
  struct Snapshot
  {
    u32 current_line;
    u32 current_pixel;
    bool display_active; // visible part
    bool in_horizontal_blank;
    bool in_vertical_blank;
    bool hsync_active;
    bool vsync_active;
  };
  Snapshot GetSnapshot(SimulationTime time) const;

  // Shorter versions of the above.
  bool IsDisplayActive(SimulationTime time) const;
  bool InVerticalBlank(SimulationTime time) const;
  u32 GetCurrentLine(SimulationTime time) const;

  // Writes frequency information to the log.
  void LogFrequencies(const char* what) const;

private:
  void UpdateHorizontalFrequency();
  void UpdateVerticalFrequency();
  void UpdateValid();

  SimulationTime m_clock_start_time = 0;

  u32 m_horizontal_visible = 0;
  u32 m_horizontal_blank_start = 0;
  u32 m_horizontal_blank_end = 0;
  u32 m_horizontal_total = 0;
  u32 m_vertical_visible = 0;
  u32 m_vertical_blank_start = 0;
  u32 m_vertical_blank_end = 0;
  u32 m_vertical_total = 0;

  double m_pixel_clock = 0.0;
  double m_horizontal_frequency = 0.0f;
  double m_vertical_frequency = 0.0f;

  // TODO: Make these doubles?
  u32 m_horizontal_pixel_duration = 0;
  u32 m_horizontal_active_duration = 0;
  u32 m_horizontal_blank_start_time = 0;
  u32 m_horizontal_blank_end_time = 0;
  u32 m_horizontal_total_duration = 0;
  u32 m_vertical_active_duration = 0;
  u32 m_vertical_blank_start_time = 0;
  u32 m_vertical_blank_end_time = 0;
  u32 m_vertical_total_duration = 0;

  bool m_clock_enable = false;
  bool m_valid = false;
};
