#include "YBaseLib/Timer.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsHeaders.h"

static double g_dCounterFrequency;
static bool g_bCounterInitialized = false;

Y_TIMER_VALUE Y_TimerGetValue()
{
  // even if this races, it should still result in the same value..
  if (!g_bCounterInitialized)
  {
    LARGE_INTEGER Freq;
    QueryPerformanceFrequency(&Freq);
    g_dCounterFrequency = (double)Freq.QuadPart / 1000000000.0;
    g_bCounterInitialized = true;
  }

  Y_TIMER_VALUE ReturnValue;
  QueryPerformanceCounter((LARGE_INTEGER*)&ReturnValue);
  return ReturnValue;
}

double Y_TimerConvertToNanoseconds(Y_TIMER_VALUE Value)
{
  return ((double)Value / g_dCounterFrequency);
}

double Y_TimerConvertToMilliseconds(Y_TIMER_VALUE Value)
{
  return (((double)Value / g_dCounterFrequency) / 1000000.0);
}

double Y_TimerConvertToSeconds(Y_TIMER_VALUE Value)
{
  return (((double)Value / g_dCounterFrequency) / 1000000000.0);
}

#elif defined(Y_PLATFORM_POSIX) || defined(Y_PLATFORM_ANDROID)

#if 1 // using clock_gettime()

#include <sys/time.h>

Y_TIMER_VALUE Y_TimerGetValue()
{
  struct timespec tv;
  clock_gettime(CLOCK_MONOTONIC, &tv);
  return ((Y_TIMER_VALUE)tv.tv_nsec + (Y_TIMER_VALUE)tv.tv_sec * 1000000000);
}

double Y_TimerConvertToNanoseconds(Y_TIMER_VALUE Value)
{
  return ((double)Value);
}

double Y_TimerConvertToMilliseconds(Y_TIMER_VALUE Value)
{
  return ((double)Value / 1000000.0);
}

double Y_TimerConvertToSeconds(Y_TIMER_VALUE Value)
{
  return ((double)Value / 1000000000.0);
}

#else // using gettimeofday()

#include <sys/time.h>

Y_TIMER_VALUE Y_TimerGetValue()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((Y_TIMER_VALUE)tv.tv_usec) + ((Y_TIMER_VALUE)tv.tv_sec * (Y_TIMER_VALUE)1000000);
}

double Y_TimerConvertToNanoseconds(Y_TIMER_VALUE Value)
{
  return ((double)Value * 1000.0);
}

double Y_TimerConvertToMilliseconds(Y_TIMER_VALUE Value)
{
  return ((double)Value / 1000.0);
}

double Y_TimerConvertToSeconds(Y_TIMER_VALUE Value)
{
  return ((double)Value / 1000000.0);
}

#endif // Y_PLATFORM_POSIX || Y_PLATFORM_ANDROID

#elif defined(Y_PLATFORM_HTML5)

Y_TIMER_VALUE Y_TimerGetValue()
{
  return emscripten_get_now();
}

double Y_TimerConvertToNanoseconds(Y_TIMER_VALUE Value)
{
  return Value * 1000000.0;
}

double Y_TimerConvertToMilliseconds(Y_TIMER_VALUE Value)
{
  return Value;
}

double Y_TimerConvertToSeconds(Y_TIMER_VALUE Value)
{
  return Value / 1000.0;
}

#endif

Timer::Timer()
{
  Reset();
}

void Timer::Reset()
{
  m_tvStartValue = Y_TimerGetValue();
}

double Timer::GetTimeSeconds() const
{
  return Y_TimerConvertToSeconds(Y_TimerGetValue() - m_tvStartValue);
}

double Timer::GetTimeMilliseconds() const
{
  return Y_TimerConvertToMilliseconds(Y_TimerGetValue() - m_tvStartValue);
}

double Timer::GetTimeNanoseconds() const
{
  return Y_TimerConvertToNanoseconds(Y_TimerGetValue() - m_tvStartValue);
}
