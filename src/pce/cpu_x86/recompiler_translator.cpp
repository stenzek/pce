#include "pce/cpu_x86/recompiler_translator.h"
#include "YBaseLib/Log.h"
#include "pce/cpu_x86/debugger_interface.h"
#include "pce/system.h"
Log_SetChannel(CPU_X86::Recompiler);

namespace CPU_X86 {

// TODO:
// Constant operands - don't move to a temporary register first
// Only sync current_ESP at the start of the block, and on push/pop instructions
// Threaded code generator?
// Push functions per stack address mode
// Only sync EIP on potentially-faulting instructions
// Lazy flag calculation - store operands and opcode

RecompilerTranslator::RecompilerTranslator(RecompilerBackend* backend, RecompilerBackend::Block* block,
                                           llvm::Module* module, llvm::Function* function)
  : m_backend(backend), m_block(block), m_module(module), m_function(function),
    m_basic_block(llvm::BasicBlock::Create(backend->GetLLVMContext(), "entry", function)), m_builder(m_basic_block)
{
}

RecompilerTranslator::~RecompilerTranslator() {}

bool RecompilerTranslator::TranslateBlock()
{
  for (const Instruction& inst : m_block->instructions)
  {
    if (!CompileInstruction(&inst))
      return false;
  }

  SyncInstructionPointers();

  // Add the final "ret".
  // TODO: This is where we would do block linking.
  m_builder.CreateRetVoid();
  return true;
}

bool RecompilerTranslator::CompileInstruction(const Instruction* instruction)
{
  bool result;
  switch (instruction->operation)
  {
    case Operation_NOP:
      result = Compile_NOP(instruction);
      break;
      //     case Operation_LEA:
      //       result = Compile_LEA(instruction);
      //       break;
      //     case Operation_MOV:
      //       result = Compile_MOV(instruction);
      //       break;
      //     case Operation_MOVSX:
      //     case Operation_MOVZX:
      //       result = Compile_MOV_Extended(instruction);
      //       break;
      //     case Operation_ADD:
      //     case Operation_SUB:
      //     case Operation_AND:
      //     case Operation_OR:
      //     case Operation_XOR:
      //       result = Compile_ALU_Binary_Update(instruction);
      //       break;
      //     case Operation_CMP:
      //     case Operation_TEST:
      //       result = Compile_ALU_Binary_Test(instruction);
      //       break;
      //     case Operation_INC:
      //     case Operation_DEC:
      //     case Operation_NEG:
      //     case Operation_NOT:
      //       result = Compile_ALU_Unary_Update(instruction);
      //       break;
      //     case Operation_SHL:
      //     case Operation_SHR:
      //     case Operation_SAR:
      //       result = Compile_ShiftRotate(instruction);
      //       break;
      //     case Operation_SHLD:
      //     case Operation_SHRD:
      //       result = Compile_DoublePrecisionShift(instruction);
      //       break;
      //     case Operation_Jcc:
      //     case Operation_LOOP:
      //       result = Compile_JumpConditional(instruction);
      //       break;
      //     case Operation_PUSH:
      //     case Operation_POP:
      //     case Operation_PUSHF:
      //     case Operation_POPF:
      //       result = Compile_Stack(instruction);
      //       break;
      //     case Operation_CALL_Far:
      //     case Operation_RET_Far:
      //     case Operation_CALL_Near:
      //     case Operation_RET_Near:
      //     case Operation_JMP_Near:
      //     case Operation_JMP_Far:
      //       result = Compile_JumpCallReturn(instruction);
      //       break;
      //     case Operation_CLC:
      //     case Operation_CLD:
      //     case Operation_STC:
      //     case Operation_STD:
      //       result = Compile_Flags(instruction);
      //       break;
    default:
      result = Compile_Fallback(instruction);
      break;
  }

  return result;
}

llvm::Function* RecompilerTranslator::GetInterpretInstructionFunction()
{
  // m_module->getOrInsertFunction()
  return nullptr;
}

bool RecompilerTranslator::OperandIsESP(const Instruction::Operand& operand)
{
  // If any instructions manipulate ESP, we need to update the shadow variable for the next instruction.
  return operand.size > OperandSize_8 && operand.mode == AddressingMode_Register && operand.reg32 == Reg32_ESP;
}

bool RecompilerTranslator::CanInstructionFault(const Instruction* instruction)
{
  switch (instruction->operation)
  {
    case Operation_AAA:
    case Operation_AAD:
    case Operation_AAM:
    case Operation_AAS:
    case Operation_CLD:
    case Operation_CLC:
    case Operation_STC:
    case Operation_STD:
      return false;

    case Operation_ADD:
    case Operation_ADC:
    case Operation_SUB:
    case Operation_SBB:
    case Operation_AND:
    case Operation_XOR:
    case Operation_OR:
    case Operation_CMP:
    case Operation_TEST:
    case Operation_MOV:
    {
      for (uint32 i = 0; i < 2; i++)
      {
        if (instruction->operands[i].mode != AddressingMode_Register &&
            instruction->operands[i].mode != AddressingMode_Immediate)
        {
          return true;
        }
      }
      return false;
    }

    case Operation_INC:
    case Operation_DEC:
    case Operation_NEG:
    case Operation_NOT:
    {
      return (instruction->operands[0].mode != AddressingMode_Register &&
              instruction->operands[0].mode != AddressingMode_Immediate);
    }

    default:
      return true;
  }
}

uint32 RecompilerTranslator::CalculateRegisterOffset(Reg8 reg)
{
  // Ugly but necessary due to the structure layout.
  switch (reg)
  {
    case Reg8_AL:
      return offsetof(CPU, m_registers.AL);
    case Reg8_CL:
      return offsetof(CPU, m_registers.CL);
    case Reg8_DL:
      return offsetof(CPU, m_registers.DL);
    case Reg8_BL:
      return offsetof(CPU, m_registers.BL);
    case Reg8_AH:
      return offsetof(CPU, m_registers.AH);
    case Reg8_CH:
      return offsetof(CPU, m_registers.CH);
    case Reg8_DH:
      return offsetof(CPU, m_registers.DH);
    case Reg8_BH:
      return offsetof(CPU, m_registers.BH);
    default:
      DebugUnreachableCode();
      return 0;
  }
}

uint32 RecompilerTranslator::CalculateRegisterOffset(Reg16 reg)
{
  // Ugly but necessary due to the structure layout.
  switch (reg)
  {
    case Reg16_AX:
      return offsetof(CPU, m_registers.AX);
    case Reg16_CX:
      return offsetof(CPU, m_registers.CX);
    case Reg16_DX:
      return offsetof(CPU, m_registers.DX);
    case Reg16_BX:
      return offsetof(CPU, m_registers.BX);
    case Reg16_SP:
      return offsetof(CPU, m_registers.SP);
    case Reg16_BP:
      return offsetof(CPU, m_registers.BP);
    case Reg16_SI:
      return offsetof(CPU, m_registers.SI);
    case Reg16_DI:
      return offsetof(CPU, m_registers.DI);
    default:
      DebugUnreachableCode();
      return 0;
  }
}

uint32 RecompilerTranslator::CalculateRegisterOffset(Reg32 reg)
{
  return uint32(offsetof(CPU, m_registers.reg32[0]) + (reg * sizeof(uint32)));
}

uint32 RecompilerTranslator::CalculateSegmentRegisterOffset(Segment segment)
{
  return uint32(offsetof(CPU, m_registers.segment_selectors[0]) + (segment * sizeof(uint16)));
}

llvm::Value* RecompilerTranslator::ReadRegister(Reg8 reg)
{
  return nullptr;
}

llvm::Value* RecompilerTranslator::ReadRegister(Reg16 reg)
{
  return nullptr;
}

llvm::Value* RecompilerTranslator::ReadRegister(Reg32 reg)
{
  return nullptr;
}

void RecompilerTranslator::WriteRegister(Reg8 reg, llvm::Value* value) {}

void RecompilerTranslator::WriteRegister(Reg16 reg, llvm::Value* value) {}

void RecompilerTranslator::WriteRegister(Reg32 reg, llvm::Value* value) {}

// llvm::Value* RecompilerTranslator::CalculateEffectiveAddress(const Instruction* instruction)
// {
//   for (size_t i = 0; i < countof(instruction->operands); i++)
//   {
//     const Instruction::Operand* operand = &instruction->operands[i];
//     switch (operand->mode)
//     {
//       case AddressingMode_RegisterIndirect:
//       {
//         if (instruction->address_size == AddressSize_16)
//           movzx(READDR32, word[RCPUPTR + CalculateRegisterOffset(operand->reg.reg16)]);
//         else
//           mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->reg.reg32)]);
//       }
//       break;
//       case AddressingMode_Indexed:
//       {
//         if (instruction->address_size == AddressSize_16)
//         {
//           mov(READDR16, word[RCPUPTR + CalculateRegisterOffset(operand->indexed.reg.reg16)]);
//           if (operand->indexed.displacement != 0)
//             add(READDR16, uint32(operand->indexed.displacement));
//           movzx(READDR32, READDR16);
//         }
//         else
//         {
//           mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->indexed.reg.reg32)]);
//           if (operand->indexed.displacement != 0)
//             add(READDR32, uint32(operand->indexed.displacement));
//         }
//       }
//       break;
//       case AddressingMode_BasedIndexed:
//       {
//         if (instruction->address_size == AddressSize_16)
//         {
//           mov(READDR16, word[RCPUPTR + CalculateRegisterOffset(operand->based_indexed.base.reg16)]);
//           add(READDR16, word[RCPUPTR + CalculateRegisterOffset(operand->based_indexed.index.reg16)]);
//           movzx(READDR32, READDR16);
//         }
//         else
//         {
//           mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->based_indexed.base.reg32)]);
//           add(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->based_indexed.index.reg32)]);
//         }
//       }
//       break;
//       case AddressingMode_BasedIndexedDisplacement:
//       {
//         if (instruction->address_size == AddressSize_16)
//         {
//           mov(READDR16, word[RCPUPTR + CalculateRegisterOffset(operand->based_indexed_displacement.base.reg16)]);
//           add(READDR16, word[RCPUPTR + CalculateRegisterOffset(operand->based_indexed_displacement.index.reg16)]);
//           if (operand->based_indexed_displacement.displacement != 0)
//             add(READDR16, uint32(operand->based_indexed_displacement.displacement));
//           movzx(READDR32, READDR16);
//         }
//         else
//         {
//           mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->based_indexed_displacement.base.reg32)]);
//           add(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->based_indexed_displacement.index.reg32)]);
//           if (operand->based_indexed_displacement.displacement != 0)
//             add(READDR32, uint32(operand->based_indexed_displacement.displacement));
//         }
//       }
//       break;
//       case AddressingMode_SIB:
//       {
//         Assert(instruction->address_size == AddressSize_32);
//         if (operand->sib.index.reg32 != Reg32_Count)
//         {
//           // This one is implemented in reverse, but should evaluate to the same results. This way we don't need a
//           // temporary.
//           mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->sib.index.reg32)]);
//           shl(READDR32, operand->sib.scale_shift);
//           if (operand->sib.base.reg32 != Reg32_Count)
//             add(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->sib.base.reg32)]);
//           if (operand->sib.displacement != 0)
//             add(READDR32, uint32(operand->sib.displacement));
//         }
//         else if (operand->sib.base.reg32 != Reg32_Count)
//         {
//           // No index.
//           mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->sib.base.reg32)]);
//           if (operand->sib.displacement != 0)
//             add(READDR32, uint32(operand->sib.displacement));
//         }
//         else
//         {
//           // No base.
//           if (operand->sib.displacement == 0)
//             xor(READDR32, READDR32);
//           else
//             mov(READDR32, uint32(operand->sib.displacement));
//         }
//       }
//       break;
//     }
//   }
// }

bool RecompilerTranslator::IsConstantOperand(const Instruction* instruction, size_t index)
{
  const Instruction::Operand* operand = &instruction->operands[index];
  return (operand->mode == AddressingMode_Immediate);
}

llvm::Constant* RecompilerTranslator::GetConstantOperand(const Instruction* instruction, size_t index, bool sign_extend)
{
  const Instruction::Operand* operand = &instruction->operands[index];
  DebugAssert(operand->mode == AddressingMode_Immediate);

  switch (operand->size)
  {
    case OperandSize_8:
      return m_builder.getInt32(sign_extend ? SignExtend32(Truncate8(operand->constant)) :
                                              ZeroExtend32(Truncate8(operand->constant)));
    case OperandSize_16:
      return m_builder.getInt32(sign_extend ? SignExtend32(Truncate16(operand->constant)) :
                                              ZeroExtend32(Truncate16(operand->constant)));
    default:
      return m_builder.getInt32(operand->constant);
  }
}

// llvm::Value* RecompilerTranslator::ReadOperand(const Instruction* instruction, size_t index, OperandSize size, bool
// sign_extend)
// {
//   const Instruction::Operand* operand = &instruction->operands[index];
//   switch (operand->mode)
//   {
//     case AddressingMode_Immediate:
//     {
//       switch (size)
//       {
//         case OperandSize_8:
//           return llvm::ConstantInt::get(m_backend->GetLLVMUInt8Type(), ZeroExtend64(Truncate8(operand->constant)));
//
//         case OperandSize_16:
//         {
//           switch (operand->size)
//           {
//             case OperandSize_8:
//               return llvm::ConstantInt::get(m_backend->GetLLVMUInt16Type(), ZeroExtend64(sign_extend ?
//               SignExtend16(Truncate8(operand->constant)) : ZeroExtend16(Truncate8(operand->constant))));
//             default:
//               return llvm::ConstantInt::get(m_backend->GetLLVMUInt16Type(), ZeroExtend64(sign_extend ?
//               SignExtend16(Truncate8(operand->constant)) : ZeroExtend16(operand->constant))); mov(dest,
//               ZeroExtend32(operand->immediate.value16)); break;
//           }
//         }
//         break;
//         case OperandSize_32:
//         {
//           switch (operand->size)
//           {
//             case OperandSize_8:
//               mov(dest,
//                   sign_extend ? SignExtend32(operand->immediate.value8) : ZeroExtend32(operand->immediate.value8));
//               break;
//             case OperandSize_16:
//               mov(dest,
//                   sign_extend ? SignExtend32(operand->immediate.value16) : ZeroExtend32(operand->immediate.value16));
//               break;
//             default:
//               mov(dest, ZeroExtend32(operand->immediate.value32));
//               break;
//           }
//         }
//         break;
//       }
//     }
//     break;
//
//     case AddressingMode_Register:
//     {
//       switch (output_size)
//       {
//         case OperandSize_8:
//           mov(dest, byte[RCPUPTR + CalculateRegisterOffset(operand->reg.reg8)]);
//           break;
//
//         case OperandSize_16:
//         {
//           switch (operand->size)
//           {
//             case OperandSize_8:
//             {
//               if (sign_extend)
//                 movsx(dest, byte[RCPUPTR + CalculateRegisterOffset(operand->reg.reg8)]);
//               else
//                 movzx(dest, byte[RCPUPTR + CalculateRegisterOffset(operand->reg.reg8)]);
//             }
//             break;
//             case OperandSize_16:
//             case OperandSize_32:
//               mov(dest, word[RCPUPTR + CalculateRegisterOffset(operand->reg.reg16)]);
//               break;
//           }
//         }
//         break;
//
//         case OperandSize_32:
//         {
//           switch (operand->size)
//           {
//             case OperandSize_8:
//             {
//               if (sign_extend)
//                 movsx(dest, byte[RCPUPTR + CalculateRegisterOffset(operand->reg.reg8)]);
//               else
//                 movzx(dest, byte[RCPUPTR + CalculateRegisterOffset(operand->reg.reg8)]);
//             }
//             break;
//             case OperandSize_16:
//             {
//               if (sign_extend)
//                 movsx(dest, word[RCPUPTR + CalculateRegisterOffset(operand->reg.reg16)]);
//               else
//                 movzx(dest, word[RCPUPTR + CalculateRegisterOffset(operand->reg.reg16)]);
//             }
//             break;
//             case OperandSize_32:
//               mov(dest, dword[RCPUPTR + CalculateRegisterOffset(operand->reg.reg32)]);
//               break;
//           }
//         }
//         break;
//       }
//     }
//     break;
//
//     case AddressingMode_SegmentRegister:
//     {
//       switch (output_size)
//       {
//         case OperandSize_16:
//           mov(dest, word[RCPUPTR + CalculateSegmentRegisterOffset(operand->reg.sreg)]);
//           break;
//         case OperandSize_32:
//           // Segment registers are sign-extended on push/pop.
//           movsx(dest, word[RCPUPTR + CalculateSegmentRegisterOffset(operand->reg.sreg)]);
//           break;
//       }
//     }
//     break;
//
//     case AddressingMode_Direct:
//     case AddressingMode_RegisterIndirect:
//     case AddressingMode_Indexed:
//     case AddressingMode_BasedIndexed:
//     case AddressingMode_BasedIndexedDisplacement:
//     case AddressingMode_SIB:
//     {
//       mov(RPARAM1_64, RCPUPTR);
//       mov(RPARAM2_32, uint32(instruction->segment));
//       if (operand->mode == AddressingMode_Direct)
//         mov(RPARAM3_32, operand->direct.address);
//       else
//         mov(RPARAM3_32, READDR32);
//
//       switch (operand->size)
//       {
//         case OperandSize_8:
//           CallModuleFunction(ReadMemoryByteTrampoline);
//           break;
//         case OperandSize_16:
//           CallModuleFunction(ReadMemoryWordTrampoline);
//           break;
//         case OperandSize_32:
//           CallModuleFunction(ReadMemoryDWordTrampoline);
//           break;
//       }
//
//       switch (output_size)
//       {
//         case OperandSize_8:
//           mov(dest, RRET_8);
//           break;
//         case OperandSize_16:
//         {
//           switch (operand->size)
//           {
//             case OperandSize_8:
//             {
//               if (sign_extend)
//                 movsx(dest, RRET_8);
//               else
//                 movzx(dest, RRET_8);
//             }
//             break;
//             case OperandSize_16:
//             case OperandSize_32:
//               mov(dest, RRET_16);
//               break;
//           }
//         }
//         break;
//         case OperandSize_32:
//         {
//           switch (operand->size)
//           {
//             case OperandSize_8:
//             {
//               if (sign_extend)
//                 movsx(dest, RRET_8);
//               else
//                 movzx(dest, RRET_8);
//             }
//             break;
//             case OperandSize_16:
//             {
//               if (sign_extend)
//                 movsx(dest, RRET_16);
//               else
//                 movzx(dest, RRET_16);
//             }
//             break;
//             case OperandSize_32:
//               mov(dest, RRET_32);
//               break;
//           }
//         }
//         break;
//       }
//     }
//     break;
//
//     default:
//       Panic("Unhandled address mode");
//       break;
//   }
// }
//
// void RecompilerTranslator::WriteOperand(const OldInstruction* instruction, size_t index, const Xbyak::Reg& src)
// {
//   const OldInstruction::Operand* operand = &instruction->operands[index];
//   switch (operand->mode)
//   {
//     case AddressingMode_Register:
//     {
//       switch (operand->size)
//       {
//         case OperandSize_8:
//           mov(byte[RCPUPTR + CalculateRegisterOffset(operand->reg.reg8)], src);
//           break;
//         case OperandSize_16:
//           mov(word[RCPUPTR + CalculateRegisterOffset(operand->reg.reg16)], src);
//           break;
//         case OperandSize_32:
//           mov(dword[RCPUPTR + CalculateRegisterOffset(operand->reg.reg32)], src);
//           break;
//       }
//     }
//     break;
//
//     case AddressingMode_SegmentRegister:
//     {
//       // Truncate higher lengths to 16-bits.
//       mov(RPARAM1_64, RCPUPTR);
//       mov(RPARAM2_32, uint32(instruction->operands[0].reg.sreg));
//       movzx(RPARAM3_32, (src.isBit(16)) ? src : src.changeBit(16));
//       CallModuleFunction(LoadSegmentRegisterTrampoline);
//     }
//     break;
//
//     case AddressingMode_Direct:
//     case AddressingMode_RegisterIndirect:
//     case AddressingMode_Indexed:
//     case AddressingMode_BasedIndexed:
//     case AddressingMode_BasedIndexedDisplacement:
//     case AddressingMode_SIB:
//     {
//       mov(RPARAM1_64, RCPUPTR);
//       mov(RPARAM2_32, uint32(instruction->segment));
//       if (operand->mode == AddressingMode_Direct)
//         mov(RPARAM3_32, operand->direct.address);
//       else
//         mov(RPARAM3_32, READDR32);
//
//       switch (operand->size)
//       {
//         case OperandSize_8:
//           movzx(RPARAM4_32, src);
//           CallModuleFunction(WriteMemoryByteTrampoline);
//           break;
//         case OperandSize_16:
//           movzx(RPARAM4_32, src);
//           CallModuleFunction(WriteMemoryWordTrampoline);
//           break;
//         case OperandSize_32:
//           mov(RPARAM4_32, src);
//           CallModuleFunction(WriteMemoryDWordTrampoline);
//           break;
//       }
//     }
//     break;
//
//     default:
//       Panic("Unhandled address mode");
//       break;
//   }
// }
//
// void RecompilerTranslator::ReadFarAddressOperand(const OldInstruction* instruction, size_t index,
//                                                 const Xbyak::Reg& dest_segment, const Xbyak::Reg& dest_offset)
// {
//   const OldInstruction::Operand* operand = &instruction->operands[index];
//   if (operand->mode == AddressingMode_FarAddress)
//   {
//     mov(dest_segment, ZeroExtend32(operand->far_address.segment_selector));
//     mov(dest_offset, operand->far_address.address);
//     return;
//   }
//
//   // TODO: Should READDR32+2 wrap at FFFF?
//   if (instruction->operand_size == OperandSize_16)
//   {
//     mov(RPARAM1_64, RCPUPTR);
//     mov(RPARAM2_32, uint32(instruction->segment));
//     if (operand->mode == AddressingMode_Direct)
//       mov(RPARAM3_32, operand->direct.address);
//     else
//       mov(RPARAM3_32, READDR32);
//     CallModuleFunction(ReadMemoryWordTrampoline);
//     movzx(dest_offset, RRET_16);
//
//     mov(RPARAM1_64, RCPUPTR);
//     mov(RPARAM2_32, uint32(instruction->segment));
//     if (operand->mode == AddressingMode_Direct)
//       mov(RPARAM3_32, operand->direct.address + 2);
//     else
//       lea(RPARAM3_32, word[READDR32 + 2]);
//     CallModuleFunction(ReadMemoryWordTrampoline);
//     mov(dest_segment, RRET_16);
//   }
//   else
//   {
//     mov(RPARAM1_64, RCPUPTR);
//     mov(RPARAM2_32, uint32(instruction->segment));
//     if (operand->mode == AddressingMode_Direct)
//       mov(RPARAM3_32, operand->direct.address);
//     else
//       mov(RPARAM3_32, READDR32);
//     CallModuleFunction(ReadMemoryDWordTrampoline);
//     mov(dest_offset, RRET_32);
//
//     mov(RPARAM1_64, RCPUPTR);
//     mov(RPARAM2_32, uint32(instruction->segment));
//     if (operand->mode == AddressingMode_Direct)
//       mov(RPARAM3_32, operand->direct.address + 4);
//     else
//       lea(RPARAM3_32, dword[READDR32 + 4]);
//     CallModuleFunction(ReadMemoryWordTrampoline);
//     mov(dest_segment, RRET_16);
//   }
// }
//
// void RecompilerTranslator::UpdateFlags(uint32 clear_mask, uint32 set_mask, uint32 host_mask)
// {
//   // Shouldn't be clearing/setting any bits we're also getting from the host.
//   DebugAssert((host_mask & clear_mask) == 0 && (host_mask & set_mask) == 0);
//
//   // Clear the bits from the host too, since we set them later.
//   clear_mask |= host_mask;
//
//   // We need to grab the flags from the host first, before we do anything that'll lose the contents.
//   // TODO: Check cpuid for LAHF support
//   uint32 supported_flags = Flag_CF | Flag_PF | Flag_AF | Flag_ZF | Flag_SF;
//   bool uses_high_flags = ((host_mask & ~supported_flags) != 0);
//   bool use_eflags = ((host_mask & UINT32_C(0xFFFF0000)) != 0);
//   bool use_lahf = !uses_high_flags;
//   if (host_mask != 0)
//   {
//     if (use_lahf)
//     {
//       // Fast path via LAHF
//       lahf();
//     }
//     else
//     {
//       pushf();
//       pop(rax);
//     }
//   }
//
//   // Clear bits.
//   if (clear_mask != 0)
//   {
//     if ((clear_mask & UINT32_C(0xFFFF0000)) != 0)
//       and(dword[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ~clear_mask);
//     else
//       and(word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ~Truncate16(clear_mask));
//   }
//
//   // Set bits.
//   if (set_mask != 0)
//   {
//     if ((set_mask & UINT32_C(0xFFFF0000)) != 0)
//       or (dword[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], set_mask);
//     else
//       or (word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Truncate16(set_mask));
//   }
//
//   // Copy bits from host (cached in eax/ax/ah).
//   if (host_mask != 0)
//   {
//     if (use_lahf)
//     {
//       and(ah, Truncate8(host_mask));
//       or (byte[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ah);
//     }
//     else if (use_eflags)
//     {
//       and(eax, host_mask);
//       or (dword[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], eax);
//     }
//     else
//     {
//       and(ax, Truncate16(host_mask));
//       or (word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ax);
//     }
//   }
// }

void RecompilerTranslator::SyncInstructionPointers()
{
  //   if (!m_block->key.cs_size)
  //   {
  //     if (m_delayed_eip_add > 1)
  //     {
  //       add(word[RCPUPTR + offsetof(CPU, m_registers.EIP)], m_delayed_eip_add);
  //       add(word[RCPUPTR + offsetof(CPU, m_current_EIP)], m_delayed_eip_add);
  //     }
  //     else if (m_delayed_eip_add == 1)
  //     {
  //       inc(word[RCPUPTR + offsetof(CPU, m_registers.EIP)]);
  //       inc(word[RCPUPTR + offsetof(CPU, m_current_EIP)]);
  //     }
  //   }
  //   else
  //   {
  //     if (m_delayed_eip_add > 1)
  //     {
  //       add(dword[RCPUPTR + offsetof(CPU, m_registers.EIP)], m_delayed_eip_add);
  //       add(dword[RCPUPTR + offsetof(CPU, m_current_EIP)], m_delayed_eip_add);
  //     }
  //     else if (m_delayed_eip_add == 1)
  //     {
  //       inc(dword[RCPUPTR + offsetof(CPU, m_registers.EIP)]);
  //       inc(dword[RCPUPTR + offsetof(CPU, m_current_EIP)]);
  //     }
  //   }
  //   m_delayed_eip_add = 0;
  //
  //   if (m_delayed_cycles_add > 1)
  //     add(qword[RCPUPTR + offsetof(CPU, m_pending_cycles)], m_delayed_cycles_add);
  //   else if (m_delayed_cycles_add == 1)
  //     inc(qword[RCPUPTR + offsetof(CPU, m_pending_cycles)]);
  //   m_delayed_cycles_add = 0;
}

void RecompilerTranslator::StartInstruction(const Instruction* instruction)
{
  // REP instructions are always annoying.
  if (instruction->IsRep())
  {
    TinyString rep_start_block_name(TinyString::FromFormat("rep_start_%08X", instruction->address));
    TinyString rep_body_block_name(TinyString::FromFormat("rep_body_%08X", instruction->address));
    TinyString rep_end_block_name(TinyString::FromFormat("rep_end_%08X", instruction->address));
    llvm::BasicBlock* rep_start_block =
      llvm::BasicBlock::Create(m_backend->GetLLVMContext(), rep_start_block_name.GetCharArray(), m_function);
    llvm::BasicBlock* rep_body_block =
      llvm::BasicBlock::Create(m_backend->GetLLVMContext(), rep_body_block_name.GetCharArray(), m_function);
    llvm::BasicBlock* rep_end_block =
      llvm::BasicBlock::Create(m_backend->GetLLVMContext(), rep_end_block_name.GetCharArray(), m_function);

    // Jump unconditionally to the REP instruction.
    SyncInstructionPointers();
    m_builder.CreateBr(rep_start_block);
    m_builder.SetInsertPoint(rep_start_block);

    // Start of instruction - compare CX/ECX to zero.
    llvm::Value* compare_res;
    if (instruction->data.address_size == AddressSize_16)
      compare_res = m_builder.CreateICmpEQ(ReadRegister(Reg16_CX), m_builder.getInt16(0));
    else
      compare_res = m_builder.CreateICmpEQ(ReadRegister(Reg32_ECX), m_builder.getInt32(0));
    m_builder.CreateCondBr(compare_res, rep_end_block, rep_body_block);

    // Switch to the body to generate the instruction code.
    m_basic_block = rep_body_block;
    m_rep_start_block = rep_start_block;
    m_rep_end_block = rep_end_block;
  }
  else if (CanInstructionFault(instruction))
  {
    // Defer updates for non-faulting instructions.
    SyncInstructionPointers();
  }

  m_delayed_eip_add += instruction->length;
  m_delayed_cycles_add++;
}

void RecompilerTranslator::EndInstruction(const Instruction* instruction, bool update_eip, bool update_esp)
{
  if (instruction->IsRep())
  {
    // Decrement CX/ECX.
    if (instruction->data.address_size == AddressSize_16)
      WriteRegister(Reg16_CX, m_builder.CreateSub(ReadRegister(Reg16_CX), m_builder.getInt16(1)));
    else
      WriteRegister(Reg32_ECX, m_builder.CreateSub(ReadRegister(Reg32_ECX), m_builder.getInt32(1)));

    // TODO: Increment the cycles variable.

    // Do we need to check the equals flag?
    if (instruction->IsRepConditional())
    {
      // branch((EFLAGS & Flag_ZF) != 0, start_of_instruction, end_of_instruction)
      llvm::Value* masked_flags =
        m_builder.CreateAnd(ReadRegister(Reg32_EFLAGS), m_builder.getInt32(Flag_ZF), "masked_flags");
      llvm::Value* flags_compare_zf =
        m_builder.CreateICmp(instruction->IsRepEqual() ? llvm::CmpInst::ICMP_EQ : llvm::CmpInst::ICMP_NE, masked_flags,
                             m_builder.getInt32(0), "flags_compare_zf");
      m_builder.CreateCondBr(flags_compare_zf, m_rep_start_block, m_rep_end_block);
    }
    else
    {
      m_builder.CreateBr(m_rep_start_block);
    }

    // Switch to the next instruction's block.
    m_builder.SetInsertPoint(m_rep_end_block);
    m_basic_block = m_rep_end_block;
    m_rep_start_block = nullptr;
    m_rep_end_block = nullptr;
  }

  if (update_eip)
    SyncInstructionPointers();

  //   // If this instruction uses the stack, we need to update m_current_ESP for the next instruction.
  //   if (update_esp)
  //   {
  //     mov(RTEMP32A, dword[RCPUPTR + offsetof(CPU, m_registers.ESP)]);
  //     mov(dword[RCPUPTR + offsetof(CPU, m_current_ESP)], RTEMP32A);
  //   }
}

bool RecompilerTranslator::Compile_NOP(const Instruction* instruction)
{
  StartInstruction(instruction);
  EndInstruction(instruction);
  return true;
}

bool RecompilerTranslator::Compile_Fallback(const Instruction* instruction)
{
  StartInstruction(instruction);

  // Assume any instruction can manipulate ESP.
  EndInstruction(instruction, true, true);
  return true;
}

#if 0

bool RecompilerTranslator::Compile_LEA(const OldInstruction* instruction)
{
  StartInstruction(instruction);

  // Direct address modes don't go through the CalculateEffectiveAddress path, so we have to handle them manually.
  if (instruction->operands[1].mode == AddressingMode_Direct)
  {
    // LEA should always target a register, so we can optimize this to a single instruction here.
    Assert(instruction->operands[0].mode == AddressingMode_Register);
    switch (instruction->operand_size)
    {
      case OperandSize_16:
        mov(word[RCPUPTR + CalculateRegisterOffset(instruction->operands[0].reg.reg16)],
            Truncate16(instruction->operands[1].direct.address));
        break;
      case OperandSize_32:
        mov(dword[RCPUPTR + CalculateRegisterOffset(instruction->operands[0].reg.reg32)],
            instruction->operands[1].direct.address);
        break;
      default:
        return false;
    }
  }
  else
  {
    CalculateEffectiveAddress(instruction);
    switch (instruction->operand_size)
    {
      case OperandSize_16:
        WriteOperand(instruction, 0, READDR16);
        break;
      case OperandSize_32:
        WriteOperand(instruction, 0, READDR32);
        break;
      default:
        return false;
    }
  }

  EndInstruction(instruction, true, OperandIsESP(instruction->operands[0]));
  return true;
}

bool RecompilerTranslator::Compile_MOV(const OldInstruction* instruction)
{
  StartInstruction(instruction);
  CalculateEffectiveAddress(instruction);

  switch (instruction->operands[0].size)
  {
    case OperandSize_8:
    {
      ReadOperand(instruction, 1, RSTORE8A, false);
      WriteOperand(instruction, 0, RSTORE8A);
    }
    break;

    case OperandSize_16:
    {
      ReadOperand(instruction, 1, RSTORE16A, false);
      WriteOperand(instruction, 0, RSTORE16A);
    }
    break;

    case OperandSize_32:
    {
      ReadOperand(instruction, 1, RSTORE32A, false);
      WriteOperand(instruction, 0, RSTORE32A);
    }
    break;

    default:
      return false;
  }

  EndInstruction(instruction, true, OperandIsESP(instruction->operands[0]));
  return true;
}

bool RecompilerTranslator::Compile_MOV_Extended(const OldInstruction* instruction)
{
  StartInstruction(instruction);
  CalculateEffectiveAddress(instruction);

  bool sign_extend = (instruction->operation == Operation_MOVSX);
  switch (instruction->operands[0].size)
  {
    case OperandSize_16:
    {
      ReadOperand(instruction, 1, RSTORE16A, sign_extend);
      WriteOperand(instruction, 0, RSTORE16A);
    }
    break;

    case OperandSize_32:
    {
      ReadOperand(instruction, 1, RSTORE32A, sign_extend);
      WriteOperand(instruction, 0, RSTORE32A);
    }
    break;

    default:
      return false;
  }

  EndInstruction(instruction, true, OperandIsESP(instruction->operands[0]));
  return true;
}

bool RecompilerTranslator::Compile_ALU_Binary_Update(const OldInstruction* instruction)
{
  StartInstruction(instruction);
  CalculateEffectiveAddress(instruction);

  switch (instruction->operands[0].size)
  {
    case OperandSize_8:
    {
      ReadOperand(instruction, 1, RSTORE8B, true);
      ReadOperand(instruction, 0, RSTORE8A, false);

      switch (instruction->operation)
      {
        case Operation_ADD:
          add(RSTORE8A, RSTORE8B);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_SUB:
          sub(RSTORE8A, RSTORE8B);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_AND:
          and(RSTORE8A, RSTORE8B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_OR:
          or (RSTORE8A, RSTORE8B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_XOR:
          xor(RSTORE8A, RSTORE8B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
      }

      WriteOperand(instruction, 0, RSTORE8A);
    }
    break;

    case OperandSize_16:
    {
      ReadOperand(instruction, 1, RSTORE16B, true);
      ReadOperand(instruction, 0, RSTORE16A, false);

      switch (instruction->operation)
      {
        case Operation_ADD:
          add(RSTORE16A, RSTORE16B);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_SUB:
          sub(RSTORE16A, RSTORE16B);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_AND:
          and(RSTORE16A, RSTORE16B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_OR:
          or (RSTORE16A, RSTORE16B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_XOR:
          xor(RSTORE16A, RSTORE16B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
      }

      WriteOperand(instruction, 0, RSTORE16A);
    }
    break;

    case OperandSize_32:
    {
      ReadOperand(instruction, 1, RSTORE32B, true);
      ReadOperand(instruction, 0, RSTORE32A, false);

      switch (instruction->operation)
      {
        case Operation_ADD:
          add(RSTORE32A, RSTORE32B);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_SUB:
          sub(RSTORE32A, RSTORE32B);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_AND:
          and(RSTORE32A, RSTORE32B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_OR:
          or (RSTORE32A, RSTORE32B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_XOR:
          xor(RSTORE32A, RSTORE32B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
      }

      WriteOperand(instruction, 0, RSTORE32A);
    }
    break;

    default:
      return false;
  }

  EndInstruction(instruction, true, OperandIsESP(instruction->operands[0]));
  return true;
}

bool RecompilerTranslator::Compile_ALU_Binary_Test(const OldInstruction* instruction)
{
  StartInstruction(instruction);
  CalculateEffectiveAddress(instruction);

  switch (instruction->operands[0].size)
  {
    case OperandSize_8:
    {
      ReadOperand(instruction, 1, RSTORE8B, true);
      ReadOperand(instruction, 0, RSTORE8A, false);

      switch (instruction->operation)
      {
        case Operation_CMP:
          sub(RSTORE8A, RSTORE8B);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_TEST:
          and(RSTORE8A, RSTORE8B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
      }
    }
    break;

    case OperandSize_16:
    {
      ReadOperand(instruction, 1, RSTORE16B, true);
      ReadOperand(instruction, 0, RSTORE16A, false);

      switch (instruction->operation)
      {
        case Operation_CMP:
          sub(RSTORE16A, RSTORE16B);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_TEST:
          and(RSTORE16A, RSTORE16B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
      }
    }
    break;

    case OperandSize_32:
    {
      ReadOperand(instruction, 1, RSTORE32B, true);
      ReadOperand(instruction, 0, RSTORE32A, false);

      switch (instruction->operation)
      {
        case Operation_CMP:
          sub(RSTORE32A, RSTORE32B);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_TEST:
          and(RSTORE32A, RSTORE32B);
          UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
          break;
      }
    }
    break;

    default:
      return false;
  }

  EndInstruction(instruction);
  return true;
}

bool RecompilerTranslator::Compile_ALU_Unary_Update(const OldInstruction* instruction)
{
  StartInstruction(instruction);
  CalculateEffectiveAddress(instruction);

  switch (instruction->operands[0].size)
  {
    case OperandSize_8:
    {
      ReadOperand(instruction, 0, RSTORE8A, false);

      switch (instruction->operation)
      {
        case Operation_INC:
          inc(RSTORE8A);
          UpdateFlags(0, 0, Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_DEC:
          dec(RSTORE8A);
          UpdateFlags(0, 0, Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_NEG:
          neg(RSTORE8A);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_NOT:
          not(RSTORE8A);
          break;
      }

      WriteOperand(instruction, 0, RSTORE8A);
    }
    break;

    case OperandSize_16:
    {
      ReadOperand(instruction, 0, RSTORE16A, false);

      switch (instruction->operation)
      {
        case Operation_INC:
          inc(RSTORE16A);
          UpdateFlags(0, 0, Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_DEC:
          dec(RSTORE16A);
          UpdateFlags(0, 0, Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_NEG:
          neg(RSTORE16A);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_NOT:
          not(RSTORE16A);
          break;
      }

      WriteOperand(instruction, 0, RSTORE16A);
    }
    break;

    case OperandSize_32:
    {
      ReadOperand(instruction, 0, RSTORE32A, false);

      switch (instruction->operation)
      {
        case Operation_INC:
          inc(RSTORE32A);
          UpdateFlags(0, 0, Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_DEC:
          dec(RSTORE32A);
          UpdateFlags(0, 0, Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_NEG:
          neg(RSTORE32A);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_NOT:
          not(RSTORE32A);
          break;
      }

      WriteOperand(instruction, 0, RSTORE32A);
    }
    break;

    default:
      return false;
  }

  EndInstruction(instruction, true, OperandIsESP(instruction->operands[0]));
  return true;
}

bool RecompilerTranslator::Compile_ShiftRotate(const OldInstruction* instruction)
{
  // Fast path for {shl,shr} reg, 0.
  bool is_constant_shift = IsConstantOperand(instruction, 1);
  bool is_unary_version = (IsConstantOperand(instruction, 1) && GetConstantOperand(instruction, 1, false) == 1);
  uint8 constant_shift = is_constant_shift ? Truncate8(GetConstantOperand(instruction, 1, false) & 0x1F) : 0;
  if (is_constant_shift && constant_shift == 0)
  {
    StartInstruction(instruction);
    EndInstruction(instruction);
    return true;
  }

  StartInstruction(instruction);
  CalculateEffectiveAddress(instruction);

  // Load the value to be shifted. This always has to happen (e.g. VGA).
  switch (instruction->operands[0].size)
  {
    case OperandSize_8:
      ReadOperand(instruction, 0, RSTORE8A, false);
      break;
    case OperandSize_16:
      ReadOperand(instruction, 0, RSTORE16A, false);
      break;
    case OperandSize_32:
      ReadOperand(instruction, 0, RSTORE32A, false);
      break;
  }

  // If the shift count operand isn't constant, we need to jump on it conditionally, and skip the operand write/flag
  // update.
  Xbyak::Label skip_label;
  if (!is_constant_shift)
  {
    ReadOperand(instruction, 1, cl, false);
    and(cl, 0x1F);
    test(cl, cl);
    jz(skip_label);
  }

  // For non-constant shifts, the shift amount is pre-loaded in CL.
  switch (instruction->operands[0].size)
  {
    case OperandSize_8:
    {
      switch (instruction->operation)
      {
        case Operation_SHL:
          (is_constant_shift) ? shl(RSTORE8A, constant_shift) : shl(RSTORE8A, cl);
          UpdateFlags(Flag_AF | (is_unary_version ? 0 : Flag_OF), 0,
                      Flag_CF | Flag_SF | Flag_ZF | Flag_PF | (is_unary_version ? Flag_OF : 0));
          break;
        case Operation_SHR:
          (is_constant_shift) ? shr(RSTORE8A, constant_shift) : shr(RSTORE8A, cl);
          UpdateFlags((is_unary_version ? 0 : Flag_OF), 0,
                      Flag_CF | Flag_SF | Flag_ZF | Flag_PF | (is_unary_version ? Flag_OF : 0));
          break;
        case Operation_SAR:
          (is_constant_shift) ? sar(RSTORE8A, constant_shift) : sar(RSTORE8A, cl);
          UpdateFlags(Flag_OF, 0, Flag_CF | Flag_SF | Flag_ZF | Flag_PF);
          break;
      }

      WriteOperand(instruction, 0, RSTORE8A);
    }
    break;

    case OperandSize_16:
    {
      switch (instruction->operation)
      {
        case Operation_SHL:
          (is_constant_shift) ? shl(RSTORE16A, constant_shift) : shl(RSTORE16A, cl);
          UpdateFlags(Flag_AF | (is_unary_version ? 0 : Flag_OF), 0,
                      Flag_CF | Flag_SF | Flag_ZF | Flag_PF | (is_unary_version ? Flag_OF : 0));
          break;
        case Operation_SHR:
          (is_constant_shift) ? (shr(RSTORE16A, constant_shift)) : shr(RSTORE16A, cl);
          UpdateFlags((is_unary_version ? 0 : Flag_OF), 0,
                      Flag_CF | Flag_SF | Flag_ZF | Flag_PF | (is_unary_version ? Flag_OF : 0));
          break;
        case Operation_SAR:
          (is_constant_shift) ? sar(RSTORE16A, constant_shift) : sar(RSTORE16A, cl);
          UpdateFlags(Flag_OF, 0, Flag_CF | Flag_SF | Flag_ZF | Flag_PF);
          break;
      }

      WriteOperand(instruction, 0, RSTORE16A);
    }
    break;

    case OperandSize_32:
    {
      switch (instruction->operation)
      {
        case Operation_SHL:
          (is_constant_shift) ? shl(RSTORE32A, constant_shift) : shl(RSTORE32A, cl);
          UpdateFlags(Flag_AF | (is_unary_version ? 0 : Flag_OF), 0,
                      Flag_CF | Flag_SF | Flag_ZF | Flag_PF | (is_unary_version ? Flag_OF : 0));
          break;
        case Operation_SHR:
          (is_constant_shift) ? shr(RSTORE32A, constant_shift) : shr(RSTORE32A, cl);
          UpdateFlags((is_unary_version ? 0 : Flag_OF), 0,
                      Flag_CF | Flag_SF | Flag_ZF | Flag_PF | (is_unary_version ? Flag_OF : 0));
          break;
        case Operation_SAR:
          (is_constant_shift) ? sar(RSTORE32A, constant_shift) : sar(RSTORE32A, cl);
          UpdateFlags(Flag_OF, 0, Flag_CF | Flag_SF | Flag_ZF | Flag_PF);
          break;
      }

      WriteOperand(instruction, 0, RSTORE32A);
    }
    break;

    default:
      return false;
  }

  L(skip_label);

  EndInstruction(instruction, true, OperandIsESP(instruction->operands[0]));
  return true;
}

bool RecompilerTranslator::Compile_DoublePrecisionShift(const OldInstruction* instruction)
{
  // Fast path for {shld,shrd} reg, reg, 0.
  bool is_constant_shift = IsConstantOperand(instruction, 2);
  uint8 constant_shift = is_constant_shift ? Truncate8(GetConstantOperand(instruction, 2, false) & 0x1F) : 0;
  if (is_constant_shift && constant_shift == 0)
  {
    StartInstruction(instruction);
    EndInstruction(instruction);
    return true;
  }

  StartInstruction(instruction);
  CalculateEffectiveAddress(instruction);

  // Load the value to be shifted. This always has to happen (e.g. VGA).
  switch (instruction->operands[0].size)
  {
    case OperandSize_8:
      ReadOperand(instruction, 0, RSTORE8A, false);
      ReadOperand(instruction, 1, RSTORE8B, false);
      break;
    case OperandSize_16:
      ReadOperand(instruction, 0, RSTORE16A, false);
      ReadOperand(instruction, 1, RSTORE16B, false);
      break;
    case OperandSize_32:
      ReadOperand(instruction, 0, RSTORE32A, false);
      ReadOperand(instruction, 1, RSTORE32B, false);
      break;
  }

  // If the shift count operand isn't constant, we need to jump on it conditionally, and skip the operand write/flag
  // update.
  Xbyak::Label skip_label;
  if (!is_constant_shift)
  {
    ReadOperand(instruction, 2, cl, false);
    and(cl, 0x1F);
    test(cl, cl);
    jz(skip_label);
  }

  // For non-constant shifts, the shift amount is pre-loaded in CL.
  switch (instruction->operands[0].size)
  {
    case OperandSize_16:
    {
      switch (instruction->operation)
      {
        case Operation_SHLD:
          (is_constant_shift) ? shld(RSTORE16A, RSTORE16B, constant_shift) : shld(RSTORE16A, RSTORE16B, cl);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_SHRD:
          (is_constant_shift) ? shrd(RSTORE16A, RSTORE16B, constant_shift) : shrd(RSTORE16A, RSTORE16B, cl);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF);
          break;
      }

      WriteOperand(instruction, 0, RSTORE16A);
    }
    break;

    case OperandSize_32:
    {
      switch (instruction->operation)
      {
        case Operation_SHLD:
          (is_constant_shift) ? shld(RSTORE32A, RSTORE32B, constant_shift) : shld(RSTORE32A, RSTORE32B, cl);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF);
          break;
        case Operation_SHRD:
          (is_constant_shift) ? shrd(RSTORE32A, RSTORE32B, constant_shift) : shrd(RSTORE32A, RSTORE32B, cl);
          UpdateFlags(0, 0, Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF);
          break;
      }

      WriteOperand(instruction, 0, RSTORE32A);
    }
    break;

    default:
      return false;
  }

  L(skip_label);

  EndInstruction(instruction, true, OperandIsESP(instruction->operands[0]));
  return true;
}

// Necessary due to BranchTo being a member function.
void RecompilerTranslator::BranchToTrampoline(CPU* cpu, uint32 address)
{
  cpu->BranchTo(address);
}

void RecompilerTranslator::PushWordTrampoline(CPU* cpu, uint16 value)
{
  cpu->PushWord(value);
}

void RecompilerTranslator::PushDWordTrampoline(CPU* cpu, uint32 value)
{
  cpu->PushDWord(value);
}

uint16 RecompilerTranslator::PopWordTrampoline(CPU* cpu)
{
  return cpu->PopWord();
}

uint32 RecompilerTranslator::PopDWordTrampoline(CPU* cpu)
{
  return cpu->PopDWord();
}

void RecompilerTranslator::LoadSegmentRegisterTrampoline(CPU* cpu, uint32 segment, uint16 value)
{
  cpu->LoadSegmentRegister(static_cast<Segment>(segment), value);
}

void RecompilerTranslator::RaiseExceptionTrampoline(CPU* cpu, uint32 interrupt, uint32 error_code)
{
  cpu->RaiseException(interrupt, error_code);
}

void RecompilerTranslator::SetFlagsTrampoline(CPU* cpu, uint32 flags)
{
  cpu->SetFlags(flags);
}

void RecompilerTranslator::SetFlags16Trampoline(CPU* cpu, uint16 flags)
{
  cpu->SetFlags16(flags);
}

void RecompilerTranslator::FarJumpTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size)
{
  cpu->FarJump(segment_selector, offset, static_cast<OperandSize>(op_size));
}

void RecompilerTranslator::FarCallTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size)
{
  cpu->FarCall(segment_selector, offset, static_cast<OperandSize>(op_size));
}

void RecompilerTranslator::FarReturnTrampoline(CPU* cpu, uint32 op_size, uint32 pop_count)
{
  cpu->FarReturn(static_cast<OperandSize>(op_size), pop_count);
}

bool RecompilerTranslator::Compile_JumpConditional(const OldInstruction* instruction)
{
  StartInstruction(instruction);

  Xbyak::Label test_pass_label;
  Xbyak::Label test_fail_label;

  // LOOP should also test ECX.
  if (instruction->operation == Operation_LOOP)
  {
    if (instruction->address_size == AddressSize_16)
    {
      dec(word[RCPUPTR + offsetof(CPU, m_registers.ECX)]);
      jz(test_fail_label);
    }
    else
    {
      dec(dword[RCPUPTR + offsetof(CPU, m_registers.ECX)]);
      jz(test_fail_label);
    }
  }

  // The jumps here are inverted, so that the fail case can jump over the branch.
  switch (instruction->jump_condition)
  {
    case JumpCondition_Always:
      // Just fall through to the real jump.
      break;

    case JumpCondition_Overflow:
      // Jump if OF is set.
      test(word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_OF);
      jz(test_fail_label);
      break;

    case JumpCondition_NotOverflow:
      // Jump if OF is not set.
      test(word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_OF);
      jnz(test_fail_label);
      break;

    case JumpCondition_Sign:
      // Jump is SF is set.
      test(byte[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_SF);
      jz(test_fail_label);
      break;

    case JumpCondition_NotSign:
      // Jump if SF is not set.
      test(byte[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_SF);
      jnz(test_fail_label);
      break;

    case JumpCondition_Equal:
      // Jump if ZF is set.
      test(byte[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_ZF);
      jz(test_fail_label);
      break;

    case JumpCondition_NotEqual:
      // Jump if ZF is not set.
      test(byte[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_ZF);
      jnz(test_fail_label);
      break;

    case JumpCondition_Below:
      // Jump if CF is set.
      test(byte[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_CF);
      jz(test_fail_label);
      break;

    case JumpCondition_AboveOrEqual:
      // Jump if CF is not set.
      test(byte[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_CF);
      jnz(test_fail_label);
      break;

    case JumpCondition_BelowOrEqual:
      // Jump if CF or ZF is set.
      test(byte[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_CF | Flag_ZF);
      jz(test_fail_label);
      break;

    case JumpCondition_Above:
      // Jump if neither CF not ZF is set.
      test(byte[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_CF | Flag_ZF);
      jnz(test_fail_label);
      break;

    case JumpCondition_Less:
      // Jump if SF != OF.
      mov(RTEMP16A, word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)]);
      mov(RTEMP16B, RTEMP16A);
      shr(RTEMP16A, 7);
      shr(RTEMP16B, 11);
      xor(RTEMP16A, RTEMP16B);
      test(RTEMP16A, 1u);
      jz(test_fail_label);
      break;

    case JumpCondition_GreaterOrEqual:
      // Jump if SF == OF.
      mov(RTEMP16A, word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)]);
      mov(RTEMP16B, RTEMP16A);
      shr(RTEMP16A, 7);
      shr(RTEMP16B, 11);
      xor(RTEMP16A, RTEMP16B);
      test(RTEMP16A, 1u);
      jnz(test_fail_label);
      break;

    case JumpCondition_LessOrEqual:
      // Jump if ZF or SF != OF.
      mov(RTEMP16A, word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)]);
      test(RTEMP16A, Flag_ZF);
      jnz(test_pass_label);
      mov(RTEMP16B, RTEMP16A);
      shr(RTEMP16A, 7);
      shr(RTEMP16B, 11);
      xor(RTEMP16A, RTEMP16B);
      test(RTEMP16A, 1u);
      jz(test_fail_label);
      break;

    case JumpCondition_Greater:
      // Jump if !ZF and SF == OF.
      mov(RTEMP16A, word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)]);
      test(RTEMP16A, Flag_ZF);
      jnz(test_fail_label);
      mov(RTEMP16B, RTEMP16A);
      shr(RTEMP16A, 7);
      shr(RTEMP16B, 11);
      xor(RTEMP16A, RTEMP16B);
      test(RTEMP16A, 1u);
      jnz(test_fail_label);
      break;

    case JumpCondition_Parity:
      // Jump if PF is set.
      test(word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_PF);
      jz(test_fail_label);
      break;

    case JumpCondition_NotParity:
      // Jump if PF is not set.
      test(word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Flag_PF);
      jnz(test_fail_label);
      break;

    case JumpCondition_CXZero:
    {
      if (instruction->address_size == AddressSize_16)
      {
        or (word[RCPUPTR + offsetof(CPU, m_registers.ECX)], 0u);
        jnz(test_fail_label);
      }
      else
      {
        or (dword[RCPUPTR + offsetof(CPU, m_registers.ECX)], 0u);
        jnz(test_fail_label);
      }
    }
    break;
  }

  // Jump pass branch.
  L(test_pass_label);

  // Should use relative addressing always.
  mov(RPARAM1_64, RCPUPTR);
  if (instruction->operand_size == OperandSize_16)
  {
    // Should be shorter than mov+add+and.
    mov(RPARAM2_16, word[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
    add(RPARAM2_16, uint32(instruction->operands[0].relative.displacement));
    movzx(RPARAM2_32, RPARAM2_16);
  }
  else
  {
    mov(RPARAM2_32, dword[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
    add(RPARAM2_32, uint32(instruction->operands[0].relative.displacement));
  }

  // m_current_EIP will not be correct here, but we will be at the end of the block anyway.
  CallModuleFunction(BranchToTrampoline);

  // Jump fail branch.
  L(test_fail_label);

  // No need to update EIP/ESP, end of block.
  EndInstruction(instruction, false, false);
  return true;
}

bool RecompilerTranslator::Compile_JumpCallReturn(const OldInstruction* instruction)
{
  StartInstruction(instruction);
  CalculateEffectiveAddress(instruction);

  switch (instruction->operation)
  {
    case Operation_JMP_Far:
    case Operation_CALL_Far:
    {
      // Far jump/call has to go through a slow path.
      ReadFarAddressOperand(instruction, 0, RSTORE16A, RSTORE32B);
      mov(RPARAM1_64, RCPUPTR);
      movzx(RPARAM2_32, RSTORE16A);
      mov(RPARAM3_32, RSTORE32B);
      mov(RPARAM4_32, static_cast<uint32>(instruction->operand_size));
      CallModuleFunction((instruction->operation == Operation_JMP_Far) ? FarJumpTrampoline : FarCallTrampoline);
    }
    break;

    case Operation_RET_Far:
    {
      // Far return also goes through a slow path.
      if (instruction->operands[0].mode != AddressingMode_None)
        ReadOperand(instruction, 0, RPARAM3_32, false);
      else
        xor(RPARAM3_32, RPARAM3_32);
      mov(RPARAM2_32, static_cast<uint32>(instruction->operand_size));
      mov(RPARAM1_64, RCPUPTR);
      CallModuleFunction(FarReturnTrampoline);
    }
    break;

    case Operation_JMP_Near:
    {
      const OldInstruction::Operand* operand = &instruction->operands[0];
      if (operand->mode == AddressingMode_Relative)
      {
        if (instruction->operand_size == OperandSize_16)
        {
          // Should be shorter than mov+add+and.
          mov(RPARAM2_16, word[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
          add(RPARAM2_16, uint32(instruction->operands[0].relative.displacement));
          movzx(RPARAM2_32, RPARAM2_16);
        }
        else
        {
          mov(RPARAM2_32, dword[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
          add(RPARAM2_32, uint32(instruction->operands[0].relative.displacement));
        }
        mov(RPARAM1_64, RCPUPTR);
        CallModuleFunction(BranchToTrampoline);
      }
      else
      {
        ReadOperand(instruction, 0, RPARAM2_32, false);
        mov(RPARAM1_64, RCPUPTR);
        CallModuleFunction(BranchToTrampoline);
      }
    }
    break;

    case Operation_CALL_Near:
    {
      const OldInstruction::Operand* operand = &instruction->operands[0];
      if (operand->mode == AddressingMode_Relative)
      {
        if (instruction->operand_size == OperandSize_16)
        {
          mov(RSTORE16A, word[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
          mov(RPARAM1_64, RCPUPTR);
          movzx(RPARAM2_32, RSTORE16A);
          CallModuleFunction(PushWordTrampoline);
          add(RSTORE16A, uint32(instruction->operands[0].relative.displacement));
          mov(RPARAM1_64, RCPUPTR);
          movzx(RPARAM2_32, RSTORE16A);
          CallModuleFunction(BranchToTrampoline);
        }
        else
        {
          mov(RSTORE32A, dword[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
          mov(RPARAM1_64, RCPUPTR);
          mov(RPARAM2_32, RSTORE32A);
          CallModuleFunction(PushDWordTrampoline);
          add(RSTORE32A, uint32(instruction->operands[0].relative.displacement));
          mov(RPARAM1_64, RCPUPTR);
          mov(RPARAM2_32, RSTORE32A);
          CallModuleFunction(BranchToTrampoline);
        }
      }
      else
      {
        // Non-relative.
        if (instruction->operand_size == OperandSize_16)
        {
          ReadOperand(instruction, 0, RSTORE16A, false);
          mov(RPARAM1_64, RCPUPTR);
          movzx(RPARAM2_32, word[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
          CallModuleFunction(PushWordTrampoline);
          mov(RPARAM1_64, RCPUPTR);
          movzx(RPARAM2_32, RSTORE16A);
          CallModuleFunction(BranchToTrampoline);
        }
        else
        {
          ReadOperand(instruction, 0, RSTORE32A, false);
          mov(RPARAM1_64, RCPUPTR);
          mov(RPARAM2_32, dword[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
          CallModuleFunction(PushDWordTrampoline);
          mov(RPARAM1_64, RCPUPTR);
          mov(RPARAM2_32, RSTORE32A);
          CallModuleFunction(BranchToTrampoline);
        }
      }
    }
    break;

    case Operation_RET_Near:
    {
      mov(RPARAM1_64, RCPUPTR);
      if (instruction->operand_size == OperandSize_16)
      {
        CallModuleFunction(PopWordTrampoline);
        movzx(RPARAM2_32, RRET_16);
      }
      else
      {
        CallModuleFunction(PopDWordTrampoline);
        mov(RPARAM2_32, RRET_32);
      }

      const OldInstruction::Operand* operand = &instruction->operands[0];
      Assert(operand->mode == AddressingMode_None || operand->mode == AddressingMode_Immediate);
      if (operand->mode == AddressingMode_Immediate)
      {
        if (m_cpu->m_stack_address_size == AddressSize_16)
          add(word[RCPUPTR + CalculateRegisterOffset(Reg32_ESP)], operand->immediate.value32);
        else
          add(dword[RCPUPTR + CalculateRegisterOffset(Reg32_ESP)], operand->immediate.value32);
      }

      mov(RPARAM1_64, RCPUPTR);
      CallModuleFunction(BranchToTrampoline);
    }
    break;

    default:
      return false;
  }

  // End of block, no need to update EIP/ESP.
  EndInstruction(instruction, false, false);
  return true;
}

bool RecompilerTranslator::Compile_Stack(const OldInstruction* instruction)
{
  // if (instruction->operands[0].mode == AddressingMode_SegmentRegister && instruction->operation == Operation_POP)
  // return Compile_Fallback(instruction);

  StartInstruction(instruction);

  // V8086/IOPL check for PUSHF/POPF. Compile out for other modes.
  if (m_cpu->InVirtual8086Mode() &&
      (instruction->operation == Operation_PUSHF || instruction->operation == Operation_POPF))
  {
    Xbyak::Label v8086_test_pass;
    mov(RTEMP32A, dword[RCPUPTR + CalculateRegisterOffset(Reg32_EFLAGS)]);
    test(RTEMP32A, Flag_VM);
    jz(v8086_test_pass);
    and(RTEMP32A, Flag_IOPL);
    shr(RTEMP32A, 12);
    cmp(RTEMP32A, 3);
    je(v8086_test_pass);
    mov(RPARAM1_64, RCPUPTR);
    mov(RPARAM2_32, Interrupt_GeneralProtectionFault);
    mov(RPARAM3_32, 0);
    CallModuleFunction(RaiseExceptionTrampoline);
    L(v8086_test_pass);
  }

  switch (instruction->operation)
  {
    case Operation_PUSH:
    {
      CalculateEffectiveAddress(instruction);
      if (instruction->operand_size == OperandSize_16)
      {
        ReadOperand(instruction, 0, RTEMP16A, true);
        mov(RPARAM1_64, RCPUPTR);
        movzx(RPARAM2_32, RTEMP16A);
        CallModuleFunction(PushWordTrampoline);
      }
      else
      {
        ReadOperand(instruction, 0, RPARAM2_32, true);
        mov(RPARAM1_64, RCPUPTR);
        CallModuleFunction(PushDWordTrampoline);
      }
    }
    break;

    case Operation_POP:
    {
      // Since we can pop to esp, EA calculation has to happen after the pop.
      if (instruction->operand_size == OperandSize_16)
      {
        mov(RPARAM1_64, RCPUPTR);
        CallModuleFunction(PopWordTrampoline);
        CalculateEffectiveAddress(instruction);
        WriteOperand(instruction, 0, RRET_16);
      }
      else
      {
        mov(RPARAM1_64, RCPUPTR);
        CallModuleFunction(PopDWordTrampoline);
        CalculateEffectiveAddress(instruction);
        WriteOperand(instruction, 0, RRET_32);
      }
    }
    break;

    case Operation_PUSHF:
    {
      if (instruction->operand_size == OperandSize_16)
      {
        mov(RPARAM1_64, RCPUPTR);
        movzx(RPARAM2_32, word[RCPUPTR + CalculateRegisterOffset(Reg32_EFLAGS)]);
        CallModuleFunction(PushWordTrampoline);
      }
      else
      {
        mov(RPARAM1_64, RCPUPTR);
        mov(RPARAM2_32, dword[RCPUPTR + CalculateRegisterOffset(Reg32_EFLAGS)]);
        and(RPARAM2_32, ~uint32(Flag_RF | Flag_VM));
        CallModuleFunction(PushDWordTrampoline);
      }
    }
    break;

    case Operation_POPF:
    {
      if (instruction->operand_size == OperandSize_16)
      {
        mov(RPARAM1_64, RCPUPTR);
        CallModuleFunction(PopWordTrampoline);
        mov(RPARAM1_64, RCPUPTR);
        movzx(RPARAM2_32, RRET_16);
        CallModuleFunction(SetFlags16Trampoline);
      }
      else
      {
        mov(RPARAM1_64, RCPUPTR);
        CallModuleFunction(PopDWordTrampoline);
        mov(RPARAM1_64, RCPUPTR);
        mov(RPARAM2_32, RRET_32);
        CallModuleFunction(SetFlagsTrampoline);
      }
    }
    break;

    default:
      return false;
  }

  EndInstruction(instruction, true, true);
  return true;
}

bool RecompilerTranslator::Compile_Flags(const OldInstruction* instruction)
{
  StartInstruction(instruction);

  switch (instruction->operation)
  {
    case Operation_CLC:
      UpdateFlags(Flag_CF, 0, 0);
      break;
    case Operation_CLD:
      UpdateFlags(Flag_DF, 0, 0);
      break;
    case Operation_STC:
      UpdateFlags(0, Flag_CF, 0);
      break;
    case Operation_STD:
      UpdateFlags(0, Flag_DF, 0);
      break;
  }

  EndInstruction(instruction);
  return true;
}
#endif
} // namespace CPU_X86
