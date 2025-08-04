#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)

#include "YBaseLib/Assert.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Windows/WindowsThread.h"
#include <process.h>
Log_SetChannel(Thread);

Thread::Thread() : m_hThread(NULL), m_threadId(0), m_bSuspended(false) {}

Thread::~Thread()
{
  if (m_hThread != NULL)
    CloseHandle(m_hThread);
}

const bool Thread::IsRunning() const
{
  if (m_hThread == NULL)
    return false;

  DWORD waitResult = WaitForSingleObject(m_hThread, 0);
  return (waitResult == WAIT_TIMEOUT);
}

void Thread::Suspend()
{
  Assert(m_hThread != NULL);
  Assert(!m_bSuspended);
  // DebugAssert(GetCurrentThreadId() != m_iThreadId);

  SuspendThread(m_hThread);
  m_bSuspended = true;
}

void Thread::Resume()
{
  Assert(m_hThread != NULL);
  Assert(m_bSuspended);
  DebugAssert(GetCurrentThreadId() != m_threadId);

  ResumeThread(m_hThread);
  m_bSuspended = false;
}

bool Thread::Start(bool createSuspended /* = false */)
{
  HANDLE hThread;
  unsigned threadId;

  const uint32 createFlags = CREATE_SUSPENDED;

  hThread = (HANDLE)_beginthreadex(NULL, 0, __ThreadStart, reinterpret_cast<void*>(this), createFlags, &threadId);
  if (hThread == NULL)
    return false;

  m_hThread = hThread;
  m_threadId = static_cast<uint32>(threadId);
  m_bSuspended = createSuspended;
  MemoryBarrier();

  if (!createSuspended)
    ResumeThread(m_hThread);

  return true;
}

int32 Thread::Join()
{
  Assert(m_hThread != NULL);

  DWORD waitResult = WaitForSingleObject(m_hThread, INFINITE);
  if (waitResult != WAIT_OBJECT_0)
  {
    Panic("Thread::Join: WaitForSingleObject call failed.");
    return -1;
  }

  DWORD exitCode;
  if (!GetExitCodeThread(m_hThread, &exitCode))
  {
    Panic("Could not obtain thread exit code.");
    return -1;
  }

  return static_cast<int32>(exitCode);
}

void Thread::SetDebugName(const char* threadName)
{
#if defined(Y_COMPILER_MSVC)
  const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
  typedef struct tagTHREADNAME_INFO
  {
    DWORD dwType;     // Must be 0x1000.
    LPCSTR szName;    // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags;    // Reserved for future use, must be zero.
  } THREADNAME_INFO;
#pragma pack(pop)

  THREADNAME_INFO threadNameInfo;
  threadNameInfo.dwType = 0x1000;
  threadNameInfo.szName = threadName;
  threadNameInfo.dwThreadID = m_threadId;
  threadNameInfo.dwFlags = 0;

  __try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(threadNameInfo) / sizeof(ULONG_PTR), (ULONG_PTR*)&threadNameInfo);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  }
#endif
}

unsigned __stdcall Thread::__ThreadStart(void* pArguments)
{
  Thread* pThread = static_cast<Thread*>(pArguments);

  // run the thread
  int threadExitCode = pThread->ThreadEntryPoint();

  // return the exit code
  _endthreadex(static_cast<unsigned>(threadExitCode));

  // not reached
  return 0;
}

int Thread::ThreadEntryPoint()
{
  return 0;
}

Thread::ThreadIdType Thread::GetCurrentThreadId()
{
  return static_cast<ThreadIdType>(::GetCurrentThreadId());
}

void Thread::Yield()
{
  SwitchToThread();
};

void Thread::Sleep(uint32 millisecondsToSleep)
{
  ::Sleep(millisecondsToSleep);
}

#endif
