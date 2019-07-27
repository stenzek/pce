#pragma once
#include "pce/cpu_x86/code_cache_types.h"
#include "pce/cpu_x86/cpu_x86.h"
#include "pce/types.h"

#if defined(Y_CPU_X64)
#include "xbyak.h"
#endif

namespace CPU_X86::Recompiler {

class Backend;
class CodeGenerator;
class RegisterCache;

#if defined(Y_CPU_X64)
using HostReg = Xbyak::Operand::Code;
using CodeEmitter = Xbyak::CodeGenerator;
constexpr u32 HostReg_Count = 16;
constexpr OperandSize HostPointerSize = OperandSize_64;

// A reasonable "maximum" number of bytes per instruction.
constexpr u32 MaximumBytesPerInstruction = 128;

#else
using HostReg = void;
using CodeEmitter = void;
constexpr u32 HostReg_Count = 0;
constexpr OperandSize HostPointerSize = OperandSize_64;
#endif

using BlockFunctionType = void (*)(CPU*);

} // namespace CPU_X86::Recompiler