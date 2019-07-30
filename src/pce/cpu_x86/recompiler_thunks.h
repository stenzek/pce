#pragma once
#include "pce/cpu_x86/cpu_x86.h"

namespace CPU_X86::Recompiler {

class Thunks
{
public:
  //////////////////////////////////////////////////////////////////////////
  // Trampolines for calling back from the JIT
  // Needed because we can't cast member functions to void*...
  //////////////////////////////////////////////////////////////////////////
  static u8 ReadSegmentMemoryByte(CPU* cpu, Segment segment, VirtualMemoryAddress address);
  static u16 ReadSegmentMemoryWord(CPU* cpu, Segment segment, VirtualMemoryAddress address);
  static u32 ReadSegmentMemoryDWord(CPU* cpu, Segment segment, VirtualMemoryAddress address);
  static void WriteSegmentMemoryByte(CPU* cpu, Segment segment, VirtualMemoryAddress address, u8 value);
  static void WriteSegmentMemoryWord(CPU* cpu, Segment segment, VirtualMemoryAddress address, u16 value);
  static void WriteSegmentMemoryDWord(CPU* cpu, Segment segment, VirtualMemoryAddress address, u32 value);
  static void RaiseException(CPU* cpu, u32 exception, u32 error_code);
  static void PushWord(CPU* cpu, u16 value);
  static void PushDWord(CPU* cpu, u32 value);
};

} // namespace CPU_X86::Recompiler
