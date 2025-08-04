#pragma once
#include "YBaseLib/Assert.h"
#include "YBaseLib/Common.h"

class Mutex
{
  friend class ConditionVariable;

public:
  Mutex() {}

  ~Mutex() {}

  void Lock() {}

  bool TryLock() { return true; }

  void Unlock() {}

private:
};
