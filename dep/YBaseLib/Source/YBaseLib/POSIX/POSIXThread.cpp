#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_POSIX)

#include "YBaseLib/Assert.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/POSIX/POSIXThread.h"
#include <unistd.h>
// Log_SetChannel(Thread);

Thread::Thread() : m_threadHandle(0), m_bRunning(false) {}

Thread::~Thread()
{
  if (m_threadHandle != 0)
    pthread_detach(m_threadHandle);
}

const bool Thread::IsRunning() const
{
  if (m_threadHandle == 0)
    return false;

  return m_bRunning;
}

// void Thread::Suspend()
// {
//     Assert(m_hThread != NULL);
//     Assert(!m_bSuspended);
//     //DebugAssert(GetCurrentThreadId() != m_iThreadId);
//
//     SuspendThread(m_hThread);
//     m_bSuspended = true;
// }
//
// void Thread::Resume()
// {
//     Assert(m_hThread != NULL);
//     Assert(m_bSuspended);
//     DebugAssert(GetCurrentThreadId() != m_iThreadId);
//
//     ResumeThread(m_hThread);
//     m_bSuspended = false;
// }

bool Thread::Start(bool createSuspended /* = false */)
{
  int result = pthread_create(&m_threadHandle, NULL, __ThreadStart, reinterpret_cast<void*>(this));
  if (result != 0)
    return false;

  return true;
}

int32 Thread::Join()
{
  Assert(m_threadHandle != 0);

  void* threadExitCode;
  int result = pthread_join(m_threadHandle, &threadExitCode);
  if (result < 0)
  {
    Panic("Thread::Join: pthread_join failed.");
    return -1;
  }

  // remove the thread handle, since join closes its resources
  m_threadHandle = 0;

  // return the result
  return *(int32*)&threadExitCode;
}

void Thread::SetDebugName(const char* threadName) {}

void* Thread::__ThreadStart(void* pArguments)
{
  Thread* pThread = static_cast<Thread*>(pArguments);
  pThread->m_bRunning = true;
  MemoryBarrier();

  // run the thread
  int threadExitCode = pThread->ThreadEntryPoint();

  // unset running flag
  pThread->m_bRunning = false;
  MemoryBarrier();

  // return the exit code
  pthread_exit(reinterpret_cast<void*>(threadExitCode));

  // not reached
  return 0;
}

int Thread::ThreadEntryPoint()
{
  return 0;
}

Thread::ThreadIdType Thread::GetCurrentThreadId()
{
  return pthread_self();
}

void Thread::Yield()
{
  sched_yield();
};

void Thread::Sleep(uint32 millisecondsToSleep)
{
  usleep(millisecondsToSleep * 1000);
}

#endif
