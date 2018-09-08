#include "YBaseLib/Log.h"
#include "pce-tests/helpers.h"
#include "pce-tests/testpcsystem.h"
#include "pce/bus.h"
#include "pce/hw/serial.h"
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <vector>
Log_SetChannel(Test186);

static void RunTest386(CPUBackendType cpu_backend)
{
  const char* image_file = "test386/test386.bin";
  const char* output_file = "test386/test386-EE-reference.txt";

  auto system = std::make_unique<TestPCSystem>(CPU_X86::MODEL_386, 4000000.0f, cpu_backend, 1024 * 1024);

  EXPECT_TRUE(system->AddMMIOROMFromFile(image_file, 0xf0000u));
  EXPECT_TRUE(system->AddMMIOROMFromFile(image_file, 0xffff0000u));

  // POST Code Port
  system->GetBus()->ConnectIOPortWrite(
    0x0190, system.get(), [](uint32 port, uint8 value) { Log_InfoPrintf("POST code 0x%02X", uint32(value)); });

  // Add a serial port to the machine at the default COM1 port.
  // We speed this up so the guest spends less time waiting for the serial port..
  HW::Serial* serial_port = system->CreateComponent<HW::Serial>("COM1", HW::Serial::Model_8250, 0x03F8, 3, 1000000000);

  // This is our data buffer that we get back from the guest.
  std::vector<uint8> data_buffer;
  // size_t last_debug_read_offset = 0;

  // Set up callback from serial port to read the data back from the guest.
  serial_port->SetDataReadyCallback([&](size_t count) {
    for (size_t i = 0; i < count; i++)
    {
      uint8 value = 0;
      serial_port->ReadData(&value, 1);
      data_buffer.push_back(value);

      //             // Dump each line to the screen as it comes in.
      //             if (value == '\n')
      //             {
      //                 size_t count = data_buffer.size() - last_debug_read_offset;
      //                 std::string temp(reinterpret_cast<const char*>(&data_buffer[last_debug_read_offset]), count);
      //                 Log_InfoPrint(temp.c_str());
      //                 last_debug_read_offset = data_buffer.size();
      //             }
    }
  });

  EXPECT_TRUE(system->Ready()) << "system did not initialize successfully";

  // Put a cap on the number of cycles, a runtime of 2 minutes should do.
  system->ExecuteSlice(120 * static_cast<SimulationTime>(1000000000));
  EXPECT_TRUE(system->GetX86CPU()->IsHalted());
  EXPECT_TRUE(system->GetX86CPU()->GetRegisters()->CS == 0xF000 && system->GetX86CPU()->GetRegisters()->EIP == 0xE04B);

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

TEST(Test386, Interpreter)
{
  RunTest386(CPUBackendType::Interpreter);
}
TEST(Test386, CachedInterpreter)
{
  RunTest386(CPUBackendType::CachedInterpreter);
}
TEST(Test386, Recompiler)
{
  RunTest386(CPUBackendType::Recompiler);
}