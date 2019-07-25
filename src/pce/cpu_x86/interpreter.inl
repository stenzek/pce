#include "YBaseLib/Endian.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/String.h"
#include "pce/bus.h"
#include "pce/cpu_x86/interpreter.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"

#ifdef Y_COMPILER_MSVC
#include <intrin.h>

// The if constexpr warning is noise, the compiler eliminates the other branches anyway.
#pragma warning(push)
#pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#endif

namespace CPU_X86 {
void Interpreter::RaiseInvalidOpcode(CPU* cpu)
{
  cpu->PrintCurrentStateAndInstruction(cpu->m_current_EIP, "Invalid opcode raised at ");
  cpu->RaiseException(Interrupt_InvalidOpcode);
}

void Interpreter::FetchModRM(CPU* cpu)
{
  cpu->idata.modrm = cpu->FetchInstructionByte();
}

template<OperandSize op_size, OperandMode op_mode, u32 op_constant>
void Interpreter::FetchImmediate(CPU* cpu)
{
  switch (op_mode)
  {
    case OperandMode_Immediate:
    {
      OperandSize actual_size = (op_size == OperandSize_Count) ? cpu->idata.operand_size : op_size;
      switch (actual_size)
      {
        case OperandSize_8:
          cpu->idata.imm8 = cpu->FetchInstructionByte();
          break;
        case OperandSize_16:
          cpu->idata.imm16 = cpu->FetchInstructionWord();
          break;
        case OperandSize_32:
          cpu->idata.imm32 = cpu->FetchInstructionDWord();
          break;
      }
    }
    break;

    case OperandMode_Immediate2:
    {
      OperandSize actual_size = (op_size == OperandSize_Count) ? cpu->idata.operand_size : op_size;
      switch (actual_size)
      {
        case OperandSize_8:
          cpu->idata.imm2_8 = cpu->FetchInstructionByte();
          break;
        case OperandSize_16:
          cpu->idata.imm2_16 = cpu->FetchInstructionWord();
          break;
        case OperandSize_32:
          cpu->idata.imm2_32 = cpu->FetchInstructionDWord();
          break;
      }
    }
    break;

    case OperandMode_Relative:
    {
      OperandSize actual_size = (op_size == OperandSize_Count) ? cpu->idata.operand_size : op_size;
      switch (actual_size)
      {
        case OperandSize_8:
          cpu->idata.disp32 = SignExtend32(cpu->FetchInstructionByte());
          break;
        case OperandSize_16:
          cpu->idata.disp32 = SignExtend32(cpu->FetchInstructionWord());
          break;
        case OperandSize_32:
          cpu->idata.disp32 = SignExtend32(cpu->FetchInstructionDWord());
          break;
      }
    }
    break;

    case OperandMode_Memory:
    {
      if (cpu->idata.address_size == AddressSize_16)
        cpu->idata.disp32 = ZeroExtend32(cpu->FetchInstructionWord());
      else
        cpu->idata.disp32 = cpu->FetchInstructionDWord();
    }
    break;

    case OperandMode_FarAddress:
    {
      if (cpu->idata.operand_size == OperandSize_16)
        cpu->idata.disp32 = ZeroExtend32(cpu->FetchInstructionWord());
      else
        cpu->idata.disp32 = cpu->FetchInstructionDWord();
      cpu->idata.imm16 = cpu->FetchInstructionWord();
    }
    break;

    case OperandMode_ModRM_RM:
    {
      const Decoder::ModRMAddress* addr = Decoder::DecodeModRMAddress(cpu->idata.address_size, cpu->idata.modrm);
      if (addr->addressing_mode == ModRMAddressingMode::Register)
      {
        // not a memory address
        cpu->idata.modrm_rm_register = true;
      }
      else
      {
        u8 displacement_size = addr->displacement_size;
        if (addr->addressing_mode == ModRMAddressingMode::SIB)
        {
          // SIB has a displacement instead of base if set to EBP
          cpu->idata.sib = cpu->FetchInstructionByte();
          const Reg32 base_reg = cpu->idata.GetSIBBaseRegister();
          if (!cpu->idata.HasSIBBase())
            displacement_size = 4;
          else if (!cpu->idata.has_segment_override && (base_reg == Reg32_ESP || base_reg == Reg32_EBP))
            cpu->idata.segment = Segment_SS;
        }
        else
        {
          if (!cpu->idata.has_segment_override)
            cpu->idata.segment = addr->default_segment;
        }

        switch (displacement_size)
        {
          case 1:
            cpu->idata.disp32 = SignExtend32(cpu->FetchInstructionByte());
            break;
          case 2:
            cpu->idata.disp32 = SignExtend32(cpu->FetchInstructionWord());
            break;
          case 4:
            cpu->idata.disp32 = cpu->FetchInstructionDWord();
            break;
        }
      }
    }
    break;
  }
}

template<OperandMode op_mode>
void Interpreter::CalculateEffectiveAddress(CPU* cpu)
{
  switch (op_mode)
  {
    case OperandMode_ModRM_RM:
    {
      // NOTE: The uint16() cast here is needed otherwise the result is an int rather than uint16.
      u8 index = ((cpu->idata.modrm >> 6) << 3) | (cpu->idata.modrm & 7);
      if (cpu->idata.address_size == AddressSize_16)
      {
        switch (index & 31)
        {
          case 0:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BX + cpu->m_registers.SI));
            break;
          case 1:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BX + cpu->m_registers.DI));
            break;
          case 2:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BP + cpu->m_registers.SI));
            break;
          case 3:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BP + cpu->m_registers.DI));
            break;
          case 4:
            cpu->m_effective_address = ZeroExtend32(cpu->m_registers.SI);
            break;
          case 5:
            cpu->m_effective_address = ZeroExtend32(cpu->m_registers.DI);
            break;
          case 6:
            cpu->m_effective_address = ZeroExtend32(cpu->idata.disp16);
            break;
          case 7:
            cpu->m_effective_address = ZeroExtend32(cpu->m_registers.BX);
            break;
          case 8:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BX + cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 9:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BX + cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 10:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BP + cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 11:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BP + cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 12:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 13:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 14:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BP + cpu->idata.disp16));
            break;
          case 15:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BX + cpu->idata.disp16));
            break;
          case 16:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BX + cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 17:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BX + cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 18:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BP + cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 19:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BP + cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 20:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 21:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 22:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BP + cpu->idata.disp16));
            break;
          case 23:
            cpu->m_effective_address = ZeroExtend32(u16(cpu->m_registers.BX + cpu->idata.disp16));
            break;
          case 24:
            cpu->m_effective_address = Reg16_AX;
            break;
          case 25:
            cpu->m_effective_address = Reg16_CX;
            break;
          case 26:
            cpu->m_effective_address = Reg16_DX;
            break;
          case 27:
            cpu->m_effective_address = Reg16_BX;
            break;
          case 28:
            cpu->m_effective_address = Reg16_SP;
            break;
          case 29:
            cpu->m_effective_address = Reg16_BP;
            break;
          case 30:
            cpu->m_effective_address = Reg16_SI;
            break;
          case 31:
            cpu->m_effective_address = Reg16_DI;
            break;
        }
      }
      else
      {
        // TODO: Collapse identical cases
        switch (index)
        {
          case 0:
            cpu->m_effective_address = cpu->m_registers.EAX;
            break;
          case 1:
            cpu->m_effective_address = cpu->m_registers.ECX;
            break;
          case 2:
            cpu->m_effective_address = cpu->m_registers.EDX;
            break;
          case 3:
            cpu->m_effective_address = cpu->m_registers.EBX;
            break;
          case 5:
            cpu->m_effective_address = cpu->idata.disp32;
            break;
          case 6:
            cpu->m_effective_address = cpu->m_registers.ESI;
            break;
          case 7:
            cpu->m_effective_address = cpu->m_registers.EDI;
            break;
          case 8:
            cpu->m_effective_address = cpu->m_registers.EAX + cpu->idata.disp32;
            break;
          case 9:
            cpu->m_effective_address = cpu->m_registers.ECX + cpu->idata.disp32;
            break;
          case 10:
            cpu->m_effective_address = cpu->m_registers.EDX + cpu->idata.disp32;
            break;
          case 11:
            cpu->m_effective_address = cpu->m_registers.EBX + cpu->idata.disp32;
            break;
          case 13:
            cpu->m_effective_address = cpu->m_registers.EBP + cpu->idata.disp32;
            break;
          case 14:
            cpu->m_effective_address = cpu->m_registers.ESI + cpu->idata.disp32;
            break;
          case 15:
            cpu->m_effective_address = cpu->m_registers.EDI + cpu->idata.disp32;
            break;
          case 16:
            cpu->m_effective_address = cpu->m_registers.EAX + cpu->idata.disp32;
            break;
          case 17:
            cpu->m_effective_address = cpu->m_registers.ECX + cpu->idata.disp32;
            break;
          case 18:
            cpu->m_effective_address = cpu->m_registers.EDX + cpu->idata.disp32;
            break;
          case 19:
            cpu->m_effective_address = cpu->m_registers.EBX + cpu->idata.disp32;
            break;
          case 21:
            cpu->m_effective_address = cpu->m_registers.EBP + cpu->idata.disp32;
            break;
          case 22:
            cpu->m_effective_address = cpu->m_registers.ESI + cpu->idata.disp32;
            break;
          case 23:
            cpu->m_effective_address = cpu->m_registers.EDI + cpu->idata.disp32;
            break;
          case 24:
            cpu->m_effective_address = Reg32_EAX;
            break;
          case 25:
            cpu->m_effective_address = Reg32_ECX;
            break;
          case 26:
            cpu->m_effective_address = Reg32_EDX;
            break;
          case 27:
            cpu->m_effective_address = Reg32_EBX;
            break;
          case 28:
            cpu->m_effective_address = Reg32_ESP;
            break;
          case 29:
            cpu->m_effective_address = Reg32_EBP;
            break;
          case 30:
            cpu->m_effective_address = Reg32_ESI;
            break;
          case 31:
            cpu->m_effective_address = Reg32_EDI;
            break;

          case 4:
          case 12:
          case 20:
          {
            // SIB modes
            const u32 base_addr = cpu->idata.HasSIBBase() ? cpu->m_registers.reg32[cpu->idata.GetSIBBaseRegister()] : 0;
            const u32 index_addr =
              cpu->idata.HasSIBIndex() ? cpu->m_registers.reg32[cpu->idata.GetSIBIndexRegister()] : 0;
            const u8 scaling_factor = cpu->idata.GetSIBScaling();
            const u32 displacement = cpu->idata.disp32;
            cpu->m_effective_address = base_addr;
            cpu->m_effective_address += index_addr << scaling_factor;
            cpu->m_effective_address += displacement;
          }
          break;
        }
      }
    }
    break;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
VirtualMemoryAddress Interpreter::CalculateJumpTarget(CPU* cpu)
{
  static_assert(dst_mode == OperandMode_Relative || dst_mode == OperandMode_ModRM_RM,
                "Operand mode is relative or indirect");

  if constexpr (dst_mode == OperandMode_Relative)
  {
    return (cpu->m_registers.EIP + cpu->idata.disp32) & cpu->idata.GetOperandSizeMask();
  }
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
  {
    return ReadZeroExtendedDWordOperand<dst_size, dst_mode, dst_constant>(cpu);
  }
  else
  {
    DebugUnreachableCode();
    return 0;
  }
}

template<OperandMode mode, u32 constant>
u8 Interpreter::ReadByteOperand(CPU* cpu)
{
  switch (mode)
  {
    case OperandMode_Constant:
      return Truncate8(constant);
    case OperandMode_Register:
      return cpu->m_registers.reg8[constant];
    case OperandMode_Immediate:
      return cpu->idata.imm8;
    case OperandMode_Immediate2:
      return cpu->idata.imm2_8;
      //     case OperandMode_RegisterIndirect:
      //         {
      //             uint8 value;
      //             if (cpu->idata.address_size == AddressSize_16)
      //                 cpu->ReadMemoryByte(cpu->idata.segment, ZeroExtend32(cpu->m_registers.reg16[constant]),
      //                 &value);
      //             else
      //                 cpu->ReadMemoryByte(cpu->idata.segment, cpu->m_registers.reg32[constant], &value);
      //             return value;
      //         }
    case OperandMode_Memory:
      return cpu->ReadMemoryByte(cpu->idata.segment, cpu->idata.disp32);
    case OperandMode_ModRM_RM:
    {
      if (cpu->idata.modrm_rm_register)
        return cpu->m_registers.reg8[cpu->m_effective_address];
      else
        return cpu->ReadMemoryByte(cpu->idata.segment, cpu->m_effective_address);
    }
    case OperandMode_ModRM_Reg:
      return cpu->m_registers.reg8[cpu->idata.GetModRM_Reg()];
    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<OperandMode mode, u32 constant>
u16 Interpreter::ReadWordOperand(CPU* cpu)
{
  switch (mode)
  {
    case OperandMode_Constant:
      return Truncate16(constant);
    case OperandMode_Register:
      return cpu->m_registers.reg16[constant];
    case OperandMode_Immediate:
      return cpu->idata.imm16;
    case OperandMode_Immediate2:
      return cpu->idata.imm2_16;
    case OperandMode_Memory:
      return cpu->ReadMemoryWord(cpu->idata.segment, cpu->idata.disp32);
    case OperandMode_ModRM_RM:
    {
      if (cpu->idata.modrm_rm_register)
        return cpu->m_registers.reg16[cpu->m_effective_address];
      else
        return cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_effective_address);
    }
    case OperandMode_ModRM_Reg:
      return cpu->m_registers.reg16[cpu->idata.GetModRM_Reg()];

    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<OperandMode mode, u32 constant>
u32 Interpreter::ReadDWordOperand(CPU* cpu)
{
  switch (mode)
  {
    case OperandMode_Constant:
      return constant;
    case OperandMode_Register:
      return cpu->m_registers.reg32[constant];
    case OperandMode_Immediate:
      return cpu->idata.imm32;
    case OperandMode_Immediate2:
      return cpu->idata.imm2_32;
    case OperandMode_Memory:
      return cpu->ReadMemoryDWord(cpu->idata.segment, cpu->idata.disp32);
    case OperandMode_ModRM_RM:
    {
      if (cpu->idata.modrm_rm_register)
        return cpu->m_registers.reg32[cpu->m_effective_address];
      else
        return cpu->ReadMemoryDWord(cpu->idata.segment, cpu->m_effective_address);
    }
    case OperandMode_ModRM_Reg:
      return cpu->m_registers.reg32[cpu->idata.GetModRM_Reg()];

      //     case OperandMode_ModRM_ControlRegister:
      //         {
      //             uint8 reg = (cpu->idata.modrm >> 3) & 7;
      //             if (reg > 4)
      //                 RaiseInvalidOpcode();
      //             return cpu->m_registers.reg32[Reg32_CR0 + reg];
      //         }

    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<OperandSize size, OperandMode mode, u32 constant>
u16 Interpreter::ReadSignExtendedWordOperand(CPU* cpu)
{
  const OperandSize actual_size = (size == OperandSize_Count) ? cpu->idata.operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      u8 value;
      switch (mode)
      {
        case OperandMode_Register:
          value = cpu->m_registers.reg8[constant];
          break;
        case OperandMode_Immediate:
          value = cpu->idata.imm8;
          break;
        case OperandMode_Immediate2:
          value = cpu->idata.imm2_8;
          break;
        case OperandMode_Memory:
          value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->idata.disp32);
          break;
        case OperandMode_ModRM_Reg:
          value = cpu->m_registers.reg8[cpu->idata.GetModRM_Reg()];
          break;
        case OperandMode_ModRM_RM:
          if (cpu->idata.modrm_rm_register)
            value = cpu->m_registers.reg8[cpu->m_effective_address];
          else
            value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->m_effective_address);
          break;
        default:
          DebugUnreachableCode();
          return 0;
      }
      return SignExtend16(value);
    }
    case OperandSize_16:
      return ReadWordOperand<mode, constant>(cpu);
    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<OperandSize size, OperandMode mode, u32 constant>
u32 Interpreter::ReadSignExtendedDWordOperand(CPU* cpu)
{
  const OperandSize actual_size = (size == OperandSize_Count) ? cpu->idata.operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      u8 value;
      switch (mode)
      {
        case OperandMode_Register:
          value = cpu->m_registers.reg8[constant];
          break;
        case OperandMode_Immediate:
          value = cpu->idata.imm8;
          break;
        case OperandMode_Immediate2:
          value = cpu->idata.imm2_8;
          break;
        case OperandMode_Memory:
          value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->idata.disp32);
          break;
        case OperandMode_ModRM_Reg:
          value = cpu->m_registers.reg8[cpu->idata.GetModRM_Reg()];
          break;
        case OperandMode_ModRM_RM:
          if (cpu->idata.modrm_rm_register)
            value = cpu->m_registers.reg8[cpu->m_effective_address];
          else
            value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->m_effective_address);
          break;
        default:
          DebugUnreachableCode();
          return 0;
      }
      return SignExtend32(value);
    }
    case OperandSize_16:
    {
      u16 value;
      switch (mode)
      {
        case OperandMode_Register:
          value = cpu->m_registers.reg16[constant];
          break;
        case OperandMode_Immediate:
          value = cpu->idata.imm16;
          break;
        case OperandMode_Immediate2:
          value = cpu->idata.imm2_16;
          break;
        case OperandMode_Memory:
          value = cpu->ReadMemoryWord(cpu->idata.segment, cpu->idata.disp32);
          break;
        case OperandMode_ModRM_Reg:
          value = cpu->m_registers.reg16[cpu->idata.GetModRM_Reg()];
          break;
        case OperandMode_ModRM_RM:
          if (cpu->idata.modrm_rm_register)
            value = cpu->m_registers.reg16[cpu->m_effective_address];
          else
            value = cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_effective_address);
          break;
        default:
          DebugUnreachableCode();
          return 0;
      }
      return SignExtend32(value);
    }
    case OperandSize_32:
      return ReadDWordOperand<mode, constant>(cpu);
    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<OperandSize size, OperandMode mode, u32 constant>
u16 Interpreter::ReadZeroExtendedWordOperand(CPU* cpu)
{
  const OperandSize actual_size = (size == OperandSize_Count) ? cpu->idata.operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      u8 value;
      switch (mode)
      {
        case OperandMode_Constant:
          value = Truncate8(constant);
          break;
        case OperandMode_Register:
          value = cpu->m_registers.reg8[constant];
          break;
        case OperandMode_Immediate:
          value = cpu->idata.imm8;
          break;
        case OperandMode_Immediate2:
          value = cpu->idata.imm2_8;
          break;
        case OperandMode_Memory:
          value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->idata.disp32);
          break;
        case OperandMode_ModRM_Reg:
          value = cpu->m_registers.reg8[cpu->idata.GetModRM_Reg()];
          break;
        case OperandMode_ModRM_RM:
          if (cpu->idata.modrm_rm_register)
            value = cpu->m_registers.reg8[cpu->m_effective_address];
          else
            value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->m_effective_address);
          break;
        default:
          DebugUnreachableCode();
          return 0;
      }
      return ZeroExtend16(value);
    }
    case OperandSize_16:
      return ReadWordOperand<mode, constant>(cpu);
    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<OperandSize size, OperandMode mode, u32 constant>
u32 Interpreter::ReadZeroExtendedDWordOperand(CPU* cpu)
{
  const OperandSize actual_size = (size == OperandSize_Count) ? cpu->idata.operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      u8 value;
      switch (mode)
      {
        case OperandMode_Constant:
          value = Truncate8(constant);
          break;
        case OperandMode_Register:
          value = cpu->m_registers.reg8[constant];
          break;
        case OperandMode_Immediate:
          value = cpu->idata.imm8;
          break;
        case OperandMode_Immediate2:
          value = cpu->idata.imm2_8;
          break;
        case OperandMode_Memory:
          value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->idata.disp32);
          break;
        case OperandMode_ModRM_Reg:
          value = cpu->m_registers.reg8[cpu->idata.GetModRM_Reg()];
          break;
        case OperandMode_ModRM_RM:
          if (cpu->idata.modrm_rm_register)
            value = cpu->m_registers.reg8[cpu->m_effective_address];
          else
            value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->m_effective_address);
          break;
        default:
          DebugUnreachableCode();
          return 0;
      }
      return ZeroExtend32(value);
    }
    case OperandSize_16:
    {
      u16 value;
      switch (mode)
      {
        case OperandMode_Constant:
          value = Truncate16(constant);
          break;
        case OperandMode_Register:
          value = cpu->m_registers.reg16[constant];
          break;
        case OperandMode_Immediate:
          value = cpu->idata.imm16;
          break;
        case OperandMode_Immediate2:
          value = cpu->idata.imm2_16;
          break;
        case OperandMode_Memory:
          value = cpu->ReadMemoryWord(cpu->idata.segment, cpu->idata.disp32);
          break;
        case OperandMode_ModRM_Reg:
          value = cpu->m_registers.reg16[cpu->idata.GetModRM_Reg()];
          break;
        case OperandMode_ModRM_RM:
          if (cpu->idata.modrm_rm_register)
            value = cpu->m_registers.reg16[cpu->m_effective_address];
          else
            value = cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_effective_address);
          break;
        default:
          DebugUnreachableCode();
          return 0;
      }
      return ZeroExtend32(value);
    }
    case OperandSize_32:
      return ReadDWordOperand<mode, constant>(cpu);
    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<OperandMode mode, u32 constant>
void Interpreter::WriteByteOperand(CPU* cpu, u8 value)
{
  switch (mode)
  {
    case OperandMode_Register:
      cpu->m_registers.reg8[constant] = value;
      break;
    case OperandMode_Memory:
      cpu->WriteMemoryByte(cpu->idata.segment, cpu->idata.disp32, value);
      break;

    case OperandMode_ModRM_RM:
    {
      if (cpu->idata.modrm_rm_register)
        cpu->m_registers.reg8[cpu->m_effective_address] = value;
      else
        cpu->WriteMemoryByte(cpu->idata.segment, cpu->m_effective_address, value);
    }
    break;

    case OperandMode_ModRM_Reg:
      cpu->m_registers.reg8[cpu->idata.GetModRM_Reg()] = value;
      break;

    default:
      DebugUnreachableCode();
      break;
  }
}

template<OperandMode mode, u32 constant>
void Interpreter::WriteWordOperand(CPU* cpu, u16 value)
{
  switch (mode)
  {
    case OperandMode_Register:
      cpu->m_registers.reg16[constant] = value;
      break;
    case OperandMode_Memory:
      cpu->WriteMemoryWord(cpu->idata.segment, cpu->idata.disp32, value);
      break;

    case OperandMode_ModRM_RM:
    {
      if (cpu->idata.modrm_rm_register)
        cpu->m_registers.reg16[cpu->m_effective_address] = value;
      else
        cpu->WriteMemoryWord(cpu->idata.segment, cpu->m_effective_address, value);
    }
    break;

    case OperandMode_ModRM_Reg:
      cpu->m_registers.reg16[cpu->idata.GetModRM_Reg()] = value;
      break;

    default:
      DebugUnreachableCode();
      break;
  }
}

template<OperandMode mode, u32 constant>
void Interpreter::WriteDWordOperand(CPU* cpu, u32 value)
{
  switch (mode)
  {
    case OperandMode_Register:
      static_assert(mode != OperandMode_Register || constant < Reg32_Count, "reg32 is in range");
      cpu->m_registers.reg32[constant] = value;
      break;
    case OperandMode_Memory:
      cpu->WriteMemoryDWord(cpu->idata.segment, cpu->idata.disp32, value);
      break;

    case OperandMode_ModRM_RM:
    {
      if (cpu->idata.modrm_rm_register)
        cpu->m_registers.reg32[cpu->m_effective_address] = value;
      else
        cpu->WriteMemoryDWord(cpu->idata.segment, cpu->m_effective_address, value);
    }
    break;

    case OperandMode_ModRM_Reg:
      cpu->m_registers.reg32[cpu->idata.GetModRM_Reg()] = value;
      break;

    default:
      DebugUnreachableCode();
      break;
  }
}

template<OperandMode mode>
void Interpreter::ReadFarAddressOperand(CPU* cpu, OperandSize size, u16* segment_selector,
                                        VirtualMemoryAddress* address)
{
  // Can either be far immediate, or memory
  switch (mode)
  {
    case OperandMode_FarAddress:
    {
      *address = cpu->idata.disp32;
      *segment_selector = cpu->idata.imm16;
    }
    break;

    case OperandMode_Memory:
    {
      if (size == OperandSize_16)
      {
        *address = ZeroExtend32(cpu->ReadMemoryWord(cpu->idata.segment, cpu->idata.disp32));
        *segment_selector = cpu->ReadMemoryWord(cpu->idata.segment, cpu->idata.disp32 + 2);
      }
      else
      {
        *address = cpu->ReadMemoryDWord(cpu->idata.segment, cpu->idata.disp32);
        *segment_selector = cpu->ReadMemoryWord(cpu->idata.segment, cpu->idata.disp32 + 4);
      }
    }
    break;

    case OperandMode_ModRM_RM:
    {
      if (size == OperandSize_16)
      {
        *address = ZeroExtend32(cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_effective_address));
        *segment_selector = cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_effective_address + 2);
      }
      else
      {
        *address = cpu->ReadMemoryDWord(cpu->idata.segment, cpu->m_effective_address);
        *segment_selector = cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_effective_address + 4);
      }
    }
    break;
    default:
      DebugUnreachableCode();
      break;
  }
}

template<OperandMode mode, u32 constant>
u64 Interpreter::ReadQWordOperand(CPU* cpu)
{
  // Only possible for memory operands.
  static_assert(mode == OperandMode_Memory || mode == OperandMode_ModRM_RM);

  u32 address;
  if constexpr (mode == OperandMode_Memory)
    address = cpu->idata.disp32;
  else
    address = cpu->m_effective_address;

  // TODO: Is the masking here correct?
  u32 qword_low = cpu->ReadMemoryDWord(cpu->idata.segment, address);
  u32 qword_high = cpu->ReadMemoryDWord(cpu->idata.segment, (address + 4) & cpu->idata.GetAddressMask());
  return (ZeroExtend64(qword_high) << 32) | ZeroExtend64(qword_low);
}

template<OperandMode mode, u32 constant>
void Interpreter::WriteQWordOperand(CPU* cpu, u64 value)
{
  // Only possible for memory operands.
  static_assert(mode == OperandMode_Memory || mode == OperandMode_ModRM_RM);

  u32 address;
  if constexpr (mode == OperandMode_Memory)
    address = cpu->idata.disp32;
  else
    address = cpu->m_effective_address;

  // TODO: Is the masking here correct?
  cpu->WriteMemoryDWord(cpu->idata.segment, address, Truncate32(value));
  cpu->WriteMemoryDWord(cpu->idata.segment, (address + 4) & cpu->idata.GetAddressMask(), Truncate32(value >> 32));
}

template<JumpCondition condition>
bool Interpreter::TestJumpCondition(CPU* cpu)
{
  switch (condition)
  {
    case JumpCondition_Always:
      return true;

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
      if (cpu->idata.address_size == AddressSize_16)
        return (cpu->m_registers.CX == 0);
      else
        return (cpu->m_registers.ECX == 0);
    }

    default:
      Panic("Unhandled jump condition");
      return false;
  }
}

ALWAYS_INLINE constexpr bool IsSign(u8 value)
{
  return !!(value >> 7);
}
ALWAYS_INLINE constexpr bool IsSign(u16 value)
{
  return !!(value >> 15);
}
ALWAYS_INLINE constexpr bool IsSign(u32 value)
{
  return !!(value >> 31);
}
template<typename T>
ALWAYS_INLINE constexpr bool IsZero(T value)
{
  return (value == 0);
}
template<typename T>
ALWAYS_INLINE constexpr bool IsAdjust(T old_value, T new_value)
{
  return (old_value & 0xF) < (new_value & 0xF);
}

#ifdef Y_COMPILER_MSVC
template<typename T>
ALWAYS_INLINE constexpr bool IsParity(T value)
{
  return static_cast<bool>(~_mm_popcnt_u32(static_cast<u32>(value & 0xFF)) & 1);
}
template<typename T>
ALWAYS_INLINE constexpr u32 ParityFlag(T value)
{
  return (static_cast<u32>(~_mm_popcnt_u32(static_cast<u32>(value & 0xFF))) & u32(1)) << 2;
}
#else
template<typename T>
ALWAYS_INLINE constexpr bool IsParity(T value)
{
  return static_cast<bool>(~Y_popcnt(static_cast<u8>(value & 0xFF)) & 1);
}
template<typename T>
ALWAYS_INLINE constexpr u8 ParityFlag(T value)
{
  return (static_cast<u32>(~Y_popcnt(static_cast<u8>(value & 0xFF))) & u8(1)) << 2;
}
#endif
// template<typename T>
// constexpr bool IsParity(T value)
// {
//   return ConvertToBoolUnchecked(ParityFlag(value));
// }

// GCC seems to be emitting bad code for our FlagAccess class..
#define SET_FLAG(regs, flag, expr)                                                                                     \
  do                                                                                                                   \
  {                                                                                                                    \
    if ((expr))                                                                                                        \
    {                                                                                                                  \
      (regs)->EFLAGS.bits |= (Flag_##flag);                                                                            \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      (regs)->EFLAGS.bits &= ~(Flag_##flag);                                                                           \
    }                                                                                                                  \
  } while (0)
//#define SET_FLAG(regs, flag, expr) (regs)->EFLAGS.flag = (expr)

template<typename ValueType>
ALWAYS_INLINE constexpr u32 SignFlag(ValueType value)
{
  return (static_cast<u32>(value >> (std::numeric_limits<ValueType>::digits - 8)) & Flag_SF);
}

template<typename ValueType>
ALWAYS_INLINE constexpr u32 ZeroFlag(ValueType value)
{
  return BoolToUInt8(value == static_cast<ValueType>(0)) << 6;
}

ALWAYS_INLINE constexpr u32 EFLAGS_ALUAdd8(u32 old_eflags, u32 old_value, u32 add_value, u32 new_value, u8 out_value)
{
  return (old_eflags & ~(Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify CF/OF/AF/SF/ZF/PF
         (static_cast<u32>(new_value >> 8) & Flag_CF) |                                // CF
         ((static_cast<u32>((new_value ^ old_value) & (new_value ^ add_value)) & u32(0x80)) << 4) | // OF
         (static_cast<u32>(old_value ^ add_value ^ new_value) & Flag_AF) |                          // AF
         SignFlag(out_value) |                                                                      // SF
         ZeroFlag(out_value) |                                                                      // ZF
         ParityFlag(out_value);
}

ALWAYS_INLINE constexpr u32 EFLAGS_ALUSub8(u32 old_eflags, u32 old_value, u32 sub_value, u32 new_value, u8 out_value)
{
  return (old_eflags & ~(Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify CF/OF/AF/SF/ZF/PF
         (static_cast<u32>(new_value >> 8) & Flag_CF) |                                // CF
         ((static_cast<u32>((new_value ^ old_value) & (old_value ^ sub_value)) & u32(0x80)) << 4) | // OF
         (static_cast<u32>(old_value ^ sub_value ^ new_value) & Flag_AF) |                          // AF
         SignFlag(out_value) |                                                                      // SF
         ZeroFlag(out_value) |                                                                      // ZF
         ParityFlag(out_value);
}

ALWAYS_INLINE u8 ALUOp_Add8(u32* eflags, u8 lhs, u8 rhs)
{
  const u32 old_value = ZeroExtend32(lhs);
  const u32 add_value = ZeroExtend32(rhs);
  const u32 new_value = old_value + add_value;
  const u8 out_value = Truncate8(new_value);

  *eflags = EFLAGS_ALUAdd8(*eflags, old_value, add_value, new_value, out_value);

  return out_value;
}

ALWAYS_INLINE u8 ALUOp_Adc8(u32* eflags, u8 lhs, u8 rhs)
{
  const u32 old_value = ZeroExtend32(lhs);
  const u32 add_value = ZeroExtend32(rhs);
  const u32 carry_in = *eflags & Flag_CF;
  const u32 new_value = old_value + add_value + carry_in;
  const u8 out_value = Truncate8(new_value);

  *eflags = EFLAGS_ALUAdd8(*eflags, old_value, add_value, new_value, out_value);

  return out_value;
}

ALWAYS_INLINE u8 ALUOp_Sub8(u32* eflags, u8 lhs, u8 rhs)
{
  const u32 old_value = ZeroExtend32(lhs);
  const u32 sub_value = ZeroExtend32(rhs);
  const u32 new_value = old_value - sub_value;
  const u8 out_value = Truncate8(new_value);

  *eflags = EFLAGS_ALUSub8(*eflags, old_value, sub_value, new_value, out_value);

  return out_value;
}

ALWAYS_INLINE u8 ALUOp_Sbb8(u32* eflags, u8 lhs, u8 rhs)
{
  const u32 old_value = ZeroExtend32(lhs);
  const u32 sub_value = ZeroExtend32(rhs);
  const u32 carry_in = *eflags & Flag_CF;
  const u32 new_value = old_value - sub_value - carry_in;
  const u8 out_value = Truncate8(new_value);

  *eflags = EFLAGS_ALUSub8(*eflags, old_value, sub_value, new_value, out_value);

  return out_value;
}

ALWAYS_INLINE constexpr u32 EFLAGS_ALUAdd16(u32 old_eflags, u32 old_value, u32 add_value, u32 new_value, u16 out_value)
{
  return (old_eflags & ~(Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify CF/OF/AF/SF/ZF/PF
         (static_cast<u32>(new_value >> 16) & Flag_CF) |                               // CF
         ((static_cast<u32>((new_value ^ old_value) & (new_value ^ add_value)) & u32(0x8000)) >> 4) | // OF
         (static_cast<u32>(old_value ^ add_value ^ new_value) & Flag_AF) |                            // AF
         SignFlag(out_value) |                                                                        // SF
         ZeroFlag(out_value) |                                                                        // ZF
         ParityFlag(out_value);
}

ALWAYS_INLINE constexpr u32 EFLAGS_ALUSub16(u32 old_eflags, u32 old_value, u32 sub_value, u32 new_value, u16 out_value)
{
  return (old_eflags & ~(Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify CF/OF/AF/SF/ZF/PF
         (static_cast<u32>(new_value >> 16) & Flag_CF) |                               // CF
         ((static_cast<u32>((new_value ^ old_value) & (old_value ^ sub_value)) & u32(0x8000)) >> 4) | // OF
         (static_cast<u32>(old_value ^ sub_value ^ new_value) & Flag_AF) |                            // AF
         SignFlag(out_value) |                                                                        // SF
         ZeroFlag(out_value) |                                                                        // ZF
         ParityFlag(out_value);
}

ALWAYS_INLINE u16 ALUOp_Add16(u32* eflags, u16 lhs, u16 rhs)
{
  const u32 old_value = ZeroExtend32(lhs);
  const u32 add_value = ZeroExtend32(rhs);
  const u32 new_value = old_value + add_value;
  const u16 out_value = Truncate16(new_value);

  *eflags = EFLAGS_ALUAdd16(*eflags, old_value, add_value, new_value, out_value);

  return out_value;
}

ALWAYS_INLINE u16 ALUOp_Adc16(u32* eflags, u16 lhs, u16 rhs)
{
  const u32 old_value = ZeroExtend32(lhs);
  const u32 add_value = ZeroExtend32(rhs);
  const u32 carry_in = *eflags & Flag_CF;
  const u32 new_value = old_value + add_value + carry_in;
  const u16 out_value = Truncate16(new_value);

  *eflags = EFLAGS_ALUAdd16(*eflags, old_value, add_value, new_value, out_value);

  return out_value;
}

ALWAYS_INLINE u16 ALUOp_Sub16(u32* eflags, u16 lhs, u16 rhs)
{
  const u32 old_value = ZeroExtend32(lhs);
  const u32 sub_value = ZeroExtend32(rhs);
  const u32 new_value = old_value - sub_value;
  const u16 out_value = Truncate16(new_value);

  *eflags = EFLAGS_ALUSub16(*eflags, old_value, sub_value, new_value, out_value);

  return out_value;
}

ALWAYS_INLINE u16 ALUOp_Sbb16(u32* eflags, u16 lhs, u16 rhs)
{
  const u32 old_value = ZeroExtend32(lhs);
  const u32 sub_value = ZeroExtend32(rhs);
  const u32 carry_in = *eflags & Flag_CF;
  const u32 new_value = old_value - sub_value - carry_in;
  const u16 out_value = Truncate16(new_value);

  *eflags = EFLAGS_ALUSub16(*eflags, old_value, sub_value, new_value, out_value);

  return out_value;
}

ALWAYS_INLINE constexpr u32 EFLAGS_ALUAdd32(u32 old_eflags, u32 old_value, u32 add_value, u64 new_value, u32 out_value)
{
  return (old_eflags & ~(Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify CF/OF/AF/SF/ZF/PF
         (static_cast<u32>(new_value >> 32) & Flag_CF) |                               // CF
         ((static_cast<u32>((Truncate32(new_value) ^ old_value) & (Truncate32(new_value) ^ add_value)) &
           u32(0x80000000)) >>
          20) |                                                                        // OF
         (static_cast<u32>(old_value ^ add_value ^ Truncate32(new_value)) & Flag_AF) | // AF
         SignFlag(out_value) |                                                         // SF
         ZeroFlag(out_value) |                                                         // ZF
         ParityFlag(out_value);
}

ALWAYS_INLINE constexpr u32 EFLAGS_ALUSub32(u32 old_eflags, u32 old_value, u32 sub_value, u64 new_value, u32 out_value)
{
  return (old_eflags & ~(Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify CF/OF/AF/SF/ZF/PF
         (static_cast<u32>(new_value >> 32) & Flag_CF) |                               // CF
         ((static_cast<u32>((Truncate32(new_value) ^ old_value) & (old_value ^ sub_value)) & u32(0x80000000)) >>
          20) |                                                                        // OF
         (static_cast<u32>(old_value ^ sub_value ^ Truncate32(new_value)) & Flag_AF) | // AF
         SignFlag(out_value) |                                                         // SF
         ZeroFlag(out_value) |                                                         // ZF
         ParityFlag(out_value);
}

ALWAYS_INLINE u32 ALUOp_Add32(u32* eflags, u32 lhs, u32 rhs)
{
  const u32 old_value = lhs;
  const u32 add_value = rhs;
  const u64 new_value = ZeroExtend64(old_value) + ZeroExtend64(add_value);
  const u32 out_value = Truncate32(new_value);

  *eflags = EFLAGS_ALUAdd32(*eflags, old_value, add_value, new_value, out_value);

  return out_value;
}

ALWAYS_INLINE u32 ALUOp_Adc32(u32* eflags, u32 lhs, u32 rhs)
{
  const u32 old_value = lhs;
  const u32 add_value = rhs;
  const u32 carry_in = *eflags & Flag_CF;
  const u64 new_value = ZeroExtend64(old_value) + ZeroExtend64(add_value) + ZeroExtend64(carry_in);
  const u32 out_value = Truncate32(new_value);

  *eflags = EFLAGS_ALUAdd32(*eflags, old_value, add_value, new_value, out_value);

  return out_value;
}

ALWAYS_INLINE u32 ALUOp_Sub32(u32* eflags, u32 lhs, u32 rhs)
{
  const u32 old_value = lhs;
  const u32 sub_value = rhs;
  const u64 new_value = ZeroExtend64(old_value) - ZeroExtend64(sub_value);
  const u32 out_value = Truncate32(new_value);

  *eflags = EFLAGS_ALUSub32(*eflags, old_value, sub_value, new_value, out_value);

  return out_value;
}

ALWAYS_INLINE u32 ALUOp_Sbb32(u32* eflags, u32 lhs, u32 rhs)
{
  const u32 old_value = lhs;
  const u32 sub_value = rhs;
  const u32 carry_in = *eflags & Flag_CF;
  const u64 new_value = ZeroExtend64(old_value) - ZeroExtend64(sub_value) - ZeroExtend64(carry_in);
  const u32 out_value = Truncate32(new_value);

  *eflags = EFLAGS_ALUSub32(*eflags, old_value, sub_value, new_value, out_value);

  return out_value;
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_ADD(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    const u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    const u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u8 new_value = ALUOp_Add8(&eflags, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    const u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    const u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u16 new_value = ALUOp_Add16(&eflags, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    const u32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u32 new_value = ALUOp_Add32(&eflags, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }

  if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
    cpu->AddCycles(CYCLES_ALU_REG_IMM);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());
  else if constexpr (src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_ADC(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    const u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    const u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u8 new_value = ALUOp_Adc8(&eflags, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    const u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    const u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u16 new_value = ALUOp_Adc16(&eflags, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    const u32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u32 new_value = ALUOp_Adc32(&eflags, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }

  if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
    cpu->AddCycles(CYCLES_ALU_REG_IMM);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());
  else if constexpr (src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_SUB(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    const u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    const u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u8 new_value = ALUOp_Sub8(&eflags, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    const u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    const u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u16 new_value = ALUOp_Sub16(&eflags, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    const u32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u32 new_value = ALUOp_Sub32(&eflags, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }

  if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
    cpu->AddCycles(CYCLES_ALU_REG_IMM);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());
  else if constexpr (src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_SBB(CPU* cpu)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u8 new_value = ALUOp_Sbb8(&eflags, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u16 new_value = ALUOp_Sbb16(&eflags, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    u32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    u32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u32 new_value = ALUOp_Sbb32(&eflags, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }

  if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
    cpu->AddCycles(CYCLES_ALU_REG_IMM);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());
  else if constexpr (src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_CMP(CPU* cpu)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  // Implemented as subtract but discarding the result
  if (actual_size == OperandSize_8)
  {
    const u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    const u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    ALUOp_Sub8(&cpu->m_registers.EFLAGS.bits, lhs, rhs);
  }
  else if (actual_size == OperandSize_16)
  {
    const u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    const u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    ALUOp_Sub16(&cpu->m_registers.EFLAGS.bits, lhs, rhs);
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    const u32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    ALUOp_Sub32(&cpu->m_registers.EFLAGS.bits, lhs, rhs);
  }

  if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
    cpu->AddCycles(CYCLES_CMP_REG_IMM);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_CMP_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());
  else if constexpr (src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_CMP_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
}

template<typename ValueType>
ALWAYS_INLINE constexpr u32 EFLAGS_BitwiseOps(u32 old_eflags, ValueType new_value)
{
  // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF flag
  // is undefined.
  return (old_eflags & ~(Flag_OF | Flag_CF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Clear OF/CF/AF
         SignFlag(new_value) |                                                         // SF
         ParityFlag(new_value) |                                                       // PF
         ZeroFlag(new_value);                                                          // ZF
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_AND(CPU* cpu)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    u8 new_value = lhs & rhs;
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    u16 new_value = lhs & rhs;
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    u32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 new_value = lhs & rhs;
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
    cpu->AddCycles(CYCLES_ALU_REG_IMM);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());
  else if constexpr (src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_OR(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    u8 new_value = lhs | rhs;
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    u16 new_value = lhs | rhs;
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    u32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 new_value = lhs | rhs;
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
    cpu->AddCycles(CYCLES_ALU_REG_IMM);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());
  else if constexpr (src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_XOR(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    u8 new_value = lhs ^ rhs;
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    u16 new_value = lhs ^ rhs;
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    u32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 new_value = lhs ^ rhs;
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
    cpu->AddCycles(CYCLES_ALU_REG_IMM);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());
  else if constexpr (src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_ALU_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_TEST(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    u8 new_value = lhs & rhs;

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    u16 rhs = ReadWordOperand<src_mode, src_constant>(cpu);
    u16 new_value = lhs & rhs;

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    u32 rhs = ReadDWordOperand<src_mode, src_constant>(cpu);
    u32 new_value = lhs & rhs;

    cpu->m_registers.EFLAGS.bits = EFLAGS_BitwiseOps(cpu->m_registers.EFLAGS.bits, new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if constexpr (src_mode == OperandMode_Immediate)
    cpu->AddCyclesRM(CYCLES_TEST_RM_MEM_REG, (dst_mode == OperandMode_ModRM_RM) ? cpu->idata.ModRM_RM_IsReg() : false);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_TEST_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());
  else if constexpr (src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_TEST_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_MOV(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  static_assert(dst_size == src_size, "dst_size == src_size");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  // Invalid with the LOCK prefix.
  // TODO: a more general solution.
  if (cpu->idata.has_lock)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
    cpu->AddCycles(CYCLES_MOV_REG_IMM);
  else if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Memory)
    cpu->AddCycles(CYCLES_MOV_REG_MEM);
  else if constexpr (dst_mode == OperandMode_Memory && src_mode == OperandMode_Register)
    cpu->AddCycles(CYCLES_MOV_RM_MEM_REG);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_MOV_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());
  else if constexpr (src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_MOV_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  if (actual_size == OperandSize_8)
  {
    u8 value = ReadByteOperand<src_mode, src_constant>(cpu);
    WriteByteOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_16)
  {
    u16 value = ReadWordOperand<src_mode, src_constant>(cpu);
    WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 value = ReadDWordOperand<src_mode, src_constant>(cpu);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_MOVZX(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if constexpr (dst_mode == OperandMode_ModRM_Reg && src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_MOVZX_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  if (actual_size == OperandSize_16)
  {
    u16 value = ReadZeroExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 value = ReadZeroExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_MOVSX(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if constexpr (dst_mode == OperandMode_ModRM_Reg && src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_MOVSX_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  if (actual_size == OperandSize_16)
  {
    u16 value = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 value = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_MOV_Sreg(CPU* cpu)
{
  static_assert(dst_size == OperandSize_16 && src_size == OperandSize_16, "Segment registers are 16-bits");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const u8 segreg = cpu->idata.GetModRM_Reg();

  // TODO: Loading the SS register with a MOV instruction inhibits all interrupts until after the execution
  // of the next instruction. This operation allows a stack pointer to be loaded into the ESP register with the next
  // instruction (MOV ESP, stack-pointer value) before an interrupt occurs1. Be aware that the LSS instruction offers a
  // more efficient method of loading the SS and ESP registers.
  if constexpr (dst_mode == OperandMode_ModRM_SegmentReg)
  {
    // Loading segment register.
    cpu->AddCyclesPMode(cpu->idata.ModRM_RM_IsReg() ? CYCLES_MOV_SREG_RM_REG : CYCLES_MOV_SREG_RM_MEM);

    // The MOV instruction cannot be used to load the CS register. Attempting to do so results in an invalid opcode
    // exception (#UD).
    if (segreg >= Segment_Count || segreg == Segment_CS)
    {
      cpu->RaiseException(Interrupt_InvalidOpcode);
      return;
    }

    u16 value = ReadWordOperand<src_mode, src_constant>(cpu);
    cpu->LoadSegmentRegister(static_cast<CPU_X86::Segment>(segreg), value);
  }
  else
  {
    // Storing segment register.
    cpu->AddCyclesRM(CYCLES_MOV_RM_MEM_SREG, cpu->idata.ModRM_RM_IsReg());
    if (segreg >= Segment_Count)
    {
      cpu->RaiseException(Interrupt_InvalidOpcode);
      return;
    }

    // These are zero-extended when the operand size is 32-bit and the destination is a register.
    u16 value = cpu->m_registers.segment_selectors[segreg];
    if constexpr (dst_mode == OperandMode_ModRM_RM)
    {
      if (cpu->idata.operand_size == OperandSize_32 && cpu->idata.modrm_rm_register)
        WriteDWordOperand<dst_mode, dst_constant>(cpu, ZeroExtend32(value));
      else
        WriteWordOperand<dst_mode, dst_constant>(cpu, value);
    }
    else
    {
      WriteWordOperand<dst_mode, dst_constant>(cpu, value);
    }
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_XCHG(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  static_assert(dst_size == src_size, "source and destination operands are of same size");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  cpu->AddCyclesRM(CYCLES_XCHG_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());

  // In memory version, memory is op0, register is op1. Memory must be written first.
  if (actual_size == OperandSize_8)
  {
    u8 value0 = ReadByteOperand<dst_mode, dst_constant>(cpu);
    u8 value1 = ReadByteOperand<src_mode, src_constant>(cpu);

    WriteByteOperand<dst_mode, dst_constant>(cpu, value1);
    WriteByteOperand<src_mode, src_constant>(cpu, value0);
  }
  else if (actual_size == OperandSize_16)
  {
    u16 value0 = ReadWordOperand<dst_mode, dst_constant>(cpu);
    u16 value1 = ReadWordOperand<src_mode, src_constant>(cpu);

    WriteWordOperand<dst_mode, dst_constant>(cpu, value1);
    WriteWordOperand<src_mode, src_constant>(cpu, value0);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 value0 = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    u32 value1 = ReadDWordOperand<src_mode, src_constant>(cpu);

    WriteDWordOperand<dst_mode, dst_constant>(cpu, value1);
    WriteDWordOperand<src_mode, src_constant>(cpu, value0);
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
         u32 count_constant>
void Interpreter::Execute_Operation_SHL(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);
  cpu->AddCyclesRM(CYCLES_ALU_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());

  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (actual_size == OperandSize_8)
  {
    const u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    const u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    const u16 shifted_value = ZeroExtend16(value) << shift_amount;
    const u8 new_value = Truncate8(shifted_value);
    const u32 new_eflags =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF | Flag_AF)) |
      (ZeroExtend32(shifted_value >> 8) & Flag_CF) | // CF, (shifted_value & 0x100) != 0
      (((ZeroExtend32(shifted_value) << 3) ^ (ZeroExtend32(shifted_value) << 4)) & Flag_OF &
       (BoolToUInt32(shift_amount == 1)
        << 11)) |           // OF, (shift_amount == 1 && (((shifted_value >> 7) & 1) ^ ((shifted_value >> 8) & 1)) != 0)
      SignFlag(new_value) | // SF
      ZeroFlag(new_value) | // ZF
      ParityFlag(new_value); // PF

    WriteByteOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = new_eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    const u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    const u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    const u32 shifted_value = ZeroExtend32(value) << shift_amount;
    const u16 new_value = Truncate16(shifted_value);
    const u32 new_eflags =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF | Flag_AF)) |
      ((shifted_value >> 16) & Flag_CF) | // CF, (shifted_value & 0x10000) != 0
      (((shifted_value >> 4) ^ (shifted_value >> 5)) & Flag_OF &
       (BoolToUInt32(shift_amount == 1)
        << 11)) | // OF, (shift_amount == 1 && (((shifted_value >> 15) & 1) ^ ((shifted_value >> 16) & 1)) != 0)
      SignFlag(new_value) |  // SF
      ZeroFlag(new_value) |  // ZF
      ParityFlag(new_value); // PF

    WriteWordOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = new_eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    const u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    const u64 shifted_value = ZeroExtend64(value) << shift_amount;
    const u32 new_value = Truncate32(shifted_value);
    const u32 new_eflags =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF | Flag_AF)) |
      (Truncate32(shifted_value >> 32) & Flag_CF) | // CF, (shifted_value & 0x100000000) != 0
      (Truncate32((shifted_value >> 20) ^ (shifted_value >> 21)) & Flag_OF &
       (BoolToUInt32(shift_amount == 1)
        << 11)) | // OF, (shift_amount == 1 && (((shifted_value >> 31) & 1) ^ ((shifted_value >> 32) & 1)) != 0)
      SignFlag(new_value) |  // SF
      ZeroFlag(new_value) |  // ZF
      ParityFlag(new_value); // PF

    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = new_eflags;
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
         u32 count_constant>
void Interpreter::Execute_Operation_SHR(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);
  cpu->AddCyclesRM(CYCLES_ALU_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());

  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (actual_size == OperandSize_8)
  {
    const u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    const u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    const u8 new_value = value >> shift_amount;
    const u32 new_eflags = (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF)) |
                           (ZeroExtend32(value >> (shift_amount - 1)) & Flag_CF) | // CF
                           (ZeroExtend32(value << 4) & Flag_OF &
                            (BoolToUInt32(shift_amount == 1) << 11)) | // OF, shift_amount == 1 && (value & 0x80)
                           SignFlag(new_value) |                       // SF
                           ZeroFlag(new_value) |                       // ZF
                           ParityFlag(new_value);                      // PF

    WriteByteOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = new_eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    const u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    const u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    const u16 new_value = value >> shift_amount;
    const u32 new_eflags = (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF)) |
                           (ZeroExtend32(value >> (shift_amount - 1)) & Flag_CF) | // CF
                           (ZeroExtend32(value >> 4) & Flag_OF &
                            (BoolToUInt32(shift_amount == 1) << 11)) | // OF, shift_amount == 1 && (value & 0x8000)
                           SignFlag(new_value) |                       // SF
                           ZeroFlag(new_value) |                       // ZF
                           ParityFlag(new_value);                      // PF

    WriteWordOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = new_eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    const u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    const u32 new_value = value >> shift_amount;
    const u32 new_eflags = (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF)) |
                           ((value >> (shift_amount - 1)) & Flag_CF) | // CF
                           ((value >> 20) & Flag_OF &
                            (BoolToUInt32(shift_amount == 1) << 11)) | // OF, shift_amount == 1 && (value & 0x80000000)
                           SignFlag(new_value) |                       // SF
                           ZeroFlag(new_value) |                       // ZF
                           ParityFlag(new_value);                      // PF

    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = new_eflags;
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
         u32 count_constant>
void Interpreter::Execute_Operation_SAR(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // Cycles have to come first due to the early-out below.
  cpu->AddCyclesRM(CYCLES_ALU_RM_MEM_REG, cpu->idata.ModRM_RM_IsReg());

  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (actual_size == OperandSize_8)
  {
    const u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    const u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    const u8 new_value = u8(s8(value) >> shift_amount);
    const u32 new_eflags = (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF)) |
                           (ZeroExtend32(s8(value) >> (shift_amount - 1)) & Flag_CF) | // CF
                           SignFlag(new_value) |                                       // SF
                           ZeroFlag(new_value) |                                       // ZF
                           ParityFlag(new_value);                                      // PF

    WriteByteOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = new_eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    const u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    const u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    const u16 new_value = u16(s16(value) >> shift_amount);
    const u32 new_eflags = (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF)) |
                           (ZeroExtend32(s16(value) >> (shift_amount - 1)) & Flag_CF) | // CF
                           SignFlag(new_value) |                                        // SF
                           ZeroFlag(new_value) |                                        // ZF
                           ParityFlag(new_value);                                       // PF

    WriteWordOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = new_eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    const u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    const u32 new_value = u32(s32(value) >> shift_amount);
    const u32 new_eflags = (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF)) |
                           (static_cast<u32>(s32(value) >> (shift_amount - 1)) & Flag_CF) | // CF
                           SignFlag(new_value) |                                            // SF
                           ZeroFlag(new_value) |                                            // ZF
                           ParityFlag(new_value);                                           // PF

    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = new_eflags;
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
         u32 count_constant>
void Interpreter::Execute_Operation_RCL(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // Cycles have to come first due to the early-out below.
  cpu->AddCyclesRM(CYCLES_RCL_RM_MEM, cpu->idata.ModRM_RM_IsReg());

  // The processor restricts the count to a number between 0 and 31 by masking all the bits in the count operand except
  // the 5 least-significant bits.
  if (actual_size == OperandSize_8)
  {
    u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    u8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    u8 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (u8 i = 0; i < rotate_count; i++)
    {
      u8 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 7);
    }
    WriteByteOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 7) ^ carry) != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    u8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    u16 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (u8 i = 0; i < rotate_count; i++)
    {
      u16 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 15);
    }
    WriteWordOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 15) ^ carry) != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    u32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    u8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    u32 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (u8 i = 0; i < rotate_count; i++)
    {
      u32 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 31);
    }
    WriteDWordOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 31) ^ carry) != 0));
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
         u32 count_constant>
void Interpreter::Execute_Operation_RCR(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // Cycles have to come first due to the early-out below.
  cpu->AddCyclesRM(CYCLES_RCL_RM_MEM, cpu->idata.ModRM_RM_IsReg());

  // The processor restricts the count to a number between 0 and 31 by masking all the bits in the count operand except
  // the 5 least-significant bits.
  if (actual_size == OperandSize_8)
  {
    u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    u8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    u8 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (u8 i = 0; i < rotate_count; i++)
    {
      u8 save_value = value;
      value = (save_value >> 1) | (carry << 7);
      carry = (save_value & 1);
    }
    WriteByteOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 7) ^ ((value >> 6) & 1)) != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    u8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    u16 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (u8 i = 0; i < rotate_count; i++)
    {
      u16 save_value = value;
      value = (save_value >> 1) | (carry << 15);
      carry = (save_value & 1);
    }
    WriteWordOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 15) ^ ((value >> 14) & 1)) != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    u32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    u8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    u32 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (u8 i = 0; i < rotate_count; i++)
    {
      u32 save_value = value;
      value = (save_value >> 1) | (carry << 31);
      carry = (save_value & 1);
    }
    WriteDWordOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 31) ^ ((value >> 30) & 1)) != 0));
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
         u32 count_constant>
void Interpreter::Execute_Operation_ROL(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // Cycles have to come first due to the early-out below.
  cpu->AddCyclesRM(CYCLES_ROL_RM_MEM, cpu->idata.ModRM_RM_IsReg());

  // Hopefully this will compile down to a native ROL instruction
  if (actual_size == OperandSize_8)
  {
    const u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    const u8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    u8 new_value = value;
    if ((count & 0x7) != 0)
    {
      u8 masked_count = count & 0x7;
      new_value = (value << masked_count) | (value >> (8 - masked_count));
      WriteByteOperand<val_mode, val_constant>(cpu, new_value);
    }

    cpu->m_registers.EFLAGS.bits =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF)) | (ZeroExtend32(new_value) & Flag_CF) | // CF
      (((ZeroExtend32(new_value) << 11) ^ (ZeroExtend32(new_value) << 4)) & Flag_OF);               // OF: bit 0 ^ bit 7
  }
  else if (actual_size == OperandSize_16)
  {
    const u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    const u8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    u16 new_value = value;
    if ((count & 0xf) != 0)
    {
      u8 masked_count = count & 0xf;
      new_value = (value << masked_count) | (value >> (16 - masked_count));
      WriteWordOperand<val_mode, val_constant>(cpu, new_value);
    }

    cpu->m_registers.EFLAGS.bits =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF)) | (ZeroExtend32(new_value) & Flag_CF) | // CF
      (((ZeroExtend32(new_value) << 11) ^ (ZeroExtend32(new_value) >> 4)) & Flag_OF); // OF: bit 0 ^ bit 15
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    const u8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    u32 new_value = value;
    u8 masked_count = count & 0x1f;
    if (masked_count != 0)
    {
      new_value = (value << masked_count) | (value >> (32 - masked_count));
      WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
    }

    cpu->m_registers.EFLAGS.bits = (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF)) | (new_value & Flag_CF) | // CF
                                   (((new_value << 11) ^ (new_value >> 20)) & Flag_OF); // OF: bit 0 ^ bit 31
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
         u32 count_constant>
void Interpreter::Execute_Operation_ROR(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // Cycles have to come first due to the early-out below.
  cpu->AddCyclesRM(CYCLES_ROL_RM_MEM, cpu->idata.ModRM_RM_IsReg());

  // Hopefully this will compile down to a native ROR instruction
  if (actual_size == OperandSize_8)
  {
    const u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    const u8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    u8 new_value = value;
    u8 masked_count = count & 0x7;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (8 - masked_count));
      WriteByteOperand<val_mode, val_constant>(cpu, new_value);
    }

    cpu->m_registers.EFLAGS.bits =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF)) | (ZeroExtend32(new_value >> 7) & Flag_CF) | // CF
      (((ZeroExtend32(new_value) << 5) ^ (ZeroExtend32(new_value) << 4)) & Flag_OF); // OF: bit 6 ^ bit 7
  }
  else if (actual_size == OperandSize_16)
  {
    const u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    const u8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    u16 new_value = value;
    u8 masked_count = count & 0xf;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (16 - masked_count));
      WriteWordOperand<val_mode, val_constant>(cpu, new_value);
    }

    cpu->m_registers.EFLAGS.bits =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF)) | (ZeroExtend32(new_value >> 15) & Flag_CF) | // CF
      (((ZeroExtend32(new_value) >> 3) ^ (ZeroExtend32(new_value) >> 4)) & Flag_OF); // OF: bit 14 ^ bit 15
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    const u8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    u32 new_value = value;
    u8 masked_count = count & 0x1f;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (32 - masked_count));
      WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
    }

    cpu->m_registers.EFLAGS.bits = (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF)) |
                                   ((new_value >> 31) & Flag_CF) |                      // CF
                                   (((new_value >> 19) ^ (new_value >> 20)) & Flag_OF); // OF: bit 30 ^ bit 31
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_IN(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if constexpr (src_mode == OperandMode_Immediate)
    cpu->AddCyclesPMode(CYCLES_IN_IMM);
  else if constexpr (src_mode == OperandMode_Register)
    cpu->AddCyclesPMode(CYCLES_IN_EDX);
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  const u16 port_number = ReadZeroExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
  if (actual_size == OperandSize_8)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(u8), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    cpu->CommitPendingCycles();

    const u8 value = cpu->m_bus->ReadIOPortByte(port_number);
    WriteByteOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_16)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(u16), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    cpu->CommitPendingCycles();

    const u16 value = cpu->m_bus->ReadIOPortWord(port_number);
    WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_32)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(u32), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    cpu->CommitPendingCycles();

    const u32 value = cpu->m_bus->ReadIOPortDWord(port_number);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_OUT(CPU* cpu)
{
  const OperandSize actual_size = (src_size == OperandSize_Count) ? cpu->idata.operand_size : src_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if constexpr (dst_mode == OperandMode_Immediate)
    cpu->AddCyclesPMode(CYCLES_OUT_IMM);
  else if constexpr (dst_mode == OperandMode_Register)
    cpu->AddCyclesPMode(CYCLES_OUT_EDX);
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  const u16 port_number = ReadZeroExtendedWordOperand<dst_size, dst_mode, dst_constant>(cpu);
  if (actual_size == OperandSize_8)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(u8), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    cpu->CommitPendingCycles();

    u8 value = ReadByteOperand<src_mode, src_constant>(cpu);
    cpu->m_bus->WriteIOPortByte(port_number, value);
  }
  else if (actual_size == OperandSize_16)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(u16), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    cpu->CommitPendingCycles();

    u16 value = ReadWordOperand<src_mode, src_constant>(cpu);
    cpu->m_bus->WriteIOPortWord(port_number, value);
  }
  else if (actual_size == OperandSize_32)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(u32), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    cpu->CommitPendingCycles();

    u32 value = ReadDWordOperand<src_mode, src_constant>(cpu);
    cpu->m_bus->WriteIOPortDWord(port_number, value);
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
void Interpreter::Execute_Operation_INC(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  if constexpr (val_mode == OperandMode_Register)
    cpu->AddCycles(CYCLES_INC_RM_REG);
  else if constexpr (val_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_INC_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<val_mode>::value, "unknown mode");

  if (actual_size == OperandSize_8)
  {
    const u32 old_value = ZeroExtend32(ReadByteOperand<val_mode, val_constant>(cpu));
    const u32 new_value = old_value + u32(1);
    const u8 out_value = Truncate8(new_value);
    const u32 eflags =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify OF/AF/SF/ZF/PF
      ((static_cast<u32>((new_value ^ old_value) & new_value) & u32(0x80)) << 4) |          // OF
      (static_cast<u32>(old_value ^ u32(1) ^ new_value) & Flag_AF) |                        // AF
      SignFlag(out_value) |                                                                 // SF
      ZeroFlag(out_value) |                                                                 // ZF
      ParityFlag(out_value);
    WriteByteOperand<val_mode, val_constant>(cpu, out_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    const u32 old_value = ZeroExtend32(ReadWordOperand<val_mode, val_constant>(cpu));
    const u32 new_value = old_value + u32(1);
    const u16 out_value = Truncate16(new_value);
    const u32 eflags =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify OF/AF/SF/ZF/PF
      ((static_cast<u32>((new_value ^ old_value) & new_value) & u32(0x8000)) >> 4) |        // OF
      (static_cast<u32>(old_value ^ new_value) & Flag_AF) |                                 // AF
      SignFlag(out_value) |                                                                 // SF
      ZeroFlag(out_value) |                                                                 // ZF
      ParityFlag(out_value);
    WriteWordOperand<val_mode, val_constant>(cpu, out_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 old_value = ReadDWordOperand<val_mode, val_constant>(cpu);
    const u32 new_value = old_value + u32(1);
    const u32 eflags =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify OF/AF/SF/ZF/PF
      ((static_cast<u32>((new_value ^ old_value) & new_value) & u32(0x80000000)) >> 20) |   // OF
      (static_cast<u32>(old_value ^ new_value) & Flag_AF) |                                 // AF
      SignFlag(new_value) |                                                                 // SF
      ZeroFlag(new_value) |                                                                 // ZF
      ParityFlag(new_value);
    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
void Interpreter::Execute_Operation_DEC(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  if constexpr (val_mode == OperandMode_Register)
    cpu->AddCycles(CYCLES_INC_RM_REG);
  else if constexpr (val_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_INC_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<val_mode>::value, "unknown mode");

  if (actual_size == OperandSize_8)
  {
    const u32 old_value = ZeroExtend32(ReadByteOperand<val_mode, val_constant>(cpu));
    const u32 new_value = old_value - u32(1);
    const u8 out_value = Truncate8(new_value);
    const u32 eflags =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify OF/AF/SF/ZF/PF
      ((static_cast<u32>((new_value ^ old_value) & old_value) & u32(0x80)) << 4) |          // OF
      (static_cast<u32>(old_value ^ new_value) & Flag_AF) |                                 // AF
      SignFlag(out_value) |                                                                 // SF
      ZeroFlag(out_value) |                                                                 // ZF
      ParityFlag(out_value);

    WriteByteOperand<val_mode, val_constant>(cpu, out_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    const u32 old_value = ZeroExtend32(ReadWordOperand<val_mode, val_constant>(cpu));
    const u32 new_value = old_value - u32(1);
    const u16 out_value = Truncate16(new_value);
    const u32 eflags =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify OF/AF/SF/ZF/PF
      ((static_cast<u32>((new_value ^ old_value) & old_value) & u32(0x8000)) >> 4) |        // OF
      (static_cast<u32>(old_value ^ new_value) & Flag_AF) |                                 // AF
      SignFlag(out_value) |                                                                 // SF
      ZeroFlag(out_value) |                                                                 // ZF
      ParityFlag(out_value);

    WriteWordOperand<val_mode, val_constant>(cpu, out_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 old_value = ReadDWordOperand<val_mode, val_constant>(cpu);
    const u32 new_value = old_value - u32(1);
    const u32 eflags =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify OF/AF/SF/ZF/PF
      ((static_cast<u32>((new_value ^ old_value) & old_value) & u32(0x80000000)) >> 20) |   // OF
      (static_cast<u32>(old_value ^ new_value) & Flag_AF) |                                 // AF
      SignFlag(new_value) |                                                                 // SF
      ZeroFlag(new_value) |                                                                 // ZF
      ParityFlag(new_value);

    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
void Interpreter::Execute_Operation_NOT(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  if constexpr (val_mode == OperandMode_Register)
    cpu->AddCycles(CYCLES_NEG_RM_REG);
  else if constexpr (val_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_NEG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<val_mode>::value, "unknown mode");

  if (actual_size == OperandSize_8)
  {
    const u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    const u8 new_value = ~value;
    WriteByteOperand<val_mode, val_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    const u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    const u16 new_value = ~value;
    WriteWordOperand<val_mode, val_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    const u32 new_value = ~value;
    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
void Interpreter::Execute_Operation_NEG(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  if constexpr (val_mode == OperandMode_Register)
    cpu->AddCycles(CYCLES_NEG_RM_REG);
  else if constexpr (val_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_NEG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<val_mode>::value, "unknown mode");

  if (actual_size == OperandSize_8)
  {
    const u32 sub_value = ZeroExtend32(ReadByteOperand<val_mode, val_constant>(cpu));
    const u32 new_value = u32(0) - sub_value;
    const u8 out_value = Truncate8(new_value);
    const u32 eflags = (cpu->m_registers.EFLAGS.bits &
                        ~(Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify CF/OF/AF/SF/ZF/PF
                       BoolToUInt32(new_value != 0) |                                   // CF
                       ((static_cast<u32>(new_value & sub_value) & u32(0x80)) << 4) |   // OF
                       (static_cast<u32>(sub_value ^ new_value) & Flag_AF) |            // AF
                       SignFlag(out_value) |                                            // SF
                       ZeroFlag(out_value) |                                            // ZF
                       ParityFlag(out_value);

    WriteByteOperand<val_mode, val_constant>(cpu, out_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    const u32 sub_value = ZeroExtend32(ReadWordOperand<val_mode, val_constant>(cpu));
    const u32 new_value = u32(0) - sub_value;
    const u16 out_value = Truncate16(new_value);
    const u32 eflags = (cpu->m_registers.EFLAGS.bits &
                        ~(Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify CF/OF/AF/SF/ZF/PF
                       BoolToUInt32(new_value != 0) |                                   // CF
                       ((static_cast<u32>(new_value & sub_value) & u32(0x8000)) >> 4) | // OF
                       (static_cast<u32>(sub_value ^ new_value) & Flag_AF) |            // AF
                       SignFlag(out_value) |                                            // SF
                       ZeroFlag(out_value) |                                            // ZF
                       ParityFlag(out_value);

    WriteWordOperand<val_mode, val_constant>(cpu, out_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 sub_value = ReadDWordOperand<val_mode, val_constant>(cpu);
    const u32 new_value = u32(0) - sub_value;
    const u32 eflags = (cpu->m_registers.EFLAGS.bits &
                        ~(Flag_CF | Flag_OF | Flag_AF | Flag_SF | Flag_ZF | Flag_PF)) |      // Modify CF/OF/AF/SF/ZF/PF
                       BoolToUInt32(new_value != 0) |                                        // CF
                       ((static_cast<u32>(new_value & sub_value) & u32(0x80000000)) >> 20) | // OF
                       (static_cast<u32>(sub_value ^ new_value) & Flag_AF) |                 // AF
                       SignFlag(new_value) |                                                 // SF
                       ZeroFlag(new_value) |                                                 // ZF
                       ParityFlag(new_value);                                                // PF

    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
    cpu->m_registers.EFLAGS.bits = eflags;
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
void Interpreter::Execute_Operation_MUL(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  // The OF and CF flags are set to 0 if the upper half of the result is 0;
  // otherwise, they are set to 1. The SF, ZF, AF, and PF flags are undefined.
  if (actual_size == OperandSize_8)
  {
    const u16 lhs = ZeroExtend16(cpu->m_registers.AL);
    const u16 rhs = ZeroExtend16(ReadByteOperand<val_mode, val_constant>(cpu));
    const u16 result = lhs * rhs;
    cpu->m_registers.AX = result;

    const bool of_cf = (cpu->m_registers.AH != 0);
    const u8 al = cpu->m_registers.AL;

    cpu->m_registers.EFLAGS.bits =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify CF/OF/SF/ZF/PF
      BoolToUInt32(of_cf) |                                                                 // CF
      (BoolToUInt32(of_cf) << 11) |                                                         // OF
      SignFlag(al) |                                                                        // SF
      ZeroFlag(al) |                                                                        // ZF
      ParityFlag(al);                                                                       // PF

    cpu->AddCyclesRM(CYCLES_MUL_8_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  }
  else if (actual_size == OperandSize_16)
  {
    const u32 lhs = ZeroExtend32(cpu->m_registers.AX);
    const u32 rhs = ZeroExtend32(ReadWordOperand<val_mode, val_constant>(cpu));
    const u32 result = lhs * rhs;
    cpu->m_registers.AX = u16(result & 0xFFFF);
    cpu->m_registers.DX = u16(result >> 16);

    const bool of_cf = (cpu->m_registers.DX != 0);
    const u16 ax = cpu->m_registers.AX;

    cpu->m_registers.EFLAGS.bits =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify CF/OF/SF/ZF/PF
      BoolToUInt32(of_cf) |                                                                 // CF
      (BoolToUInt32(of_cf) << 11) |                                                         // OF
      SignFlag(ax) |                                                                        // SF
      ZeroFlag(ax) |                                                                        // ZF
      ParityFlag(ax);                                                                       // PF

    cpu->AddCyclesRM(CYCLES_MUL_16_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  }
  else if (actual_size == OperandSize_32)
  {
    const u64 lhs = ZeroExtend64(cpu->m_registers.EAX);
    const u64 rhs = ZeroExtend64(ReadDWordOperand<val_mode, val_constant>(cpu));
    const u64 result = lhs * rhs;
    cpu->m_registers.EAX = Truncate32(result);
    cpu->m_registers.EDX = Truncate32(result >> 32);

    const bool of_cf = (cpu->m_registers.EDX != 0);
    const u32 eax = cpu->m_registers.EAX;

    cpu->m_registers.EFLAGS.bits =
      (cpu->m_registers.EFLAGS.bits & ~(Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF)) | // Modify CF/OF/SF/ZF/PF
      BoolToUInt32(of_cf) |                                                                 // CF
      (BoolToUInt32(of_cf) << 11) |                                                         // OF
      SignFlag(eax) |                                                                       // SF
      ZeroFlag(eax) |                                                                       // ZF
      ParityFlag(eax);                                                                      // PF

    cpu->AddCyclesRM(CYCLES_MUL_32_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  }
}

template<OperandSize op1_size, OperandMode op1_mode, u32 op1_constant, OperandSize op2_size, OperandMode op2_mode,
         u32 op2_constant, OperandSize op3_size, OperandMode op3_mode, u32 op3_constant>
void Interpreter::Execute_Operation_IMUL(CPU* cpu)
{
  CalculateEffectiveAddress<op1_mode>(cpu);
  CalculateEffectiveAddress<op2_mode>(cpu);
  CalculateEffectiveAddress<op3_mode>(cpu);

  const OperandSize actual_size = (op1_size == OperandSize_Count) ? cpu->idata.operand_size : op1_size;
  if (actual_size == OperandSize_8)
  {
    // Two and three-operand forms do not exist for 8-bit sources
    static_assert(op1_size != OperandSize_8 || (op2_mode == OperandMode_None && op3_mode == OperandMode_None),
                  "8-bit source only has one operand form");

    u16 lhs = SignExtend16(cpu->m_registers.AL);
    u16 rhs = SignExtend16(ReadByteOperand<op1_mode, op1_constant>(cpu));
    u16 result = u16(s16(lhs) * s16(rhs));
    u8 truncated_result = Truncate8(result);

    cpu->m_registers.AX = result;

    cpu->AddCyclesRM(CYCLES_IMUL_8_RM_MEM, cpu->idata.ModRM_RM_IsReg());
    cpu->m_registers.EFLAGS.OF = cpu->m_registers.EFLAGS.CF = (SignExtend16(truncated_result) != result);
    cpu->m_registers.EFLAGS.SF = IsSign(truncated_result);
    cpu->m_registers.EFLAGS.ZF = IsZero(truncated_result);
    cpu->m_registers.EFLAGS.PF = IsParity(truncated_result);
  }
  else if (actual_size == OperandSize_16)
  {
    u32 lhs, rhs;
    u32 result;
    u16 truncated_result;

    if constexpr (op3_mode != OperandMode_None)
    {
      // Three-operand form
      lhs = SignExtend32(ReadSignExtendedWordOperand<op2_size, op2_mode, op2_constant>(cpu));
      rhs = SignExtend32(ReadSignExtendedWordOperand<op3_size, op3_mode, op3_constant>(cpu));
      result = u32(s32(lhs) * s32(rhs));
      truncated_result = Truncate16(result);

      cpu->AddCyclesRM(CYCLES_IMUL_16_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
      WriteWordOperand<op1_mode, op1_constant>(cpu, truncated_result);
    }
    else if constexpr (op2_mode != OperandMode_None)
    {
      // Two-operand form
      lhs = SignExtend32(ReadSignExtendedWordOperand<op1_size, op1_mode, op1_constant>(cpu));
      rhs = SignExtend32(ReadSignExtendedWordOperand<op2_size, op2_mode, op2_constant>(cpu));
      result = u32(s32(lhs) * s32(rhs));
      truncated_result = Truncate16(result);

      cpu->AddCyclesRM(CYCLES_IMUL_16_RM_MEM, cpu->idata.ModRM_RM_IsReg());
      WriteWordOperand<op1_mode, op1_constant>(cpu, truncated_result);
    }
    else
    {
      // One-operand form
      lhs = SignExtend32(cpu->m_registers.AX);
      rhs = SignExtend32(ReadSignExtendedWordOperand<op1_size, op1_mode, op1_constant>(cpu));
      result = u32(s32(lhs) * s32(rhs));
      truncated_result = Truncate16(result);

      cpu->AddCyclesRM(CYCLES_IMUL_16_RM_MEM, cpu->idata.ModRM_RM_IsReg());
      cpu->m_registers.DX = Truncate16(result >> 16);
      cpu->m_registers.AX = truncated_result;
    }

    cpu->m_registers.EFLAGS.OF = cpu->m_registers.EFLAGS.CF = (SignExtend32(truncated_result) != result);
    cpu->m_registers.EFLAGS.SF = IsSign(truncated_result);
    cpu->m_registers.EFLAGS.ZF = IsZero(truncated_result);
    cpu->m_registers.EFLAGS.PF = IsParity(truncated_result);
  }
  else if (actual_size == OperandSize_32)
  {
    u64 lhs, rhs;
    u64 result;
    u32 truncated_result;

    if constexpr (op3_mode != OperandMode_None)
    {
      // Three-operand form
      lhs = SignExtend64(ReadSignExtendedDWordOperand<op2_size, op2_mode, op2_constant>(cpu));
      rhs = SignExtend64(ReadSignExtendedDWordOperand<op3_size, op3_mode, op3_constant>(cpu));
      result = u64(s64(lhs) * s64(rhs));
      truncated_result = Truncate32(result);

      cpu->AddCyclesRM(CYCLES_IMUL_32_REG_RM_MEM, cpu->idata.ModRM_RM_IsReg());
      WriteDWordOperand<op1_mode, op1_constant>(cpu, truncated_result);
    }
    else if constexpr (op2_mode != OperandMode_None)
    {
      // Two-operand form
      lhs = SignExtend64(ReadSignExtendedDWordOperand<op1_size, op1_mode, op1_constant>(cpu));
      rhs = SignExtend64(ReadSignExtendedDWordOperand<op2_size, op2_mode, op2_constant>(cpu));
      result = u64(s64(lhs) * s64(rhs));
      truncated_result = Truncate32(result);

      cpu->AddCyclesRM(CYCLES_IMUL_32_RM_MEM, cpu->idata.ModRM_RM_IsReg());
      WriteDWordOperand<op1_mode, op1_constant>(cpu, truncated_result);
    }
    else
    {
      // One-operand form
      lhs = SignExtend64(cpu->m_registers.EAX);
      rhs = SignExtend64(ReadSignExtendedDWordOperand<op1_size, op1_mode, op1_constant>(cpu));
      result = u64(s64(lhs) * s64(rhs));
      truncated_result = Truncate32(result);

      cpu->AddCyclesRM(CYCLES_IMUL_32_RM_MEM, cpu->idata.ModRM_RM_IsReg());
      cpu->m_registers.EDX = Truncate32(u64(result) >> 32);
      cpu->m_registers.EAX = truncated_result;
    }

    cpu->m_registers.EFLAGS.OF = cpu->m_registers.EFLAGS.CF = (SignExtend64(truncated_result) != result);
    cpu->m_registers.EFLAGS.SF = IsSign(truncated_result);
    cpu->m_registers.EFLAGS.ZF = IsZero(truncated_result);
    cpu->m_registers.EFLAGS.PF = IsParity(truncated_result);
  }
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
void Interpreter::Execute_Operation_DIV(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    cpu->AddCyclesRM(CYCLES_DIV_8_RM_MEM, cpu->idata.ModRM_RM_IsReg());

    // Eight-bit divides use AX as a source
    u8 divisor = ReadByteOperand<val_mode, val_constant>(cpu);
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    u16 source = cpu->m_registers.AX;
    u16 quotient = source / divisor;
    u16 remainder = source % divisor;
    if (quotient > 0xFF)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    cpu->m_registers.AL = u8(quotient);
    cpu->m_registers.AH = u8(remainder);
  }
  else if (actual_size == OperandSize_16)
  {
    cpu->AddCyclesRM(CYCLES_DIV_16_RM_MEM, cpu->idata.ModRM_RM_IsReg());

    // 16-bit divides use DX:AX as a source
    u16 divisor = ReadWordOperand<val_mode, val_constant>(cpu);
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    u32 source = (u32(cpu->m_registers.DX) << 16) | u32(cpu->m_registers.AX);
    u32 quotient = source / divisor;
    u32 remainder = source % divisor;
    if (quotient > 0xFFFF)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    cpu->m_registers.AX = u16(quotient);
    cpu->m_registers.DX = u16(remainder);
  }
  else if (actual_size == OperandSize_32)
  {
    cpu->AddCyclesRM(CYCLES_DIV_32_RM_MEM, cpu->idata.ModRM_RM_IsReg());

    // 32-bit divides use EDX:EAX as a source
    u32 divisor = ReadDWordOperand<val_mode, val_constant>(cpu);
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    u64 source = (ZeroExtend64(cpu->m_registers.EDX) << 32) | ZeroExtend64(cpu->m_registers.EAX);
    u64 quotient = source / divisor;
    u64 remainder = source % divisor;
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

template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
void Interpreter::Execute_Operation_IDIV(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    cpu->AddCyclesRM(CYCLES_IDIV_8_RM_MEM, cpu->idata.ModRM_RM_IsReg());

    // Eight-bit divides use AX as a source
    s8 divisor = s8(ReadByteOperand<val_mode, val_constant>(cpu));
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    s16 source = s16(cpu->m_registers.AX);
    s16 quotient = source / divisor;
    s16 remainder = source % divisor;
    u8 truncated_quotient = Truncate8(u16(quotient));
    u8 truncated_remainder = Truncate8(u16(remainder));
    if (s8(truncated_quotient) != quotient)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    cpu->m_registers.AL = truncated_quotient;
    cpu->m_registers.AH = truncated_remainder;
  }
  else if (actual_size == OperandSize_16)
  {
    cpu->AddCyclesRM(CYCLES_IDIV_16_RM_MEM, cpu->idata.ModRM_RM_IsReg());

    // 16-bit divides use DX:AX as a source
    s16 divisor = s16(ReadWordOperand<val_mode, val_constant>(cpu));
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    s32 source = s32((u32(cpu->m_registers.DX) << 16) | u32(cpu->m_registers.AX));
    s32 quotient = source / divisor;
    s32 remainder = source % divisor;
    u16 truncated_quotient = Truncate16(u32(quotient));
    u16 truncated_remainder = Truncate16(u32(remainder));
    if (s16(truncated_quotient) != quotient)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    cpu->m_registers.AX = truncated_quotient;
    cpu->m_registers.DX = truncated_remainder;
  }
  else if (actual_size == OperandSize_32)
  {
    cpu->AddCyclesRM(CYCLES_IDIV_32_RM_MEM, cpu->idata.ModRM_RM_IsReg());

    // 16-bit divides use DX:AX as a source
    s32 divisor = s32(ReadDWordOperand<val_mode, val_constant>(cpu));
    if (divisor == 0)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    s64 source = s64((ZeroExtend64(cpu->m_registers.EDX) << 32) | ZeroExtend64(cpu->m_registers.EAX));
    s64 quotient = source / divisor;
    s64 remainder = source % divisor;
    u32 truncated_quotient = Truncate32(u64(quotient));
    u32 truncated_remainder = Truncate32(u64(remainder));
    if (s32(truncated_quotient) != quotient)
    {
      cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    cpu->m_registers.EAX = truncated_quotient;
    cpu->m_registers.EDX = truncated_remainder;
  }
}

template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
void Interpreter::Execute_Operation_PUSH(CPU* cpu)
{
  CalculateEffectiveAddress<src_mode>(cpu);

  if constexpr (src_mode == OperandMode_Immediate)
    cpu->AddCycles(CYCLES_PUSH_IMM);
  else if constexpr (src_mode == OperandMode_Register)
    cpu->AddCycles(CYCLES_PUSH_REG);
  else if constexpr (src_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_PUSH_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<src_mode>::value, "unknown mode");

  if (cpu->idata.operand_size == OperandSize_16)
  {
    u16 value = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    cpu->PushWord(value);
  }
  else if (cpu->idata.operand_size == OperandSize_32)
  {
    u32 value = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    cpu->PushDWord(value);
  }
}

template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
void Interpreter::Execute_Operation_PUSH_Sreg(CPU* cpu)
{
  static_assert(src_size == OperandSize_16 && src_mode == OperandMode_SegmentRegister && src_constant < Segment_Count,
                "operands are of correct type and in range");

  cpu->AddCycles(CYCLES_PUSH_SREG);

  u16 selector = cpu->m_registers.segment_selectors[src_constant];
  if (cpu->idata.operand_size == OperandSize_16)
    cpu->PushWord(selector);
  else
    cpu->PushWord32(selector);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_POP_Sreg(CPU* cpu)
{
  static_assert(dst_size == OperandSize_16 && dst_mode == OperandMode_SegmentRegister && dst_constant < Segment_Count,
                "operands are of correct type and in range");

  cpu->AddCyclesPMode(CYCLES_POP_SREG);

  u16 selector;
  if (cpu->idata.operand_size == OperandSize_16)
    selector = cpu->PopWord();
  else
    selector = Truncate16(cpu->PopDWord());

  cpu->LoadSegmentRegister(static_cast<Segment>(dst_constant), selector);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_POP(CPU* cpu)
{
  if constexpr (dst_mode == OperandMode_Register)
    cpu->AddCycles(CYCLES_PUSH_REG);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_PUSH_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  // POP can use ESP in the address calculations, in this case the value of ESP
  // is that after the pop operation has occurred, not before.
  if (cpu->idata.operand_size == OperandSize_16)
  {
    u16 value = cpu->PopWord();
    CalculateEffectiveAddress<dst_mode>(cpu);
    WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else
  {
    u32 value = cpu->PopDWord();
    CalculateEffectiveAddress<dst_mode>(cpu);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
}

void Interpreter::Execute_Operation_PUSHA(CPU* cpu)
{
  cpu->AddCycles(CYCLES_PUSHA);
  if (cpu->idata.operand_size == OperandSize_16)
  {
    u16 old_SP = cpu->m_registers.SP;
    cpu->PushWord(cpu->m_registers.AX);
    cpu->PushWord(cpu->m_registers.CX);
    cpu->PushWord(cpu->m_registers.DX);
    cpu->PushWord(cpu->m_registers.BX);
    cpu->PushWord(old_SP);
    cpu->PushWord(cpu->m_registers.BP);
    cpu->PushWord(cpu->m_registers.SI);
    cpu->PushWord(cpu->m_registers.DI);
  }
  else if (cpu->idata.operand_size == OperandSize_32)
  {
    u32 old_ESP = cpu->m_registers.ESP;
    cpu->PushDWord(cpu->m_registers.EAX);
    cpu->PushDWord(cpu->m_registers.ECX);
    cpu->PushDWord(cpu->m_registers.EDX);
    cpu->PushDWord(cpu->m_registers.EBX);
    cpu->PushDWord(old_ESP);
    cpu->PushDWord(cpu->m_registers.EBP);
    cpu->PushDWord(cpu->m_registers.ESI);
    cpu->PushDWord(cpu->m_registers.EDI);
  }
}

void Interpreter::Execute_Operation_POPA(CPU* cpu)
{
  cpu->AddCycles(CYCLES_POPA);

  // Assignment split from reading in case of exception.
  if (cpu->idata.operand_size == OperandSize_16)
  {
    u16 DI = cpu->PopWord();
    u16 SI = cpu->PopWord();
    u16 BP = cpu->PopWord();
    /*uint16 SP = */ cpu->PopWord();
    u16 BX = cpu->PopWord();
    u16 DX = cpu->PopWord();
    u16 CX = cpu->PopWord();
    u16 AX = cpu->PopWord();
    cpu->m_registers.DI = DI;
    cpu->m_registers.SI = SI;
    cpu->m_registers.BP = BP;
    cpu->m_registers.BX = BX;
    cpu->m_registers.DX = DX;
    cpu->m_registers.CX = CX;
    cpu->m_registers.AX = AX;
  }
  else if (cpu->idata.operand_size == OperandSize_32)
  {
    u32 EDI = cpu->PopDWord();
    u32 ESI = cpu->PopDWord();
    u32 EBP = cpu->PopDWord();
    /*uint32 ESP = */ cpu->PopDWord();
    u32 EBX = cpu->PopDWord();
    u32 EDX = cpu->PopDWord();
    u32 ECX = cpu->PopDWord();
    u32 EAX = cpu->PopDWord();
    cpu->m_registers.EDI = EDI;
    cpu->m_registers.ESI = ESI;
    cpu->m_registers.EBP = EBP;
    cpu->m_registers.EBX = EBX;
    cpu->m_registers.EDX = EDX;
    cpu->m_registers.ECX = ECX;
    cpu->m_registers.EAX = EAX;
  }
}

template<OperandSize frame_size, OperandMode frame_mode, u32 frame_constant, OperandSize level_size,
         OperandMode level_mode, u32 level_constant>
void Interpreter::Execute_Operation_ENTER(CPU* cpu)
{
  static_assert(frame_size == OperandSize_16 && level_size == OperandSize_8, "args have correct size");
  cpu->AddCycles(CYCLES_ENTER);

  u16 stack_frame_size = ReadWordOperand<frame_mode, frame_constant>(cpu);
  u8 level = ReadByteOperand<level_mode, level_constant>(cpu);

  // Push current frame pointer.
  if (cpu->idata.operand_size == OperandSize_16)
    cpu->PushWord(cpu->m_registers.BP);
  else
    cpu->PushDWord(cpu->m_registers.EBP);

  u32 frame_pointer = cpu->m_registers.ESP;
  if (level > 0)
  {
    // Use our own local copy of EBP in case any of these fail.
    if (cpu->idata.operand_size == OperandSize_16)
    {
      u16 BP = cpu->m_registers.BP;
      for (u8 i = 1; i < level; i++)
      {
        BP -= sizeof(u16);

        u16 prev_ptr = cpu->ReadMemoryWord(Segment_SS, BP);
        cpu->PushWord(prev_ptr);
      }
      cpu->PushDWord(frame_pointer);
      cpu->m_registers.BP = BP;
    }
    else
    {
      u32 EBP = cpu->m_registers.EBP;
      for (u8 i = 1; i < level; i++)
      {
        EBP -= sizeof(u32);

        u32 prev_ptr = cpu->ReadMemoryDWord(Segment_SS, EBP);
        cpu->PushDWord(prev_ptr);
      }
      cpu->PushDWord(frame_pointer);
      cpu->m_registers.EBP = EBP;
    }
  }

  if (cpu->idata.operand_size == OperandSize_16)
    cpu->m_registers.BP = Truncate16(frame_pointer);
  else
    cpu->m_registers.EBP = frame_pointer;

  if (cpu->m_stack_address_size == AddressSize_16)
    cpu->m_registers.SP -= stack_frame_size;
  else
    cpu->m_registers.ESP -= stack_frame_size;
}

void Interpreter::Execute_Operation_LEAVE(CPU* cpu)
{
  cpu->AddCycles(CYCLES_LEAVE);
  if (cpu->m_stack_address_size == AddressSize_16)
    cpu->m_registers.SP = cpu->m_registers.BP;
  else
    cpu->m_registers.ESP = cpu->m_registers.EBP;

  if (cpu->idata.operand_size == OperandSize_16)
    cpu->m_registers.BP = cpu->PopWord();
  else if (cpu->idata.operand_size == OperandSize_32)
    cpu->m_registers.EBP = cpu->PopDWord();
}

template<OperandSize sreg_size, OperandMode sreg_mode, u32 sreg_constant, OperandSize reg_size, OperandMode reg_mode,
         u32 reg_constant, OperandSize ptr_size, OperandMode ptr_mode, u32 ptr_constant>
void Interpreter::Execute_Operation_LXS(CPU* cpu)
{
  static_assert(sreg_mode == OperandMode_SegmentRegister, "sreg_mode is Segment Register");
  static_assert(reg_mode == OperandMode_ModRM_Reg, "reg_mode is Register");
  static_assert(ptr_mode == OperandMode_ModRM_RM, "reg_mode is Pointer");

  // #UD if the second argument is a register, not memory.
  if (ptr_mode == OperandMode_ModRM_RM && cpu->idata.modrm_rm_register)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  CalculateEffectiveAddress<ptr_mode>(cpu);
  cpu->AddCyclesPMode(CYCLES_LxS);

  u16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand<ptr_mode>(cpu, cpu->idata.operand_size, &segment_selector, &address);

  Segment sreg = static_cast<Segment>(sreg_constant);
  cpu->LoadSegmentRegister(sreg, segment_selector);

  if (cpu->idata.operand_size == OperandSize_16)
    WriteWordOperand<reg_mode, reg_constant>(cpu, Truncate16(address));
  else if (cpu->idata.operand_size == OperandSize_32)
    WriteDWordOperand<reg_mode, reg_constant>(cpu, address);
  else
    DebugUnreachableCode();
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_LEA(CPU* cpu)
{
  static_assert(src_mode == OperandMode_ModRM_RM, "Source operand is a pointer");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  cpu->AddCycles(CYCLES_LEA);

  // Calculate full address in instruction's address mode, truncate/extend to operand size.
  if (cpu->idata.operand_size == OperandSize_16)
    WriteWordOperand<dst_mode, dst_constant>(cpu, Truncate16(cpu->m_effective_address));
  else
    WriteDWordOperand<dst_mode, dst_constant>(cpu, cpu->m_effective_address);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_JMP_Near(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  if constexpr (dst_mode == OperandMode_Relative)
    cpu->AddCycles(CYCLES_JMP_NEAR);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_JMP_NEAR_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_size, dst_mode, dst_constant>(cpu);
  cpu->BranchTo(jump_address);
}

template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_Jcc(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  if (!TestJumpCondition<condition>(cpu))
  {
    cpu->AddCycles((condition == JumpCondition_CXZero) ? CYCLES_JCXZ_NOT_TAKEN : CYCLES_Jcc_NOT_TAKEN);
    return;
  }

  VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_size, dst_mode, dst_constant>(cpu);
  cpu->AddCycles((condition == JumpCondition_CXZero) ? CYCLES_JCXZ_TAKEN : CYCLES_Jcc_TAKEN);
  cpu->BranchTo(jump_address);
}

template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_LOOP(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  cpu->AddCycles((condition != JumpCondition_Always) ? CYCLES_LOOPZ : CYCLES_LOOP);

  u32 count;
  if (cpu->idata.address_size == AddressSize_16)
    count = ZeroExtend32(--cpu->m_registers.CX);
  else
    count = ZeroExtend32(--cpu->m_registers.ECX);

  bool branch = (count != 0) && TestJumpCondition<condition>(cpu);
  if (!branch)
    return;

  VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_size, dst_mode, dst_constant>(cpu);
  cpu->BranchTo(jump_address);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_CALL_Near(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  if constexpr (dst_mode == OperandMode_Relative)
    cpu->AddCycles(CYCLES_CALL_NEAR);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_CALL_NEAR_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_size, dst_mode, dst_constant>(cpu);
  if (cpu->idata.operand_size == OperandSize_16)
    cpu->PushWord(Truncate16(cpu->m_registers.EIP));
  else
    cpu->PushDWord(cpu->m_registers.EIP);

  cpu->BranchTo(jump_address);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_RET_Near(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  cpu->AddCycles(CYCLES_RET_NEAR);

  u32 pop_count = 0;
  if constexpr (dst_mode != OperandMode_None)
    pop_count = ReadZeroExtendedDWordOperand<dst_size, dst_mode, dst_constant>(cpu);

  u32 return_EIP;
  if (cpu->idata.operand_size == OperandSize_16)
  {
    return_EIP = ZeroExtend32(cpu->PopWord());
  }
  else if (cpu->idata.operand_size == OperandSize_32)
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

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_JMP_Far(CPU* cpu)
{
  // #UD if the second argument is a register, not memory.
  if (dst_mode == OperandMode_ModRM_RM && cpu->idata.modrm_rm_register)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);

  if constexpr (dst_mode == OperandMode_FarAddress)
    cpu->AddCycles(CYCLES_JMP_FAR);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesPMode(CYCLES_JMP_FAR_PTR);
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  u16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand<dst_mode>(cpu, actual_size, &segment_selector, &address);

  cpu->FarJump(segment_selector, address, actual_size);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_CALL_Far(CPU* cpu)
{
  // #UD if the second argument is a register, not memory.
  if (dst_mode == OperandMode_ModRM_RM && cpu->idata.modrm_rm_register)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);

  if constexpr (dst_mode == OperandMode_FarAddress)
    cpu->AddCycles(CYCLES_CALL_FAR);
  else if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesPMode(CYCLES_CALL_FAR_PTR);
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  u16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand<dst_mode>(cpu, actual_size, &segment_selector, &address);

  cpu->FarCall(segment_selector, address, actual_size);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_RET_Far(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  cpu->AddCycles(CYCLES_RET_FAR);

  u32 pop_count = 0;
  if constexpr (dst_mode != OperandMode_None)
    pop_count = ReadZeroExtendedDWordOperand<dst_size, dst_mode, dst_constant>(cpu);

  cpu->FarReturn(cpu->idata.operand_size, pop_count);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_INT(CPU* cpu)
{
  static_assert(dst_size == OperandSize_8, "size is 8 bits");
  static_assert(dst_mode == OperandMode_Constant || dst_mode == OperandMode_Immediate, "constant or immediate");
  u8 interrupt = ReadByteOperand<dst_mode, dst_constant>(cpu);

  cpu->AddCycles(CYCLES_INT);
  cpu->SoftwareInterrupt(interrupt);
}

void Interpreter::Execute_Operation_INT3(CPU* cpu)
{
  cpu->AddCycles(CYCLES_INT3);
  cpu->RaiseSoftwareException(Interrupt_Breakpoint);
}

void Interpreter::Execute_Operation_INTO(CPU* cpu)
{
  // Call overflow exception if OF is set
  if (!cpu->m_registers.EFLAGS.OF)
  {
    cpu->AddCycles(CYCLES_INTO_FALSE);
    return;
  }

  cpu->AddCycles(CYCLES_INTO_TRUE);
  cpu->RaiseSoftwareException(Interrupt_Overflow);
}

void Interpreter::Execute_Operation_IRET(CPU* cpu)
{
  cpu->AddCyclesPMode(CYCLES_IRET);
  cpu->InterruptReturn(cpu->idata.operand_size);
}

void Interpreter::Execute_Operation_NOP(CPU* cpu)
{
  cpu->AddCycles(CYCLES_NOP);
}

void Interpreter::Execute_Operation_CLC(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CLEAR_SET_FLAG);
  cpu->m_registers.EFLAGS.CF = false;
}

void Interpreter::Execute_Operation_CLD(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CLEAR_SET_FLAG);
  cpu->m_registers.EFLAGS.DF = false;
}

void Interpreter::Execute_Operation_CLI(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CLI);

  // TODO: Delay of one instruction
  if (cpu->InProtectedMode() && cpu->GetIOPL() < cpu->GetCPL())
  {
    // Adjust VIF instead of IF for VME/PVI.
    if (cpu->InVirtual8086Mode())
    {
      if (cpu->m_registers.CR4.VME)
      {
        cpu->m_registers.EFLAGS.VIF = false;
        return;
      }
    }
    else if (cpu->m_registers.CR4.PVI && cpu->GetCPL() == 3)
    {
      cpu->m_registers.EFLAGS.VIF = false;
      return;
    }

    cpu->RaiseException(Interrupt_GeneralProtectionFault);
    return;
  }

  cpu->m_registers.EFLAGS.IF = false;
}

void Interpreter::Execute_Operation_CMC(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CLEAR_SET_FLAG);
  cpu->m_registers.EFLAGS.bits ^= Flag_CF; // cpu->m_registers.EFLAGS.CF = !cpu->m_registers.EFLAGS.CF
}

void Interpreter::Execute_Operation_CLTS(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CLTS);
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  cpu->m_registers.CR0 &= ~CR0Bit_TS;
}

void Interpreter::Execute_Operation_STC(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CLEAR_SET_FLAG);
  cpu->m_registers.EFLAGS.CF = true;
}

void Interpreter::Execute_Operation_STD(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CLEAR_SET_FLAG);
  cpu->m_registers.EFLAGS.DF = true;
}

void Interpreter::Execute_Operation_STI(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CLI);

  if (cpu->InProtectedMode() && cpu->GetIOPL() < cpu->GetCPL())
  {
    // Adjust VIF instead of IF for VME/PVI.
    if (cpu->InVirtual8086Mode())
    {
      if (cpu->m_registers.CR4.VME && !cpu->m_registers.EFLAGS.VIP)
      {
        cpu->m_registers.EFLAGS.VIF = true;
        return;
      }
    }
    else if (cpu->m_registers.CR4.PVI && cpu->GetCPL() == 3 && !cpu->m_registers.EFLAGS.VIP)
    {
      cpu->m_registers.EFLAGS.VIF = true;
      return;
    }

    cpu->RaiseException(Interrupt_GeneralProtectionFault);
    return;
  }

  cpu->m_registers.EFLAGS.IF = true;
}

void Interpreter::Execute_Operation_SALC(CPU* cpu)
{
  // Undocumented instruction. Same as SBB AL, AL without modifying any flags.
  u32 eflags = cpu->m_registers.EFLAGS.bits;
  cpu->AddCycles(CYCLES_ALU_REG_RM_REG);
  cpu->m_registers.AL = ALUOp_Sbb8(&eflags, cpu->m_registers.AL, cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.bits = eflags;
}

void Interpreter::Execute_Operation_LAHF(CPU* cpu)
{
  //         // Don't clear/set all flags, only those allowed
  //     const uint16 MASK = Flag_CF | Flag_Reserved | Flag_PF | Flag_AF | Flag_ZF | Flag_SF | Flag_TF | Flag_IF |
  //     Flag_DF | Flag_OF
  //         // 286+ only?
  //         // | Flag_IOPL;
  //     ;
  //
  //     return (Truncate16(m_registers.EFLAGS.bits) & MASK);
  //     uint16 flags = GetFlags16();
  //     m_registers.AH = uint8(flags & 0xFF);

  cpu->AddCycles(CYCLES_LAHF);
  cpu->m_registers.AH = Truncate8(cpu->m_registers.EFLAGS.bits);
}

void Interpreter::Execute_Operation_SAHF(CPU* cpu)
{
  const u32 change_mask = Flag_SF | Flag_ZF | Flag_AF | Flag_CF | Flag_PF;
  cpu->AddCycles(CYCLES_SAHF);
  cpu->SetFlags((cpu->m_registers.EFLAGS.bits & ~change_mask) | (ZeroExtend32(cpu->m_registers.AH) & change_mask));
}

void Interpreter::Execute_Operation_PUSHF(CPU* cpu)
{
  cpu->AddCycles(CYCLES_PUSHF);

  // RF and VM flags are cleared in the copy
  u32 EFLAGS = cpu->m_registers.EFLAGS.bits & ~(Flag_RF | Flag_VM);

  // IF modification is protected by IOPL in V8086 mode.
  if (cpu->InVirtual8086Mode() && cpu->GetIOPL() < 3)
  {
    // Without VME, trap to monitor.
    if (cpu->idata.operand_size != OperandSize_16 || !cpu->m_registers.CR4.VME)
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    // With VME, only the 16-bit operand version replaces IF with VIF.
    EFLAGS = (EFLAGS & ~(Flag_IF)) | ((EFLAGS & Flag_VIF) >> 10);
  }

  if (cpu->idata.operand_size == OperandSize_16)
    cpu->PushWord(Truncate16(EFLAGS));
  else if (cpu->idata.operand_size == OperandSize_32)
    cpu->PushDWord(EFLAGS);
}

void Interpreter::Execute_Operation_POPF(CPU* cpu)
{
  cpu->AddCycles(CYCLES_POPF);

  bool move_if_to_vif = false;

  // Some flags can't be changed if we're not in CPL=0.
  u32 change_mask =
    Flag_CF | Flag_PF | Flag_AF | Flag_ZF | Flag_SF | Flag_TF | Flag_DF | Flag_OF | Flag_NT | Flag_AC | Flag_ID;
  if (cpu->InProtectedMode())
  {
    // If V8086 and IOPL!=3, trap to monitor, unless VME is enabled.
    if (cpu->InVirtual8086Mode() && cpu->GetIOPL() < 3)
    {
      // Only the 16-bit PUSHF can be used in VME. If VIP is set, we should also #GP.
      if (cpu->idata.operand_size != OperandSize_16 || !cpu->m_registers.CR4.VME || cpu->m_registers.EFLAGS.VIP)
      {
        cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
        return;
      }

      move_if_to_vif = true;
      change_mask |= Flag_VIF;
    }
    else
    {
      // Neither of these will be true in V8086 mode and IOPL=3.
      if (cpu->GetCPL() <= cpu->GetIOPL())
        change_mask |= Flag_IF;
      if (cpu->GetCPL() == 0)
        change_mask |= Flag_IOPL;
    }
  }
  else
  {
    // Both can be updated in real mode.
    change_mask |= Flag_IF | Flag_IOPL;
  }

  // Pop flags off stack, leaving top 16 bits intact for 16-bit instructions.
  u32 flags = 0;
  if (cpu->idata.operand_size == OperandSize_16)
    flags = (cpu->m_registers.EFLAGS.bits & 0xFFFF0000) | ZeroExtend32(cpu->PopWord());
  else if (cpu->idata.operand_size == OperandSize_32)
    flags = cpu->PopDWord();
  else
    DebugUnreachableCode();

  // V8086 mode extensions, IF -> VIF.
  if (move_if_to_vif)
    flags = (flags & ~Flag_IF) | ((flags & Flag_VIF) >> 10);

  // Update flags.
  cpu->SetFlags((flags & change_mask) | (cpu->m_registers.EFLAGS.bits & ~change_mask));
}

void Interpreter::Execute_Operation_HLT(CPU* cpu)
{
  cpu->AddCycles(CYCLES_HLT);

  // HLT is a privileged instruction
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  cpu->Halt();
}

void Interpreter::Execute_Operation_CBW(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CBW);

  if (cpu->idata.operand_size == OperandSize_16)
  {
    // Sign-extend AL to AH
    cpu->m_registers.AH = ((cpu->m_registers.AL & 0x80) != 0) ? 0xFF : 0x00;
  }
  else if (cpu->idata.operand_size == OperandSize_32)
  {
    // Sign-extend AX to EAX
    cpu->m_registers.EAX = SignExtend32(cpu->m_registers.AX);
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_Operation_CWD(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CWD);

  if (cpu->idata.operand_size == OperandSize_16)
  {
    // Sign-extend AX to DX
    cpu->m_registers.DX = ((cpu->m_registers.AX & 0x8000) != 0) ? 0xFFFF : 0x0000;
  }
  else if (cpu->idata.operand_size == OperandSize_32)
  {
    // Sign-extend EAX to EDX
    cpu->m_registers.EDX = ((cpu->m_registers.EAX & 0x80000000) != 0) ? 0xFFFFFFFF : 0x00000000;
  }
  else
  {
    DebugUnreachableCode();
  }
}

void Interpreter::Execute_Operation_XLAT(CPU* cpu)
{
  cpu->AddCycles(CYCLES_XLAT);

  u8 value;
  if (cpu->idata.address_size == AddressSize_16)
  {
    u16 address = cpu->m_registers.BX + ZeroExtend16(cpu->m_registers.AL);
    value = cpu->ReadMemoryByte(cpu->idata.segment, address);
  }
  else if (cpu->idata.address_size == AddressSize_32)
  {
    u32 address = cpu->m_registers.EBX + ZeroExtend32(cpu->m_registers.AL);
    value = cpu->ReadMemoryByte(cpu->idata.segment, address);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
  cpu->m_registers.AL = value;
}

void Interpreter::Execute_Operation_AAA(CPU* cpu)
{
  cpu->AddCycles(CYCLES_BCD_ADDSUB);

  u32 new_eflags = cpu->m_registers.EFLAGS.bits & ~(Flag_AF | Flag_CF | Flag_SF | Flag_ZF | Flag_PF);
  if ((cpu->m_registers.AL & 0xF) > 0x09 || cpu->m_registers.EFLAGS.AF)
  {
    cpu->m_registers.AX += 0x0106;
    new_eflags |= Flag_AF | Flag_CF;
  }

  cpu->m_registers.AL &= 0x0F;

  new_eflags |= SignFlag(cpu->m_registers.AL) | ZeroFlag(cpu->m_registers.AL) | ParityFlag(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.bits = new_eflags;
}

void Interpreter::Execute_Operation_AAS(CPU* cpu)
{
  cpu->AddCycles(CYCLES_BCD_ADDSUB);

  u32 new_eflags = cpu->m_registers.EFLAGS.bits & ~(Flag_AF | Flag_CF | Flag_SF | Flag_ZF | Flag_PF);
  if ((cpu->m_registers.AL & 0xF) > 0x09 || cpu->m_registers.EFLAGS.AF)
  {
    cpu->m_registers.AX -= 0x0106;
    new_eflags |= Flag_AF | Flag_CF;
  }

  cpu->m_registers.AL &= 0x0F;

  new_eflags |= SignFlag(cpu->m_registers.AL) | ZeroFlag(cpu->m_registers.AL) | ParityFlag(cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.bits = new_eflags;
}

template<OperandSize op_size, OperandMode op_mode, u32 op_constant>
void Interpreter::Execute_Operation_AAM(CPU* cpu)
{
  CalculateEffectiveAddress<op_mode>(cpu);
  cpu->AddCycles(CYCLES_AAM);

  const u8 operand = ReadByteOperand<op_mode, op_constant>(cpu);
  if (operand == 0)
  {
    cpu->RaiseException(Interrupt_DivideError);
    return;
  }

  cpu->m_registers.AH = cpu->m_registers.AL / operand;
  cpu->m_registers.AL = cpu->m_registers.AL % operand;

  cpu->m_registers.EFLAGS.bits =
    (cpu->m_registers.EFLAGS.bits & ~(Flag_AF | Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF)) |
    SignFlag(cpu->m_registers.AL) | ZeroFlag(cpu->m_registers.AL) | ParityFlag(cpu->m_registers.AL);
}

template<OperandSize op_size, OperandMode op_mode, u32 op_constant>
void Interpreter::Execute_Operation_AAD(CPU* cpu)
{
  CalculateEffectiveAddress<op_mode>(cpu);
  cpu->AddCycles(CYCLES_AAD);

  const u8 operand = ReadByteOperand<op_mode, op_constant>(cpu);
  const u16 result = u16(cpu->m_registers.AH) * u16(operand) + u16(cpu->m_registers.AL);

  cpu->m_registers.AL = u8(result & 0xFF);
  cpu->m_registers.AH = 0;

  cpu->m_registers.EFLAGS.bits =
    (cpu->m_registers.EFLAGS.bits & ~(Flag_AF | Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF)) |
    SignFlag(cpu->m_registers.AL) | ZeroFlag(cpu->m_registers.AL) | ParityFlag(cpu->m_registers.AL);
}

void Interpreter::Execute_Operation_DAA(CPU* cpu)
{
  cpu->AddCycles(CYCLES_BCD_ADDSUB);

  u8 new_al = cpu->m_registers.AL;
  u32 new_eflags = cpu->m_registers.EFLAGS.bits & ~(Flag_AF | Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF);

  if ((cpu->m_registers.AL & 0xF) > 0x9 || cpu->m_registers.EFLAGS.AF)
  {
    new_al += 0x6;
    new_eflags |= Flag_AF | BoolToUInt32(cpu->m_registers.AL > 0xF9 || cpu->m_registers.EFLAGS.CF); // CF
  }

  if (cpu->m_registers.AL > 0x99 || cpu->m_registers.EFLAGS.CF)
  {
    new_al += 0x60;
    new_eflags |= Flag_CF;
  }

  new_eflags |= SignFlag(new_al) | ZeroFlag(new_al) | ParityFlag(new_al);

  cpu->m_registers.AL = new_al;
  cpu->m_registers.EFLAGS.bits = new_eflags;
}

void Interpreter::Execute_Operation_DAS(CPU* cpu)
{
  cpu->AddCycles(CYCLES_BCD_ADDSUB);

  u8 new_al = cpu->m_registers.AL;
  u32 new_eflags = cpu->m_registers.EFLAGS.bits & ~(Flag_AF | Flag_CF | Flag_OF | Flag_SF | Flag_ZF | Flag_PF);
  if ((cpu->m_registers.AL & 0xF) > 0x9 || cpu->m_registers.EFLAGS.AF)
  {
    new_al -= 0x6;
    new_eflags |= Flag_AF | BoolToUInt32(cpu->m_registers.AL < 0x06 || cpu->m_registers.EFLAGS.CF); // CF
  }

  if (cpu->m_registers.AL > 0x99 || cpu->m_registers.EFLAGS.CF)
  {
    new_al -= 0x60;
    new_eflags |= Flag_CF;
  }

  new_eflags |= SignFlag(new_al) | ZeroFlag(new_al) | ParityFlag(new_al);

  cpu->m_registers.AL = new_al;
  cpu->m_registers.EFLAGS.bits = new_eflags;
}

template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
void Interpreter::Execute_Operation_BSWAP(CPU* cpu)
{
  CalculateEffectiveAddress<val_mode>(cpu);
  cpu->AddCycles(CYCLES_BSWAP);

  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  if (actual_size == OperandSize_32)
  {
    u32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    value = Y_byteswap_uint32(value);
    WriteDWordOperand<val_mode, val_constant>(cpu, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize addr_size, OperandMode addr_mode, u32 addr_constant>
void Interpreter::Execute_Operation_INVLPG(CPU* cpu)
{
  CalculateEffectiveAddress<addr_mode>(cpu);
  cpu->AddCycles(CYCLES_INVLPG);

  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // Get effective address of operand, this is the linear address to clear.
  if (cpu->idata.ModRM_RM_IsReg() || cpu->idata.has_lock)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode, 0);
    return;
  }

  cpu->InvalidateTLBEntry(cpu->m_effective_address);
}

template<OperandSize addr_size, OperandMode addr_mode, u32 addr_constant, OperandSize table_size,
         OperandMode table_mode, u32 table_constant>
void Interpreter::Execute_Operation_BOUND(CPU* cpu)
{
  CalculateEffectiveAddress<addr_mode>(cpu);
  CalculateEffectiveAddress<table_mode>(cpu);

  u32 address;
  u32 lower_bound;
  u32 upper_bound;
  VirtualMemoryAddress table_address = cpu->m_effective_address;
  const OperandSize actual_size = (addr_size == OperandSize_Count) ? cpu->idata.operand_size : addr_size;
  if (actual_size == OperandSize_16)
  {
    address = ZeroExtend32(ReadWordOperand<addr_mode, addr_constant>(cpu));
    lower_bound = ZeroExtend32(cpu->ReadMemoryWord(cpu->idata.segment, table_address + 0));
    upper_bound = ZeroExtend32(cpu->ReadMemoryWord(cpu->idata.segment, table_address + 2));
  }
  else if (actual_size == OperandSize_32)
  {
    address = ReadDWordOperand<addr_mode, addr_constant>(cpu);
    lower_bound = cpu->ReadMemoryDWord(cpu->idata.segment, table_address + 0);
    upper_bound = cpu->ReadMemoryDWord(cpu->idata.segment, table_address + 4);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (address < lower_bound || address > upper_bound)
  {
    cpu->AddCycles(CYCLES_BOUND_FAIL);
    cpu->RaiseSoftwareException(Interrupt_Bounds);
  }
  else
  {
    cpu->AddCycles(CYCLES_BOUND_SUCCESS);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_ARPL(CPU* cpu)
{
  static_assert(src_size == OperandSize_16 && dst_size == OperandSize_16, "operand sizes are 16-bits");
  cpu->AddCyclesRM(CYCLES_ARPL_RM_MEM, cpu->idata.ModRM_RM_IsReg());

  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  SEGMENT_SELECTOR_VALUE dst = {ReadWordOperand<dst_mode, dst_constant>(cpu)};
  SEGMENT_SELECTOR_VALUE src = {ReadWordOperand<src_mode, src_constant>(cpu)};

  if (dst.rpl < src.rpl)
  {
    dst.rpl = src.rpl.GetValue();
    WriteWordOperand<dst_mode, dst_constant>(cpu, dst.bits);
    cpu->m_registers.EFLAGS.ZF = true;
  }
  else
  {
    cpu->m_registers.EFLAGS.ZF = false;
  }
}

template<Operation operation, OperandSize selector_size, OperandMode selector_mode, u32 selector_constant>
void Interpreter::Execute_Operation_VERx(CPU* cpu)
{
  static_assert(selector_size == OperandSize_16, "selector size is 16-bits");
  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  CalculateEffectiveAddress<selector_mode>(cpu);

  SEGMENT_SELECTOR_VALUE selector;
  selector.bits = ReadWordOperand<selector_mode, selector_constant>(cpu);

  // Check the selector is valid, and read the selector
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
  if constexpr (operation == Operation_VERR)
  {
    bool is_readable = (!descriptor.IsCodeSegment() || descriptor.memory.access.code_readable);
    cpu->m_registers.EFLAGS.ZF = is_readable;
  }
  else if constexpr (operation == Operation_VERW)
  {
    bool is_writable = (!descriptor.IsCodeSegment() && descriptor.memory.access.data_writable);
    cpu->m_registers.EFLAGS.ZF = is_writable;
  }
}

template<OperandSize selector_size, OperandMode selector_mode, u32 selector_constant>
void Interpreter::Execute_Operation_VERW(CPU* cpu)
{
  cpu->AddCyclesRM(CYCLES_VERR_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  Execute_Operation_VERx<Operation_VERW, selector_size, selector_mode, selector_constant>(cpu);
}

template<OperandSize selector_size, OperandMode selector_mode, u32 selector_constant>
void Interpreter::Execute_Operation_VERR(CPU* cpu)
{
  cpu->AddCyclesRM(CYCLES_VERW_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  Execute_Operation_VERx<Operation_VERR, selector_size, selector_mode, selector_constant>(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize selector_size,
         OperandMode selector_mode, u32 selector_constant>
void Interpreter::Execute_Operation_LSL(CPU* cpu)
{
  static_assert(selector_size == OperandSize_16, "selector size is 16-bits");
  cpu->AddCyclesRM(CYCLES_LSL_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<selector_mode>(cpu);

  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  SEGMENT_SELECTOR_VALUE selector;
  selector.bits = ReadWordOperand<selector_mode, selector_constant>(cpu);

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

  const u32 limit = descriptor.is_memory_descriptor ? descriptor.memory.GetLimit() : descriptor.tss.GetLimit();
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_16)
    WriteWordOperand<dst_mode, dst_constant>(cpu, Truncate16(limit));
  else if (actual_size == OperandSize_32)
    WriteDWordOperand<dst_mode, dst_constant>(cpu, limit);
  else
    DebugUnreachableCode();

  cpu->m_registers.EFLAGS.ZF = true;
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize selector_size,
         OperandMode selector_mode, u32 selector_constant>
void Interpreter::Execute_Operation_LAR(CPU* cpu)
{
  static_assert(selector_size == OperandSize_16, "selector size is 16-bits");
  cpu->AddCyclesRM(CYCLES_LAR_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<selector_mode>(cpu);

  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  SEGMENT_SELECTOR_VALUE selector;
  selector.bits = ReadWordOperand<selector_mode, selector_constant>(cpu);

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
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  const u32 result = descriptor.bits1 & 0x00FFFF00;
  if (actual_size == OperandSize_16)
    WriteWordOperand<dst_mode, dst_constant>(cpu, Truncate16(result));
  else if (actual_size == OperandSize_32)
    WriteDWordOperand<dst_mode, dst_constant>(cpu, result);
  else
    DebugUnreachableCode();

  cpu->m_registers.EFLAGS.ZF = true;
}

template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
void Interpreter::Execute_Operation_LIDT(CPU* cpu)
{
  cpu->AddCycles(CYCLES_LGDT);
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  CalculateEffectiveAddress<src_mode>(cpu);
  const VirtualMemoryAddress base_address = cpu->m_effective_address;
  u32 table_limit = ZeroExtend32(cpu->ReadMemoryWord(cpu->idata.segment, base_address + 0));
  u32 table_base_address = cpu->ReadMemoryDWord(cpu->idata.segment, base_address + 2);

  // 16-bit operand drops higher order bits
  if (cpu->idata.operand_size == OperandSize_16)
    table_base_address &= 0xFFFFFF;

  cpu->LoadInterruptDescriptorTable(table_base_address, table_limit);
}

template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
void Interpreter::Execute_Operation_LGDT(CPU* cpu)
{
  cpu->AddCycles(CYCLES_LGDT);
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  CalculateEffectiveAddress<src_mode>(cpu);
  const VirtualMemoryAddress base_address = cpu->m_effective_address;
  u32 table_limit = ZeroExtend32(cpu->ReadMemoryWord(cpu->idata.segment, base_address + 0));
  u32 table_base_address = cpu->ReadMemoryDWord(cpu->idata.segment, base_address + 2);

  // 16-bit operand drops higher order bits
  if (cpu->idata.operand_size == OperandSize_16)
    table_base_address &= 0xFFFFFF;

  cpu->LoadGlobalDescriptorTable(table_base_address, table_limit);
}

template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
void Interpreter::Execute_Operation_LLDT(CPU* cpu)
{
  cpu->AddCyclesRM(CYCLES_LLDT_RM_MEM, cpu->idata.ModRM_RM_IsReg());
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

  CalculateEffectiveAddress<src_mode>(cpu);
  const u16 selector = ReadWordOperand<src_mode, src_constant>(cpu);
  cpu->LoadLocalDescriptorTable(selector);
}

template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
void Interpreter::Execute_Operation_LTR(CPU* cpu)
{
  cpu->AddCyclesRM(CYCLES_LTR_RM_MEM, cpu->idata.ModRM_RM_IsReg());
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

  CalculateEffectiveAddress<src_mode>(cpu);
  const u16 selector = ReadWordOperand<src_mode, src_constant>(cpu);
  cpu->LoadTaskSegment(selector);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_SIDT(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  cpu->AddCycles(CYCLES_SGDT);

  u32 idt_address = Truncate32(cpu->m_idt_location.base_address);
  u16 idt_limit = Truncate16(cpu->m_idt_location.limit);

  // 16-bit operand sets higher-order bits to zero
  if (cpu->idata.operand_size == OperandSize_16)
    idt_address = (idt_address & 0xFFFFFF);

  // Write back to memory
  const VirtualMemoryAddress base_address = cpu->m_effective_address;
  cpu->WriteMemoryWord(cpu->idata.segment, base_address + 0, idt_limit);
  cpu->WriteMemoryDWord(cpu->idata.segment, base_address + 2, idt_address);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_SGDT(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  cpu->AddCycles(CYCLES_SGDT);

  u32 gdt_address = Truncate32(cpu->m_gdt_location.base_address);
  u16 gdt_limit = Truncate16(cpu->m_gdt_location.limit);

  // 16-bit operand sets higher-order bits to zero
  if (cpu->idata.operand_size == OperandSize_16)
    gdt_address = (gdt_address & 0xFFFFFF);

  // Write back to memory
  CalculateEffectiveAddress<dst_mode>(cpu);
  const VirtualMemoryAddress base_address = cpu->m_effective_address;
  cpu->WriteMemoryWord(cpu->idata.segment, base_address + 0, gdt_limit);
  cpu->WriteMemoryDWord(cpu->idata.segment, base_address + 2, gdt_address);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_SLDT(CPU* cpu)
{
  cpu->AddCyclesRM(CYCLES_SLDT_RM_MEM, cpu->idata.ModRM_RM_IsReg());

  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  CalculateEffectiveAddress<dst_mode>(cpu);
  WriteWordOperand<dst_mode, dst_constant>(cpu, cpu->m_registers.LDTR);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_STR(CPU* cpu)
{
  cpu->AddCyclesRM(CYCLES_STR_RM_MEM, cpu->idata.ModRM_RM_IsReg());

  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  CalculateEffectiveAddress<dst_mode>(cpu);
  WriteWordOperand<dst_mode, dst_constant>(cpu, cpu->m_registers.TR);
}

template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
void Interpreter::Execute_Operation_LMSW(CPU* cpu)
{
  cpu->AddCyclesRM(CYCLES_LMSW_RM_MEM, cpu->idata.ModRM_RM_IsReg());

  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // LMSW cannot clear the PE bit of CR0.
  CalculateEffectiveAddress<src_mode>(cpu);
  const u16 value = ReadWordOperand<src_mode, src_constant>(cpu);
  cpu->LoadSpecialRegister(Reg32_CR0, (cpu->m_registers.CR0 & 0xFFFFFFF1) | ZeroExtend32(value & 0xF));
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_SMSW(CPU* cpu)
{
  cpu->AddCyclesRM(CYCLES_SMSW_RM_MEM, cpu->idata.ModRM_RM_IsReg());

  CalculateEffectiveAddress<dst_mode>(cpu);
  const u16 value = Truncate16(cpu->m_registers.CR0);
  WriteWordOperand<dst_mode, dst_constant>(cpu, value);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant, OperandSize count_size, OperandMode count_mode, u32 count_constant>
void Interpreter::Execute_Operation_SHLD(CPU* cpu)
{
  static_assert(dst_size == src_size && count_size == OperandSize_8, "args are correct size");
  if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_SHLD_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_16)
  {
    u16 value = ReadWordOperand<dst_mode, dst_constant>(cpu);
    u16 shift_in = ReadWordOperand<src_mode, src_constant>(cpu);
    u8 shift_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_count == 0)
      return;

    u32 temp_value1 = ((ZeroExtend32(value) << 16) | ZeroExtend32(shift_in));
    u32 temp_value2 = temp_value1 << shift_count;
    if (shift_count > 16)
      temp_value2 |= (value << (shift_count - 16));

    // temp_value >>= 16;
    u16 new_value = Truncate16(temp_value2 >> 16);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.CF = (((temp_value1 >> (32 - shift_count)) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = (((value ^ new_value) & 0x8000) != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 value = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    u32 shift_in = ReadDWordOperand<src_mode, src_constant>(cpu);
    u8 shift_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_count == 0)
      return;

    u32 new_value = (value << shift_count) | (shift_in >> (32 - shift_count));
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.CF = (((value >> (32 - shift_count)) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = ((BoolToUInt32(cpu->m_registers.EFLAGS.CF) ^ (new_value >> 31)) != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant, OperandSize count_size, OperandMode count_mode, u32 count_constant>
void Interpreter::Execute_Operation_SHRD(CPU* cpu)
{
  static_assert(dst_size == src_size && count_size == OperandSize_8, "args are correct size");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);
  if constexpr (dst_mode == OperandMode_ModRM_RM)
    cpu->AddCyclesRM(CYCLES_SHLD_RM_MEM, cpu->idata.ModRM_RM_IsReg());
  else
    static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_16)
  {
    u16 value = ReadWordOperand<dst_mode, dst_constant>(cpu);
    u16 shift_in = ReadWordOperand<src_mode, src_constant>(cpu);
    u8 shift_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_count == 0)
      return;

    u32 temp_value = ((ZeroExtend32(shift_in) << 16) | ZeroExtend32(value));
    temp_value >>= shift_count;
    if (shift_count > 16)
      temp_value |= (value << (32 - shift_count));

    u16 new_value = Truncate16(temp_value);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.CF = (((value >> (shift_count - 1)) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = (((value ^ new_value) & 0x8000) != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 value = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    u32 shift_in = ReadDWordOperand<src_mode, src_constant>(cpu);
    u8 shift_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_count == 0)
      return;

    u32 new_value = (shift_in << (32 - shift_count)) | (value >> shift_count);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.CF = (((value >> (shift_count - 1)) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = (((value ^ new_value) & UINT32_C(0x80000000)) != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_XADD(CPU* cpu)
{
  cpu->AddCycles(CYCLES_XADD);

  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  // Order is swapped when both src/dst are registers.
  // We have to write the destination first for the memory version though, in case it faults.
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  const bool swap_order = (dst_mode == OperandMode_ModRM_RM && cpu->idata.ModRM_RM_IsReg());

  if (actual_size == OperandSize_8)
  {
    u8 dst = ReadByteOperand<dst_mode, dst_constant>(cpu);
    u8 src = ReadByteOperand<src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u8 tmp = ALUOp_Add8(&eflags, dst, src);
    src = dst;
    dst = tmp;

    if (swap_order)
    {
      WriteByteOperand<src_mode, src_constant>(cpu, src);
      WriteByteOperand<dst_mode, dst_constant>(cpu, dst);
    }
    else
    {
      WriteByteOperand<dst_mode, dst_constant>(cpu, dst);
      WriteByteOperand<src_mode, src_constant>(cpu, src);
    }

    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    u16 dst = ReadWordOperand<dst_mode, dst_constant>(cpu);
    u16 src = ReadWordOperand<src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u16 tmp = ALUOp_Add16(&eflags, dst, src);
    src = dst;
    dst = tmp;
    if (swap_order)
    {
      WriteWordOperand<src_mode, src_constant>(cpu, src);
      WriteWordOperand<dst_mode, dst_constant>(cpu, dst);
    }
    else
    {
      WriteWordOperand<dst_mode, dst_constant>(cpu, dst);
      WriteWordOperand<src_mode, src_constant>(cpu, src);
    }

    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    u32 dst = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    u32 src = ReadDWordOperand<src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    u32 tmp = ALUOp_Add32(&eflags, dst, src);
    src = dst;
    dst = tmp;
    if (swap_order)
    {
      WriteDWordOperand<src_mode, src_constant>(cpu, src);
      WriteDWordOperand<dst_mode, dst_constant>(cpu, dst);
    }
    else
    {
      WriteDWordOperand<dst_mode, dst_constant>(cpu, dst);
      WriteDWordOperand<src_mode, src_constant>(cpu, src);
    }

    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_CMPXCHG(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CMPXCHG);

  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_8)
  {
    const u8 dest = ReadByteOperand<dst_mode, dst_constant>(cpu);
    const u8 source = ReadByteOperand<src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    if (ALUOp_Sub8(&eflags, cpu->m_registers.AL, dest) == 0)
    {
      // ZF should be set by the ALU op
      DebugAssert(eflags & Flag_ZF);
      WriteByteOperand<dst_mode, dst_constant>(cpu, source);
    }
    else
    {
      // ZF should be clear by the ALU op
      // Even if the test passes the write is still issued
      DebugAssert(!(eflags & Flag_ZF));
      WriteByteOperand<dst_mode, dst_constant>(cpu, dest);
      cpu->m_registers.AL = dest;
    }
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_16)
  {
    const u16 dest = ReadWordOperand<dst_mode, dst_constant>(cpu);
    const u16 source = ReadWordOperand<src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    if (ALUOp_Sub16(&eflags, cpu->m_registers.AX, dest) == 0)
    {
      // ZF should be set by the ALU op
      DebugAssert(eflags & Flag_ZF);
      WriteWordOperand<dst_mode, dst_constant>(cpu, source);
    }
    else
    {
      // ZF should be clear by the ALU op
      // Even if the test passes the write is still issued
      DebugAssert(!(eflags & Flag_ZF));
      WriteWordOperand<dst_mode, dst_constant>(cpu, dest);
      cpu->m_registers.AX = dest;
    }
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else if (actual_size == OperandSize_32)
  {
    const u32 dest = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    const u32 source = ReadDWordOperand<src_mode, src_constant>(cpu);
    u32 eflags = cpu->m_registers.EFLAGS.bits;
    if (ALUOp_Sub32(&eflags, cpu->m_registers.EAX, dest) == 0)
    {
      // ZF should be set by the ALU op
      DebugAssert(eflags & Flag_ZF);
      WriteDWordOperand<dst_mode, dst_constant>(cpu, source);
    }
    else
    {
      // ZF should be clear by the ALU op
      // Even if the test passes the write is still issued
      DebugAssert(!(eflags & Flag_ZF));
      WriteDWordOperand<dst_mode, dst_constant>(cpu, dest);
      cpu->m_registers.EAX = dest;
    }
    cpu->m_registers.EFLAGS.bits = eflags;
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize mem_size, OperandMode mem_mode, u32 mem_constant>
void Interpreter::Execute_Operation_CMPXCHG8B(CPU* cpu)
{
  static_assert(mem_size == OperandSize_64, "operands is 64-bit");
  cpu->AddCycles(CYCLES_CMPXCHG8B);

  CalculateEffectiveAddress<mem_mode>(cpu);

  // If r/m is is register, #UD.
  if (cpu->idata.modrm_rm_register)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  u64 temp = ReadQWordOperand<mem_mode, mem_constant>(cpu);
  u64 edx_eax = (ZeroExtend64(cpu->m_registers.EDX) << 32) | ZeroExtend64(cpu->m_registers.EAX);
  if (edx_eax == temp)
  {
    u64 ecx_ebx = (ZeroExtend64(cpu->m_registers.ECX) << 32) | ZeroExtend64(cpu->m_registers.EBX);
    WriteQWordOperand<mem_mode, mem_constant>(cpu, ecx_ebx);
    cpu->m_registers.EFLAGS.ZF = true;
  }
  else
  {
    // NOTE: Memory write occurs regardless, as part of the LOCK bus cycle.
    WriteQWordOperand<mem_mode, mem_constant>(cpu, temp);
    cpu->m_registers.EDX = Truncate32(temp >> 32);
    cpu->m_registers.EAX = Truncate32(temp);
    cpu->m_registers.EFLAGS.ZF = false;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_BSR(CPU* cpu)
{
  // TODO: Timing.
  cpu->AddCycles(CYCLES_BSF_BASE);
  cpu->AddCycles(CYCLES_BSF_N);

  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_16)
  {
    u16 mask = ReadZeroExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    if (mask != 0)
    {
      u16 index = 0;
      Y_bitscanreverse(mask, &index);
      WriteWordOperand<dst_mode, dst_constant>(cpu, Truncate16(index));
      cpu->m_registers.EFLAGS.ZF = false;
    }
    else
    {
      cpu->m_registers.EFLAGS.ZF = true;
    }
  }
  else if (actual_size == OperandSize_32)
  {
    u32 mask = ReadZeroExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    if (mask != 0)
    {
      u32 index = 0;
      Y_bitscanreverse(mask, &index);
      WriteDWordOperand<dst_mode, dst_constant>(cpu, index);
      cpu->m_registers.EFLAGS.ZF = false;
    }
    else
    {
      cpu->m_registers.EFLAGS.ZF = true;
    }
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_BSF(CPU* cpu)
{
  // TODO: Timing.
  cpu->AddCycles(CYCLES_BSF_BASE);
  cpu->AddCycles(CYCLES_BSF_N);

  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_16)
  {
    u16 mask = ReadZeroExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    if (mask != 0)
    {
      u16 index = 0;
      Y_bitscanforward(mask, &index);
      WriteWordOperand<dst_mode, dst_constant>(cpu, Truncate16(index));
      cpu->m_registers.EFLAGS.ZF = false;
    }
    else
    {
      cpu->m_registers.EFLAGS.ZF = true;
    }
  }
  else if (actual_size == OperandSize_32)
  {
    u32 mask = ReadZeroExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    if (mask != 0)
    {
      u32 index = 0;
      Y_bitscanforward(mask, &index);
      WriteDWordOperand<dst_mode, dst_constant>(cpu, index);
      cpu->m_registers.EFLAGS.ZF = false;
    }
    else
    {
      cpu->m_registers.EFLAGS.ZF = true;
    }
  }
}

template<Operation operation, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size,
         OperandMode src_mode, u32 src_constant>
void Interpreter::Execute_Operation_BTx(CPU* cpu)
{
  static_assert(dst_mode == OperandMode_ModRM_RM, "dst_mode is modrm r/m");
  if constexpr (src_mode == OperandMode_Immediate)
  {
    cpu->AddCyclesRM((operation == Operation_BT) ? CYCLES_BT_RM_MEM_IMM : CYCLES_BTx_RM_MEM_IMM,
                     cpu->idata.ModRM_RM_IsReg());
  }
  else
  {
    cpu->AddCyclesRM((operation == Operation_BT) ? CYCLES_BT_RM_MEM_REG : CYCLES_BTx_RM_MEM_REG,
                     cpu->idata.ModRM_RM_IsReg());
  }

  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  // When combined with a memory operand, these instructions can access more than 16/32 bits.
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  const bool is_register_operand = cpu->idata.modrm_rm_register;
  if (actual_size == OperandSize_16)
  {
    u16 source = ReadZeroExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    u16 bit = source & 0xF;
    u16 mask = (UINT16_C(1) << bit);

    u16 in_value, out_value;
    VirtualMemoryAddress effective_address = 0;
    if (is_register_operand)
    {
      // Ignore displacement.
      DebugAssert(cpu->m_effective_address < Reg16_Count);
      in_value = cpu->m_registers.reg16[cpu->m_effective_address];
    }
    else
    {
      // Displacement can be signed, annoyingly, so we need to divide rather than shift.
      s16 displacement = s16(source & 0xFFF0) / 16;
      effective_address = cpu->m_effective_address;
      effective_address += SignExtend<VirtualMemoryAddress>(displacement) * 2;
      if (cpu->idata.address_size == AddressSize_16)
        effective_address &= 0xFFFF;

      in_value = cpu->ReadMemoryWord(cpu->idata.segment, effective_address);
    }

    // Output value depends on operation, since we share this handler for multiple operations
    switch (operation)
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
        cpu->m_registers.reg16[cpu->m_effective_address] = out_value;
      else
        cpu->WriteMemoryWord(cpu->idata.segment, effective_address, out_value);
    }

    // CF flag depends on whether the bit was set in the input value
    cpu->m_registers.EFLAGS.CF = ((in_value & mask) != 0);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 source = ReadZeroExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    u32 bit = source & 0x1F;
    u32 mask = (UINT32_C(1) << bit);

    u32 in_value, out_value;
    VirtualMemoryAddress effective_address = 0;
    if (is_register_operand)
    {
      // Ignore displacement.
      DebugAssert(cpu->m_effective_address < Reg32_Count);
      in_value = cpu->m_registers.reg32[cpu->m_effective_address];
    }
    else
    {
      // Displacement can be signed, annoyingly, so we need to divide rather than shift.
      s32 displacement = s32(source & 0xFFFFFFE0) / 32;
      effective_address = cpu->m_effective_address;
      effective_address += SignExtend<VirtualMemoryAddress>(displacement) * 4;
      if (cpu->idata.address_size == AddressSize_16)
        effective_address &= 0xFFFF;

      in_value = cpu->ReadMemoryDWord(cpu->idata.segment, effective_address);
    }

    // Output value depends on operation, since we share this handler for multiple operations
    switch (operation)
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
        cpu->m_registers.reg32[cpu->m_effective_address] = out_value;
      else
        cpu->WriteMemoryDWord(cpu->idata.segment, effective_address, out_value);
    }

    // CF flag depends on whether the bit was set in the input value
    cpu->m_registers.EFLAGS.CF = ((in_value & mask) != 0);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_BTC(CPU* cpu)
{
  Execute_Operation_BTx<Operation_BTC, dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_BTR(CPU* cpu)
{
  Execute_Operation_BTx<Operation_BTR, dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_BTS(CPU* cpu)
{
  Execute_Operation_BTx<Operation_BTS, dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_BT(CPU* cpu)
{
  Execute_Operation_BTx<Operation_BT, dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
}

template<Operation operation, bool check_equal, typename callback>
void Interpreter::Execute_REP(CPU* cpu, callback cb)
{
  const bool has_rep = cpu->idata.has_rep;
  if constexpr (operation == Operation_CMPS)
    cpu->AddCycles((has_rep) ? CYCLES_REP_CMPS_BASE : CYCLES_CMPS);
  else if constexpr (operation == Operation_INS)
    cpu->AddCyclesPMode((has_rep) ? CYCLES_REP_INS_BASE : CYCLES_INS);
  else if constexpr (operation == Operation_LODS)
    cpu->AddCycles((has_rep) ? CYCLES_REP_LODS_BASE : CYCLES_LODS);
  else if constexpr (operation == Operation_MOVS)
    cpu->AddCycles((has_rep) ? CYCLES_REP_MOVS_BASE : CYCLES_MOVS);
  else if constexpr (operation == Operation_OUTS)
    cpu->AddCyclesPMode((has_rep) ? CYCLES_REP_OUTS_BASE : CYCLES_OUTS);
  else if constexpr (operation == Operation_SCAS)
    cpu->AddCycles((has_rep) ? CYCLES_REP_SCAS_BASE : CYCLES_SCAS);
  else if constexpr (operation == Operation_STOS)
    cpu->AddCycles((has_rep) ? CYCLES_REP_STOS_BASE : CYCLES_STOS);

  // IN/OUT are often timing-sensitive.
  if constexpr (operation == Operation_INS || operation == Operation_OUTS)
    cpu->CommitPendingCycles();

  // We can execute this instruction as a non-rep.
  if (!has_rep)
  {
    cb(cpu);
    return;
  }

  for (;;)
  {
    if constexpr (operation == Operation_CMPS)
      cpu->AddCycles(CYCLES_REP_CMPS_N);
    else if constexpr (operation == Operation_INS)
      cpu->AddCyclesPMode(CYCLES_REP_INS_N);
    else if constexpr (operation == Operation_LODS)
      cpu->AddCycles(CYCLES_REP_LODS_N);
    else if constexpr (operation == Operation_MOVS)
      cpu->AddCycles(CYCLES_REP_MOVS_N);
    else if constexpr (operation == Operation_OUTS)
      cpu->AddCyclesPMode(CYCLES_REP_OUTS_N);
    else if constexpr (operation == Operation_SCAS)
      cpu->AddCycles(CYCLES_REP_SCAS_N);
    else if constexpr (operation == Operation_STOS)
      cpu->AddCycles(CYCLES_REP_STOS_N);

    // Only the counter is checked at the beginning.
    if (cpu->idata.address_size == AddressSize_16)
    {
      if (cpu->m_registers.CX == 0)
        return;
    }
    else
    {
      if (cpu->m_registers.ECX == 0)
        return;
    }

    // Execute the actual instruction.
    cb(cpu);

    // Decrement the count register after the operation.
    bool branch = true;
    if (cpu->idata.address_size == AddressSize_16)
      branch = (--cpu->m_registers.CX != 0);
    else
      branch = (--cpu->m_registers.ECX != 0);

    // Finally test the post-condition.
    if constexpr (check_equal)
    {
      if (!cpu->idata.has_repne)
        branch &= TestJumpCondition<JumpCondition_Equal>(cpu);
      else
        branch &= TestJumpCondition<JumpCondition_NotEqual>(cpu);
    }

    // Try to batch REP instructions together for speed.
    if (!branch)
      return;

#if 0
    // Add a cycle, for the next byte. This could cause an interrupt.
    // This way long-running REP instructions will be paused mid-way.
    cpu->CommitPendingCycles();

    // Interrupts should apparently be checked before the instruction.
    // We check them before dispatching, but since we're looping around, do it again.
    if (cpu->HasExternalInterrupt())
    {
      // If the interrupt line gets signaled, we need the return address set to the REP instruction.
      cpu->RestartCurrentInstruction();
      cpu->AbortCurrentInstruction();
      return;
    }
#endif

    // Add an extra instruction since we're looping around again.
    cpu->AddCycle();
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_INS(CPU* cpu)
{
  // TODO: Move the port number check out of the loop.
  Execute_REP<Operation_INS, false>(cpu, [](CPU* cpu) {
    const VirtualMemoryAddress dst_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    const u16 port_number = cpu->m_registers.DX;
    u8 data_size;

    if (actual_size == OperandSize_8)
    {
      if (!cpu->HasIOPermissions(port_number, sizeof(u8), true))
      {
        cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
        return;
      }

      cpu->CommitPendingCycles();

      const u8 value = cpu->m_bus->ReadIOPortByte(port_number);
      cpu->WriteMemoryByte(Segment_ES, dst_address, value);
      data_size = sizeof(u8);
    }
    else if (actual_size == OperandSize_16)
    {
      if (!cpu->HasIOPermissions(port_number, sizeof(u16), true))
      {
        cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
        return;
      }

      cpu->CommitPendingCycles();

      const u16 value = cpu->m_bus->ReadIOPortWord(port_number);
      cpu->WriteMemoryWord(Segment_ES, dst_address, value);
      data_size = sizeof(u16);
    }
    else if (actual_size == OperandSize_32)
    {
      if (!cpu->HasIOPermissions(port_number, sizeof(u32), true))
      {
        cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
        return;
      }

      cpu->CommitPendingCycles();

      const u32 value = cpu->m_bus->ReadIOPortDWord(port_number);
      cpu->WriteMemoryDWord(Segment_ES, dst_address, value);
      data_size = sizeof(u32);
    }
    else
    {
      DebugUnreachableCode();
      return;
    }

    if (cpu->idata.address_size == AddressSize_16)
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
  });
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_OUTS(CPU* cpu)
{
  Execute_REP<Operation_OUTS, false>(cpu, [](CPU* cpu) {
    const Segment segment = cpu->idata.segment;
    const VirtualMemoryAddress src_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
    const OperandSize actual_size = (src_size == OperandSize_Count) ? cpu->idata.operand_size : src_size;
    u16 port_number = cpu->m_registers.DX;
    u8 data_size;

    if (actual_size == OperandSize_8)
    {
      if (!cpu->HasIOPermissions(port_number, sizeof(u8), true))
      {
        cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
        return;
      }

      cpu->CommitPendingCycles();

      u8 value = cpu->ReadMemoryByte(segment, src_address);
      cpu->m_bus->WriteIOPortByte(port_number, value);
      data_size = sizeof(u8);
    }
    else if (actual_size == OperandSize_16)
    {
      if (!cpu->HasIOPermissions(port_number, sizeof(u16), true))
      {
        cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
        return;
      }

      cpu->CommitPendingCycles();

      u16 value = cpu->ReadMemoryWord(segment, src_address);
      cpu->m_bus->WriteIOPortWord(port_number, value);
      data_size = sizeof(u16);
    }
    else if (actual_size == OperandSize_32)
    {
      if (!cpu->HasIOPermissions(port_number, sizeof(u32), true))
      {
        cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
        return;
      }

      cpu->CommitPendingCycles();

      u32 value = cpu->ReadMemoryDWord(segment, src_address);
      cpu->m_bus->WriteIOPortDWord(port_number, value);
      data_size = sizeof(u32);
    }
    else
    {
      DebugUnreachableCode();
      return;
    }

    if (cpu->idata.address_size == AddressSize_16)
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
  });
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_SCAS(CPU* cpu)
{
  static_assert(src_size == dst_size, "operand sizes are the same");
  Execute_REP<Operation_SCAS, true>(cpu, [](CPU* cpu) {
    // The ES segment cannot be overridden with a segment override prefix.
    VirtualMemoryAddress dst_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    u8 data_size;
    if (actual_size == OperandSize_8)
    {
      const u8 lhs = cpu->m_registers.AL;
      const u8 rhs = cpu->ReadMemoryByte(Segment_ES, dst_address);
      ALUOp_Sub8(&cpu->m_registers.EFLAGS.bits, lhs, rhs);
      data_size = sizeof(u8);
    }
    else if (actual_size == OperandSize_16)
    {
      const u16 lhs = cpu->m_registers.AX;
      const u16 rhs = cpu->ReadMemoryWord(Segment_ES, dst_address);
      ALUOp_Sub16(&cpu->m_registers.EFLAGS.bits, lhs, rhs);
      data_size = sizeof(u16);
    }
    else if (actual_size == OperandSize_32)
    {
      const u32 lhs = cpu->m_registers.EAX;
      const u32 rhs = cpu->ReadMemoryDWord(Segment_ES, dst_address);
      ALUOp_Sub32(&cpu->m_registers.EFLAGS.bits, lhs, rhs);
      data_size = sizeof(u32);
    }
    else
    {
      DebugUnreachableCode();
      return;
    }

    if (cpu->idata.address_size == AddressSize_16)
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
  });
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_LODS(CPU* cpu)
{
  static_assert(src_size == dst_size, "operand sizes are the same");
  Execute_REP<Operation_LODS, false>(cpu, [](CPU* cpu) {
    const Segment segment = cpu->idata.segment;
    const VirtualMemoryAddress src_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    u8 data_size;

    if (actual_size == OperandSize_8)
    {
      u8 value = cpu->ReadMemoryByte(segment, src_address);
      cpu->m_registers.AL = value;
      data_size = sizeof(u8);
    }
    else if (actual_size == OperandSize_16)
    {
      u16 value = cpu->ReadMemoryWord(segment, src_address);
      cpu->m_registers.AX = value;
      data_size = sizeof(u16);
    }
    else if (actual_size == OperandSize_32)
    {
      u32 value = cpu->ReadMemoryDWord(segment, src_address);
      cpu->m_registers.EAX = value;
      data_size = sizeof(u32);
    }
    else
    {
      DebugUnreachableCode();
      return;
    }

    if (cpu->idata.address_size == AddressSize_16)
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
  });
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_STOS(CPU* cpu)
{
  static_assert(src_size == dst_size, "operand sizes are the same");
  Execute_REP<Operation_STOS, false>(cpu, [](CPU* cpu) {
    const VirtualMemoryAddress dst_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    u8 data_size;

    if (actual_size == OperandSize_8)
    {
      u8 value = cpu->m_registers.AL;
      cpu->WriteMemoryByte(Segment_ES, dst_address, value);
      data_size = sizeof(u8);
    }
    else if (actual_size == OperandSize_16)
    {
      u16 value = cpu->m_registers.AX;
      cpu->WriteMemoryWord(Segment_ES, dst_address, value);
      data_size = sizeof(u16);
    }
    else if (actual_size == OperandSize_32)
    {
      u32 value = cpu->m_registers.EAX;
      cpu->WriteMemoryDWord(Segment_ES, dst_address, value);
      data_size = sizeof(u32);
    }
    else
    {
      DebugUnreachableCode();
      return;
    }

    if (cpu->idata.address_size == AddressSize_16)
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
  });
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_CMPS(CPU* cpu)
{
  static_assert(src_size == dst_size, "operand sizes are the same");
  Execute_REP<Operation_CMPS, true>(cpu, [](CPU* cpu) {
    // The DS segment may be overridden with a segment override prefix, but the ES segment cannot be overridden.
    Segment src_segment = cpu->idata.segment;
    VirtualMemoryAddress src_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
    VirtualMemoryAddress dst_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    u8 data_size;

    if (actual_size == OperandSize_8)
    {
      const u8 lhs = cpu->ReadMemoryByte(src_segment, src_address);
      const u8 rhs = cpu->ReadMemoryByte(Segment_ES, dst_address);
      ALUOp_Sub8(&cpu->m_registers.EFLAGS.bits, lhs, rhs);
      data_size = sizeof(u8);
    }
    else if (actual_size == OperandSize_16)
    {
      const u16 lhs = cpu->ReadMemoryWord(src_segment, src_address);
      const u16 rhs = cpu->ReadMemoryWord(Segment_ES, dst_address);
      ALUOp_Sub16(&cpu->m_registers.EFLAGS.bits, lhs, rhs);
      data_size = sizeof(u16);
    }
    else if (actual_size == OperandSize_32)
    {
      const u32 lhs = cpu->ReadMemoryDWord(src_segment, src_address);
      const u32 rhs = cpu->ReadMemoryDWord(Segment_ES, dst_address);
      ALUOp_Sub32(&cpu->m_registers.EFLAGS.bits, lhs, rhs);
      data_size = sizeof(u32);
    }
    else
    {
      DebugUnreachableCode();
      return;
    }

    if (cpu->idata.address_size == AddressSize_16)
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
  });
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_MOVS(CPU* cpu)
{
  static_assert(src_size == dst_size, "operand sizes are the same");
  Execute_REP<Operation_MOVS, false>(cpu, [](CPU* cpu) {
    // The DS segment may be over-ridden with a segment override prefix, but the ES segment cannot be overridden.
    const Segment src_segment = cpu->idata.segment;
    const VirtualMemoryAddress src_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
    const VirtualMemoryAddress dst_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    u8 data_size;

    if (actual_size == OperandSize_8)
    {
      u8 value = cpu->ReadMemoryByte(src_segment, src_address);
      cpu->WriteMemoryByte(Segment_ES, dst_address, value);
      data_size = sizeof(u8);
    }
    else if (actual_size == OperandSize_16)
    {
      u16 value = cpu->ReadMemoryWord(src_segment, src_address);
      cpu->WriteMemoryWord(Segment_ES, dst_address, value);
      data_size = sizeof(u16);
    }
    else if (actual_size == OperandSize_32)
    {
      u32 value = cpu->ReadMemoryDWord(src_segment, src_address);
      cpu->WriteMemoryDWord(Segment_ES, dst_address, value);
      data_size = sizeof(u32);
    }
    else
    {
      DebugUnreachableCode();
      return;
    }

    if (cpu->idata.address_size == AddressSize_16)
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
  });
}

template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size,
         OperandMode src_mode, u32 src_constant>
void Interpreter::Execute_Operation_CMOVcc(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  cpu->AddCycles(CYCLES_CMOV);

  CalculateEffectiveAddress<src_mode>(cpu);
  CalculateEffectiveAddress<dst_mode>(cpu);

  // NOTE: Memory access is performed even if the predicate does not hold.
  bool do_move = TestJumpCondition<condition>(cpu);
  if (actual_size == OperandSize_16)
  {
    u16 value = ReadWordOperand<src_mode, src_constant>(cpu);
    if (do_move)
      WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_32)
  {
    u32 value = ReadDWordOperand<src_mode, src_constant>(cpu);
    if (do_move)
      WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
}

template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
void Interpreter::Execute_Operation_SETcc(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  cpu->AddCyclesRM(CYCLES_SETcc_RM_MEM, cpu->idata.ModRM_RM_IsReg());

  bool flag = TestJumpCondition<condition>(cpu);
  WriteByteOperand<dst_mode, dst_constant>(cpu, BoolToUInt8(flag));
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_MOV_TR(CPU* cpu)
{
  static_assert((src_mode == OperandMode_ModRM_TestRegister && dst_mode == OperandMode_ModRM_RM) ||
                  (src_mode == OperandMode_ModRM_RM && dst_mode == OperandMode_ModRM_TestRegister),
                "loading or storing debug register");
  static_assert(src_size == OperandSize_32 && dst_size == OperandSize_32, "source sizes are 32-bits");

  // TODO: Timing
  cpu->AddCycle();

  // Requires privilege level zero
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // Load test register
  u8 tr_index = cpu->idata.GetModRM_Reg();
  if constexpr (dst_mode == OperandMode_ModRM_TestRegister)
  {
    u32 value = cpu->m_registers.reg32[cpu->idata.GetModRM_RM()];

    // Validate selected register
    switch (tr_index)
    {
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
        // Only load to registers 3-7
        cpu->LoadSpecialRegister(static_cast<Reg32>(Reg32_TR3 + (tr_index - 3)), value);
        break;

      default:
        cpu->RaiseException(Interrupt_InvalidOpcode);
        return;
    }
  }
  // Store test register
  if constexpr (src_mode == OperandMode_ModRM_TestRegister)
  {
    // Validate selected register
    u32 value;
    switch (tr_index)
    {
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
        value = cpu->m_registers.reg32[Reg32_TR3 + (tr_index - 3)];
        break;

      default:
        cpu->RaiseException(Interrupt_InvalidOpcode);
        return;
    }

    cpu->m_registers.reg32[cpu->idata.GetModRM_RM()] = value;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_MOV_DR(CPU* cpu)
{
  static_assert((src_mode == OperandMode_ModRM_DebugRegister && dst_mode == OperandMode_ModRM_RM) ||
                  (src_mode == OperandMode_ModRM_RM && dst_mode == OperandMode_ModRM_DebugRegister),
                "loading or storing debug register");
  static_assert(src_size == OperandSize_32 && dst_size == OperandSize_32, "source sizes are 32-bits");

  // Requires privilege level zero
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // TODO Validation:
  // #UD If CR4.DE[bit 3] = 1 (debug extensions) and a MOV instruction is executed involving DR4 or DR5.
  // #DB If any debug register is accessed while the DR7.GD[bit 13] = 1.

  const u8 dr_index = cpu->idata.GetModRM_Reg();
  if constexpr (dst_mode == OperandMode_ModRM_DebugRegister)
  {
    // Load debug register
    cpu->AddCycles((dr_index <= 3) ? CYCLES_MOV_DR0_3_REG : CYCLES_MOV_DR6_7_REG);

    u32 value = cpu->m_registers.reg32[cpu->idata.GetModRM_RM()];
    cpu->LoadSpecialRegister(static_cast<Reg32>(Reg32_DR0 + dr_index), value);
  }
  else if constexpr (src_mode == OperandMode_ModRM_DebugRegister)
  {
    // Store debug register
    cpu->AddCycles((dr_index <= 3) ? CYCLES_MOV_REG_DR0_3 : CYCLES_MOV_REG_DR6_7);

    u32 value = cpu->m_registers.reg32[Reg32_DR0 + dr_index];
    cpu->m_registers.reg32[cpu->idata.GetModRM_RM()] = value;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
         u32 src_constant>
void Interpreter::Execute_Operation_MOV_CR(CPU* cpu)
{
  static_assert((src_mode == OperandMode_ModRM_ControlRegister && dst_mode == OperandMode_ModRM_RM) ||
                  (src_mode == OperandMode_ModRM_RM && dst_mode == OperandMode_ModRM_ControlRegister),
                "loading or storing control register");
  static_assert(src_size == OperandSize_32 && dst_size == OperandSize_32, "source sizes are 32-bits");

  // Requires privilege level zero
  if (cpu->GetCPL() != 0)
  {
    cpu->AddCycle();
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // Load control register
  if constexpr (dst_mode == OperandMode_ModRM_ControlRegister)
  {
    // Validate selected register
    u32 value = cpu->m_registers.reg32[cpu->idata.GetModRM_RM()];
    switch (cpu->idata.modrm_reg)
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

        cpu->AddCycles(CYCLES_MOV_CR0_REG);
        cpu->LoadSpecialRegister(Reg32_CR0, value);
      }
      break;

      case 2:
        cpu->AddCycles(CYCLES_MOV_CR2_REG);
        cpu->LoadSpecialRegister(Reg32_CR2, value);
        break;

      case 3:
        cpu->AddCycles(CYCLES_MOV_CR3_REG);
        cpu->LoadSpecialRegister(Reg32_CR3, value);
        break;

      case 4:
      {
        // TODO: Validate reserved bits in CR4, GP(0) if any are set to 1.
        // cpu->AddCycles(CYCLES_MOV_CR4_REG);
        cpu->LoadSpecialRegister(Reg32_CR4, value);
      }
      break;

      default:
        cpu->RaiseException(Interrupt_InvalidOpcode);
        return;
    }
  }
  // Store control register
  else if constexpr (src_mode == OperandMode_ModRM_ControlRegister)
  {
    // Validate selected register
    u32 value;
    switch (cpu->idata.modrm_reg)
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
        value = cpu->m_registers.CR4.bits;
        break;
      default:
        cpu->RaiseException(Interrupt_InvalidOpcode);
        return;
    }

    cpu->m_registers.reg32[cpu->idata.GetModRM_RM()] = value;
    cpu->AddCycles(CYCLES_MOV_REG_CR);
  }
}

void Interpreter::Execute_Operation_INVD(CPU* cpu)
{
  cpu->AddCycles(CYCLES_INVD);
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  if (cpu->idata.has_lock)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode, 0);
    return;
  }
}

void Interpreter::Execute_Operation_WBINVD(CPU* cpu)
{
  cpu->AddCycles(CYCLES_INVD);
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  if (cpu->idata.has_lock)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode, 0);
    return;
  }

  // Log_WarningPrintf("WBINVD instruction");
}

void Interpreter::Execute_Operation_CPUID(CPU* cpu)
{
  cpu->AddCycles(CYCLES_CPUID);

  // TODO: Support on Intel DX4+.
  if (cpu->m_model < MODEL_PENTIUM)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  cpu->ExecuteCPUIDInstruction();
}

void Interpreter::Execute_Operation_RDTSC(CPU* cpu)
{
  cpu->AddCycles(CYCLES_RDTSC);

  // TODO: Support on Intel DX4+.
  if (cpu->m_model < MODEL_PENTIUM)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  // TSD flag in CR4 controls whether this is privileged or unprivileged
  if (cpu->m_registers.CR4.TSD && cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault);
    return;
  }

  const u64 tsc = cpu->ReadTSC();
  cpu->m_registers.EAX = Truncate32(tsc);
  cpu->m_registers.EDX = Truncate32(tsc >> 32);
}

void Interpreter::Execute_Operation_RDMSR(CPU* cpu)
{
  if (cpu->m_model < MODEL_PENTIUM)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  cpu->AddCycles(CYCLES_RDMSR);

  // Must be in CPL=0, and not in V8086 mode. V8086 mode implies CPL=3, so no need to check explicitly.
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  const u64 value = cpu->ReadMSR(cpu->m_registers.ECX);
  cpu->m_registers.EAX = Truncate32(value);
  cpu->m_registers.EDX = Truncate32(value >> 32);
}

void Interpreter::Execute_Operation_WRMSR(CPU* cpu)
{
  if (cpu->m_model < MODEL_PENTIUM)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  cpu->AddCycles(CYCLES_RDMSR);

  // Must be in CPL=0, and not in V8086 mode. V8086 mode implies CPL=3, so no need to check explicitly.
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  const u64 value = (ZeroExtend64(cpu->m_registers.EDX) << 32) | ZeroExtend64(cpu->m_registers.EAX);
  cpu->WriteMSR(cpu->m_registers.ECX, value);
}

void Interpreter::Execute_Operation_RSM(CPU* cpu)
{
  Panic("RSM instruction");
  cpu->RaiseException(Interrupt_InvalidOpcode);
}
} // namespace CPU_X86

#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif
