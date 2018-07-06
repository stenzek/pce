#pragma once

#include <memory>
#include <vector>

#include "pce/cpu_x86/cpu.h"
#include "pce/hw/i8042_ps2.h"
#include "pce/hw/i8237_dma.h"
#include "pce/hw/i8253_pit.h"
#include "pce/hw/i8259_pic.h"
#include "pce/systems/pcbase.h"

class ByteStream;

class TestPCSystem : public Systems::PCBase
{
public:
  static const PhysicalMemoryAddress BIOS_ROM_ADDRESS = 0xF0000;
  static const uint32 BIOS_ROM_SIZE = 65536;

  TestPCSystem(CPU_X86::Model cpu_model = CPU_X86::MODEL_486, float cpu_frequency = 1000000.0f,
               CPUBackendType cpu_backend = CPUBackendType::Interpreter, uint32 ram_size = 1024 * 1024);

  ~TestPCSystem();

  const char* GetSystemName() const override { return "Test Harness PC"; }
  InterruptController* GetInterruptController() const override { return m_interrupt_controller; }

  bool Initialize() override;

  CPU_X86::CPU* GetX86CPU() const { return static_cast<CPU_X86::CPU*>(m_cpu); }

  bool AddROMFromFile(const char* filename, PhysicalMemoryAddress address);

private:
  void AddComponents();

  HW::i8259_PIC* m_interrupt_controller = nullptr;

  std::vector<std::unique_ptr<byte[]>> m_rom_data;
};
