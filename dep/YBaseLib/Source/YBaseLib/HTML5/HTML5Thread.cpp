#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_HTML5)

#include "YBaseLib/Assert.h"
#include "YBaseLib/HTML5/HTML5Thread.h"
#include "YBaseLib/Log.h"
// Log_SetChannel(Thread);

Thread::Thread() {}

Thread::~Thread() {}

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
  Panic("attempt to start thread");
  return false;
}

int32 Thread::Join()
{
  Panic("attempt to join thread");
  return 0;
}

int Thread::ThreadEntryPoint()
{
  return 0;
}

void Thread::SetDebugName(const char* threadName) {}

Thread::ThreadIdType Thread::GetCurrentThreadId()
{
  return 0;
}

void Thread::Yield(){
  // sched_yield();
};

void Thread::Sleep(uint32 millisecondsToSleep)
{
  // usleep(millisecondsToSleep * 1000);
}

#endif
