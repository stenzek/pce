#pragma once
#include "YBaseLib/Assert.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/Windows/WindowsHeaders.h"

class Event
{
public:
  Event(bool autoReset = false)
  {
    m_hEvent = CreateEvent(NULL, autoReset ? FALSE : TRUE, FALSE, NULL);
    Assert(m_hEvent != NULL);
  }

  ~Event() { CloseHandle(m_hEvent); }

  void Signal() { SetEvent(m_hEvent); }

  void Wait() { WaitForSingleObject(m_hEvent, INFINITE); }

  bool TryWait(uint32 timeout) { return (WaitForSingleObject(m_hEvent, timeout) == WAIT_OBJECT_0); }

  void Reset() { ResetEvent(m_hEvent); }

  static void WaitForMultipleEvents(Event** ppEvents, uint32 nEvents)
  {
    DebugAssert(nEvents > 0);

    uint32 i;
    HANDLE* hEvents = (HANDLE*)alloca(sizeof(HANDLE) * nEvents);
    for (i = 0; i < nEvents; i++)
      hEvents[i] = ppEvents[i]->m_hEvent;

    WaitForMultipleObjects(nEvents, hEvents, TRUE, INFINITE);
  }

private:
  HANDLE m_hEvent;
};
