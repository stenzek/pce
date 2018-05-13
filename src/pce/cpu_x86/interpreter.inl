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
void NewInterpreter::RaiseInvalidOpcode(CPU* cpu)
{
  cpu->PrintCurrentStateAndInstruction("Invalid opcode raised at ");
  cpu->RaiseException(Interrupt_InvalidOpcode);
}

template<OperandMode op_mode>
void NewInterpreter::CalculateEffectiveAddress(CPU* cpu)
{
  switch (op_mode)
  {
    case OperandMode_ModRM_RM:
    {
      // NOTE: The uint16() cast here is needed otherwise the result is an int rather than uint16.
      uint8 index = ((cpu->idata.modrm >> 6) << 3) | (cpu->idata.modrm & 7);
      if (cpu->idata.address_size == AddressSize_16)
      {
        switch (index & 31)
        {
          case 0:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.BX + cpu->m_registers.SI));
            break;
          case 1:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.BX + cpu->m_registers.DI));
            break;
          case 2:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.BP + cpu->m_registers.SI));
            break;
          case 3:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.BP + cpu->m_registers.DI));
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
            cpu->m_effective_address =
              ZeroExtend32(uint16(cpu->m_registers.BX + cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 9:
            cpu->m_effective_address =
              ZeroExtend32(uint16(cpu->m_registers.BX + cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 10:
            cpu->m_effective_address =
              ZeroExtend32(uint16(cpu->m_registers.BP + cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 11:
            cpu->m_effective_address =
              ZeroExtend32(uint16(cpu->m_registers.BP + cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 12:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 13:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 14:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.BP + cpu->idata.disp16));
            break;
          case 15:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.BX + cpu->idata.disp16));
            break;
          case 16:
            cpu->m_effective_address =
              ZeroExtend32(uint16(cpu->m_registers.BX + cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 17:
            cpu->m_effective_address =
              ZeroExtend32(uint16(cpu->m_registers.BX + cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 18:
            cpu->m_effective_address =
              ZeroExtend32(uint16(cpu->m_registers.BP + cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 19:
            cpu->m_effective_address =
              ZeroExtend32(uint16(cpu->m_registers.BP + cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 20:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.SI + cpu->idata.disp16));
            break;
          case 21:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.DI + cpu->idata.disp16));
            break;
          case 22:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.BP + cpu->idata.disp16));
            break;
          case 23:
            cpu->m_effective_address = ZeroExtend32(uint16(cpu->m_registers.BX + cpu->idata.disp16));
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
            const uint32 base_addr =
              cpu->idata.HasSIBBase() ? cpu->m_registers.reg32[cpu->idata.GetSIBBaseRegister()] : 0;
            const uint32 index_addr =
              cpu->idata.HasSIBIndex() ? cpu->m_registers.reg32[cpu->idata.GetSIBIndexRegister()] : 0;
            const uint8 scaling_factor = cpu->idata.GetSIBScaling();
            const uint32 displacement = cpu->idata.disp32;
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
VirtualMemoryAddress CPU_X86::NewInterpreter::CalculateJumpTarget(CPU* cpu)
{
  static_assert(dst_mode == OperandMode_Relative || dst_mode == OperandMode_ModRM_RM,
                "Operand mode is relative or indirect");

  if constexpr (dst_mode == OperandMode_Relative)
  {
    if (cpu->idata.operand_size == OperandSize_16)
    {
      // TODO: Should this be extended to addressing mode?
      uint16 address = Truncate16(Truncate16(cpu->m_registers.EIP) + cpu->idata.disp16);
      // address &= cpu->m_EIP_mask;
      return ZeroExtend32(address);
    }
    else
    {
      uint32 address = cpu->m_registers.EIP + cpu->idata.disp32;
      return address;
    }
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

template<OperandMode mode, uint32 constant>
uint8 NewInterpreter::ReadByteOperand(CPU* cpu)
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

template<OperandMode mode, uint32 constant>
uint16 NewInterpreter::ReadWordOperand(CPU* cpu)
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

template<OperandMode mode, uint32 constant>
uint32 NewInterpreter::ReadDWordOperand(CPU* cpu)
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

template<OperandSize size, OperandMode mode, uint32 constant>
uint16 CPU_X86::NewInterpreter::ReadSignExtendedWordOperand(CPU* cpu)
{
  const OperandSize actual_size = (size == OperandSize_Count) ? cpu->idata.operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      uint8 value;
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

template<OperandSize size, OperandMode mode, uint32 constant>
uint32 CPU_X86::NewInterpreter::ReadSignExtendedDWordOperand(CPU* cpu)
{
  const OperandSize actual_size = (size == OperandSize_Count) ? cpu->idata.operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      uint8 value;
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
      uint16 value;
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

template<OperandSize size, OperandMode mode, uint32 constant>
uint16 CPU_X86::NewInterpreter::ReadZeroExtendedWordOperand(CPU* cpu)
{
  const OperandSize actual_size = (size == OperandSize_Count) ? cpu->idata.operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      uint8 value;
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

template<OperandSize size, OperandMode mode, uint32 constant>
uint32 CPU_X86::NewInterpreter::ReadZeroExtendedDWordOperand(CPU* cpu)
{
  const OperandSize actual_size = (size == OperandSize_Count) ? cpu->idata.operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      uint8 value;
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
      uint16 value;
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

template<OperandMode mode, uint32 constant>
void CPU_X86::NewInterpreter::WriteByteOperand(CPU* cpu, uint8 value)
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

template<OperandMode mode, uint32 constant>
void CPU_X86::NewInterpreter::WriteWordOperand(CPU* cpu, uint16 value)
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

template<OperandMode mode, uint32 constant>
void CPU_X86::NewInterpreter::WriteDWordOperand(CPU* cpu, uint32 value)
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
void CPU_X86::NewInterpreter::ReadFarAddressOperand(CPU* cpu, OperandSize size, uint16* segment_selector,
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

template<JumpCondition condition>
bool CPU_X86::NewInterpreter::TestJumpCondition(CPU* cpu)
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
constexpr bool IsAdjust(T old_value, T new_value)
{
  return (old_value & 0xF) < (new_value & 0xF);
}

#ifdef Y_COMPILER_MSVC
template<typename T>
constexpr bool IsParity(T value)
{
  return static_cast<bool>(~_mm_popcnt_u32(static_cast<uint32>(value & 0xFF)) & 1);
}
#else
template<typename T>
constexpr bool IsParity(T value)
{
  return static_cast<bool>(~Y_popcnt(static_cast<uint8>(value & 0xFF)) & 1);
}
#endif

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

inline uint8 ALUOp_Add8(CPU::Registers* registers, uint8 lhs, uint8 rhs)
{
  uint16 old_value = lhs;
  uint16 add_value = rhs;
  uint16 new_value = old_value + add_value;
  uint8 out_value = uint8(new_value & 0xFF);

  SET_FLAG(registers, CF, ((new_value & 0xFF00) != 0));
  SET_FLAG(registers, OF, ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x80) == 0x80));
  SET_FLAG(registers, AF, (((old_value ^ add_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

inline uint8 ALUOp_Adc8(CPU::Registers* registers, uint8 lhs, uint8 rhs)
{
  uint16 old_value = lhs;
  uint16 add_value = rhs;
  uint16 carry_in = (registers->EFLAGS.CF) ? 1 : 0;
  uint16 new_value = old_value + add_value + carry_in;
  uint8 out_value = uint8(new_value & 0xFF);

  SET_FLAG(registers, CF, ((new_value & 0xFF00) != 0));
  SET_FLAG(registers, OF, ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x80) == 0x80));
  SET_FLAG(registers, AF, (((old_value ^ add_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

inline uint8 ALUOp_Sub8(CPU::Registers* registers, uint8 lhs, uint8 rhs)
{
  uint16 old_value = lhs;
  uint16 sub_value = rhs;
  uint16 new_value = old_value - sub_value;
  uint8 out_value = uint8(new_value & 0xFF);

  SET_FLAG(registers, CF, ((new_value & 0xFF00) != 0));
  SET_FLAG(registers, OF, ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x80) == 0x80));
  SET_FLAG(registers, AF, (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

inline uint8 ALUOp_Sbb8(CPU::Registers* registers, uint8 lhs, uint8 rhs)
{
  uint16 old_value = lhs;
  uint16 sub_value = rhs;
  uint16 carry_in = registers->EFLAGS.CF ? 1 : 0;
  uint16 new_value = old_value - sub_value - carry_in;
  uint8 out_value = uint8(new_value & 0xFF);

  SET_FLAG(registers, CF, ((new_value & 0xFF00) != 0));
  SET_FLAG(registers, OF, ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x80) == 0x80));
  SET_FLAG(registers, AF, (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

inline uint16 ALUOp_Add16(CPU::Registers* registers, uint16 lhs, uint16 rhs)
{
  uint32 old_value = lhs;
  uint32 add_value = rhs;
  uint32 new_value = old_value + add_value;
  uint16 out_value = uint16(new_value & 0xFFFF);

  SET_FLAG(registers, CF, ((new_value & 0xFFFF0000) != 0));
  SET_FLAG(registers, OF, ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x8000) == 0x8000));
  SET_FLAG(registers, AF, (((old_value ^ add_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

inline uint16 ALUOp_Adc16(CPU::Registers* registers, uint16 lhs, uint16 rhs)
{
  uint32 old_value = lhs;
  uint32 add_value = rhs;
  uint32 carry_in = (registers->EFLAGS.CF) ? 1 : 0;
  uint32 new_value = old_value + add_value + carry_in;
  uint16 out_value = uint16(new_value & 0xFFFF);

  SET_FLAG(registers, CF, ((new_value & 0xFFFF0000) != 0));
  SET_FLAG(registers, OF, ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x8000) == 0x8000));
  SET_FLAG(registers, AF, (((old_value ^ add_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

inline uint16 ALUOp_Sub16(CPU::Registers* registers, uint16 lhs, uint16 rhs)
{
  uint32 old_value = lhs;
  uint32 sub_value = rhs;
  uint32 new_value = old_value - sub_value;
  uint16 out_value = uint16(new_value & 0xFFFF);

  SET_FLAG(registers, CF, ((new_value & 0xFFFF0000) != 0));
  SET_FLAG(registers, OF, ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x8000) == 0x8000));
  SET_FLAG(registers, AF, (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

inline uint16 ALUOp_Sbb16(CPU::Registers* registers, uint16 lhs, uint16 rhs)
{
  uint32 old_value = lhs;
  uint32 sub_value = rhs;
  uint32 carry_in = registers->EFLAGS.CF ? 1 : 0;
  uint32 new_value = old_value - sub_value - carry_in;
  uint16 out_value = uint16(new_value & 0xFFFF);

  SET_FLAG(registers, CF, ((new_value & 0xFFFF0000) != 0));
  SET_FLAG(registers, OF, ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x8000) == 0x8000));
  SET_FLAG(registers, AF, (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

inline uint32 ALUOp_Add32(CPU::Registers* registers, uint32 lhs, uint32 rhs)
{
  uint64 old_value = ZeroExtend64(lhs);
  uint64 add_value = ZeroExtend64(rhs);
  uint64 new_value = old_value + add_value;
  uint32 out_value = Truncate32(new_value);

  SET_FLAG(registers, CF, ((new_value & UINT64_C(0xFFFFFFFF00000000)) != 0));
  SET_FLAG(registers, OF,
           ((((new_value ^ old_value) & (new_value ^ add_value)) & UINT64_C(0x80000000)) == UINT64_C(0x80000000)));
  SET_FLAG(registers, AF, (((old_value ^ add_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

inline uint32 ALUOp_Adc32(CPU::Registers* registers, uint32 lhs, uint32 rhs)
{
  uint64 old_value = ZeroExtend64(lhs);
  uint64 add_value = ZeroExtend64(rhs);
  uint64 carry_in = (registers->EFLAGS.CF) ? 1 : 0;
  uint64 new_value = old_value + add_value + carry_in;
  uint32 out_value = Truncate32(new_value);

  SET_FLAG(registers, CF, ((new_value & UINT64_C(0xFFFFFFFF00000000)) != 0));
  SET_FLAG(registers, OF,
           ((((new_value ^ old_value) & (new_value ^ add_value)) & UINT64_C(0x80000000)) == UINT64_C(0x80000000)));
  SET_FLAG(registers, AF, (((old_value ^ add_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

inline uint32 ALUOp_Sub32(CPU::Registers* registers, uint32 lhs, uint32 rhs)
{
  uint64 old_value = ZeroExtend64(lhs);
  uint64 sub_value = ZeroExtend64(rhs);
  uint64 new_value = old_value - sub_value;
  uint32 out_value = Truncate32(new_value);

  SET_FLAG(registers, CF, ((new_value & UINT64_C(0xFFFFFFFF00000000)) != 0));
  SET_FLAG(registers, OF,
           ((((new_value ^ old_value) & (old_value ^ sub_value)) & UINT64_C(0x80000000)) == UINT64_C(0x80000000)));
  SET_FLAG(registers, AF, (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

inline uint32 ALUOp_Sbb32(CPU::Registers* registers, uint32 lhs, uint32 rhs)
{
  uint64 old_value = ZeroExtend64(lhs);
  uint64 sub_value = ZeroExtend64(rhs);
  uint64 carry_in = registers->EFLAGS.CF ? 1 : 0;
  uint64 new_value = old_value - sub_value - carry_in;
  uint32 out_value = Truncate32(new_value);

  SET_FLAG(registers, CF, ((new_value & UINT64_C(0xFFFFFFFF00000000)) != 0));
  SET_FLAG(registers, OF,
           ((((new_value ^ old_value) & (old_value ^ sub_value)) & UINT64_C(0x80000000)) == UINT64_C(0x80000000)));
  SET_FLAG(registers, AF, (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10));
  SET_FLAG(registers, SF, IsSign(out_value));
  SET_FLAG(registers, ZF, IsZero(out_value));
  SET_FLAG(registers, PF, IsParity(out_value));

  return out_value;
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_ADD(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    uint8 new_value = ALUOp_Add8(&cpu->m_registers, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    uint16 new_value = ALUOp_Add16(&cpu->m_registers, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    uint32 new_value = ALUOp_Add32(&cpu->m_registers, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_ADC(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    uint8 new_value = ALUOp_Adc8(&cpu->m_registers, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    uint16 new_value = ALUOp_Adc16(&cpu->m_registers, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    uint32 new_value = ALUOp_Adc32(&cpu->m_registers, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_SUB(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    uint8 new_value = ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    uint16 new_value = ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    uint32 new_value = ALUOp_Sub32(&cpu->m_registers, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_SBB(CPU* cpu)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    uint8 new_value = ALUOp_Sbb8(&cpu->m_registers, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    uint16 new_value = ALUOp_Sbb16(&cpu->m_registers, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    uint32 new_value = ALUOp_Sbb32(&cpu->m_registers, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_CMP(CPU* cpu)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  // Implemented as subtract but discarding the result
  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    ALUOp_Sub32(&cpu->m_registers, lhs, rhs);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_AND(CPU* cpu)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  bool sf, zf, pf;
  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    uint8 new_value = lhs & rhs;
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    uint16 new_value = lhs & rhs;
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    uint32 new_value = lhs & rhs;
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);

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
  SET_FLAG(&cpu->m_registers, OF, false);
  SET_FLAG(&cpu->m_registers, CF, false);
  SET_FLAG(&cpu->m_registers, SF, sf);
  SET_FLAG(&cpu->m_registers, ZF, zf);
  SET_FLAG(&cpu->m_registers, PF, pf);
  SET_FLAG(&cpu->m_registers, AF, false);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_OR(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  bool sf, zf, pf;
  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    uint8 new_value = lhs | rhs;
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    uint16 new_value = lhs | rhs;
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    uint32 new_value = lhs | rhs;
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);

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
  SET_FLAG(&cpu->m_registers, OF, false);
  SET_FLAG(&cpu->m_registers, CF, false);
  SET_FLAG(&cpu->m_registers, SF, sf);
  SET_FLAG(&cpu->m_registers, ZF, zf);
  SET_FLAG(&cpu->m_registers, PF, pf);
  SET_FLAG(&cpu->m_registers, AF, false);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_XOR(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  bool sf, zf, pf;
  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    uint8 new_value = lhs ^ rhs;
    WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    uint16 new_value = lhs ^ rhs;
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 rhs = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    uint32 new_value = lhs ^ rhs;
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);

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
  SET_FLAG(&cpu->m_registers, OF, false);
  SET_FLAG(&cpu->m_registers, CF, false);
  SET_FLAG(&cpu->m_registers, SF, sf);
  SET_FLAG(&cpu->m_registers, ZF, zf);
  SET_FLAG(&cpu->m_registers, PF, pf);
  SET_FLAG(&cpu->m_registers, AF, false);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_TEST(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  bool sf, zf, pf;
  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
    uint8 new_value = lhs & rhs;

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 rhs = ReadWordOperand<src_mode, src_constant>(cpu);
    uint16 new_value = lhs & rhs;

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 rhs = ReadDWordOperand<src_mode, src_constant>(cpu);
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
  SET_FLAG(&cpu->m_registers, OF, false);
  SET_FLAG(&cpu->m_registers, CF, false);
  SET_FLAG(&cpu->m_registers, SF, sf);
  SET_FLAG(&cpu->m_registers, ZF, zf);
  SET_FLAG(&cpu->m_registers, PF, pf);
  SET_FLAG(&cpu->m_registers, AF, false);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_MOV(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  static_assert(dst_size == src_size, "dst_size == src_size");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<src_mode, src_constant>(cpu);
    WriteByteOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<src_mode, src_constant>(cpu);
    WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<src_mode, src_constant>(cpu);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_MOVZX(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_16)
  {
    uint16 value = ReadZeroExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadZeroExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_MOVSX(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  if (actual_size == OperandSize_16)
  {
    uint16 value = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_MOV_Sreg(CPU* cpu)
{
  static_assert(dst_size == OperandSize_16 && src_size == OperandSize_16, "Segment registers are 16-bits");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  uint8 segreg = cpu->idata.GetModRM_Reg();
  if (segreg >= Segment_Count)
    RaiseInvalidOpcode(cpu);

  // TODO: The MOV instruction cannot be used to load the CS register. Attempting to do so results in an invalid opcode
  // exception (#UD). Loading the SS register with a MOV instruction inhibits all interrupts until after the execution
  // of the next instruction. This operation allows a stack pointer to be loaded into the ESP register with the next
  // instruction (MOV ESP, stack-pointer value) before an interrupt occurs1. Be aware that the LSS instruction offers a
  // more efficient method of loading the SS and ESP registers.
  if constexpr (dst_mode == OperandMode_ModRM_SegmentReg)
  {
    // Loading segment register
    uint16 value = ReadWordOperand<src_mode, src_constant>(cpu);
    cpu->LoadSegmentRegister(static_cast<CPU_X86::Segment>(segreg), value);
  }
  else
  {
    // Storing segment register - these are zero-extended when the operand size is 32-bit and the destination is a
    // register.
    uint16 value = cpu->m_registers.segment_selectors[segreg];
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_XCHG(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  static_assert(dst_size == src_size, "source and destination operands are of same size");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  // In memory version, memory is op0, register is op1. Memory must be written first.
  if (actual_size == OperandSize_8)
  {
    uint8 value0 = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 value1 = ReadByteOperand<src_mode, src_constant>(cpu);

    WriteByteOperand<dst_mode, dst_constant>(cpu, value1);
    WriteByteOperand<src_mode, src_constant>(cpu, value0);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value0 = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 value1 = ReadWordOperand<src_mode, src_constant>(cpu);

    WriteWordOperand<dst_mode, dst_constant>(cpu, value1);
    WriteWordOperand<src_mode, src_constant>(cpu, value0);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value0 = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 value1 = ReadDWordOperand<src_mode, src_constant>(cpu);

    WriteDWordOperand<dst_mode, dst_constant>(cpu, value1);
    WriteDWordOperand<src_mode, src_constant>(cpu, value0);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant, OperandSize count_size,
         OperandMode count_mode, uint32 count_constant>
void NewInterpreter::Execute_Operation_SHL(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    uint16 shifted_value = ZeroExtend16(value) << shift_amount;
    uint8 new_value = Truncate8(shifted_value);
    WriteByteOperand<val_mode, val_constant>(cpu, new_value);

    SET_FLAG(&cpu->m_registers, CF, ((shifted_value & 0x100) != 0));
    SET_FLAG(&cpu->m_registers, OF,
             (shift_amount == 1 && (((shifted_value >> 7) & 1) ^ ((shifted_value >> 8) & 1)) != 0));
    SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
    SET_FLAG(&cpu->m_registers, AF, false);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    uint32 shifted_value = ZeroExtend32(value) << shift_amount;
    uint16 new_value = Truncate16(shifted_value);
    WriteWordOperand<val_mode, val_constant>(cpu, new_value);

    SET_FLAG(&cpu->m_registers, CF, ((shifted_value & 0x10000) != 0));
    SET_FLAG(&cpu->m_registers, OF,
             (shift_amount == 1 && (((shifted_value >> 15) & 1) ^ ((shifted_value >> 16) & 1)) != 0));
    SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
    SET_FLAG(&cpu->m_registers, AF, false);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    uint64 shifted_value = ZeroExtend64(value) << shift_amount;
    uint32 new_value = Truncate32(shifted_value);
    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);

    SET_FLAG(&cpu->m_registers, CF, ((shifted_value & UINT64_C(0x100000000)) != 0));
    SET_FLAG(&cpu->m_registers, OF,
             (shift_amount == 1 && (((shifted_value >> 31) & 1) ^ ((shifted_value >> 32) & 1)) != 0));
    SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
    SET_FLAG(&cpu->m_registers, AF, false);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant, OperandSize count_size,
         OperandMode count_mode, uint32 count_constant>
void NewInterpreter::Execute_Operation_SHR(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    uint8 new_value = value >> shift_amount;
    WriteByteOperand<val_mode, val_constant>(cpu, new_value);

    SET_FLAG(&cpu->m_registers, CF, ((shift_amount ? (value >> (shift_amount - 1) & 1) : (value & 1)) != 0));
    SET_FLAG(&cpu->m_registers, OF, (shift_amount == 1 && (value & 0x80) != 0));
    SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    uint16 new_value = value >> shift_amount;
    WriteWordOperand<val_mode, val_constant>(cpu, new_value);

    SET_FLAG(&cpu->m_registers, CF, ((shift_amount ? (value >> (shift_amount - 1) & 1) : (value & 1)) != 0));
    SET_FLAG(&cpu->m_registers, OF, (shift_amount == 1 && (value & 0x8000) != 0));
    SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    uint32 new_value = value >> shift_amount;
    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);

    SET_FLAG(&cpu->m_registers, CF, ((shift_amount ? (value >> (shift_amount - 1) & 1) : (value & 1)) != 0));
    SET_FLAG(&cpu->m_registers, OF, (shift_amount == 1 && (value & 0x80000000) != 0));
    SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant, OperandSize count_size,
         OperandMode count_mode, uint32 count_constant>
void NewInterpreter::Execute_Operation_SAR(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    uint8 new_value = uint8(int8(value) >> shift_amount);
    WriteByteOperand<val_mode, val_constant>(cpu, new_value);

    SET_FLAG(&cpu->m_registers, CF, ((int8(value) >> (shift_amount - 1) & 1) != 0));
    SET_FLAG(&cpu->m_registers, OF, false);
    SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    uint16 new_value = uint16(int16(value) >> shift_amount);
    WriteWordOperand<val_mode, val_constant>(cpu, new_value);

    SET_FLAG(&cpu->m_registers, CF, ((int16(value) >> (shift_amount - 1) & 1) != 0));
    SET_FLAG(&cpu->m_registers, OF, false);
    SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_amount == 0)
      return;

    uint32 new_value = uint32(int32(value) >> shift_amount);
    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);

    SET_FLAG(&cpu->m_registers, CF, ((int32(value) >> (shift_amount - 1) & 1) != 0));
    SET_FLAG(&cpu->m_registers, OF, false);
    SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant, OperandSize count_size,
         OperandMode count_mode, uint32 count_constant>
void NewInterpreter::Execute_Operation_RCL(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // The processor restricts the count to a number between 0 and 31 by masking all the bits in the count operand except
  // the 5 least-significant bits.
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    uint8 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint8 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 7);
    }
    WriteByteOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 7) ^ carry) != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    uint16 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint16 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 15);
    }
    WriteWordOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 15) ^ carry) != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    uint32 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint32 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 31);
    }
    WriteDWordOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 31) ^ carry) != 0));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant, OperandSize count_size,
         OperandMode count_mode, uint32 count_constant>
void NewInterpreter::Execute_Operation_RCR(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // The processor restricts the count to a number between 0 and 31 by masking all the bits in the count operand except
  // the 5 least-significant bits.
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    uint8 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint8 save_value = value;
      value = (save_value >> 1) | (carry << 7);
      carry = (save_value & 1);
    }
    WriteByteOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 7) ^ ((value >> 6) & 1)) != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    uint16 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint16 save_value = value;
      value = (save_value >> 1) | (carry << 15);
      carry = (save_value & 1);
    }
    WriteWordOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 15) ^ ((value >> 14) & 1)) != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (rotate_count == 0)
      return;

    uint32 carry = (cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint32 save_value = value;
      value = (save_value >> 1) | (carry << 31);
      carry = (save_value & 1);
    }
    WriteDWordOperand<val_mode, val_constant>(cpu, value);

    SET_FLAG(&cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&cpu->m_registers, OF, (((value >> 31) ^ ((value >> 30) & 1)) != 0));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant, OperandSize count_size,
         OperandMode count_mode, uint32 count_constant>
void NewInterpreter::Execute_Operation_ROL(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // Hopefully this will compile down to a native ROL instruction
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    uint8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    uint8 new_value = value;
    if ((count & 0x7) != 0)
    {
      uint8 masked_count = count & 0x7;
      new_value = (value << masked_count) | (value >> (8 - masked_count));
      WriteByteOperand<val_mode, val_constant>(cpu, new_value);
    }

    uint8 b0 = (new_value & 1);
    uint8 b7 = (new_value >> 7);
    SET_FLAG(&cpu->m_registers, CF, (b0 != 0));
    SET_FLAG(&cpu->m_registers, OF, ((b0 ^ b7) != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    uint8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    uint16 new_value = value;
    if ((count & 0xf) != 0)
    {
      uint8 masked_count = count & 0xf;
      new_value = (value << masked_count) | (value >> (16 - masked_count));
      WriteWordOperand<val_mode, val_constant>(cpu, new_value);
    }

    uint16 b0 = (new_value & 1);
    uint16 b15 = (new_value >> 15);
    SET_FLAG(&cpu->m_registers, CF, (b0 != 0));
    SET_FLAG(&cpu->m_registers, OF, ((b0 ^ b15) != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    uint8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    uint32 new_value = value;
    uint8 masked_count = count & 0x1f;
    if (masked_count != 0)
    {
      new_value = (value << masked_count) | (value >> (32 - masked_count));
      WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
    }

    uint32 b0 = (new_value & 1);
    uint32 b31 = ((new_value >> 31) & 1);
    SET_FLAG(&cpu->m_registers, CF, (b0 != 0));
    SET_FLAG(&cpu->m_registers, OF, ((b0 ^ b31) != 0));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant, OperandSize count_size,
         OperandMode count_mode, uint32 count_constant>
void NewInterpreter::Execute_Operation_ROR(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
  CalculateEffectiveAddress<val_mode>(cpu);
  CalculateEffectiveAddress<count_mode>(cpu);

  // Hopefully this will compile down to a native ROR instruction
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    uint8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    uint8 new_value = value;
    uint8 masked_count = count & 0x7;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (8 - masked_count));
      WriteByteOperand<val_mode, val_constant>(cpu, new_value);
    }

    uint16 b6 = ((new_value >> 6) & 1);
    uint16 b7 = ((new_value >> 7) & 1);
    SET_FLAG(&cpu->m_registers, CF, (b7 != 0));
    SET_FLAG(&cpu->m_registers, OF, ((b6 ^ b7) != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    uint8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    uint16 new_value = value;
    uint8 masked_count = count & 0xf;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (16 - masked_count));
      WriteWordOperand<val_mode, val_constant>(cpu, new_value);
    }

    uint16 b14 = ((new_value >> 14) & 1);
    uint16 b15 = ((new_value >> 15) & 1);
    SET_FLAG(&cpu->m_registers, CF, (b15 != 0));
    SET_FLAG(&cpu->m_registers, OF, ((b14 ^ b15) != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    uint8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (count == 0)
      return;

    uint32 new_value = value;
    uint8 masked_count = count & 0x1f;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (32 - masked_count));
      WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
    }

    uint32 b30 = ((new_value >> 30) & 1);
    uint32 b31 = ((new_value >> 31) & 1);
    SET_FLAG(&cpu->m_registers, CF, (b31 != 0));
    SET_FLAG(&cpu->m_registers, OF, ((b30 ^ b31) != 0));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_IN(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const uint16 port_number = ReadZeroExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
  if (actual_size == OperandSize_8)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint8), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint8 value;
    cpu->m_bus->ReadIOPortByte(port_number, &value);
    WriteByteOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_16)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint16), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint16 value;
    cpu->m_bus->ReadIOPortWord(port_number, &value);
    WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_32)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint32), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint32 value;
    cpu->m_bus->ReadIOPortDWord(port_number, &value);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_OUT(CPU* cpu)
{
  const OperandSize actual_size = (src_size == OperandSize_Count) ? cpu->idata.operand_size : src_size;
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const uint16 port_number = ReadZeroExtendedWordOperand<dst_size, dst_mode, dst_constant>(cpu);
  if (actual_size == OperandSize_8)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint8), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint8 value = ReadByteOperand<src_mode, src_constant>(cpu);
    cpu->m_bus->WriteIOPortByte(port_number, value);
  }
  else if (actual_size == OperandSize_16)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint16), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint16 value = ReadWordOperand<src_mode, src_constant>(cpu);
    cpu->m_bus->WriteIOPortWord(port_number, value);
  }
  else if (actual_size == OperandSize_32)
  {
    if (!cpu->HasIOPermissions(port_number, sizeof(uint32), true))
    {
      cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint32 value = ReadDWordOperand<src_mode, src_constant>(cpu);
    cpu->m_bus->WriteIOPortDWord(port_number, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant>
void NewInterpreter::Execute_Operation_INC(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  // Preserve CF
  bool cf = cpu->m_registers.EFLAGS.CF;
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    uint8 new_value = ALUOp_Add8(&cpu->m_registers, value, 1);
    WriteByteOperand<val_mode, val_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    uint16 new_value = ALUOp_Add16(&cpu->m_registers, value, 1);
    WriteWordOperand<val_mode, val_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    uint32 new_value = ALUOp_Add32(&cpu->m_registers, value, 1);
    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  SET_FLAG(&cpu->m_registers, CF, cf);
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant>
void NewInterpreter::Execute_Operation_DEC(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  // Preserve CF
  bool cf = cpu->m_registers.EFLAGS.CF;
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    uint8 new_value = ALUOp_Sub8(&cpu->m_registers, value, 1);
    WriteByteOperand<val_mode, val_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    uint16 new_value = ALUOp_Sub16(&cpu->m_registers, value, 1);
    WriteWordOperand<val_mode, val_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    uint32 new_value = ALUOp_Sub32(&cpu->m_registers, value, 1);
    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  SET_FLAG(&cpu->m_registers, CF, cf);
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant>
void NewInterpreter::Execute_Operation_NOT(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    uint8 new_value = ~value;
    WriteByteOperand<val_mode, val_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    uint16 new_value = ~value;
    WriteWordOperand<val_mode, val_constant>(cpu, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    uint32 new_value = ~value;
    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant>
void NewInterpreter::Execute_Operation_NEG(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(cpu);
    uint8 new_value = uint8(-int8(value));
    WriteByteOperand<val_mode, val_constant>(cpu, new_value);

    ALUOp_Sub8(&cpu->m_registers, 0, value);
    SET_FLAG(&cpu->m_registers, CF, (new_value != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(cpu);
    uint16 new_value = uint16(-int16(value));
    WriteWordOperand<val_mode, val_constant>(cpu, new_value);

    ALUOp_Sub16(&cpu->m_registers, 0, value);
    SET_FLAG(&cpu->m_registers, CF, (new_value != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    uint32 new_value = uint32(-int32(value));
    WriteDWordOperand<val_mode, val_constant>(cpu, new_value);

    ALUOp_Sub32(&cpu->m_registers, 0, value);
    SET_FLAG(&cpu->m_registers, CF, (new_value != 0));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant>
void NewInterpreter::Execute_Operation_MUL(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  // The OF and CF flags are set to 0 if the upper half of the result is 0;
  // otherwise, they are set to 1. The SF, ZF, AF, and PF flags are undefined.
  if (actual_size == OperandSize_8)
  {
    uint16 lhs = uint16(cpu->m_registers.AL);
    uint16 rhs = uint16(ReadByteOperand<val_mode, val_constant>(cpu));
    uint16 result = lhs * rhs;
    cpu->m_registers.AX = result;
    SET_FLAG(&cpu->m_registers, OF, (cpu->m_registers.AH != 0));
    SET_FLAG(&cpu->m_registers, CF, (cpu->m_registers.AH != 0));
    SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.AL));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.AL));
    SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.AL));
  }
  else if (actual_size == OperandSize_16)
  {
    uint32 lhs = uint32(cpu->m_registers.AX);
    uint32 rhs = uint32(ReadWordOperand<val_mode, val_constant>(cpu));
    uint32 result = lhs * rhs;
    cpu->m_registers.AX = uint16(result & 0xFFFF);
    cpu->m_registers.DX = uint16(result >> 16);
    SET_FLAG(&cpu->m_registers, OF, (cpu->m_registers.DX != 0));
    SET_FLAG(&cpu->m_registers, CF, (cpu->m_registers.DX != 0));
    SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.AX));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.AX));
    SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.AX));
  }
  else if (actual_size == OperandSize_32)
  {
    uint64 lhs = ZeroExtend64(cpu->m_registers.EAX);
    uint64 rhs = ZeroExtend64(ReadDWordOperand<val_mode, val_constant>(cpu));
    uint64 result = lhs * rhs;
    cpu->m_registers.EAX = Truncate32(result);
    cpu->m_registers.EDX = Truncate32(result >> 32);
    SET_FLAG(&cpu->m_registers, OF, (cpu->m_registers.EDX != 0));
    SET_FLAG(&cpu->m_registers, CF, (cpu->m_registers.EDX != 0));
    SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.EAX));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.EAX));
    SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.EAX));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize op1_size, OperandMode op1_mode, uint32 op1_constant, OperandSize op2_size, OperandMode op2_mode,
         uint32 op2_constant, OperandSize op3_size, OperandMode op3_mode, uint32 op3_constant>
void NewInterpreter::Execute_Operation_IMUL(CPU* cpu)
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

    int16 lhs = int8(cpu->m_registers.AL);
    int16 rhs = int8(ReadByteOperand<op1_mode, op1_constant>(cpu));
    int16 result = lhs * rhs;
    uint8 truncated_result = uint8(uint16(result) & 0xFFFF);

    cpu->m_registers.AX = uint16(result);

    cpu->m_registers.EFLAGS.OF = (int16(int8(truncated_result)) != result);
    cpu->m_registers.EFLAGS.CF = (int16(int8(truncated_result)) != result);
    cpu->m_registers.EFLAGS.SF = IsSign(truncated_result);
    cpu->m_registers.EFLAGS.ZF = IsZero(truncated_result);
    cpu->m_registers.EFLAGS.PF = IsParity(truncated_result);
  }
  else if (actual_size == OperandSize_16)
  {
    int32 lhs, rhs;
    int32 result;
    uint16 truncated_result;

    if constexpr (op3_mode != OperandMode_None)
    {
      // Three-operand form
      lhs = int16(ReadSignExtendedWordOperand<op2_size, op2_mode, op2_constant>(cpu));
      rhs = int16(ReadSignExtendedWordOperand<op3_size, op3_mode, op3_constant>(cpu));
      result = lhs * rhs;
      truncated_result = uint16(uint32(result) & 0xFFFF);

      WriteWordOperand<op1_mode, op1_constant>(cpu, truncated_result);
    }
    else if constexpr (op2_mode != OperandMode_None)
    {
      // Two-operand form
      lhs = int16(ReadSignExtendedWordOperand<op1_size, op1_mode, op1_constant>(cpu));
      rhs = int16(ReadSignExtendedWordOperand<op2_size, op2_mode, op2_constant>(cpu));
      result = lhs * rhs;
      truncated_result = uint16(uint32(result) & 0xFFFF);

      WriteWordOperand<op1_mode, op1_constant>(cpu, truncated_result);
    }
    else
    {
      // One-operand form
      lhs = int16(cpu->m_registers.AX);
      rhs = int16(ReadSignExtendedWordOperand<op1_size, op1_mode, op1_constant>(cpu));
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
  else if (actual_size == OperandSize_32)
  {
    int64 lhs, rhs;
    int64 result;
    uint32 truncated_result;

    if constexpr (op3_mode != OperandMode_None)
    {
      // Three-operand form
      lhs = int32(ReadSignExtendedDWordOperand<op2_size, op2_mode, op2_constant>(cpu));
      rhs = int32(ReadSignExtendedDWordOperand<op3_size, op3_mode, op3_constant>(cpu));
      result = lhs * rhs;
      truncated_result = Truncate32(result);

      WriteDWordOperand<op1_mode, op1_constant>(cpu, truncated_result);
    }
    else if constexpr (op2_mode != OperandMode_None)
    {
      // Two-operand form
      lhs = int32(ReadSignExtendedDWordOperand<op1_size, op1_mode, op1_constant>(cpu));
      rhs = int32(ReadSignExtendedDWordOperand<op2_size, op2_mode, op2_constant>(cpu));
      result = lhs * rhs;
      truncated_result = Truncate32(result);

      WriteDWordOperand<op1_mode, op1_constant>(cpu, truncated_result);
    }
    else
    {
      // One-operand form
      lhs = int32(cpu->m_registers.EAX);
      rhs = int32(ReadSignExtendedDWordOperand<op1_size, op1_mode, op1_constant>(cpu));
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

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant>
void NewInterpreter::Execute_Operation_DIV(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    // Eight-bit divides use AX as a source
    uint8 divisor = ReadByteOperand<val_mode, val_constant>(cpu);
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
  else if (actual_size == OperandSize_16)
  {
    // 16-bit divides use DX:AX as a source
    uint16 divisor = ReadWordOperand<val_mode, val_constant>(cpu);
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
  else if (actual_size == OperandSize_32)
  {
    // 32-bit divides use EDX:EAX as a source
    uint32 divisor = ReadDWordOperand<val_mode, val_constant>(cpu);
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

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant>
void NewInterpreter::Execute_Operation_IDIV(CPU* cpu)
{
  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  CalculateEffectiveAddress<val_mode>(cpu);

  if (actual_size == OperandSize_8)
  {
    // Eight-bit divides use AX as a source
    int8 divisor = int8(ReadByteOperand<val_mode, val_constant>(cpu));
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
  else if (actual_size == OperandSize_16)
  {
    // 16-bit divides use DX:AX as a source
    int16 divisor = int16(ReadWordOperand<val_mode, val_constant>(cpu));
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
  else if (actual_size == OperandSize_32)
  {
    // 16-bit divides use DX:AX as a source
    int32 divisor = int32(ReadDWordOperand<val_mode, val_constant>(cpu));
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

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void NewInterpreter::Execute_Operation_PUSH(CPU* cpu)
{
  CalculateEffectiveAddress<src_mode>(cpu);
  if (cpu->idata.operand_size == OperandSize_16)
  {
    uint16 value = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    cpu->PushWord(value);
  }
  else if (cpu->idata.operand_size == OperandSize_32)
  {
    uint32 value = ReadSignExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    cpu->PushDWord(value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void NewInterpreter::Execute_Operation_PUSH_Sreg(CPU* cpu)
{
  static_assert(src_size == OperandSize_16 && src_mode == OperandMode_SegmentRegister && src_constant < Segment_Count,
                "operands are of correct type and in range");

  // TODO: Is this correct for 32-bits? Bochs only writes 2 of the 4 bytes.
  uint16 selector = cpu->m_registers.segment_selectors[src_constant];
  if (cpu->idata.operand_size == OperandSize_16)
    cpu->PushWord(selector);
  else
    cpu->PushDWord(SignExtend32(selector));
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_POP_Sreg(CPU* cpu)
{
  static_assert(dst_size == OperandSize_16 && dst_mode == OperandMode_SegmentRegister && dst_constant < Segment_Count,
                "operands are of correct type and in range");

  uint16 selector;
  if (cpu->idata.operand_size == OperandSize_16)
    selector = cpu->PopWord();
  else
    selector = Truncate16(cpu->PopDWord());

  cpu->LoadSegmentRegister(static_cast<Segment>(dst_constant), selector);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_POP(CPU* cpu)
{
  static_assert(dst_size == OperandSize_Count, "operand size is current mode");

  // POP can use ESP in the address calculations, in this case the value of ESP
  // is that after the pop operation has occurred, not before.
  if (cpu->idata.operand_size == OperandSize_16)
  {
    uint16 value = cpu->PopWord();
    CalculateEffectiveAddress<dst_mode>(cpu);
    WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else
  {
    uint32 value = cpu->PopDWord();
    CalculateEffectiveAddress<dst_mode>(cpu);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
}

void NewInterpreter::Execute_Operation_PUSHA(CPU* cpu)
{
  if (cpu->idata.operand_size == OperandSize_16)
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
  else if (cpu->idata.operand_size == OperandSize_32)
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

void NewInterpreter::Execute_Operation_POPA(CPU* cpu)
{
  // Assignment split from reading in case of exception.
  if (cpu->idata.operand_size == OperandSize_16)
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
  else if (cpu->idata.operand_size == OperandSize_32)
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

template<OperandSize frame_size, OperandMode frame_mode, uint32 frame_constant, OperandSize level_size,
         OperandMode level_mode, uint32 level_constant>
void NewInterpreter::Execute_Operation_ENTER(CPU* cpu)
{
  static_assert(frame_size == OperandSize_16 && level_size == OperandSize_8, "args have correct size");
  uint16 stack_frame_size = ReadWordOperand<frame_mode, frame_constant>(cpu);
  uint8 level = ReadByteOperand<level_mode, level_constant>(cpu);

  // Push current frame pointer.
  if (cpu->idata.operand_size == OperandSize_16)
    cpu->PushWord(cpu->m_registers.BP);
  else
    cpu->PushDWord(cpu->m_registers.EBP);

  uint32 frame_pointer = cpu->m_registers.ESP;
  if (level > 0)
  {
    // Use our own local copy of EBP in case any of these fail.
    if (cpu->idata.operand_size == OperandSize_16)
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

  if (cpu->idata.operand_size == OperandSize_16)
    cpu->m_registers.BP = Truncate16(frame_pointer);
  else
    cpu->m_registers.EBP = frame_pointer;

  if (cpu->m_stack_address_size == AddressSize_16)
    cpu->m_registers.SP -= stack_frame_size;
  else
    cpu->m_registers.ESP -= stack_frame_size;
}

void NewInterpreter::Execute_Operation_LEAVE(CPU* cpu)
{
  if (cpu->m_stack_address_size == AddressSize_16)
    cpu->m_registers.SP = cpu->m_registers.BP;
  else
    cpu->m_registers.ESP = cpu->m_registers.EBP;

  if (cpu->idata.operand_size == OperandSize_16)
    cpu->m_registers.BP = cpu->PopWord();
  else if (cpu->idata.operand_size == OperandSize_32)
    cpu->m_registers.EBP = cpu->PopDWord();
  else
    DebugUnreachableCode();
}

template<OperandSize sreg_size, OperandMode sreg_mode, uint32 sreg_constant, OperandSize reg_size, OperandMode reg_mode,
         uint32 reg_constant, OperandSize ptr_size, OperandMode ptr_mode, uint32 ptr_constant>
void NewInterpreter::Execute_Operation_LXS(CPU* cpu)
{
  static_assert(sreg_mode == OperandMode_SegmentRegister, "sreg_mode is Segment Register");
  static_assert(reg_mode == OperandMode_ModRM_Reg, "reg_mode is Register");
  static_assert(ptr_mode == OperandMode_ModRM_RM, "reg_mode is Pointer");
  CalculateEffectiveAddress<ptr_mode>(cpu);

  uint16 segment_selector;
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_LEA(CPU* cpu)
{
  static_assert(src_mode == OperandMode_ModRM_RM, "Source operand is a pointer");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  // Calculate full address in instruction's address mode, truncate/extend to operand size.
  if (cpu->idata.operand_size == OperandSize_16)
    WriteWordOperand<dst_mode, dst_constant>(cpu, Truncate16(cpu->m_effective_address));
  else
    WriteDWordOperand<dst_mode, dst_constant>(cpu, cpu->m_effective_address);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_JMP_Near(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_size, dst_mode, dst_constant>(cpu);
  cpu->BranchTo(jump_address);
}

template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_Jcc(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  if (!TestJumpCondition<condition>(cpu))
    return;

  VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_size, dst_mode, dst_constant>(cpu);
  cpu->BranchTo(jump_address);
}

template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_LOOP(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  uint32 count;
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_CALL_Near(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_size, dst_mode, dst_constant>(cpu);
  if (cpu->idata.operand_size == OperandSize_16)
    cpu->PushWord(Truncate16(cpu->m_registers.EIP));
  else
    cpu->PushDWord(cpu->m_registers.EIP);

  cpu->BranchTo(jump_address);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_RET_Near(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  uint32 pop_count = 0;
  if constexpr (dst_mode != OperandMode_None)
    pop_count = ReadZeroExtendedDWordOperand<dst_size, dst_mode, dst_constant>(cpu);

  uint32 return_EIP;
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_JMP_Far(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);

  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand<dst_mode>(cpu, actual_size, &segment_selector, &address);

  cpu->FarJump(segment_selector, address, actual_size);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_CALL_Far(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<dst_mode>(cpu);

  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand<dst_mode>(cpu, actual_size, &segment_selector, &address);

  cpu->FarCall(segment_selector, address, actual_size);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_RET_Far(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  uint32 pop_count = 0;
  if constexpr (dst_mode != OperandMode_None)
    pop_count = ReadZeroExtendedDWordOperand<dst_size, dst_mode, dst_constant>(cpu);

  cpu->FarReturn(cpu->idata.operand_size, pop_count);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_INT(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  uint32 interrupt = ReadZeroExtendedWordOperand<dst_size, dst_mode, dst_constant>(cpu);
  cpu->SoftwareInterrupt(cpu->idata.operand_size, interrupt);
}

void NewInterpreter::Execute_Operation_INTO(CPU* cpu)
{
  // Call overflow exception if OF is set
  if (cpu->m_registers.EFLAGS.OF)
    cpu->RaiseException(Interrupt_Overflow);
}

void NewInterpreter::Execute_Operation_IRET(CPU* cpu)
{
  cpu->InterruptReturn(cpu->idata.operand_size);
}

void NewInterpreter::Execute_Operation_NOP(CPU* cpu) {}

void NewInterpreter::Execute_Operation_CLC(CPU* cpu)
{
  SET_FLAG(&cpu->m_registers, CF, false);
}

void NewInterpreter::Execute_Operation_CLD(CPU* cpu)
{
  SET_FLAG(&cpu->m_registers, DF, false);
}

void NewInterpreter::Execute_Operation_CLI(CPU* cpu)
{
  // TODO: Delay of one instruction
  if (cpu->InProtectedMode() && cpu->GetCPL() > cpu->GetIOPL())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault);
    return;
  }

  SET_FLAG(&cpu->m_registers, IF, false);
}

void NewInterpreter::Execute_Operation_CMC(CPU* cpu)
{
  SET_FLAG(&cpu->m_registers, CF, !cpu->m_registers.EFLAGS.CF);
}

void NewInterpreter::Execute_Operation_CLTS(CPU* cpu)
{
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  cpu->m_registers.CR0 &= ~CR0Bit_TS;
}

void NewInterpreter::Execute_Operation_STC(CPU* cpu)
{
  SET_FLAG(&cpu->m_registers, CF, true);
}

void NewInterpreter::Execute_Operation_STD(CPU* cpu)
{
  SET_FLAG(&cpu->m_registers, DF, true);
}

void NewInterpreter::Execute_Operation_STI(CPU* cpu)
{
  if (cpu->InProtectedMode() && cpu->GetCPL() > cpu->GetIOPL())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault);
    return;
  }

  SET_FLAG(&cpu->m_registers, IF, true);
}

void NewInterpreter::Execute_Operation_SALC(CPU* cpu)
{
  // Undocumented instruction. Same as SBB AL, AL without modifying any flags.
  uint32 old_flags = cpu->m_registers.EFLAGS.bits;
  cpu->m_registers.AL = ALUOp_Sbb8(&cpu->m_registers, cpu->m_registers.AL, cpu->m_registers.AL);
  cpu->m_registers.EFLAGS.bits = old_flags;
}

void NewInterpreter::Execute_Operation_LAHF(CPU* cpu)
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

  cpu->m_registers.AH = Truncate8(cpu->m_registers.EFLAGS.bits);
}

void NewInterpreter::Execute_Operation_SAHF(CPU* cpu)
{
  uint16 flags = Truncate16(cpu->m_registers.EFLAGS.bits & 0xFF00) | ZeroExtend16(cpu->m_registers.AH);
  cpu->SetFlags16(flags);
}

void NewInterpreter::Execute_Operation_PUSHF(CPU* cpu)
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

  if (cpu->idata.operand_size == OperandSize_16)
    cpu->PushWord(Truncate16(EFLAGS));
  else if (cpu->idata.operand_size == OperandSize_32)
    cpu->PushDWord(EFLAGS);
  else
    DebugUnreachableCode();
}

void NewInterpreter::Execute_Operation_POPF(CPU* cpu)
{
  // If V8086 and IOPL!=3, trap to monitor
  if (cpu->InVirtual8086Mode() && cpu->GetIOPL() != 3)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  if (cpu->idata.operand_size == OperandSize_16)
  {
    uint16 flags = cpu->PopWord();
    cpu->SetFlags16(flags);
  }
  else if (cpu->idata.operand_size == OperandSize_32)
  {
    uint32 flags = cpu->PopDWord();
    cpu->SetFlags(flags);
  }
  else
  {
    DebugUnreachableCode();
  }
}

void NewInterpreter::Execute_Operation_HLT(CPU* cpu)
{
  // HLT is a privileged instruction
  if ((cpu->InProtectedMode() && cpu->GetCPL() != 0) || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  cpu->SetHalted(true);
}

void NewInterpreter::Execute_Operation_CBW(CPU* cpu)
{
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

void NewInterpreter::Execute_Operation_CWD(CPU* cpu)
{
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

void NewInterpreter::Execute_Operation_XLAT(CPU* cpu)
{
  uint8 value;
  if (cpu->idata.address_size == AddressSize_16)
  {
    uint16 address = cpu->m_registers.BX + ZeroExtend16(cpu->m_registers.AL);
    value = cpu->ReadMemoryByte(cpu->idata.segment, address);
  }
  else if (cpu->idata.address_size == AddressSize_32)
  {
    uint32 address = cpu->m_registers.EBX + ZeroExtend32(cpu->m_registers.AL);
    value = cpu->ReadMemoryByte(cpu->idata.segment, address);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
  cpu->m_registers.AL = value;
}

void NewInterpreter::Execute_Operation_AAA(CPU* cpu)
{
  if ((cpu->m_registers.AL & 0xF) > 0x09 || cpu->m_registers.EFLAGS.AF)
  {
    cpu->m_registers.AX += 0x0106;
    SET_FLAG(&cpu->m_registers, AF, true);
    SET_FLAG(&cpu->m_registers, CF, true);
  }
  else
  {
    SET_FLAG(&cpu->m_registers, AF, false);
    SET_FLAG(&cpu->m_registers, CF, false);
  }

  cpu->m_registers.AL &= 0x0F;

  SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.AL));
}

void NewInterpreter::Execute_Operation_AAS(CPU* cpu)
{
  if ((cpu->m_registers.AL & 0xF) > 0x09 || cpu->m_registers.EFLAGS.AF)
  {
    cpu->m_registers.AX -= 0x0106;
    SET_FLAG(&cpu->m_registers, AF, true);
    SET_FLAG(&cpu->m_registers, CF, true);
  }
  else
  {
    SET_FLAG(&cpu->m_registers, AF, false);
    SET_FLAG(&cpu->m_registers, CF, false);
  }

  cpu->m_registers.AL &= 0x0F;

  SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.AL));
}

template<OperandSize op_size, OperandMode op_mode, uint32 op_constant>
void NewInterpreter::Execute_Operation_AAM(CPU* cpu)
{
  CalculateEffectiveAddress<op_mode>(cpu);

  uint8 operand = ReadByteOperand<op_mode, op_constant>(cpu);
  if (operand == 0)
  {
    cpu->RaiseException(Interrupt_DivideError);
    return;
  }

  cpu->m_registers.AH = cpu->m_registers.AL / operand;
  cpu->m_registers.AL = cpu->m_registers.AL % operand;

  SET_FLAG(&cpu->m_registers, AF, false);
  SET_FLAG(&cpu->m_registers, CF, false);
  SET_FLAG(&cpu->m_registers, OF, false);

  SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.AL));
}

template<OperandSize op_size, OperandMode op_mode, uint32 op_constant>
void NewInterpreter::Execute_Operation_AAD(CPU* cpu)
{
  CalculateEffectiveAddress<op_mode>(cpu);

  uint8 operand = ReadByteOperand<op_mode, op_constant>(cpu);
  uint16 result = uint16(cpu->m_registers.AH) * uint16(operand) + uint16(cpu->m_registers.AL);

  cpu->m_registers.AL = uint8(result & 0xFF);
  cpu->m_registers.AH = 0;

  SET_FLAG(&cpu->m_registers, AF, false);
  SET_FLAG(&cpu->m_registers, CF, false);
  SET_FLAG(&cpu->m_registers, OF, false);

  SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.AL));
}

void NewInterpreter::Execute_Operation_DAA(CPU* cpu)
{
  uint8 old_AL = cpu->m_registers.AL;
  bool old_CF = cpu->m_registers.EFLAGS.CF;

  if ((old_AL & 0xF) > 0x9 || cpu->m_registers.EFLAGS.AF)
  {
    SET_FLAG(&cpu->m_registers, CF, ((old_AL > 0xF9) || old_CF));
    cpu->m_registers.AL += 0x6;
    SET_FLAG(&cpu->m_registers, AF, true);
  }
  else
  {
    SET_FLAG(&cpu->m_registers, AF, false);
  }

  if (old_AL > 0x99 || old_CF)
  {
    cpu->m_registers.AL += 0x60;
    SET_FLAG(&cpu->m_registers, CF, true);
  }
  else
  {
    SET_FLAG(&cpu->m_registers, CF, false);
  }

  SET_FLAG(&cpu->m_registers, OF, false);
  SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.AL));
}

void NewInterpreter::Execute_Operation_DAS(CPU* cpu)
{
  uint8 old_AL = cpu->m_registers.AL;
  bool old_CF = cpu->m_registers.EFLAGS.CF;

  if ((old_AL & 0xF) > 0x9 || cpu->m_registers.EFLAGS.AF)
  {
    SET_FLAG(&cpu->m_registers, CF, ((old_AL < 0x06) || old_CF));
    cpu->m_registers.AL -= 0x6;
    SET_FLAG(&cpu->m_registers, AF, true);
  }
  else
  {
    SET_FLAG(&cpu->m_registers, AF, false);
  }

  if (old_AL > 0x99 || old_CF)
  {
    cpu->m_registers.AL -= 0x60;
    SET_FLAG(&cpu->m_registers, CF, true);
  }

  SET_FLAG(&cpu->m_registers, OF, false);
  SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.AL));
  SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.AL));
}

template<OperandSize val_size, OperandMode val_mode, uint32 val_constant>
void NewInterpreter::Execute_Operation_BSWAP(CPU* cpu)
{
  CalculateEffectiveAddress<val_mode>(cpu);

  const OperandSize actual_size = (val_size == OperandSize_Count) ? cpu->idata.operand_size : val_size;
  if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(cpu);
    value = Y_byteswap_uint32(value);
    WriteDWordOperand<val_mode, val_constant>(cpu, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize addr_size, OperandMode addr_mode, uint32 addr_constant>
void NewInterpreter::Execute_Operation_INVLPG(CPU* cpu)
{
  if (cpu->InVirtual8086Mode() || (cpu->InProtectedMode() && cpu->GetCPL() != 0))
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // Get effective address of operand, this is the linear address to clear.
  CalculateEffectiveAddress<addr_mode>(cpu);
  cpu->InvalidateTLBEntry(cpu->m_effective_address);
}

template<OperandSize addr_size, OperandMode addr_mode, uint32 addr_constant, OperandSize table_size,
         OperandMode table_mode, uint32 table_constant>
void NewInterpreter::Execute_Operation_BOUND(CPU* cpu)
{
  CalculateEffectiveAddress<addr_mode>(cpu);
  CalculateEffectiveAddress<table_mode>(cpu);

  uint32 address;
  uint32 lower_bound;
  uint32 upper_bound;
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
    cpu->RaiseException(Interrupt_Bounds);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_ARPL(CPU* cpu)
{
  static_assert(src_size == OperandSize_16 && dst_size == OperandSize_16, "operand sizes are 16-bits");
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
    dst.rpl = src.rpl;
    WriteWordOperand<dst_mode, dst_constant>(cpu, dst.bits);
    cpu->m_registers.EFLAGS.ZF = true;
  }
  else
  {
    cpu->m_registers.EFLAGS.ZF = false;
  }
}

template<Operation operation, OperandSize selector_size, OperandMode selector_mode, uint32 selector_constant>
void NewInterpreter::Execute_Operation_VERx(CPU* cpu)
{
  static_assert(selector_size == OperandSize_16, "selector size is 16-bits");
  CalculateEffectiveAddress<selector_mode>(cpu);

  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

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
    bool is_readable = (descriptor.IsCodeSegment() || descriptor.memory.access.code_readable);
    cpu->m_registers.EFLAGS.ZF = is_readable;
  }
  else if constexpr (operation == Operation_VERW)
  {
    bool is_writable = (!descriptor.IsCodeSegment() && descriptor.memory.access.data_writable);
    cpu->m_registers.EFLAGS.ZF = is_writable;
  }
}

template<OperandSize selector_size, OperandMode selector_mode, uint32 selector_constant>
void NewInterpreter::Execute_Operation_VERW(CPU* cpu)
{
  Execute_Operation_VERx<Operation_VERW, selector_size, selector_mode, selector_constant>(cpu);
}

template<OperandSize selector_size, OperandMode selector_mode, uint32 selector_constant>
void NewInterpreter::Execute_Operation_VERR(CPU* cpu)
{
  Execute_Operation_VERx<Operation_VERR, selector_size, selector_mode, selector_constant>(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize selector_size,
         OperandMode selector_mode, uint32 selector_constant>
void NewInterpreter::Execute_Operation_LSL(CPU* cpu)
{
  static_assert(selector_size == OperandSize_16, "selector size is 16-bits");
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

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_16)
    WriteWordOperand<dst_mode, dst_constant>(cpu, Truncate16(limit));
  else if (actual_size == OperandSize_32)
    WriteDWordOperand<dst_mode, dst_constant>(cpu, limit);
  else
    DebugUnreachableCode();

  cpu->m_registers.EFLAGS.ZF = true;
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize selector_size,
         OperandMode selector_mode, uint32 selector_constant>
void NewInterpreter::Execute_Operation_LAR(CPU* cpu)
{
  static_assert(selector_size == OperandSize_16, "selector size is 16-bits");
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
  const uint32 result = descriptor.bits1 & 0x00FFFF00;
  if (actual_size == OperandSize_16)
    WriteWordOperand<dst_mode, dst_constant>(cpu, Truncate16(result));
  else if (actual_size == OperandSize_32)
    WriteDWordOperand<dst_mode, dst_constant>(cpu, result);
  else
    DebugUnreachableCode();

  cpu->m_registers.EFLAGS.ZF = true;
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void NewInterpreter::Execute_Operation_LIDT(CPU* cpu)
{
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  CalculateEffectiveAddress<src_mode>(cpu);
  const VirtualMemoryAddress base_address = cpu->m_effective_address;
  uint32 table_limit = ZeroExtend32(cpu->ReadMemoryWord(cpu->idata.segment, base_address + 0));
  uint32 table_base_address = cpu->ReadMemoryDWord(cpu->idata.segment, base_address + 2);

  // 16-bit operand drops higher order bits
  if (cpu->idata.operand_size == OperandSize_16)
    table_base_address &= 0xFFFFFF;

  cpu->LoadInterruptDescriptorTable(table_base_address, table_limit);
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void NewInterpreter::Execute_Operation_LGDT(CPU* cpu)
{
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  CalculateEffectiveAddress<src_mode>(cpu);
  const VirtualMemoryAddress base_address = cpu->m_effective_address;
  uint32 table_limit = ZeroExtend32(cpu->ReadMemoryWord(cpu->idata.segment, base_address + 0));
  uint32 table_base_address = cpu->ReadMemoryDWord(cpu->idata.segment, base_address + 2);

  // 16-bit operand drops higher order bits
  if (cpu->idata.operand_size == OperandSize_16)
    table_base_address &= 0xFFFFFF;

  cpu->LoadGlobalDescriptorTable(table_base_address, table_limit);
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void NewInterpreter::Execute_Operation_LLDT(CPU* cpu)
{
  CalculateEffectiveAddress<src_mode>(cpu);

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

  const uint16 selector = ReadWordOperand<src_mode, src_constant>(cpu);
  cpu->LoadLocalDescriptorTable(selector);
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void NewInterpreter::Execute_Operation_LTR(CPU* cpu)
{
  CalculateEffectiveAddress<src_mode>(cpu);

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

  const uint16 selector = ReadWordOperand<src_mode, src_constant>(cpu);
  cpu->LoadTaskSegment(selector);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_SIDT(CPU* cpu)
{
  uint32 idt_address = Truncate32(cpu->m_idt_location.base_address);
  uint16 idt_limit = Truncate16(cpu->m_idt_location.limit);

  // 80286+ sets higher-order bits to 0xFF
  if (cpu->m_model == MODEL_286)
    idt_address = (idt_address & 0xFFFFFF) | 0xFF000000;
  // 16-bit operand sets higher-order bits to zero
  else if (cpu->idata.operand_size == OperandSize_16)
    idt_address = (idt_address & 0xFFFFFF);

  // Write back to memory
  CalculateEffectiveAddress<dst_mode>(cpu);
  const VirtualMemoryAddress base_address = cpu->m_effective_address;
  cpu->WriteMemoryWord(cpu->idata.segment, base_address + 0, idt_limit);
  cpu->WriteMemoryDWord(cpu->idata.segment, base_address + 2, idt_address);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_SGDT(CPU* cpu)
{
  uint32 gdt_address = Truncate32(cpu->m_gdt_location.base_address);
  uint16 gdt_limit = Truncate16(cpu->m_gdt_location.limit);

  // 80286+ sets higher-order bits to 0xFF
  if (cpu->m_model == MODEL_286)
    gdt_address = (gdt_address & 0xFFFFFF) | 0xFF000000;
  // 16-bit operand sets higher-order bits to zero
  else if (cpu->idata.operand_size == OperandSize_16)
    gdt_address = (gdt_address & 0xFFFFFF);

  // Write back to memory
  CalculateEffectiveAddress<dst_mode>(cpu);
  const VirtualMemoryAddress base_address = cpu->m_effective_address;
  cpu->WriteMemoryWord(cpu->idata.segment, base_address + 0, gdt_limit);
  cpu->WriteMemoryDWord(cpu->idata.segment, base_address + 2, gdt_address);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_SLDT(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  WriteWordOperand<dst_mode, dst_constant>(cpu, cpu->m_registers.LDTR);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_STR(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  if (cpu->InRealMode() || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  WriteWordOperand<dst_mode, dst_constant>(cpu, cpu->m_registers.TR);
}

template<OperandSize src_size, OperandMode src_mode, uint32 src_constant>
void NewInterpreter::Execute_Operation_LMSW(CPU* cpu)
{
  CalculateEffectiveAddress<src_mode>(cpu);
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  const uint16 value = ReadWordOperand<src_mode, src_constant>(cpu);
  cpu->LoadSpecialRegister(Reg32_CR0, (cpu->m_registers.CR0 & 0xFFFFFFF0) | ZeroExtend32(value & 0xF));
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_SMSW(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  const uint16 value = Truncate16(cpu->m_registers.CR0);
  WriteWordOperand<dst_mode, dst_constant>(cpu, value);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant, OperandSize count_size, OperandMode count_mode, uint32 count_constant>
void NewInterpreter::Execute_Operation_SHLD(CPU* cpu)
{
  static_assert(dst_size == src_size && count_size == OperandSize_8, "args are correct size");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 shift_in = ReadWordOperand<src_mode, src_constant>(cpu);
    uint8 shift_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_count == 0)
      return;

    uint32 temp_value1 = ((ZeroExtend32(value) << 16) | ZeroExtend32(shift_in));
    uint32 temp_value2 = temp_value1 << shift_count;
    if (shift_count > 16)
      temp_value2 |= (value << (shift_count - 16));

    // temp_value >>= 16;
    uint16 new_value = Truncate16(temp_value2 >> 16);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.CF = (((temp_value1 >> (32 - shift_count)) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = (((value ^ new_value) & 0x8000) != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 shift_in = ReadDWordOperand<src_mode, src_constant>(cpu);
    uint8 shift_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_count == 0)
      return;

    uint32 new_value = (value << shift_count) | (shift_in >> (32 - shift_count));
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);

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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant, OperandSize count_size, OperandMode count_mode, uint32 count_constant>
void NewInterpreter::Execute_Operation_SHRD(CPU* cpu)
{
  static_assert(dst_size == src_size && count_size == OperandSize_8, "args are correct size");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 shift_in = ReadWordOperand<src_mode, src_constant>(cpu);
    uint8 shift_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_count == 0)
      return;

    uint32 temp_value = ((ZeroExtend32(shift_in) << 16) | ZeroExtend32(value));
    temp_value >>= shift_count;
    if (shift_count > 16)
      temp_value |= (value << (32 - shift_count));

    uint16 new_value = Truncate16(temp_value);
    WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

    cpu->m_registers.EFLAGS.CF = (((value >> (shift_count - 1)) & 1) != 0);
    cpu->m_registers.EFLAGS.OF = (((value ^ new_value) & 0x8000) != 0);
    cpu->m_registers.EFLAGS.SF = IsSign(new_value);
    cpu->m_registers.EFLAGS.ZF = IsZero(new_value);
    cpu->m_registers.EFLAGS.PF = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 shift_in = ReadDWordOperand<src_mode, src_constant>(cpu);
    uint8 shift_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
    if (shift_count == 0)
      return;

    uint32 new_value = (shift_in << (32 - shift_count)) | (value >> shift_count);
    WriteDWordOperand<dst_mode, dst_constant>(cpu, new_value);

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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_XADD(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_8)
  {
    uint8 dst = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 src = ReadByteOperand<src_mode, src_constant>(cpu);
    uint8 tmp = ALUOp_Add8(&cpu->m_registers, dst, src);
    src = dst;
    dst = tmp;
    WriteByteOperand<dst_mode, dst_constant>(cpu, dst);
    WriteByteOperand<src_mode, src_constant>(cpu, src);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 dst = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 src = ReadWordOperand<src_mode, src_constant>(cpu);
    uint16 tmp = ALUOp_Add16(&cpu->m_registers, dst, src);
    src = dst;
    dst = tmp;
    WriteWordOperand<dst_mode, dst_constant>(cpu, dst);
    WriteWordOperand<src_mode, src_constant>(cpu, src);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 dst = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 src = ReadDWordOperand<src_mode, src_constant>(cpu);
    uint32 tmp = ALUOp_Add32(&cpu->m_registers, dst, src);
    src = dst;
    dst = tmp;
    WriteDWordOperand<dst_mode, dst_constant>(cpu, dst);
    WriteDWordOperand<src_mode, src_constant>(cpu, src);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_CMPXCHG(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_8)
  {
    uint8 dest = ReadByteOperand<dst_mode, dst_constant>(cpu);
    uint8 source = ReadByteOperand<src_mode, src_constant>(cpu);
    if (ALUOp_Sub8(&cpu->m_registers, cpu->m_registers.AL, dest) == 0)
    {
      // ZF should be set by the ALU op
      Assert(cpu->m_registers.EFLAGS.ZF);
      WriteByteOperand<dst_mode, dst_constant>(cpu, source);
    }
    else
    {
      // ZF should be clear by the ALU op
      // Even if the test passes the write is still issued
      Assert(!cpu->m_registers.EFLAGS.ZF);
      WriteByteOperand<dst_mode, dst_constant>(cpu, dest);
      cpu->m_registers.AL = dest;
    }
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 dest = ReadWordOperand<dst_mode, dst_constant>(cpu);
    uint16 source = ReadWordOperand<src_mode, src_constant>(cpu);
    if (ALUOp_Sub16(&cpu->m_registers, cpu->m_registers.AX, dest) == 0)
    {
      // ZF should be set by the ALU op
      Assert(cpu->m_registers.EFLAGS.ZF);
      WriteWordOperand<dst_mode, dst_constant>(cpu, source);
    }
    else
    {
      // ZF should be clear by the ALU op
      // Even if the test passes the write is still issued
      Assert(!cpu->m_registers.EFLAGS.ZF);
      WriteWordOperand<dst_mode, dst_constant>(cpu, dest);
      cpu->m_registers.AX = dest;
    }
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 dest = ReadDWordOperand<dst_mode, dst_constant>(cpu);
    uint32 source = ReadDWordOperand<src_mode, src_constant>(cpu);
    if (ALUOp_Sub32(&cpu->m_registers, cpu->m_registers.EAX, dest) == 0)
    {
      // ZF should be set by the ALU op
      Assert(cpu->m_registers.EFLAGS.ZF);
      WriteDWordOperand<dst_mode, dst_constant>(cpu, source);
    }
    else
    {
      // ZF should be clear by the ALU op
      // Even if the test passes the write is still issued
      Assert(!cpu->m_registers.EFLAGS.ZF);
      WriteDWordOperand<dst_mode, dst_constant>(cpu, dest);
      cpu->m_registers.EAX = dest;
    }
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_BSR(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_16)
  {
    uint16 mask = ReadZeroExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    if (mask != 0)
    {
      uint16 index = 0;
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
    uint32 mask = ReadZeroExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    if (mask != 0)
    {
      uint32 index = 0;
      Y_bitscanreverse(mask, &index);
      WriteDWordOperand<dst_mode, dst_constant>(cpu, index);
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_BSF(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  if (actual_size == OperandSize_16)
  {
    uint16 mask = ReadZeroExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    if (mask != 0)
    {
      uint16 index = 0;
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
    uint32 mask = ReadZeroExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    if (mask != 0)
    {
      uint32 index = 0;
      Y_bitscanforward(mask, &index);
      WriteDWordOperand<dst_mode, dst_constant>(cpu, index);
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_BTC(CPU* cpu)
{
  Execute_Operation_BTx<Operation_BTC, dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_BTR(CPU* cpu)
{
  Execute_Operation_BTx<Operation_BTR, dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_BTS(CPU* cpu)
{
  Execute_Operation_BTx<Operation_BTS, dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_BT(CPU* cpu)
{
  Execute_Operation_BTx<Operation_BT, dst_size, dst_mode, dst_constant, src_size, src_mode, src_constant>(cpu);
}

template<Operation operation, OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size,
         OperandMode src_mode, uint32 src_constant>
void NewInterpreter::Execute_Operation_BTx(CPU* cpu)
{
  static_assert(dst_mode == OperandMode_ModRM_RM, "dst_mode is modrm r/m");
  CalculateEffectiveAddress<dst_mode>(cpu);
  CalculateEffectiveAddress<src_mode>(cpu);

  // When combined with a memory operand, these instructions can access more than 16/32 bits.
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  const bool is_register_operand = cpu->idata.modrm_rm_register;
  if (actual_size == OperandSize_16)
  {
    uint16 source = ReadZeroExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    uint16 bit = source & 0xF;
    uint16 mask = (UINT16_C(1) << bit);

    uint16 in_value, out_value;
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
      int16 displacement = int16(source & 0xFFF0) / 16;
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
    uint32 source = ReadZeroExtendedDWordOperand<src_size, src_mode, src_constant>(cpu);
    uint32 bit = source & 0x1F;
    uint32 mask = (UINT32_C(1) << bit);

    uint32 in_value, out_value;
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
      int32 displacement = int32(source & 0xFFFFFFE0) / 32;
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
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<bool check_equal, typename callback>
void NewInterpreter::Execute_REP(CPU* cpu, callback cb)
{
  // We can execute this instruction as a non-rep.
  if (!cpu->idata.has_rep)
  {
    cb(cpu);
    return;
  }

  for (;;)
  {
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_INS(CPU* cpu)
{
  // TODO: Move the port number check out of the loop.
  Execute_REP<false>(cpu, [](CPU* cpu) {
    const VirtualMemoryAddress dst_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    const uint16 port_number = cpu->m_registers.DX;
    uint8 data_size;

    if (actual_size == OperandSize_8)
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
    else if (actual_size == OperandSize_16)
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
    else if (actual_size == OperandSize_32)
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_OUTS(CPU* cpu)
{
  Execute_REP<false>(cpu, [](CPU* cpu) {
    const Segment segment = cpu->idata.segment;
    const VirtualMemoryAddress src_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
    const OperandSize actual_size = (src_size == OperandSize_Count) ? cpu->idata.operand_size : src_size;
    uint16 port_number = cpu->m_registers.DX;
    uint8 data_size;

    if (actual_size == OperandSize_8)
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
    else if (actual_size == OperandSize_16)
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
    else if (actual_size == OperandSize_32)
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_SCAS(CPU* cpu)
{
  static_assert(src_size == dst_size, "operand sizes are the same");
  Execute_REP<true>(cpu, [](CPU* cpu) {
    // The ES segment cannot be overridden with a segment override prefix.
    VirtualMemoryAddress dst_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    uint8 data_size;
    if (actual_size == OperandSize_8)
    {
      uint8 lhs = cpu->m_registers.AL;
      uint8 rhs = cpu->ReadMemoryByte(Segment_ES, dst_address);
      ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
      data_size = sizeof(uint8);
    }
    else if (actual_size == OperandSize_16)
    {
      uint16 lhs = cpu->m_registers.AX;
      uint16 rhs = cpu->ReadMemoryWord(Segment_ES, dst_address);
      ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
      data_size = sizeof(uint16);
    }
    else if (actual_size == OperandSize_32)
    {
      uint32 lhs = cpu->m_registers.EAX;
      uint32 rhs = cpu->ReadMemoryDWord(Segment_ES, dst_address);
      ALUOp_Sub32(&cpu->m_registers, lhs, rhs);
      data_size = sizeof(uint32);
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_LODS(CPU* cpu)
{
  static_assert(src_size == dst_size, "operand sizes are the same");
  Execute_REP<false>(cpu, [](CPU* cpu) {
    const Segment segment = cpu->idata.segment;
    const VirtualMemoryAddress src_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    uint8 data_size;

    if (actual_size == OperandSize_8)
    {
      uint8 value = cpu->ReadMemoryByte(segment, src_address);
      cpu->m_registers.AL = value;
      data_size = sizeof(uint8);
    }
    else if (actual_size == OperandSize_16)
    {
      uint16 value = cpu->ReadMemoryWord(segment, src_address);
      cpu->m_registers.AX = value;
      data_size = sizeof(uint16);
    }
    else if (actual_size == OperandSize_32)
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_STOS(CPU* cpu)
{
  static_assert(src_size == dst_size, "operand sizes are the same");
  Execute_REP<false>(cpu, [](CPU* cpu) {
    const VirtualMemoryAddress dst_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    uint8 data_size;

    if (actual_size == OperandSize_8)
    {
      uint8 value = cpu->m_registers.AL;
      cpu->WriteMemoryByte(Segment_ES, dst_address, value);
      data_size = sizeof(uint8);
    }
    else if (actual_size == OperandSize_16)
    {
      uint16 value = cpu->m_registers.AX;
      cpu->WriteMemoryWord(Segment_ES, dst_address, value);
      data_size = sizeof(uint16);
    }
    else if (actual_size == OperandSize_32)
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_CMPS(CPU* cpu)
{
  static_assert(src_size == dst_size, "operand sizes are the same");
  Execute_REP<true>(cpu, [](CPU* cpu) {
    // The DS segment may be overridden with a segment override prefix, but the ES segment cannot be overridden.
    Segment src_segment = cpu->idata.segment;
    VirtualMemoryAddress src_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
    VirtualMemoryAddress dst_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    uint8 data_size;

    if (actual_size == OperandSize_8)
    {
      uint8 lhs = cpu->ReadMemoryByte(src_segment, src_address);
      uint8 rhs = cpu->ReadMemoryByte(Segment_ES, dst_address);
      ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
      data_size = sizeof(uint8);
    }
    else if (actual_size == OperandSize_16)
    {
      uint16 lhs = cpu->ReadMemoryWord(src_segment, src_address);
      uint16 rhs = cpu->ReadMemoryWord(Segment_ES, dst_address);
      ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
      data_size = sizeof(uint16);
    }
    else if (actual_size == OperandSize_32)
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

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_MOVS(CPU* cpu)
{
  static_assert(src_size == dst_size, "operand sizes are the same");
  Execute_REP<false>(cpu, [](CPU* cpu) {
    // The DS segment may be over-ridden with a segment override prefix, but the ES segment cannot be overridden.
    const Segment src_segment = cpu->idata.segment;
    const VirtualMemoryAddress src_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.SI) : cpu->m_registers.ESI;
    const VirtualMemoryAddress dst_address =
      (cpu->idata.address_size == AddressSize_16) ? ZeroExtend32(cpu->m_registers.DI) : cpu->m_registers.EDI;
    const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
    uint8 data_size;

    if (actual_size == OperandSize_8)
    {
      uint8 value = cpu->ReadMemoryByte(src_segment, src_address);
      cpu->WriteMemoryByte(Segment_ES, dst_address, value);
      data_size = sizeof(uint8);
    }
    else if (actual_size == OperandSize_16)
    {
      uint16 value = cpu->ReadMemoryWord(src_segment, src_address);
      cpu->WriteMemoryWord(Segment_ES, dst_address, value);
      data_size = sizeof(uint16);
    }
    else if (actual_size == OperandSize_32)
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

template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size,
         OperandMode src_mode, uint32 src_constant>
void NewInterpreter::Execute_Operation_CMOVcc(CPU* cpu)
{
  const OperandSize actual_size = (dst_size == OperandSize_Count) ? cpu->idata.operand_size : dst_size;
  CalculateEffectiveAddress<src_mode>(cpu);
  CalculateEffectiveAddress<dst_mode>(cpu);

  // NOTE: Memory access is performed even if the predicate does not hold.
  bool do_move = TestJumpCondition<condition>(cpu);
  if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<src_mode, src_constant>(cpu);
    if (do_move)
      WriteWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<src_mode, src_constant>(cpu);
    if (do_move)
      WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant>
void NewInterpreter::Execute_Operation_SETcc(CPU* cpu)
{
  CalculateEffectiveAddress<dst_mode>(cpu);

  bool flag = TestJumpCondition<condition>(cpu);
  WriteByteOperand<dst_mode, dst_constant>(cpu, BoolToUInt8(flag));
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_MOV_TR(CPU* cpu)
{
  static_assert((src_mode == OperandMode_ModRM_TestRegister && dst_mode == OperandMode_ModRM_RM) ||
                  (src_mode == OperandMode_ModRM_RM && dst_mode == OperandMode_ModRM_TestRegister),
                "loading or storing debug register");
  static_assert(src_size == OperandSize_32 && dst_size == OperandSize_32, "source sizes are 32-bits");

  CalculateEffectiveAddress<src_mode>(cpu);
  CalculateEffectiveAddress<dst_mode>(cpu);

  // Requires privilege level zero
  if ((cpu->InProtectedMode() && cpu->GetCPL() != 0) || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // TODO: Is this correct?
  DebugAssert(cpu->idata.modrm_rm_register);
  if (!cpu->idata.modrm_rm_register)
    RaiseInvalidOpcode(cpu);

  // Load test register
  uint8 tr_index = cpu->idata.GetModRM_Reg();
  if constexpr (dst_mode == OperandMode_ModRM_TestRegister)
  {
    uint32 value = ReadDWordOperand<src_mode, src_constant>(cpu);

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
    uint32 value;
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

    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_MOV_DR(CPU* cpu)
{
  static_assert((src_mode == OperandMode_ModRM_DebugRegister && dst_mode == OperandMode_ModRM_RM) ||
                  (src_mode == OperandMode_ModRM_RM && dst_mode == OperandMode_ModRM_DebugRegister),
                "loading or storing debug register");
  static_assert(src_size == OperandSize_32 && dst_size == OperandSize_32, "source sizes are 32-bits");

  CalculateEffectiveAddress<src_mode>(cpu);
  CalculateEffectiveAddress<dst_mode>(cpu);

  // Requires privilege level zero
  if ((cpu->InProtectedMode() && cpu->GetCPL() != 0) || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // TODO Validation:
  // #UD If CR4.DE[bit 3] = 1 (debug extensions) and a MOV instruction is executed involving DR4 or DR5.
  // #DB If any debug register is accessed while the DR7.GD[bit 13] = 1.

  // TODO: Is this correct?
  DebugAssert(cpu->idata.modrm_rm_register);
  if (!cpu->idata.modrm_rm_register)
    RaiseInvalidOpcode(cpu);

  uint8 dr_index = cpu->idata.GetModRM_Reg();
  if constexpr (dst_mode == OperandMode_ModRM_DebugRegister)
  {
    // Load debug register
    uint32 value = ReadDWordOperand<src_mode, src_constant>(cpu);
    cpu->LoadSpecialRegister(static_cast<Reg32>(Reg32_DR0 + dr_index), value);
  }
  else if constexpr (src_mode == OperandMode_ModRM_DebugRegister)
  {
    // Store debug register
    uint32 value = cpu->m_registers.reg32[Reg32_DR0 + dr_index];
    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
}

template<OperandSize dst_size, OperandMode dst_mode, uint32 dst_constant, OperandSize src_size, OperandMode src_mode,
         uint32 src_constant>
void NewInterpreter::Execute_Operation_MOV_CR(CPU* cpu)
{
  static_assert((src_mode == OperandMode_ModRM_ControlRegister && dst_mode == OperandMode_ModRM_RM) ||
                  (src_mode == OperandMode_ModRM_RM && dst_mode == OperandMode_ModRM_ControlRegister),
                "loading or storing control register");
  static_assert(src_size == OperandSize_32 && dst_size == OperandSize_32, "source sizes are 32-bits");

  CalculateEffectiveAddress<src_mode>(cpu);
  CalculateEffectiveAddress<dst_mode>(cpu);

  // Requires privilege level zero
  if ((cpu->InProtectedMode() && cpu->GetCPL() != 0) || cpu->InVirtual8086Mode())
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // TODO: Is this correct?
  // TODO: No, it ignores the mod bits
  DebugAssert(cpu->idata.modrm_rm_register);
  if (!cpu->idata.modrm_rm_register)
    RaiseInvalidOpcode(cpu);

  // Load control register
  if constexpr (dst_mode == OperandMode_ModRM_ControlRegister)
  {
    // Validate selected register
    uint32 value = ReadDWordOperand<src_mode, src_constant>(cpu);
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
  else if constexpr (src_mode == OperandMode_ModRM_ControlRegister)
  {
    // Validate selected register
    uint32 value;
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
        value = cpu->m_registers.CR4;
        break;
      default:
        cpu->RaiseException(Interrupt_InvalidOpcode);
        return;
    }

    WriteDWordOperand<dst_mode, dst_constant>(cpu, value);
  }
}

void NewInterpreter::Execute_Operation_WBINVD(CPU* cpu)
{
  if (cpu->InVirtual8086Mode() || (cpu->InProtectedMode() && cpu->GetCPL() != 0))
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // Log_WarningPrintf("WBINVD instruction");
}

void NewInterpreter::Execute_Operation_WAIT(CPU* cpu)
{
  cpu->CheckFloatingPointException();
}

void NewInterpreter::Execute_Operation_RDTSC(CPU* cpu)
{
  // TSD flag in CR4 controls whether this is privileged or unprivileged
  // Log_WarningPrintf("RDTSC instruction");
  cpu->m_registers.EDX = 0;
  cpu->m_registers.EAX = 0;
}

void NewInterpreter::Execute_Operation_LOADALL_286(CPU* cpu)
{
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

  if (cpu->m_model != MODEL_286)
  {
    cpu->RaiseException(Interrupt_InvalidOpcode);
    return;
  }

  // Check CPL = 0, GPF?
  if (cpu->GetCPL() != 0)
  {
    cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

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
} // namespace CPU_X86

#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif