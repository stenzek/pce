#pragma once
#include "pce/cpu_x86/cpu.h"
#include "pce/cpu_x86/decode.h"

extern "C" {
#include <softfloat.h>
}

namespace CPU_X86 {

class Interpreter
{
public:
  static void ExecuteInstruction(CPU* cpu, const OldInstruction* instruction);

private:
  static void DispatchInstruction(CPU* cpu, const OldInstruction* instruction);

  static VirtualMemoryAddress CalculateEffectiveAddress(CPU* cpu, const OldInstruction* instruction,
                                                        const OldInstruction::Operand* operand);
  static uint8 ReadByteOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand);
  static uint16 ReadWordOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand);
  static uint16 ReadSignExtendedWordOperand(CPU* cpu, const OldInstruction* instruction,
                                            const OldInstruction::Operand* operand);
  static uint32 ReadDWordOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand);
  static uint32 ReadSignExtendedDWordOperand(CPU* cpu, const OldInstruction* instruction,
                                             const OldInstruction::Operand* operand);
  static void ReadFarAddressOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand,
                                    uint16* segment_selector, VirtualMemoryAddress* address);
  static void WriteByteOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand,
                               uint8 value);
  static void WriteWordOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand,
                               uint16 value);
  static void WriteDWordOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand,
                                uint32 value);

  static bool TestCondition(CPU* cpu, JumpCondition condition, AddressSize address_size);

  static void Execute_NOP(CPU* cpu, const OldInstruction* instruction);
  static void Execute_MOV(CPU* cpu, const OldInstruction* instruction);
  static void Execute_MOVS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LODS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_STOS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_MOV_Sreg(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LDS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LES(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LSS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LFS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LGS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LEA(CPU* cpu, const OldInstruction* instruction);
  static void Execute_XCHG(CPU* cpu, const OldInstruction* instruction);
  static void Execute_XLAT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CBW(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CWD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_PUSH(CPU* cpu, const OldInstruction* instruction);
  static void Execute_PUSHA(CPU* cpu, const OldInstruction* instruction);
  static void Execute_PUSHF(CPU* cpu, const OldInstruction* instruction);
  static void Execute_POP(CPU* cpu, const OldInstruction* instruction);
  static void Execute_POPA(CPU* cpu, const OldInstruction* instruction);
  static void Execute_POPF(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LAHF(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SAHF(CPU* cpu, const OldInstruction* instruction);
  static void Execute_Jcc(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LOOP(CPU* cpu, const OldInstruction* instruction);
  static void Execute_JMP_Near(CPU* cpu, const OldInstruction* instruction);
  static void Execute_JMP_Far(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CALL_Near(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CALL_Far(CPU* cpu, const OldInstruction* instruction);
  static void Execute_RET_Near(CPU* cpu, const OldInstruction* instruction);
  static void Execute_RET_Far(CPU* cpu, const OldInstruction* instruction);
  static void Execute_INT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_INTO(CPU* cpu, const OldInstruction* instruction);
  static void Execute_IRET(CPU* cpu, const OldInstruction* instruction);
  static void Execute_HLT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_IN(CPU* cpu, const OldInstruction* instruction);
  static void Execute_INS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_OUT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_OUTS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CLC(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CLD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CLI(CPU* cpu, const OldInstruction* instruction);
  static void Execute_STC(CPU* cpu, const OldInstruction* instruction);
  static void Execute_STD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_STI(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CMC(CPU* cpu, const OldInstruction* instruction);
  static void Execute_INC(CPU* cpu, const OldInstruction* instruction);
  static void Execute_ADD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_ADC(CPU* cpu, const OldInstruction* instruction);
  static void Execute_DEC(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SUB(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SBB(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CMP(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CMPS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SCAS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_MUL(CPU* cpu, const OldInstruction* instruction);
  static void Execute_IMUL(CPU* cpu, const OldInstruction* instruction);
  static void Execute_DIV(CPU* cpu, const OldInstruction* instruction);
  static void Execute_IDIV(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SHL(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SHR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SAR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_RCL(CPU* cpu, const OldInstruction* instruction);
  static void Execute_RCR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_ROL(CPU* cpu, const OldInstruction* instruction);
  static void Execute_ROR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_AND(CPU* cpu, const OldInstruction* instruction);
  static void Execute_OR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_XOR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_TEST(CPU* cpu, const OldInstruction* instruction);
  static void Execute_NEG(CPU* cpu, const OldInstruction* instruction);
  static void Execute_NOT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_AAA(CPU* cpu, const OldInstruction* instruction);
  static void Execute_AAS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_AAM(CPU* cpu, const OldInstruction* instruction);
  static void Execute_AAD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_DAA(CPU* cpu, const OldInstruction* instruction);
  static void Execute_DAS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_BTx(CPU* cpu, const OldInstruction* instruction);
  static void Execute_BSF(CPU* cpu, const OldInstruction* instruction);
  static void Execute_BSR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SHLD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SHRD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LGDT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LIDT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SGDT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SIDT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LMSW(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SMSW(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LLDT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SLDT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LTR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_STR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CLTS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LAR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LSL(CPU* cpu, const OldInstruction* instruction);
  static void Execute_ARPL(CPU* cpu, const OldInstruction* instruction);
  static void Execute_VERx(CPU* cpu, const OldInstruction* instruction);
  static void Execute_BOUND(CPU* cpu, const OldInstruction* instruction);
  static void Execute_WAIT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_ENTER(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LEAVE(CPU* cpu, const OldInstruction* instruction);
  static void Execute_LOADALL_286(CPU* cpu, const OldInstruction* instruction);
  static void Execute_MOVSX(CPU* cpu, const OldInstruction* instruction);
  static void Execute_MOVZX(CPU* cpu, const OldInstruction* instruction);
  static void Execute_MOV_CR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_MOV_DR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_MOV_TR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_SETcc(CPU* cpu, const OldInstruction* instruction);
  static void Execute_BSWAP(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CMPXCHG(CPU* cpu, const OldInstruction* instruction);
  static void Execute_CMOVcc(CPU* cpu, const OldInstruction* instruction);
  static void Execute_WBINVD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_INVLPG(CPU* cpu, const OldInstruction* instruction);
  static void Execute_XADD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_RDTSC(CPU* cpu, const OldInstruction* instruction);

  // x87/FPU
  static void StartX87Instruction(CPU* cpu, const OldInstruction* instruction);
  static floatx80 ReadFloatOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand,
                                   float_status* fs);
  static floatx80 ReadIntegerOperandAsFloat(CPU* cpu, const OldInstruction* instruction,
                                            const OldInstruction::Operand* operand, float_status* fs);
  static void WriteFloatOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand,
                                float_status* fs, const floatx80& value);
  static void CheckFloatStackOverflow(CPU* cpu);
  static void CheckFloatStackUnderflow(CPU* cpu, uint8 relative_index);
  static void PushFloatStack(CPU* cpu);
  static void PopFloatStack(CPU* cpu);
  static floatx80 ReadFloatRegister(CPU* cpu, uint8 relative_index);
  static void WriteFloatRegister(CPU* cpu, uint8 relative_index, const floatx80& value, bool update_tag = true);
  static void UpdateFloatTagRegister(CPU* cpu, uint8 index);
  static float_status GetFloatStatus(CPU* cpu);
  static void RaiseFloatExceptions(CPU* cpu, const float_status& fs);

  static void Execute_FNINIT(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FSETPM(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FNSTCW(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FNSTSW(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FNCLEX(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FLDCW(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FLD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FLD1(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FLDZ(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FST(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FADD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FSUB(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FSUBR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FMUL(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FDIV(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FDIVR(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FCOM(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FILD(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FIST(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FIDIV(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FABS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FCHS(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FFREE(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FPREM(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FXAM(CPU* cpu, const OldInstruction* instruction);
  static void Execute_FXCH(CPU* cpu, const OldInstruction* instruction);
};
} // namespace CPU_X86