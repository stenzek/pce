#pragma once
#include "cpu_x86.h"
#include "decoder.h"

#include <map>
#include <softfloat.h>
#include <softfloatx80.h>

namespace CPU_X86 {
class Interpreter
{
public:
  // Instruction executer, can be used by other backends.
  static void ExecuteInstruction(CPU* cpu);

  // Retrieves a function pointer to execute a given instruction.
  using HandlerFunction = void (*)(CPU*);
  static HandlerFunction GetInterpreterHandlerForInstruction(const Instruction* instruction);

  static void RaiseInvalidOpcode(CPU* cpu);

private:
  // Helper routines
  static inline void FetchModRM(CPU* cpu);

  template<OperandSize op_size, OperandMode op_mode, u32 op_constant>
  static inline void FetchImmediate(CPU* cpu);

  static inline void Dispatch(CPU* cpu);

  // Calculate the effective address for memory operands
  template<OperandMode op_mode>
  static void CalculateEffectiveAddress(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static VirtualMemoryAddress CalculateJumpTarget(CPU* cpu);

  // Instruction handlers
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_ADD(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_ADC(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_SUB(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_SBB(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_CMP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_AND(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_OR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_XOR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_TEST(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_MOV(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_MOVZX(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_MOVSX(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_MOV_Sreg(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_MOV_CR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_MOV_DR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_MOV_TR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_XCHG(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_SHL(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_SHR(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_SAR(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_RCL(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_RCR(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_ROL(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_ROR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_IN(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_OUT(CPU* cpu);

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_INC(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_DEC(CPU* cpu);

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_NOT(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_NEG(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_MUL(CPU* cpu);
  template<OperandSize op1_size, OperandMode op1_mode, u32 op1_constant, OperandSize op2_size = OperandSize_Count,
           OperandMode op2_mode = OperandMode_None, u32 op2_constant = 0, OperandSize op3_size = OperandSize_Count,
           OperandMode op3_mode = OperandMode_None, u32 op3_constant = 0>
  static inline void Execute_Operation_IMUL(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_DIV(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_IDIV(CPU* cpu);

  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_PUSH(CPU* cpu);
  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_PUSH_Sreg(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_POP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_POP_Sreg(CPU* cpu);

  static inline void Execute_Operation_PUSHA(CPU* cpu);
  static inline void Execute_Operation_POPA(CPU* cpu);

  template<OperandSize frame_size, OperandMode frame_mode, u32 frame_constant, OperandSize level_size,
           OperandMode level_mode, u32 level_constant>
  static inline void Execute_Operation_ENTER(CPU* cpu);
  static inline void Execute_Operation_LEAVE(CPU* cpu);

  template<OperandSize sreg_size, OperandMode sreg_mode, u32 sreg_constant, OperandSize reg_size, OperandMode reg_mode,
           u32 reg_constant, OperandSize ptr_size, OperandMode ptr_mode, u32 ptr_constant>
  static inline void Execute_Operation_LXS(CPU* cpu);

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_LEA(CPU* cpu);

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_JMP_Near(CPU* cpu);

  template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_Jcc(CPU* cpu);
  template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_LOOP(CPU* cpu);
  template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_SETcc(CPU* cpu);
  template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size,
           OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_CMOVcc(CPU* cpu);

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_CALL_Near(CPU* cpu);
  template<OperandSize dst_size = OperandSize_Count, OperandMode dst_mode = OperandMode_None, u32 dst_constant = 0>
  static inline void Execute_Operation_RET_Near(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_JMP_Far(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_CALL_Far(CPU* cpu);
  template<OperandSize dst_size = OperandSize_Count, OperandMode dst_mode = OperandMode_None, u32 dst_constant = 0>
  static inline void Execute_Operation_RET_Far(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_INT(CPU* cpu);
  static inline void Execute_Operation_INT3(CPU* cpu);
  static inline void Execute_Operation_INTO(CPU* cpu);
  static inline void Execute_Operation_IRET(CPU* cpu);

  // String operations
  template<Operation operation, bool check_equal, typename callback>
  static inline void Execute_REP(CPU* cpu, callback cb);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_MOVS(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_CMPS(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_STOS(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_LODS(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_SCAS(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_INS(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_OUTS(CPU* cpu);

  template<Operation operation, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size,
           OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_BTx(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_BT(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_BTS(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_BTR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_BTC(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_BSF(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_BSR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_CMPXCHG(CPU* cpu);
  template<OperandSize mem_size, OperandMode mem_mode, u32 mem_constant>
  static inline void Execute_Operation_CMPXCHG8B(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_XADD(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant, OperandSize count_size, OperandMode count_mode, u32 count_constant>
  static inline void Execute_Operation_SHLD(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant, OperandSize count_size, OperandMode count_mode, u32 count_constant>
  static inline void Execute_Operation_SHRD(CPU* cpu);

  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_LMSW(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_SMSW(CPU* cpu);

  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_LIDT(CPU* cpu);
  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_LGDT(CPU* cpu);
  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_LLDT(CPU* cpu);
  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_LTR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_SIDT(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_SGDT(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_SLDT(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_STR(CPU* cpu);
  static inline void Execute_Operation_CLTS(CPU* cpu);

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize selector_size,
           OperandMode selector_mode, u32 selector_constant>
  static inline void Execute_Operation_LAR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize selector_size,
           OperandMode selector_mode, u32 selector_constant>
  static inline void Execute_Operation_LSL(CPU* cpu);
  template<Operation operation, OperandSize selector_size, OperandMode selector_mode, u32 selector_constant>
  static inline void Execute_Operation_VERx(CPU* cpu);
  template<OperandSize selector_size, OperandMode selector_mode, u32 selector_constant>
  static inline void Execute_Operation_VERR(CPU* cpu);
  template<OperandSize selector_size, OperandMode selector_mode, u32 selector_constant>
  static inline void Execute_Operation_VERW(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_ARPL(CPU* cpu);
  template<OperandSize addr_size, OperandMode addr_mode, u32 addr_constant, OperandSize table_size,
           OperandMode table_mode, u32 table_constant>
  static inline void Execute_Operation_BOUND(CPU* cpu);

  template<OperandSize addr_size, OperandMode addr_mode, u32 addr_constant>
  static inline void Execute_Operation_INVLPG(CPU* cpu);
  static inline void Execute_Operation_INVD(CPU* cpu);
  static inline void Execute_Operation_WBINVD(CPU* cpu);

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_BSWAP(CPU* cpu);

  // Inline due to ODR
  static inline void Execute_Operation_NOP(CPU* cpu);
  static inline void Execute_Operation_CLC(CPU* cpu);
  static inline void Execute_Operation_CLD(CPU* cpu);
  static inline void Execute_Operation_CLI(CPU* cpu);
  static inline void Execute_Operation_CMC(CPU* cpu);
  static inline void Execute_Operation_STC(CPU* cpu);
  static inline void Execute_Operation_STD(CPU* cpu);
  static inline void Execute_Operation_STI(CPU* cpu);
  static inline void Execute_Operation_SALC(CPU* cpu);
  static inline void Execute_Operation_LAHF(CPU* cpu);
  static inline void Execute_Operation_SAHF(CPU* cpu);
  static inline void Execute_Operation_PUSHF(CPU* cpu);
  static inline void Execute_Operation_POPF(CPU* cpu);
  static inline void Execute_Operation_HLT(CPU* cpu);
  static inline void Execute_Operation_CBW(CPU* cpu);
  static inline void Execute_Operation_CWD(CPU* cpu);
  static inline void Execute_Operation_XLAT(CPU* cpu);
  static inline void Execute_Operation_WAIT(CPU* cpu);
  static inline void Execute_Operation_CPUID(CPU* cpu);
  static inline void Execute_Operation_RDTSC(CPU* cpu);
  static inline void Execute_Operation_RDMSR(CPU* cpu);
  static inline void Execute_Operation_WRMSR(CPU* cpu);
  static inline void Execute_Operation_RSM(CPU* cpu);

  static inline void Execute_Operation_AAA(CPU* cpu);
  static inline void Execute_Operation_AAS(CPU* cpu);
  template<OperandSize op_size, OperandMode op_mode, u32 op_constant>
  static inline void Execute_Operation_AAM(CPU* cpu);
  template<OperandSize op_size, OperandMode op_mode, u32 op_constant>
  static inline void Execute_Operation_AAD(CPU* cpu);
  static inline void Execute_Operation_DAA(CPU* cpu);
  static inline void Execute_Operation_DAS(CPU* cpu);

  // Reads operands based on operand mode
  // Use only when you have already confirmed the operand size
  template<OperandMode mode, u32 constant>
  static inline u8 ReadByteOperand(CPU* cpu);
  template<OperandMode mode, u32 constant>
  static inline u16 ReadWordOperand(CPU* cpu);
  template<OperandMode mode, u32 constant>
  static inline u32 ReadDWordOperand(CPU* cpu);
  template<OperandMode mode, u32 constant>
  static inline u64 ReadQWordOperand(CPU* cpu);
  // Zero-extending/sign-extending, uee when size is unconfirmed
  template<OperandSize size, OperandMode mode, u32 constant>
  static inline u16 ReadZeroExtendedWordOperand(CPU* cpu);
  template<OperandSize size, OperandMode mode, u32 constant>
  static inline u32 ReadZeroExtendedDWordOperand(CPU* cpu);
  template<OperandSize size, OperandMode mode, u32 constant>
  static inline u16 ReadSignExtendedWordOperand(CPU* cpu);
  template<OperandSize size, OperandMode mode, u32 constant>
  static inline u32 ReadSignExtendedDWordOperand(CPU* cpu);
  template<OperandMode mode, u32 constant>
  static inline void WriteByteOperand(CPU* cpu, u8 value);
  template<OperandMode mode, u32 constant>
  static inline void WriteWordOperand(CPU* cpu, u16 value);
  template<OperandMode mode, u32 constant>
  static inline void WriteDWordOperand(CPU* cpu, u32 value);
  template<OperandMode mode, u32 constant>
  static inline void WriteQWordOperand(CPU* cpu, u64 value);

  template<OperandMode mode>
  static inline void ReadFarAddressOperand(CPU* cpu, OperandSize size, u16* segment_selector,
                                           VirtualMemoryAddress* address);

  template<JumpCondition condition>
  static inline bool TestJumpCondition(CPU* cpu);

  // x87/FPU
  static inline void StartX87Instruction(CPU* cpu, bool check_exceptions = true);
  template<OperandSize size, OperandMode mode, u32 constant>
  static inline floatx80 ReadFloatOperand(CPU* cpu, float_status_t& fs);
  template<OperandSize size, OperandMode mode, u32 constant>
  static inline floatx80 ReadIntegerOperandAsFloat(CPU* cpu, float_status_t& fs);
  template<OperandSize size, OperandMode mode, u32 constant>
  static inline void WriteFloatOperand(CPU* cpu, float_status_t& fs, const floatx80& value);
  static inline void CheckFloatStackOverflow(CPU* cpu);
  static inline void CheckFloatStackUnderflow(CPU* cpu, u8 relative_index);
  static inline void PushFloatStack(CPU* cpu);
  static inline void PopFloatStack(CPU* cpu);
  static inline floatx80 ReadFloatRegister(CPU* cpu, u8 relative_index);
  static inline void WriteFloatRegister(CPU* cpu, u8 relative_index, const floatx80& value, bool update_tag = true);
  static inline void UpdateFloatTagRegister(CPU* cpu, u8 index);
  static inline float_status_t GetFloatStatus(CPU* cpu);
  static inline void RaiseFloatExceptions(CPU* cpu, const float_status_t& fs);
  static inline void SetStatusWordFromCompare(CPU* cpu, const float_status_t& fs, int res);
  static inline void ClearC1(CPU* cpu);

  static inline void Execute_Operation_F2XM1(CPU* cpu);
  static inline void Execute_Operation_FABS(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FADD(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FADDP(CPU* cpu);
  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_FBLD(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_FBSTP(CPU* cpu);
  static inline void Execute_Operation_FCHS(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant, bool quiet = false>
  static inline void Execute_Operation_FCOM(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant, bool quiet = false>
  static inline void Execute_Operation_FCOMP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant, bool quiet = false>
  static inline void Execute_Operation_FCOMPP(CPU* cpu);
  static inline void Execute_Operation_FDECSTP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FDIV(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FDIVP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FDIVR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FDIVRP(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_FFREE(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FIADD(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant, bool quiet = false>
  static inline void Execute_Operation_FICOM(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FICOMP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FIDIV(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FIDIVR(CPU* cpu);
  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_FILD(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FIMUL(CPU* cpu);
  static inline void Execute_Operation_FINCSTP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_FIST(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_FISTP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FISUB(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FISUBR(CPU* cpu);
  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_FLD(CPU* cpu);
  static inline void Execute_Operation_FLD1(CPU* cpu);
  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_FLDCW(CPU* cpu);
  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_FLDENV(CPU* cpu);
  static inline void Execute_Operation_FLDL2E(CPU* cpu);
  static inline void Execute_Operation_FLDL2T(CPU* cpu);
  static inline void Execute_Operation_FLDLG2(CPU* cpu);
  static inline void Execute_Operation_FLDLN2(CPU* cpu);
  static inline void Execute_Operation_FLDPI(CPU* cpu);
  static inline void Execute_Operation_FLDZ(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FMUL(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FMULP(CPU* cpu);
  static inline void Execute_Operation_FNCLEX(CPU* cpu);
  static inline void Execute_Operation_FNDISI(CPU* cpu);
  static inline void Execute_Operation_FNENI(CPU* cpu);
  static inline void Execute_Operation_FNINIT(CPU* cpu);
  static inline void Execute_Operation_FNOP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_FNSAVE(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_FNSTCW(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_FNSTENV(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_FNSTSW(CPU* cpu);
  static inline void Execute_Operation_FPATAN(CPU* cpu);
  template<bool ieee754 = false>
  static inline void Execute_Operation_FPREM(CPU* cpu);
  static inline void Execute_Operation_FPTAN(CPU* cpu);
  static inline void Execute_Operation_FRNDINT(CPU* cpu);
  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_FRSTOR(CPU* cpu);
  static inline void Execute_Operation_FSCALE(CPU* cpu);
  static inline void Execute_Operation_FSQRT(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_FST(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_FSTP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FSUB(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FSUBP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FSUBR(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FSUBRP(CPU* cpu);
  static inline void Execute_Operation_FTST(CPU* cpu);
  static inline void Execute_Operation_FXAM(CPU* cpu);
  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_FXCH(CPU* cpu);
  static inline void Execute_Operation_FXTRACT(CPU* cpu);
  static inline void Execute_Operation_FYL2X(CPU* cpu);
  static inline void Execute_Operation_FYL2XP1(CPU* cpu);
  static inline void Execute_Operation_FSETPM(CPU* cpu);
  static inline void Execute_Operation_FCOS(CPU* cpu);
  static inline void Execute_Operation_FPREM1(CPU* cpu);
  static inline void Execute_Operation_FSIN(CPU* cpu);
  static inline void Execute_Operation_FSINCOS(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FUCOM(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FUCOMP(CPU* cpu);
  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_FUCOMPP(CPU* cpu);

  /// Instruction handler key - used to get a pointer to an opcode handler
  union HandlerFunctionKey
  {
    u64 bits;
    struct
    {
      u16 operation;
      struct
      {
        u16 size : 3;
        u16 mode : 5;
        u16 data : 5;
        u16 pad : 3;
      } operands[3];
    };

    static constexpr u64 Build(Operation operation, OperandSize opsize_1 = OperandSize_8,
                               OperandMode opmode_1 = OperandMode_None, u32 opdata_1 = 0,
                               OperandSize opsize_2 = OperandSize_8, OperandMode opmode_2 = OperandMode_None,
                               u32 opdata_2 = 0, OperandSize opsize_3 = OperandSize_8,
                               OperandMode opmode_3 = OperandMode_None, u32 opdata_3 = 0)
    {
      HandlerFunctionKey k = {};
      k.operation = static_cast<u16>(operation);
      k.operands[0].size = static_cast<u16>(opsize_1);
      k.operands[0].mode = static_cast<u16>(opmode_1);
      k.operands[0].data = static_cast<u16>(opdata_1);
      k.operands[1].size = static_cast<u16>(opsize_2);
      k.operands[1].mode = static_cast<u16>(opmode_2);
      k.operands[1].data = static_cast<u16>(opdata_2);
      k.operands[2].size = static_cast<u16>(opsize_3);
      k.operands[2].mode = static_cast<u16>(opmode_3);
      k.operands[2].data = static_cast<u16>(opdata_3);
      return k.bits;
    }
  };
  static_assert(sizeof(HandlerFunctionKey) == 8, "InstructionHandlerKey is qword-sized");
  using HandlerFunctionMap = std::map<u64, HandlerFunction>;
  static HandlerFunctionMap s_handler_functions;
};
} // namespace CPU_X86