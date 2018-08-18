#include "YBaseLib/Log.h"
#include "pce-tests/helpers.h"
#include "pce-tests/testpcsystem.h"
#include "pce/bus.h"
#include <gtest/gtest.h>
Log_SetChannel(Tests);

namespace CPU_X86 {
extern bool TRACE_EXECUTION;
}

static bool RunTest(CPUBackendType backend, const char* code_file)
{
  CPU_X86::TRACE_EXECUTION = true;
  Log::GetInstance().SetConsoleOutputParams(true, "", LOGLEVEL_PROFILE);
  Log::GetInstance().SetFilterLevel(LOGLEVEL_PROFILE);

  auto system = std::make_unique<TestPCSystem>(CPU_X86::MODEL_486, 1000000.0f, backend, 1024 * 1024);

  EXPECT_TRUE(system->AddMMIOROMFromFile(code_file, 0xf0000u));
  EXPECT_TRUE(system->AddMMIOROMFromFile(code_file, 0xffff0000u));

  system->Initialize();
  system->SetState(System::State::Running);

  // Put a cap on the number of cycles, a runtime of 10 seconds should do.
  CycleCount remaining_cycles = CycleCount(system->GetCPU()->GetFrequency() * 10);
  while (!system->GetX86CPU()->IsHalted() && remaining_cycles > 0)
  {
    CycleCount slice_cycles = std::min(remaining_cycles, CycleCount(system->GetCPU()->GetFrequency()));
    remaining_cycles -= slice_cycles;
    system->GetX86CPU()->ExecuteCycles(slice_cycles);
  }

  // CPU should be halted at the end of the test.
  EXPECT_TRUE(system->GetX86CPU()->IsHalted())
    << "TIMEOUT: CPU is not halted indicating the test did not finish in time";
  if (!system->GetX86CPU()->IsHalted())
  {
    system->Stop();
    return false;
  }

  system->Stop();
  return true;
}

// Test on both Interpreter and FastInterpreter
#define MAKE_TEST(name, code_file)                                                                                     \
  TEST(Tests_Interpreter, name) { EXPECT_TRUE(RunTest(CPUBackendType::Interpreter, code_file)); }

MAKE_TEST(test_init, "tests/test_init.bin")
