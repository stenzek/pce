#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)

#include "YBaseLib/Windows/WindowsConditionVariable.h"
#include "YBaseLib/Windows/WindowsMutex.h"
#include "YBaseLib/Windows/WindowsRecursiveMutex.h"

static DWORD GetWaitTimeout()
{
// This gets around a stranage bug. When VS2013 is attached, the thread sleeping does not
// wake, despite multiple WakeConditionVariable calls, until the timeout elapses. Then,
// it returns TRUE instead of FALSE + ERROR_TIMEOUT as expected. But not always. Something
// really strange going on. Re-check this on Windows 10 RTM..
#ifndef Y_BUILD_CONFIG_SHIPPING
  return (IsDebuggerPresent()) ? 1 : INFINITE;
#else
  return INFINITE;
#endif
}

ConditionVariable::ConditionVariable()
{
  InitializeConditionVariable(&m_ConditionVariable);
}

ConditionVariable::~ConditionVariable() {}

void ConditionVariable::SleepAndRelease(Mutex* pMutex)
{
  DebugAssert(pMutex->m_CriticalSection.RecursionCount == 1);
  SleepConditionVariableCS(&m_ConditionVariable, &pMutex->m_CriticalSection, GetWaitTimeout());
}

void ConditionVariable::SleepAndRelease(RecursiveMutex* pMutex)
{
  DebugAssert(pMutex->m_CriticalSection.RecursionCount == 1);
  SleepConditionVariableCS(&m_ConditionVariable, &pMutex->m_CriticalSection, GetWaitTimeout());
}

void ConditionVariable::WakeAll()
{
  WakeAllConditionVariable(&m_ConditionVariable);
}

void ConditionVariable::Wake()
{
  WakeConditionVariable(&m_ConditionVariable);
}

#endif
