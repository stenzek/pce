#pragma once
#include "pce/cpu_x86/recompiler_backend.h"
#include <utility>

namespace CPU_X86 {

class RecompilerTrampolines
{
  static uint8 ReadMemoryByteTrampoline(CPU* cpu, uint32 segment, uint32 offset);
  static uint16 ReadMemoryWordTrampoline(CPU* cpu, uint32 segment, uint32 offset);
  static uint32 ReadMemoryDWordTrampoline(CPU* cpu, uint32 segment, uint32 offset);
  static void WriteMemoryByteTrampoline(CPU* cpu, uint32 segment, uint32 offset, uint8 value);
  static void WriteMemoryWordTrampoline(CPU* cpu, uint32 segment, uint32 offset, uint16 value);
  static void WriteMemoryDWordTrampoline(CPU* cpu, uint32 segment, uint32 offset, uint32 value);
  static void BranchToTrampoline(CPU* cpu, uint32 address);
  static void PushWordTrampoline(CPU* cpu, uint16 value);
  static void PushDWordTrampoline(CPU* cpu, uint32 value);
  static uint16 PopWordTrampoline(CPU* cpu);
  static uint32 PopDWordTrampoline(CPU* cpu);
  static void LoadSegmentRegisterTrampoline(CPU* cpu, uint32 segment, uint16 value);
  static void RaiseExceptionTrampoline(CPU* cpu, uint32 interrupt, uint32 error_code);
  static void SetFlagsTrampoline(CPU* cpu, uint32 flags);
  static void SetFlags16Trampoline(CPU* cpu, uint16 flags);
  static void FarJumpTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size);
  static void FarCallTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size);
  static void FarReturnTrampoline(CPU* cpu, uint32 op_size, uint32 pop_count);
};

} // namespace CPU_X86
