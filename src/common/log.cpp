#include "log.h"

#include <retro2/retro2_log.h>

static_assert(static_cast<int>(Log::Level::None) == RETRO2_LOG_LEVEL_NONE);
static_assert(static_cast<int>(Log::Level::Error) == RETRO2_LOG_LEVEL_ERROR);
static_assert(static_cast<int>(Log::Level::Warning) == RETRO2_LOG_LEVEL_WARNING);
static_assert(static_cast<int>(Log::Level::Info) == RETRO2_LOG_LEVEL_INFO);
static_assert(static_cast<int>(Log::Level::Verbose) == RETRO2_LOG_LEVEL_VERBOSE);
static_assert(static_cast<int>(Log::Level::Dev) == RETRO2_LOG_LEVEL_DEV);
static_assert(static_cast<int>(Log::Level::Debug) == RETRO2_LOG_LEVEL_DEBUG);
static_assert(static_cast<int>(Log::Level::Trace) == RETRO2_LOG_LEVEL_TRACE);

namespace Log {

  static bool FilterTest(const char* channelName, Level level);

namespace {

struct State
{
  Level log_level = Level::Trace;
};

} // namespace

static State s_state;

} // namespace Log

extern IRetro2Log R2Log;

Log::Level Log::GetLogLevel()
{
  return s_state.log_level;
}

bool Log::IsLogVisible(Level level, const char* channelName)
{
  return FilterTest(channelName, level);
}

void Log::Initialize()
{
  s_state.log_level = static_cast<Log::Level>(R2Log.GetLevel());
}

void Log::UpdateSettings()
{
  // TODO: channel visibility
  s_state.log_level = static_cast<Log::Level>(R2Log.GetLevel());
}

bool Log::FilterTest(const char* channelName, Level level)
{
  return (level <= s_state.log_level /*&& s_state.log_channels_enabled[static_cast<size_t>(channel)]*/);
}

void Log::Write(const char* channelName, const char* functionName, Level level, const char* message)
{
  if (!FilterTest(channelName, level))
    return;

  R2Log.Write(static_cast<RETRO2_LOG_LEVEL>(level), message);
}

void Log::Writef(const char* channelName, const char* functionName, Level level, const char* format, ...)
{
  if (!FilterTest(channelName, level))
    return;

  va_list ap;
  va_start(ap, format);
  Writev(channelName, functionName, level, format, ap);
  va_end(ap);
}

void Log::Writev(const char* channelName, const char* functionName, Level level, const char* format, va_list ap)
{
  if (!FilterTest(channelName, level))
    return;

  va_list apCopy;
  va_copy(apCopy, ap);

  #ifdef WIN32
  u32 requiredSize = static_cast<u32>(_vscprintf(format, apCopy));
#else
  u32 requiredSize = std::vsnprintf(nullptr, 0, format, apCopy);
#endif

  if (requiredSize < 256)
  {
    char buffer[256];
    std::vsnprintf(buffer, countof(buffer), format, ap);
    R2Log.Write(static_cast<RETRO2_LOG_LEVEL>(level), buffer);
  }
  else
  {
    char* buffer = new char[requiredSize + 1];
    std::vsnprintf(buffer, requiredSize + 1, format, ap);
    R2Log.Write(static_cast<RETRO2_LOG_LEVEL>(level), buffer);
    delete[] buffer;
  }
}
