#pragma once
#include "YBaseLib/Atomic.h"
#include "YBaseLib/Event.h"

class Barrier
{
public:
  Barrier(uint32 threadCount) : m_threadCount(threadCount) { pthread_barrier_init(&m_barrier, nullptr, threadCount); }

  ~Barrier() { pthread_barrier_destroy(&m_barrier); }

  const uint32 GetThreadCount() const { return m_threadCount; }
  void SetThreadCount(uint32 threadCount)
  {
    pthread_barrier_destroy(&m_barrier);
    pthread_barrier_init(&m_barrier, nullptr, threadCount);
    m_threadCount = threadCount;
  }

  void Wait() { pthread_barrier_wait(&m_barrier); }

private:
  uint32 m_threadCount;
  pthread_barrier_t m_barrier;
};
