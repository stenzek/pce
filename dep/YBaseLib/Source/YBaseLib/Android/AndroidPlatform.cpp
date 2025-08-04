#include "YBaseLib/Platform.h"

#if defined(Y_PLATFORM_ANDROID)
#include "YBaseLib/FileSystem.h"

#define _GNU_SOURCE 1
#include <dlfcn.h>

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
  static const char* exeFileName = "/proc/self/exe";

  char stackBuffer[FS_MAX_PATH];
  int len = readlink(exeFileName, stackBuffer, sizeof(stackBuffer));
  if (len < 0)
  {
    return false;
  }
  else if ((size_t)len < sizeof(stackBuffer))
  {
    destination.Clear();
    destination.AppendString(stackBuffer, len);
    return true;
  }

  int curSize = sizeof(stackBuffer) * 2;
  char* buffer = (char*)std::realloc(NULL, curSize);
  for (;;)
  {
    len = readlink(exeFileName, buffer, curSize);
    if (len < 0)
    {
      std::free(buffer);
      return false;
    }
    else if (len < curSize)
    {
      buffer[len] = '\0';
      destination.Clear();
      destination.AppendString(buffer, len);
      return true;
    }

    curSize *= 2;
    buffer = (char*)std::realloc(buffer, curSize);
  }

  return false;
}

size_t Platform::GetProgramMemoryUsage()
{
  return 0;
}

bool Platform::InitializeSocketSupport(Error* pError)
{
  return true;
}

#endif // Y_PLATFORM_ANDROID
