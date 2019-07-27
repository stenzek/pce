#include "recompiler_code_generator.h"
#include "../system.h"
#include "YBaseLib/Log.h"
#include "debugger_interface.h"
#include "interpreter.h"
Log_SetChannel(CPU_X86::Recompiler);

namespace CPU_X86::Recompiler {

// TODO:
// Constant operands - don't move to a temporary register first
// Only sync current_ESP at the start of the block, and on push/pop instructions
// Threaded code generator?
// Push functions per stack address mode
// Only sync EIP on potentially-faulting instructions
// Lazy flag calculation - store operands and opcode
// TODO: Block leaking on invalidation
// TODO: Remove physical references when block is destroyed
// TODO: block linking
// TODO: memcpy-like stuff from bus for validation

CodeGenerator::CodeGenerator(JitCodeBuffer* code_buffer)
  : m_code_buffer(code_buffer), m_register_cache(*this),
    m_emit(code_buffer->GetFreeCodeSpace(), code_buffer->GetFreeCodePointer())
{
  InitHostRegs();
}

CodeGenerator::~CodeGenerator() {}

u32 CodeGenerator::CalculateRegisterOffset(Reg8 reg)
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

u32 CodeGenerator::CalculateRegisterOffset(Reg16 reg)
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

u32 CodeGenerator::CalculateRegisterOffset(Reg32 reg)
{
  return uint32(offsetof(CPU, m_registers.reg32[0]) + (reg * sizeof(uint32)));
}

u32 CodeGenerator::CalculateSegmentRegisterOffset(Segment segment)
{
  return uint32(offsetof(CPU, m_registers.segment_selectors[0]) + (segment * sizeof(uint16)));
}

bool CodeGenerator::CompileBlock(const BlockBase* block, BlockFunctionType* out_function_ptr, size_t* out_code_size)
{
  // TODO: Align code buffer.

  m_block = block;
  m_block_start = block->instructions.data();
  m_block_end = block->instructions.data() + block->instructions.size();

  EmitBeginBlock();

  const Instruction* instruction = m_block_start;
  while (instruction != m_block_end)
  {
    if (!CompileInstruction(instruction))
    {
      m_block_end = nullptr;
      m_block_start = nullptr;
      m_block = nullptr;
      return false;
    }

    instruction++;
  }

  // Re-sync instruction pointers.
  SyncInstructionPointer();
  EmitEndBlock();

  FinalizeBlock(out_function_ptr, out_code_size);

  m_block_end = nullptr;
  m_block_start = nullptr;
  m_block = nullptr;
  return true;
}

bool CodeGenerator::CompileInstruction(const Instruction* instruction)
{
  bool result;
  switch (instruction->operation)
  {
    default:
      result = Compile_Fallback(instruction);
      break;
  }

  // release temporary effective addresses
  for (Value& value : m_operand_memory_addresses)
    value.Clear();

  return result;
}

Value CodeGenerator::ConvertValueSize(const Value& value, OperandSize size, bool sign_extend)
{
  // We should only be going up in size, not down..
  DebugAssert(size > value.size);

  if (value.IsConstant())
  {
    // compile-time conversion, woo!
    switch (size)
    {
      case OperandSize_8:
        return Value::FromConstantU8(value.constant_value & 0xFF);

      case OperandSize_16:
      {
        switch (value.size)
        {
          case OperandSize_8:
            return Value::FromConstantU16(sign_extend ? SignExtend16(Truncate8(value.constant_value)) :
                                                        ZeroExtend16(Truncate8(value.constant_value)));

          default:
            return Value::FromConstantU16(value.constant_value & 0xFFFF);
        }
      }
      break;

      case OperandSize_32:
      {
        switch (value.size)
        {
          case OperandSize_8:
            return Value::FromConstantU32(sign_extend ? SignExtend32(Truncate8(value.constant_value)) :
                                                        ZeroExtend32(Truncate8(value.constant_value)));
          case OperandSize_16:
            return Value::FromConstantU32(sign_extend ? SignExtend32(Truncate16(value.constant_value)) :
                                                        ZeroExtend32(Truncate16(value.constant_value)));
          default:
            break;
        }
      }
      break;

      default:
        break;
    }

    UnreachableCode();
    return Value{};
  }

  Value new_value = m_register_cache.AllocateScratch(size);
  if (sign_extend)
    EmitSignExtend(new_value.host_reg, size, value.host_reg, value.size);
  else
    EmitZeroExtend(new_value.host_reg, size, value.host_reg, value.size);
  return new_value;
}

void CodeGenerator::ConvertValueSizeInPlace(Value& value, OperandSize size, bool sign_extend)
{
  // We don't want to mess up the register cache value, so generate a new value if it's not scratch.
  if (value.IsConstant() || !value.IsScratch())
  {
    value = ConvertValueSize(value, size, sign_extend);
    return;
  }

  DebugAssert(value.IsInHostRegister() && value.IsScratch());
  if (sign_extend)
    EmitSignExtend(value.host_reg, size, value.host_reg, value.size);
  else
    EmitZeroExtend(value.host_reg, size, value.host_reg, value.size);

  value.size = size;
}

Value CodeGenerator::ReadOperand(const Instruction* instruction, size_t index, OperandSize output_size,
                                 bool sign_extend)
{
  const Instruction::Operand* operand = &instruction->operands[index];

  auto MakeRegisterAccess = [&](uint32 reg) -> Value {
    switch (operand->size)
    {
      case OperandSize_8:
      {
        Value val = m_register_cache.ReadGuestRegister(static_cast<Reg8>(reg));
        if (output_size != OperandSize_8)
          ConvertValueSize(val, output_size, sign_extend);

        return val;
      }

      case OperandSize_16:
      {
        Value val = m_register_cache.ReadGuestRegister(static_cast<Reg16>(reg));
        if (output_size != OperandSize_16)
          ConvertValueSize(val, output_size, sign_extend);

        return val;
      }

      case OperandSize_32:
      {
        Value val = m_register_cache.ReadGuestRegister(static_cast<Reg32>(reg));
        if (output_size != OperandSize_32)
          ConvertValueSize(val, output_size, sign_extend);

        return val;
      }

      default:
        Panic("Unhandled register size.");
        return Value{};
    }
  };

  switch (operand->mode)
  {
    case OperandMode_Immediate:
    {
      switch (output_size)
      {
        case OperandSize_8:
          DebugAssert(operand->size == OperandSize_8);
          return Value::FromConstantU8(instruction->data.imm8);

        case OperandSize_16:
        {
          switch (operand->size)
          {
            case OperandSize_8:
              return Value::FromConstantU16(sign_extend ? SignExtend16(instruction->data.imm8) :
                                                          ZeroExtend16(instruction->data.imm8));

            default:
              return Value::FromConstantU16(ZeroExtend16(instruction->data.imm16));
          }
        }
        break;
        case OperandSize_32:
        {
          switch (operand->size)
          {
            case OperandSize_8:
              return Value::FromConstantU32(sign_extend ? SignExtend32(instruction->data.imm8) :
                                                          ZeroExtend32(instruction->data.imm8));

            case OperandSize_16:
              return Value::FromConstantU32(sign_extend ? SignExtend32(instruction->data.imm16) :
                                                          ZeroExtend32(instruction->data.imm16));

            default:
              return Value::FromConstantU32(ZeroExtend32(instruction->data.imm32));
          }
        }
        break;
      }
    }
    break;

    case OperandMode_Register:
      return MakeRegisterAccess(operand->reg32);

    case OperandMode_SegmentRegister:
    {
      // slow path for this for now..
      Value value = m_register_cache.AllocateScratch(OperandSize_16);
      EmitLoadCPUStructField(value.host_reg, OperandSize_16, CalculateSegmentRegisterOffset(operand->segreg));
      if (output_size == OperandSize_32)
      {
        // Segment registers are sign-extended on push/pop.
        EmitSignExtend(value.host_reg, OperandSize_32, value.host_reg, OperandSize_16);
      }

      return value;
    }

    case OperandMode_Memory:
    case OperandMode_ModRM_RM:
    {
      if (operand->mode == OperandMode_ModRM_RM && instruction->ModRM_RM_IsReg())
        return MakeRegisterAccess(instruction->data.modrm_rm_register);

      // Memory loads can fault. Ditch all the cached registers so we don't need to push them.
      // The effective address should remain..
      m_register_cache.FlushAllGuestRegisters(false);

      // we get the result back in eax, which we can use as a temporary.
      Value val = m_register_cache.AllocateScratch(operand->size);
      EmitLoadGuestMemory(val.host_reg, operand->size, m_operand_memory_addresses[index],
                          instruction->GetMemorySegment());

      // handle sign-extension
      if (operand->size != output_size)
        ConvertValueSizeInPlace(val, output_size, sign_extend);

      return val;
    }

    default:
      break;
  }

  Panic("Unhandled address mode");
  return Value{};
}

void CodeGenerator::WriteOperand(const Instruction* instruction, size_t index, Value&& value)
{
  const Instruction::Operand* operand = &instruction->operands[index];
  switch (operand->mode)
  {
    case OperandMode_Register:
    {
      switch (operand->size)
      {
        case OperandSize_8:
          m_register_cache.WriteGuestRegister(operand->reg8, std::move(value));
          return;
        case OperandSize_16:
          m_register_cache.WriteGuestRegister(operand->reg16, std::move(value));
          return;
        case OperandSize_32:
          m_register_cache.WriteGuestRegister(operand->reg32, std::move(value));
          return;
        default:
          break;
      }
    }
    break;

    case OperandMode_Memory:
    case OperandMode_ModRM_RM:
    {
      if (operand->mode == OperandMode_ModRM_RM && instruction->ModRM_RM_IsReg())
      {
        switch (operand->size)
        {
          case OperandSize_8:
            m_register_cache.WriteGuestRegister(static_cast<Reg8>(instruction->data.modrm_rm_register),
                                                std::move(value));
            break;
          case OperandSize_16:
            m_register_cache.WriteGuestRegister(static_cast<Reg16>(instruction->data.modrm_rm_register),
                                                std::move(value));
            break;
          case OperandSize_32:
            m_register_cache.WriteGuestRegister(static_cast<Reg32>(instruction->data.modrm_rm_register),
                                                std::move(value));
            break;
          default:
            break;
        }
        return;
      }

      // Memory writes can fault. Ditch all cached registers before continuing.
      m_register_cache.FlushAllGuestRegisters(false);
      EmitStoreGuestMemory(value, m_operand_memory_addresses[index], instruction->GetMemorySegment());
      return;
    }
  }

  Panic("Unhandled operand mode");
}

void CodeGenerator::CalculateEffectiveAddress(const Instruction* instruction)
{
  for (size_t i = 0; i < countof(instruction->operands); i++)
    m_operand_memory_addresses[i] = CalculateOperandMemoryAddress(instruction, i);
}

Value CodeGenerator::CalculateOperandMemoryAddress(const Instruction* instruction, size_t index)
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
              xor (READDR32, READDR32);
            else
              mov(READDR32, uint32(operand->sib.displacement));
          }
        }
        break;
    }
  }
#endif
  return Value();
}

bool CodeGenerator::IsConstantOperand(const Instruction* instruction, size_t index)
{
  const Instruction::Operand* operand = &instruction->operands[index];
  return (operand->mode == OperandMode_Immediate);
}

uint32 CodeGenerator::GetConstantOperand(const Instruction* instruction, size_t index, bool sign_extend)
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

void CodeGenerator::EmitInstructionPrologue(const Instruction* instruction, bool force_sync /* = false */)
{
  if (!CanInstructionFault(instruction) && !force_sync)
  {
    // Defer updates for non-faulting instructions.
    m_delayed_eip_add += instruction->length;
    m_delayed_current_eip_add += instruction->length;
    m_delayed_cycles_add++;
    return;
  }

  // Update EIP to point to the next instruction.
  if (m_block->Is32BitCode())
  {
    if (m_delayed_current_eip_add > 0)
      EmitAddCPUStructField(offsetof(CPU, m_current_EIP), Value::FromConstantU32(m_delayed_current_eip_add));

    EmitAddCPUStructField(offsetof(CPU, m_registers.EIP),
                          Value::FromConstantU32(m_delayed_eip_add + instruction->length));
  }
  else
  {
    if (m_delayed_current_eip_add > 0)
    {
      EmitAddCPUStructField(offsetof(CPU, m_current_EIP), Value::FromConstantU16(Truncate16(m_delayed_current_eip_add)));
    }

    EmitAddCPUStructField(offsetof(CPU, m_registers.EIP),
                          Value::FromConstantU16(Truncate16(m_delayed_eip_add + instruction->length)));
  }

  // Delay the add to current_EIP until the next instruction.
  m_delayed_current_eip_add = instruction->length;

  // Add pending cycles for this instruction.
  EmitAddCPUStructField(offsetof(CPU, m_pending_cycles), Value::FromConstantU64(m_delayed_cycles_add + 1));
  m_delayed_cycles_add = 0;
}

void CodeGenerator::SyncInstructionPointer()
{
  if (m_delayed_eip_add > 0)
  {
    if (m_block->Is32BitCode())
    {
      EmitAddCPUStructField(offsetof(CPU, m_registers.EIP), Value::FromConstantU32(m_delayed_eip_add));
    }
    else
    {
      EmitAddCPUStructField(offsetof(CPU, m_registers.EIP), Value::FromConstantU16(Truncate16(m_delayed_eip_add)));
    }

    m_delayed_eip_add = 0;
  }

  if (m_delayed_cycles_add > 0)
  {
    EmitAddCPUStructField(offsetof(CPU, m_pending_cycles), Value::FromConstantU64(m_delayed_eip_add));
    m_delayed_cycles_add = 0;
  }
}

void CodeGenerator::SyncCurrentEIP()
{
  EmitStoreCPUStructField(offsetof(CPU, m_current_EIP), m_register_cache.ReadGuestRegister(Reg32_EIP, false));
}

void CodeGenerator::SyncCurrentESP()
{
  EmitStoreCPUStructField(offsetof(CPU, m_current_ESP), m_register_cache.ReadGuestRegister(Reg32_ESP, false));
}

bool CodeGenerator::Compile_Fallback(const Instruction* instruction)
{
  EmitInstructionPrologue(instruction, true);

  // flush and invalidate all guest registers, since the fallback could change any of them
  m_register_cache.FlushAllGuestRegisters(true);

  // set up the instruction data
  EmitStoreCPUStructField(offsetof(CPU, idata.bits64[0]), Value::FromConstantU64(instruction->data.bits64[0]));
  EmitStoreCPUStructField(offsetof(CPU, idata.bits64[1]), Value::FromConstantU64(instruction->data.bits64[1]));

  // emit the function call
  Interpreter::HandlerFunction handler = Interpreter::GetInterpreterHandlerForInstruction(instruction);
  DebugAssert(handler);
  EmitFunctionCall(handler, m_register_cache.GetCPUPtr());

  // assume any instruction can manipulate esp
  SyncCurrentESP();

  return true;
}

bool CodeGenerator::Compile_NOP(const Instruction* instruction)
{
  EmitInstructionPrologue(instruction);
  return true;
}

bool CodeGenerator::Compile_LEA(const Instruction* instruction)
{
  return false;
}

bool CodeGenerator::Compile_MOV(const Instruction* instruction)
{
  return false;
}

} // namespace CPU_X86::Recompiler
