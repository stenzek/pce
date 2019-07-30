#include "recompiler_code_generator.h"
#include "../system.h"
#include "YBaseLib/Log.h"
#include "debugger_interface.h"
#include "decoder.h"
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
// TODO: xor eax, eax -> invalidate and constant 0

CodeGenerator::CodeGenerator(CPU* cpu, JitCodeBuffer* code_buffer)
  : m_cpu(cpu), m_code_buffer(code_buffer), m_register_cache(*this),
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
#ifndef Y_BUILD_CONFIG_RELEASE
    SmallString disasm;
    Decoder::DisassembleToString(instruction, &disasm);
    Log_DebugPrintf("Compiling instruction '%08x: %s'", instruction->address, disasm.GetCharArray());
#endif

    if (!CompileInstruction(*instruction))
    {
      m_block_end = nullptr;
      m_block_start = nullptr;
      m_block = nullptr;
      return false;
    }

    instruction++;
  }

  // Re-sync instruction pointers.
  m_register_cache.FlushAllGuestRegisters(true);
  SyncInstructionPointer();
  EmitEndBlock();

  FinalizeBlock(out_function_ptr, out_code_size);

  DebugAssert(m_register_cache.GetUsedHostRegisters() == 0);

  m_block_end = nullptr;
  m_block_start = nullptr;
  m_block = nullptr;
  return true;
}

bool CodeGenerator::CompileInstruction(const Instruction& instruction)
{
  if (IsInvalidInstruction(instruction))
  {
    InstructionPrologue(instruction, true);
    RaiseException(Interrupt_InvalidOpcode);
    return true;
  }

  bool result;
  switch (instruction.operation)
  {
    case Operation_NOP:
      result = Compile_NOP(instruction);
      break;

    case Operation_LEA:
      result = Compile_LEA(instruction);
      break;

    case Operation_MOV:
      result = Compile_MOV(instruction);
      break;

    case Operation_AND:
    case Operation_OR:
    case Operation_XOR:
    case Operation_TEST:
      result = Compile_Bitwise(instruction);
      break;

    case Operation_NOT:
      result = Compile_NOT(instruction);
      break;

    case Operation_ADD:
    case Operation_SUB:
    case Operation_CMP:
      result = Compile_AddSub(instruction);
      break;

    case Operation_PUSH:
      result = Compile_PUSH(instruction);
      break;

    case Operation_POP:
      result = Compile_POP(instruction);
      break;

    default:
      result = Compile_Fallback(instruction);
      break;
  }

  // release temporary effective addresses
  for (Value& value : m_operand_memory_addresses)
    value.ReleaseAndClear();

  return result;
}

Value CodeGenerator::ConvertValueSize(const Value& value, OperandSize size, bool sign_extend)
{
  DebugAssert(value.size != size);

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

          case OperandSize_32:
            return value;

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
  if (size < value.size)
  {
    EmitCopyValue(new_value.host_reg, value);
  }
  else
  {
    if (sign_extend)
      EmitSignExtend(new_value.host_reg, size, value.host_reg, value.size);
    else
      EmitZeroExtend(new_value.host_reg, size, value.host_reg, value.size);
  }

  return new_value;
}

void CodeGenerator::ConvertValueSizeInPlace(Value* value, OperandSize size, bool sign_extend)
{
  DebugAssert(value->size != size);

  // We don't want to mess up the register cache value, so generate a new value if it's not scratch.
  if (value->IsConstant() || !value->IsScratch())
  {
    *value = ConvertValueSize(*value, size, sign_extend);
    return;
  }

  DebugAssert(value->IsInHostRegister() && value->IsScratch());

  // If the size is smaller and the value is in a register, we can just "view" the lower part.
  if (size < value->size)
  {
    value->size = size;
  }
  else
  {
    if (sign_extend)
      EmitSignExtend(value->host_reg, size, value->host_reg, value->size);
    else
      EmitZeroExtend(value->host_reg, size, value->host_reg, value->size);
  }

  value->size = size;
}

Value CodeGenerator::AddValues(const Value& lhs, const Value& rhs)
{
  DebugAssert(lhs.size == rhs.size);
  if (lhs.IsConstant() && rhs.IsConstant())
  {
    // compile-time
    u64 new_cv = lhs.constant_value + rhs.constant_value;
    switch (lhs.size)
    {
      case OperandSize_8:
        return Value::FromConstantU8(Truncate8(new_cv));

      case OperandSize_16:
        return Value::FromConstantU16(Truncate16(new_cv));

      case OperandSize_32:
        return Value::FromConstantU32(Truncate32(new_cv));

      case OperandSize_64:
        return Value::FromConstantU64(new_cv);

      default:
        return Value();
    }
  }

  Value res = m_register_cache.AllocateScratch(lhs.size);
  EmitCopyValue(res.host_reg, lhs);
  EmitAdd(res.host_reg, rhs);
  return res;
}

Value CodeGenerator::ShlValues(const Value& lhs, const Value& rhs)
{
  DebugAssert(lhs.size == rhs.size);
  if (lhs.IsConstant() && rhs.IsConstant())
  {
    // compile-time
    u64 new_cv = lhs.constant_value << rhs.constant_value;
    switch (lhs.size)
    {
      case OperandSize_8:
        return Value::FromConstantU8(Truncate8(new_cv));

      case OperandSize_16:
        return Value::FromConstantU16(Truncate16(new_cv));

      case OperandSize_32:
        return Value::FromConstantU32(Truncate32(new_cv));

      case OperandSize_64:
        return Value::FromConstantU64(new_cv);

      default:
        return Value();
    }
  }

  Value res = m_register_cache.AllocateScratch(lhs.size);
  EmitCopyValue(res.host_reg, lhs);
  EmitShl(res.host_reg, rhs);
  return res;
}

void CodeGenerator::UpdateEFLAGS(Value&& merge_value, u32 clear_flags_mask, u32 copy_flags_mask, u32 set_flags_mask)
{
  Value eflags = m_register_cache.ReadGuestRegister(Reg32_EFLAGS, true, true);

  const u32 bits_to_clear = clear_flags_mask | copy_flags_mask;
  if (bits_to_clear != 0)
    EmitAnd(eflags.GetHostRegister(), Value::FromConstantU32(~bits_to_clear));
  if (copy_flags_mask != 0)
  {
    if (merge_value.IsConstant())
    {
      EmitOr(eflags.GetHostRegister(),
             Value::FromConstantU32(Truncate32(merge_value.constant_value) & copy_flags_mask));
    }
    else
    {
      EmitAnd(merge_value.GetHostRegister(), Value::FromConstantU32(copy_flags_mask));
      EmitOr(eflags.GetHostRegister(), merge_value);
    }
  }
  if (set_flags_mask != 0)
    EmitOr(eflags.GetHostRegister(), Value::FromConstantU32(set_flags_mask));

  m_register_cache.WriteGuestRegister(Reg32_EFLAGS, std::move(eflags));
}

Value CodeGenerator::ReadOperand(const Instruction& instruction, size_t index, OperandSize output_size,
                                 bool sign_extend, bool force_host_register /* = false */)
{
  const Instruction::Operand* operand = &instruction.operands[index];

  auto MakeRegisterAccess = [&](uint32 reg) -> Value {
    switch (operand->size)
    {
      case OperandSize_8:
      {
        Value val = m_register_cache.ReadGuestRegister(static_cast<Reg8>(reg), true, force_host_register);
        if (output_size != OperandSize_8)
          val = ConvertValueSize(val, output_size, sign_extend);

        return val;
      }

      case OperandSize_16:
      {
        Value val = m_register_cache.ReadGuestRegister(static_cast<Reg16>(reg), true, force_host_register);
        if (output_size != OperandSize_16)
          val = ConvertValueSize(val, output_size, sign_extend);

        return val;
      }

      case OperandSize_32:
      {
        Value val = m_register_cache.ReadGuestRegister(static_cast<Reg32>(reg), true, force_host_register);
        if (output_size != OperandSize_32)
          val = ConvertValueSize(val, output_size, sign_extend);

        return val;
      }

      default:
        Panic("Unhandled register size.");
        return Value{};
    }
  };

  auto MakeMemoryAccess = [&]() -> Value {
    // we get the result back in eax, which we can use as a temporary.
    Value val = m_register_cache.AllocateScratch(operand->size);
    LoadSegmentMemory(&val, operand->size, m_operand_memory_addresses[index], instruction.GetMemorySegment());

    // handle sign-extension
    if (operand->size != output_size)
      ConvertValueSizeInPlace(&val, output_size, sign_extend);

    return val;
  };

  Value val;
  switch (operand->mode)
  {
    case OperandMode_Immediate:
    {
      switch (output_size)
      {
        case OperandSize_8:
          DebugAssert(operand->size == OperandSize_8);
          val = Value::FromConstantU8(instruction.data.imm8);
          break;

        case OperandSize_16:
        {
          switch (operand->size)
          {
            case OperandSize_8:
              val = Value::FromConstantU16(sign_extend ? SignExtend16(instruction.data.imm8) :
                                                         ZeroExtend16(instruction.data.imm8));
              break;

            default:
              val = Value::FromConstantU16(ZeroExtend16(instruction.data.imm16));
              break;
          }
        }
        break;
        case OperandSize_32:
        {
          switch (operand->size)
          {
            case OperandSize_8:
              val = Value::FromConstantU32(sign_extend ? SignExtend32(instruction.data.imm8) :
                                                         ZeroExtend32(instruction.data.imm8));
              break;

            case OperandSize_16:
              val = Value::FromConstantU32(sign_extend ? SignExtend32(instruction.data.imm16) :
                                                         ZeroExtend32(instruction.data.imm16));
              break;

            default:
              val = Value::FromConstantU32(ZeroExtend32(instruction.data.imm32));
              break;
          }
        }
        break;
      }
    }
    break;

    case OperandMode_Register:
      val = MakeRegisterAccess(operand->reg32);
      break;

    case OperandMode_SegmentRegister:
    {
      // slow path for this for now..
      val = m_register_cache.AllocateScratch(OperandSize_16);
      EmitLoadCPUStructField(val.host_reg, OperandSize_16, CalculateSegmentRegisterOffset(operand->segreg));
      if (output_size == OperandSize_32)
      {
        // Segment registers are sign-extended on push/pop.
        EmitSignExtend(val.host_reg, OperandSize_32, val.host_reg, OperandSize_16);
      }
    }
    break;

    case OperandMode_Memory:
      val = MakeMemoryAccess();
      break;

    case OperandMode_ModRM_Reg:
      val = MakeRegisterAccess(instruction.GetModRM_Reg());
      break;

    case OperandMode_ModRM_RM:
      val = instruction.ModRM_RM_IsReg() ? MakeRegisterAccess(instruction.GetModRM_RM_Reg()) : MakeMemoryAccess();
      break;

    default:
      Panic("Unhandled address mode");
      return Value{};
  }

  if (force_host_register && !val.IsInHostRegister())
  {
    Value temp = m_register_cache.AllocateScratch(val.size);
    EmitCopyValue(temp.host_reg, val);
    val = std::move(temp);
  }

  return val;
}

Value CodeGenerator::WriteOperand(const Instruction& instruction, size_t index, Value&& value)
{
  const Instruction::Operand* operand = &instruction.operands[index];
  auto MakeRegisterAccess = [&](u32 reg) {
    switch (operand->size)
    {
      case OperandSize_8:
        return m_register_cache.WriteGuestRegister(static_cast<Reg8>(reg), std::move(value));
      case OperandSize_16:
        return m_register_cache.WriteGuestRegister(static_cast<Reg16>(reg), std::move(value));
      case OperandSize_32:
        return m_register_cache.WriteGuestRegister(static_cast<Reg32>(reg), std::move(value));
      default:
        return Value{};
    }
  };

  auto MakeMemoryAccess = [&]() {
    StoreSegmentMemory(value, m_operand_memory_addresses[index], instruction.GetMemorySegment());
    return std::move(value);
  };

  switch (operand->mode)
  {
    case OperandMode_Register:
      return MakeRegisterAccess(static_cast<u32>(operand->reg32));

    case OperandMode_Memory:
      return MakeMemoryAccess();

    case OperandMode_ModRM_Reg:
      return MakeRegisterAccess(instruction.GetModRM_Reg());

    case OperandMode_ModRM_RM:
      return instruction.ModRM_RM_IsReg() ? MakeRegisterAccess(instruction.GetModRM_RM_Reg()) : MakeMemoryAccess();

    default:
      Panic("Unhandled operand mode");
      return Value{};
  }
}

void CodeGenerator::CalculateEffectiveAddress(const Instruction& instruction)
{
  for (size_t i = 0; i < countof(instruction.operands); i++)
  {
    m_operand_memory_addresses[i] = CalculateOperandMemoryAddress(instruction, i);
    if (m_operand_memory_addresses[i].IsValid())
    {
      if (m_operand_memory_addresses[i].IsInHostRegister())
      {
        Log_DebugPrintf("Effective address is stored in host register %s",
                        GetHostRegName(m_operand_memory_addresses[i].GetHostRegister()));
      }
      else
      {
        Log_DebugPrintf("Effective address is constant 0x%" PRIX64, m_operand_memory_addresses[i].constant_value);
      }

      if (m_operand_memory_addresses[i].size != OperandSize_32)
        ConvertValueSizeInPlace(&m_operand_memory_addresses[i], OperandSize_32, false);
    }
  }
}

Value CodeGenerator::CalculateOperandMemoryAddress(const Instruction& instruction, size_t index)
{
  const Instruction::Operand& operand = instruction.operands[index];
  switch (operand.mode)
  {
    case OperandMode_Memory:
    {
      return instruction.Is32BitAddressSize() ? Value::FromConstantU32(instruction.data.disp32) :
                                                Value::FromConstantU16(instruction.data.disp16);
    }

    case OperandMode_ModRM_RM:
    {
      const Decoder::ModRMAddress* modrm =
        Decoder::DecodeModRMAddress(instruction.GetAddressSize(), instruction.data.modrm);
      switch (modrm->addressing_mode)
      {
        case ModRMAddressingMode::Direct:
        {
          return instruction.Is32BitAddressSize() ? Value::FromConstantU32(instruction.data.disp32) :
                                                    Value::FromConstantU16(instruction.data.disp16);
        }

        case ModRMAddressingMode::Indirect:
        {
          return instruction.Is32BitAddressSize() ?
                   m_register_cache.ReadGuestRegister(static_cast<Reg32>(modrm->base_register)) :
                   m_register_cache.ReadGuestRegister(static_cast<Reg16>(modrm->base_register));
        }

        case ModRMAddressingMode::Indexed:
        {
          Value base_reg = instruction.Is32BitAddressSize() ?
                             m_register_cache.ReadGuestRegister(static_cast<Reg32>(modrm->base_register)) :
                             m_register_cache.ReadGuestRegister(static_cast<Reg16>(modrm->base_register));
          Value displacement = instruction.Is32BitAddressSize() ? Value::FromConstantU32(instruction.data.disp32) :
                                                                  Value::FromConstantU16(instruction.data.disp16);
          return AddValues(std::move(base_reg), std::move(displacement));
        }

        case ModRMAddressingMode::BasedIndexed:
        {
          Value base_reg = instruction.Is32BitAddressSize() ?
                             m_register_cache.ReadGuestRegister(static_cast<Reg32>(modrm->base_register)) :
                             m_register_cache.ReadGuestRegister(static_cast<Reg16>(modrm->base_register));
          Value index_reg = instruction.Is32BitAddressSize() ?
                              m_register_cache.ReadGuestRegister(static_cast<Reg32>(modrm->index_register)) :
                              m_register_cache.ReadGuestRegister(static_cast<Reg16>(modrm->index_register));
          return AddValues(std::move(base_reg), std::move(index_reg));
        }

        case ModRMAddressingMode::BasedIndexedDisplacement:
        {
          Value base_reg = instruction.Is32BitAddressSize() ?
                             m_register_cache.ReadGuestRegister(static_cast<Reg32>(modrm->base_register)) :
                             m_register_cache.ReadGuestRegister(static_cast<Reg16>(modrm->base_register));
          Value index_reg = instruction.Is32BitAddressSize() ?
                              m_register_cache.ReadGuestRegister(static_cast<Reg32>(modrm->index_register)) :
                              m_register_cache.ReadGuestRegister(static_cast<Reg16>(modrm->index_register));
          Value displacement = instruction.Is32BitAddressSize() ? Value::FromConstantU32(instruction.data.disp32) :
                                                                  Value::FromConstantU16(instruction.data.disp16);
          return AddValues(AddValues(std::move(base_reg), std::move(index_reg)), std::move(displacement));
        }
        break;

        case ModRMAddressingMode::SIB:
        {
          if (instruction.HasSIBBase())
          {
            Value val = m_register_cache.ReadGuestRegister(instruction.GetSIBBaseRegister());
            if (instruction.HasSIBIndex())
            {
              Value sib_index = m_register_cache.ReadGuestRegister(instruction.GetSIBIndexRegister());
              if (instruction.GetSIBScaling() != 0)
                sib_index = ShlValues(std::move(sib_index), Value::FromConstantU32(instruction.data.GetSIBScaling()));

              val = AddValues(std::move(val), std::move(sib_index));
            }
            if (instruction.data.disp32 != 0)
              val = AddValues(std::move(val), Value::FromConstantU32(instruction.data.disp32));

            return val;
          }
          else if (instruction.HasSIBIndex())
          {
            Value val = m_register_cache.ReadGuestRegister(instruction.GetSIBIndexRegister());
            if (instruction.GetSIBScaling() != 0)
              val = ShlValues(std::move(val), Value::FromConstantU32(instruction.data.GetSIBScaling()));

            if (instruction.data.disp32 != 0)
              val = AddValues(std::move(val), Value::FromConstantU32(instruction.data.disp32));

            return val;
          }
          else
          {
            return Value::FromConstantU32(instruction.data.disp32);
          }
        }
        break;

        case ModRMAddressingMode::Register:
        default:
          break;
      }
    }

    default:
      break;
  }

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

void CodeGenerator::LoadSegmentMemory(Value* dest_value, OperandSize size, const Value& address, Segment segment)
{
  DebugAssert(address.size == OperandSize_32);
  switch (size)
  {
    case OperandSize_8:
      EmitFunctionCall(dest_value, &Thunks::ReadSegmentMemoryByte, m_register_cache.GetCPUPtr(),
                       Value::FromConstantU32(static_cast<u32>(segment)), address);
      break;
    case OperandSize_16:
      EmitFunctionCall(dest_value, &Thunks::ReadSegmentMemoryWord, m_register_cache.GetCPUPtr(),
                       Value::FromConstantU32(static_cast<u32>(segment)), address);
      break;
    case OperandSize_32:
      EmitFunctionCall(dest_value, &Thunks::ReadSegmentMemoryDWord, m_register_cache.GetCPUPtr(),
                       Value::FromConstantU32(static_cast<u32>(segment)), address);
      break;
    default:
      break;
  }
}

void CodeGenerator::StoreSegmentMemory(const Value& value, const Value& address, Segment segment)
{
  if (value.IsConstant())
  {
    if (address.IsConstant())
    {
      Log_DebugPrintf("Store constant value %08X to constant address %08X", Truncate32(value.constant_value),
                      Truncate32(address.constant_value));
    }
    else
    {
      Log_DebugPrintf("Store constant value %08X to address in register %s", Truncate32(value.constant_value),
                      GetHostRegName(address.host_reg));
    }
  }
  else
  {
    if (address.IsConstant())
    {
      Log_DebugPrintf("Store register %s to constant address %08X", GetHostRegName(value.host_reg),
                      Truncate32(address.constant_value));
    }
    else
    {
      Log_DebugPrintf("Store register %s to address in register %s", GetHostRegName(value.host_reg),
                      GetHostRegName(address.host_reg));
    }
  }

  DebugAssert(address.size == OperandSize_32);
  switch (value.size)
  {
    case OperandSize_8:
      EmitFunctionCall(nullptr, &Thunks::WriteSegmentMemoryByte, m_register_cache.GetCPUPtr(),
                       Value::FromConstantU32(static_cast<u32>(segment)), address, value);
      break;
    case OperandSize_16:
      EmitFunctionCall(nullptr, &Thunks::WriteSegmentMemoryWord, m_register_cache.GetCPUPtr(),
                       Value::FromConstantU32(static_cast<u32>(segment)), address, value);
      break;
    case OperandSize_32:
      EmitFunctionCall(nullptr, &Thunks::WriteSegmentMemoryDWord, m_register_cache.GetCPUPtr(),
                       Value::FromConstantU32(static_cast<u32>(segment)), address, value);
      break;
    default:
      break;
  }
}

void CodeGenerator::RaiseException(u32 exception, const Value& ec /*= Value::FromConstantU32(0)*/)
{
  EmitFunctionCall(nullptr, &Thunks::RaiseException, m_register_cache.GetCPUPtr(), Value::FromConstantU32(exception),
                   ec);
}

void CodeGenerator::InstructionPrologue(const Instruction& instruction, CycleCount cycles,
                                        bool force_sync /* = false */)
{
  cycles++;

  if (!CanInstructionFault(&instruction) && !force_sync)
  {
    // Defer updates for non-faulting instructions.
    m_delayed_eip_add += instruction.length;
    m_delayed_current_eip_add += instruction.length;
    m_delayed_cycles_add += cycles;
    return;
  }

  // Update EIP to point to the next instruction.
  if (m_block->Is32BitCode())
  {
    if (m_delayed_current_eip_add > 0)
      EmitAddCPUStructField(offsetof(CPU, m_current_EIP), Value::FromConstantU32(m_delayed_current_eip_add));

    EmitAddCPUStructField(offsetof(CPU, m_registers.EIP),
                          Value::FromConstantU32(m_delayed_eip_add + instruction.length));
  }
  else
  {
    if (m_delayed_current_eip_add > 0)
    {
      EmitAddCPUStructField(offsetof(CPU, m_current_EIP),
                            Value::FromConstantU16(Truncate16(m_delayed_current_eip_add)));
    }

    EmitAddCPUStructField(offsetof(CPU, m_registers.EIP),
                          Value::FromConstantU16(Truncate16(m_delayed_eip_add + instruction.length)));
  }

  // Delay the add to current_EIP until the next instruction.
  m_delayed_eip_add = 0;
  m_delayed_current_eip_add = instruction.length;

  // Add pending cycles for this instruction.
  EmitAddCPUStructField(offsetof(CPU, m_pending_cycles), Value::FromConstantU64(m_delayed_cycles_add + cycles));
  m_delayed_cycles_add = 0;

  // Flush all registers from the last instruction if this instruction can fault.
  m_register_cache.FlushAllGuestRegisters(false);
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

static std::array<u64, Operation_Count> s_fallback_operation_instruction_counts;

bool CodeGenerator::Compile_Fallback(const Instruction& instruction)
{
  InstructionPrologue(instruction, 0, true);
  s_fallback_operation_instruction_counts[instruction.operation]++;

  // flush and invalidate all guest registers, since the fallback could change any of them
  m_register_cache.FlushAllGuestRegisters(true);

  // set up the instruction data
  EmitStoreCPUStructField(offsetof(CPU, idata.bits64[0]), Value::FromConstantU64(instruction.data.bits64[0]));
  EmitStoreCPUStructField(offsetof(CPU, idata.bits64[1]), Value::FromConstantU64(instruction.data.bits64[1]));

  // emit the function call
  Interpreter::HandlerFunction handler = Interpreter::GetInterpreterHandlerForInstruction(&instruction);
  DebugAssert(handler);
  EmitFunctionCall(nullptr, handler, m_register_cache.GetCPUPtr());

  // assume any instruction can manipulate esp
  SyncCurrentESP();

  return true;
}

bool CodeGenerator::Compile_NOP(const Instruction& instruction)
{
  InstructionPrologue(instruction, m_cpu->GetCycles(CYCLES_NOP));
  return true;
}

bool CodeGenerator::Compile_LEA(const Instruction& instruction)
{
  InstructionPrologue(instruction, m_cpu->GetCycles(CYCLES_LEA));
  CalculateEffectiveAddress(instruction);

  // 16-bit leas need conversion
  if (m_operand_memory_addresses[1].size != instruction.operands[0].size)
    ConvertValueSizeInPlace(&m_operand_memory_addresses[1], instruction.operands[0].size, false);

  WriteOperand(instruction, 0, std::move(m_operand_memory_addresses[1]));
  return true;
}

bool CodeGenerator::Compile_MOV(const Instruction& instruction)
{
  DebugAssert(instruction.operands[0].size == instruction.operands[1].size);

  CycleCount cycles = 0;
  if (instruction.DestinationMode() == OperandMode_Register && instruction.SourceMode() == OperandMode_Immediate)
    cycles = m_cpu->GetCycles(CYCLES_MOV_REG_IMM);
  else if (instruction.DestinationMode() == OperandMode_Register && instruction.SourceMode() == OperandMode_Memory)
    cycles = m_cpu->GetCycles(CYCLES_MOV_REG_MEM);
  else if (instruction.DestinationMode() == OperandMode_Memory && instruction.SourceMode() == OperandMode_Register)
    cycles = m_cpu->GetCycles(CYCLES_MOV_RM_MEM_REG);
  else if (instruction.DestinationMode() == OperandMode_ModRM_RM)
    cycles = m_cpu->GetCyclesRM(CYCLES_MOV_RM_MEM_REG, instruction.ModRM_RM_IsReg());
  else if (instruction.SourceMode() == OperandMode_ModRM_RM)
    cycles = m_cpu->GetCyclesRM(CYCLES_MOV_REG_RM_MEM, instruction.ModRM_RM_IsReg());

  InstructionPrologue(instruction, cycles);
  CalculateEffectiveAddress(instruction);
  WriteOperand(instruction, 0, ReadOperand(instruction, 1, instruction.operands[1].size, false));

  if (OperandIsESP(instruction, 0))
    SyncCurrentESP();

  return true;
}

bool CodeGenerator::Compile_Bitwise(const Instruction& instruction)
{
  const bool is_test = (instruction.operation == Operation_TEST);

  CycleCount cycles = 0;
  if (instruction.DestinationMode() == OperandMode_Register && instruction.SourceMode() == OperandMode_Immediate)
  {
    cycles = is_test ?
               m_cpu->GetCyclesRM(CYCLES_TEST_RM_MEM_REG, (instruction.DestinationMode() == OperandMode_ModRM_RM) ?
                                                            instruction.ModRM_RM_IsReg() :
                                                            false) :
               m_cpu->GetCycles(CYCLES_ALU_REG_IMM);
  }
  else if (instruction.DestinationMode() == OperandMode_ModRM_RM)
    cycles = m_cpu->GetCyclesRM(is_test ? CYCLES_TEST_RM_MEM_REG : CYCLES_ALU_RM_MEM_REG, instruction.ModRM_RM_IsReg());
  else if (instruction.SourceMode() == OperandMode_ModRM_RM)
    cycles = m_cpu->GetCyclesRM(is_test ? CYCLES_TEST_REG_RM_MEM : CYCLES_ALU_REG_RM_MEM, instruction.ModRM_RM_IsReg());
  else
    Panic("Unknown mode");

  // Special case for xor reg, reg.
  if (instruction.operation == Operation_XOR && OperandRegistersMatch(instruction, 0, 1))
  {
    // Register contains zero, eflags has PF and ZF set.
    InstructionPrologue(instruction, cycles);
    CalculateEffectiveAddress(instruction);
    WriteOperand(instruction, 0, Value::FromConstant(0, instruction.operands[0].size));
    Value eflags = m_register_cache.ReadGuestRegister(Reg32_EFLAGS, true, true);
    EmitAnd(eflags.GetHostRegister(),
            Value::FromConstantU32(~(Flag_OF | Flag_CF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)));
    EmitOr(eflags.GetHostRegister(), Value::FromConstantU32(Flag_PF | Flag_ZF));
    m_register_cache.WriteGuestRegister(Reg32_EFLAGS, std::move(eflags));
  }
  else
  {
    // TODO: constant folding here
    if (!Compile_Bitwise_Impl(instruction, cycles))
      return Compile_Fallback(instruction);
  }

  if (OperandIsESP(instruction, 0))
    SyncCurrentESP();

  return true;
}

bool CodeGenerator::Compile_NOT(const Instruction& instruction)
{
  CycleCount cycles = 0;
  if (instruction.DestinationMode() == OperandMode_Register)
    cycles = m_cpu->GetCycles(CYCLES_NEG_RM_REG);
  else if (instruction.DestinationMode() == OperandMode_ModRM_RM)
    cycles = m_cpu->GetCyclesRM(CYCLES_NEG_RM_MEM, instruction.ModRM_RM_IsReg());
  else
    Panic("Unknown mode");

  InstructionPrologue(instruction, cycles);
  CalculateEffectiveAddress(instruction);

  Value value = ReadOperand(instruction, 0, instruction.operands[0].size, false, false);

  // const prop
  // TODO: Option to disable
  if (value.IsConstant())
  {
    switch (value.size)
    {
      case OperandSize_8:
        value.constant_value = ZeroExtend64(~Truncate8(value.constant_value));
        break;

      case OperandSize_16:
        value.constant_value = ZeroExtend64(~Truncate16(value.constant_value));
        break;

      case OperandSize_32:
        value.constant_value = ZeroExtend64(~Truncate32(value.constant_value));
        break;

      default:
        UnreachableCode();
        break;
    }
  }
  else
  {
    EmitNot(value.GetHostRegister(), value.size);
  }

  WriteOperand(instruction, 0, std::move(value));
  return true;
}

bool CodeGenerator::Compile_AddSub(const Instruction& instruction)
{
  const bool is_cmp = (instruction.operation == Operation_CMP);

  CycleCount cycles = 0;
  if (instruction.DestinationMode() == OperandMode_Register && instruction.SourceMode() == OperandMode_Immediate)
    cycles = m_cpu->GetCycles(is_cmp ? CYCLES_CMP_REG_IMM : CYCLES_ALU_REG_IMM);
  else if (instruction.DestinationMode() == OperandMode_ModRM_RM)
    cycles = m_cpu->GetCyclesRM(is_cmp ? CYCLES_CMP_RM_MEM_REG : CYCLES_ALU_RM_MEM_REG, instruction.ModRM_RM_IsReg());
  else if (instruction.SourceMode() == OperandMode_ModRM_RM)
    cycles = m_cpu->GetCyclesRM(is_cmp ? CYCLES_CMP_REG_RM_MEM : CYCLES_ALU_REG_RM_MEM, instruction.ModRM_RM_IsReg());
  else
    Panic("Unknown mode");

  // TODO: constant folding here

  if (!Compile_AddSub_Impl(instruction, cycles))
    return Compile_Fallback(instruction);

  if (OperandIsESP(instruction, 0))
    SyncCurrentESP();

  return true;
}

bool CodeGenerator::Compile_PUSH(const Instruction& instruction)
{
  CycleCount cycles = 0;
  if (instruction.operands[0].mode == OperandMode_Immediate)
    cycles = m_cpu->GetCycles(CYCLES_PUSH_IMM);
  else if (instruction.operands[0].mode == OperandMode_Register)
    cycles = m_cpu->GetCycles(CYCLES_PUSH_REG);
  else if (instruction.operands[0].mode == OperandMode_ModRM_RM)
    cycles = m_cpu->GetCycles(instruction.ModRM_RM_IsReg() ? CYCLES_PUSH_REG : CYCLES_PUSH_MEM);
  else
    Panic("Unknown mode");

  InstructionPrologue(instruction, cycles);
  CalculateEffectiveAddress(instruction);

  // For calling the thunk, we need ESP flushed. Invalidate as well, since the push will change it.
  m_register_cache.FlushGuestRegister(Reg32_ESP, true);

  // size is determined by the general operand size of the instruction, sign-extended if smaller
  // TODO: value can be discarded once the function is called, no need to restore it if scratch
  // TODO: don't use a thunk, the whole thing can be done inline manipulating the registers
  Value value = ReadOperand(instruction, 0, instruction.GetOperandSize(), true, false);
  switch (instruction.GetOperandSize())
  {
    case OperandSize_16:
      EmitFunctionCall(nullptr, &Thunks::PushWord, m_register_cache.GetCPUPtr(), value);
      break;

    case OperandSize_32:
      EmitFunctionCall(nullptr, &Thunks::PushDWord, m_register_cache.GetCPUPtr(), value);
      break;

    default:
      UnreachableCode();
      break;
  }

  // ESP is updated after the instruction.
  SyncCurrentESP();
  return true;
}

bool CodeGenerator::Compile_POP(const Instruction& instruction)
{
  CycleCount cycles = 0;
  if (instruction.operands[0].mode == OperandMode_Register)
    cycles = m_cpu->GetCycles(CYCLES_POP_REG);
  else if (instruction.operands[0].mode == OperandMode_ModRM_RM)
    cycles = m_cpu->GetCycles(instruction.ModRM_RM_IsReg() ? CYCLES_POP_REG : CYCLES_POP_MEM);
  else
    Panic("Unknown mode");

  InstructionPrologue(instruction, cycles);
  CalculateEffectiveAddress(instruction);

  // For calling the thunk, we need ESP flushed. Invalidate as well, since the pop will change it.
  m_register_cache.FlushGuestRegister(Reg32_ESP, true);

  // TODO: don't use a thunk, the whole thing can be done inline manipulating the registers
  Value value = m_register_cache.AllocateScratch(instruction.GetOperandSize());
  switch (instruction.GetOperandSize())
  {
    case OperandSize_16:
      EmitFunctionCall(&value, &Thunks::PopWord, m_register_cache.GetCPUPtr());
      break;

    case OperandSize_32:
      EmitFunctionCall(&value, &Thunks::PopDWord, m_register_cache.GetCPUPtr());
      break;

    default:
      UnreachableCode();
      break;
  }
  WriteOperand(instruction, 0, std::move(value));

  // ESP is updated after the pop, but we don't want to alter the value for exceptions until after the write occurs, if
  // this is popping to memory.
  SyncCurrentESP();
  return true;
}

} // namespace CPU_X86::Recompiler
