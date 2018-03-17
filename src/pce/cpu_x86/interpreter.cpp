#include "pce/cpu_x86/interpreter.h"
#include "YBaseLib/Endian.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
Log_SetChannel(CPU_X86::Interpreter);

namespace CPU_X86 {

VirtualMemoryAddress Interpreter::CalculateEffectiveAddress(CPU* cpu, const OldInstruction* instruction,
                                                            const OldInstruction::Operand* operand)
{
  if (instruction->address_size == AddressSize_16)
  {
    switch (operand->mode)
    {
      case AddressingMode_Direct:
        return ZeroExtend32(Truncate16(operand->direct.address));

      case AddressingMode_RegisterIndirect:
        DebugAssert(operand->reg.reg16 < Reg16_Count);
        return ZeroExtend32(cpu->m_registers.reg16[operand->reg.reg16]);

      case AddressingMode_Indexed:
      {
        DebugAssert(operand->indexed.reg.reg16 < Reg16_Count);
        uint16 address = cpu->m_registers.reg16[operand->indexed.reg.reg16];
        address += uint16(operand->indexed.displacement);
        return ZeroExtend32(address);
      }

      case AddressingMode_BasedIndexed:
      {
        DebugAssert(operand->based_indexed.base.reg16 < Reg16_Count);
        uint16 address = cpu->m_registers.reg16[operand->based_indexed.base.reg16];
        address += cpu->m_registers.reg16[operand->based_indexed.index.reg16];
        return ZeroExtend32(address);
      }

      case AddressingMode_BasedIndexedDisplacement:
      {
        DebugAssert(operand->based_indexed_displacement.base.reg16 < Reg16_Count);
        uint16 address = (cpu->m_registers.reg16[operand->based_indexed_displacement.base.reg16] +
                          cpu->m_registers.reg16[operand->based_indexed_displacement.index.reg16]);
        address += uint16(operand->based_indexed_displacement.displacement);
        return ZeroExtend32(address);
      }

      default:
        Panic("Unhandled address mode");
        return 0;
    }
  }
  else // if (instruction->address_size == AddressSize_32)
  {
    DebugAssert(instruction->address_size == AddressSize_32);
    switch (operand->mode)
    {
      case AddressingMode_Direct:
        return operand->direct.address;

      case AddressingMode_RegisterIndirect:
        DebugAssert(operand->reg.reg32 < Reg32_Count);
        return cpu->m_registers.reg32[operand->reg.reg32];

      case AddressingMode_Indexed:
      {
        DebugAssert(operand->indexed.reg.reg32 < Reg32_Count);
        uint32 address = cpu->m_registers.reg32[operand->indexed.reg.reg32];
        address += uint32(operand->indexed.displacement);
        return address;
      }

      case AddressingMode_SIB:
      {
        uint32 address = 0;

        // base
        if (operand->sib.base.reg32 != Reg32_Count)
        {
          DebugAssert(operand->sib.base.raw < Reg32_Count);
          address = cpu->m_registers.reg32[operand->sib.base.reg32];
        }

        // + index * scale
        if (operand->sib.index.reg32 != Reg32_Count)
        {
          DebugAssert(operand->sib.index.raw < Reg32_Count);
          address += cpu->m_registers.reg32[operand->sib.index.reg32] << operand->sib.scale_shift;
        }

        // + displacement
        address += uint32(operand->sib.displacement);
        return address;
      }

      default:
        Panic("Unhandled address mode");
        return 0;
    }
  }
}

uint8 Interpreter::ReadByteOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand)
{
  switch (operand->mode)
  {
    case AddressingMode_Register:
    {
      // Have to read from the correct register, drop high bits.
      switch (operand->size)
      {
        case OperandSize_8:
          return cpu->m_registers.reg8[operand->reg.reg8];
        case OperandSize_16:
          return uint8(cpu->m_registers.reg16[operand->reg.reg16]);
        default:
          DebugUnreachableCode();
          return 0;
      }
    }

    case AddressingMode_Immediate:
    {
      // Drop higher bits if the operand size isn't equal to 8.
      return operand->immediate.value8;
    }

    default:
    {
      // Memory read, drop higher bits.
      VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, operand);
      switch (operand->size)
      {
        case OperandSize_8:
          return cpu->ReadMemoryByte(instruction->segment, address);

        case OperandSize_16:
          return Truncate8(cpu->ReadMemoryWord(instruction->segment, address));

        case OperandSize_32:
          return Truncate8(cpu->ReadMemoryDWord(instruction->segment, address));

        default:
          DebugUnreachableCode();
          return 0;
      }
    }
  }
}

uint16 Interpreter::ReadWordOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand)
{
  switch (operand->mode)
  {
    case AddressingMode_Register:
    {
      // Zero-extend smaller registers.
      switch (operand->size)
      {
        case OperandSize_8:
          DebugAssert(operand->reg.reg8 < Reg8_Count);
          return uint16(cpu->m_registers.reg8[operand->reg.reg8]);

        case OperandSize_16:
          DebugAssert(operand->reg.reg16 < Reg16_Count);
          return cpu->m_registers.reg16[operand->reg.reg16];

        default:
          DebugUnreachableCode();
          return 0;
      }
    }

    case AddressingMode_SegmentRegister:
    {
      DebugAssert(operand->reg.sreg < Segment_Count);
      return cpu->m_registers.segment_selectors[operand->reg.sreg];
    }

    case AddressingMode_Immediate:
    {
      // Drop higher bits.
      return operand->immediate.value16;
    }

    case AddressingMode_Relative:
    {
      // TODO: Should this be extended to addressing mode?
      uint16 address = Truncate16(cpu->m_registers.EIP);
      address += static_cast<uint16>(operand->relative.displacement);
      return address;
    }

    default:
    {
      // Memory read, drop higher bits. Zero-extend smaller reads.
      VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, operand);
      switch (operand->size)
      {
        case OperandSize_8:
          return cpu->ReadMemoryByte(instruction->segment, address);

        case OperandSize_16:
          return cpu->ReadMemoryWord(instruction->segment, address);

        case OperandSize_32:
          return Truncate16(cpu->ReadMemoryDWord(instruction->segment, address));

        default:
          DebugUnreachableCode();
          return 0;
      }
    }
  }
}

uint16 Interpreter::ReadSignExtendedWordOperand(CPU* cpu, const OldInstruction* instruction,
                                                const OldInstruction::Operand* operand)
{
  switch (operand->mode)
  {
    case AddressingMode_Register:
    {
      // Sign-extend smaller registers.
      switch (operand->size)
      {
        case OperandSize_8:
          return uint16(int16(int8(cpu->m_registers.reg8[operand->reg.reg8])));
        case OperandSize_16:
          return cpu->m_registers.reg16[operand->reg.reg16];
        default:
          DebugUnreachableCode();
          return 0;
      }
    }

    case AddressingMode_Immediate:
    {
      // Sign-extend smaller types, drop higher bits on larger types.
      switch (operand->size)
      {
        case OperandSize_8:
          return uint16(int16(int8(operand->immediate.value8)));
        case OperandSize_16:
          return operand->immediate.value16;
        case OperandSize_32:
          return operand->immediate.value16;
        default:
          DebugUnreachableCode();
          return 0;
      }
    }

    default:
    {
      // Memory read, drop higher bits. Zero-extend smaller reads.
      VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, operand);
      switch (operand->size)
      {
        case OperandSize_8:
          return SignExtend16(cpu->ReadMemoryByte(instruction->segment, address));

        case OperandSize_16:
          return cpu->ReadMemoryWord(instruction->segment, address);

        case OperandSize_32:
          return Truncate16(cpu->ReadMemoryDWord(instruction->segment, address));

        default:
          DebugUnreachableCode();
          return 0;
      }
    }
  }
}

uint32 Interpreter::ReadDWordOperand(CPU* cpu, const OldInstruction* instruction,
                                     const OldInstruction::Operand* operand)
{
  switch (operand->mode)
  {
    case AddressingMode_Register:
    {
      // Zero-extend smaller registers.
      switch (operand->size)
      {
        case OperandSize_8:
          DebugAssert(operand->reg.reg8 < Reg8_Count);
          return ZeroExtend32(cpu->m_registers.reg8[operand->reg.reg8]);

        case OperandSize_16:
          DebugAssert(operand->reg.reg16 < Reg16_Count);
          return ZeroExtend32(cpu->m_registers.reg16[operand->reg.reg16]);

        case OperandSize_32:
          DebugAssert(operand->reg.reg32 < Reg32_Count);
          return cpu->m_registers.reg32[operand->reg.reg32];

        default:
          DebugUnreachableCode();
          return 0;
      }
    }

    case AddressingMode_Immediate:
    {
      return operand->immediate.value32;
    }

    case AddressingMode_Relative:
    {
      // TODO: Should this be extended to addressing mode?
      uint32 address = cpu->m_registers.EIP;
      address += static_cast<uint32>(operand->relative.displacement);
      return address;
    }

    default:
    {
      // Memory read, zero-extend smaller reads.
      VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, operand);
      switch (operand->size)
      {
        case OperandSize_8:
          return ZeroExtend32(cpu->ReadMemoryByte(instruction->segment, address));

        case OperandSize_16:
          return ZeroExtend32(cpu->ReadMemoryWord(instruction->segment, address));

        case OperandSize_32:
          return cpu->ReadMemoryDWord(instruction->segment, address);

        default:
          DebugUnreachableCode();
          return 0;
      }
    }
  }
}

uint32 Interpreter::ReadSignExtendedDWordOperand(CPU* cpu, const OldInstruction* instruction,
                                                 const OldInstruction::Operand* operand)
{
  switch (operand->mode)
  {
    case AddressingMode_Register:
    {
      // Sign-extend smaller registers.
      switch (operand->size)
      {
        case OperandSize_8:
          DebugAssert(operand->reg.reg8 < Reg8_Count);
          return SignExtend32(cpu->m_registers.reg8[operand->reg.reg8]);

        case OperandSize_16:
          DebugAssert(operand->reg.reg16 < Reg16_Count);
          return SignExtend32(cpu->m_registers.reg16[operand->reg.reg16]);

        case OperandSize_32:
          DebugAssert(operand->reg.reg32 < Reg32_Count);
          return cpu->m_registers.reg32[operand->reg.reg32];

        default:
          DebugUnreachableCode();
          return 0;
      }
    }

    case AddressingMode_Immediate:
    {
      // Sign-extend smaller types.
      switch (operand->size)
      {
        case OperandSize_8:
          return SignExtend32(operand->immediate.value8);
        case OperandSize_16:
          return SignExtend32(operand->immediate.value16);
        case OperandSize_32:
          return operand->immediate.value32;

        default:
          DebugUnreachableCode();
          return 0;
      }
    }

    default:
    {
      // Memory read, sign-extend smaller reads.
      VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, operand);
      switch (operand->size)
      {
        case OperandSize_8:
          return SignExtend32(cpu->ReadMemoryByte(instruction->segment, address));

        case OperandSize_16:
          return SignExtend32(cpu->ReadMemoryWord(instruction->segment, address));

        case OperandSize_32:
          return cpu->ReadMemoryDWord(instruction->segment, address);

        default:
          DebugUnreachableCode();
          return 0;
      }
    }
  }
}

void Interpreter::ReadFarAddressOperand(CPU* cpu, const OldInstruction* instruction,
                                        const OldInstruction::Operand* operand, uint16* segment_selector,
                                        VirtualMemoryAddress* address)
{
  if (operand->mode == AddressingMode_FarAddress)
  {
    *address = operand->far_address.address;
    *segment_selector = operand->far_address.segment_selector;
    return;
  }

  // In 32-bit addressing mode, this is 16:32
  VirtualMemoryAddress base_address = CalculateEffectiveAddress(cpu, instruction, operand);
  if (instruction->operand_size == OperandSize_16)
  {
    uint16 mem_address = cpu->ReadMemoryWord(instruction->segment, base_address);
    uint16 mem_segment_selector = cpu->ReadMemoryWord(instruction->segment, base_address + 2);
    *address = ZeroExtend32(mem_address);
    *segment_selector = mem_segment_selector;
  }
  else
  {
    uint32 mem_address = cpu->ReadMemoryDWord(instruction->segment, base_address);
    uint16 mem_segment_selector = cpu->ReadMemoryWord(instruction->segment, base_address + 4);
    *address = ZeroExtend32(mem_address);
    *segment_selector = mem_segment_selector;
  }
}

void Interpreter::WriteByteOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand,
                                   uint8 value)
{
  if (operand->mode == AddressingMode_Register)
  {
    cpu->m_registers.reg8[operand->reg.reg8] = value;
    return;
  }

  VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, operand);
  cpu->WriteMemoryByte(instruction->segment, address, value);
}

void Interpreter::WriteWordOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand,
                                   uint16 value)
{
  if (operand->mode == AddressingMode_Register)
  {
    DebugAssert(operand->reg.reg16 < Reg16_Count);
    cpu->m_registers.reg16[operand->reg.reg16] = value;
    return;
  }
  else if (operand->mode == AddressingMode_SegmentRegister)
  {
    DebugAssert(operand->reg.sreg < Segment_Count);
    cpu->LoadSegmentRegister(operand->reg.sreg, value);
    return;
  }

  VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, operand);
  cpu->WriteMemoryWord(instruction->segment, address, value);
}

void Interpreter::WriteDWordOperand(CPU* cpu, const OldInstruction* instruction, const OldInstruction::Operand* operand,
                                    uint32 value)
{
  if (operand->mode == AddressingMode_Register)
  {
    DebugAssert(operand->reg.reg32 < Reg16_Count);
    cpu->m_registers.reg32[operand->reg.reg32] = value;
    return;
  }

  VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, operand);
  cpu->WriteMemoryDWord(instruction->segment, address, value);
}

bool Interpreter::TestCondition(CPU* cpu, JumpCondition condition, AddressSize address_size)
{
  switch (condition)
  {
    case JumpCondition_Overflow:
      return cpu->m_registers.EFLAGS.OF;

    case JumpCondition_NotOverflow:
      return !cpu->m_registers.EFLAGS.OF;

    case JumpCondition_Sign:
      return cpu->m_registers.EFLAGS.SF;

    case JumpCondition_NotSign:
      return !cpu->m_registers.EFLAGS.SF;

    case JumpCondition_Equal:
      return cpu->m_registers.EFLAGS.ZF;

    case JumpCondition_NotEqual:
      return !cpu->m_registers.EFLAGS.ZF;

    case JumpCondition_Below:
      return cpu->m_registers.EFLAGS.CF;

    case JumpCondition_AboveOrEqual:
      return !cpu->m_registers.EFLAGS.CF;

    case JumpCondition_BelowOrEqual:
      return (cpu->m_registers.EFLAGS.CF | cpu->m_registers.EFLAGS.ZF);

    case JumpCondition_Above:
      return !(cpu->m_registers.EFLAGS.CF | cpu->m_registers.EFLAGS.ZF);

    case JumpCondition_Less:
      return (cpu->m_registers.EFLAGS.SF != cpu->m_registers.EFLAGS.OF);

    case JumpCondition_GreaterOrEqual:
      return (cpu->m_registers.EFLAGS.SF == cpu->m_registers.EFLAGS.OF);

    case JumpCondition_LessOrEqual:
      return (cpu->m_registers.EFLAGS.ZF || (cpu->m_registers.EFLAGS.SF != cpu->m_registers.EFLAGS.OF));

    case JumpCondition_Greater:
      return (!cpu->m_registers.EFLAGS.ZF && (cpu->m_registers.EFLAGS.SF == cpu->m_registers.EFLAGS.OF));

    case JumpCondition_Parity:
      return cpu->m_registers.EFLAGS.PF;

    case JumpCondition_NotParity:
      return !cpu->m_registers.EFLAGS.PF;

    case JumpCondition_CXZero:
    {
      if (address_size == AddressSize_16)
        return (cpu->m_registers.CX == 0);
      else
        return (cpu->m_registers.ECX == 0);
    }

    default:
      Panic("Unhandled jump condition");
      return false;
  }
}

constexpr bool IsSign(uint8 value)
{
  return !!(value >> 7);
}
constexpr bool IsSign(uint16 value)
{
  return !!(value >> 15);
}
constexpr bool IsSign(uint32 value)
{
  return !!(value >> 31);
}
template<typename T>
constexpr bool IsZero(T value)
{
  return (value == 0);
}
template<typename T>
constexpr bool IsParity(T value)
{
  return static_cast<bool>(~Y_popcnt(static_cast<uint8>(value & 0xFF)) & 1);
}
template<typename T>
constexpr bool IsAdjust(T old_value, T new_value)
{
  return (old_value & 0xF) < (new_value & 0xF);
}

inline uint8 ALUOp_Add8(CPU::Registers* registers, uint8 lhs, uint8 rhs)
{
  uint16 old_value = lhs;
  uint16 add_value = rhs;
  uint16 new_value = old_value + add_value;
  uint8 out_value = uint8(new_value & 0xFF);

  registers->EFLAGS.CF = ((new_value & 0xFF00) != 0);
  registers->EFLAGS.OF = ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x80) == 0x80);
  registers->EFLAGS.AF = (((old_value ^ add_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

inline uint8 ALUOp_Adc8(CPU::Registers* registers, uint8 lhs, uint8 rhs)
{
  uint16 old_value = lhs;
  uint16 add_value = rhs;
  uint16 carry_in = (registers->EFLAGS.CF) ? 1 : 0;
  uint16 new_value = old_value + add_value + carry_in;
  uint8 out_value = uint8(new_value & 0xFF);

  registers->EFLAGS.CF = ((new_value & 0xFF00) != 0);
  registers->EFLAGS.OF = ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x80) == 0x80);
  registers->EFLAGS.AF = (((old_value ^ add_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

inline uint8 ALUOp_Sub8(CPU::Registers* registers, uint8 lhs, uint8 rhs)
{
  uint16 old_value = lhs;
  uint16 sub_value = rhs;
  uint16 new_value = old_value - sub_value;
  uint8 out_value = uint8(new_value & 0xFF);

  registers->EFLAGS.CF = ((new_value & 0xFF00) != 0);
  registers->EFLAGS.OF = ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x80) == 0x80);
  registers->EFLAGS.AF = (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

inline uint8 ALUOp_Sbb8(CPU::Registers* registers, uint8 lhs, uint8 rhs)
{
  uint16 old_value = lhs;
  uint16 sub_value = rhs;
  uint16 carry_in = registers->EFLAGS.CF ? 1 : 0;
  uint16 new_value = old_value - sub_value - carry_in;
  uint8 out_value = uint8(new_value & 0xFF);

  registers->EFLAGS.CF = ((new_value & 0xFF00) != 0);
  registers->EFLAGS.OF = ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x80) == 0x80);
  registers->EFLAGS.AF = (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

inline uint16 ALUOp_Add16(CPU::Registers* registers, uint16 lhs, uint16 rhs)
{
  uint32 old_value = lhs;
  uint32 add_value = rhs;
  uint32 new_value = old_value + add_value;
  uint16 out_value = uint16(new_value & 0xFFFF);

  registers->EFLAGS.CF = ((new_value & 0xFFFF0000) != 0);
  registers->EFLAGS.OF = ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x8000) == 0x8000);
  registers->EFLAGS.AF = (((old_value ^ add_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

inline uint16 ALUOp_Adc16(CPU::Registers* registers, uint16 lhs, uint16 rhs)
{
  uint32 old_value = lhs;
  uint32 add_value = rhs;
  uint32 carry_in = (registers->EFLAGS.CF) ? 1 : 0;
  uint32 new_value = old_value + add_value + carry_in;
  uint16 out_value = uint16(new_value & 0xFFFF);

  registers->EFLAGS.CF = ((new_value & 0xFFFF0000) != 0);
  registers->EFLAGS.OF = ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x8000) == 0x8000);
  registers->EFLAGS.AF = (((old_value ^ add_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

inline uint16 ALUOp_Sub16(CPU::Registers* registers, uint16 lhs, uint16 rhs)
{
  uint32 old_value = lhs;
  uint32 sub_value = rhs;
  uint32 new_value = old_value - sub_value;
  uint16 out_value = uint16(new_value & 0xFFFF);

  registers->EFLAGS.CF = ((new_value & 0xFFFF0000) != 0);
  registers->EFLAGS.OF = ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x8000) == 0x8000);
  registers->EFLAGS.AF = (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

inline uint16 ALUOp_Sbb16(CPU::Registers* registers, uint16 lhs, uint16 rhs)
{
  uint32 old_value = lhs;
  uint32 sub_value = rhs;
  uint32 carry_in = registers->EFLAGS.CF ? 1 : 0;
  uint32 new_value = old_value - sub_value - carry_in;
  uint16 out_value = uint16(new_value & 0xFFFF);

  registers->EFLAGS.CF = ((new_value & 0xFFFF0000) != 0);
  registers->EFLAGS.OF = ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x8000) == 0x8000);
  registers->EFLAGS.AF = (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

inline uint32 ALUOp_Add32(CPU::Registers* registers, uint32 lhs, uint32 rhs)
{
  uint64 old_value = ZeroExtend64(lhs);
  uint64 add_value = ZeroExtend64(rhs);
  uint64 new_value = old_value + add_value;
  uint32 out_value = Truncate32(new_value);

  registers->EFLAGS.CF = ((new_value & UINT64_C(0xFFFFFFFF00000000)) != 0);
  registers->EFLAGS.OF =
    ((((new_value ^ old_value) & (new_value ^ add_value)) & UINT64_C(0x80000000)) == UINT64_C(0x80000000));
  registers->EFLAGS.AF = (((old_value ^ add_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

inline uint32 ALUOp_Adc32(CPU::Registers* registers, uint32 lhs, uint32 rhs)
{
  uint64 old_value = ZeroExtend64(lhs);
  uint64 add_value = ZeroExtend64(rhs);
  uint64 carry_in = (registers->EFLAGS.CF) ? 1 : 0;
  uint64 new_value = old_value + add_value + carry_in;
  uint32 out_value = Truncate32(new_value);

  registers->EFLAGS.CF = ((new_value & UINT64_C(0xFFFFFFFF00000000)) != 0);
  registers->EFLAGS.OF =
    ((((new_value ^ old_value) & (new_value ^ add_value)) & UINT64_C(0x80000000)) == UINT64_C(0x80000000));
  registers->EFLAGS.AF = (((old_value ^ add_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

inline uint32 ALUOp_Sub32(CPU::Registers* registers, uint32 lhs, uint32 rhs)
{
  uint64 old_value = ZeroExtend64(lhs);
  uint64 sub_value = ZeroExtend64(rhs);
  uint64 new_value = old_value - sub_value;
  uint32 out_value = Truncate32(new_value);

  registers->EFLAGS.CF = ((new_value & UINT64_C(0xFFFFFFFF00000000)) != 0);
  registers->EFLAGS.OF =
    ((((new_value ^ old_value) & (old_value ^ sub_value)) & UINT64_C(0x80000000)) == UINT64_C(0x80000000));
  registers->EFLAGS.AF = (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

inline uint32 ALUOp_Sbb32(CPU::Registers* registers, uint32 lhs, uint32 rhs)
{
  uint64 old_value = ZeroExtend64(lhs);
  uint64 sub_value = ZeroExtend64(rhs);
  uint64 carry_in = registers->EFLAGS.CF ? 1 : 0;
  uint64 new_value = old_value - sub_value - carry_in;
  uint32 out_value = Truncate32(new_value);

  registers->EFLAGS.CF = ((new_value & UINT64_C(0xFFFFFFFF00000000)) != 0);
  registers->EFLAGS.OF =
    ((((new_value ^ old_value) & (old_value ^ sub_value)) & UINT64_C(0x80000000)) == UINT64_C(0x80000000));
  registers->EFLAGS.AF = (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10);
  registers->EFLAGS.SF = IsSign(out_value);
  registers->EFLAGS.ZF = IsZero(out_value);
  registers->EFLAGS.PF = IsParity(out_value);

  return out_value;
}

static uint64 operation_count[Operation_Count];

void Interpreter::ExecuteInstruction(CPU* cpu, const OldInstruction* instruction)
{
  operation_count[instruction->operation]++;

  // Handle repeat prefixes.
  // This only applies to a few instructions.
  if (instruction->flags & InstructionFlag_Rep)
  {
    // Use CX instead of ECX in 16-bit mode.
    // Don't execute a single iteration if it's zero.
    if (instruction->address_size == AddressSize_16)
    {
      if (cpu->m_registers.CX == 0)
        return;

      DispatchInstruction(cpu, instruction);
      cpu->m_registers.CX--;
    }
    else
    {
      if (cpu->m_registers.ECX == 0)
        return;

      DispatchInstruction(cpu, instruction);
      cpu->m_registers.ECX--;
    }

    // Check for the termination condition.
    bool terminate_counter =
      (instruction->address_size == AddressSize_16) ? (cpu->m_registers.CX == 0) : (cpu->m_registers.ECX == 0);
    bool terminate_equal = (instruction->flags & InstructionFlag_RepEqual && !cpu->m_registers.EFLAGS.ZF);
    bool terminate_notequal = (instruction->flags & InstructionFlag_RepNotEqual && cpu->m_registers.EFLAGS.ZF);
    if (!(terminate_counter | terminate_equal | terminate_notequal))
    {
      // Loop is not terminating. Restore IP to the original value so we can re-run the instruction.
      cpu->RestartCurrentInstruction();
    }
  }
  else
  {
    DispatchInstruction(cpu, instruction);
  }
}

void Interpreter::DispatchInstruction(CPU* cpu, const OldInstruction* instruction)
{
  switch (instruction->operation)
  {
    case Operation_NOP:
      Execute_NOP(cpu, instruction);
      break;
    case Operation_MOV:
      Execute_MOV(cpu, instruction);
      break;
    case Operation_MOVS:
      Execute_MOVS(cpu, instruction);
      break;
    case Operation_LODS:
      Execute_LODS(cpu, instruction);
      break;
    case Operation_STOS:
      Execute_STOS(cpu, instruction);
      break;
    case Operation_MOV_Sreg:
      Execute_MOV_Sreg(cpu, instruction);
      break;
    case Operation_LDS:
      Execute_LDS(cpu, instruction);
      break;
    case Operation_LES:
      Execute_LES(cpu, instruction);
      break;
    case Operation_LSS:
      Execute_LSS(cpu, instruction);
      break;
    case Operation_LFS:
      Execute_LFS(cpu, instruction);
      break;
    case Operation_LGS:
      Execute_LGS(cpu, instruction);
      break;
    case Operation_LEA:
      Execute_LEA(cpu, instruction);
      break;
    case Operation_XCHG:
      Execute_XCHG(cpu, instruction);
      break;
    case Operation_XLAT:
      Execute_XLAT(cpu, instruction);
      break;
    case Operation_CBW:
      Execute_CBW(cpu, instruction);
      break;
    case Operation_CWD:
      Execute_CWD(cpu, instruction);
      break;
    case Operation_PUSH:
      Execute_PUSH(cpu, instruction);
      break;
    case Operation_PUSHA:
      Execute_PUSHA(cpu, instruction);
      break;
    case Operation_PUSHF:
      Execute_PUSHF(cpu, instruction);
      break;
    case Operation_POP:
      Execute_POP(cpu, instruction);
      break;
    case Operation_POPA:
      Execute_POPA(cpu, instruction);
      break;
    case Operation_POPF:
      Execute_POPF(cpu, instruction);
      break;
    case Operation_LAHF:
      Execute_LAHF(cpu, instruction);
      break;
    case Operation_SAHF:
      Execute_SAHF(cpu, instruction);
      break;
    case Operation_Jcc:
      Execute_Jcc(cpu, instruction);
      break;
    case Operation_LOOP:
      Execute_LOOP(cpu, instruction);
      break;
    case Operation_JMP_Near:
      Execute_JMP_Near(cpu, instruction);
      break;
    case Operation_JMP_Far:
      Execute_JMP_Far(cpu, instruction);
      break;
    case Operation_CALL_Near:
      Execute_CALL_Near(cpu, instruction);
      break;
    case Operation_CALL_Far:
      Execute_CALL_Far(cpu, instruction);
      break;
    case Operation_RET_Near:
      Execute_RET_Near(cpu, instruction);
      break;
    case Operation_RET_Far:
      Execute_RET_Far(cpu, instruction);
      break;
    case Operation_INT:
      Execute_INT(cpu, instruction);
      break;
    case Operation_INTO:
      Execute_INTO(cpu, instruction);
      break;
    case Operation_IRET:
      Execute_IRET(cpu, instruction);
      break;
    case Operation_HLT:
      Execute_HLT(cpu, instruction);
      break;
    case Operation_IN:
      Execute_IN(cpu, instruction);
      break;
    case Operation_INS:
      Execute_INS(cpu, instruction);
      break;
    case Operation_OUT:
      Execute_OUT(cpu, instruction);
      break;
    case Operation_OUTS:
      Execute_OUTS(cpu, instruction);
      break;
    case Operation_CLC:
      Execute_CLC(cpu, instruction);
      break;
    case Operation_CLD:
      Execute_CLD(cpu, instruction);
      break;
    case Operation_CLI:
      Execute_CLI(cpu, instruction);
      break;
    case Operation_STC:
      Execute_STC(cpu, instruction);
      break;
    case Operation_STD:
      Execute_STD(cpu, instruction);
      break;
    case Operation_STI:
      Execute_STI(cpu, instruction);
      break;
    case Operation_CMC:
      Execute_CMC(cpu, instruction);
      break;
    case Operation_INC:
      Execute_INC(cpu, instruction);
      break;
    case Operation_ADD:
      Execute_ADD(cpu, instruction);
      break;
    case Operation_ADC:
      Execute_ADC(cpu, instruction);
      break;
    case Operation_DEC:
      Execute_DEC(cpu, instruction);
      break;
    case Operation_SUB:
      Execute_SUB(cpu, instruction);
      break;
    case Operation_SBB:
      Execute_SBB(cpu, instruction);
      break;
    case Operation_CMP:
      Execute_CMP(cpu, instruction);
      break;
    case Operation_CMPS:
      Execute_CMPS(cpu, instruction);
      break;
    case Operation_SCAS:
      Execute_SCAS(cpu, instruction);
      break;
    case Operation_MUL:
      Execute_MUL(cpu, instruction);
      break;
    case Operation_IMUL:
      Execute_IMUL(cpu, instruction);
      break;
    case Operation_DIV:
      Execute_DIV(cpu, instruction);
      break;
    case Operation_IDIV:
      Execute_IDIV(cpu, instruction);
      break;
    case Operation_SHL:
      Execute_SHL(cpu, instruction);
      break;
    case Operation_SHR:
      Execute_SHR(cpu, instruction);
      break;
    case Operation_SAR:
      Execute_SAR(cpu, instruction);
      break;
    case Operation_RCL:
      Execute_RCL(cpu, instruction);
      break;
    case Operation_RCR:
      Execute_RCR(cpu, instruction);
      break;
    case Operation_ROL:
      Execute_ROL(cpu, instruction);
      break;
    case Operation_ROR:
      Execute_ROR(cpu, instruction);
      break;
    case Operation_AND:
      Execute_AND(cpu, instruction);
      break;
    case Operation_OR:
      Execute_OR(cpu, instruction);
      break;
    case Operation_XOR:
      Execute_XOR(cpu, instruction);
      break;
    case Operation_TEST:
      Execute_TEST(cpu, instruction);
      break;
    case Operation_NEG:
      Execute_NEG(cpu, instruction);
      break;
    case Operation_NOT:
      Execute_NOT(cpu, instruction);
      break;
    case Operation_AAA:
      Execute_AAA(cpu, instruction);
      break;
    case Operation_AAS:
      Execute_AAS(cpu, instruction);
      break;
    case Operation_AAM:
      Execute_AAM(cpu, instruction);
      break;
    case Operation_AAD:
      Execute_AAD(cpu, instruction);
      break;
    case Operation_DAA:
      Execute_DAA(cpu, instruction);
      break;
    case Operation_DAS:
      Execute_DAS(cpu, instruction);
      break;
    case Operation_BT:
      Execute_BTx(cpu, instruction);
      break;
    case Operation_BTS:
      Execute_BTx(cpu, instruction);
      break;
    case Operation_BTC:
      Execute_BTx(cpu, instruction);
      break;
    case Operation_BTR:
      Execute_BTx(cpu, instruction);
      break;
    case Operation_BSF:
      Execute_BSF(cpu, instruction);
      break;
    case Operation_BSR:
      Execute_BSR(cpu, instruction);
      break;
    case Operation_SHLD:
      Execute_SHLD(cpu, instruction);
      break;
    case Operation_SHRD:
      Execute_SHRD(cpu, instruction);
      break;
    case Operation_LGDT:
      Execute_LGDT(cpu, instruction);
      break;
    case Operation_LIDT:
      Execute_LIDT(cpu, instruction);
      break;
    case Operation_SGDT:
      Execute_SGDT(cpu, instruction);
      break;
    case Operation_SIDT:
      Execute_SIDT(cpu, instruction);
      break;
    case Operation_LMSW:
      Execute_LMSW(cpu, instruction);
      break;
    case Operation_SMSW:
      Execute_SMSW(cpu, instruction);
      break;
    case Operation_LLDT:
      Execute_LLDT(cpu, instruction);
      break;
    case Operation_SLDT:
      Execute_SLDT(cpu, instruction);
      break;
    case Operation_LTR:
      Execute_LTR(cpu, instruction);
      break;
    case Operation_STR:
      Execute_STR(cpu, instruction);
      break;
    case Operation_BOUND:
      Execute_BOUND(cpu, instruction);
      break;
    case Operation_WAIT:
      Execute_WAIT(cpu, instruction);
      break;
    case Operation_ENTER:
      Execute_ENTER(cpu, instruction);
      break;
    case Operation_LEAVE:
      Execute_LEAVE(cpu, instruction);
      break;
    case Operation_CLTS:
      Execute_CLTS(cpu, instruction);
      break;
    case Operation_LAR:
      Execute_LAR(cpu, instruction);
      break;
    case Operation_LSL:
      Execute_LSL(cpu, instruction);
      break;
    case Operation_VERR:
      Execute_VERx(cpu, instruction);
      break;
    case Operation_VERW:
      Execute_VERx(cpu, instruction);
      break;
    case Operation_ARPL:
      Execute_ARPL(cpu, instruction);
      break;
    case Operation_LOADALL_286:
      Execute_LOADALL_286(cpu, instruction);
      break;
    case Operation_MOVSX:
      Execute_MOVSX(cpu, instruction);
      break;
    case Operation_MOVZX:
      Execute_MOVZX(cpu, instruction);
      break;
    case Operation_MOV_CR:
      Execute_MOV_CR(cpu, instruction);
      break;
    case Operation_MOV_DR:
      Execute_MOV_DR(cpu, instruction);
      break;
    case Operation_MOV_TR:
      Execute_MOV_TR(cpu, instruction);
      break;
    case Operation_SETcc:
      Execute_SETcc(cpu, instruction);
      break;
    case Operation_BSWAP:
      Execute_BSWAP(cpu, instruction);
      break;
    case Operation_CMPXCHG:
      Execute_CMPXCHG(cpu, instruction);
      break;
    case Operation_CMOVcc:
      Execute_CMOVcc(cpu, instruction);
      break;
    case Operation_WBINVD:
      Execute_WBINVD(cpu, instruction);
      break;
    case Operation_INVLPG:
      Execute_INVLPG(cpu, instruction);
      break;
    case Operation_XADD:
      Execute_XADD(cpu, instruction);
      break;
    case Operation_RDTSC:
      Execute_RDTSC(cpu, instruction);
      break;
      // x87
    case Operation_FNINIT:
      Execute_FNINIT(cpu, instruction);
      break;
    case Operation_FSETPM:
      Execute_FSETPM(cpu, instruction);
      break;
    case Operation_FNSTCW:
      Execute_FNSTCW(cpu, instruction);
      break;
    case Operation_FNSTSW:
      Execute_FNSTSW(cpu, instruction);
      break;
    case Operation_FNCLEX:
      Execute_FNCLEX(cpu, instruction);
      break;
    case Operation_FLDCW:
      Execute_FLDCW(cpu, instruction);
      break;
    case Operation_FLD:
      Execute_FLD(cpu, instruction);
      break;
    case Operation_FLD1:
      Execute_FLD1(cpu, instruction);
      break;
    case Operation_FLDZ:
      Execute_FLDZ(cpu, instruction);
      break;
    case Operation_FST:
    case Operation_FSTP:
      Execute_FST(cpu, instruction);
      break;
    case Operation_FADD:
    case Operation_FADDP:
      Execute_FADD(cpu, instruction);
      break;
    case Operation_FSUB:
    case Operation_FSUBP:
      Execute_FSUB(cpu, instruction);
      break;
    case Operation_FSUBR:
    case Operation_FSUBRP:
      Execute_FSUBR(cpu, instruction);
      break;
    case Operation_FMUL:
    case Operation_FMULP:
      Execute_FMUL(cpu, instruction);
      break;
    case Operation_FDIV:
    case Operation_FDIVP:
      Execute_FDIV(cpu, instruction);
      break;
    case Operation_FDIVR:
    case Operation_FDIVRP:
      Execute_FDIVR(cpu, instruction);
      break;
    case Operation_FCOM:
    case Operation_FCOMP:
    case Operation_FCOMPP:
    case Operation_FUCOM:
    case Operation_FUCOMP:
    case Operation_FUCOMPP:
      Execute_FCOM(cpu, instruction);
      break;
    case Operation_FILD:
      Execute_FILD(cpu, instruction);
      break;
    case Operation_FIST:
    case Operation_FISTP:
      Execute_FIST(cpu, instruction);
      break;
    case Operation_FIDIV:
      Execute_FIDIV(cpu, instruction);
      break;
    case Operation_FABS:
      Execute_FABS(cpu, instruction);
      break;
    case Operation_FCHS:
      Execute_FCHS(cpu, instruction);
      break;
    case Operation_FFREE:
      Execute_FFREE(cpu, instruction);
      break;
    case Operation_FXAM:
      Execute_FXAM(cpu, instruction);
      break;
    case Operation_FXCH:
      Execute_FXCH(cpu, instruction);
      break;

    default:
      Panic("Unhandled operation");
      break;
  }
}

void Interpreter::Execute_NOP(CPU* cpu, const OldInstruction* instruction)
{
  // Do nothing, update no flags
}

void Interpreter::Execute_MOV(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    WriteByteOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_MOVS(CPU* cpu, const OldInstruction* instruction)
{
  // The DS segment may be over-ridden with a segment override prefix, but the ES segment cannot be overridden.
  Segment src_segment = instruction->segment;
  VirtualMemoryAddress src_address =
    (instruction->address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
  VirtualMemoryAddress dst_address =
    (instruction->address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
  uint8 data_size;

  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = cpu->ReadMemoryByte(src_segment, src_address);
    cpu->WriteMemoryByte(Segment_ES, dst_address, value);
    data_size = sizeof(uint8);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = cpu->ReadMemoryWord(src_segment, src_address);
    cpu->WriteMemoryWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint16);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = cpu->ReadMemoryDWord(src_segment, src_address);
    cpu->WriteMemoryDWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (instruction->address_size == AddressSize_16)
  {
    if (!cpu->m_registers.EFLAGS.DF)
    {
      cpu->m_registers.SI += ZeroExtend16(data_size);
      cpu->m_registers.DI += ZeroExtend16(data_size);
    }
    else
    {
      cpu->m_registers.SI -= ZeroExtend16(data_size);
      cpu->m_registers.DI -= ZeroExtend16(data_size);
    }
  }
  else
  {
    if (!cpu->m_registers.EFLAGS.DF)
    {
      cpu->m_registers.ESI += ZeroExtend32(data_size);
      cpu->m_registers.EDI += ZeroExtend32(data_size);
    }
    else
    {
      cpu->m_registers.ESI -= ZeroExtend32(data_size);
      cpu->m_registers.EDI -= ZeroExtend32(data_size);
    }
  }
}

void Interpreter::Execute_LODS(CPU* cpu, const OldInstruction* instruction)
{
  Segment segment = instruction->segment;
  VirtualMemoryAddress src_address =
    (instruction->address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
  uint8 data_size;

  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = cpu->ReadMemoryByte(segment, src_address);
    cpu->m_registers.AL = value;
    data_size = sizeof(uint8);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = cpu->ReadMemoryWord(segment, src_address);
    cpu->m_registers.AX = value;
    data_size = sizeof(uint16);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = cpu->ReadMemoryDWord(segment, src_address);
    cpu->m_registers.EAX = value;
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (instruction->address_size == AddressSize_16)
  {
    if (!cpu->m_registers.EFLAGS.DF)
      cpu->m_registers.SI += ZeroExtend16(data_size);
    else
      cpu->m_registers.SI -= ZeroExtend16(data_size);
  }
  else
  {
    if (!cpu->m_registers.EFLAGS.DF)
      cpu->m_registers.ESI += ZeroExtend32(data_size);
    else
      cpu->m_registers.ESI -= ZeroExtend32(data_size);
  }
}

void Interpreter::Execute_STOS(CPU* cpu, const OldInstruction* instruction)
{
  VirtualMemoryAddress dst_address =
    (instruction->address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
  uint8 data_size;

  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = cpu->m_registers.AL;
    cpu->WriteMemoryByte(Segment_ES, dst_address, value);
    data_size = sizeof(uint8);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = cpu->m_registers.AX;
    cpu->WriteMemoryWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint16);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = cpu->m_registers.EAX;
    cpu->WriteMemoryDWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (instruction->address_size == AddressSize_16)
  {
    if (!cpu->m_registers.EFLAGS.DF)
      cpu->m_registers.DI += ZeroExtend16(data_size);
    else
      cpu->m_registers.DI -= ZeroExtend16(data_size);
  }
  else
  {
    if (!cpu->m_registers.EFLAGS.DF)
      cpu->m_registers.EDI += ZeroExtend32(data_size);
    else
      cpu->m_registers.EDI -= ZeroExtend32(data_size);
  }
}

void Interpreter::Execute_MOV_Sreg(CPU* cpu, const OldInstruction* instruction)
{
  // Load segment register
  if (instruction->operands[0].mode == AddressingMode_SegmentRegister)
  {
    DebugAssert(instruction->operands[1].size == OperandSize_16);
    uint16 selector = ReadWordOperand(cpu, instruction, &instruction->operands[1]);

    DebugAssert(instruction->operands[0].reg.sreg < Segment_Count);
    cpu->LoadSegmentRegister(instruction->operands[0].reg.sreg, selector);
  }
  // Store segment register
  else
  {
    DebugAssert(instruction->operands[1].mode == AddressingMode_SegmentRegister);
    DebugAssert(instruction->operands[1].reg.sreg < Segment_Count);
    uint16 selector = cpu->m_registers.segment_selectors[instruction->operands[1].reg.sreg];

    // When executing MOV Reg, Sreg, the processor copies the content of Sreg to the 16 least significant bits of the
    // general-purpose register. The upper bits of the destination register are zero for most IA-32 processors (Pentium
    // Pro processors and later) and all Intel 64 processors, with the exception that bits 31:16 are undefined for Intel
    // Quark X1000 processors, Pentium and earlier processors.
    if (instruction->operand_size == OperandSize_32 && instruction->operands[0].mode == AddressingMode_Register)
    {
      DebugAssert(instruction->operands[0].reg.reg32 < Reg32_Count);
      cpu->m_registers.reg32[instruction->operands[0].reg.reg32] = ZeroExtend32(selector);
    }
    else
    {
      DebugAssert(instruction->operands[0].size == OperandSize_16);
      WriteWordOperand(cpu, instruction, &instruction->operands[0], selector);
    }
  }
}

void Interpreter::Execute_LDS(CPU* cpu, const OldInstruction* instruction)
{
  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand(cpu, instruction, &instruction->operands[1], &segment_selector, &address);

  cpu->LoadSegmentRegister(Segment_DS, segment_selector);

  if (instruction->operands[0].size == OperandSize_16)
    WriteWordOperand(cpu, instruction, &instruction->operands[0], Truncate16(address));
  else if (instruction->operands[0].size == OperandSize_32)
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], address);
  else
    DebugUnreachableCode();
}

void Interpreter::Execute_LES(CPU* cpu, const OldInstruction* instruction)
{
  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand(cpu, instruction, &instruction->operands[1], &segment_selector, &address);

  cpu->LoadSegmentRegister(Segment_ES, segment_selector);

  if (instruction->operands[0].size == OperandSize_16)
    WriteWordOperand(cpu, instruction, &instruction->operands[0], Truncate16(address));
  else if (instruction->operands[0].size == OperandSize_32)
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], address);
  else
    DebugUnreachableCode();
}

void Interpreter::Execute_LSS(CPU* cpu, const OldInstruction* instruction)
{
  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand(cpu, instruction, &instruction->operands[1], &segment_selector, &address);

  cpu->LoadSegmentRegister(Segment_SS, segment_selector);

  if (instruction->operands[0].size == OperandSize_16)
    WriteWordOperand(cpu, instruction, &instruction->operands[0], Truncate16(address));
  else if (instruction->operands[0].size == OperandSize_32)
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], address);
  else
    DebugUnreachableCode();
}

void Interpreter::Execute_LFS(CPU* cpu, const OldInstruction* instruction)
{
  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand(cpu, instruction, &instruction->operands[1], &segment_selector, &address);

  cpu->LoadSegmentRegister(Segment_FS, segment_selector);

  if (instruction->operands[0].size == OperandSize_16)
    WriteWordOperand(cpu, instruction, &instruction->operands[0], Truncate16(address));
  else if (instruction->operands[0].size == OperandSize_32)
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], address);
  else
    DebugUnreachableCode();
}

void Interpreter::Execute_LGS(CPU* cpu, const OldInstruction* instruction)
{
  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand(cpu, instruction, &instruction->operands[1], &segment_selector, &address);

  cpu->LoadSegmentRegister(Segment_GS, segment_selector);

  if (instruction->operands[0].size == OperandSize_16)
    WriteWordOperand(cpu, instruction, &instruction->operands[0], Truncate16(address));
  else if (instruction->operands[0].size == OperandSize_32)
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], address);
  else
    DebugUnreachableCode();
}

void Interpreter::Execute_LEA(CPU* cpu, const OldInstruction* instruction)
{
  // Calculate full address in instruction's address mode, truncate/extend to operand size.
  VirtualMemoryAddress effective_address = CalculateEffectiveAddress(cpu, instruction, &instruction->operands[1]);
  if (instruction->operand_size == OperandSize_16)
    WriteWordOperand(cpu, instruction, &instruction->operands[0], Truncate16(effective_address));
  else
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], effective_address);
}

void Interpreter::Execute_XCHG(CPU* cpu, const OldInstruction* instruction)
{
  // In memory version, memory is op0, register is op1. Memory must be written first.
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value0 = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 value1 = ReadByteOperand(cpu, instruction, &instruction->operands[1]);

    WriteByteOperand(cpu, instruction, &instruction->operands[0], value1);
    WriteByteOperand(cpu, instruction, &instruction->operands[1], value0);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value0 = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 value1 = ReadWordOperand(cpu, instruction, &instruction->operands[1]);

    WriteWordOperand(cpu, instruction, &instruction->operands[0], value1);
    WriteWordOperand(cpu, instruction, &instruction->operands[1], value0);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value0 = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 value1 = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);

    WriteDWordOperand(cpu, instruction, &instruction->operands[0], value1);
    WriteDWordOperand(cpu, instruction, &instruction->operands[1], value0);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_XLAT(CPU* cpu, const OldInstruction* instruction)
{
  uint8 value;
  if (instruction->address_size == AddressSize_16)
  {
    uint16 address = cpu->m_registers.BX + ZeroExtend16(cpu->m_registers.AL);
    value = cpu->ReadMemoryByte(instruction->segment, address);
  }
  else if (instruction->address_size == AddressSize_32)
  {
    uint32 address = cpu->m_registers.EBX + ZeroExtend32(cpu->m_registers.AL);
    value = cpu->ReadMemoryByte(instruction->segment, address);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
  cpu->m_registers.AL = value;
}

void Interpreter::Execute_CBW(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operand_size == OperandSize_16)
  {
    // Sign-extend AL to AH
    cpu->m_registers.AH = ((cpu->m_registers.AL & 0x80) != 0) ? 0xFF : 0x00;
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    // Sign-extend AX to EAX
    cpu->m_registers.EAX = SignExtend32(cpu->m_registers.AX);
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_CWD(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operand_size == OperandSize_16)
  {
    // Sign-extend AX to DX
    cpu->m_registers.DX = ((cpu->m_registers.AX & 0x8000) != 0) ? 0xFFFF : 0x0000;
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    // Sign-extend EAX to EDX
    cpu->m_registers.EDX = ((cpu->m_registers.EAX & 0x80000000) != 0) ? 0xFFFFFFFF : 0x00000000;
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_PUSH(CPU* cpu, const OldInstruction* instruction)
{
  // Operand size determines the number of bytes to write to the stack.
  if (instruction->operand_size == OperandSize_16)
  {
    // Needed because of segment registers
    uint16 value;
    if (instruction->operands[0].size == OperandSize_8)
      value = SignExtend16(ReadByteOperand(cpu, instruction, &instruction->operands[0]));
    else
      value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);

    cpu->PushWord(value);
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 value;
    if (instruction->operands[0].size == OperandSize_8)
      value = SignExtend32(ReadByteOperand(cpu, instruction, &instruction->operands[0]));
    else if (instruction->operands[0].size == OperandSize_16)
      value = SignExtend32(ReadWordOperand(cpu, instruction, &instruction->operands[0]));
    else
      value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);

    cpu->PushDWord(value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_PUSHA(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operand_size == OperandSize_16)
  {
    uint16 old_SP = cpu->m_registers.SP;
    cpu->PushWord(cpu->m_registers.AX);
    cpu->PushWord(cpu->m_registers.CX);
    cpu->PushWord(cpu->m_registers.DX);
    cpu->PushWord(cpu->m_registers.BX);
    cpu->PushWord(old_SP);
    cpu->PushWord(cpu->m_registers.BP);
    cpu->PushWord(cpu->m_registers.SI);
    cpu->PushWord(cpu->m_registers.DI);
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 old_ESP = cpu->m_registers.ESP;
    cpu->PushDWord(cpu->m_registers.EAX);
    cpu->PushDWord(cpu->m_registers.ECX);
    cpu->PushDWord(cpu->m_registers.EDX);
    cpu->PushDWord(cpu->m_registers.EBX);
    cpu->PushDWord(old_ESP);
    cpu->PushDWord(cpu->m_registers.EBP);
    cpu->PushDWord(cpu->m_registers.ESI);
    cpu->PushDWord(cpu->m_registers.EDI);
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_PUSHF(CPU* cpu, const OldInstruction* instruction)
{
  // In V8086 mode if IOPL!=3, trap to V8086 monitor
  if (cpu->InVirtual8086Mode() && cpu->GetIOPL() != 3)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // RF flag is cleared in the copy
  uint32 EFLAGS = cpu->m_registers.EFLAGS.bits;
  EFLAGS &= ~Flag_RF;

  // VM flag is never set from PUSHF
  EFLAGS &= ~Flag_VM;

  if (instruction->operand_size == OperandSize_16)
    cpu->PushWord(Truncate16(EFLAGS));
  else if (instruction->operand_size == OperandSize_32)
    cpu->PushDWord(EFLAGS);
  else
    DebugUnreachableCode();
}

void Interpreter::Execute_POP(CPU* cpu, const OldInstruction* instruction)
{
  // POP can use ESP in the address calculations, in this case the value of ESP
  // is that after the pop operation has occurred, not before.
  if (instruction->operand_size == OperandSize_16)
  {
    uint16 value = cpu->PopWord();
    WriteWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 value = cpu->PopDWord();

    // POP to segment register is also passed through this function
    if (instruction->operands[0].size == OperandSize_16)
      WriteWordOperand(cpu, instruction, &instruction->operands[0], Truncate16(value));
    else
      WriteDWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_POPA(CPU* cpu, const OldInstruction* instruction)
{
  // Assignment split from reading in case of exception.
  if (instruction->operand_size == OperandSize_16)
  {
    uint16 DI = cpu->PopWord();
    uint16 SI = cpu->PopWord();
    uint16 BP = cpu->PopWord();
    /*uint16 SP = */ cpu->PopWord();
    uint16 BX = cpu->PopWord();
    uint16 DX = cpu->PopWord();
    uint16 CX = cpu->PopWord();
    uint16 AX = cpu->PopWord();
    cpu->m_registers.DI = DI;
    cpu->m_registers.SI = SI;
    cpu->m_registers.BP = BP;
    cpu->m_registers.BX = BX;
    cpu->m_registers.DX = DX;
    cpu->m_registers.CX = CX;
    cpu->m_registers.AX = AX;
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 EDI = cpu->PopDWord();
    uint32 ESI = cpu->PopDWord();
    uint32 EBP = cpu->PopDWord();
    /*uint32 ESP = */ cpu->PopDWord();
    uint32 EBX = cpu->PopDWord();
    uint32 EDX = cpu->PopDWord();
    uint32 ECX = cpu->PopDWord();
    uint32 EAX = cpu->PopDWord();
    cpu->m_registers.EDI = EDI;
    cpu->m_registers.ESI = ESI;
    cpu->m_registers.EBP = EBP;
    cpu->m_registers.EBX = EBX;
    cpu->m_registers.EDX = EDX;
    cpu->m_registers.ECX = ECX;
    cpu->m_registers.EAX = EAX;
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_POPF(CPU* cpu, const OldInstruction* instruction)
{
  // If V8086 and IOPL!=3, trap to monitor
  if (cpu->InVirtual8086Mode() && cpu->GetIOPL() != 3)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  if (instruction->operand_size == OperandSize_16)
  {
    uint16 flags = cpu->PopWord();
    cpu->SetFlags16(flags);
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 flags = cpu->PopDWord();
    cpu->SetFlags(flags);
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_LAHF(CPU* cpu, const OldInstruction* instruction)
{
  //         // Don't clear/set all flags, only those allowed
  //     const uint16 MASK = Flag_CF | Flag_Reserved | Flag_PF | Flag_AF | Flag_ZF | Flag_SF | Flag_TF | Flag_IF |
  //     Flag_DF | Flag_OF
  //         // 286+ only?
  //         // | Flag_IOPL;
  //     ;
  //
  //     return (Truncate16(cpu->m_registers.EFLAGS.bits) & MASK);
  //     uint16 flags = GetFlags16();
  //     cpu->m_registers.AH = uint8(flags & 0xFF);

  cpu->m_registers.AH = Truncate8(cpu->m_registers.EFLAGS.bits);
}

void Interpreter::Execute_SAHF(CPU* cpu, const OldInstruction* instruction)
{
  uint16 flags = Truncate16(cpu->m_registers.EFLAGS.bits & 0xFF00) | ZeroExtend16(cpu->m_registers.AH);
  cpu->SetFlags16(flags);
}

void Interpreter::Execute_Jcc(CPU* cpu, const OldInstruction* instruction)
{
  bool take_branch = TestCondition(cpu, instruction->jump_condition, instruction->address_size);
  if (!take_branch)
    return;

  if (instruction->operand_size == OperandSize_16)
  {
    uint16 jump_address = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    cpu->BranchTo(ZeroExtend32(jump_address));
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 jump_address = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    cpu->BranchTo(jump_address);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_LOOP(CPU* cpu, const OldInstruction* instruction)
{
  uint32 count;
  if (instruction->address_size == AddressSize_16)
    count = ZeroExtend32(--cpu->m_registers.CX);
  else
    count = ZeroExtend32(--cpu->m_registers.ECX);

  bool branch;
  switch (instruction->jump_condition)
  {
    case JumpCondition_Equal:
      branch = ((cpu->m_registers.EFLAGS.ZF) && (count != 0));
      break;

    case JumpCondition_NotEqual:
      branch = ((!cpu->m_registers.EFLAGS.ZF) && (count != 0));
      break;

    default:
      branch = (count != 0);
      break;
  }

  if (!branch)
    return;

  if (instruction->operand_size == OperandSize_16)
  {
    uint16 jump_address = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    cpu->BranchTo(ZeroExtend32(jump_address));
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 jump_address = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    cpu->BranchTo(jump_address);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_JMP_Near(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operand_size == OperandSize_16)
  {
    uint16 jump_address = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    cpu->BranchTo(ZeroExtend32(jump_address));
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 jump_address = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    cpu->BranchTo(jump_address);
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_JMP_Far(CPU* cpu, const OldInstruction* instruction)
{
  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand(cpu, instruction, &instruction->operands[0], &segment_selector, &address);
  cpu->FarJump(segment_selector, address, instruction->operand_size);
}

void Interpreter::Execute_CALL_Near(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operand_size == OperandSize_16)
  {
    uint32 address = ZeroExtend32(ReadWordOperand(cpu, instruction, &instruction->operands[0]));
    cpu->PushWord(Truncate16(cpu->m_registers.EIP));
    cpu->BranchTo(address);
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 address = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    cpu->PushDWord(cpu->m_registers.EIP);
    cpu->BranchTo(address);
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_CALL_Far(CPU* cpu, const OldInstruction* instruction)
{
  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand(cpu, instruction, &instruction->operands[0], &segment_selector, &address);
  cpu->FarCall(segment_selector, address, instruction->operand_size);
}

void Interpreter::Execute_RET_Near(CPU* cpu, const OldInstruction* instruction)
{
  uint32 pop_count = 0;
  if (instruction->operands[0].mode != AddressingMode_None)
    pop_count = ZeroExtend32(ReadWordOperand(cpu, instruction, &instruction->operands[0]));

  uint32 return_EIP;
  if (instruction->operand_size == OperandSize_16)
  {
    return_EIP = ZeroExtend32(cpu->PopWord());
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    return_EIP = cpu->PopDWord();
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (cpu->m_stack_address_size == AddressSize_16)
    cpu->m_registers.SP += Truncate16(pop_count);
  else
    cpu->m_registers.ESP += pop_count;

  cpu->BranchTo(return_EIP);
}

void Interpreter::Execute_RET_Far(CPU* cpu, const OldInstruction* instruction)
{
  uint32 pop_count = 0;
  if (instruction->operands[0].mode != AddressingMode_None)
    pop_count = ZeroExtend32(ReadWordOperand(cpu, instruction, &instruction->operands[0]));

  cpu->FarReturn(instruction->operand_size, pop_count);
}

void Interpreter::Execute_INT(CPU* cpu, const OldInstruction* instruction)
{
  uint32 interrupt = ZeroExtend32(instruction->operands[0].immediate.value8);
  cpu->SoftwareInterrupt(instruction->operand_size, interrupt);
}

void Interpreter::Execute_INTO(CPU* cpu, const OldInstruction* instruction)
{
  // Call overflow exception if OF is set
  if (cpu->m_registers.EFLAGS.OF)
    cpu->RaiseException(Interrupt_Overflow);
}

void Interpreter::Execute_IRET(CPU* cpu, const OldInstruction* instruction)
{
  cpu->InterruptReturn(instruction->operand_size);
}

void Interpreter::Execute_HLT(CPU* cpu, const OldInstruction* instruction)
{
  // HLT is a privileged instruction
  if ((cpu->InProtectedMode() && cpu->GetCPL() != 0) || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  cpu->SetHalted(true);
}

void Interpreter::Execute_IN(CPU* cpu, const OldInstruction* instruction)
{
  uint16 port_number = ReadWordOperand(cpu, instruction, &instruction->operands[1]);

  if (instruction->operands[0].size == OperandSize_8)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint8), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint8 value;
    cpu->m_bus->ReadIOPortByte(port_number, &value);
    WriteByteOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint16), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint16 value;
    cpu->m_bus->ReadIOPortWord(port_number, &value);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint32), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint32 value;
    cpu->m_bus->ReadIOPortDWord(port_number, &value);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_INS(CPU* cpu, const OldInstruction* instruction)
{
  VirtualMemoryAddress dst_address =
    (instruction->address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
  uint16 port_number = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
  uint8 data_size;

  if (instruction->operands[0].size == OperandSize_8)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint8), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint8 value;
    cpu->m_bus->ReadIOPortByte(port_number, &value);
    cpu->WriteMemoryByte(Segment_ES, dst_address, value);
    data_size = sizeof(uint8);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint16), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint16 value;
    cpu->m_bus->ReadIOPortWord(port_number, &value);
    cpu->WriteMemoryWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint16);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint32), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint32 value;
    cpu->m_bus->ReadIOPortDWord(port_number, &value);
    cpu->WriteMemoryDWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (instruction->address_size == AddressSize_16)
  {
    if (!cpu->m_registers.EFLAGS.DF)
      cpu->m_registers.DI += ZeroExtend16(data_size);
    else
      cpu->m_registers.DI -= ZeroExtend16(data_size);
  }
  else
  {
    if (!cpu->m_registers.EFLAGS.DF)
      cpu->m_registers.EDI += ZeroExtend32(data_size);
    else
      cpu->m_registers.EDI -= ZeroExtend32(data_size);
  }
}

void Interpreter::Execute_OUT(CPU* cpu, const OldInstruction* instruction)
{
  uint16 port_number = ReadWordOperand(cpu, instruction, &instruction->operands[0]);

  if (instruction->operands[1].size == OperandSize_8)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint8), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    cpu->m_bus->WriteIOPortByte(port_number, value);
  }
  else if (instruction->operands[1].size == OperandSize_16)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint16), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    cpu->m_bus->WriteIOPortWord(port_number, value);
  }
  else if (instruction->operands[1].size == OperandSize_32)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint32), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    cpu->m_bus->WriteIOPortDWord(port_number, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_OUTS(CPU* cpu, const OldInstruction* instruction)
{
  Segment segment = instruction->segment;
  VirtualMemoryAddress src_address =
    (instruction->address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
  uint16 port_number = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
  uint8 data_size;

  if (instruction->operands[1].size == OperandSize_8)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint8), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint8 value = cpu->ReadMemoryByte(segment, src_address);
    cpu->m_bus->WriteIOPortByte(port_number, value);
    data_size = sizeof(uint8);
  }
  else if (instruction->operands[1].size == OperandSize_16)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint16), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint16 value = cpu->ReadMemoryWord(segment, src_address);
    cpu->m_bus->WriteIOPortWord(port_number, value);
    data_size = sizeof(uint16);
  }
  else if (instruction->operands[1].size == OperandSize_32)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint32), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint32 value = cpu->ReadMemoryDWord(segment, src_address);
    cpu->m_bus->WriteIOPortDWord(port_number, value);
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (instruction->address_size == AddressSize_16)
  {
    if (!cpu->m_registers.EFLAGS.DF)
      cpu->m_registers.SI += ZeroExtend16(data_size);
    else
      cpu->m_registers.SI -= ZeroExtend16(data_size);
  }
  else
  {
    if (!cpu->m_registers.EFLAGS.DF)
      cpu->m_registers.ESI += ZeroExtend32(data_size);
    else
      cpu->m_registers.ESI -= ZeroExtend32(data_size);
  }
}

void Interpreter::Execute_CLC(CPU* cpu, const OldInstruction* instruction)
{
  cpu->m_registers.EFLAGS.CF = false;
}

void Interpreter::Execute_CLD(CPU* cpu, const OldInstruction* instruction)
{
  cpu->m_registers.EFLAGS.DF = false;
}

void Interpreter::Execute_CLI(CPU* cpu, const OldInstruction* instruction)
{
  // TODO: Delay of one instruction
  if (cpu->InProtectedMode() && cpu->GetCPL() > cpu->GetIOPL())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault);
    return;
  }

  cpu->m_registers.EFLAGS.IF = false;
}

void Interpreter::Execute_STC(CPU* cpu, const OldInstruction* instruction)
{
  cpu->m_registers.EFLAGS.CF = true;
}

void Interpreter::Execute_STD(CPU* cpu, const OldInstruction* instruction)
{
  cpu->m_registers.EFLAGS.DF = true;
}

void Interpreter::Execute_STI(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->InProtectedMode() && cpu->GetCPL() > cpu->GetIOPL())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault);
    return;
  }

  cpu->m_registers.EFLAGS.IF = true;
}

void Interpreter::Execute_CMC(CPU* cpu, const OldInstruction* instruction)
{
  cpu->m_registers.EFLAGS.CF = !cpu->m_registers.EFLAGS.CF;
}

void Interpreter::Execute_INC(CPU* cpu, const OldInstruction* instruction)
{
  // Preserve CF
  // TODO: CF won't be correct if WriteByteOperand throws an exception
  bool cf = cpu->m_registers.EFLAGS.CF;

  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 new_value = ALUOp_Add8(&cpu->m_registers, value, 1);
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 new_value = ALUOp_Add16(&cpu->m_registers, value, 1);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 new_value = ALUOp_Add32(&cpu->m_registers, value, 1);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  cpu->m_registers.EFLAGS.CF = cf;
}

void Interpreter::Execute_ADD(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rhs = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    uint8 new_value = ALUOp_Add8(&cpu->m_registers, lhs, rhs);
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 rhs = ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[1]);
    uint16 new_value = ALUOp_Add16(&cpu->m_registers, lhs, rhs);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 rhs = ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint32 new_value = ALUOp_Add32(&cpu->m_registers, lhs, rhs);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_ADC(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rhs = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    uint8 new_value = ALUOp_Adc8(&cpu->m_registers, lhs, rhs);
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 rhs = ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[1]);
    uint16 new_value = ALUOp_Adc16(&cpu->m_registers, lhs, rhs);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 rhs = ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint32 new_value = ALUOp_Adc32(&cpu->m_registers, lhs, rhs);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_SUB(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rhs = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    uint8 new_value = ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 rhs = ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[1]);
    uint16 new_value = ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 rhs = ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint32 new_value = ALUOp_Sub32(&cpu->m_registers, lhs, rhs);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_SBB(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rhs = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    uint8 new_value = ALUOp_Sbb8(&cpu->m_registers, lhs, rhs);
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 rhs = ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[1]);
    uint16 new_value = ALUOp_Sbb16(&cpu->m_registers, lhs, rhs);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 rhs = ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint32 new_value = ALUOp_Sbb32(&cpu->m_registers, lhs, rhs);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_DEC(CPU* cpu, const OldInstruction* instruction)
{
  // Preserve CF
  bool cf = cpu->m_registers.EFLAGS.CF;

  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 new_value = ALUOp_Sub8(&cpu->m_registers, value, 1);
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 new_value = ALUOp_Sub16(&cpu->m_registers, value, 1);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 new_value = ALUOp_Sub32(&cpu->m_registers, value, 1);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  cpu->m_registers.EFLAGS.CF = cf;
}

void Interpreter::Execute_CMP(CPU* cpu, const OldInstruction* instruction)
{
  // Implemented as subtract but discarding the result
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rhs = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 rhs = ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[1]);
    ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 rhs = ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[1]);
    ALUOp_Sub32(&cpu->m_registers, lhs, rhs);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_CMPS(CPU* cpu, const OldInstruction* instruction)
{
  // The DS segment may be overridden with a segment override prefix, but the ES segment cannot be overridden.
  Segment src_segment = instruction->segment;
  VirtualMemoryAddress src_address =
    (instruction->address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
  VirtualMemoryAddress dst_address =
    (instruction->address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
  uint8 data_size;

  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 lhs = cpu->ReadMemoryByte(src_segment, src_address);
    uint8 rhs = cpu->ReadMemoryByte(Segment_ES, dst_address);
    ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
    data_size = sizeof(uint8);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 lhs = cpu->ReadMemoryWord(src_segment, src_address);
    uint16 rhs = cpu->ReadMemoryWord(Segment_ES, dst_address);
    ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
    data_size = sizeof(uint16);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 lhs = cpu->ReadMemoryDWord(src_segment, src_address);
    uint32 rhs = cpu->ReadMemoryDWord(Segment_ES, dst_address);
    ALUOp_Sub32(&cpu->m_registers, lhs, rhs);
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (instruction->address_size == AddressSize_16)
  {
    if (!cpu->m_registers.EFLAGS.DF)
    {
      cpu->m_registers.SI += ZeroExtend16(data_size);
      cpu->m_registers.DI += ZeroExtend16(data_size);
    }
    else
    {
      cpu->m_registers.SI -= ZeroExtend16(data_size);
      cpu->m_registers.DI -= ZeroExtend16(data_size);
    }
  }
  else
  {
    if (!cpu->m_registers.EFLAGS.DF)
    {
      cpu->m_registers.ESI += ZeroExtend32(data_size);
      cpu->m_registers.EDI += ZeroExtend32(data_size);
    }
    else
    {
      cpu->m_registers.ESI -= ZeroExtend32(data_size);
      cpu->m_registers.EDI -= ZeroExtend32(data_size);
    }
  }
}

void Interpreter::Execute_SCAS(CPU* cpu, const OldInstruction* instruction)
{
  // The ES segment cannot be overridden with a segment override prefix.
  VirtualMemoryAddress dst_address =
    (instruction->address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
  uint8 data_size;
  switch (instruction->operands[0].size)
  {
    case OperandSize_8:
    {
      uint8 lhs = cpu->m_registers.AL;
      uint8 rhs = cpu->ReadMemoryByte(Segment_ES, dst_address);
      ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
      data_size = sizeof(uint8);
    }
    break;

    case OperandSize_16:
    {
      uint16 lhs = cpu->m_registers.AX;
      uint16 rhs = cpu->ReadMemoryWord(Segment_ES, dst_address);
      ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
      data_size = sizeof(uint16);
    }
    break;

    case OperandSize_32:
    {
      uint32 lhs = cpu->m_registers.EAX;
      uint32 rhs = cpu->ReadMemoryDWord(Segment_ES, dst_address);
      ALUOp_Sub32(&cpu->m_registers, lhs, rhs);
      data_size = sizeof(uint32);
    }
    break;

    default:
      DebugUnreachableCode();
      return;
  }

  if (instruction->address_size == AddressSize_16)
  {
    if (!cpu->m_registers.EFLAGS.DF)
      cpu->m_registers.DI += ZeroExtend16(data_size);
    else
      cpu->m_registers.DI -= ZeroExtend16(data_size);
  }
  else
  {
    if (!cpu->m_registers.EFLAGS.DF)
      cpu->m_registers.EDI += ZeroExtend32(data_size);
    else
      cpu->m_registers.EDI -= ZeroExtend32(data_size);
  }
}

void Interpreter::Execute_MUL(CPU* cpu, const OldInstruction* instruction)
{
  // The OF and CF flags are set to 0 if the upper half of the result is 0;
  // otherwise, they are set to 1. The SF, ZF, AF, and PF flags are undefined.
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint16 lhs = uint16(cpu->m_registers.AL);
    uint16 rhs = uint16(ReadByteOperand(cpu, instruction, &instruction->operands[0]));
    uint16 result = lhs * rhs;
    cpu->m_registers.AX = result;
    cpu->m_registers.EFLAGS.OF = (cpu->m_registers.AH != 0);
    cpu->m_registers.EFLAGS.CF = (cpu->m_registers.AH != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(cpu->m_registers.AL);
    cpu->m_registers.EFLAGS.ZF = IsZero(cpu->m_registers.AL);
    cpu->m_registers.EFLAGS.PF = IsParity(cpu->m_registers.AL);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint32 lhs = uint32(cpu->m_registers.AX);
    uint32 rhs = uint32(ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[0]));
    uint32 result = lhs * rhs;
    cpu->m_registers.AX = uint16(result & 0xFFFF);
    cpu->m_registers.DX = uint16(result >> 16);
    cpu->m_registers.EFLAGS.OF = (cpu->m_registers.DX != 0);
    cpu->m_registers.EFLAGS.CF = (cpu->m_registers.DX != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(cpu->m_registers.AX);
    cpu->m_registers.EFLAGS.ZF = IsZero(cpu->m_registers.AX);
    cpu->m_registers.EFLAGS.PF = IsParity(cpu->m_registers.AX);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint64 lhs = ZeroExtend64(cpu->m_registers.EAX);
    uint64 rhs = ZeroExtend64(ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[0]));
    uint64 result = lhs * rhs;
    cpu->m_registers.EAX = Truncate32(result);
    cpu->m_registers.EDX = Truncate32(result >> 32);
    cpu->m_registers.EFLAGS.OF = (cpu->m_registers.EDX != 0);
    cpu->m_registers.EFLAGS.CF = (cpu->m_registers.EDX != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(cpu->m_registers.EAX);
    cpu->m_registers.EFLAGS.ZF = IsZero(cpu->m_registers.EAX);
    cpu->m_registers.EFLAGS.PF = IsParity(cpu->m_registers.EAX);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_IMUL(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    // Two and three-operand forms do not exist for 8-bit sources
    DebugAssert(instruction->operands[1].mode == AddressingMode_None);
    DebugAssert(instruction->operands[2].mode == AddressingMode_None);

    int16 lhs = int8(cpu->m_registers.AL);
    int16 rhs = int8(ReadByteOperand(cpu, instruction, &instruction->operands[0]));
    int16 result = lhs * rhs;
    uint8 truncated_result = uint8(uint16(result) & 0xFFFF);

    cpu->m_registers.AX = uint16(result);

    cpu->m_registers.EFLAGS.OF = (int16(int8(truncated_result)) != result);
    cpu->m_registers.EFLAGS.CF = (int16(int8(truncated_result)) != result);
    cpu->m_registers.EFLAGS.SF = IsSign(truncated_result);
    cpu->m_registers.EFLAGS.ZF = IsZero(truncated_result);
    cpu->m_registers.EFLAGS.PF = IsParity(truncated_result);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    int32 lhs, rhs;
    int32 result;
    uint16 truncated_result;

    if (instruction->operands[2].mode != AddressingMode_None)
    {
      // Three-operand form
      lhs = int16(ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[1]));
      rhs = int16(ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[2]));
      result = lhs * rhs;
      truncated_result = uint16(uint32(result) & 0xFFFF);

      WriteWordOperand(cpu, instruction, &instruction->operands[0], truncated_result);
    }
    else if (instruction->operands[1].mode != AddressingMode_None)
    {
      // Two-operand form
      lhs = int16(ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[0]));
      rhs = int16(ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[1]));
      result = lhs * rhs;
      truncated_result = uint16(uint32(result) & 0xFFFF);

      WriteWordOperand(cpu, instruction, &instruction->operands[0], truncated_result);
    }
    else
    {
      // One-operand form
      lhs = int16(cpu->m_registers.AX);
      rhs = int16(ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[0]));
      result = lhs * rhs;
      truncated_result = uint16(uint32(result) & 0xFFFF);

      cpu->m_registers.DX = uint16((uint32(result) >> 16) & 0xFFFF);
      cpu->m_registers.AX = truncated_result;
    }

    cpu->m_registers.EFLAGS.OF = (int32(int16(truncated_result)) != result);
    cpu->m_registers.EFLAGS.CF = (int32(int16(truncated_result)) != result);
    cpu->m_registers.EFLAGS.SF = IsSign(truncated_result);
    cpu->m_registers.EFLAGS.ZF = IsZero(truncated_result);
    cpu->m_registers.EFLAGS.PF = IsParity(truncated_result);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    int64 lhs, rhs;
    int64 result;
    uint32 truncated_result;

    if (instruction->operands[2].mode != AddressingMode_None)
    {
      // Three-operand form
      lhs = int32(ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[1]));
      rhs = int32(ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[2]));
      result = lhs * rhs;
      truncated_result = Truncate32(result);

      WriteDWordOperand(cpu, instruction, &instruction->operands[0], truncated_result);
    }
    else if (instruction->operands[1].mode != AddressingMode_None)
    {
      // Two-operand form
      lhs = int32(ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[0]));
      rhs = int32(ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[1]));
      result = lhs * rhs;
      truncated_result = Truncate32(result);

      WriteDWordOperand(cpu, instruction, &instruction->operands[0], truncated_result);
    }
    else
    {
      // One-operand form
      lhs = int32(cpu->m_registers.EAX);
      rhs = int32(ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[0]));
      result = lhs * rhs;
      truncated_result = Truncate32(result);

      cpu->m_registers.EDX = Truncate32(uint64(result) >> 32);
      cpu->m_registers.EAX = truncated_result;
    }

    cpu->m_registers.EFLAGS.OF = (int64(SignExtend64(truncated_result)) != result);
    cpu->m_registers.EFLAGS.CF = (int64(SignExtend64(truncated_result)) != result);
    cpu->m_registers.EFLAGS.SF = IsSign(truncated_result);
    cpu->m_registers.EFLAGS.ZF = IsZero(truncated_result);
    cpu->m_registers.EFLAGS.PF = IsParity(truncated_result);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_DIV(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    // Eight-bit divides use AX as a source
    uint8 divisor = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    uint16 source = cpu->m_registers.AX;
    uint16 quotient = source / divisor;
    uint16 remainder = source % divisor;
    if (quotient > 0xFF)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    cpu->m_registers.AL = uint8(quotient);
    cpu->m_registers.AH = uint8(remainder);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    // 16-bit divides use DX:AX as a source
    uint16 divisor = ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[0]);
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    uint32 source = (uint32(cpu->m_registers.DX) << 16) | uint32(cpu->m_registers.AX);
    uint32 quotient = source / divisor;
    uint32 remainder = source % divisor;
    if (quotient > 0xFFFF)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    cpu->m_registers.AX = uint16(quotient);
    cpu->m_registers.DX = uint16(remainder);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    // 32-bit divides use EDX:EAX as a source
    uint32 divisor = ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[0]);
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    uint64 source = (ZeroExtend64(cpu->m_registers.EDX) << 32) | ZeroExtend64(cpu->m_registers.EAX);
    uint64 quotient = source / divisor;
    uint64 remainder = source % divisor;
    if (quotient > UINT64_C(0xFFFFFFFF))
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    cpu->m_registers.EAX = Truncate32(quotient);
    cpu->m_registers.EDX = Truncate32(remainder);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_IDIV(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    // Eight-bit divides use AX as a source
    int8 divisor = int8(ReadByteOperand(cpu, instruction, &instruction->operands[0]));
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    int16 source = int16(cpu->m_registers.AX);
    int16 quotient = source / divisor;
    int16 remainder = source % divisor;
    uint8 truncated_quotient = uint8(uint16(quotient) & 0xFFFF);
    uint8 truncated_remainder = uint8(uint16(remainder) & 0xFFFF);
    if (int8(truncated_quotient) != quotient)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    cpu->m_registers.AL = truncated_quotient;
    cpu->m_registers.AH = truncated_remainder;
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    // 16-bit divides use DX:AX as a source
    int16 divisor = int16(ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[0]));
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    int32 source = int32((uint32(cpu->m_registers.DX) << 16) | uint32(cpu->m_registers.AX));
    int32 quotient = source / divisor;
    int32 remainder = source % divisor;
    uint16 truncated_quotient = uint16(uint32(quotient) & 0xFFFF);
    uint16 truncated_remainder = uint16(uint32(remainder) & 0xFFFF);
    if (int16(truncated_quotient) != quotient)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    cpu->m_registers.AX = truncated_quotient;
    cpu->m_registers.DX = truncated_remainder;
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    // 16-bit divides use DX:AX as a source
    int32 divisor = int32(ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[0]));
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    int64 source = int64((ZeroExtend64(cpu->m_registers.EDX) << 32) | ZeroExtend64(cpu->m_registers.EAX));
    int64 quotient = source / divisor;
    int64 remainder = source % divisor;
    uint32 truncated_quotient = Truncate32(uint64(quotient));
    uint32 truncated_remainder = Truncate32(uint64(remainder));
    if (int32(truncated_quotient) != quotient)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    cpu->m_registers.EAX = truncated_quotient;
    cpu->m_registers.EDX = truncated_remainder;
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_SHL(CPU* cpu, const OldInstruction* instruction)
{
  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 shift_amount = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (shift_amount == 0)
      return;

    uint16 shifted_value = ZeroExtend16(value) << shift_amount;
    uint8 new_value = Truncate8(shifted_value);
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = ((shifted_value & 0x100) != 0);
    cpu->m_registers.EFLAGS.OF = (shift_amount == 1 && (((shifted_value >> 7) & 1) ^ ((shifted_value >> 8) & 1)) != 0);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.AF = false;
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 shift_amount = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (shift_amount == 0)
      return;

    uint32 shifted_value = ZeroExtend32(value) << shift_amount;
    uint16 new_value = Truncate16(shifted_value);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = ((shifted_value & 0x10000) != 0);
    cpu->m_registers.EFLAGS.OF =
      (shift_amount == 1 && (((shifted_value >> 15) & 1) ^ ((shifted_value >> 16) & 1)) != 0);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.AF = false;
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 shift_amount = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (shift_amount == 0)
      return;

    uint64 shifted_value = ZeroExtend64(value) << shift_amount;
    uint32 new_value = Truncate32(shifted_value);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = ((shifted_value & UINT64_C(0x100000000)) != 0);
    cpu->m_registers.EFLAGS.OF =
      (shift_amount == 1 && (((shifted_value >> 31) & 1) ^ ((shifted_value >> 32) & 1)) != 0);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.AF = false;
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_SHR(CPU* cpu, const OldInstruction* instruction)
{
  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 shift_amount = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (shift_amount == 0)
      return;

    uint8 new_value = value >> shift_amount;
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = ((shift_amount ? (value >> (shift_amount - 1) & 1) : (value & 1)) != 0);
    cpu->m_registers.EFLAGS.OF = (shift_amount == 1 && (value & 0x80) != 0);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 shift_amount = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (shift_amount == 0)
      return;

    uint16 new_value = value >> shift_amount;
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = ((shift_amount ? (value >> (shift_amount - 1) & 1) : (value & 1)) != 0);
    cpu->m_registers.EFLAGS.OF = (shift_amount == 1 && (value & 0x8000) != 0);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 shift_amount = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (shift_amount == 0)
      return;

    uint32 new_value = value >> shift_amount;
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = ((shift_amount ? (value >> (shift_amount - 1) & 1) : (value & 1)) != 0);
    cpu->m_registers.EFLAGS.OF = (shift_amount == 1 && (value & 0x80000000) != 0);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_SAR(CPU* cpu, const OldInstruction* instruction)
{
  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 shift_amount = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (shift_amount == 0)
      return;

    uint8 new_value = uint8(int8(value) >> shift_amount);
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = ((int8(value) >> (shift_amount - 1) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = false;
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 shift_amount = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (shift_amount == 0)
      return;

    uint16 new_value = uint16(int16(value) >> shift_amount);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = ((int16(value) >> (shift_amount - 1) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = false;
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 shift_amount = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (shift_amount == 0)
      return;

    uint32 new_value = uint32(int32(value) >> shift_amount);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = ((int32(value) >> (shift_amount - 1) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = false;
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_RCL(CPU* cpu, const OldInstruction* instruction)
{
  // The processor restricts the count to a number between 0 and 31 by masking all the bits in the count operand except
  // the 5 least-significant bits.
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rotate_count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (rotate_count == 0)
      return;

    uint8 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint8 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 7);
    }
    WriteByteOperand(cpu, instruction, &instruction->operands[0], value);

    cpu->m_registers.EFLAGS.CF = (carry != 0);
    cpu->m_registers.EFLAGS.OF = (((value >> 7) ^ carry) != 0);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rotate_count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (rotate_count == 0)
      return;

    uint16 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint16 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 15);
    }
    WriteWordOperand(cpu, instruction, &instruction->operands[0], value);

    cpu->m_registers.EFLAGS.CF = (carry != 0);
    cpu->m_registers.EFLAGS.OF = (((value >> 15) ^ carry) != 0);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rotate_count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (rotate_count == 0)
      return;

    uint32 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint32 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 31);
    }
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], value);

    cpu->m_registers.EFLAGS.CF = (carry != 0);
    cpu->m_registers.EFLAGS.OF = (((value >> 31) ^ carry) != 0);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_RCR(CPU* cpu, const OldInstruction* instruction)
{
  // The processor restricts the count to a number between 0 and 31 by masking all the bits in the count operand except
  // the 5 least-significant bits.
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rotate_count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (rotate_count == 0)
      return;

    uint8 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint8 save_value = value;
      value = (save_value >> 1) | (carry << 7);
      carry = (save_value & 1);
    }
    WriteByteOperand(cpu, instruction, &instruction->operands[0], value);

    cpu->m_registers.EFLAGS.CF = (carry != 0);
    cpu->m_registers.EFLAGS.OF = (((value >> 7) ^ ((value >> 6) & 1)) != 0);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rotate_count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (rotate_count == 0)
      return;

    uint16 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint16 save_value = value;
      value = (save_value >> 1) | (carry << 15);
      carry = (save_value & 1);
    }
    WriteWordOperand(cpu, instruction, &instruction->operands[0], value);

    cpu->m_registers.EFLAGS.CF = (carry != 0);
    cpu->m_registers.EFLAGS.OF = (((value >> 15) ^ ((value >> 14) & 1)) != 0);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rotate_count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (rotate_count == 0)
      return;

    uint32 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint32 save_value = value;
      value = (save_value >> 1) | (carry << 31);
      carry = (save_value & 1);
    }
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], value);

    cpu->m_registers.EFLAGS.CF = (carry != 0);
    cpu->m_registers.EFLAGS.OF = (((value >> 31) ^ ((value >> 30) & 1)) != 0);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_ROL(CPU* cpu, const OldInstruction* instruction)
{
  // Hopefully this will compile down to a native ROL instruction
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (count == 0)
      return;

    uint8 new_value = value;
    if ((count & 0x7) != 0)
    {
      uint8 masked_count = count & 0x7;
      new_value = (value << masked_count) | (value >> (8 - masked_count));
      WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);
    }

    uint8 b0 = (new_value & 1);
    uint8 b7 = (new_value >> 7);
    cpu->m_registers.EFLAGS.CF = (b0 != 0);
    cpu->m_registers.EFLAGS.OF = ((b0 ^ b7) != 0);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (count == 0)
      return;

    uint16 new_value = value;
    if ((count & 0xf) != 0)
    {
      uint8 masked_count = count & 0xf;
      new_value = (value << masked_count) | (value >> (16 - masked_count));
      WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);
    }

    uint16 b0 = (new_value & 1);
    uint16 b15 = (new_value >> 15);
    cpu->m_registers.EFLAGS.CF = (b0 != 0);
    cpu->m_registers.EFLAGS.OF = ((b0 ^ b15) != 0);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (count == 0)
      return;

    uint32 new_value = value;
    uint8 masked_count = count & 0x1f;
    if (masked_count != 0)
    {
      new_value = (value << masked_count) | (value >> (32 - masked_count));
      WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);
    }

    uint32 b0 = (new_value & 1);
    uint32 b31 = ((new_value >> 31) & 1);
    cpu->m_registers.EFLAGS.CF = (b0 != 0);
    cpu->m_registers.EFLAGS.OF = ((b0 ^ b31) != 0);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_ROR(CPU* cpu, const OldInstruction* instruction)
{
  // Hopefully this will compile down to a native ROR instruction
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (count == 0)
      return;

    uint8 new_value = value;
    uint8 masked_count = count & 0x7;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (8 - masked_count));
      WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);
    }

    uint16 b6 = ((new_value >> 6) & 1);
    uint16 b7 = ((new_value >> 7) & 1);
    cpu->m_registers.EFLAGS.CF = (b7 != 0);
    cpu->m_registers.EFLAGS.OF = ((b6 ^ b7) != 0);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (count == 0)
      return;

    uint16 new_value = value;
    uint8 masked_count = count & 0xf;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (16 - masked_count));
      WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);
    }

    uint16 b14 = ((new_value >> 14) & 1);
    uint16 b15 = ((new_value >> 15) & 1);
    cpu->m_registers.EFLAGS.CF = (b15 != 0);
    cpu->m_registers.EFLAGS.OF = ((b14 ^ b15) != 0);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint8 count = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;
    if (count == 0)
      return;

    uint32 new_value = value;
    uint8 masked_count = count & 0x1f;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (32 - masked_count));
      WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);
    }

    uint32 b30 = ((new_value >> 30) & 1);
    uint32 b31 = ((new_value >> 31) & 1);
    cpu->m_registers.EFLAGS.CF = (b31 != 0);
    cpu->m_registers.EFLAGS.OF = ((b30 ^ b31) != 0);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_AND(CPU* cpu, const OldInstruction* instruction)
{
  bool sf, zf, pf;
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rhs = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    uint8 new_value = lhs & rhs;
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 rhs = ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[1]);
    uint16 new_value = lhs & rhs;
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 rhs = ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint32 new_value = lhs & rhs;
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF flag
  // is undefined.
  cpu->m_registers.EFLAGS.OF = false;
  cpu->m_registers.EFLAGS.CF = false;
  cpu->m_registers.EFLAGS.SF = sf;
  cpu->m_registers.EFLAGS.ZF = zf;
  cpu->m_registers.EFLAGS.PF = pf;
  cpu->m_registers.EFLAGS.AF = false;
}

void Interpreter::Execute_OR(CPU* cpu, const OldInstruction* instruction)
{
  bool sf, zf, pf;
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rhs = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    uint8 new_value = lhs | rhs;
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 rhs = ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[1]);
    uint16 new_value = lhs | rhs;
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 rhs = ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint32 new_value = lhs | rhs;
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF flag
  // is undefined.
  cpu->m_registers.EFLAGS.OF = false;
  cpu->m_registers.EFLAGS.CF = false;
  cpu->m_registers.EFLAGS.SF = sf;
  cpu->m_registers.EFLAGS.ZF = zf;
  cpu->m_registers.EFLAGS.PF = pf;
  cpu->m_registers.EFLAGS.AF = false;
}

void Interpreter::Execute_XOR(CPU* cpu, const OldInstruction* instruction)
{
  bool sf, zf, pf;
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rhs = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    uint8 new_value = lhs ^ rhs;
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 rhs = ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[1]);
    uint16 new_value = lhs ^ rhs;
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 rhs = ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint32 new_value = lhs ^ rhs;
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF flag
  // is undefined.
  cpu->m_registers.EFLAGS.OF = false;
  cpu->m_registers.EFLAGS.CF = false;
  cpu->m_registers.EFLAGS.SF = sf;
  cpu->m_registers.EFLAGS.ZF = zf;
  cpu->m_registers.EFLAGS.PF = pf;
  cpu->m_registers.EFLAGS.AF = false;
}

void Interpreter::Execute_TEST(CPU* cpu, const OldInstruction* instruction)
{
  bool sf, zf, pf;
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 rhs = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    uint8 new_value = lhs & rhs;

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 rhs = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    uint16 new_value = lhs & rhs;

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 rhs = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint32 new_value = lhs & rhs;

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF flag
  // is undefined.
  cpu->m_registers.EFLAGS.OF = false;
  cpu->m_registers.EFLAGS.CF = false;
  cpu->m_registers.EFLAGS.SF = sf;
  cpu->m_registers.EFLAGS.ZF = zf;
  cpu->m_registers.EFLAGS.PF = pf;
  cpu->m_registers.EFLAGS.AF = false;
}

void Interpreter::Execute_NEG(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 new_value = uint8(-int8(value));
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);

    ALUOp_Sub8(&cpu->m_registers, 0, value);
    cpu->m_registers.EFLAGS.CF = (new_value != 0);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 new_value = uint16(-int16(value));
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    ALUOp_Sub16(&cpu->m_registers, 0, value);
    cpu->m_registers.EFLAGS.CF = (new_value != 0);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 new_value = uint32(-int32(value));
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    ALUOp_Sub32(&cpu->m_registers, 0, value);
    cpu->m_registers.EFLAGS.CF = (new_value != 0);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_NOT(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 value = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 new_value = ~value;
    WriteByteOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 new_value = ~value;
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 new_value = ~value;
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_AAA(CPU* cpu, const OldInstruction* instruction)
{
  if ((cpu->m_registers.AL & 0xF) > 0x09 || cpu->m_registers.EFLAGS.AF)
  {
    cpu->m_registers.AX += 0x0106;
    cpu->m_registers.EFLAGS.AF = true;
    cpu->m_registers.EFLAGS.CF = true;
  }
  else
  {
    cpu->m_registers.EFLAGS.AF = false;
    cpu->m_registers.EFLAGS.CF = false;
  }

  cpu->m_registers.AL &= 0x0F;

  cpu->m_registers.EFLAGS.SF = IsSign(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.ZF = IsZero(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.PF = IsParity(cpu->m_registers.AL);
}

void Interpreter::Execute_AAS(CPU* cpu, const OldInstruction* instruction)
{
  if ((cpu->m_registers.AL & 0xF) > 0x09 || cpu->m_registers.EFLAGS.AF)
  {
    cpu->m_registers.AX -= 0x0106;
    cpu->m_registers.EFLAGS.AF = true;
    cpu->m_registers.EFLAGS.CF = true;
  }
  else
  {
    cpu->m_registers.EFLAGS.AF = false;
    cpu->m_registers.EFLAGS.CF = false;
  }

  cpu->m_registers.AL &= 0x0F;

  cpu->m_registers.EFLAGS.SF = IsSign(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.ZF = IsZero(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.PF = IsParity(cpu->m_registers.AL);
}

void Interpreter::Execute_AAM(CPU* cpu, const OldInstruction* instruction)
{
  uint8 operand = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
  if (operand == 0)
  {
    cpu->RaiseException(Interrupt_DivideError);
    return;
  }

  cpu->m_registers.AH = cpu->m_registers.AL / operand;
  cpu->m_registers.AL = cpu->m_registers.AL % operand;

  cpu->m_registers.EFLAGS.AF = false;
  cpu->m_registers.EFLAGS.CF = false;
  cpu->m_registers.EFLAGS.OF = false;

  cpu->m_registers.EFLAGS.SF = IsSign(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.ZF = IsZero(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.PF = IsParity(cpu->m_registers.AL);
}

void Interpreter::Execute_AAD(CPU* cpu, const OldInstruction* instruction)
{
  uint8 operand = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
  uint16 result = uint16(cpu->m_registers.AH) * uint16(operand) + uint16(cpu->m_registers.AL);

  cpu->m_registers.AL = uint8(result & 0xFF);
  cpu->m_registers.AH = 0;

  cpu->m_registers.EFLAGS.AF = false;
  cpu->m_registers.EFLAGS.CF = false;
  cpu->m_registers.EFLAGS.OF = false;

  cpu->m_registers.EFLAGS.SF = IsSign(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.ZF = IsZero(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.PF = IsParity(cpu->m_registers.AL);
}

void Interpreter::Execute_DAA(CPU* cpu, const OldInstruction* instruction)
{
  uint8 old_AL = cpu->m_registers.AL;
  bool old_CF = cpu->m_registers.EFLAGS.CF;

  if ((old_AL & 0xF) > 0x9 || cpu->m_registers.EFLAGS.AF)
  {
    cpu->m_registers.EFLAGS.CF = ((old_AL > 0xF9) || old_CF);
    cpu->m_registers.AL += 0x6;
    cpu->m_registers.EFLAGS.AF = true;
  }
  else
  {
    cpu->m_registers.EFLAGS.AF = false;
  }

  if (old_AL > 0x99 || old_CF)
  {
    cpu->m_registers.AL += 0x60;
    cpu->m_registers.EFLAGS.CF = true;
  }
  else
  {
    cpu->m_registers.EFLAGS.CF = false;
  }

  cpu->m_registers.EFLAGS.OF = false;
  cpu->m_registers.EFLAGS.SF = IsSign(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.ZF = IsZero(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.PF = IsParity(cpu->m_registers.AL);
}

void Interpreter::Execute_DAS(CPU* cpu, const OldInstruction* instruction)
{
  uint8 old_AL = cpu->m_registers.AL;
  bool old_CF = cpu->m_registers.EFLAGS.CF;

  if ((old_AL & 0xF) > 0x9 || cpu->m_registers.EFLAGS.AF)
  {
    cpu->m_registers.EFLAGS.CF = ((old_AL < 0x06) || old_CF);
    cpu->m_registers.AL -= 0x6;
    cpu->m_registers.EFLAGS.AF = true;
  }
  else
  {
    cpu->m_registers.EFLAGS.AF = false;
  }

  if (old_AL > 0x99 || old_CF)
  {
    cpu->m_registers.AL -= 0x60;
    cpu->m_registers.EFLAGS.CF = true;
  }

  cpu->m_registers.EFLAGS.OF = false;
  cpu->m_registers.EFLAGS.SF = IsSign(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.ZF = IsZero(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.PF = IsParity(cpu->m_registers.AL);
}

void Interpreter::Execute_BTx(CPU* cpu, const OldInstruction* instruction)
{
  // When combined with a memory operand, these instructions can access more than 16/32 bits.
  bool is_register_operand = (instruction->operands[0].mode == AddressingMode_Register);
  if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 source = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    uint16 bit = source & 0xF;
    uint16 mask = (UINT16_C(1) << bit);

    uint16 in_value, out_value;
    VirtualMemoryAddress effective_address = 0;
    if (is_register_operand)
    {
      // Ignore displacement.
      DebugAssert(instruction->operands[0].reg.reg16 < Reg16_Count);
      in_value = cpu->m_registers.reg16[instruction->operands[0].reg.reg16];
    }
    else
    {
      // Displacement can be signed, annoyingly, so we need to divide rather than shift.
      int16 displacement = int16(source & 0xFFF0) / 16;
      effective_address = CalculateEffectiveAddress(cpu, instruction, &instruction->operands[0]);
      effective_address += SignExtend<VirtualMemoryAddress>(displacement) * 2;
      if (instruction->address_size == AddressSize_16)
        effective_address &= 0xFFFF;

      in_value = cpu->ReadMemoryWord(instruction->segment, effective_address);
    }

    // Output value depends on operation, since we share this handler for multiple operations
    switch (instruction->operation)
    {
      case Operation_BTC:
        out_value = in_value ^ mask;
        break;
      case Operation_BTR:
        out_value = in_value & ~mask;
        break;
      case Operation_BTS:
        out_value = in_value | mask;
        break;
      case Operation_BT:
      default:
        out_value = in_value;
        break;
    }

    // Write back to register/memory
    if (out_value != in_value)
    {
      if (is_register_operand)
        cpu->m_registers.reg16[instruction->operands[0].reg.reg16] = out_value;
      else
        cpu->WriteMemoryWord(instruction->segment, effective_address, out_value);
    }

    // CF flag depends on whether the bit was set in the input value
    cpu->m_registers.EFLAGS.CF = ((in_value & mask) != 0);
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 source = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint32 bit = source & 0x1F;
    uint32 mask = (UINT32_C(1) << bit);

    uint32 in_value, out_value;
    VirtualMemoryAddress effective_address = 0;
    if (is_register_operand)
    {
      // Ignore displacement.
      DebugAssert(instruction->operands[0].reg.reg32 < Reg32_Count);
      in_value = cpu->m_registers.reg32[instruction->operands[0].reg.reg32];
    }
    else
    {
      // Displacement can be signed, annoyingly, so we need to divide rather than shift.
      int32 displacement = int32(source & 0xFFFFFFE0) / 32;
      effective_address = CalculateEffectiveAddress(cpu, instruction, &instruction->operands[0]);
      effective_address += SignExtend<VirtualMemoryAddress>(displacement) * 4;
      if (instruction->address_size == AddressSize_16)
        effective_address &= 0xFFFF;

      in_value = cpu->ReadMemoryDWord(instruction->segment, effective_address);
    }

    // Output value depends on operation, since we share this handler for multiple operations
    switch (instruction->operation)
    {
      case Operation_BTC:
        out_value = in_value ^ mask;
        break;
      case Operation_BTR:
        out_value = in_value & ~mask;
        break;
      case Operation_BTS:
        out_value = in_value | mask;
        break;
      case Operation_BT:
      default:
        out_value = in_value;
        break;
    }

    // Write back to register/memory
    if (out_value != in_value)
    {
      if (is_register_operand)
        cpu->m_registers.reg32[instruction->operands[0].reg.reg32] = out_value;
      else
        cpu->WriteMemoryDWord(instruction->segment, effective_address, out_value);
    }

    // CF flag depends on whether the bit was set in the input value
    cpu->m_registers.EFLAGS.CF = ((in_value & mask) != 0);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_BSF(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 mask = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    if (mask != 0)
    {
      uint16 index = 0;
      Y_bitscanforward(mask, &index);
      WriteWordOperand(cpu, instruction, &instruction->operands[0], Truncate16(index));
      cpu->m_registers.EFLAGS.ZF = false;
    }
    else
    {
      cpu->m_registers.EFLAGS.ZF = true;
    }
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 mask = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    if (mask != 0)
    {
      uint32 index = 0;
      Y_bitscanforward(mask, &index);
      WriteDWordOperand(cpu, instruction, &instruction->operands[0], index);
      cpu->m_registers.EFLAGS.ZF = false;
    }
    else
    {
      cpu->m_registers.EFLAGS.ZF = true;
    }
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_BSR(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 mask = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    if (mask != 0)
    {
      uint16 index = 0;
      Y_bitscanreverse(mask, &index);
      WriteWordOperand(cpu, instruction, &instruction->operands[0], Truncate16(index));
      cpu->m_registers.EFLAGS.ZF = false;
    }
    else
    {
      cpu->m_registers.EFLAGS.ZF = true;
    }
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 mask = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    if (mask != 0)
    {
      uint32 index = 0;
      Y_bitscanreverse(mask, &index);
      WriteDWordOperand(cpu, instruction, &instruction->operands[0], index);
      cpu->m_registers.EFLAGS.ZF = false;
    }
    else
    {
      cpu->m_registers.EFLAGS.ZF = true;
    }
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_SHLD(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operand_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 shift_in = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    uint8 shift_count = ReadByteOperand(cpu, instruction, &instruction->operands[2]) & 0x1F;
    if (shift_count == 0)
      return;

    uint32 temp_value1 = ((ZeroExtend32(value) << 16) | ZeroExtend32(shift_in));
    uint32 temp_value2 = temp_value1 << shift_count;
    if (shift_count > 16)
      temp_value2 |= (value << (shift_count - 16));

    // temp_value >>= 16;
    uint16 new_value = Truncate16(temp_value2 >> 16);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = (((temp_value1 >> (32 - shift_count)) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = (((value ^ new_value) & 0x8000) != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 shift_in = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint8 shift_count = ReadByteOperand(cpu, instruction, &instruction->operands[2]) & 0x1F;
    if (shift_count == 0)
      return;

    uint32 new_value = (value << shift_count) | (shift_in >> (32 - shift_count));
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = (((value >> (32 - shift_count)) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = ((BoolToUInt32(cpu->m_registers.EFLAGS.CF) ^ (new_value >> 31)) != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_SHRD(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operand_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 shift_in = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    uint8 shift_count = ReadByteOperand(cpu, instruction, &instruction->operands[2]) & 0x1F;
    if (shift_count == 0)
      return;

    uint32 temp_value = ((ZeroExtend32(shift_in) << 16) | ZeroExtend32(value));
    temp_value >>= shift_count;
    if (shift_count > 16)
      temp_value |= (value << (32 - shift_count));

    uint16 new_value = Truncate16(temp_value);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = (((value >> (shift_count - 1)) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = (((value ^ new_value) & 0x8000) != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
  }
  else if (instruction->operand_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 shift_in = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint8 shift_count = ReadByteOperand(cpu, instruction, &instruction->operands[2]) & 0x1F;
    if (shift_count == 0)
      return;

    uint32 new_value = (shift_in << (32 - shift_count)) | (value >> shift_count);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], new_value);

    cpu->m_registers.EFLAGS.CF = (((value >> (shift_count - 1)) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = (((value ^ new_value) & UINT32_C(0x80000000)) != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_LGDT(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  VirtualMemoryAddress base_address = CalculateEffectiveAddress(cpu, instruction, &instruction->operands[0]);
  uint16 table_limit = cpu->ReadMemoryWord(instruction->segment, base_address + 0);
  uint32 table_base_address = cpu->ReadMemoryDWord(instruction->segment, base_address + 2);
  cpu->m_gdt_location.limit = ZeroExtend32(table_limit);
  cpu->m_gdt_location.base_address = table_base_address;

  // 16-bit operand drops higher order bits
  if (instruction->operand_size == OperandSize_16)
    cpu->m_gdt_location.base_address &= 0xFFFFFF;

  Log_DevPrintf("Load GDT from %04X:%04X: Base 0x%08X limit 0x%04X",
                cpu->m_registers.segment_selectors[instruction->segment], base_address,
                cpu->m_gdt_location.base_address, cpu->m_gdt_location.limit);
}

void Interpreter::Execute_LIDT(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  VirtualMemoryAddress base_address = CalculateEffectiveAddress(cpu, instruction, &instruction->operands[0]);
  uint16 table_limit = cpu->ReadMemoryWord(instruction->segment, base_address + 0);
  uint32 table_base_address = cpu->ReadMemoryDWord(instruction->segment, base_address + 2);

  cpu->m_idt_location.limit = ZeroExtend32(table_limit);
  cpu->m_idt_location.base_address = table_base_address;

  // 16-bit operand drops higher order bits
  if (instruction->operand_size == OperandSize_16)
    cpu->m_idt_location.base_address &= 0xFFFFFF;

  Log_DevPrintf("Load IDT from %04X:%04X: Base 0x%08X limit 0x%04X",
                cpu->m_registers.segment_selectors[instruction->segment], base_address,
                cpu->m_idt_location.base_address, cpu->m_idt_location.limit);
}

void Interpreter::Execute_SGDT(CPU* cpu, const OldInstruction* instruction)
{
  uint32 gdt_address = Truncate32(cpu->m_gdt_location.base_address);
  uint16 gdt_limit = Truncate16(cpu->m_gdt_location.limit);

  // 80286+ sets higher-order bits to 0xFF
  if (cpu->m_model == MODEL_286)
    gdt_address = (gdt_address & 0xFFFFFF) | 0xFF000000;
  // 16-bit operand sets higher-order bits to zero
  else if (instruction->operand_size == OperandSize_16)
    gdt_address = (gdt_address & 0xFFFFFF);

  // Write back to memory
  VirtualMemoryAddress base_address = CalculateEffectiveAddress(cpu, instruction, &instruction->operands[0]);
  cpu->WriteMemoryWord(instruction->segment, base_address + 0, gdt_limit);
  cpu->WriteMemoryDWord(instruction->segment, base_address + 2, gdt_address);
}

void Interpreter::Execute_SIDT(CPU* cpu, const OldInstruction* instruction)
{
  uint32 idt_address = Truncate32(cpu->m_idt_location.base_address);
  uint16 idt_limit = Truncate16(cpu->m_idt_location.limit);

  // 80286+ sets higher-order bits to 0xFF
  if (cpu->m_model == MODEL_286)
    idt_address = (idt_address & 0xFFFFFF) | 0xFF000000;
  // 16-bit operand sets higher-order bits to zero
  else if (instruction->operand_size == OperandSize_16)
    idt_address = (idt_address & 0xFFFFFF);

  // Write back to memory
  VirtualMemoryAddress base_address = CalculateEffectiveAddress(cpu, instruction, &instruction->operands[0]);
  cpu->WriteMemoryWord(instruction->segment, base_address + 0, idt_limit);
  cpu->WriteMemoryDWord(instruction->segment, base_address + 2, idt_address);
}

void Interpreter::Execute_LMSW(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
  Log_DevPrintf("Load MSW: 0x%04X", value);

  cpu->LoadSpecialRegister(Reg32_CR0, (cpu->m_registers.CR0 & 0xFFFFFFF0) | ZeroExtend32(value & 0xF));
}

void Interpreter::Execute_SMSW(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = Truncate16(cpu->m_registers.CR0);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = cpu->m_registers.CR0;
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_LLDT(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  uint16 selector = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
  cpu->LoadLocalDescriptorTable(selector);
}

void Interpreter::Execute_SLDT(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  WriteWordOperand(cpu, instruction, &instruction->operands[0], cpu->m_registers.LDTR);
}

void Interpreter::Execute_LTR(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  uint16 selector = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
  cpu->LoadTaskSegment(selector);
}

void Interpreter::Execute_STR(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  WriteWordOperand(cpu, instruction, &instruction->operands[0], cpu->m_registers.TR);
}

void Interpreter::Execute_CLTS(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  cpu->m_registers.CR0 &= ~CR0Bit_TS;
}

void Interpreter::Execute_LAR(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  SEGMENT_SELECTOR_VALUE selector;
  selector.bits = ReadWordOperand(cpu, instruction, &instruction->operands[1]);

  // Validity checks
  DESCRIPTOR_ENTRY descriptor;
  if (selector.index == 0 ||
      !cpu->ReadDescriptorEntry(&descriptor, selector.ti ? cpu->m_ldt_location : cpu->m_gdt_location, selector.index))
  {
    cpu->m_registers.EFLAGS.ZF = false;
    return;
  }

  // Can only read certain types of descriptors
  if (!descriptor.IsDataSegment() && !descriptor.IsCodeSegment() &&
      descriptor.type != DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16 && descriptor.type != DESCRIPTOR_TYPE_LDT &&
      descriptor.type != DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16 && descriptor.type != DESCRIPTOR_TYPE_CALL_GATE_16 &&
      descriptor.type != DESCRIPTOR_TYPE_TASK_GATE && descriptor.type != DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32 &&
      descriptor.type != DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32 && descriptor.type != DESCRIPTOR_TYPE_CALL_GATE_32)
  {
    cpu->m_registers.EFLAGS.ZF = false;
    return;
  }

  // Privilege level check for non-conforming code segments
  if (!descriptor.IsConformingCodeSegment() && (cpu->GetCPL() > descriptor.dpl || selector.rpl > descriptor.dpl))
  {
    cpu->m_registers.EFLAGS.ZF = false;
    return;
  }

  // All good
  uint32 result = descriptor.bits1 & 0x00FFFF00;
  if (instruction->operands[0].size == OperandSize_16)
    WriteWordOperand(cpu, instruction, &instruction->operands[0], Truncate16(result));
  else if (instruction->operands[0].size == OperandSize_32)
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], result);
  else
    DebugUnreachableCode();

  cpu->m_registers.EFLAGS.ZF = true;
}

void Interpreter::Execute_LSL(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  SEGMENT_SELECTOR_VALUE selector;
  selector.bits = ReadWordOperand(cpu, instruction, &instruction->operands[1]);

  // Validity checks
  DESCRIPTOR_ENTRY descriptor;
  if (selector.index == 0 ||
      !cpu->ReadDescriptorEntry(&descriptor, selector.ti ? cpu->m_ldt_location : cpu->m_gdt_location, selector.index))
  {
    cpu->m_registers.EFLAGS.ZF = false;
    return;
  }

  // Can only read certain types of descriptors
  if (!descriptor.IsDataSegment() && !descriptor.IsCodeSegment() &&
      descriptor.type != DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_16 && descriptor.type != DESCRIPTOR_TYPE_LDT &&
      descriptor.type != DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_16 &&
      descriptor.type != DESCRIPTOR_TYPE_AVAILABLE_TASK_SEGMENT_32 &&
      descriptor.type != DESCRIPTOR_TYPE_BUSY_TASK_SEGMENT_32)
  {
    cpu->m_registers.EFLAGS.ZF = false;
    return;
  }

  // Privilege level check for non-conforming code segments
  if (!descriptor.IsConformingCodeSegment() && (cpu->GetCPL() > descriptor.dpl || selector.rpl > descriptor.dpl))
  {
    cpu->m_registers.EFLAGS.ZF = false;
    return;
  }

  // TODO: This is the same
  uint32 limit;
  if (descriptor.is_memory_descriptor)
  {
    // TODO: Update this in load too
    limit = descriptor.memory.GetLimit();
    if (descriptor.memory.flags.granularity)
      limit = (limit << 12) | 0xFFF;
  }
  else
  {
    limit = descriptor.tss.GetLimit();
    if (descriptor.tss.granularity)
      limit = (limit << 12) | 0xFFF;
  }

  cpu->m_registers.EFLAGS.ZF = true;
  if (instruction->operands[0].size == OperandSize_16)
    WriteWordOperand(cpu, instruction, &instruction->operands[0], Truncate16(limit));
  else if (instruction->operands[0].size == OperandSize_32)
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], limit);
  else
    DebugUnreachableCode();

  cpu->m_registers.EFLAGS.ZF = true;
}

void Interpreter::Execute_ARPL(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  SEGMENT_SELECTOR_VALUE dst = {ReadWordOperand(cpu, instruction, &instruction->operands[0])};
  SEGMENT_SELECTOR_VALUE src = {ReadWordOperand(cpu, instruction, &instruction->operands[1])};

  if (dst.rpl < src.rpl)
  {
    dst.rpl = src.rpl;
    WriteWordOperand(cpu, instruction, &instruction->operands[0], dst.bits);
    cpu->m_registers.EFLAGS.ZF = true;
  }
  else
  {
    cpu->m_registers.EFLAGS.ZF = false;
  }
}

void Interpreter::Execute_VERx(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  uint16 selector_bits = ReadWordOperand(cpu, instruction, &instruction->operands[0]);

  // Check the selector is valid, and read the selector
  SEGMENT_SELECTOR_VALUE selector = {selector_bits};
  DESCRIPTOR_ENTRY descriptor;
  if (selector.index == 0 ||
      !cpu->ReadDescriptorEntry(&descriptor, selector.ti ? cpu->m_ldt_location : cpu->m_gdt_location, selector.index))
  {
    cpu->m_registers.EFLAGS.ZF = false;
    return;
  }

  // Check descriptor type
  if (!descriptor.IsCodeSegment() && !descriptor.IsDataSegment())
  {
    cpu->m_registers.EFLAGS.ZF = false;
    return;
  }

  // Check for non-conforming code segments
  if (!descriptor.memory.IsConformingCodeSegment() && (cpu->GetCPL() > descriptor.dpl || selector.rpl > descriptor.dpl))
  {
    cpu->m_registers.EFLAGS.ZF = false;
    return;
  }

  // Check readable/writable flags. Code segments are never writable
  bool is_readable = (descriptor.IsCodeSegment() || descriptor.memory.access.code_readable);
  bool is_writable = (!descriptor.IsCodeSegment() && descriptor.memory.access.data_writable);

  // Select result based on instruction
  switch (instruction->operation)
  {
    case Operation_VERR:
      cpu->m_registers.EFLAGS.ZF = is_readable;
      break;
    case Operation_VERW:
      cpu->m_registers.EFLAGS.ZF = is_writable;
      break;
    default:
      break;
  }
}

void Interpreter::Execute_BOUND(CPU* cpu, const OldInstruction* instruction)
{
  VirtualMemoryAddress table_address = CalculateEffectiveAddress(cpu, instruction, &instruction->operands[1]);

  uint32 address;
  uint32 lower_bound;
  uint32 upper_bound;
  if (instruction->operands[0].size == OperandSize_16)
  {
    address = ZeroExtend32(ReadWordOperand(cpu, instruction, &instruction->operands[0]));
    lower_bound = ZeroExtend32(cpu->ReadMemoryWord(instruction->segment, table_address + 0));
    upper_bound = ZeroExtend32(cpu->ReadMemoryWord(instruction->segment, table_address + 2));
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    address = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    lower_bound = cpu->ReadMemoryDWord(instruction->segment, table_address + 0);
    upper_bound = cpu->ReadMemoryDWord(instruction->segment, table_address + 4);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (address < lower_bound || address > upper_bound)
    cpu->RaiseException(Interrupt_Bounds);
}

void Interpreter::Execute_WAIT(CPU* cpu, const OldInstruction* instruction)
{
  // Check and handle any pending floating point exceptions
  cpu->CheckFloatingPointException();
}

void Interpreter::Execute_ENTER(CPU* cpu, const OldInstruction* instruction)
{
  uint16 stack_frame_size = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
  uint8 level = ReadByteOperand(cpu, instruction, &instruction->operands[1]) & 0x1F;

  // Push current frame pointer.
  if (instruction->operand_size == OperandSize_16)
    cpu->PushWord(cpu->m_registers.BP);
  else
    cpu->PushDWord(cpu->m_registers.EBP);

  uint32 frame_pointer = cpu->m_registers.ESP;
  if (level > 0)
  {
    // Use our own local copy of EBP in case any of these fail.
    if (instruction->operand_size == OperandSize_16)
    {
      uint16 BP = cpu->m_registers.BP;
      for (uint8 i = 1; i < level; i++)
      {
        BP -= sizeof(uint16);

        uint16 prev_ptr = cpu->ReadMemoryWord(Segment_SS, BP);
        cpu->PushWord(prev_ptr);
      }
      cpu->PushDWord(frame_pointer);
      cpu->m_registers.BP = BP;
    }
    else
    {
      uint32 EBP = cpu->m_registers.EBP;
      for (uint8 i = 1; i < level; i++)
      {
        EBP -= sizeof(uint32);

        uint32 prev_ptr = cpu->ReadMemoryDWord(Segment_SS, EBP);
        cpu->PushDWord(prev_ptr);
      }
      cpu->PushDWord(frame_pointer);
      cpu->m_registers.EBP = EBP;
    }
  }

  if (instruction->operand_size == OperandSize_16)
    cpu->m_registers.BP = Truncate16(frame_pointer);
  else
    cpu->m_registers.EBP = frame_pointer;

  if (cpu->m_stack_address_size == AddressSize_16)
    cpu->m_registers.SP -= stack_frame_size;
  else
    cpu->m_registers.ESP -= stack_frame_size;
}

void Interpreter::Execute_LEAVE(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->m_stack_address_size == AddressSize_16)
    cpu->m_registers.SP = cpu->m_registers.BP;
  else
    cpu->m_registers.ESP = cpu->m_registers.EBP;

  if (instruction->operand_size == OperandSize_16)
    cpu->m_registers.BP = cpu->PopWord();
  else if (instruction->operand_size == OperandSize_32)
    cpu->m_registers.EBP = cpu->PopDWord();
  else
    DebugUnreachableCode();
}

#pragma pack(push, 1)
union LOADALL_286_TABLE
{
  struct ADDRESS24
  {
    uint8 bits[3];

    uint32 GetValue() const
    {
      return ((ZeroExtend32(bits[0])) | (ZeroExtend32(bits[1]) << 8) | (ZeroExtend32(bits[2]) << 16));
    }
  };

  struct DESC_CACHE_286
  {
    ADDRESS24 physical_address;
    uint8 access_rights_or_zero;
    uint16 limit;
  };

  struct
  {
    uint16 unused_00;
    uint16 unused_02;
    uint16 MSW;
    uint16 unused_06;
    uint16 unused_08;
    uint16 unused_0A;
    uint16 unused_0C;
    uint16 unused_0E;
    uint16 unused_10;
    uint16 unused_12;
    uint16 unused_14;
    uint16 TR_REG;
    uint16 FLAGS;
    uint16 IP;
    uint16 LDT_REG;
    uint16 DS_REG;
    uint16 SS_REG;
    uint16 CS_REG;
    uint16 ES_REG;
    uint16 DI;
    uint16 SI;
    uint16 BP;
    uint16 SP;
    uint16 BX;
    uint16 DX;
    uint16 CX;
    uint16 AX;
    DESC_CACHE_286 ES_DESC;
    DESC_CACHE_286 CS_DESC;
    DESC_CACHE_286 SS_DESC;
    DESC_CACHE_286 DS_DESC;
    DESC_CACHE_286 GDT_DESC;
    DESC_CACHE_286 LDT_DESC;
    DESC_CACHE_286 IDT_DESC;
    DESC_CACHE_286 TSS_DESC;
  };

  uint16 words[51];
};
static_assert(sizeof(LOADALL_286_TABLE) == 0x66, "286 loadall table is correct size");
#pragma pack(pop)

void Interpreter::Execute_LOADALL_286(CPU* cpu, const OldInstruction* instruction)
{
  // trace_execution = true;
  if (cpu->m_model != MODEL_286)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  // TODO: Check CPL = 0
  Log_WarningPrint("Executing LOADALL instruction");

  LOADALL_286_TABLE table = {};
  for (uint32 i = 0; i < countof(table.words); i++)
    table.words[i] = cpu->m_bus->ReadMemoryWord(0x800 + i * 2);

  // 286 can't switch from protected to real mode.
  // cpu->m_registers.CR0 = table.MSW;
  if (table.MSW & CR0Bit_PE)
    cpu->m_registers.CR0 |= CR0Bit_PE;

  cpu->SetFlags16(table.FLAGS);
  cpu->m_registers.IP = table.IP;
  cpu->m_registers.DS = table.DS_REG;
  cpu->m_registers.SS = table.SS_REG;
  cpu->m_registers.CS = table.CS_REG;
  cpu->m_registers.ES = table.ES_REG;
  cpu->m_registers.DI = table.DI;
  cpu->m_registers.SI = table.SI;
  cpu->m_registers.BP = table.BP;
  cpu->m_registers.SP = table.SP;
  cpu->m_registers.BX = table.BX;
  cpu->m_registers.DX = table.DX;
  cpu->m_registers.CX = table.CX;
  cpu->m_registers.AX = table.AX;

  // TODO: Handle expand-up here, and access mask.
  cpu->m_segment_cache[Segment_ES].base_address = table.ES_DESC.physical_address.GetValue();
  cpu->m_segment_cache[Segment_ES].access.bits = table.ES_DESC.access_rights_or_zero;
  cpu->m_segment_cache[Segment_ES].limit_low = 0;
  cpu->m_segment_cache[Segment_ES].limit_high = table.ES_DESC.limit;
  cpu->m_segment_cache[Segment_ES].limit_raw = table.ES_DESC.limit;
  cpu->m_segment_cache[Segment_ES].access_mask = AccessTypeMask::All;
  cpu->m_segment_cache[Segment_CS].base_address = table.CS_DESC.physical_address.GetValue();
  cpu->m_segment_cache[Segment_CS].access.bits = table.CS_DESC.access_rights_or_zero;
  cpu->m_segment_cache[Segment_CS].limit_low = 0;
  cpu->m_segment_cache[Segment_CS].limit_high = table.CS_DESC.limit;
  cpu->m_segment_cache[Segment_CS].limit_raw = table.CS_DESC.limit;
  cpu->m_segment_cache[Segment_CS].access_mask = AccessTypeMask::All;
  cpu->m_segment_cache[Segment_SS].base_address = table.SS_DESC.physical_address.GetValue();
  cpu->m_segment_cache[Segment_SS].access.bits = table.SS_DESC.access_rights_or_zero;
  cpu->m_segment_cache[Segment_SS].limit_low = 0;
  cpu->m_segment_cache[Segment_SS].limit_high = table.SS_DESC.limit;
  cpu->m_segment_cache[Segment_SS].limit_raw = table.SS_DESC.limit;
  cpu->m_segment_cache[Segment_SS].access_mask = AccessTypeMask::All;
  cpu->m_segment_cache[Segment_DS].base_address = table.DS_DESC.physical_address.GetValue();
  cpu->m_segment_cache[Segment_DS].access.bits = table.DS_DESC.access_rights_or_zero;
  cpu->m_segment_cache[Segment_DS].limit_low = 0;
  cpu->m_segment_cache[Segment_DS].limit_high = table.DS_DESC.limit;
  cpu->m_segment_cache[Segment_DS].limit_raw = table.DS_DESC.limit;
  cpu->m_segment_cache[Segment_DS].access_mask = AccessTypeMask::All;

  cpu->m_gdt_location.base_address = table.GDT_DESC.physical_address.GetValue();
  cpu->m_gdt_location.limit = table.GDT_DESC.limit;
  cpu->m_ldt_location.base_address = table.LDT_DESC.physical_address.GetValue();
  cpu->m_ldt_location.limit = table.LDT_DESC.limit;
  cpu->m_idt_location.base_address = table.IDT_DESC.physical_address.GetValue();
  cpu->m_idt_location.limit = table.IDT_DESC.limit;
}

void Interpreter::Execute_MOVSX(CPU* cpu, const OldInstruction* instruction)
{
  // The sign-extending is done behind the scenes in the operand read functions
  if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadSignExtendedWordOperand(cpu, instruction, &instruction->operands[1]);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadSignExtendedDWordOperand(cpu, instruction, &instruction->operands[1]);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_MOVZX(CPU* cpu, const OldInstruction* instruction)
{
  // The zero-extending is done behind the scenes in the operand read functions
  if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    WriteWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_MOV_CR(CPU* cpu, const OldInstruction* instruction)
{
  DebugAssert(instruction->operands[0].size == OperandSize_32 && instruction->operands[1].size == OperandSize_32);

  // Requires privilege level zero
  if ((cpu->InProtectedMode() && cpu->GetCPL() != 0) || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // Load control register
  if (instruction->operands[0].mode == AddressingMode_ControlRegister)
  {
    DebugAssert(instruction->operands[1].mode == AddressingMode_Register);
    DebugAssert(instruction->operands[1].reg.reg32 < Reg32_Count);
    uint32 value = cpu->m_registers.reg32[instruction->operands[1].reg.reg32];

    // Validate selected register
    switch (instruction->operands[0].reg.raw)
    {
      case 0:
      {
        // GP(0) if an attempt is made to write invalid bit combinations in CR0 (such as
        // setting the PG flag to 1 when the PE flag is set to 0, or setting the CD flag
        // to 0 when the NW flag is set to 1).
        if ((value & CR0Bit_PG) != 0 && !(value & CR0Bit_PE))
        {
          cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
          return;
        }

        cpu->LoadSpecialRegister(Reg32_CR0, value);
      }
      break;

      case 2:
        cpu->LoadSpecialRegister(Reg32_CR2, value);
        break;

      case 3:
        cpu->LoadSpecialRegister(Reg32_CR3, value);
        break;

      case 4:
      {
        // TODO: Validate reserved bits in CR4, GP(0) if any are set to 1.
        cpu->LoadSpecialRegister(Reg32_CR4, value);
      }
      break;

      default:
        cpu->RaiseException(Interrupt_InvalidOpcode);
        return;
    }
  }
  // Store control register
  else if (instruction->operands[1].mode == AddressingMode_ControlRegister)
  {
    // Validate selected register
    uint32 value;
    switch (instruction->operands[1].reg.raw)
    {
      case 0:
        value = cpu->m_registers.CR0;
        break;
      case 2:
        value = cpu->m_registers.CR2;
        break;
      case 3:
        value = cpu->m_registers.CR3;
        break;
      case 4:
        value = cpu->m_registers.CR4;
        break;
      default:
        cpu->RaiseException(Interrupt_InvalidOpcode);
        return;
    }

    DebugAssert(instruction->operands[0].mode == AddressingMode_Register);
    DebugAssert(instruction->operands[0].reg.reg32 < Reg32_Count);
    cpu->m_registers.reg32[instruction->operands[0].reg.reg32] = value;
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_MOV_DR(CPU* cpu, const OldInstruction* instruction)
{
  DebugAssert(instruction->operands[0].size == OperandSize_32 && instruction->operands[1].size == OperandSize_32);

  // Requires privilege level zero
  if ((cpu->InProtectedMode() && cpu->GetCPL() != 0) || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // TODO Validation:
  // #UD If CR4.DE[bit 3] = 1 (debug extensions) and a MOV instruction is executed involving DR4 or DR5.
  // #DB If any debug register is accessed while the DR7.GD[bit 13] = 1.

  // Load debug register
  if (instruction->operands[0].mode == AddressingMode_DebugRegister)
  {
    if (instruction->operands[0].reg.raw >= 8)
    {
      cpu->RaiseException(Interrupt_InvalidOpcode);
      return;
    }

    DebugAssert(instruction->operands[1].mode == AddressingMode_Register);
    DebugAssert(instruction->operands[1].reg.reg32 < Reg32_Count);
    uint32 value = cpu->m_registers.reg32[instruction->operands[1].reg.reg32];
    cpu->LoadSpecialRegister(static_cast<Reg32>(Reg32_DR0 + instruction->operands[0].reg.raw), value);
  }
  // Store debug register
  else if (instruction->operands[1].mode == AddressingMode_DebugRegister)
  {
    if (instruction->operands[1].reg.raw >= 8)
    {
      cpu->RaiseException(Interrupt_InvalidOpcode);
      return;
    }

    uint32 value = cpu->m_registers.reg32[Reg32_DR0 + instruction->operands[1].reg.raw];
    DebugAssert(instruction->operands[0].mode == AddressingMode_Register);
    cpu->m_registers.reg32[instruction->operands[0].reg.reg32] = value;
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_MOV_TR(CPU* cpu, const OldInstruction* instruction)
{
  DebugAssert(instruction->operands[0].size == OperandSize_32 && instruction->operands[1].size == OperandSize_32);

  // Requires privilege level zero
  if ((cpu->InProtectedMode() && cpu->GetCPL() != 0) || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // Load test register
  if (instruction->operands[0].mode == AddressingMode_TestRegister)
  {
    DebugAssert(instruction->operands[1].mode == AddressingMode_Register);
    DebugAssert(instruction->operands[1].reg.reg32 < Reg32_Count);
    uint32 value = cpu->m_registers.reg32[instruction->operands[1].reg.reg32];

    // Validate selected register
    switch (instruction->operands[1].reg.raw)
    {
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
        // Only load to registers 3-7
        cpu->LoadSpecialRegister(static_cast<Reg32>(Reg32_TR3 + (instruction->operands[1].reg.raw - 3)), value);
        break;

      default:
        cpu->RaiseException(Interrupt_InvalidOpcode);
        return;
    }
  }
  // Store test register
  else if (instruction->operands[1].mode == AddressingMode_TestRegister)
  {
    DebugAssert(instruction->operands[0].mode == AddressingMode_Register);

    // Validate selected register
    uint32 value;
    switch (instruction->operands[1].reg.raw)
    {
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
        value = cpu->m_registers.reg32[Reg32_TR3 + (instruction->operands[1].reg.raw - 3)];
        break;

      default:
        cpu->RaiseException(Interrupt_InvalidOpcode);
        return;
    }

    DebugAssert(instruction->operands[0].reg.reg32 < Reg32_Count);
    cpu->m_registers.reg32[instruction->operands[0].reg.reg32] = value;
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_SETcc(CPU* cpu, const OldInstruction* instruction)
{
  bool flag = TestCondition(cpu, instruction->jump_condition, instruction->address_size);
  WriteByteOperand(cpu, instruction, &instruction->operands[0], BoolToUInt8(flag));
}

void Interpreter::Execute_BSWAP(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    value = Y_byteswap_uint32(value);
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_CMPXCHG(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 dest = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 source = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    if (ALUOp_Sub8(&cpu->m_registers, cpu->m_registers.AL, dest) == 0)
    {
      // ZF should be set by the ALU op
      Assert(cpu->m_registers.EFLAGS.ZF);
      WriteByteOperand(cpu, instruction, &instruction->operands[0], source);
    }
    else
    {
      // ZF should be clear by the ALU op
      // Even if the test passes the write is still issued
      Assert(!cpu->m_registers.EFLAGS.ZF);
      WriteByteOperand(cpu, instruction, &instruction->operands[0], dest);
      cpu->m_registers.AL = dest;
    }
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 dest = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 source = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    if (ALUOp_Sub16(&cpu->m_registers, cpu->m_registers.AX, dest) == 0)
    {
      // ZF should be set by the ALU op
      Assert(cpu->m_registers.EFLAGS.ZF);
      WriteWordOperand(cpu, instruction, &instruction->operands[0], source);
    }
    else
    {
      // ZF should be clear by the ALU op
      // Even if the test passes the write is still issued
      Assert(!cpu->m_registers.EFLAGS.ZF);
      WriteWordOperand(cpu, instruction, &instruction->operands[0], dest);
      cpu->m_registers.AX = dest;
    }
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 dest = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 source = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    if (ALUOp_Sub32(&cpu->m_registers, cpu->m_registers.EAX, dest) == 0)
    {
      // ZF should be set by the ALU op
      Assert(cpu->m_registers.EFLAGS.ZF);
      WriteDWordOperand(cpu, instruction, &instruction->operands[0], source);
    }
    else
    {
      // ZF should be clear by the ALU op
      // Even if the test passes the write is still issued
      Assert(!cpu->m_registers.EFLAGS.ZF);
      WriteDWordOperand(cpu, instruction, &instruction->operands[0], dest);
      cpu->m_registers.EAX = dest;
    }
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_CMOVcc(CPU* cpu, const OldInstruction* instruction)
{
  // NOTE: Memory access is performed even if the predicate does not hold.
  bool do_move = TestCondition(cpu, instruction->jump_condition, instruction->address_size);
  if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 value = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    if (do_move)
      WriteWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    if (do_move)
      WriteDWordOperand(cpu, instruction, &instruction->operands[0], value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_WBINVD(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->InVirtual8086Mode() || (cpu->InProtectedMode() && cpu->GetCPL() != 0))
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  Log_WarningPrintf("WBINVD instruction");
}

void Interpreter::Execute_INVLPG(CPU* cpu, const OldInstruction* instruction)
{
  if (cpu->InVirtual8086Mode() || (cpu->InProtectedMode() && cpu->GetCPL() != 0))
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // Get effective address of operand, this is the linear address to clear.
  VirtualMemoryAddress address = CalculateEffectiveAddress(cpu, instruction, &instruction->operands[0]);
  cpu->InvalidateTLBEntry(address);
}

void Interpreter::Execute_XADD(CPU* cpu, const OldInstruction* instruction)
{
  if (instruction->operands[0].size == OperandSize_8)
  {
    uint8 dst = ReadByteOperand(cpu, instruction, &instruction->operands[0]);
    uint8 src = ReadByteOperand(cpu, instruction, &instruction->operands[1]);
    uint8 tmp = ALUOp_Add8(&cpu->m_registers, dst, src);
    src = dst;
    dst = tmp;
    WriteByteOperand(cpu, instruction, &instruction->operands[0], dst);
    WriteByteOperand(cpu, instruction, &instruction->operands[1], src);
  }
  else if (instruction->operands[0].size == OperandSize_16)
  {
    uint16 dst = ReadWordOperand(cpu, instruction, &instruction->operands[0]);
    uint16 src = ReadWordOperand(cpu, instruction, &instruction->operands[1]);
    uint16 tmp = ALUOp_Add16(&cpu->m_registers, dst, src);
    src = dst;
    dst = tmp;
    WriteWordOperand(cpu, instruction, &instruction->operands[0], dst);
    WriteWordOperand(cpu, instruction, &instruction->operands[1], src);
  }
  else if (instruction->operands[0].size == OperandSize_32)
  {
    uint32 dst = ReadDWordOperand(cpu, instruction, &instruction->operands[0]);
    uint32 src = ReadDWordOperand(cpu, instruction, &instruction->operands[1]);
    uint32 tmp = ALUOp_Add32(&cpu->m_registers, dst, src);
    src = dst;
    dst = tmp;
    WriteDWordOperand(cpu, instruction, &instruction->operands[0], dst);
    WriteDWordOperand(cpu, instruction, &instruction->operands[1], src);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

void Interpreter::Execute_RDTSC(CPU* cpu, const OldInstruction* instruction)
{
  // TSD flag in CR4 controls whether this is privileged or unprivileged
  Log_WarningPrintf("RDTSC instruction");
  cpu->m_registers.EDX = 0;
  cpu->m_registers.EAX = 0;
}
} // namespace CPU_X86