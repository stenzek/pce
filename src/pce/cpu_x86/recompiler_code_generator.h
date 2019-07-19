#pragma once
#include <array>
#include <initializer_list>
#include <utility>

#include "pce/types.h"

#include "pce/cpu_x86/decoder.h"
#include "pce/cpu_x86/recompiler_backend.h"
#include "pce/cpu_x86/types.h"

struct dasm_State;

namespace CPU_X86::Recompiler {

class CodeGenerator
{
public:
  CodeGenerator(Backend* backend, Block* block);
  ~CodeGenerator();

  bool CompileBlock();

private:
  u32 CalculateRegisterOffset(Reg8 reg);
  u32 CalculateRegisterOffset(Reg16 reg);
  u32 CalculateRegisterOffset(Reg32 reg);
  u32 CalculateSegmentRegisterOffset(Segment segment);

  //////////////////////////////////////////////////////////////////////////
  // Register Allocation/Cache
  //////////////////////////////////////////////////////////////////////////
  void SetHostRegAllocationOrder(std::initializer_list<HostReg> regs);
  void SetCallerSavedHostRegs(std::initializer_list<HostReg> regs);

  /// Returns true if the register is permitted to be used in the register cache.
  bool IsCacheableHostRegister(HostReg reg) const;

  HostReg AllocateHostReg(OperandSize size, HostRegState state = HostRegState::InUse);
  Value AllocateTemporaryHostReg(OperandSize size);
  void FreeHostReg(HostReg reg);
  void ReleaseValue(Value& value);
  void ConvertValueSize(Value& value, OperandSize size, bool sign_extend);

  void FlushOverlappingGuestRegisters(Reg8 guest_reg);
  void FlushOverlappingGuestRegisters(Reg16 guest_reg);
  void FlushOverlappingGuestRegisters(Reg32 guest_reg);

  Value ReadGuestRegister(Reg8 guest_reg, bool cache = true);
  Value ReadGuestRegister(Reg16 guest_reg, bool cache = true);
  Value ReadGuestRegister(Reg32 guest_reg, bool cache = true);

  /// Creates a copy of value, and stores it to guest_reg.
  void WriteGuestRegister(Reg8 guest_reg, Value&& value);
  void WriteGuestRegister(Reg16 guest_reg, Value&& value);
  void WriteGuestRegister(Reg32 guest_reg, Value&& value);

  /// Binds host_reg to guest_reg, ignoring any previously cached value.
  void BindHostRegToGuestReg(HostReg host_reg, Reg8 guest_reg);
  void BindHostRegToGuestReg(HostReg host_reg, Reg16 guest_reg);
  void BindHostRegToGuestReg(HostReg host_reg, Reg32 guest_reg);

  void FlushGuestRegister(Reg8 guest_reg, bool invalidate);
  void FlushGuestRegister(Reg16 guest_reg, bool invalidate);
  void FlushGuestRegister(Reg32 guest_reg, bool invalidate);
  void InvalidateGuestRegister(Reg8 guest_reg);
  void InvalidateGuestRegister(Reg16 guest_reg);
  void InvalidateGuestRegister(Reg32 guest_reg);

  void FlushGuestRegister(OperandSize guest_size, u8 guest_reg, bool invalidate);
  void FlushAllGuestRegisters(bool invalidate);
  HostReg EvictOneGuestRegister();

  //////////////////////////////////////////////////////////////////////////
  // Helpers
  //////////////////////////////////////////////////////////////////////////
  bool IsConstantOperand(const Instruction* instruction, size_t index);
  u32 GetConstantOperand(const Instruction* instruction, size_t index, bool sign_extend);
  bool CompileInstruction(const Instruction* instruction, bool is_final);

  //////////////////////////////////////////////////////////////////////////
  // Code Generation
  //////////////////////////////////////////////////////////////////////////
  void BeginBlock();
  void EndBlock();

  void PushLockedHostRegisters();
  void PopLockedHostRegisters();

  void EmitSignExtend(HostReg to_reg, OperandSize to_size, HostReg from_reg, OperandSize from_size);
  void EmitCopyValue(HostReg to_reg, const Value& value);

  void EmitLoadGuestRegister(HostReg host_reg, OperandSize guest_size, u8 guest_reg);
  void EmitStoreGuestRegister(OperandSize guest_size, u8 guest_reg);
  void EmitLoadCPUStructField(HostReg host_reg, OperandSize guest_size, u32 offset);

  HostReg EmitLoadGuestMemory(OperandSize size, const Value& address, Segment segment);
  void EmitStoreGuestMemory(const Value& value, const Value& address, Segment segment);

  //////////////////////////////////////////////////////////////////////////
  // Code Generation Helpers
  //////////////////////////////////////////////////////////////////////////
  Value CalculateEffectiveAddress(const Instruction* instruction, size_t index);
  Value ReadOperand(const Instruction* instruction, size_t index, OperandSize output_size, bool sign_extend);
  void WriteOperand(const Instruction* instruction, size_t index, Value&& value);

  //////////////////////////////////////////////////////////////////////////
  // Instruction Code Generators
  //////////////////////////////////////////////////////////////////////////
  bool Compile_NOP(const Instruction* instruction);
  bool Compile_LEA(const Instruction* instruction);
  bool Compile_MOV(const Instruction* instruction);

  Backend* m_backend;
  Block* m_block;
  const Instruction* m_block_start;
  const Instruction* m_block_end;

  struct HostRegData
  {
    HostRegState state;
    OperandSize size;
  };
  std::array<HostRegData, HostReg_Count> m_host_registers{};
  std::array<HostReg, HostReg_Count> m_host_register_allocation_order{};
  u32 m_host_register_available_count = 0;

  std::array<GuestRegData, Reg8_Count> m_guest_reg8_state{};
  std::array<GuestRegData, Reg16_Count> m_guest_reg16_state{};
  std::array<GuestRegData, Reg32_Count> m_guest_reg32_state{};

  u32 m_delayed_eip_add = 0;
  u32 m_delayed_cycles_add = 0;

  std::array<Value, 3> m_operand_memory_addresses{};

#if 0
  void ReadFarAddressOperand(const Instruction * instruction, size_t index, const Xbyak::Reg & dest_segment,
                             const Xbyak::Reg & dest_offset);
  void UpdateFlags(uint32 clear_mask, uint32 set_mask, uint32 host_mask);

  void SyncInstructionPointers(const Instruction* next_instruction);
  void StartInstruction(const Instruction* instruction);
  void EndInstruction(const Instruction* instruction, bool update_eip = true, bool update_esp = false);

  bool Compile_NOP(const Instruction* instruction);
  bool Compile_LEA(const Instruction* instruction);
  bool Compile_MOV(const Instruction* instruction);
  bool Compile_MOV_Extended(const Instruction* instruction);
  bool Compile_ALU_Binary_Update(const Instruction* instruction);
  bool Compile_ALU_Binary_Test(const Instruction* instruction);
  bool Compile_ALU_Unary_Update(const Instruction* instruction);
  bool Compile_ShiftRotate(const Instruction* instruction);
  bool Compile_DoublePrecisionShift(const Instruction* instruction);
  bool Compile_JumpConditional(const Instruction* instruction);
  bool Compile_JumpCallReturn(const Instruction* instruction);
  bool Compile_Stack(const Instruction* instruction);
  bool Compile_Flags(const Instruction* instruction);
  bool Compile_Fallback(const Instruction* instruction);

  // Helper/wrapper methods.
  static void GenerateTrampolines(dasm_State* Dst);
  static void BranchToTrampoline(CPU* cpu, uint32 address);
  static void PushWordTrampoline(CPU* cpu, uint16 value);
  static void PushDWordTrampoline(CPU* cpu, uint32 value);
  static uint16 PopWordTrampoline(CPU* cpu);
  static uint32 PopDWordTrampoline(CPU* cpu);
  static void LoadSegmentRegisterTrampoline(CPU* cpu, uint32 segment, uint16 value);
  static void RaiseExceptionTrampoline(CPU* cpu, uint32 interrupt, uint32 error_code);
  static void SetFlagsTrampoline(CPU* cpu, uint32 flags);
  static void FarJumpTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size);
  static void FarCallTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size);
  static void FarReturnTrampoline(CPU* cpu, uint32 op_size, uint32 pop_count);
  static void InterpretInstructionTrampoline(CPU* cpu, const Instruction* instruction);
#endif
};

} // namespace CPU_X86::Recompiler
