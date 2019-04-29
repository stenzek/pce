#pragma once
#include "pce/cpu_x86/cpu_x86.h"
#include "pce/debugger_interface.h"

class System;

namespace CPU_X86 {

class DebuggerInterface : public ::DebuggerInterface
{
public:
  DebuggerInterface(CPU* cpu, System* system);
  virtual ~DebuggerInterface() override;

  u32 GetRegisterCount() const override;
  RegisterType GetRegisterType(u32 index) const override;
  const char* GetRegisterName(u32 index) const override;
  RegisterValue GetRegisterValue(u32 index) const override;
  void SetRegisterValue(u32 index, RegisterValue value) const override;
  bool ReadMemoryByte(LinearMemoryAddress address, u8* value) override;
  bool ReadMemoryWord(LinearMemoryAddress address, u16* value) override;
  bool ReadMemoryDWord(LinearMemoryAddress address, u32* value) override;
  bool WriteMemoryByte(LinearMemoryAddress address, u8 value) override;
  bool WriteMemoryWord(LinearMemoryAddress address, u16 value) override;
  bool WriteMemoryDWord(LinearMemoryAddress address, u32 value) override;
  bool ReadPhysicalMemoryByte(PhysicalMemoryAddress address, u8* value) override;
  bool ReadPhysicalMemoryWord(PhysicalMemoryAddress address, u16* value) override;
  bool ReadPhysicalMemoryDWord(PhysicalMemoryAddress address, u32* value) override;
  bool WritePhysicalMemoryByte(PhysicalMemoryAddress address, u8 value) override;
  bool WritePhysicalMemoryWord(PhysicalMemoryAddress address, u16 value) override;
  bool WritePhysicalMemoryDWord(PhysicalMemoryAddress address, u32 value) override;
  LinearMemoryAddress GetInstructionPointer() const override;
  RegisterType GetStackValueType() const override;
  LinearMemoryAddress GetStackTop() const override;
  LinearMemoryAddress GetStackBottom() const override;
  bool DisassembleCode(LinearMemoryAddress instruction_pointer, LinearMemoryAddress* out_linear_address,
                       String* out_formatted_address, String* out_code_bytes, String* out_disassembly,
                       u32* out_size) const override;
  bool IsStepping() const override;
  void SetStepping(bool enabled, u32 instructions_to_execute) override;

  // Resets step count to zero
  u32 GetSteppingInstructionCount();

private:
  CPU* m_cpu;
  System* m_system;
  bool m_stepping = false;
  u32 m_stepping_instructions = 0;
};

} // namespace CPU_X86
