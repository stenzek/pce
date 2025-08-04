#include "YBaseLib/CPUID.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/String.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsHeaders.h"
#elif defined(Y_PLATFORM_LINUX) || defined(Y_PLATFORM_OSX)
#include <unistd.h> // sysconf
#elif defined(Y_PLATFORM_FREEBSD)
#error fixme
#elif defined(Y_PLATFORM_ANDROID)
#include <unistd.h>
#elif defined(Y_PLATFORM_HTML5)

#else
#error unknown platform
#endif

#if defined(Y_CPU_X86) || defined(Y_CPU_X64)

#if defined(Y_PLATFORM_WINDOWS)
#include <intrin.h>
#define CALL_CPUID(in, out) __cpuid((int*)out, in)
#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG)
//#define CALL_CPUID(in, out) asm volatile ( "cpuid" : "=a"(*(out + 0)), "=b"(*(out + 1)), "=c"(*(out + 2)), "=d"(*(out
//+ 3)) : "a"(in) )
#include <cpuid.h>
#define CALL_CPUID(in, out) __get_cpuid(in, &out[0], &out[1], &out[2], &out[3])
#endif

#define CheckBit(val, bit) (((((val) & (1 << (bit))) != 0) ? 1 : 0) != 0)

static void CPUID_ReadCPUData(Y_CPUID_RESULT* pResult)
{
  // http://www.sandpile.org/ia32/cpuid.htm

  // TODO: Check for CPUID supportability

  // cpuid return values
  uint32 data[4];

  // max basic level and vendor string
  CALL_CPUID(0x00000000, data);
  pResult->MaxBasicLevel = data[0];
  *(uint32*)(&pResult->VendorString[0]) = data[1];
  *(uint32*)(&pResult->VendorString[4]) = data[3];
  *(uint32*)(&pResult->VendorString[8]) = data[2];

  // type/family/model/stepping
  CALL_CPUID(0x00000001, data);
  pResult->Stepping = data[0] & 0xf;
  pResult->Model = ((data[0] >> 4) & 0xf) | (((data[0] >> 16) & 0xf) << 4);
  pResult->Family = ((data[0] >> 8) & 0xf) + ((data[0] >> 20) & 0xf);
  pResult->Type = (data[0] >> 12) & 0x3;
  pResult->BrandId = data[1] & 0xf;

  // feature flags
  pResult->SupportsAVX = CheckBit(data[2], 28);
  pResult->SupportsAES = CheckBit(data[2], 25);
  pResult->SupportsSSE42 = CheckBit(data[2], 20);
  pResult->SupportsSSE41 = CheckBit(data[2], 19);
  pResult->SupportsSSSE3 = CheckBit(data[2], 9);
  pResult->SupportsSSE3 = CheckBit(data[2], 0);
  pResult->SupportsHTT = CheckBit(data[3], 28);
  pResult->SupportsSSE2 = CheckBit(data[3], 26);
  pResult->SupportsSSE = CheckBit(data[3], 25);
  pResult->SupportsMMX = CheckBit(data[3], 23);
  pResult->SupportsCMOV = CheckBit(data[3], 15);

  // max extended level
  CALL_CPUID(0x80000000, data);
  pResult->MaxExtendedLevel = data[0];

  // extended feature flags
  CALL_CPUID(0x80000001, data);
  pResult->SupportsSSE5A = CheckBit(data[2], 11);
  pResult->SupportsSSE4A = CheckBit(data[2], 6);
  pResult->Supports3DNow2 = CheckBit(data[3], 31);
  pResult->Supports3DNow = CheckBit(data[3], 30);

  // get brand string
  CALL_CPUID(0x80000002, data);
  *(uint32*)(&pResult->BrandString[0]) = data[0];
  *(uint32*)(&pResult->BrandString[4]) = data[1];
  *(uint32*)(&pResult->BrandString[8]) = data[2];
  *(uint32*)(&pResult->BrandString[12]) = data[3];
  CALL_CPUID(0x80000003, data);
  *(uint32*)(&pResult->BrandString[16]) = data[0];
  *(uint32*)(&pResult->BrandString[20]) = data[1];
  *(uint32*)(&pResult->BrandString[24]) = data[2];
  *(uint32*)(&pResult->BrandString[28]) = data[3];
  CALL_CPUID(0x80000004, data);
  *(uint32*)(&pResult->BrandString[32]) = data[0];
  *(uint32*)(&pResult->BrandString[36]) = data[1];
  *(uint32*)(&pResult->BrandString[40]) = data[2];
  *(uint32*)(&pResult->BrandString[44]) = data[3];
}

#elif defined(Y_CPU_ARM)

static void CPUID_ReadCPUData(Y_CPUID_RESULT* pResult)
{
  Y_strncpy(pResult->VendorString, sizeof(pResult->VendorString), "ARM");
  Y_strncpy(pResult->BrandString, sizeof(pResult->BrandString), "ARM");
}

#elif defined(Y_PLATFORM_HTML5)

static void CPUID_ReadCPUData(Y_CPUID_RESULT* pResult)
{
  Y_strncpy(pResult->VendorString, sizeof(pResult->VendorString), "JavaScript");
  Y_strncpy(pResult->BrandString, sizeof(pResult->BrandString), "Emscripten");
}

#else

static void CPUID_ReadCPUData(Y_CPUID_RESULT* pResult) {}

#endif

static void CPUID_ReadOSData(Y_CPUID_RESULT* pResult)
{
#if defined(Y_PLATFORM_WINDOWS)
  // get processor count
  {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    pResult->ThreadCount = sysinfo.dwNumberOfProcessors;
  }

  // handle HT
  if (pResult->SupportsHTT && pResult->ThreadCount > 1)
    pResult->ProcessorCount = pResult->ThreadCount / 2;
  else
    pResult->ProcessorCount = pResult->ThreadCount;

  // read processor clock
  {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) ==
        ERROR_SUCCESS)
    {
      DWORD value;
      DWORD type = REG_DWORD;
      DWORD cbValue = sizeof(DWORD);
      if (RegQueryValueExA(hKey, "~Mhz", 0, &type, (LPBYTE)&value, &cbValue) == ERROR_SUCCESS)
        pResult->ClockSpeed = value;

      RegCloseKey(hKey);
    }
  }

#elif defined(Y_PLATFORM_LINUX) || defined(Y_PLATFORM_OSX)
  // http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
  int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
  pResult->ThreadCount = numCPU;
  pResult->ProcessorCount = numCPU;
  pResult->ClockSpeed = 1;
#elif defined(Y_PLATFORM_FREEBSD)
  int mib[4];
  size_t len = sizeof(numCPU);

  /* set the mib for hw.ncpu */
  mib[0] = CTL_HW;
  mib[1] = HW_AVAILCPU; // alternatively, try HW_NCPU;

  /* get the number of CPUs from the system */
  int numCPU;
  sysctl(mib, 2, &numCPU, &len, NULL, 0);

  if (numCPU < 1)
  {
    mib[1] = HW_NCPU;
    sysctl(mib, 2, &numCPU, &len, NULL, 0);

    if (numCPU < 1)
    {
      numCPU = 1;
    }
  }

  pResult->ThreadCount = numCPU;
  pResult->ProcessorCount = numCPU;
  pResult->ClockSpeed = 1;
#elif defined(Y_PLATFORM_ANDROID)
  pResult->ThreadCount = 1;
  pResult->ProcessorCount = 1;
  pResult->ClockSpeed = 1;
#elif defined(Y_PLATFORM_HTML5)
  pResult->ThreadCount = 1;
  pResult->ProcessorCount = 1;
  pResult->ClockSpeed = 1;
#else
#error unhandled platform
#endif
}

static void CPUID_GenerateSummaryString(Y_CPUID_RESULT* pResult)
{
  char tmp[128];
#define CONCAT_FMT(...)                                                                                                \
  {                                                                                                                    \
    Y_snprintf(tmp, sizeof(tmp), __VA_ARGS__);                                                                         \
    Y_strncat(pResult->SummaryString, sizeof(pResult->SummaryString), tmp);                                            \
  }

  // should be zero length
  DebugAssert(pResult->SummaryString[0] == 0);

  // Example:
  // GenuineIntel Intel(R) Core(TM) i7 CPU      930  @  2.80Ghz (4 cores, 8 threads) at 4000mhz
  // MMX/3DNOW/SSE/SSE2/SSE3/SSE4A/SSE4.1/SSE4.2
  CONCAT_FMT("%s ", pResult->VendorString);
  CONCAT_FMT("%s ", pResult->BrandString);
  CONCAT_FMT(" (%u cores, %u threads) ", pResult->ProcessorCount, pResult->ThreadCount);
  CONCAT_FMT(" at %umhz ", pResult->ClockSpeed);

  if (pResult->SupportsMMX)
    CONCAT_FMT("MMX/");
  if (pResult->Supports3DNow)
    CONCAT_FMT("3DNow/");
  if (pResult->Supports3DNow2)
    CONCAT_FMT("3DNow2/");
  if (pResult->SupportsSSE)
    CONCAT_FMT("SSE/");
  if (pResult->SupportsSSE2)
    CONCAT_FMT("SSE2/");
  if (pResult->SupportsSSE3)
    CONCAT_FMT("SSE3/");
  if (pResult->SupportsSSSE3)
    CONCAT_FMT("SSSE3/");
  if (pResult->SupportsSSE4A)
    CONCAT_FMT("SSE4A/");
  if (pResult->SupportsSSE41)
    CONCAT_FMT("SSE4.1/");
  if (pResult->SupportsSSE42)
    CONCAT_FMT("SSE4.2/");
  if (pResult->SupportsAVX)
    CONCAT_FMT("AVX/");

  // remove trailing / if present
  uint32 len = Y_strlen(pResult->SummaryString);
  DebugAssert(len < sizeof(pResult->SummaryString));
  if (len > 0 && pResult->SummaryString[len - 1] == '/')
    pResult->SummaryString[len - 1] = 0;
}

void Y_ReadCPUID(Y_CPUID_RESULT* pResult)
{
  static Y_CPUID_RESULT CachedResult;
  static bool CachedResultFilled = false;
  if (!CachedResultFilled)
  {
    Y_memzero(&CachedResult, sizeof(CachedResult));
    CPUID_ReadCPUData(&CachedResult);
    CPUID_ReadOSData(&CachedResult);
    CPUID_GenerateSummaryString(&CachedResult);
    CachedResultFilled = true;
  }

  std::memcpy(pResult, &CachedResult, sizeof(CachedResult));
}
