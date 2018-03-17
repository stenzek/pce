#pragma once
#include "pce/cpu_x86/cpu.h"
#include "pce/debugger_interface.h"

class System;

namespace CPU_X86 {

class DebuggerInterface : public ::DebuggerInterface
{
public:
  DebuggerInterface(CPU* cpu, System* system);
  virtual ~DebuggerInterface() override;

  uint32 GetRegisterCount() const override;
  RegisterType GetRegisterType(uint32 index) const override;
  const char* GetRegisterName(uint32 index) const override;
  RegisterValue GetRegisterValue(uint32 index) const override;
  void SetRegisterValue(uint32 index, RegisterValue value) const override;
  bool ReadMemoryByte(LinearMemoryAddress address, uint8* value) override;
  bool ReadMemoryWord(LinearMemoryAddress address, uint16* value) override;
  bool ReadMemoryDWord(LinearMemoryAddress address, uint32* value) override;
  bool WriteMemoryByte(LinearMemoryAddress address, uint8 value) override;
  bool WriteMemoryWord(LinearMemoryAddress address, uint16 value) override;
  bool WriteMemoryDWord(LinearMemoryAddress address, uint32 value) override;
  bool ReadPhysicalMemoryByte(PhysicalMemoryAddress address, uint8* value) override;
  bool ReadPhysicalMemoryWord(PhysicalMemoryAddress address, uint16* value) override;
  bool ReadPhysicalMemoryDWord(PhysicalMemoryAddress address, uint32* value) override;
  bool WritePhysicalMemoryByte(PhysicalMemoryAddress address, uint8 value) override;
  bool WritePhysicalMemoryWord(PhysicalMemoryAddress address, uint16 value) override;
  bool WritePhysicalMemoryDWord(PhysicalMemoryAddress address, uint32 value) override;
  LinearMemoryAddress GetInstructionPointer() const override;
  RegisterType GetStackValueType() const override;
  LinearMemoryAddress GetStackTop() const override;
  LinearMemoryAddress GetStackBottom() const override;
  bool DisassembleCode(LinearMemoryAddress address, String* out_line, uint32* out_size) const override;
  bool IsStepping() const override;
  void SetStepping(bool enabled, uint32 instructions_to_execute) override;

  // Resets step count to zero
  uint32 GetSteppingInstructionCount();

private:
  CPU* m_cpu;
  System* m_system;
  bool m_stepping = false;
  uint32 m_stepping_instructions = 0;
};

} // namespace CPU_X86
