#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/Windows/WindowsHeaders.h"

class ReadWriteLock
{
  friend class ConditionVariable;

public:
  ReadWriteLock();
  ~ReadWriteLock();

  void LockShared();
  void LockExclusive();

  bool TryLockShared();
  bool TryLockExclusive();

  void UnlockShared();
  void UnlockExclusive();

  void UpgradeSharedLockToExclusive();

private:
  SRWLOCK m_ReadWriteLock;
};
