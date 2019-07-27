#pragma once
#include <array>
#include <initializer_list>
#include <utility>

#include "common/jit_code_buffer.h"

#include "pce/types.h"
#include "pce/cpu_x86/decoder.h"
#include "pce/cpu_x86/types.h"
#include "pce/cpu_x86/recompiler_register_cache.h"

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
  CodeGenerator(JitCodeBuffer* code_buffer);
  ~CodeGenerator();

  static u32 CalculateRegisterOffset(Reg8 reg);
  static u32 CalculateRegisterOffset(Reg16 reg);
  static u32 CalculateRegisterOffset(Reg32 reg);
  static u32 CalculateSegmentRegisterOffset(Segment segment);

  bool CompileBlock(const BlockBase* block, BlockFunctionType* out_function_ptr, size_t* out_code_size);

  //////////////////////////////////////////////////////////////////////////
  // Helpers
  //////////////////////////////////////////////////////////////////////////
  bool IsConstantOperand(const Instruction* instruction, size_t index);
  u32 GetConstantOperand(const Instruction* instruction, size_t index, bool sign_extend);
  bool CompileInstruction(const Instruction* instruction);

  //////////////////////////////////////////////////////////////////////////
  // Code Generation
  //////////////////////////////////////////////////////////////////////////
  void EmitBeginBlock();
  void EmitEndBlock();
  void FinalizeBlock(BlockFunctionType* out_function_ptr, size_t* out_code_size);

  void EmitSignExtend(HostReg to_reg, OperandSize to_size, HostReg from_reg, OperandSize from_size);
  void EmitZeroExtend(HostReg to_reg, OperandSize to_size, HostReg from_reg, OperandSize from_size);
  void EmitCopyValue(HostReg to_reg, const Value& value);

  void EmitLoadGuestRegister(HostReg host_reg, OperandSize guest_size, u8 guest_reg);
  void EmitStoreGuestRegister(OperandSize guest_size, u8 guest_reg, const Value& value);
  void EmitLoadCPUStructField(HostReg host_reg, OperandSize guest_size, u32 offset);
  void EmitStoreCPUStructField(u32 offset, const Value& value);
  void EmitAddCPUStructField(u32 offset, const Value& value);
  void EmitLoadGuestMemory(HostReg dest_reg, OperandSize size, const Value& address, Segment segment);
  void EmitStoreGuestMemory(const Value& value, const Value& address, Segment segment);

  void PrepareStackForCall(u32 num_parameters);
  void RestoreStackAfterCall(u32 num_parameters);

  //void EmitFunctionCall(const void* ptr);
  void EmitFunctionCall(const void* ptr, const Value& arg1);
  //void EmitFunctionCall(const void* ptr, const Value& arg1, const Value& arg2);
  //void EmitFunctionCall(const void* ptr, const Value& arg1, const Value& arg2, const Value& arg3);
  //void EmitFunctionCall(const void* ptr, const Value& arg1, const Value& arg2, const Value& arg3, const Value& arg4);

  // Host register saving.
  void EmitPushHostReg(HostReg reg);
  void EmitPopHostReg(HostReg reg);

private:
  // Host register setup
  void InitHostRegs();

  Value ConvertValueSize(const Value& value, OperandSize size, bool sign_extend);
  void ConvertValueSizeInPlace(Value& value, OperandSize size, bool sign_extend);

  //////////////////////////////////////////////////////////////////////////
  // Code Generation Helpers
  //////////////////////////////////////////////////////////////////////////
  void CalculateEffectiveAddress(const Instruction* instruction);
  Value CalculateOperandMemoryAddress(const Instruction* instruction, size_t index);
  Value ReadOperand(const Instruction* instruction, size_t index, OperandSize output_size, bool sign_extend);
  void WriteOperand(const Instruction* instruction, size_t index, Value&& value);
  void EmitInstructionPrologue(const Instruction* instruction, bool force_sync = false);
  void SyncInstructionPointer();
  void SyncCurrentEIP();
  void SyncCurrentESP();

  //////////////////////////////////////////////////////////////////////////
  // Instruction Code Generators
  //////////////////////////////////////////////////////////////////////////
  bool Compile_Fallback(const Instruction* instruction);
  bool Compile_NOP(const Instruction* instruction);
  bool Compile_LEA(const Instruction* instruction);
  bool Compile_MOV(const Instruction* instruction);

  JitCodeBuffer* m_code_buffer;
  const BlockBase* m_block = nullptr;
  const Instruction* m_block_start = nullptr;
  const Instruction* m_block_end = nullptr;
  RegisterCache m_register_cache;
  CodeEmitter m_emit;

  u32 m_delayed_eip_add = 0;
  u32 m_delayed_current_eip_add = 0;
  u32 m_delayed_cycles_add = 0;

  std::array<Value, 3> m_operand_memory_addresses{};
};

} // namespace CPU_X86::Recompiler