#pragma once
#include <array>
#include <initializer_list>
#include <utility>

#include "common/jit_code_buffer.h"

#include "pce/cpu_x86/decoder.h"
#include "pce/cpu_x86/recompiler_register_cache.h"
#include "pce/cpu_x86/recompiler_thunks.h"
#include "pce/cpu_x86/types.h"
#include "pce/types.h"
#include "xbyak.h"

// ABI selection
#if defined(Y_CPU_X64)
#if defined(Y_PLATFORM_WINDOWS)
#define ABI_WIN64 1
#elif defined(Y_PLATFORM_LINUX) || defined(Y_PLATFORM_OSX)
#define ABI_SYSV 1
#else
#error Unknown ABI.
#endif
#endif

namespace CPU_X86::Recompiler {

class CodeGenerator
{
public:
  CodeGenerator(CPU* cpu, JitCodeBuffer* code_buffer);
  ~CodeGenerator();

  static u32 CalculateRegisterOffset(Reg8 reg);
  static u32 CalculateRegisterOffset(Reg16 reg);
  static u32 CalculateRegisterOffset(Reg32 reg);
  static u32 CalculateSegmentRegisterOffset(Segment segment);
  static const char* GetHostRegName(HostReg reg, OperandSize size = HostPointerSize);

  RegisterCache& GetRegisterCache() { return m_register_cache; }
  CodeEmitter& GetCodeEmitter() { return m_emit; }

  bool CompileBlock(const BlockBase* block, BlockFunctionType* out_function_ptr, size_t* out_code_size);

  //////////////////////////////////////////////////////////////////////////
  // Helpers
  //////////////////////////////////////////////////////////////////////////
  bool IsConstantOperand(const Instruction* instruction, size_t index);
  u32 GetConstantOperand(const Instruction* instruction, size_t index, bool sign_extend);

  //////////////////////////////////////////////////////////////////////////
  // Code Generation
  //////////////////////////////////////////////////////////////////////////
  void EmitBeginBlock();
  void EmitEndBlock();
  void FinalizeBlock(BlockFunctionType* out_function_ptr, size_t* out_code_size);

  void EmitSignExtend(HostReg to_reg, OperandSize to_size, HostReg from_reg, OperandSize from_size);
  void EmitZeroExtend(HostReg to_reg, OperandSize to_size, HostReg from_reg, OperandSize from_size);
  void EmitCopyValue(HostReg to_reg, const Value& value);
  void EmitAdd(HostReg to_reg, const Value& value);
  void EmitSub(HostReg to_reg, const Value& value);
  void EmitCmp(HostReg to_reg, const Value& value);
  void EmitShl(HostReg to_reg, const Value& value);
  void EmitAnd(HostReg to_reg, const Value& value);
  void EmitOr(HostReg to_reg, const Value& value);
  void EmitXor(HostReg to_reg, const Value& value);
  void EmitTest(HostReg to_reg, const Value& value);
  void EmitNot(HostReg to_reg, OperandSize size);

  void EmitLoadGuestRegister(HostReg host_reg, OperandSize guest_size, u8 guest_reg);
  void EmitStoreGuestRegister(OperandSize guest_size, u8 guest_reg, const Value& value);
  void EmitLoadCPUStructField(HostReg host_reg, OperandSize guest_size, u32 offset);
  void EmitStoreCPUStructField(u32 offset, const Value& value);
  void EmitAddCPUStructField(u32 offset, const Value& value);

  u32 PrepareStackForCall();
  void RestoreStackAfterCall(u32 adjust_size);

  // void EmitFunctionCall(const void* ptr);
  void EmitFunctionCall(Value* return_value, const void* ptr, const Value& arg1);
  void EmitFunctionCall(Value* return_value, const void* ptr, const Value& arg1, const Value& arg2);
  void EmitFunctionCall(Value* return_value, const void* ptr, const Value& arg1, const Value& arg2, const Value& arg3);
  void EmitFunctionCall(Value* return_value, const void* ptr, const Value& arg1, const Value& arg2, const Value& arg3,
                        const Value& arg4);

#if 0
  template<typename R, typename A1, typename A2>
  void EmitCPUFunctionCall(Value* return_value, R (CPU::*ptr)(A1, A2), const Value& arg1, const Value& arg2)
  {
    return EmitFunctionCall(return_value, std::mem_fn(ptr), m_register_cache.GetCPUPtr(), arg1, arg2);
  }
#endif

  // Host register saving.
  void EmitPushHostReg(HostReg reg);
  void EmitPopHostReg(HostReg reg);

  // Flags copying from host.
#if defined(Y_CPU_X64)
  Value ReadFlagsFromHost();
#endif

  // Value ops
  Value AddValues(const Value& lhs, const Value& rhs);
  Value MulValues(const Value& lhs, const Value& rhs);
  Value ShlValues(const Value& lhs, const Value& rhs);

  // EFLAGS merging.
  void UpdateEFLAGS(Value&& merge_value, u32 clear_flags_mask, u32 copy_flags_mask, u32 set_flags_mask);

  // Guest stack operations.
  void GuestPush(const Value& value);
  void GuestPush(Value&& value);
  Value GuestPop(OperandSize size);

private:
  // Host register setup
  void InitHostRegs();

  Value ConvertValueSize(const Value& value, OperandSize size, bool sign_extend);
  void ConvertValueSizeInPlace(Value* value, OperandSize size, bool sign_extend);

  //////////////////////////////////////////////////////////////////////////
  // Code Generation Helpers
  //////////////////////////////////////////////////////////////////////////
  void CalculateEffectiveAddress(const Instruction& instruction);
  Value CalculateOperandMemoryAddress(const Instruction& instruction, size_t index);
  Value CalculateJumpTarget(const Instruction& instruction, size_t index = 0);
  Value ReadOperand(const Instruction& instruction, size_t index, OperandSize output_size, bool sign_extend,
                    bool force_host_register = false);
  Value WriteOperand(const Instruction& instruction, size_t index, Value&& value);
  void LoadSegmentMemory(Value* dest_value, OperandSize size, const Value& address, Segment segment);
  void StoreSegmentMemory(const Value& value, const Value& address, Segment segment);
  void RaiseException(u32 exception, const Value& ec = Value::FromConstantU32(0));
  void InstructionPrologue(const Instruction& instruction, CycleCount cycles, bool force_sync = false);
  void SyncInstructionPointer();
  void SyncCurrentEIP();
  void SyncCurrentESP();

  Value GetSignFlag(const Value& value);
  Value GetZeroFlag(const Value& value);
  Value GetParityFlag(const Value& value);

  //////////////////////////////////////////////////////////////////////////
  // Instruction Code Generators
  //////////////////////////////////////////////////////////////////////////
  bool CompileInstruction(const Instruction& instruction);
  bool Compile_Fallback(const Instruction& instruction);
  bool Compile_NOP(const Instruction& instruction);
  bool Compile_LEA(const Instruction& instruction);
  bool Compile_MOV(const Instruction& instruction);
  bool Compile_Bitwise(const Instruction& instruction);
  bool Compile_Bitwise_Impl(const Instruction& instruction, CycleCount cycles);
  bool Compile_NOT(const Instruction& instruction);
  bool Compile_AddSub(const Instruction& instruction);
  bool Compile_AddSub_Impl(const Instruction& instruction, CycleCount cycles);
  bool Compile_PUSH(const Instruction& instruction);
  bool Compile_POP(const Instruction& instruction);
  bool Compile_CALL_Near(const Instruction& instruction);
  bool Compile_RET_Near(const Instruction& instruction);

  CPU* m_cpu;
  JitCodeBuffer* m_code_buffer;
  const BlockBase* m_block = nullptr;
  const Instruction* m_block_start = nullptr;
  const Instruction* m_block_end = nullptr;
  RegisterCache m_register_cache;
  CodeEmitter m_emit;

  u32 m_delayed_eip_add = 0;
  u32 m_delayed_current_eip_add = 0;
  CycleCount m_delayed_cycles_add = 0;

  std::array<Value, 3> m_operand_memory_addresses{};
};

} // namespace CPU_X86::Recompiler
