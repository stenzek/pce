#pragma once
#include "YBaseLib/Assert.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Windows/WindowsHeaders.h"

class Mutex
{
  friend class ConditionVariable;

public:
  Mutex() { InitializeCriticalSection(&m_CriticalSection); }

  ~Mutex() { DeleteCriticalSection(&m_CriticalSection); }

  void Lock()
  {
    EnterCriticalSection(&m_CriticalSection);
    DebugAssert(m_CriticalSection.RecursionCount == 1);
  }

  bool TryLock()
  {
    if (TryEnterCriticalSection(&m_CriticalSection) == TRUE)
    {
      DebugAssert(m_CriticalSection.RecursionCount == 1);
      return true;
    }

    return false;
  }

  void Unlock()
  {
    DebugAssert(m_CriticalSection.RecursionCount == 1);
    LeaveCriticalSection(&m_CriticalSection);
  }

private:
  CRITICAL_SECTION m_CriticalSection;
};
