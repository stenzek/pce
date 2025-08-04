#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_POSIX)

#include "YBaseLib/POSIX/POSIXReadWriteLock.h"

ReadWriteLock::ReadWriteLock()
{
  pthread_rwlock_init(&m_ReadWriteLock, NULL);
}

ReadWriteLock::~ReadWriteLock()
{
  pthread_rwlock_destroy(&m_ReadWriteLock);
}

void ReadWriteLock::LockShared()
{
  pthread_rwlock_rdlock(&m_ReadWriteLock);
}

void ReadWriteLock::LockExclusive()
{
  pthread_rwlock_wrlock(&m_ReadWriteLock);
}

void ReadWriteLock::UnlockShared()
{
  pthread_rwlock_unlock(&m_ReadWriteLock);
}

void ReadWriteLock::UnlockExclusive()
{
  pthread_rwlock_unlock(&m_ReadWriteLock);
}

bool ReadWriteLock::TryLockShared()
{
  return (pthread_rwlock_tryrdlock(&m_ReadWriteLock) == 0);
}

bool ReadWriteLock::TryLockExclusive()
{
  return (pthread_rwlock_trywrlock(&m_ReadWriteLock) == 0);
}

void ReadWriteLock::UpgradeSharedLockToExclusive()
{
  // windows provides no api for this, so it may be slow dependant on contention
  pthread_rwlock_unlock(&m_ReadWriteLock);
  pthread_rwlock_wrlock(&m_ReadWriteLock);
}

#endif
