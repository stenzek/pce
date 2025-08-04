#include "TestSuite.h"
#include "YBaseLib/CPUID.h"
#include "YBaseLib/Log.h"
Log_SetChannel(CPUID);

DEFINE_TEST_SUITE(CPUID)
{
  Y_CPUID_RESULT cpuid;
  Y_ReadCPUID(&cpuid);

  Log_DevPrintf("CPUID Information");
  Log_DevPrintf("");
  Log_DevPrintf("Summary string:");
  Log_DevPrintf("%s", cpuid.SummaryString);
  Log_DevPrintf("");
  Log_DevPrintf("Supported instruction sets:");
  Log_DevPrintf("=============================================");
  Log_DevPrintf("| Instruction Set               | Supported |");
  Log_DevPrintf("=============================================");
  Log_DevPrintf("| CMOV                          | %9s |", cpuid.SupportsCMOV ? "Yes" : "No");
  Log_DevPrintf("| MMX                           | %9s |", cpuid.SupportsMMX ? "Yes" : "No");
  Log_DevPrintf("| 3DNow                         | %9s |", cpuid.Supports3DNow ? "Yes" : "No");
  Log_DevPrintf("| 3DNow2                        | %9s |", cpuid.Supports3DNow2 ? "Yes" : "No");
  Log_DevPrintf("| SSE                           | %9s |", cpuid.SupportsSSE ? "Yes" : "No");
  Log_DevPrintf("| SSE2                          | %9s |", cpuid.SupportsSSE2 ? "Yes" : "No");
  Log_DevPrintf("| SSE3                          | %9s |", cpuid.SupportsSSE3 ? "Yes" : "No");
  Log_DevPrintf("| SSSE3                         | %9s |", cpuid.SupportsSSSE3 ? "Yes" : "No");
  Log_DevPrintf("| SSE4A                         | %9s |", cpuid.SupportsSSE4A ? "Yes" : "No");
  Log_DevPrintf("| SSE4.1                        | %9s |", cpuid.SupportsSSE41 ? "Yes" : "No");
  Log_DevPrintf("| SSE4.2                        | %9s |", cpuid.SupportsSSE42 ? "Yes" : "No");
  Log_DevPrintf("| SSE5A                         | %9s |", cpuid.SupportsSSE5A ? "Yes" : "No");
  Log_DevPrintf("| AVX                           | %9s |", cpuid.SupportsAVX ? "Yes" : "No");
  Log_DevPrintf("| AES                           | %9s |", cpuid.SupportsAES ? "Yes" : "No");
  Log_DevPrintf("| HTT (Hyper-threading)         | %9s |", cpuid.SupportsHTT ? "Yes" : "No");
  Log_DevPrintf("=============================================");
  Log_DevPrintf("");
  return true;
}
