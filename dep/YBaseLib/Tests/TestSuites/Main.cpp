#include "TestSuite.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/Log.h"
Log_SetChannel(Main);

DECLARE_TEST_SUITE(Base64);
DECLARE_TEST_SUITE(BitSet);
DECLARE_TEST_SUITE(CPUID);

struct TestSuiteEntry
{
  const char* Name;
  TestSuiteRunnerFunction function;
};

static const TestSuiteEntry s_testSuites[] = {
  {"Base64", INVOKE_TEST_SUITE(Base64)},
  {"BitSet", INVOKE_TEST_SUITE(BitSet)},
  {"CPUID", INVOKE_TEST_SUITE(CPUID)},
};

int main(int argc, char* argv[])
{
  Log::GetInstance().SetConsoleOutputParams(true);
  Log::GetInstance().SetDebugOutputParams(true);

  const char* searchName = (argc > 1) ? argv[1] : nullptr;
  if (searchName != nullptr)
    Log_InfoPrintf("## Running only test for '%s'", searchName);

  uint32 failures = 0;
  uint32 passes = 0;
  for (size_t i = 0; i < countof(s_testSuites); i++)
  {
    if (searchName != nullptr && Y_stricmp(searchName, s_testSuites[i].Name) != 0)
      continue;

    Log_InfoPrintf("-> Running test suite '%s'", s_testSuites[i].Name);
    if (s_testSuites[i].function())
    {
      Log_SuccessPrint("  Passed.");
      passes++;
    }
    else
    {
      Log_ErrorPrint("  Failed.");
      failures++;
    }
  }

  Log_InfoPrintf("Test summary: %u suites, %u passes, %u failures", countof(s_testSuites), passes, failures);
  return (failures > 0) ? -1 : 0;
}
