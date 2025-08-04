#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_HTML5)

#include "YBaseLib/HTML5/HTML5ReadWriteLock.h"

ReadWriteLock::ReadWriteLock() {}

ReadWriteLock::~ReadWriteLock() {}

void ReadWriteLock::LockShared() {}

void ReadWriteLock::LockExclusive() {}

void ReadWriteLock::UnlockShared() {}

void ReadWriteLock::UnlockExclusive() {}

bool ReadWriteLock::TryLockShared()
{
  return true;
}

bool ReadWriteLock::TryLockExclusive()
{
  return true;
}

void ReadWriteLock::UpgradeSharedLockToExclusive() {}

#endif
