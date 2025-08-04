#pragma once
#include "YBaseLib/Common.h"

class Mutex;
class RecursiveMutex;

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
  pthread_cond_t m_ConditionVariable;
};
