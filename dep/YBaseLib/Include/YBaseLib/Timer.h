#pragma once

#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS) || defined(Y_PLATFORM_POSIX) || defined(Y_PLATFORM_ANDROID)
typedef uint64 Y_TIMER_VALUE;
#elif defined(Y_PLATFORM_HTML5)
typedef double Y_TIMER_VALUE;
#else
#error unknown platform
#endif

Y_TIMER_VALUE Y_TimerGetValue();
double Y_TimerConvertToSeconds(Y_TIMER_VALUE Value);
double Y_TimerConvertToMilliseconds(Y_TIMER_VALUE Value);
double Y_TimerConvertToNanoseconds(Y_TIMER_VALUE Value);

class Timer
{
public:
  Timer();

  void Reset();

  double GetTimeSeconds() const;
  double GetTimeMilliseconds() const;
  double GetTimeNanoseconds() const;

private:
  Y_TIMER_VALUE m_tvStartValue;
};
