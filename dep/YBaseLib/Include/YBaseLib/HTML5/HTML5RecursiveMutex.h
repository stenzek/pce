#pragma once
#include "YBaseLib/Assert.h"
#include "YBaseLib/Common.h"

class RecursiveMutex
{
  friend class ConditionVariable;

public:
  RecursiveMutex() {}

  ~RecursiveMutex() {}

  void Lock() {}

  bool TryLock() { return true; }

  void Unlock() {}

private:
};
