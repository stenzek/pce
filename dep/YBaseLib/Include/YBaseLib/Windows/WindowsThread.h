#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/ReferenceCounted.h"
#include "YBaseLib/Windows/WindowsHeaders.h"

class Thread
{
public:
  typedef HANDLE ThreadHandleType;
  typedef DWORD ThreadIdType;

public:
  Thread();
  virtual ~Thread();

  const bool IsRunning() const;
  const bool IsSuspended() const { return m_bSuspended; }

  const ThreadHandleType GetThreadHandle() const { return m_hThread; }
  ThreadIdType GetThreadId() const { return m_threadId; }

  void Suspend();
  void Resume();

  bool Start(bool createSuspended = false);
  int32 Join();

protected:
  // entry point of the thread
  virtual int ThreadEntryPoint();

  // thread name in debugger
  void SetDebugName(const char* threadName);

private:
  // platform-specific
  ThreadHandleType m_hThread;
  ThreadIdType m_threadId;
  bool m_bSuspended;

  // internal method
  static unsigned __stdcall __ThreadStart(void* pArguments);

public:
  // public methods
  static ThreadIdType GetCurrentThreadId();
  static void Yield();
  static void Sleep(uint32 millisecondsToSleep);
};
