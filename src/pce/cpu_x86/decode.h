#pragma once

#include "pce/cpu_x86/types.h"

class String;

namespace CPU_X86 {

struct InstructionFetchCallback
{
  virtual uint8 FetchByte() = 0;
  virtual uint16 FetchWord();
  virtual uint32 FetchDWord();
};

// Instruction decoding
bool DecodeInstruction(OldInstruction* instruction, AddressSize address_size, OperandSize operand_size,
                       InstructionFetchCallback& fetch_callback);

// Instruction disassembly
bool DisassembleToString(const OldInstruction* instruction, VirtualMemoryAddress physical_address, String* out_string);

} // namespace CPU_X86