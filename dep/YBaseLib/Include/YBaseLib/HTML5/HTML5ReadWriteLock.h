#pragma once
#include "YBaseLib/Common.h"

class ReadWriteLock
{
  friend class ConditionVariable;

public:
  ReadWriteLock();
  ~ReadWriteLock();

  void LockShared();
  void LockExclusive();
  void UnlockShared();
  void UnlockExclusive();

  bool TryLockShared();
  bool TryLockExclusive();

  void UpgradeSharedLockToExclusive();

private:
};
