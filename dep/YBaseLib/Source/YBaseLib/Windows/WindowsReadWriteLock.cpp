#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)

#include "YBaseLib/Windows/WindowsReadWriteLock.h"

ReadWriteLock::ReadWriteLock()
{
  InitializeSRWLock(&m_ReadWriteLock);
}

ReadWriteLock::~ReadWriteLock() {}

void ReadWriteLock::LockShared()
{
  AcquireSRWLockShared(&m_ReadWriteLock);
}

void ReadWriteLock::LockExclusive()
{
  AcquireSRWLockExclusive(&m_ReadWriteLock);
}

bool ReadWriteLock::TryLockShared()
{
  return (TryAcquireSRWLockShared(&m_ReadWriteLock) == TRUE);
}

bool ReadWriteLock::TryLockExclusive()
{
  return (TryAcquireSRWLockExclusive(&m_ReadWriteLock) == TRUE);
}

void ReadWriteLock::UnlockShared()
{
  ReleaseSRWLockShared(&m_ReadWriteLock);
}

void ReadWriteLock::UnlockExclusive()
{
  ReleaseSRWLockExclusive(&m_ReadWriteLock);
}

void ReadWriteLock::UpgradeSharedLockToExclusive()
{
  // windows provides no api for this, so it may be slow dependant on contention
  ReleaseSRWLockShared(&m_ReadWriteLock);
  AcquireSRWLockExclusive(&m_ReadWriteLock);
}

#endif
