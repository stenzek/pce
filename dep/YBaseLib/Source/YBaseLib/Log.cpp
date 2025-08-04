#include "YBaseLib/Log.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/String.h"
#include "YBaseLib/Timer.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsHeaders.h"
#elif defined(Y_PLATFORM_ANDROID)
#include <android/log.h>
#endif

Log* g_pLog = &Log::GetInstance();
static Y_TIMER_VALUE s_startTimeStamp;

Log::Log()
{
  s_startTimeStamp = Y_TimerGetValue();
  m_filterLevel = LOGLEVEL_TRACE;
}

Log::~Log()
{
  DebugAssert(g_pLog == this);
  g_pLog = NULL;
}

void Log::RegisterCallback(CallbackFunctionType callbackFunction, void* pUserParam)
{
  RegisteredCallback Callback;
  Callback.Function = callbackFunction;
  Callback.Parameter = pUserParam;

  m_callbackLock.Lock();
  m_callbacks.Add(Callback);
  m_callbackLock.Unlock();
}

void Log::UnregisterCallback(CallbackFunctionType callbackFunction, void* pUserParam)
{
  m_callbackLock.Lock();

  for (uint32 i = 0; i < m_callbacks.GetSize(); i++)
  {
    if (m_callbacks[i].Function == callbackFunction && m_callbacks[i].Parameter == pUserParam)
    {
      m_callbacks.OrderedRemove(i);
      break;
    }
  }

  m_callbackLock.Unlock();
}

void Log::FormatLogMessageForDisplay(const char* channelName, const char* functionName, LOGLEVEL level,
                                     const char* message, void (*printCallback)(const char*, void*),
                                     void* pCallbackUserData)
{
  static const char levelCharacters[LOGLEVEL_COUNT] = {'X', 'E', 'W', 'P', 'S', 'I', 'D', 'R', 'B', 'T'};

  // find time since start of process
  float messageTime = (float)Y_TimerConvertToSeconds(Y_TimerGetValue() - s_startTimeStamp);

  // write prefix
#ifndef Y_BUILD_CONFIG_SHIPPING
  char prefix[256];
  if (level <= LOGLEVEL_PERF)
    Y_snprintf(prefix, countof(prefix), "[%10.4f] %c(%s): ", messageTime, levelCharacters[level], functionName);
  else
    Y_snprintf(prefix, countof(prefix), "[%10.4f] %c/%s: ", messageTime, levelCharacters[level], channelName);

  printCallback(prefix, pCallbackUserData);
#else
  char prefix[256];
  Y_snprintf(prefix, countof(prefix), "[%10.4f] %c/%s: ", messageTime, levelCharacters[level], channelName);
  printCallback(prefix, pCallbackUserData);
#endif

  // write message
  printCallback(message, pCallbackUserData);
}

static bool s_consoleOutputEnabled = false;
static String s_consoleOutputChannelFilter;
static LOGLEVEL s_consoleOutputLevelFilter = LOGLEVEL_TRACE;

static bool s_debugOutputEnabled = false;
static String s_debugOutputChannelFilter;
static LOGLEVEL s_debugOutputLevelFilter = LOGLEVEL_TRACE;

#if defined(Y_PLATFORM_WINDOWS)

static void ConsoleOutputLogCallback(void* pUserParam, const char* channelName, const char* functionName,
                                     LOGLEVEL level, const char* message)
{
  if (!s_consoleOutputEnabled || level > s_consoleOutputLevelFilter ||
      s_consoleOutputChannelFilter.Find(channelName) >= 0)
    return;

  if (level > LOGLEVEL_COUNT)
    level = LOGLEVEL_TRACE;

  HANDLE hConsole = GetStdHandle((level <= LOGLEVEL_WARNING) ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
  if (hConsole != INVALID_HANDLE_VALUE)
  {
    static const WORD levelColors[LOGLEVEL_COUNT] = {
      FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN,                        // NONE
      FOREGROUND_RED | FOREGROUND_INTENSITY,                                      // ERROR
      FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,                   // WARNING
      FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,                    // PERF
      FOREGROUND_GREEN | FOREGROUND_INTENSITY,                                    // SUCCESS
      FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // INFO
      FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN,                        // DEV
      FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY,                  // PROFILE
      FOREGROUND_GREEN,                                                           // DEBUG
      FOREGROUND_BLUE,                                                            // TRACE
    };

    CONSOLE_SCREEN_BUFFER_INFO oldConsoleScreenBufferInfo;
    GetConsoleScreenBufferInfo(hConsole, &oldConsoleScreenBufferInfo);
    SetConsoleTextAttribute(hConsole, levelColors[level]);

    // write message in the formatted way
    Log::FormatLogMessageForDisplay(channelName, functionName, level, message,
                                    [](const char* text, void* hConsole) {
                                      DWORD written;
                                      WriteConsoleA((HANDLE)hConsole, text, Y_strlen(text), &written, nullptr);
                                    },
                                    (void*)hConsole);

    // write newline
    DWORD written;
    WriteConsoleA(hConsole, "\r\n", 2, &written, nullptr);

    // restore color
    SetConsoleTextAttribute(hConsole, oldConsoleScreenBufferInfo.wAttributes);
  }
}

static void DebugOutputLogCallback(void* pUserParam, const char* channelName, const char* functionName, LOGLEVEL level,
                                   const char* message)
{
  if (!s_debugOutputEnabled || level > s_debugOutputLevelFilter || s_debugOutputChannelFilter.Find(channelName) >= 0)
    return;

  Log::FormatLogMessageForDisplay(channelName, functionName, level, message,
                                  [](const char* text, void*) { OutputDebugStringA(text); });

  OutputDebugStringA("\n");
}

#elif defined(Y_PLATFORM_POSIX)

static void ConsoleOutputLogCallback(void* pUserParam, const char* channelName, const char* functionName,
                                     LOGLEVEL level, const char* message)
{
  if (!s_consoleOutputEnabled || level > s_consoleOutputLevelFilter ||
      s_consoleOutputChannelFilter.Find(channelName) >= 0)
    return;

  static const char* colorCodes[LOGLEVEL_COUNT] = {
    "\033[0m",    // NONE
    "\033[1;31m", // ERROR
    "\033[1;33m", // WARNING
    "\033[1;35m", // PERF
    "\033[1;32m", // SUCCESS
    "\033[1;37m", // INFO
    "\033[0;37m", // DEV
    "\033[1;36m", // PROFILE
    "\033[0;32m", // DEBUG
    "\033[0;34m", // TRACE
  };

  int outputFd = (level <= LOGLEVEL_WARNING) ? STDERR_FILENO : STDOUT_FILENO;

  write(outputFd, colorCodes[level], Y_strlen(colorCodes[level]));

  Log::FormatLogMessageForDisplay(
    channelName, functionName, level, message,
    [](const char* text, void* outputFd) { write((int)(intptr_t)outputFd, text, Y_strlen(text)); },
    (void*)(intptr_t)outputFd);

  write(outputFd, colorCodes[0], Y_strlen(colorCodes[0]));
  write(outputFd, "\n", 1);
}

static void DebugOutputLogCallback(void* pUserParam, const char* channelName, const char* functionName, LOGLEVEL level,
                                   const char* message)
{
}

#elif defined(Y_PLATFORM_ANDROID)

static void ConsoleOutputLogCallback(void* pUserParam, const char* channelName, const char* functionName,
                                     LOGLEVEL level, const char* message)
{
}

static void DebugOutputLogCallback(void* pUserParam, const char* channelName, const char* functionName, LOGLEVEL level,
                                   const char* message)
{
  if (!s_debugOutputEnabled || level > s_debugOutputLevelFilter || s_debugOutputChannelFilter.Find(functionName) >= 0)
    return;

  static const int logPriority[LOGLEVEL_COUNT] = {
    ANDROID_LOG_INFO,  // NONE
    ANDROID_LOG_ERROR, // ERROR
    ANDROID_LOG_WARN,  // WARNING
    ANDROID_LOG_INFO,  // PERF
    ANDROID_LOG_INFO,  // SUCCESS
    ANDROID_LOG_INFO,  // INFO
    ANDROID_LOG_DEBUG, // DEV
    ANDROID_LOG_DEBUG, // PROFILE
    ANDROID_LOG_DEBUG, // DEBUG
    ANDROID_LOG_DEBUG, // TRACE
  };

  __android_log_write(logPriority[level], channelName, message);
}

#elif defined(Y_PLATFORM_HTML5)

static void ConsoleOutputLogCallback(void* pUserParam, const char* channelName, const char* functionName,
                                     LOGLEVEL level, const char* message)
{
  if (!s_consoleOutputEnabled || level > s_consoleOutputLevelFilter ||
      s_consoleOutputChannelFilter.Find(channelName) >= 0)
    return;

  Log::FormatLogMessageForDisplay(channelName, functionName, level, message,
                                  [](const char* text, void*) { write(STDOUT_FILENO, text, Y_strlen(text)); });

  write(STDOUT_FILENO, "\n", 1);
}

static void DebugOutputLogCallback(void* pUserParam, const char* channelName, const char* functionName, LOGLEVEL level,
                                   const char* message)
{
}

#endif

void Log::SetConsoleOutputParams(bool Enabled, const char* ChannelFilter, LOGLEVEL LevelFilter)
{
  if (s_consoleOutputEnabled != Enabled)
  {
    s_consoleOutputEnabled = Enabled;
    if (Enabled)
      RegisterCallback(ConsoleOutputLogCallback, NULL);
    else
      UnregisterCallback(ConsoleOutputLogCallback, NULL);

#if defined(Y_PLATFORM_WINDOWS)
    // On windows, no console is allocated by default on a windows based application
    static bool consoleWasAllocated = false;
    if (Enabled)
    {
      if (GetConsoleWindow() == NULL)
      {
        DebugAssert(!consoleWasAllocated);
        consoleWasAllocated = true;
        AllocConsole();

        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
      }
    }
    else
    {
      if (consoleWasAllocated)
      {
        FreeConsole();
        consoleWasAllocated = false;
      }
    }
#endif
  }

  s_consoleOutputChannelFilter = (ChannelFilter != NULL) ? ChannelFilter : "";
  s_consoleOutputLevelFilter = LevelFilter;
}

void Log::SetDebugOutputParams(bool enabled, const char* channelFilter /* = nullptr */,
                               LOGLEVEL levelFilter /* = LOGLEVEL_TRACE */)
{
  if (s_debugOutputEnabled != enabled)
  {
    s_debugOutputEnabled = enabled;
    if (enabled)
      RegisterCallback(DebugOutputLogCallback, nullptr);
    else
      UnregisterCallback(DebugOutputLogCallback, nullptr);
  }

  s_debugOutputChannelFilter = (channelFilter != nullptr) ? channelFilter : "";
  s_debugOutputLevelFilter = levelFilter;
}

void Log::SetFilterLevel(LOGLEVEL level)
{
  DebugAssert(level < LOGLEVEL_COUNT);
  m_filterLevel = level;
}

void Log::ExecuteCallbacks(const char* channelName, const char* functionName, LOGLEVEL level, const char* message)
{
  m_callbackLock.Lock();

  for (RegisteredCallback& callback : m_callbacks)
    callback.Function(callback.Parameter, channelName, functionName, level, message);

  m_callbackLock.Unlock();
}

void Log::Write(const char* channelName, const char* functionName, LOGLEVEL level, const char* message)
{
  if (level > m_filterLevel)
    return;

  ExecuteCallbacks(channelName, functionName, level, message);
}

void Log::Writef(const char* channelName, const char* functionName, LOGLEVEL level, const char* format, ...)
{
  if (level > m_filterLevel)
    return;

  va_list ap;
  va_start(ap, format);
  Writev(channelName, functionName, level, format, ap);
  va_end(ap);
}

void Log::Writev(const char* channelName, const char* functionName, LOGLEVEL level, const char* format, va_list ap)
{
  if (level > m_filterLevel)
    return;

  va_list apCopy;
  va_copy(apCopy, ap);

  uint32 requiredSize = Y_vscprintf(format, apCopy);
  if (requiredSize < 512)
  {
    char buffer[512];
    Y_vsnprintf(buffer, countof(buffer), format, ap);
    ExecuteCallbacks(channelName, functionName, level, buffer);
  }
  else
  {
    char* buffer = (char*)std::malloc(requiredSize + 1);
    Y_vsnprintf(buffer, requiredSize + 1, format, ap);
    ExecuteCallbacks(channelName, functionName, level, buffer);
    std::free(buffer);
  }
}
