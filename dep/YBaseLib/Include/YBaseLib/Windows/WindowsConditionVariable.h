#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/Windows/WindowsHeaders.h"

class Mutex;
class RecursiveMutex;

class ConditionVariable
{
public:
  ConditionVariable();
  ~ConditionVariable();

  void Sleep();
  void SleepAndRelease(Mutex* pMutex);
  void SleepAndRelease(RecursiveMutex* pMutex);

  void WakeAll();
  void Wake();

private:
  CONDITION_VARIABLE m_ConditionVariable;
};
