#pragma once

#include <memory>
#include <vector>

#include "pce/cpu_x86/cpu_x86.h"
#include "pce/hw/i8259_pic.h"
#include "pce/system.h"

class ByteStream;

class CPU_X86_TestSystem : public System
{
  DECLARE_OBJECT_TYPE_INFO(CPU_X86_TestSystem, System);
  DECLARE_OBJECT_NO_FACTORY(CPU_X86_TestSystem);
  DECLARE_OBJECT_NO_PROPERTIES(CPU_X86_TestSystem);

public:
  static const PhysicalMemoryAddress BIOS_ROM_ADDRESS = 0xF0000;
  static const uint32 BIOS_ROM_SIZE = 65536;

  CPU_X86_TestSystem(CPU_X86::Model cpu_model = CPU_X86::MODEL_486, float cpu_frequency = 1000000.0f,
                     CPUBackendType cpu_backend = CPUBackendType::Interpreter, uint32 ram_size = 1024 * 1024);

  ~CPU_X86_TestSystem();

  CPU_X86::CPU* GetX86CPU() const { return static_cast<CPU_X86::CPU*>(m_cpu); }

  void AddROMFile(const char* filename, PhysicalMemoryAddress load_address, u32 expected_size = 0);

  bool Ready();

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
