#pragma once
#include "YBaseLib/Common.h"

class Event
{
public:
  Event(bool autoReset = false) : m_autoReset(autoReset), m_set(false) {}

  ~Event() {}

  void Signal() { m_set = true; }

  void Wait()
  {
    while (!m_set)
      ;

    if (m_autoReset)
      Reset();
  }

  bool TryWait(uint32 timeout)
  {
    if (!m_set)
      return false;

    if (m_autoReset)
      Reset();

    return true;
  }

  void Reset() { m_set = false; }

  static void WaitForMultipleEvents(Event** ppEvents, uint32 nEvents)
  {
    for (uint32 i = 0; i < nEvents; i++)
    {
      while (!ppEvents[i]->m_set)
        ;
      if (ppEvents[i]->m_autoReset)
        ppEvents[i]->m_set = false;
    }
  }

private:
  bool m_autoReset;
  volatile bool m_set;
};
