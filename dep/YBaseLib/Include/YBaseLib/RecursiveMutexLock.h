#pragma once
#include "YBaseLib/NonCopyable.h"
#include "YBaseLib/RecursiveMutex.h"

class RecursiveMutexLock
{
  DeclareNonCopyable(RecursiveMutexLock);

public:
  RecursiveMutexLock(RecursiveMutex& mutex) : m_mutex(mutex)
  {
    m_mutex.Lock();
    m_locked = true;
  }

  ~RecursiveMutexLock()
  {
    if (m_locked)
      m_mutex.Unlock();
  }

  void Lock()
  {
    if (m_locked)
      return;

    m_mutex.Lock();
    m_locked = true;
  }

  bool TryLock()
  {
    if (m_locked)
      return true;

    if (m_mutex.TryLock())
    {
      m_locked = true;
      return true;
    }

    return false;
  }

  void Unlock()
  {
    if (!m_locked)
      return;

    m_mutex.Unlock();
    m_locked = false;
  }

private:
  RecursiveMutex& m_mutex;
  bool m_locked;
};
