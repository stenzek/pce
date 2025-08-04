#include "YBaseLib/Error.h"
#include "YBaseLib/Platform.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/CString.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Windows/WindowsHeaders.h"
#include <psapi.h>
#include <winsock2.h>
#include <cstdlib>
#include <io.h>
#if defined(Y_COMPILER_MSVC)
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif
Log_SetChannel(Platform);

void Platform::MakeTempFileName(char* filename, uint32 len)
{
  size_t length = strlen(filename);
  _mktemp_s(filename, length);
}

void Platform::MakeTempFileName(String& filename)
{
  _mktemp_s(filename.GetWriteableCharArray(), filename.GetLength());
}

bool Platform::GetProgramFileName(String& destination)
{
#if 0
    HMODULE hModule = GetModuleHandleA(NULL);

    DWORD bufferSize = MAX_PATH;
    char *buffer = (char *)Y_malloc(bufferSize + 1);
    DWORD nChars;

    for (;;)
    {
        nChars = GetModuleFileNameA(hModule, buffer, bufferSize + 1);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            bufferSize *= 2;
            buffer = (char *)Y_realloc(buffer, bufferSize + 1);
        }
        else
        {
            break;
        }
    }
#else
  HANDLE hProcess = GetCurrentProcess();

  DWORD bufferSize = MAX_PATH;
  char* buffer = (char*)std::malloc(bufferSize + 1);
  DWORD nChars;

  for (;;)
  {
    nChars = bufferSize + 1;
    if (QueryFullProcessImageNameA(hProcess, 0, buffer, &nChars))
      break;

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
      bufferSize *= 2;
      buffer = (char*)std::realloc(buffer, bufferSize + 1);
    }
    else
    {
      break;
    }
  }
#endif

  if (nChars == 0)
    return false;

  destination.Clear();
  destination.AppendString(buffer, nChars);
  std::free(buffer);

  FileSystem::CanonicalizePath(destination);
  return true;
}

size_t Platform::GetProgramMemoryUsage()
{
  PROCESS_MEMORY_COUNTERS_EX pmc;
  pmc.cb = sizeof(pmc);
  GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS)&pmc, sizeof(pmc));
  return pmc.PrivateUsage;
}

bool Platform::InitializeSocketSupport(Error* pError)
{
  static bool initialized = false;
  if (initialized)
    return true;

  WSADATA wsd;
  if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
  {
    if (pError != nullptr)
      pError->SetErrorSocket(WSAGetLastError());

    Log_ErrorPrintf("WSAStartup failed: %s",
                    Error::CreateErrorSocket(WSAGetLastError()).GetErrorCodeAndDescription().GetCharArray());
    return false;
  }

  // Register cleanup function
  atexit([]() -> void { WSACleanup(); });
  initialized = true;
  return true;
}

#endif // Y_PLATFORM_WINDOWS
