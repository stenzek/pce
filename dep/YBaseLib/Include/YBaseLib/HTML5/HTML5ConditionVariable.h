#pragma once
#include "YBaseLib/Common.h"

class Mutex;
class RecursiveMutex;
// class ReadWriteLock;

class ConditionVariable
{
public:
  ConditionVariable();
  ~ConditionVariable();

  void SleepAndRelease(Mutex* pMutex);
  void SleepAndRelease(RecursiveMutex* pMutex);

  void WakeAll();
  void Wake();

private:
};
