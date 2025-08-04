#include "YBaseLib/Assert.h"
#include "YBaseLib/Barrier.h"

#ifdef Y_PLATFORM_WINDOWS

#define MAX_THREADS 1024

Barrier::Barrier(uint32 threadCount) : m_threadCount(threadCount), m_enteredThreadCount(0), m_exitedThreadCount(0)
{
  m_hEnterSemaphore = CreateSemaphoreA(NULL, 0, MAX_THREADS, NULL);
  m_hExitSemaphore = CreateSemaphoreA(NULL, 0, MAX_THREADS, NULL);
}

Barrier::~Barrier()
{
  CloseHandle(m_hExitSemaphore);
  CloseHandle(m_hEnterSemaphore);
}

void Barrier::SetThreadCount(uint32 threadCount)
{
  Assert(m_enteredThreadCount == 0);
  m_threadCount = threadCount;
}

void Barrier::Wait()
{
  if (Y_AtomicIncrement(m_enteredThreadCount) == m_threadCount)
  {
    m_exitedThreadCount = 0;
    ReleaseSemaphore(m_hEnterSemaphore, m_threadCount - 1, NULL);
  }
  else
  {
    WaitForSingleObject(m_hEnterSemaphore, INFINITE);
  }

  if (Y_AtomicIncrement(m_exitedThreadCount) == m_threadCount)
  {
    m_enteredThreadCount = 0;
    ReleaseSemaphore(m_hExitSemaphore, m_threadCount - 1, NULL);
  }
  else
  {
    WaitForSingleObject(m_hExitSemaphore, INFINITE);
  }
}

#endif
