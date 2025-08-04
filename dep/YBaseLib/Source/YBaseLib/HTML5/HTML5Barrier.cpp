#include "YBaseLib/Assert.h"
#include "YBaseLib/Barrier.h"
#include "YBaseLib/Log.h"

#ifdef Y_PLATFORM_HTML5
Log_SetChannel(HTML5Barrier);

#define MAX_THREADS 1024

Barrier::Barrier(uint32 threadCount) : m_threadCount(threadCount), m_enteredThreadCount(0), m_exitedThreadCount(0)
{
  // sem_init(&m_enterSemaphore, 0, 0);
  // sem_init(&m_exitSemaphore, 0, 0);
}

Barrier::~Barrier()
{
  // sem_destroy(&m_exitSemaphore);
  // sem_destroy(&m_enterSemaphore);
}

void Barrier::SetThreadCount(uint32 threadCount)
{
  Assert(m_enteredThreadCount == 0);
  m_threadCount = threadCount;
}

void Barrier::Wait()
{
  Log_WarningPrint("Barrier::Wait unimplemented on HTML5.");

  if (Y_AtomicIncrement(m_enteredThreadCount) == m_threadCount)
  {
    m_exitedThreadCount = 0;
    // sem_post(&m_enterSemaphore);
  }
  else
  {
    // sem_wait(&m_exitSemaphore);
  }

  if (Y_AtomicIncrement(m_exitedThreadCount) == m_threadCount)
  {
    m_enteredThreadCount = 0;
    // sem_post(&m_enterSemaphore);
  }
  else
  {
    // sem_wait(&m_exitSemaphore);
  }
}

#endif // Y_PLATFORM_POSIX
