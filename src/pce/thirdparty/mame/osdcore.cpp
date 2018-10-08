// license:BSD-3-Clause
// copyright-holders:Aaron Giles

#include "osdcore.h"
#include <chrono>
#include <thread>

//============================================================
//  osd_ticks
//============================================================

osd_ticks_t osd_ticks(void)
{
  return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

//============================================================
//  osd_ticks_per_second
//============================================================

osd_ticks_t osd_ticks_per_second(void)
{
  return std::chrono::high_resolution_clock::period::den / std::chrono::high_resolution_clock::period::num;
}

//============================================================
//  osd_sleep
//============================================================

void osd_sleep(osd_ticks_t duration)
{
  std::this_thread::sleep_for(std::chrono::high_resolution_clock::duration(duration));
}
