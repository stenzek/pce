#pragma once
#include "YBaseLib/Common.h"

class Event
{
public:
  Event(bool autoReset = false);
  ~Event();

  void Signal();
  void Wait();
  bool TryWait(uint32 timeout);
  void Reset();

  static void WaitForMultipleEvents(Event** ppEvents, uint32 nEvents);

private:
  int m_pipeFds[2];
  bool m_autoReset;
};
