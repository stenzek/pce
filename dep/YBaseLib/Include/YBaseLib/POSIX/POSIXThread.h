#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/ReferenceCounted.h"
#include <pthread.h>

class Thread
{
public:
  typedef pthread_t ThreadHandleType;
  typedef pthread_t ThreadIdType;

public:
  Thread();
  virtual ~Thread();

  const bool IsRunning() const;
  // const bool IsSuspended() const { return m_bSuspended; }

  const ThreadHandleType GetThreadHandle() const { return m_threadHandle; }
  const ThreadIdType GetThreadId() const { return m_threadHandle; }

  // void Suspend();
  // void Resume();

  bool Start(bool createSuspended = false);
  int32 Join();

protected:
  // entry point of the thread
  virtual int ThreadEntryPoint();

  // thread name in debugger
  void SetDebugName(const char* threadName);

private:
  // platform-specific
  ThreadHandleType m_threadHandle;
  bool m_bRunning;

  // internal method
  static void* __ThreadStart(void* pArguments);

public:
  // public methods
  static ThreadIdType GetCurrentThreadId();
  static void Yield();
  static void Sleep(uint32 millisecondsToSleep);
};
