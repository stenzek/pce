#pragma once

#include "YBaseLib/Common.h"
#include "types.h"
#include <cinttypes>

struct IRetro2Log;

namespace Log {

enum class Level : u32
{
  None, // Silences all log traffic
  Error,
  Warning,
  Info,
  Verbose,
  Dev,
  Debug,
  Trace,

  MaxCount
};

// Returns the current global filtering level.
Level GetLogLevel();

// Returns true if log messages for the specified log level/filter would not be filtered (and visible).
bool IsLogVisible(Level level, const char* channelName);

// Initializes the log forwarder.
void Initialize();
void UpdateSettings();

// writes a message to the log
void Write(const char* channelName, const char* functionName, Level level, const char* message);
void Writef(const char* channelName, const char* functionName, Level level, const char* format, ...);
void Writev(const char* channelName, const char* functionName, Level level, const char* format, va_list ap);

}; // namespace Log

#ifdef Y_BUILD_CONFIG_SHIPPING
#define LOG_MESSAGE_FUNCTION_NAME ""
#else
#define LOG_MESSAGE_FUNCTION_NAME __FUNCTION__
#endif

// log wrappers
#define Log_SetChannel(ChannelName) static const char* ___LogChannel___ = #ChannelName;
#define Log_ErrorPrint(msg) Log::Write(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Error, msg)
#define Log_ErrorPrintf(...) Log::Writef(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Error, __VA_ARGS__)
#define Log_WarningPrint(msg) Log::Write(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Warning, msg)
#define Log_WarningPrintf(...)                                                                                         \
  Log::Writef(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Warning, __VA_ARGS__)
#define Log_SuccessPrint(msg) Log::Write(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Verbose, msg)
#define Log_SuccessPrintf(...)                                                                                         \
  Log::Writef(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Verbose, __VA_ARGS__)
#define Log_InfoPrint(msg) Log::Write(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Info, msg)
#define Log_InfoPrintf(...) Log::Writef(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Info, __VA_ARGS__)
#define Log_PerfPrint(msg) Log::Write(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Verbose, msg)
#define Log_PerfPrintf(...) Log::Writef(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Verbose, __VA_ARGS__)
#define Log_DevPrint(msg) Log::Write(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Dev, msg)
#define Log_DevPrintf(...) Log::Writef(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Dev, __VA_ARGS__)
#define Log_ProfilePrint(msg) Log::Write(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Dev, msg)
#define Log_ProfilePrintf(...) Log::Writef(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Dev, __VA_ARGS__)

#ifdef Y_BUILD_CONFIG_RELEASE
#define Log_DebugPrint(msg) MULTI_STATEMENT_MACRO_BEGIN MULTI_STATEMENT_MACRO_END
#define Log_DebugPrintf(...) MULTI_STATEMENT_MACRO_BEGIN MULTI_STATEMENT_MACRO_END
#define Log_TracePrint(msg) MULTI_STATEMENT_MACRO_BEGIN MULTI_STATEMENT_MACRO_END
#define Log_TracePrintf(...) MULTI_STATEMENT_MACRO_BEGIN MULTI_STATEMENT_MACRO_END
#else
#define Log_DebugPrint(msg) Log::Write(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Debug, msg)
#define Log_DebugPrintf(...) Log::Writef(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Debug, __VA_ARGS__)
#define Log_TracePrint(msg) Log::Write(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Trace, msg)
#define Log_TracePrintf(...) Log::Writef(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, Log::Level::Trace, __VA_ARGS__)
#endif
