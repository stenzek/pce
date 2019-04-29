#pragma once
#include "YBaseLib/String.h"
#include "pce/cpu.h"
#include "pce/types.h"

class DebuggerInterface
{
public:
  enum class RegisterType
  {
    Byte,
    Word,
    DWord,
    QWord
  };

  union RegisterValue
  {
    u64 val_qword;
    u32 val_dword;
    u16 val_word;
    u8 val_byte;
  };

  virtual ~DebuggerInterface() {}

  // Get the number of guest registers in the CPU
  virtual u32 GetRegisterCount() const = 0;

  // Get the type/name of register
  virtual RegisterType GetRegisterType(u32 index) const = 0;
  virtual const char* GetRegisterName(u32 index) const = 0;

  // Get the current value of a register
  virtual RegisterValue GetRegisterValue(u32 index) const = 0;

  // Sets the current value of a register
  virtual void SetRegisterValue(u32 index, RegisterValue value) const = 0;

  // Read memory at the specified location, applying page translation
  virtual bool ReadMemoryByte(LinearMemoryAddress address, u8* value) = 0;
  virtual bool ReadMemoryWord(LinearMemoryAddress address, u16* value) = 0;
  virtual bool ReadMemoryDWord(LinearMemoryAddress address, u32* value) = 0;
  virtual bool WriteMemoryByte(LinearMemoryAddress address, u8 value) = 0;
  virtual bool WriteMemoryWord(LinearMemoryAddress address, u16 value) = 0;
  virtual bool WriteMemoryDWord(LinearMemoryAddress address, u32 value) = 0;

  // Read memory at the specified location, not applying page translation
  virtual bool ReadPhysicalMemoryByte(PhysicalMemoryAddress address, u8* value) = 0;
  virtual bool ReadPhysicalMemoryWord(PhysicalMemoryAddress address, u16* value) = 0;
  virtual bool ReadPhysicalMemoryDWord(PhysicalMemoryAddress address, u32* value) = 0;
  virtual bool WritePhysicalMemoryByte(PhysicalMemoryAddress address, u8 value) = 0;
  virtual bool WritePhysicalMemoryWord(PhysicalMemoryAddress address, u16 value) = 0;
  virtual bool WritePhysicalMemoryDWord(PhysicalMemoryAddress address, u32 value) = 0;

  // Get the liner address of the current instruction
  virtual LinearMemoryAddress GetInstructionPointer() const = 0;

  // Examine the stack in the guest
  virtual RegisterType GetStackValueType() const = 0;
  virtual LinearMemoryAddress GetStackTop() const = 0;
  virtual LinearMemoryAddress GetStackBottom() const = 0;

  // Dissemble code at the specified linear address in the current code segment
  virtual bool DisassembleCode(LinearMemoryAddress instruction_pointer, LinearMemoryAddress* out_linear_address,
                               String* out_formatted_address, String* out_code_bytes, String* out_disassembly,
                               u32* out_size) const = 0;

  // Single stepping
  // This also indicates whether we are broken into the debugger or not
  virtual bool IsStepping() const = 0;
  virtual void SetStepping(bool enabled, u32 instructions_to_execute = 0) = 0;
};
