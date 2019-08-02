#pragma once
#include "common/jit_code_buffer.h"
#include "pce/cpu_x86/cpu_x86.h"
#include <array>

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
  static void PushWord32(CPU* cpu, u16 value);
  static void PushDWord(CPU* cpu, u32 value);
  static u16 PopWord(CPU* cpu);
  static u32 PopDWord(CPU* cpu);
  static void BranchTo(CPU* cpu, u32 address);
};

class ASMFunctions
{
public:
  u8 (*read_memory_byte)(u8 segment, u32 address);
  u16 (*read_memory_word)(u8 segment, u32 address);
  u32 (*read_memory_dword)(u8 segment, u32 address);
  void (*write_memory_byte)(u8 segment, u32 address, u8 value);
  void (*write_memory_word)(u8 segment, u32 address, u16 value);
  void (*write_memory_dword)(u8 segment, u32 address, u32 value);

  static ASMFunctions Generate(JitCodeBuffer* code_buffer);
};

} // namespace CPU_X86::Recompiler
