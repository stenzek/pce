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
    uint64 val_qword;
    uint32 val_dword;
    uint16 val_word;
    uint8 val_byte;
  };

  virtual ~DebuggerInterface() {}

  // Get the number of guest registers in the CPU
  virtual uint32 GetRegisterCount() const = 0;

  // Get the type/name of register
  virtual RegisterType GetRegisterType(uint32 index) const = 0;
  virtual const char* GetRegisterName(uint32 index) const = 0;

  // Get the current value of a register
  virtual RegisterValue GetRegisterValue(uint32 index) const = 0;

  // Sets the current value of a register
  virtual void SetRegisterValue(uint32 index, RegisterValue value) const = 0;

  // Read memory at the specified location, applying page translation
  virtual bool ReadMemoryByte(LinearMemoryAddress address, uint8* value) = 0;
  virtual bool ReadMemoryWord(LinearMemoryAddress address, uint16* value) = 0;
  virtual bool ReadMemoryDWord(LinearMemoryAddress address, uint32* value) = 0;
  virtual bool WriteMemoryByte(LinearMemoryAddress address, uint8 value) = 0;
  virtual bool WriteMemoryWord(LinearMemoryAddress address, uint16 value) = 0;
  virtual bool WriteMemoryDWord(LinearMemoryAddress address, uint32 value) = 0;

  // Read memory at the specified location, not applying page translation
  virtual bool ReadPhysicalMemoryByte(PhysicalMemoryAddress address, uint8* value) = 0;
  virtual bool ReadPhysicalMemoryWord(PhysicalMemoryAddress address, uint16* value) = 0;
  virtual bool ReadPhysicalMemoryDWord(PhysicalMemoryAddress address, uint32* value) = 0;
  virtual bool WritePhysicalMemoryByte(PhysicalMemoryAddress address, uint8 value) = 0;
  virtual bool WritePhysicalMemoryWord(PhysicalMemoryAddress address, uint16 value) = 0;
  virtual bool WritePhysicalMemoryDWord(PhysicalMemoryAddress address, uint32 value) = 0;

  // Get the liner address of the current instruction
  virtual LinearMemoryAddress GetInstructionPointer() const = 0;

  // Examine the stack in the guest
  virtual RegisterType GetStackValueType() const = 0;
  virtual LinearMemoryAddress GetStackTop() const = 0;
  virtual LinearMemoryAddress GetStackBottom() const = 0;

  // Dissemble code at the specified linear address
  virtual bool DisassembleCode(LinearMemoryAddress address, String* out_line, uint32* out_size) const = 0;

  // Single stepping
  // This also indicates whether we are broken into the debugger or not
  virtual bool IsStepping() const = 0;
  virtual void SetStepping(bool enabled, uint32 instructions_to_execute = 0) = 0;
};
