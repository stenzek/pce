#pragma once

#include <memory>
#include <vector>

#include "pce/cpu_8086/cpu.h"
#include "pce/hw/i8259_pic.h"
#include "pce/system.h"

class ByteStream;

class CPU_8086_TestSystem : public System
{
  DECLARE_OBJECT_TYPE_INFO(CPU_8086_TestSystem, System);
  DECLARE_OBJECT_NO_FACTORY(CPU_8086_TestSystem);
  DECLARE_OBJECT_NO_PROPERTIES(CPU_8086_TestSystem);

public:
  CPU_8086_TestSystem(CPU_8086::Model cpu_model = CPU_8086::MODEL_8086, float cpu_frequency = 1000000.0f,
                      u32 ram_size = 1024 * 1024);
  ~CPU_8086_TestSystem();

  CPU_8086::CPU* Get8086CPU() const { return static_cast<CPU_8086::CPU*>(m_cpu); }

  void AddROMFile(const char* filename, PhysicalMemoryAddress load_address, u32 expected_size = 0);

  bool Execute(SimulationTime timeout = SecondsToSimulationTime(60));

private:
  bool Initialize() override;

  void AddComponents();

  HW::i8259_PIC* m_interrupt_controller = nullptr;

  struct ROMFile
  {
    String filename;
    PhysicalMemoryAddress load_address;
    u32 expected_size;
  };
  std::vector<ROMFile> m_rom_files;
};
