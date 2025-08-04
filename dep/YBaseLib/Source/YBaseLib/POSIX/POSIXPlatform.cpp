#include "YBaseLib/Platform.h"

#if defined(Y_PLATFORM_POSIX)
#include "YBaseLib/FileSystem.h"

#if defined(Y_PLATFORM_LINUX)
#define _GNU_SOURCE 1
#include <dlfcn.h>
#endif

#if defined(Y_PLATFORM_OSX)
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <sys/param.h>
#endif

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
#if defined(Y_PLATFORM_LINUX)

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

    /*

    char procPath[200];
    Y_snprintf(procPath, countof(procPath), "/proc/%d/exe", (int)getpid());

    struct stat statBuffer;
    if (lstat(procPath, &statBuffer) < 0)
        return false;

    char *buffer = (char *)alloca(statBuffer.st_size);
    int nBytes = readlink(procPath, buffer, statBuffer.st_size);
    if (nBytes > 0)
    {
        destination.Clear();
        destination.AppendString(buffer, nBytes);
        return true;
    }

    return false;

    */

#elif defined(Y_PLATFORM_OSX)

  int curSize = FS_MAX_PATH;
  char* buffer = (char*)std::realloc(NULL, curSize + 1);
  for (;;)
  {
    uint32 nChars = FS_MAX_PATH - 1;
    int res = _NSGetExecutablePath(buffer, &nChars);
    if (res == 0)
    {
      buffer[nChars] = 0;

      char* resolvedBuffer = realpath(buffer, NULL);
      if (resolvedBuffer == NULL)
      {
        std::free(buffer);
        return false;
      }

      destination.Clear();
      destination.AppendString(resolvedBuffer);
      free(resolvedBuffer);
      std::free(buffer);
      return true;
    }

    if (curSize >= 1048576)
    {
      std::free(buffer);
      return false;
    }

    curSize *= 2;
    buffer = (char*)std::realloc(buffer, curSize + 1);
  }

#else
  /*
  Dl_info dlInfo;
  Y_memzero(&dlInfo, sizeof(dlInfo));
  if (dladdr(reinterpret_cast<void *>(GetProgramFileName), &dlInfo) != 0 || dlInfo.dli_fname == NULL)
      Y_strncpy(modulePath, cbModulePath, "");
  else
      Y_strncpy(modulePath, cbModulePath, dlInfo.dli_fname);
*/
  return false;
#endif
}

size_t Platform::GetProgramMemoryUsage()
{
  return 0;
}

bool Platform::InitializeSocketSupport(Error* pError)
{
  return true;
}

#endif // Y_PLATFORM_POSIX
