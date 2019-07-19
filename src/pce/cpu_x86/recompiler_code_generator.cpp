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

CodeGenerator::CodeGenerator(Backend* backend, Block* block)
  : m_backend(backend), m_block(block), m_block_start(block->instructions.data()),
  m_block_end(block->instructions.data() + block->instructions.size())
{
}

CodeGenerator::~CodeGenerator() {}

void CodeGenerator::SetHostRegAllocationOrder(std::initializer_list<HostReg> regs)
{
  size_t index = 0;
  for (HostReg reg : regs)
  {
    m_host_registers[reg].state = HostRegState::Free | HostRegState::UsableInRegisterCache;
    m_host_register_allocation_order[index++] = reg;
  }
  m_host_register_available_count = static_cast<u32>(index);
}

void CodeGenerator::SetCallerSavedHostRegs(std::initializer_list<HostReg> regs)
{
  for (HostReg reg : regs)
    m_host_registers[reg].state |= HostRegState::CallerSaved;
}

bool CodeGenerator::IsCacheableHostRegister(HostReg reg) const
{
  return (m_host_registers[reg].state & HostRegState::UsableInRegisterCache) != HostRegState::None;
}

HostReg CodeGenerator::AllocateHostReg(OperandSize size, HostRegState state /* = HostRegState::InUse */)
{
  // try for a free register in allocation order
  for (u32 i = 0; i < m_host_register_available_count; i++)
  {
    const HostReg reg = m_host_register_allocation_order[i];
    if ((m_host_registers[reg].state & HostRegState::Free) != HostRegState::None)
    {
      m_host_registers[reg].state = (m_host_registers[reg].state & ~(HostRegState::Free)) | state;
      m_host_registers[reg].size = size;
      return reg;
    }
  }

  // evict one of the cached guest registers
  HostReg reg = EvictOneGuestRegister();
  if (reg >= HostReg_Count)
    Panic("Failed to evict guest register for new allocation");

  m_host_registers[reg].state = (m_host_registers[reg].state & ~(HostRegState::Free)) | state;
  m_host_registers[reg].size = size;
  return reg;
}

Value CodeGenerator::AllocateTemporaryHostReg(OperandSize size)
{
  HostReg reg = AllocateHostReg(size);
  return Value::FromTemporary(reg, size);
}

void CodeGenerator::FreeHostReg(HostReg reg)
{
  m_host_registers[reg].state = (m_host_registers[reg].state & ~(HostRegState::CallerSaved)) | HostRegState::Free;
}

void CodeGenerator::ReleaseValue(Value& value)
{
  if (value.IsTemporary())
    FreeHostReg(value.host_reg);
  value.Clear();
}

void CodeGenerator::ConvertValueSize(Value& value, OperandSize size, bool sign_extend)
{
  // We should only be going up in size, not down..
  DebugAssert(size > value.size);

  if (value.IsConstant())
  {
    // compile-time conversion, woo!
    switch (size)
    {
      case OperandSize_8:
        {
          value.constant_value &= 0xFF;
        }
        break;

      case OperandSize_16:
        {
          switch (value.size)
          {
            case OperandSize_8:
              value.constant_value = sign_extend ? ZeroExtend32(SignExtend16(static_cast<u8>(value.constant_value))) :
                ZeroExtend32(static_cast<u8>(value.constant_value));
              break;
            default:
              value.constant_value &= 0xFFFF;
              break;
          }
        }
        break;

      case OperandSize_32:
        {
          switch (value.size)
          {
            case OperandSize_8:
              value.constant_value = sign_extend ? SignExtend32(static_cast<u8>(value.constant_value)) :
                ZeroExtend32(static_cast<u8>(value.constant_value));
              break;
            case OperandSize_16:
              value.constant_value = sign_extend ? SignExtend32(static_cast<u16>(value.constant_value)) :
                ZeroExtend32(static_cast<u16>(value.constant_value));
              break;
            default:
              break;
          }
        }
        break;

      default:
        break;
    }

    value.size = size;
    return;
  }

  // if it's not in a temporary, we don't want to mess up the regcache value.. so we need a temporary.
  if (!value.IsTemporary())
  {
    HostReg reg = AllocateHostReg(value.size);
    EmitSignExtend(reg, size, value.host_reg, value.size);
    value = Value::FromTemporary(reg, size);
  }
  else
  {
    // otherwise, we can just adjust the value
    EmitSignExtend(value.host_reg, size, value.host_reg, value.size);
    value.size = size;
  }
}

void CodeGenerator::FlushOverlappingGuestRegisters(Reg8 guest_reg)
{
  switch (guest_reg)
  {
    case Reg8_AL:
    case Reg8_AH:
      FlushGuestRegister(Reg16_AX, true);
      FlushGuestRegister(Reg32_EAX, true);
      break;
    case Reg8_CL:
    case Reg8_CH:
      FlushGuestRegister(Reg16_CX, true);
      FlushGuestRegister(Reg32_ECX, true);
      break;
    case Reg8_BL:
    case Reg8_BH:
      FlushGuestRegister(Reg16_BX, true);
      FlushGuestRegister(Reg32_EBX, true);
      break;
    case Reg8_DL:
    case Reg8_DH:
      FlushGuestRegister(Reg16_DX, true);
      FlushGuestRegister(Reg32_EDX, true);
      break;
    default:
      break;
  }
}

void CodeGenerator::FlushOverlappingGuestRegisters(Reg16 guest_reg)
{
  switch (guest_reg)
  {
    case Reg16_AX:
      FlushGuestRegister(Reg8_AL, true);
      FlushGuestRegister(Reg8_AH, true);
      FlushGuestRegister(Reg32_EAX, true);
      break;
    case Reg16_BX:
      FlushGuestRegister(Reg8_BL, true);
      FlushGuestRegister(Reg8_BH, true);
      FlushGuestRegister(Reg32_EBX, true);
      break;
    case Reg16_CX:
      FlushGuestRegister(Reg8_CL, true);
      FlushGuestRegister(Reg8_CH, true);
      FlushGuestRegister(Reg16_CX, true);
      FlushGuestRegister(Reg32_ECX, true);
      break;
    case Reg16_DX:
      FlushGuestRegister(Reg8_DL, true);
      FlushGuestRegister(Reg8_DH, true);
      FlushGuestRegister(Reg16_DX, true);
      FlushGuestRegister(Reg32_EDX, true);
      break;
    case Reg16_SP:
      FlushGuestRegister(Reg32_ESP, true);
      break;
    case Reg16_BP:
      FlushGuestRegister(Reg32_EBP, true);
      break;
    case Reg16_SI:
      FlushGuestRegister(Reg32_ESI, true);
      break;
    case Reg16_DI:
      FlushGuestRegister(Reg32_EDI, true);
      break;
    default:
      break;
  }
}

void CodeGenerator::FlushOverlappingGuestRegisters(Reg32 guest_reg)
{
  switch (guest_reg)
  {
    case Reg32_EAX:
      FlushGuestRegister(Reg8_AL, true);
      FlushGuestRegister(Reg8_AH, true);
      FlushGuestRegister(Reg16_AX, true);
      break;
    case Reg32_EBX:
      FlushGuestRegister(Reg8_BL, true);
      FlushGuestRegister(Reg8_BH, true);
      FlushGuestRegister(Reg16_BX, true);
      break;
    case Reg32_ECX:
      FlushGuestRegister(Reg8_CL, true);
      FlushGuestRegister(Reg8_CH, true);
      FlushGuestRegister(Reg16_CX, true);
      FlushGuestRegister(Reg32_ECX, true);
      break;
    case Reg32_EDX:
      FlushGuestRegister(Reg8_DL, true);
      FlushGuestRegister(Reg8_DH, true);
      FlushGuestRegister(Reg16_DX, true);
      FlushGuestRegister(Reg32_EDX, true);
      break;
    case Reg32_ESP:
      FlushGuestRegister(Reg32_ESP, true);
      break;
    case Reg32_EBP:
      FlushGuestRegister(Reg32_EBP, true);
      break;
    case Reg32_ESI:
      FlushGuestRegister(Reg32_ESI, true);
      break;
    case Reg32_EDI:
      FlushGuestRegister(Reg32_EDI, true);
      break;
    default:
      break;
  }
}

Value CodeGenerator::ReadGuestRegister(Reg8 guest_reg, bool cache /* = true */)
{
  if (m_guest_reg8_state[guest_reg].IsCached())
    return m_guest_reg8_state[guest_reg].ToValue(OperandSize_8);

  FlushOverlappingGuestRegisters(guest_reg);

  const HostReg host_reg = AllocateHostReg(OperandSize_8);
  EmitLoadGuestRegister(host_reg, OperandSize_8, guest_reg);

  // Now in cache.
  if (cache)
  {
    m_guest_reg8_state[guest_reg].SetHostReg(host_reg);
    return Value::FromHostReg(host_reg, OperandSize_8);
  }
  else
  {
    return Value::FromTemporary(host_reg, OperandSize_8);
  }
}

Value CodeGenerator::ReadGuestRegister(Reg16 guest_reg, bool cache /* = true */)
{
  if (m_guest_reg16_state[guest_reg].IsCached())
    return m_guest_reg16_state[guest_reg].ToValue(OperandSize_16);

  FlushOverlappingGuestRegisters(guest_reg);

  const HostReg host_reg = AllocateHostReg(OperandSize_16);
  EmitLoadGuestRegister(host_reg, OperandSize_16, guest_reg);

  // Now in cache.
  if (cache)
  {
    m_guest_reg16_state[guest_reg].SetHostReg(host_reg);
    return Value::FromHostReg(host_reg, OperandSize_16);
  }
  else
  {
    return Value::FromTemporary(host_reg, OperandSize_16);
  }
}

Value CodeGenerator::ReadGuestRegister(Reg32 guest_reg, bool cache /* = true */)
{
  if (m_guest_reg32_state[guest_reg].IsCached())
    return m_guest_reg32_state[guest_reg].ToValue(OperandSize_32);

  FlushOverlappingGuestRegisters(guest_reg);

  const HostReg host_reg = AllocateHostReg(OperandSize_32);
  EmitLoadGuestRegister(host_reg, OperandSize_32, guest_reg);

  // Now in cache.
  if (cache)
  {
    m_guest_reg32_state[guest_reg].SetHostReg(host_reg);
    return Value::FromHostReg(host_reg, OperandSize_32);
  }
  else
  {
    return Value::FromTemporary(host_reg, OperandSize_32);
  }
}

void CodeGenerator::WriteGuestRegister(Reg8 guest_reg, Value&& value)
{
  FlushOverlappingGuestRegisters(guest_reg);
  InvalidateGuestRegister(guest_reg);

  GuestRegData& guest_reg_state = m_guest_reg8_state[guest_reg];
  if (value.IsConstant())
  {
    // No need to allocate a host register, and we can defer the store.
    guest_reg_state.SetConstant(value.constant_value);
    guest_reg_state.SetDirty();
    return;
  }

  // If it's a temporary, we can bind that to the guest register.
  if (value.IsTemporary() && IsCacheableHostRegister(value.host_reg))
  {
    guest_reg_state.SetHostReg(value.host_reg);
    guest_reg_state.SetDirty();
    value.Clear();
    return;
  }

  // Allocate host register, and copy value to it.
  HostReg host_reg = AllocateHostReg(OperandSize_8);
  EmitCopyValue(host_reg, value);
  guest_reg_state.SetHostReg(host_reg);
  ReleaseValue(value);
}

void CodeGenerator::WriteGuestRegister(Reg16 guest_reg, Value&& value)
{
  FlushOverlappingGuestRegisters(guest_reg);
  InvalidateGuestRegister(guest_reg);

  GuestRegData& guest_reg_state = m_guest_reg16_state[guest_reg];
  if (value.IsConstant())
  {
    // No need to allocate a host register, and we can defer the store.
    guest_reg_state.SetConstant(value.constant_value);
    guest_reg_state.SetDirty();
    return;
  }

  // If it's a temporary, we can bind that to the guest register.
  if (value.IsTemporary() && IsCacheableHostRegister(value.host_reg))
  {
    guest_reg_state.SetHostReg(value.host_reg);
    guest_reg_state.SetDirty();
    value.Clear();
    return;
  }

  // Allocate host register, and copy value to it.
  HostReg host_reg = AllocateHostReg(OperandSize_16);
  EmitCopyValue(host_reg, value);
  guest_reg_state.SetHostReg(host_reg);
  ReleaseValue(value);
}

void CodeGenerator::WriteGuestRegister(Reg32 guest_reg, Value&& value)
{
  FlushOverlappingGuestRegisters(guest_reg);
  InvalidateGuestRegister(guest_reg);

  GuestRegData& guest_reg_state = m_guest_reg32_state[guest_reg];
  if (value.IsConstant())
  {
    // No need to allocate a host register, and we can defer the store.
    guest_reg_state.SetConstant(value.constant_value);
    guest_reg_state.SetDirty();
    return;
  }

  // If it's a temporary, we can bind that to the guest register.
  if (value.IsTemporary() && IsCacheableHostRegister(value.host_reg))
  {
    guest_reg_state.SetHostReg(value.host_reg);
    guest_reg_state.SetDirty();
    value.Clear();
    return;
  }

  // Allocate host register, and copy value to it.
  HostReg host_reg = AllocateHostReg(OperandSize_32);
  EmitCopyValue(host_reg, value);
  guest_reg_state.SetHostReg(host_reg);
  ReleaseValue(value);
}

void CodeGenerator::FlushGuestRegister(Reg8 guest_reg, bool invalidate)
{
  if (!m_guest_reg8_state[guest_reg].IsDirty())
    EmitStoreGuestRegister(OperandSize_8, guest_reg);

  if (invalidate)
    InvalidateGuestRegister(guest_reg);
}

void CodeGenerator::InvalidateGuestRegister(Reg8 guest_reg)
{
  if (m_guest_reg32_state[guest_reg].IsInHostRegister())
    FreeHostReg(m_guest_reg32_state[guest_reg].host_reg);

  m_guest_reg8_state[guest_reg].Invalidate();
}

void CodeGenerator::FlushGuestRegister(Reg16 guest_reg, bool invalidate)
{
  if (m_guest_reg16_state[guest_reg].IsDirty())
    EmitStoreGuestRegister(OperandSize_16, guest_reg);

  if (invalidate)
    InvalidateGuestRegister(guest_reg);
}

void CodeGenerator::InvalidateGuestRegister(Reg16 guest_reg)
{
  if (m_guest_reg16_state[guest_reg].IsInHostRegister())
    FreeHostReg(m_guest_reg16_state[guest_reg].host_reg);

  m_guest_reg16_state[guest_reg].Invalidate();
}

void CodeGenerator::FlushGuestRegister(Reg32 guest_reg, bool invalidate)
{
  if (m_guest_reg32_state[guest_reg].IsDirty())
    EmitStoreGuestRegister(OperandSize_32, guest_reg);

  if (invalidate)
    InvalidateGuestRegister(guest_reg);
}

void CodeGenerator::InvalidateGuestRegister(Reg32 guest_reg)
{
  if (m_guest_reg32_state[guest_reg].IsInHostRegister())
    FreeHostReg(m_guest_reg32_state[guest_reg].host_reg);

  m_guest_reg32_state[guest_reg].Invalidate();
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
          Value val = ReadGuestRegister(static_cast<Reg8>(reg));
          if (output_size != OperandSize_8)
            ConvertValueSize(val, output_size, sign_extend);

          return val;
        }

      case OperandSize_16:
        {
          Value val = ReadGuestRegister(static_cast<Reg16>(reg));
          if (output_size != OperandSize_16)
            ConvertValueSize(val, output_size, sign_extend);

          return val;
        }

      case OperandSize_32:
        {
          Value val = ReadGuestRegister(static_cast<Reg32>(reg));
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
                  return Value::FromConstantU16(sign_extend ? SignExtend32(instruction->data.imm8) :
                                                ZeroExtend32(instruction->data.imm8));

                default:
                  return Value::FromConstantU16(ZeroExtend32(instruction->data.imm16));
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
        HostReg reg = AllocateHostReg(OperandSize_16);
        EmitLoadCPUStructField(reg, OperandSize_16, CalculateSegmentRegisterOffset(operand->segreg));
        if (output_size == OperandSize_32)
        {
          // Segment registers are sign-extended on push/pop.
          EmitSignExtend(reg, OperandSize_32, reg, OperandSize_16);
        }

        return Value::FromTemporary(reg, output_size);
      }

    case OperandMode_Memory:
    case OperandMode_ModRM_RM:
      {
        if (operand->mode == OperandMode_ModRM_RM && instruction->ModRM_RM_IsReg())
          return MakeRegisterAccess(instruction->data.modrm_rm_register);

        // Memory loads can fault. Ditch all the cached registers so we don't need to push them.
        // The effective address should remain..
        FlushAllGuestRegisters(false);

        // we get the result back in eax, which we can use as a temporary.
        HostReg reg =
          EmitLoadGuestMemory(operand->size, m_operand_memory_addresses[index], instruction->GetMemorySegment());
        Value val = Value::FromTemporary(reg, operand->size);

        // handle sign-extension
        if (operand->size != output_size)
          ConvertValueSize(val, output_size, sign_extend);

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
            WriteGuestRegister(operand->reg8, std::move(value));
            return;
          case OperandSize_16:
            WriteGuestRegister(operand->reg16, std::move(value));
            return;
          case OperandSize_32:
            WriteGuestRegister(operand->reg32, std::move(value));
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
              WriteGuestRegister(static_cast<Reg8>(instruction->data.modrm_rm_register), std::move(value));
              break;
            case OperandSize_16:
              WriteGuestRegister(static_cast<Reg16>(instruction->data.modrm_rm_register), std::move(value));
              break;
            case OperandSize_32:
              WriteGuestRegister(static_cast<Reg32>(instruction->data.modrm_rm_register), std::move(value));
              break;
            default:
              break;
          }
          return;
        }

        // Memory writes can fault. Ditch all cached registers before continuing.
        FlushAllGuestRegisters(false);
        EmitStoreGuestMemory(value, m_operand_memory_addresses[index], instruction->GetMemorySegment());
        return;
      }
  }

  Panic("Unhandled operand mode");
}



std::pair<const void*, size_t> CodeGenerator::FinishBlock()
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

bool CodeGenerator::CompileInstruction(const Instruction* instruction, bool is_final)
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

  // release temporary effective addresses
  for (Value& value : m_operand_memory_addresses)
    ReleaseValue(value);

  return result;
}

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

void CodeGenerator::CalculateEffectiveAddress(const Instruction* instruction)
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

void CodeGenerator::WriteOperand(const Instruction* instruction, size_t index, const Xbyak::Reg& src)
{
#if 0

#endif
}

void CodeGenerator::ReadFarAddressOperand(const Instruction* instruction, size_t index, const Xbyak::Reg& dest_segment,
                                          const Xbyak::Reg& dest_offset)
{
#if 0
  const Instruction::Operand * operand = &instruction->operands[index];
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

void CodeGenerator::UpdateFlags(uint32 clear_mask, uint32 set_mask, uint32 host_mask)
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
      and (dword[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ~clear_mask);
    else
      and (word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ~Truncate16(clear_mask));
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
      and (ah, Truncate8(host_mask));
      or (byte[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ah);
    }
    else if (use_eflags)
    {
      and (eax, host_mask);
      or (dword[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], eax);
    }
    else
    {
      and (ax, Truncate16(host_mask));
      or (word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)], ax);
    }
  }
}

void CodeGenerator::SyncInstructionPointers(const Instruction* next_instruction)
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

void CodeGenerator::StartInstruction(const Instruction* instruction)
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

void CodeGenerator::EndInstruction(const Instruction* instruction, bool update_eip, bool update_esp)
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

bool CodeGenerator::Compile_NOP(const Instruction* instruction)
{
  StartInstruction(instruction);
  EndInstruction(instruction);
  return true;
}

#if 0

bool JitX64CodeGenerator::Compile_LEA(const Instruction * instruction)
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
            and (RSTORE8A, RSTORE8B);
            UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
            break;
          case Operation_OR:
            or (RSTORE8A, RSTORE8B);
            UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
            break;
          case Operation_XOR:
            xor (RSTORE8A, RSTORE8B);
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
            and (RSTORE16A, RSTORE16B);
            UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
            break;
          case Operation_OR:
            or (RSTORE16A, RSTORE16B);
            UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
            break;
          case Operation_XOR:
            xor (RSTORE16A, RSTORE16B);
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
            and (RSTORE32A, RSTORE32B);
            UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
            break;
          case Operation_OR:
            or (RSTORE32A, RSTORE32B);
            UpdateFlags(Flag_OF | Flag_CF | Flag_AF, 0, Flag_SF | Flag_ZF | Flag_PF);
            break;
          case Operation_XOR:
            xor (RSTORE32A, RSTORE32B);
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
            and (RSTORE8A, RSTORE8B);
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
            and (RSTORE16A, RSTORE16B);
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
            and (RSTORE32A, RSTORE32B);
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
    and (cl, 0x1F);
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
    and (cl, 0x1F);
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
void CodeGenerator::BranchToTrampoline(CPU* cpu, uint32 address)
{
  cpu->BranchTo(address);
}

void CodeGenerator::PushWordTrampoline(CPU* cpu, uint16 value)
{
  cpu->PushWord(value);
}

void CodeGenerator::PushDWordTrampoline(CPU* cpu, uint32 value)
{
  cpu->PushDWord(value);
}

uint16 CodeGenerator::PopWordTrampoline(CPU* cpu)
{
  return cpu->PopWord();
}

uint32 CodeGenerator::PopDWordTrampoline(CPU* cpu)
{
  return cpu->PopDWord();
}

void CodeGenerator::LoadSegmentRegisterTrampoline(CPU* cpu, uint32 segment, uint16 value)
{
  cpu->LoadSegmentRegister(static_cast<Segment>(segment), value);
}

void CodeGenerator::RaiseExceptionTrampoline(CPU* cpu, uint32 interrupt, uint32 error_code)
{
  cpu->RaiseException(interrupt, error_code);
}

void CodeGenerator::SetFlagsTrampoline(CPU* cpu, uint32 flags)
{
  cpu->SetFlags(flags);
}

void CodeGenerator::FarJumpTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size)
{
  cpu->FarJump(segment_selector, offset, static_cast<OperandSize>(op_size));
}

void CodeGenerator::FarCallTrampoline(CPU* cpu, uint16 segment_selector, uint32 offset, uint32 op_size)
{
  cpu->FarCall(segment_selector, offset, static_cast<OperandSize>(op_size));
}

void CodeGenerator::FarReturnTrampoline(CPU* cpu, uint32 op_size, uint32 pop_count)
{
  cpu->FarReturn(static_cast<OperandSize>(op_size), pop_count);
}

bool CodeGenerator::Compile_JumpConditional(const Instruction* instruction)
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
      xor (RTEMP16A, RTEMP16B);
      test(RTEMP16A, 1u);
      jz(test_fail_label);
      break;

    case JumpCondition_GreaterOrEqual:
      // Jump if SF == OF.
      mov(RTEMP16A, word[RCPUPTR + offsetof(CPU, m_registers.EFLAGS.bits)]);
      mov(RTEMP16B, RTEMP16A);
      shr(RTEMP16A, 7);
      shr(RTEMP16B, 11);
      xor (RTEMP16A, RTEMP16B);
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
      xor (RTEMP16A, RTEMP16B);
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
      xor (RTEMP16A, RTEMP16B);
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

bool CodeGenerator::Compile_JumpCallReturn(const Instruction* instruction)
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
          xor (RPARAM3_32, RPARAM3_32);
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

bool CodeGenerator::Compile_Stack(const Instruction* instruction)
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
    and (RTEMP32A, Flag_IOPL);
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
          and (RPARAM2_32, ~uint32(Flag_RF | Flag_VM));
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
          and (RPARAM2_32, UINT32_C(0xFFFF0000));
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

bool CodeGenerator::Compile_Flags(const Instruction* instruction)
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

void CodeGenerator::InterpretInstructionTrampoline(CPU* cpu, const Instruction* instruction)
{
  std::memcpy(&cpu->idata, &instruction->data, sizeof(cpu->idata));
  // instruction->interpreter_handler(cpu);
  Panic("Fixme");
}

bool CodeGenerator::Compile_Fallback(const Instruction* instruction)
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

  Interpreter::HandlerFunction interpreter_handler = Interpreter::GetInterpreterHandlerForInstruction(instruction);
  if (!interpreter_handler)
    return false;

  u64 idata_qwords[2];
  std::memcpy(&idata_qwords[0], &instruction->data, sizeof(idata_qwords[0]));
  std::memcpy(&idata_qwords[1], reinterpret_cast<const u8*>(&instruction->data) + sizeof(idata_qwords[0]),
              sizeof(idata_qwords[1]));

  mov(RSCRATCH64, idata_qwords[0]);
  mov(qword[RCPUPTR + offsetof(CPU, idata)], RSCRATCH64);
  mov(RSCRATCH64, idata_qwords[1]);
  mov(qword[RCPUPTR + (offsetof(CPU, idata) + sizeof(idata_qwords[0]))], RSCRATCH64);
  mov(RPARAM1_64, RCPUPTR);
  mov(RSCRATCH64, reinterpret_cast<size_t>(interpreter_handler));
  call(RSCRATCH64);

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



} // namespace CPU_X86::Recompiler
