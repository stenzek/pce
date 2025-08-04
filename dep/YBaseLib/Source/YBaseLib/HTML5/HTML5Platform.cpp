#include "YBaseLib/Platform.h"

#if defined(Y_PLATFORM_HTML5)
#include "YBaseLib/FileSystem.h"
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

void Platform::MakeTempFileName(char* filename, uint32 len)
{
  uint32 length = Y_strlen(filename);
  DebugAssert(length < len);
  mktemp(filename);
}

void Platform::MakeTempFileName(String& filename)
{
  mktemp(filename.GetWriteableCharArray());
}

bool Platform::GetProgramFileName(String& destination)
{
  // cop-out
  destination.Assign("/module.js");
  return true;
}

size_t Platform::GetProgramMemoryUsage()
{
  return 0;
}

#endif // Y_PLATFORM_POSIX
