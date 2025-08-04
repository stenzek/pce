#pragma once
#include "YBaseLib/Mutex.h"
#include "YBaseLib/NonCopyable.h"

class MutexLock
{
  DeclareNonCopyable(MutexLock);

public:
  MutexLock(Mutex& mutex) : m_mutex(mutex)
  {
    m_mutex.Lock();
    m_locked = true;
  }

  ~MutexLock()
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
  Mutex& m_mutex;
  bool m_locked;
};
