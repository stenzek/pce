#include "pce/cpu_x86/jitx64_codegen.h"
#include "YBaseLib/Log.h"
#include "pce/cpu_x86/debugger_interface.h"
#include "pce/system.h"
#include "xbyak.h"
Log_SetChannel(CPUX86::Interpreter);

namespace CPU_X86 {

// TODO:
// Constant operands - don't move to a temporary register first
// Only sync current_ESP at the start of the block, and on push/pop instructions
// Threaded code generator?
// Push functions per stack address mode
// Only sync EIP on potentially-faulting instructions
// Lazy flag calculation - store operands and opcode

// void JitX64Backend::Block::AllocCode(size_t size)
// {
//     code_size = size;
//     code_pointer = reinterpret_cast<CodePointer>(Xbyak::AlignedMalloc(code_size, 4096));
//     if (!code_pointer)
//         Panic("Failed to allocate code pointer");
//
//     if (!Xbyak::CodeArray::protect(reinterpret_cast<void*>(code_pointer), code_size, true))
//         Panic("Failed to protect code pointer");
// }

JitX64Backend::Block::~Block()
{
  //     if (code_pointer)
  //         Xbyak::AlignedFree(reinterpret_cast<void*>(code_pointer));
}

JitX64CodeGenerator::JitX64CodeGenerator(JitX64Backend* backend, void* code_ptr, size_t code_size)
  : Xbyak::CodeGenerator(code_size, code_ptr), m_backend(backend), m_cpu(backend->m_cpu)
#if ABI_WIN64
    ,
    RTEMP8A(al), RTEMP8B(cl), RTEMP8C(dl), RTEMP16A(ax), RTEMP16B(cx), RTEMP16C(dx), RTEMP32A(eax), RTEMP32B(ecx),
    RTEMP32C(edx), RTEMP64A(rax), RTEMP64B(rcx), RTEMP64C(rdx), RTEMPADDR(r8), RSTORE8A(bl), RSTORE8B(r12b),
    RSTORE8C(r13b), RSTORE16A(bx), RSTORE16B(r12w), RSTORE16C(r13w), RSTORE32A(ebx), RSTORE32B(r12d), RSTORE32C(r13d),
    RSTORE64A(rbx), RSTORE64B(r12), RSTORE64C(r13), READDR16(r14w), READDR32(r14d), READDR64(r14), RCPUPTR(rsi),
    RSCRATCH64(r11), RSCRATCH32(r11d), RSCRATCH16(r11w), RSCRATCH8(r11b), RPARAM1_8(cl), RPARAM2_8(dl), RPARAM3_8(r8b),
    RPARAM4_8(r9b), RRET_8(al), RPARAM1_16(cx), RPARAM2_16(dx), RPARAM3_16(r8w), RPARAM4_16(r9w), RRET_16(ax),
    RPARAM1_32(ecx), RPARAM2_32(edx), RPARAM3_32(r8d), RPARAM4_32(r9d), RRET_32(eax), RPARAM1_64(rcx), RPARAM2_64(rdx),
    RPARAM3_64(r8), RPARAM4_64(r9), RRET_64(rax)
#elif ABI_SYSV
    ,
    RTEMP8A(al), RTEMP8B(cl), RTEMP8C(dl), RTEMP16A(ax), RTEMP16B(cx), RTEMP16C(dx), RTEMP32A(eax), RTEMP32B(ecx),
    RTEMP32C(edx), RTEMP64A(rax), RTEMP64B(rcx), RTEMP64C(rdx), RTEMPADDR(r8), RSTORE8A(bl), RSTORE8B(r12b),
    RSTORE8C(r13b), RSTORE16A(bx), RSTORE16B(r12w), RSTORE16C(r13w), RSTORE32A(ebx), RSTORE32B(r12d), RSTORE32C(r13d),
    RSTORE64A(rbx), RSTORE64B(r12), RSTORE64C(r13), READDR16(r14w), READDR32(r14d), READDR64(r14), RCPUPTR(rbp),
    RSCRATCH64(r11), RSCRATCH32(r11d), RSCRATCH16(r11w), RSCRATCH8(r11b), RPARAM1_8(dil), RPARAM2_8(sil), RPARAM3_8(dl),
    RPARAM4_8(cl), RRET_8(al), RPARAM1_16(di), RPARAM2_16(si), RPARAM3_16(dx), RPARAM4_16(cx), RRET_16(ax),
    RPARAM1_32(edi), RPARAM2_32(esi), RPARAM3_32(edx), RPARAM4_32(ecx), RRET_32(eax), RPARAM1_64(rdi), RPARAM2_64(rsi),
    RPARAM3_64(rdx), RPARAM4_64(rcx), RRET_64(rax)
#endif
{
  // Save nonvolatile registers
  // TODO: Stash nops and a forward code pointer, skip for unneeded registers.
  // NOTE: Should be aligned so that rsp+8h % 16 = 0
  // TODO: Use sil? shorter?
  push(RSTORE64A);
  push(RSTORE64B);
  push(RSTORE64C);
  push(READDR64);
  push(RCPUPTR);

#if ABI_WIN64
  // Only on Windows
  sub(rsp, 0x20);
#endif

  // Load CPU pointer.
  mov(RCPUPTR, RPARAM1_64);

  // Update current EIP/ESP for exceptions.
  mov(RTEMP32A, dword[RCPUPTR + offsetof(CPU, m_registers.EIP)]);
  mov(RTEMP32B, dword[RCPUPTR + offsetof(CPU, m_registers.ESP)]);
  mov(dword[RCPUPTR + offsetof(CPU, m_current_EIP)], RTEMP32A);
  mov(dword[RCPUPTR + offsetof(CPU, m_current_ESP)], RTEMP32B);
}

JitX64CodeGenerator::~JitX64CodeGenerator() {}

std::pair<const void*, size_t> JitX64CodeGenerator::FinishBlock()
{
  Assert(m_delayed_eip_add == 0 && m_delayed_cycles_add == 0);

#if ABI_WIN64
  add(rsp, 0x20);
#endif

  // Restore nonvolatile registers
  pop(RCPUPTR);
  pop(READDR64);
  pop(RSTORE64C);
  pop(RSTORE64B);
  pop(RSTORE64A);

  // Exit to dispatcher
  ret();

  // Done
  ready();
  return std::make_pair(reinterpret_cast<const void*>(getCode()), getSize());
}

bool JitX64CodeGenerator::CompileInstruction(const Instruction* instruction, bool is_final)
{
  bool result;
  switch (instruction->operation)
  {
#if 0
    case Operation_NOP:
      result = Compile_NOP(instruction);
      break;
    case Operation_LEA:
      result = Compile_LEA(instruction);
      break;
    case Operation_MOV:
      result = Compile_MOV(instruction);
      break;
    case Operation_MOVSX:
    case Operation_MOVZX:
      result = Compile_MOV_Extended(instruction);
      break;
    case Operation_ADD:
    case Operation_SUB:
    case Operation_AND:
    case Operation_OR:
    case Operation_XOR:
      result = Compile_ALU_Binary_Update(instruction);
      break;
    case Operation_CMP:
    case Operation_TEST:
      result = Compile_ALU_Binary_Test(instruction);
      break;
    case Operation_INC:
    case Operation_DEC:
    case Operation_NEG:
    case Operation_NOT:
      result = Compile_ALU_Unary_Update(instruction);
      break;
    case Operation_SHL:
    case Operation_SHR:
    case Operation_SAR:
      result = Compile_ShiftRotate(instruction);
      break;
    case Operation_SHLD:
    case Operation_SHRD:
      result = Compile_DoublePrecisionShift(instruction);
      break;
    case Operation_Jcc:
    case Operation_LOOP:
      result = Compile_JumpConditional(instruction);
      break;
    case Operation_PUSH:
    case Operation_POP:
    case Operation_PUSHF:
    case Operation_POPF:
      result = Compile_Stack(instruction);
      break;
    case Operation_CALL_Far:
    case Operation_RET_Far:
    case Operation_CALL_Near:
    case Operation_RET_Near:
    case Operation_JMP_Near:
    case Operation_JMP_Far:
      result = Compile_JumpCallReturn(instruction);
      break;
    case Operation_CLC:
    case Operation_CLD:
    case Operation_STC:
    case Operation_STD:
      result = Compile_Flags(instruction);
      break;
#endif
    default:
      result = Compile_Fallback(instruction);
      break;
  }

  if (is_final)
    SyncInstructionPointers(instruction);

  return result;
}

uint32 JitX64CodeGenerator::CalculateRegisterOffset(Reg8 reg)
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

uint32 JitX64CodeGenerator::CalculateRegisterOffset(Reg16 reg)
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

uint32 JitX64CodeGenerator::CalculateRegisterOffset(Reg32 reg)
{
  return uint32(offsetof(CPU, m_registers.reg32[0]) + (reg * sizeof(uint32)));
}

uint32 JitX64CodeGenerator::CalculateSegmentRegisterOffset(Segment segment)
{
  return uint32(offsetof(CPU, m_registers.segment_selectors[0]) + (segment * sizeof(uint16)));
}

void JitX64CodeGenerator::CalculateEffectiveAddress(const Instruction* instruction)
{
#if 0
  for (size_t i = 0; i < countof(instruction->operands); i++)
  {
    const Instruction::Operand* operand = &instruction->operands[i];
    switch (operand->mode)
    {
      case AddressingMode_RegisterIndirect:
      {
        if (instruction->GetAddressSize() == AddressSize_16)
          movzx(READDR32, word[RCPUPTR + CalculateRegisterOffset(operand->reg.reg16)]);
        else
          mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->reg.reg32)]);
      }
      break;
      case AddressingMode_Indexed:
      {
        if (instruction->GetAddressSize() == AddressSize_16)
        {
          mov(READDR16, word[RCPUPTR + CalculateRegisterOffset(operand->indexed.reg.reg16)]);
          if (operand->indexed.displacement != 0)
            add(READDR16, uint32(operand->indexed.displacement));
          movzx(READDR32, READDR16);
        }
        else
        {
          mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->indexed.reg.reg32)]);
          if (operand->indexed.displacement != 0)
            add(READDR32, uint32(operand->indexed.displacement));
        }
      }
      break;
      case AddressingMode_BasedIndexed:
      {
        if (instruction->GetAddressSize() == AddressSize_16)
        {
          mov(READDR16, word[RCPUPTR + CalculateRegisterOffset(operand->based_indexed.base.reg16)]);
          add(READDR16, word[RCPUPTR + CalculateRegisterOffset(operand->based_indexed.index.reg16)]);
          movzx(READDR32, READDR16);
        }
        else
        {
          mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->based_indexed.base.reg32)]);
          add(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->based_indexed.index.reg32)]);
        }
      }
      break;
      case AddressingMode_BasedIndexedDisplacement:
      {
        if (instruction->GetAddressSize() == AddressSize_16)
        {
          mov(READDR16, word[RCPUPTR + CalculateRegisterOffset(operand->based_indexed_displacement.base.reg16)]);
          add(READDR16, word[RCPUPTR + CalculateRegisterOffset(operand->based_indexed_displacement.index.reg16)]);
          if (operand->based_indexed_displacement.displacement != 0)
            add(READDR16, uint32(operand->based_indexed_displacement.displacement));
          movzx(READDR32, READDR16);
        }
        else
        {
          mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->based_indexed_displacement.base.reg32)]);
          add(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->based_indexed_displacement.index.reg32)]);
          if (operand->based_indexed_displacement.displacement != 0)
            add(READDR32, uint32(operand->based_indexed_displacement.displacement));
        }
      }
      break;
      case AddressingMode_SIB:
      {
        Assert(instruction->GetAddressSize() == AddressSize_32);
        if (operand->sib.index.reg32 != Reg32_Count)
        {
          // This one is implemented in reverse, but should evaluate to the same results. This way we don't need a
          // temporary.
          mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->sib.index.reg32)]);
          shl(READDR32, operand->sib.scale_shift);
          if (operand->sib.base.reg32 != Reg32_Count)
            add(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->sib.base.reg32)]);
          if (operand->sib.displacement != 0)
            add(READDR32, uint32(operand->sib.displacement));
        }
        else if (operand->sib.base.reg32 != Reg32_Count)
        {
          // No index.
          mov(READDR32, dword[RCPUPTR + CalculateRegisterOffset(operand->sib.base.reg32)]);
          if (operand->sib.displacement != 0)
            add(READDR32, uint32(operand->sib.displacement));
        }
        else
        {
          // No base.
          if (operand->sib.displacement == 0)
            xor(READDR32, READDR32);
          else
            mov(READDR32, uint32(operand->sib.displacement));
        }
      }
      break;
    }
  }
#endif
}

bool JitX64CodeGenerator::IsConstantOperand(const Instruction* instruction, size_t index)
{
  const Instruction::Operand* operand = &instruction->operands[index];
  return (operand->mode == OperandMode_Immediate);
}

uint32 JitX64CodeGenerator::GetConstantOperand(const Instruction* instruction, size_t index, bool sign_extend)
{
  const Instruction::Operand* operand = &instruction->operands[index];
  DebugAssert(operand->mode == OperandMode_Immediate);

  switch (operand->size)
  {
    case OperandSize_8:
      return sign_extend ? SignExtend32(instruction->data.imm8) : ZeroExtend32(instruction->data.imm8);
      break;
    case OperandSize_16:
      return sign_extend ? SignExtend32(instruction->data.imm16) : ZeroExtend32(instruction->data.imm16);
    default:
      return instruction->data.imm32;
  }
}

static uint8 ReadMemoryByteTrampoline(CPU* cpu, uint32 segment, uint32 offset)
{
  return cpu->ReadMemoryByte(static_cast<Segment>(segment), offset);
}

static uint16 ReadMemoryWordTrampoline(CPU* cpu, uint32 segment, uint32 offset)
{
  return cpu->ReadMemoryWord(static_cast<Segment>(segment), offset);
}

static uint32 ReadMemoryDWordTrampoline(CPU* cpu, uint32 segment, uint32 offset)
{
  return cpu->ReadMemoryDWord(static_cast<Segment>(segment), offset);
}

static void WriteMemoryByteTrampoline(CPU* cpu, uint32 segment, uint32 offset, uint8 value)
{
  cpu->WriteMemoryByte(static_cast<Segment>(segment), offset, value);
}

static void WriteMemoryWordTrampoline(CPU* cpu, uint32 segment, uint32 offset, uint16 value)
{
  cpu->WriteMemoryWord(static_cast<Segment>(segment), offset, value);
}

static void WriteMemoryDWordTrampoline(CPU* cpu, uint32 segment, uint32 offset, uint32 value)
{
  cpu->WriteMemoryDWord(static_cast<Segment>(segment), offset, value);
}

void JitX64CodeGenerator::ReadOperand(const Instruction* instruction, size_t index, const Xbyak::Reg& dest,
                                      bool sign_extend)
{
  const Instruction::Operand* operand = &instruction->operands[index];
  OperandSize output_size;
  if (dest.isBit(8))
    output_size = OperandSize_8;
  else if (dest.isBit(16))
    output_size = OperandSize_16;
  else
    output_size = OperandSize_32;

  auto MakeRegisterAccess = [&](uint32 reg) {
    switch (output_size)
    {
      case OperandSize_8:
        mov(dest, byte[RCPUPTR + CalculateRegisterOffset(Reg8(reg))]);
        break;

      case OperandSize_16:
      {
        switch (operand->size)
        {
          case OperandSize_8:
          {
            if (sign_extend)
              movsx(dest, byte[RCPUPTR + CalculateRegisterOffset(Reg8(reg))]);
            else
              movzx(dest, byte[RCPUPTR + CalculateRegisterOffset(Reg8(reg))]);
          }
          break;
          case OperandSize_16:
          case OperandSize_32:
            mov(dest, word[RCPUPTR + CalculateRegisterOffset(Reg16(reg))]);
            break;
        }
      }
      break;

      case OperandSize_32:
      {
        switch (operand->size)
        {
          case OperandSize_8:
          {
            if (sign_extend)
              movsx(dest, byte[RCPUPTR + CalculateRegisterOffset(Reg8(reg))]);
            else
              movzx(dest, byte[RCPUPTR + CalculateRegisterOffset(Reg8(reg))]);
          }
          break;
          case OperandSize_16:
          {
            if (sign_extend)
              movsx(dest, word[RCPUPTR + CalculateRegisterOffset(Reg16(reg))]);
            else
              movzx(dest, word[RCPUPTR + CalculateRegisterOffset(Reg16(reg))]);
          }
          break;
          case OperandSize_32:
            mov(dest, dword[RCPUPTR + CalculateRegisterOffset(Reg32(reg))]);
            break;
        }
      }
      break;
    }
  };

  switch (operand->mode)
  {
    case OperandMode_Immediate:
    {
      switch (output_size)
      {
        case OperandSize_8:
          mov(dest, ZeroExtend32(instruction->data.imm8));
          break;
        case OperandSize_16:
        {
          switch (operand->size)
          {
            case OperandSize_8:
              mov(dest, sign_extend ? SignExtend32(instruction->data.imm8) : ZeroExtend32(instruction->data.imm8));
              break;
            default:
              mov(dest, ZeroExtend32(instruction->data.imm16));
              break;
          }
        }
        break;
        case OperandSize_32:
        {
          switch (operand->size)
          {
            case OperandSize_8:
              mov(dest, sign_extend ? SignExtend32(instruction->data.imm8) : ZeroExtend32(instruction->data.imm8));
              break;
            case OperandSize_16:
              mov(dest, sign_extend ? SignExtend32(instruction->data.imm16) : ZeroExtend32(instruction->data.imm16));
              break;
            default:
              mov(dest, ZeroExtend32(instruction->data.imm32));
              break;
          }
        }
        break;
      }
    }
    break;

    case OperandMode_Register:
      MakeRegisterAccess(operand->reg32);
      break;

    case OperandMode_SegmentRegister:
    {
      switch (output_size)
      {
        case OperandSize_16:
          mov(dest, word[RCPUPTR + CalculateSegmentRegisterOffset(operand->segreg)]);
          break;
        case OperandSize_32:
          // Segment registers are sign-extended on push/pop.
          movsx(dest, word[RCPUPTR + CalculateSegmentRegisterOffset(operand->segreg)]);
          break;
      }
    }
    break;

    case OperandMode_Memory:
    case OperandMode_ModRM_RM:
    {
      if (operand->mode == OperandMode_ModRM_RM && instruction->ModRM_RM_IsReg())
      {
        MakeRegisterAccess(instruction->data.modrm_rm_register);
        break;
      }

      mov(RPARAM1_64, RCPUPTR);
      mov(RPARAM2_32, uint32(instruction->GetMemorySegment()));
      if (operand->mode == OperandMode_Memory)
        mov(RPARAM3_32, instruction->data.disp32);
      else
        mov(RPARAM3_32, READDR32);

      switch (operand->size)
      {
        case OperandSize_8:
          CallModuleFunction(ReadMemoryByteTrampoline);
          break;
        case OperandSize_16:
          CallModuleFunction(ReadMemoryWordTrampoline);
          break;
        case OperandSize_32:
          CallModuleFunction(ReadMemoryDWordTrampoline);
          break;
      }

      switch (output_size)
      {
        case OperandSize_8:
          mov(dest, RRET_8);
          break;
        case OperandSize_16:
        {
          switch (operand->size)
          {
            case OperandSize_8:
            {
              if (sign_extend)
                movsx(dest, RRET_8);
              else
                movzx(dest, RRET_8);
            }
            break;
            case OperandSize_16:
            case OperandSize_32:
              mov(dest, RRET_16);
              break;
          }
        }
        break;
        case OperandSize_32:
        {
          switch (operand->size)
          {
            case OperandSize_8:
            {
              if (sign_extend)
                movsx(dest, RRET_8);
              else
                movzx(dest, RRET_8);
            }
            break;
            case OperandSize_16:
            {
              if (sign_extend)
                movsx(dest, RRET_16);
              else
                movzx(dest, RRET_16);
            }
            break;
            case OperandSize_32:
              mov(dest, RRET_32);
              break;
          }
        }
        break;
      }
    }
    break;

    default:
      Panic("Unhandled address mode");
      break;
  }
}

void JitX64CodeGenerator::WriteOperand(const Instruction* instruction, size_t index, const Xbyak::Reg& src)
{
#if 0
  const Instruction::Operand* operand = &instruction->operands[index];
  switch (operand->mode)
  {
    case AddressingMode_Register:
    {
      switch (operand->size)
      {
        case OperandSize_8:
          mov(byte[RCPUPTR + CalculateRegisterOffset(operand->reg.reg8)], src);
          break;
        case OperandSize_16:
          mov(word[RCPUPTR + CalculateRegisterOffset(operand->reg.reg16)], src);
          break;
        case OperandSize_32:
          mov(dword[RCPUPTR + CalculateRegisterOffset(operand->reg.reg32)], src);
          break;
      }
    }
    break;

    case AddressingMode_SegmentRegister:
    {
      // Truncate higher lengths to 16-bits.
      mov(RPARAM1_64, RCPUPTR);
      mov(RPARAM2_32, uint32(instruction->operands[0].reg.sreg));
      movzx(RPARAM3_32, (src.isBit(16)) ? src : src.changeBit(16));
      CallModuleFunction(LoadSegmentRegisterTrampoline);
    }
    break;

    case AddressingMode_Direct:
    case AddressingMode_RegisterIndirect:
    case AddressingMode_Indexed:
    case AddressingMode_BasedIndexed:
    case AddressingMode_BasedIndexedDisplacement:
    case AddressingMode_SIB:
    {
      mov(RPARAM1_64, RCPUPTR);
      mov(RPARAM2_32, uint32(instruction->segment));
      if (operand->mode == AddressingMode_Direct)
        mov(RPARAM3_32, operand->direct.address);
      else
        mov(RPARAM3_32, READDR32);

      switch (operand->size)
      {
        case OperandSize_8:
          movzx(RPARAM4_32, src);
          CallModuleFunction(WriteMemoryByteTrampoline);
          break;
        case OperandSize_16:
          movzx(RPARAM4_32, src);
          CallModuleFunction(WriteMemoryWordTrampoline);
          break;
        case OperandSize_32:
          mov(RPARAM4_32, src);
          CallModuleFunction(WriteMemoryDWordTrampoline);
          break;
      }
    }
    break;

    default:
      Panic("Unhandled address mode");
      break;
  }
#endif
}

void JitX64CodeGenerator::ReadFarAddressOperand(const Instruction* instruction, size_t index,
                                                const Xbyak::Reg& dest_segment, const Xbyak::Reg& dest_offset)
{
#if 0
  const Instruction::Operand* operand = &instruction->operands[index];
  if (operand->mode == AddressingMode_FarAddress)
  {
    mov(dest_segment, ZeroExtend32(operand->far_address.segment_selector));
    mov(dest_offset, operand->far_address.address);
    return;
  }

  // TODO: Should READDR32+2 wrap at FFFF?
  if (instruction->operand_size == OperandSize_16)
  {
    mov(RPARAM1_64, RCPUPTR);
    mov(RPARAM2_32, uint32(instruction->segment));
    if (operand->mode == AddressingMode_Direct)
      mov(RPARAM3_32, operand->direct.address);
    else
      mov(RPARAM3_32, READDR32);
    CallModuleFunction(ReadMemoryWordTrampoline);
    movzx(dest_offset, RRET_16);

    mov(RPARAM1_64, RCPUPTR);
    mov(RPARAM2_32, uint32(instruction->segment));
    if (operand->mode == AddressingMode_Direct)
      mov(RPARAM3_32, operand->direct.address + 2);
    else
      lea(RPARAM3_32, word[READDR32 + 2]);
    CallModuleFunction(ReadMemoryWordTrampoline);
    mov(dest_segment, RRET_16);
  }
  else
  {
    mov(RPARAM1_64, RCPUPTR);
    mov(RPARAM2_32, uint32(instruction->segment));
    if (operand->mode == AddressingMode_Direct)
      mov(RPARAM3_32, operand->direct.address);
    else
      mov(RPARAM3_32, READDR32);
    CallModuleFunction(ReadMemoryDWordTrampoline);
    mov(dest_offset, RRET_32);

    mov(RPARAM1_64, RCPUPTR);
    mov(RPARAM2_32, uint32(instruction->segment));
    if (operand->mode == AddressingMode_Direct)
      mov(RPARAM3_32, operand->direct.address + 4);
    else
      lea(RPARAM3_32, dword[READDR32 + 4]);
    CallModuleFunction(ReadMemoryWordTrampoline);
    mov(dest_segment, RRET_16);
  }
#endif
}

void JitX64CodeGenerator::UpdateFlags(uint32 clear_mask, uint32 set_mask, uint32 host_mask)
{
  // Shouldn't be clearing/setting any bits we're also getting from the host.
  DebugAssert((host_mask & clear_mask) == 0 && (host_mask & set_mask) == 0);

  // Clear the bits from the host too, since we set them later.
  clear_mask |= host_mask;

  // We need to grab the flags from the host first, before we do anything that'll lose the contents.
  // TODO: Check cpuid for LAHF support
  uint32 supported_flags = Flag_CF | Flag_PF | Flag_AF | Flag_ZF | Flag_SF;
  bool uses_high_flags = ((host_mask & ~supported_flags) != 0);
  bool use_eflags = ((host_mask & UINT32_C(0xFFFF0000)) != 0);
  bool use_lahf = !uses_high_flags;
  if (host_mask != 0)
  {
    if (use_lahf)
    {
      // Fast path via LAHF
      lahf();
    }
    else
    {
      pushf();
      pop(rax);
    }
  }

  // Clear bits.
  if (clear_mask != 0)
  {
    if ((clear_mask & UINT32_C(0xFFFF0000)) != 0)
      and(dword[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ~clear_mask);
    else
      and(word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ~Truncate16(clear_mask));
  }

  // Set bits.
  if (set_mask != 0)
  {
    if ((set_mask & UINT32_C(0xFFFF0000)) != 0)
      or (dword[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], set_mask);
    else
      or (word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], Truncate16(set_mask));
  }

  // Copy bits from host (cached in eax/ax/ah).
  if (host_mask != 0)
  {
    if (use_lahf)
    {
      and(ah, Truncate8(host_mask));
      or (byte[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ah);
    }
    else if (use_eflags)
    {
      and(eax, host_mask);
      or (dword[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], eax);
    }
    else
    {
      and(ax, Truncate16(host_mask));
      or (word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ax);
    }
  }
}

inline bool OperandIsESP(const Instruction* instruction, const Instruction::Operand& operand)
{
  // If any instructions manipulate ESP, we need to update the shadow variable for the next instruction.
  if (operand.size <= OperandSize_8)
    return false;

  return (operand.mode == OperandMode_Register && operand.reg32 == Reg32_ESP) ||
         (operand.mode == OperandMode_ModRM_Reg && instruction->GetModRM_Reg() == Reg32_ESP) ||
         (operand.mode == OperandMode_ModRM_RM && instruction->ModRM_RM_IsReg() &&
          instruction->data.modrm_rm_register == Reg32_ESP);
}

inline bool CanInstructionFault(const Instruction* instruction)
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
        if (!instruction->IsRegisterOperand(i) && instruction->operands[i].mode != OperandMode_Immediate)
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
      return (!instruction->IsRegisterOperand(0) && instruction->operands[0].mode != OperandMode_Immediate);
    }

    default:
      return true;
  }
}

void JitX64CodeGenerator::SyncInstructionPointers(const Instruction* next_instruction)
{
  if (next_instruction->GetAddressSize() == AddressSize_16)
  {
    if (m_delayed_eip_add > 1)
    {
      add(word[RCPUPTR + offsetof(CPU, m_registers.EIP)], m_delayed_eip_add);
      add(word[RCPUPTR + offsetof(CPU, m_current_EIP)], m_delayed_eip_add);
    }
    else if (m_delayed_eip_add == 1)
    {
      inc(word[RCPUPTR + offsetof(CPU, m_registers.EIP)]);
      inc(word[RCPUPTR + offsetof(CPU, m_current_EIP)]);
    }
  }
  else
  {
    if (m_delayed_eip_add > 1)
    {
      add(dword[RCPUPTR + offsetof(CPU, m_registers.EIP)], m_delayed_eip_add);
      add(dword[RCPUPTR + offsetof(CPU, m_current_EIP)], m_delayed_eip_add);
    }
    else if (m_delayed_eip_add == 1)
    {
      inc(dword[RCPUPTR + offsetof(CPU, m_registers.EIP)]);
      inc(dword[RCPUPTR + offsetof(CPU, m_current_EIP)]);
    }
  }
  m_delayed_eip_add = 0;

  if (m_delayed_cycles_add > 1)
    add(qword[RCPUPTR + offsetof(CPU, m_pending_cycles)], m_delayed_cycles_add);
  else if (m_delayed_cycles_add == 1)
    inc(qword[RCPUPTR + offsetof(CPU, m_pending_cycles)]);
  m_delayed_cycles_add = 0;
}

void JitX64CodeGenerator::StartInstruction(const Instruction* instruction)
{
#ifndef Y_BUILD_CONFIG_RELEASE
  nop();
#endif

  if (!CanInstructionFault(instruction))
  {
    // Defer updates for non-faulting instructions.
    m_delayed_eip_add += instruction->length;
    m_delayed_cycles_add++;
    return;
  }

  // Update EIP to point to the next instruction.
  uint32 inst_len = instruction->length + m_delayed_eip_add;
  if (m_cpu->m_current_address_size == AddressSize_16)
  {
    // Add pending EndInstruction(), since we clear delayed_eip_add
    if (m_delayed_eip_add > 1)
      add(word[RCPUPTR + offsetof(CPU, m_current_EIP)], m_delayed_eip_add);
    else if (m_delayed_eip_add == 1)
      inc(word[RCPUPTR + offsetof(CPU, m_current_EIP)]);

    if (inst_len > 1)
      add(word[RCPUPTR + offsetof(CPU, m_registers.EIP)], inst_len);
    else
      inc(word[RCPUPTR + offsetof(CPU, m_registers.EIP)]);
  }
  else
  {
    // Add pending EndInstruction(), since we clear delayed_eip_add
    if (m_delayed_eip_add > 1)
      add(dword[RCPUPTR + offsetof(CPU, m_current_EIP)], m_delayed_eip_add);
    else if (m_delayed_eip_add == 1)
      inc(dword[RCPUPTR + offsetof(CPU, m_current_EIP)]);

    if (inst_len > 1)
      add(dword[RCPUPTR + offsetof(CPU, m_registers.EIP)], inst_len);
    else
      inc(dword[RCPUPTR + offsetof(CPU, m_registers.EIP)]);
  }
  m_delayed_eip_add = 0;

  // Add pending cycles for this instruction.
  uint32 cycles = m_delayed_cycles_add + 1;
  if (cycles > 1)
    add(qword[RCPUPTR + offsetof(CPU, m_pending_cycles)], cycles);
  else
    inc(qword[RCPUPTR + offsetof(CPU, m_pending_cycles)]);
  m_delayed_cycles_add = 0;
}

void JitX64CodeGenerator::EndInstruction(const Instruction* instruction, bool update_eip, bool update_esp)
{
  if (CanInstructionFault(instruction))
  {
    // Update EIP after instruction completes, ready for the next.
    // This way it points to the next instruction while it's executing.
    if (update_eip)
    {
      if (m_cpu->m_current_address_size == AddressSize_16)
      {
        if (instruction->length > 1)
          add(word[RCPUPTR + offsetof(CPU, m_current_EIP)], instruction->length);
        else
          inc(word[RCPUPTR + offsetof(CPU, m_current_EIP)]);
      }
      else
      {
        if (instruction->length > 1)
          add(dword[RCPUPTR + offsetof(CPU, m_current_EIP)], instruction->length);
        else
          inc(dword[RCPUPTR + offsetof(CPU, m_current_EIP)]);
      }
    }
  }

  // If this instruction uses the stack, we need to update m_current_ESP for the next instruction.
  if (update_esp)
  {
    mov(RTEMP32A, dword[RCPUPTR + offsetof(CPU, m_registers.ESP)]);
    mov(dword[RCPUPTR + offsetof(CPU, m_current_ESP)], RTEMP32A);
  }

#ifndef Y_BUILD_CONFIG_RELEASE
  nop();
#endif
}

bool JitX64CodeGenerator::Compile_NOP(const Instruction* instruction)
{
  StartInstruction(instruction);
  EndInstruction(instruction);
  return true;
}

#if 0

bool JitX64CodeGenerator::Compile_LEA(const Instruction* instruction)
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

bool JitX64CodeGenerator::Compile_MOV(const Instruction* instruction)
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

bool JitX64CodeGenerator::Compile_MOV_Extended(const Instruction* instruction)
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

bool JitX64CodeGenerator::Compile_ALU_Binary_Update(const Instruction* instruction)
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

bool JitX64CodeGenerator::Compile_ALU_Binary_Test(const Instruction* instruction)
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

bool JitX64CodeGenerator::Compile_ALU_Unary_Update(const Instruction* instruction)
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

bool JitX64CodeGenerator::Compile_ShiftRotate(const Instruction* instruction)
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

bool JitX64CodeGenerator::Compile_DoublePrecisionShift(const Instruction* instruction)
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

#endif

// Necessary due to BranchTo being a member function.
void JitX64CodeGenerator::BranchToTrampoline(CPU* cpu, uint32 address)
{
  cpu->BranchTo(address);
}

void JitX64CodeGenerator::PushWordTrampoline(CPU* cpu, uint16 value)
{
  cpu->PushWord(value);
}

void JitX64CodeGenerator::PushDWordTrampoline(CPU* cpu, uint32 value)
{
  cpu->PushDWord(value);
}

uint16 JitX64CodeGenerator::PopWordTrampoline(CPU* cpu)
{
  return cpu->PopWord();
}

uint32 JitX64CodeGenerator::PopDWordTrampoline(CPU* cpu)
{
  return cpu->PopDWord();
}

void JitX64CodeGenerator::LoadSegmentRegisterTrampoline(CPU* cpu, uint32 segment, uint16 value)
{
  cpu->LoadSegmentRegister(static_cast<Segment>(segment), value);
}

void JitX64CodeGenerator::RaiseExceptionTrampoline(CPU* cpu, uint32 interrupt, uint32 error_code)
{
  cpu->RaiseException(interrupt, error_code);
}

void JitX64CodeGenerator::SetFlagsTrampoline(CPU* cpu, uint32 flags)
{
  cpu->SetFlags(flags);
}

void JitX64CodeGenerator::FarJumpTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size)
{
  cpu->FarJump(segment_selector, offset, static_cast<OperandSize>(op_size));
}

void JitX64CodeGenerator::FarCallTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size)
{
  cpu->FarCall(segment_selector, offset, static_cast<OperandSize>(op_size));
}

void JitX64CodeGenerator::FarReturnTrampoline(CPU* cpu, uint32 op_size, uint32 pop_count)
{
  cpu->FarReturn(static_cast<OperandSize>(op_size), pop_count);
}

bool JitX64CodeGenerator::Compile_JumpConditional(const Instruction* instruction)
{
  StartInstruction(instruction);

  Xbyak::Label test_pass_label;
  Xbyak::Label test_fail_label;

  // LOOP should also test ECX.
  if (instruction->operation == Operation_LOOP)
  {
    if (instruction->GetAddressSize() == AddressSize_16)
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
  switch (instruction->operands[0].jump_condition)
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
      if (instruction->GetAddressSize() == AddressSize_16)
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
  if (instruction->GetOperandSize() == OperandSize_16)
  {
    // Should be shorter than mov+add+and.
    mov(RPARAM2_16, word[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
    add(RPARAM2_16, uint32(instruction->data.disp16));
    movzx(RPARAM2_32, RPARAM2_16);
  }
  else
  {
    mov(RPARAM2_32, dword[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
    add(RPARAM2_32, uint32(instruction->data.disp32));
  }

  // m_current_EIP will not be correct here, but we will be at the end of the block anyway.
  CallModuleFunction(BranchToTrampoline);

  // Jump fail branch.
  L(test_fail_label);

  // No need to update EIP/ESP, end of block.
  EndInstruction(instruction, false, false);
  return true;
}

bool JitX64CodeGenerator::Compile_JumpCallReturn(const Instruction* instruction)
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
      mov(RPARAM4_32, static_cast<uint32>(instruction->GetOperandSize()));
      CallModuleFunction((instruction->operation == Operation_JMP_Far) ? FarJumpTrampoline : FarCallTrampoline);
    }
    break;

    case Operation_RET_Far:
    {
      // Far return also goes through a slow path.
      if (instruction->operands[0].mode != OperandMode_None)
        ReadOperand(instruction, 0, RPARAM3_32, false);
      else
        xor(RPARAM3_32, RPARAM3_32);
      mov(RPARAM2_32, static_cast<uint32>(instruction->GetOperandSize()));
      mov(RPARAM1_64, RCPUPTR);
      CallModuleFunction(FarReturnTrampoline);
    }
    break;

    case Operation_JMP_Near:
    {
      const Instruction::Operand* operand = &instruction->operands[0];
      if (operand->mode == OperandMode_Relative)
      {
        if (instruction->GetOperandSize() == OperandSize_16)
        {
          // Should be shorter than mov+add+and.
          mov(RPARAM2_16, word[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
          add(RPARAM2_16, uint32(instruction->data.disp16));
          movzx(RPARAM2_32, RPARAM2_16);
        }
        else
        {
          mov(RPARAM2_32, dword[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
          add(RPARAM2_32, uint32(instruction->data.disp32));
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
      const Instruction::Operand* operand = &instruction->operands[0];
      if (operand->mode == OperandMode_Relative)
      {
        if (instruction->GetOperandSize() == OperandSize_16)
        {
          mov(RSTORE16A, word[RCPUPTR + CalculateRegisterOffset(Reg32_EIP)]);
          mov(RPARAM1_64, RCPUPTR);
          movzx(RPARAM2_32, RSTORE16A);
          CallModuleFunction(PushWordTrampoline);
          add(RSTORE16A, uint32(instruction->data.disp16));
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
          add(RSTORE32A, uint32(instruction->data.disp32));
          mov(RPARAM1_64, RCPUPTR);
          mov(RPARAM2_32, RSTORE32A);
          CallModuleFunction(BranchToTrampoline);
        }
      }
      else
      {
        // Non-relative.
        if (instruction->GetOperandSize() == OperandSize_16)
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
      if (instruction->GetOperandSize() == OperandSize_16)
      {
        CallModuleFunction(PopWordTrampoline);
        movzx(RPARAM2_32, RRET_16);
      }
      else
      {
        CallModuleFunction(PopDWordTrampoline);
        mov(RPARAM2_32, RRET_32);
      }

      const Instruction::Operand* operand = &instruction->operands[0];
      Assert(operand->mode == OperandMode_None || operand->mode == OperandMode_Immediate);
      if (operand->mode == OperandMode_Immediate)
      {
        if (m_cpu->m_stack_address_size == AddressSize_16)
          add(word[RCPUPTR + CalculateRegisterOffset(Reg32_ESP)], instruction->data.imm32);
        else
          add(dword[RCPUPTR + CalculateRegisterOffset(Reg32_ESP)], instruction->data.imm32);
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

bool JitX64CodeGenerator::Compile_Stack(const Instruction* instruction)
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
      if (instruction->GetOperandSize() == OperandSize_16)
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
      if (instruction->GetOperandSize() == OperandSize_16)
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
      if (instruction->GetOperandSize() == OperandSize_16)
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
      if (instruction->GetOperandSize() == OperandSize_16)
      {
        mov(RPARAM1_64, RCPUPTR);
        CallModuleFunction(PopWordTrampoline);
        movzx(RRET_32, RRET_16);
        mov(RPARAM2_32, dword[RCPUPTR + CalculateRegisterOffset(Reg32_EFLAGS)]);
        and(RPARAM2_32, UINT32_C(0xFFFF0000));
        or (RPARAM2_32, RRET_32);
        CallModuleFunction(SetFlagsTrampoline);
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

bool JitX64CodeGenerator::Compile_Flags(const Instruction* instruction)
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

void JitX64CodeGenerator::InterpretInstructionTrampoline(CPU* cpu, const Instruction* instruction)
{
  std::memcpy(&cpu->idata, &instruction->data, sizeof(cpu->idata));
  instruction->interpreter_handler(cpu);
}

bool JitX64CodeGenerator::Compile_Fallback(const Instruction* instruction)
{
  // REP instructions are always annoying.
  std::unique_ptr<Xbyak::Label> rep_label;
  if (instruction->data.has_rep & InstructionFlag_Rep)
  {
    SyncInstructionPointers(instruction);
    rep_label = std::make_unique<Xbyak::Label>();
    L(*rep_label);
  }

  StartInstruction(instruction);

  // Xbyak::Label blah;
  // mov(RTEMP32A, dword[RCPUPTR + offsetof(CPU, m_current_ESP)]);
  // mov(RTEMP32B, dword[RCPUPTR + offsetof(CPU, m_registers.ESP)]);
  // cmp(RTEMP32A, RTEMP32B);
  // je(blah);
  // db(0xcc);
  // L(blah);

  mov(RPARAM1_64, RCPUPTR);
  mov(RPARAM2_64, reinterpret_cast<size_t>(instruction));
  CallModuleFunction(InterpretInstructionTrampoline);

  if (instruction->data.has_rep & InstructionFlag_Rep)
  {
    mov(RTEMP32A, dword[RCPUPTR + offsetof(CPU, m_current_EIP)]);
    mov(RTEMP32B, dword[RCPUPTR + offsetof(CPU, m_registers.EIP)]);
    cmp(RTEMP32A, RTEMP32B);
    je(*rep_label);
  }

  // Assume any instruction can manipulate ESP.
  EndInstruction(instruction, true, true);
  return true;
}
} // namespace CPU_X86
