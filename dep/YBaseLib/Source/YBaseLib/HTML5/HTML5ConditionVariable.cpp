#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_HTML5)
#include "YBaseLib/HTML5/HTML5ConditionVariable.h"
#include "YBaseLib/HTML5/HTML5Mutex.h"
#include "YBaseLib/HTML5/HTML5ReadWriteLock.h"
#include "YBaseLib/Log.h"
Log_SetChannel(HTML5ConditionVariable);

ConditionVariable::ConditionVariable() {}

ConditionVariable::~ConditionVariable() {}

void ConditionVariable::SleepAndRelease(Mutex* pMutex)
{
  Log_WarningPrintf("ConditionVariable::SleepAndRelease unimplemented on HTML5.");
}

void ConditionVariable::SleepAndRelease(RecursiveMutex* pMutex)
{
  Log_WarningPrintf("ConditionVariable::SleepAndRelease unimplemented on HTML5.");
}

void ConditionVariable::WakeAll()
{
  Log_WarningPrintf("ConditionVariable::WakeAll unimplemented on HTML5.");
}

void ConditionVariable::Wake()
{
  Log_WarningPrintf("ConditionVariable::Wake unimplemented on HTML5.");
}

#endif
