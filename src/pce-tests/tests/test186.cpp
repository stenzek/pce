#include "YBaseLib/Log.h"
#include "pce-tests/helpers.h"
#include "pce-tests/testpcsystem.h"
#include "pce/bus.h"
#include <gtest/gtest.h>
Log_SetChannel(Test186);

static bool RunTest(CPUBackendType backend, const char* code_file, const char* expected_ouput_file)
{
  auto system = std::make_unique<TestPCSystem>(CPU_X86::MODEL_186, 1000000.0f, backend, 1024 * 1024);

  if (!system->AddMMIOROMFromFile(code_file, 0xF0000))
    return false;

  PODArray<byte> expected_buffer;
  PODArray<byte> actual_buffer;
  if (!ReadFileToArray(&expected_buffer, expected_ouput_file))
    return false;

  EXPECT_TRUE(system->Ready()) << "system did not initialize successfully";

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

  // Read back results
  actual_buffer.Resize(expected_buffer.GetSize());
  for (uint32 i = 0; i < expected_buffer.GetSize(); i++)
  {
    if (!system->GetBus()->CheckedReadMemoryByte(i, &actual_buffer[i]))
    {
      Log_ErrorPrintf("Failed memory read %x", i);
      actual_buffer[i] = 0;
    }
  }

  bool result = true;
  for (uint32 i = 0; i < expected_buffer.GetSize(); i++)
  {
    uint8 val1 = expected_buffer[i];
    uint8 val2 = actual_buffer[i];
    EXPECT_EQ(val1, val2) << StringFromFormat("Difference at offset 0x%04X (%u): expected %02X got %02X", i, i, val1,
                                              val2);
    if (val1 != val2)
      result = false;
  }

  if (!result)
  {
    Log_DevPrintf("Expected results: ");
    for (uint32 i = 0; i < expected_buffer.GetSize(); i += 16)
    {
      SmallString hex_string;
      for (uint32 j = 0; j < 16 && (i + j) < expected_buffer.GetSize(); j++)
        hex_string.AppendFormattedString("%02X ", expected_buffer[i + j]);
      Log_DevPrintf("%04X: %s", i, hex_string.GetCharArray());
    }

    Log_DevPrintf("Actual results: ");
    for (uint32 i = 0; i < expected_buffer.GetSize(); i += 16)
    {
      SmallString hex_string;
      for (uint32 j = 0; j < 16 && (i + j) < expected_buffer.GetSize(); j++)
        hex_string.AppendFormattedString("%02X ", actual_buffer[i + j]);
      Log_DevPrintf("%04X: %s", i, hex_string.GetCharArray());
    }
  }

  system->Stop();
  return result;
}

// Test on both Interpreter and FastInterpreter
#define MAKE_TEST(name, code_file, results_file)                                                                       \
  TEST(Test186_Interpreter, name) { EXPECT_TRUE(RunTest(CPUBackendType::Interpreter, code_file, results_file)); }      \
  TEST(Test186_CachedInterpreter, name)                                                                                \
  {                                                                                                                    \
    EXPECT_TRUE(RunTest(CPUBackendType::CachedInterpreter, code_file, results_file));                                  \
  }                                                                                                                    \
  TEST(Test186_Recompiler, name) { EXPECT_TRUE(RunTest(CPUBackendType::Recompiler, code_file, results_file)); }

MAKE_TEST(add, "test186/add.bin", "test186/res_add.bin")
MAKE_TEST(bcdcnv, "test186/bcdcnv.bin", "test186/res_bcdcnv.bin")
MAKE_TEST(bitwise, "test186/bitwise.bin", "test186/res_bitwise.bin")
MAKE_TEST(cmpneg, "test186/cmpneg.bin", "test186/res_cmpneg.bin")
MAKE_TEST(control, "test186/control.bin", "test186/res_control.bin")
MAKE_TEST(datatrnf, "test186/datatrnf.bin", "test186/res_datatrnf.bin")
MAKE_TEST(div, "test186/div.bin", "test186/res_div.bin")
MAKE_TEST(interrupt, "test186/interrupt.bin", "test186/res_interrupt.bin")
MAKE_TEST(jmpmov, "test186/jmpmov.bin", "test186/res_jmpmov.bin")
MAKE_TEST(jump1, "test186/jump1.bin", "test186/res_jump1.bin")
MAKE_TEST(jump2, "test186/jump2.bin", "test186/res_jump2.bin")
MAKE_TEST(mul, "test186/mul.bin", "test186/res_mul.bin")
MAKE_TEST(rep, "test186/rep.bin", "test186/res_rep.bin")
MAKE_TEST(rotate, "test186/rotate.bin", "test186/res_rotate.bin")
MAKE_TEST(segpr, "test186/segpr.bin", "test186/res_segpr.bin")
MAKE_TEST(shifts, "test186/shifts.bin", "test186/res_shifts.bin")
MAKE_TEST(strings, "test186/strings.bin", "test186/res_strings.bin")
MAKE_TEST(sub, "test186/sub.bin", "test186/res_sub.bin")
