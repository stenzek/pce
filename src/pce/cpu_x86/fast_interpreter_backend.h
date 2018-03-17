#pragma once

#include "pce/cpu_x86/backend.h"
#include "pce/cpu_x86/cpu.h"
#include "pce/cpu_x86/decode.h"
#include <csetjmp>

namespace CPU_X86 {
enum FIOperandMode : uint8
{
  FIOperandMode_None,
  FIOperandMode_Constant,
  FIOperandMode_Register,
  FIOperandMode_SegmentRegister,
  FIOperandMode_Immediate,
  FIOperandMode_Relative,
  FIOperandMode_Memory,
  FIOperandMode_FarMemory,
  FIOperandMode_ModRM_Reg,
  FIOperandMode_ModRM_RM,
  FIOperandMode_ModRM_SegmentReg,
  FIOperandMode_ModRM_ControlRegister,
  FIOperandMode_ModRM_DebugRegister,
  FIOperandMode_ModRM_TestRegister,
};

class FastInterpreterBackend : public Backend
{
public:
  FastInterpreterBackend(CPU* cpu);
  ~FastInterpreterBackend();

  void Reset() override;
  void Execute() override;
  void AbortCurrentInstruction() override;
  void BranchTo(uint32 new_EIP) override;
  void BranchFromException(uint32 new_EIP) override;

  void OnControlRegisterLoaded(Reg32 reg, uint32 old_value, uint32 new_value) override;

  void OnLockedMemoryAccess(PhysicalMemoryAddress address, PhysicalMemoryAddress range_start,
                            PhysicalMemoryAddress range_end, MemoryLockAccess access) override;

  void FlushCodeCache() override;

private:
  CPU* m_cpu;
  Bus* m_bus;
  System* m_system;

#ifdef Y_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
  std::jmp_buf m_jmp_buf = {};
#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif

  struct InstructionState
  {
    VirtualMemoryAddress effective_address;
    union
    {
      uint8 value8;
      uint16 value16;
      uint32 value32;
    } immediate;

    OperandSize operand_size;
    AddressSize address_size;
    Segment segment;
    uint8 modrm;

    // TODO: Use flags for these
    bool modrm_rm_register;
    bool has_segment_override;
    bool has_rep;
    bool has_repne;

    uint8 GetModRM_Reg() const { return ((modrm >> 3) & 7); }
  };

  void ExecuteInstruction();

  // Raises undefined opcode exception.
  void RaiseInvalidOpcode();

  // Falls back to interpreter for handling the current instruction.
  void FallbackToInterpreter();

  // Calculate the effective address for memory operands
  void FetchModRM(InstructionState* istate);
  template<FIOperandMode op_mode, OperandSize op_size>
  void FetchImmediate(InstructionState* istate);
  template<FIOperandMode op_mode>
  void CalculateEffectiveAddress(InstructionState* istate);

  // Reads operands based on operand mode
  template<FIOperandMode mode, uint32 constant>
  uint8 ReadByteOperand(InstructionState* istate);
  template<FIOperandMode mode, uint32 constant>
  uint16 ReadWordOperand(InstructionState* istate);
  template<FIOperandMode mode, uint32 constant>
  uint32 ReadDWordOperand(InstructionState* istate);
  template<FIOperandMode mode, OperandSize size, uint32 constant>
  uint16 ReadZeroExtendedWordOperand(InstructionState* istate);
  template<FIOperandMode mode, OperandSize size, uint32 constant>
  uint32 ReadZeroExtendedDWordOperand(InstructionState* istate);
  template<FIOperandMode mode, OperandSize size, uint32 constant>
  uint16 ReadSignExtendedWordOperand(InstructionState* istate);
  template<FIOperandMode mode, OperandSize size, uint32 constant>
  uint32 ReadSignExtendedDWordOperand(InstructionState* istate);
  template<FIOperandMode mode, uint32 constant>
  void WriteByteOperand(InstructionState* istate, uint8 value);
  template<FIOperandMode mode, uint32 constant>
  void WriteWordOperand(InstructionState* istate, uint16 value);
  template<FIOperandMode mode, uint32 constant>
  void WriteDWordOperand(InstructionState* istate, uint32 value);

  template<FIOperandMode mode>
  void ReadFarAddressOperand(InstructionState* istate, OperandSize size, uint16* segment_selector,
                             VirtualMemoryAddress* address);

  template<JumpCondition condition>
  bool TestJumpCondition(InstructionState* istate);

  void DispatchInstruction(InstructionState* istate, uint8 opcode);

  // REP handler
  void Execute_REP(InstructionState* istate, bool is_REPNE);
  void Execute_0F(InstructionState* istate);

  // Instruction groups
  template<FIOperandMode op1_mode, OperandSize op1_size, uint32 op1_constant, FIOperandMode op2_mode,
           OperandSize op2_size, uint32 op2_constant>
  void Execute_GRP1(InstructionState* istate);
  template<FIOperandMode op1_mode, OperandSize op1_size, uint32 op1_constant, FIOperandMode op2_mode,
           OperandSize op2_size, uint32 op2_constant>
  void Execute_GRP2(InstructionState* istate);
  template<FIOperandMode op1_mode, OperandSize op1_size, uint32 op1_constant, FIOperandMode op2_mode,
           OperandSize op2_size, uint32 op2_constant>
  void Execute_GRP3(InstructionState* istate);
  template<FIOperandMode op1_mode, OperandSize op1_size, uint32 op1_constant>
  void Execute_GRP4(InstructionState* istate);
  template<FIOperandMode op1_mode, OperandSize op1_size, uint32 op1_constant>
  void Execute_GRP5(InstructionState* istate);

  // Instruction handlers
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_ADD(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_ADC(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_SUB(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_SBB(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_CMP(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_AND(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_OR(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_XOR(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_TEST(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_MOV(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_MOVZX(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_MOVSX(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_MOV_Sreg(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_XCHG(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
           OperandSize count_size, uint32 count_constant>
  void Execute_SHL(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
           OperandSize count_size, uint32 count_constant>
  void Execute_SHR(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
           OperandSize count_size, uint32 count_constant>
  void Execute_SAR(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
           OperandSize count_size, uint32 count_constant>
  void Execute_RCL(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
           OperandSize count_size, uint32 count_constant>
  void Execute_RCR(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
           OperandSize count_size, uint32 count_constant>
  void Execute_ROL(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
           OperandSize count_size, uint32 count_constant>
  void Execute_ROR(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_IN(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_OUT(InstructionState* istate);

  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
  void Execute_INC(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
  void Execute_DEC(InstructionState* istate);

  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
  void Execute_NOT(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
  void Execute_NEG(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
  void Execute_MUL(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
  void Execute_IMUL1(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
  void Execute_DIV(InstructionState* istate);
  template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
  void Execute_IDIV(InstructionState* istate);

  template<FIOperandMode src_mode, OperandSize src_size, uint32 src_constant>
  void Execute_PUSH(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
  void Execute_POP(InstructionState* istate);

  template<FIOperandMode src_mode, OperandSize src_size, uint32 src_constant>
  void Execute_PUSH_Sreg(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
  void Execute_POP_Sreg(InstructionState* istate);

  void Execute_PUSHA(InstructionState* istate);
  void Execute_POPA(InstructionState* istate);

  template<FIOperandMode frame_mode, OperandSize frame_size, uint32 frame_constant, FIOperandMode level_mode,
           OperandSize level_size, uint32 level_constant>
  void Execute_ENTER(InstructionState* istate);
  void Execute_LEAVE(InstructionState* istate);

  template<FIOperandMode sreg_mode, OperandSize sreg_size, uint32 sreg_constant, FIOperandMode reg_mode,
           OperandSize reg_size, uint32 reg_constant, FIOperandMode ptr_mode, OperandSize ptr_size, uint32 ptr_constant>
  void Execute_LXS(InstructionState* istate);

  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_LEA(InstructionState* istate);

  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
  VirtualMemoryAddress CalculateJumpTarget(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
  void Execute_JMP_Near(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, JumpCondition condition>
  void Execute_Jcc(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, JumpCondition condition>
  void Execute_LOOP(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
  void Execute_CALL_Near(InstructionState* istate);
  template<FIOperandMode dst_mode = FIOperandMode_None, OperandSize dst_size = OperandSize_Count,
           uint32 dst_constant = 0>
  void Execute_RET_Near(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
  void Execute_JMP_Far(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
  void Execute_CALL_Far(InstructionState* istate);
  template<FIOperandMode dst_mode = FIOperandMode_None, OperandSize dst_size = OperandSize_Count,
           uint32 dst_constant = 0>
  void Execute_RET_Far(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
  void Execute_INT(InstructionState* istate);
  void Execute_INTO(InstructionState* istate);
  void Execute_IRET(InstructionState* istate);

  // String operations
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_MOVS(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_CMPS(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_STOS(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_LODS(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_SCAS(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_INS(InstructionState* istate);
  template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
           OperandSize src_size, uint32 src_constant>
  void Execute_OUTS(InstructionState* istate);

  // No-operand instructions
  void Execute_CLC(InstructionState* istate);
  void Execute_CLD(InstructionState* istate);
  void Execute_CLI(InstructionState* istate);
  void Execute_CMC(InstructionState* istate);
  void Execute_STC(InstructionState* istate);
  void Execute_STD(InstructionState* istate);
  void Execute_STI(InstructionState* istate);
  void Execute_LAHF(InstructionState* istate);
  void Execute_SAHF(InstructionState* istate);
  void Execute_PUSHF(InstructionState* istate);
  void Execute_POPF(InstructionState* istate);
  void Execute_HLT(InstructionState* istate);
  void Execute_CBW(InstructionState* istate);
  void Execute_CWD(InstructionState* istate);
  void Execute_XLAT(InstructionState* istate);

  // BCD stuff
  void Execute_AAA(InstructionState* istate);
  void Execute_AAS(InstructionState* istate);
  template<FIOperandMode op_mode, OperandSize op_size, uint32 op_constant>
  void Execute_AAM(InstructionState* istate);
  template<FIOperandMode op_mode, OperandSize op_size, uint32 op_constant>
  void Execute_AAD(InstructionState* istate);
  void Execute_DAA(InstructionState* istate);
  void Execute_DAS(InstructionState* istate);
};
} // namespace CPU_X86