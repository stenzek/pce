#include "../stub_host_interface.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "system.h"
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <vector>
Log_SetChannel(CPU_X86_Test186);

static void RunTest386(CPU::BackendType cpu_backend)
{
  const char* image_file = "test386/test386.bin";
  const char* output_file = "test386/test386-EE-reference.txt";

  // The clock speed has to be sufficiently high to execute the entire test program. 100MHz should do.
  const float cpu_frequency = 100000000.0f;
  CPU_X86_TestSystem* system =
    StubHostInterface::CreateSystem<CPU_X86_TestSystem>(CPU_X86::MODEL_386, cpu_frequency, cpu_backend, 1024 * 1024);

  system->AddROMFile(image_file, 0xf0000u);

  // POST Code Port
  system->GetBus()->ConnectIOPortWrite(
    0x0190, system, [](uint32 port, uint8 value) { Log_InfoPrintf("POST code 0x%02X", uint32(value)); });

  // This is our data buffer that we get back from the guest.
  std::vector<uint8> data_buffer;

  // Printing Port
  system->GetBus()->ConnectIOPortWrite(0x0080, system,
                                       [&data_buffer](uint32 port, uint8 value) { data_buffer.push_back(value); });

  // Initialize the system.
  EXPECT_TRUE(system->Ready()) << "system did not initialize successfully";

  // Put a cap on the number of cycles, a runtime of 2 minutes should do.
  system->ExecuteSlice(120 * static_cast<SimulationTime>(1000000000));
  EXPECT_TRUE(system->GetX86CPU()->IsHalted());

  // Compare output with known correct output.
  // TODO: Refactor this into a general diff method.
  std::ifstream good_output;
  good_output.open(output_file);
  EXPECT_TRUE(good_output.is_open());
  if (good_output.is_open())
  {
    std::istringstream our_output(std::string(reinterpret_cast<char*>(data_buffer.data()), data_buffer.size()));
    for (uint32 line_number = 1; good_output.good() || our_output.good(); line_number++)
    {
      std::string good_line;
      std::string our_line;
      if (good_output.good())
        std::getline(good_output, good_line);
      if (our_output.good())
        std::getline(our_output, our_line);

      if (good_line != our_line)
      {
        std::stringstream message;
        message << "Diff at line " << line_number << ":" << std::endl
                << "Reference: " << good_line << std::endl
                << "Actual:    " << our_line << std::endl
                << "           ";

        size_t max_length = std::max(good_line.length(), our_line.length());
        for (size_t i = 0; i < max_length; i++)
        {
          if (i >= good_line.length() || i >= our_line.length() || good_line[i] != our_line[i])
            message << '^';
          else
            message << ' ';
        }

        EXPECT_EQ(good_line, our_line) << message.str();
      }
    }
  }
}

TEST(CPU_X86_Test386, Interpreter)
{
  RunTest386(CPU::BackendType::Interpreter);
}
TEST(CPU_X86_Test386, CachedInterpreter)
{
  RunTest386(CPU::BackendType::CachedInterpreter);
}
TEST(CPU_X86_Test386, Recompiler)
{
  RunTest386(CPU::BackendType::Recompiler);
}