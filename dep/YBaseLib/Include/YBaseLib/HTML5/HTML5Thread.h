#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/ReferenceCounted.h"

class Thread
{
public:
  typedef unsigned int ThreadHandleType;
  typedef unsigned int ThreadIdType;

public:
  Thread();
  virtual ~Thread();

  const bool IsRunning() const { return false; }
  // const bool IsSuspended() const { return false; }

  const ThreadHandleType GetThreadHandle() const { return 0; }
  const ThreadIdType GetThreadId() const { return 0; }

  // void Suspend();
  // void Resume();

  bool Start(bool createSuspended = false);
  int32 Join();

protected:
  // entry point of the thread
  virtual int ThreadEntryPoint();

  // thread name in debugger
  void SetDebugName(const char* threadName);

public:
  // public methods
  static ThreadIdType GetCurrentThreadId();
  static void Yield();
  static void Sleep(uint32 millisecondsToSleep);
};
