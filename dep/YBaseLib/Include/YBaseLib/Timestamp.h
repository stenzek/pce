#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/String.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsHeaders.h"
#elif defined(Y_PLATFORM_POSIX) || defined(Y_PLATFORM_HTML5) || defined(Y_PLATFORM_ANDROID)
#include <sys/time.h>
#endif

class Timestamp
{
public:
  typedef uint64 UnixTimestampValue;
  struct ExpandedTime
  {
    uint32 Year;         // 0-...
    uint32 Month;        // 1-12
    uint32 DayOfMonth;   // 1-31
    uint32 DayOfWeek;    // 0-6, starting at Sunday
    uint32 Hour;         // 0-23
    uint32 Minute;       // 0-59
    uint32 Second;       // 0-59
    uint32 Milliseconds; // 0-999
  };

public:
  Timestamp();
  Timestamp(const Timestamp& copy);

  // readers
  UnixTimestampValue AsUnixTimestamp() const;
  ExpandedTime AsExpandedTime() const;

  // calculators
  double DifferenceInSeconds(Timestamp& other) const;
  int64 DifferenceInSecondsInt(Timestamp& other) const;

  // setters
  void SetNow();
  void SetUnixTimestamp(UnixTimestampValue value);
  void SetExpandedTime(const ExpandedTime& value);

  // string conversion
  String ToString(const char* format) const;
  void ToString(String& destination, const char* format) const;

  // creators
  static Timestamp Now();
  static Timestamp FromUnixTimestamp(UnixTimestampValue value);
  static Timestamp FromExpandedTime(const ExpandedTime& value);

// windows-specific
#ifdef Y_PLATFORM_WINDOWS
  FILETIME AsFileTime();
  void SetWindowsFileTime(const FILETIME* pFileTime);
  static Timestamp FromWindowsFileTime(const FILETIME* pFileTime);
#endif

  // operators
  bool operator==(const Timestamp& other) const;
  bool operator!=(const Timestamp& other) const;
  bool operator<(const Timestamp& other) const;
  bool operator<=(const Timestamp& other) const;
  bool operator>(const Timestamp& other) const;
  bool operator>=(const Timestamp& other) const;
  Timestamp& operator=(const Timestamp& other);

private:
#if defined(Y_PLATFORM_WINDOWS)
  SYSTEMTIME m_value;
#elif defined(Y_PLATFORM_POSIX) || defined(Y_PLATFORM_HTML5) || defined(Y_PLATFORM_ANDROID)
  struct timeval m_value;
#endif
};
