#include "YBaseLib/Endian.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/String.h"
#include "pce/bus.h"
#include "pce/cpu_8086/cpu.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"

#ifdef Y_COMPILER_MSVC
#include <intrin.h>
#endif

namespace CPU_8086 {
bool TRACE_EXECUTION = false;
u16 TRACE_EXECUTION_LAST_EIP = 0;

class Instructions
{
public:
  static inline void RaiseInvalidOpcode(CPU* cpu) { cpu->PrintCurrentStateAndInstruction("Invalid opcode raised at "); }

  static inline void FetchModRM(CPU* cpu) { cpu->idata.modrm = cpu->FetchInstructionByte(); }

  static inline CycleCount MemoryCycles(CPU* cpu, int cycles_16bit, int cycles_8bit)
  {
    return cpu->m_data_bus_is_8bit ? cycles_8bit : cycles_16bit;
  }

  static inline CycleCount RMCycles(CPU* cpu, int cycles_reg, int cycles_16bit, int cycles_8bit)
  {
    return cpu->idata.ModRM_RM_IsReg() ? cycles_reg : (cpu->m_data_bus_is_8bit ? cycles_8bit : cycles_16bit);
  }

  template<OperandMode dst_mode, OperandMode src_mode>
  static inline CycleCount ALUCycles(CPU* cpu)
  {
    if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
      return 4;
    else if constexpr (dst_mode == OperandMode_ModRM_RM && src_mode == OperandMode_Immediate)
      return RMCycles(cpu, 4, 17, 25);
    else if constexpr (dst_mode == OperandMode_ModRM_RM)
      return RMCycles(cpu, 3, 16, 24);
    else if constexpr (src_mode == OperandMode_ModRM_RM)
      return RMCycles(cpu, 3, 9, 13);
    else
      static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
  }

  template<OperandMode val_mode>
  static inline CycleCount ShiftCycles(CPU* cpu)
  {
    if constexpr (val_mode == OperandMode_Constant)
      return RMCycles(cpu, 2, 15, 23);
    else
      return RMCycles(cpu, 8, 20, 28) + 4;
  }

  template<OperandSize op_size, OperandMode op_mode, u32 op_constant>
  static inline void FetchImmediate(CPU* cpu)
  {
    switch (op_mode)
    {
      case OperandMode_Immediate:
      {
        if constexpr (op_size == OperandSize_8)
          cpu->idata.imm8 = cpu->FetchInstructionByte();
        else if constexpr (op_size == OperandSize_16)
          cpu->idata.imm16 = cpu->FetchInstructionWord();
      }
      break;

      case OperandMode_Immediate2:
      {
        if constexpr (op_size == OperandSize_8)
          cpu->idata.imm2_8 = cpu->FetchInstructionByte();
        else if constexpr (op_size == OperandSize_16)
          cpu->idata.imm2_16 = cpu->FetchInstructionWord();
      }
      break;

      case OperandMode_Relative:
      {
        if constexpr (op_size == OperandSize_8)
          cpu->idata.disp16 = SignExtend16(cpu->FetchInstructionByte());
        else if constexpr (op_size == OperandSize_16)
          cpu->idata.disp16 = cpu->FetchInstructionWord();
      }
      break;

      case OperandMode_Memory:
      {
        cpu->idata.disp16 = cpu->FetchInstructionWord();
      }
      break;

      case OperandMode_FarAddress:
      {
        cpu->idata.disp16 = cpu->FetchInstructionWord();
        cpu->idata.imm16 = cpu->FetchInstructionWord();
      }
      break;

      case OperandMode_ModRM_RM:
      {
        u8 index = ((cpu->idata.modrm >> 6) << 3) | (cpu->idata.modrm & 7);
        switch (index & 31)
        {
          case 8:
          case 9:
          case 10:
          case 11:
          case 12:
          case 13:
          case 14:
          case 15:
            cpu->idata.disp16 = SignExtend16(cpu->FetchInstructionByte());
            break;

          case 6:
          case 16:
          case 17:
          case 18:
          case 19:
          case 20:
          case 21:
          case 22:
          case 23:
            cpu->idata.disp16 = cpu->FetchInstructionWord();
            break;
        }
      }
      break;
    }
  }

  template<OperandMode op_mode>
  static inline void CalculateEffectiveAddress(CPU* cpu)
  {
    if constexpr (op_mode == OperandMode_ModRM_RM)
    {
      enum : CycleCount
      {
        CYCLES_DISP = 6,
        CYCLES_BASE = 5,
        CYCLES_DISP_INDEX = 9,
        CYCLES_BASE_INDEX_DI = 7,
        CYCLES_BASE_INDEX_SI = 8,
        CYCLES_BASE_INDEX_DISP_DI = 11,
        CYCLES_BASE_INDEX_DISP_SI = 12,
        CYCLES_SEG_OVERRIDE = 2
      };

      u8 index = ((cpu->idata.modrm >> 6) << 3) | (cpu->idata.modrm & 7);
      switch (index & 31)
      {
        case 0:
          cpu->m_effective_address = u16(cpu->m_registers.BX + cpu->m_registers.SI);
          cpu->AddCycles(CYCLES_BASE_INDEX_DI);
          break;
        case 1:
          cpu->m_effective_address = u16(cpu->m_registers.BX + cpu->m_registers.DI);
          cpu->AddCycles(CYCLES_BASE_INDEX_SI);
          break;
        case 2:
          cpu->m_effective_address = u16(cpu->m_registers.BP + cpu->m_registers.SI);
          if (!cpu->idata.has_segment_override)
            cpu->idata.segment = Segment_SS;
          cpu->AddCycles(CYCLES_BASE_INDEX_SI);
          break;
        case 3:
          cpu->m_effective_address = u16(cpu->m_registers.BP + cpu->m_registers.DI);
          if (!cpu->idata.has_segment_override)
            cpu->idata.segment = Segment_SS;
          cpu->AddCycles(CYCLES_BASE_INDEX_DI);
          break;
        case 4:
          cpu->m_effective_address = cpu->m_registers.SI;
          cpu->AddCycles(CYCLES_BASE);
          break;
        case 5:
          cpu->m_effective_address = cpu->m_registers.DI;
          cpu->AddCycles(CYCLES_BASE);
          break;
        case 6:
          cpu->m_effective_address = cpu->idata.disp16;
          cpu->AddCycles(CYCLES_DISP);
          break;
        case 7:
          cpu->m_effective_address = cpu->m_registers.BX;
          cpu->AddCycles(CYCLES_BASE);
          break;
        case 8:
          cpu->m_effective_address = u16(cpu->m_registers.BX + cpu->m_registers.SI + cpu->idata.disp16);
          cpu->AddCycles(CYCLES_BASE_INDEX_DISP_DI);
          break;
        case 9:
          cpu->m_effective_address = u16(cpu->m_registers.BX + cpu->m_registers.DI + cpu->idata.disp16);
          cpu->AddCycles(CYCLES_BASE_INDEX_DISP_SI);
          break;
        case 10:
          cpu->m_effective_address = u16(cpu->m_registers.BP + cpu->m_registers.SI + cpu->idata.disp16);
          if (!cpu->idata.has_segment_override)
            cpu->idata.segment = Segment_SS;
          cpu->AddCycles(CYCLES_BASE_INDEX_DISP_SI);
          break;
        case 11:
          cpu->m_effective_address = u16(cpu->m_registers.BP + cpu->m_registers.DI + cpu->idata.disp16);
          if (!cpu->idata.has_segment_override)
            cpu->idata.segment = Segment_SS;
          cpu->AddCycles(CYCLES_BASE_INDEX_DISP_DI);
          break;
        case 12:
          cpu->m_effective_address = u16(cpu->m_registers.SI + cpu->idata.disp16);
          cpu->AddCycles(CYCLES_DISP_INDEX);
          break;
        case 13:
          cpu->m_effective_address = u16(cpu->m_registers.DI + cpu->idata.disp16);
          cpu->AddCycles(CYCLES_DISP_INDEX);
          break;
        case 14:
          cpu->m_effective_address = u16(cpu->m_registers.BP + cpu->idata.disp16);
          if (!cpu->idata.has_segment_override)
            cpu->idata.segment = Segment_SS;
          cpu->AddCycles(CYCLES_DISP_INDEX);
          break;
        case 15:
          cpu->m_effective_address = u16(cpu->m_registers.BX + cpu->idata.disp16);
          cpu->AddCycles(CYCLES_DISP_INDEX);
          break;
        case 16:
          cpu->m_effective_address = u16(cpu->m_registers.BX + cpu->m_registers.SI + cpu->idata.disp16);
          cpu->AddCycles(CYCLES_BASE_INDEX_DISP_DI);
          break;
        case 17:
          cpu->m_effective_address = u16(cpu->m_registers.BX + cpu->m_registers.DI + cpu->idata.disp16);
          cpu->AddCycles(CYCLES_BASE_INDEX_DISP_SI);
          break;
        case 18:
          cpu->m_effective_address = u16(cpu->m_registers.BP + cpu->m_registers.SI + cpu->idata.disp16);
          if (!cpu->idata.has_segment_override)
            cpu->idata.segment = Segment_SS;
          cpu->AddCycles(CYCLES_BASE_INDEX_DISP_SI);
          break;
        case 19:
          cpu->m_effective_address = u16(cpu->m_registers.BP + cpu->m_registers.DI + cpu->idata.disp16);
          if (!cpu->idata.has_segment_override)
            cpu->idata.segment = Segment_SS;
          cpu->AddCycles(CYCLES_BASE_INDEX_DISP_DI);
          break;
        case 20:
          cpu->m_effective_address = u16(cpu->m_registers.SI + cpu->idata.disp16);
          cpu->AddCycles(CYCLES_DISP_INDEX);
          break;
        case 21:
          cpu->m_effective_address = u16(cpu->m_registers.DI + cpu->idata.disp16);
          cpu->AddCycles(CYCLES_DISP_INDEX);
          break;
        case 22:
          cpu->m_effective_address = u16(cpu->m_registers.BP + cpu->idata.disp16);
          if (!cpu->idata.has_segment_override)
            cpu->idata.segment = Segment_SS;
          cpu->AddCycles(CYCLES_DISP_INDEX);
          break;
        case 23:
          cpu->m_effective_address = u16(cpu->m_registers.BX + cpu->idata.disp16);
          cpu->AddCycles(CYCLES_DISP_INDEX);
          break;
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
          cpu->m_effective_address = ZeroExtend16(index & 0x07);
          break;
      }

      if (cpu->idata.has_segment_override)
        cpu->AddCycles(CYCLES_SEG_OVERRIDE);
    }
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline VirtualMemoryAddress CalculateJumpTarget(CPU* cpu)
  {
    static_assert(dst_mode == OperandMode_Relative || dst_mode == OperandMode_ModRM_RM,
                  "Operand mode is relative or indirect");

    if constexpr (dst_mode == OperandMode_Relative)
      return cpu->m_registers.IP + cpu->idata.disp16;
    else if constexpr (dst_mode == OperandMode_ModRM_RM)
      return ReadZeroExtendedWordOperand<dst_size, dst_mode, dst_constant>(cpu);
  }

  template<OperandMode mode, u32 constant>
  static inline u8 ReadByteOperand(CPU* cpu)
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
      case OperandMode_Memory:
        return cpu->ReadMemoryByte(cpu->idata.segment, cpu->idata.disp16);
      case OperandMode_ModRM_RM:
      {
        if (cpu->idata.ModRM_RM_IsReg())
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
  static inline u16 ReadWordOperand(CPU* cpu)
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
        return cpu->ReadMemoryWord(cpu->idata.segment, cpu->idata.disp16);
      case OperandMode_ModRM_RM:
      {
        if (cpu->idata.ModRM_RM_IsReg())
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

  template<OperandSize size, OperandMode mode, u32 constant>
  static inline u16 ReadSignExtendedWordOperand(CPU* cpu)
  {
    switch (size)
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
            value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->idata.disp16);
            break;
          case OperandMode_ModRM_Reg:
            value = cpu->m_registers.reg8[cpu->idata.GetModRM_Reg()];
            break;
          case OperandMode_ModRM_RM:
            if (cpu->idata.ModRM_RM_IsReg())
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
  static u16 ReadZeroExtendedWordOperand(CPU* cpu)
  {
    switch (size)
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
            value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->idata.disp16);
            break;
          case OperandMode_ModRM_Reg:
            value = cpu->m_registers.reg8[cpu->idata.GetModRM_Reg()];
            break;
          case OperandMode_ModRM_RM:
            if (cpu->idata.ModRM_RM_IsReg())
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

  template<OperandMode mode, u32 constant>
  static inline void WriteByteOperand(CPU* cpu, u8 value)
  {
    switch (mode)
    {
      case OperandMode_Register:
        cpu->m_registers.reg8[constant] = value;
        break;
      case OperandMode_Memory:
        cpu->WriteMemoryByte(cpu->idata.segment, cpu->idata.disp16, value);
        break;

      case OperandMode_ModRM_RM:
      {
        if (cpu->idata.ModRM_RM_IsReg())
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
  static inline void WriteWordOperand(CPU* cpu, u16 value)
  {
    switch (mode)
    {
      case OperandMode_Register:
        cpu->m_registers.reg16[constant] = value;
        break;
      case OperandMode_Memory:
        cpu->WriteMemoryWord(cpu->idata.segment, cpu->idata.disp16, value);
        break;

      case OperandMode_ModRM_RM:
      {
        if (cpu->idata.ModRM_RM_IsReg())
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

  template<OperandMode mode>
  static inline void ReadFarAddressOperand(CPU* cpu, u16* segment_selector, VirtualMemoryAddress* address)
  {
    // Can either be far immediate, or memory
    switch (mode)
    {
      case OperandMode_FarAddress:
      {
        *address = cpu->idata.disp16;
        *segment_selector = cpu->idata.imm16;
      }
      break;

      case OperandMode_Memory:
      {
        *address = cpu->ReadMemoryWord(cpu->idata.segment, cpu->idata.disp16);
        *segment_selector = cpu->ReadMemoryWord(cpu->idata.segment, cpu->idata.disp16 + 2);
      }
      break;

      case OperandMode_ModRM_RM:
      {
        *address = cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_effective_address);
        *segment_selector = cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_effective_address + 2);
      }
      break;
      default:
        DebugUnreachableCode();
        break;
    }
  }

  template<JumpCondition condition>
  static inline bool TestJumpCondition(CPU* cpu)
  {
    switch (condition)
    {
      case JumpCondition_Always:
        return true;

      case JumpCondition_Overflow:
        return cpu->m_registers.FLAGS.OF;

      case JumpCondition_NotOverflow:
        return !cpu->m_registers.FLAGS.OF;

      case JumpCondition_Sign:
        return cpu->m_registers.FLAGS.SF;

      case JumpCondition_NotSign:
        return !cpu->m_registers.FLAGS.SF;

      case JumpCondition_Equal:
        return cpu->m_registers.FLAGS.ZF;

      case JumpCondition_NotEqual:
        return !cpu->m_registers.FLAGS.ZF;

      case JumpCondition_Below:
        return cpu->m_registers.FLAGS.CF;

      case JumpCondition_AboveOrEqual:
        return !cpu->m_registers.FLAGS.CF;

      case JumpCondition_BelowOrEqual:
        return (cpu->m_registers.FLAGS.CF | cpu->m_registers.FLAGS.ZF);

      case JumpCondition_Above:
        return !(cpu->m_registers.FLAGS.CF | cpu->m_registers.FLAGS.ZF);

      case JumpCondition_Less:
        return (cpu->m_registers.FLAGS.SF != cpu->m_registers.FLAGS.OF);

      case JumpCondition_GreaterOrEqual:
        return (cpu->m_registers.FLAGS.SF == cpu->m_registers.FLAGS.OF);

      case JumpCondition_LessOrEqual:
        return (cpu->m_registers.FLAGS.ZF || (cpu->m_registers.FLAGS.SF != cpu->m_registers.FLAGS.OF));

      case JumpCondition_Greater:
        return (!cpu->m_registers.FLAGS.ZF && (cpu->m_registers.FLAGS.SF == cpu->m_registers.FLAGS.OF));

      case JumpCondition_Parity:
        return cpu->m_registers.FLAGS.PF;

      case JumpCondition_NotParity:
        return !cpu->m_registers.FLAGS.PF;

      case JumpCondition_CXZero:
        return (cpu->m_registers.CX == 0);

      default:
        Panic("Unhandled jump condition");
        return false;
    }
  }

  static constexpr bool IsSign(u8 value) { return !!(value >> 7); }
  static constexpr bool IsSign(u16 value) { return !!(value >> 15); }
  static constexpr bool IsSign(u32 value) { return !!(value >> 31); }
  template<typename T>
  static constexpr bool IsZero(T value)
  {
    return (value == 0);
  }
  template<typename T>
  static constexpr bool IsAdjust(T old_value, T new_value)
  {
    return (old_value & 0xF) < (new_value & 0xF);
  }

#ifdef Y_COMPILER_MSVC
  template<typename T>
  static constexpr bool IsParity(T value)
  {
    return static_cast<bool>(~_mm_popcnt_u32(static_cast<u32>(value & 0xFF)) & 1);
  }
#else
  template<typename T>
  static constexpr bool IsParity(T value)
  {
    return static_cast<bool>(~Y_popcnt(static_cast<u8>(value & 0xFF)) & 1);
  }
#endif

// GCC seems to be emitting bad code for our FlagAccess class..
#define SET_FLAG(regs, flag, expr)                                                                                     \
  do                                                                                                                   \
  {                                                                                                                    \
    if ((expr))                                                                                                        \
    {                                                                                                                  \
      (regs)->FLAGS.bits |= (Flag_##flag);                                                                             \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      (regs)->FLAGS.bits &= ~(Flag_##flag);                                                                            \
    }                                                                                                                  \
  } while (0)
  //#define SET_FLAG(regs, flag, expr) (regs)->FLAGS.flag = (expr)

  static inline u8 ALUOp_Add8(CPU::Registers* registers, u8 lhs, u8 rhs)
  {
    u16 old_value = lhs;
    u16 add_value = rhs;
    u16 new_value = old_value + add_value;
    u8 out_value = u8(new_value & 0xFF);

    SET_FLAG(registers, CF, ((new_value & 0xFF00) != 0));
    SET_FLAG(registers, OF, ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x80) == 0x80));
    SET_FLAG(registers, AF, (((old_value ^ add_value ^ new_value) & 0x10) == 0x10));
    SET_FLAG(registers, SF, IsSign(out_value));
    SET_FLAG(registers, ZF, IsZero(out_value));
    SET_FLAG(registers, PF, IsParity(out_value));

    return out_value;
  }

  static inline u8 ALUOp_Adc8(CPU::Registers* registers, u8 lhs, u8 rhs)
  {
    u16 old_value = lhs;
    u16 add_value = rhs;
    u16 carry_in = (registers->FLAGS.CF) ? 1 : 0;
    u16 new_value = old_value + add_value + carry_in;
    u8 out_value = u8(new_value & 0xFF);

    SET_FLAG(registers, CF, ((new_value & 0xFF00) != 0));
    SET_FLAG(registers, OF, ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x80) == 0x80));
    SET_FLAG(registers, AF, (((old_value ^ add_value ^ new_value) & 0x10) == 0x10));
    SET_FLAG(registers, SF, IsSign(out_value));
    SET_FLAG(registers, ZF, IsZero(out_value));
    SET_FLAG(registers, PF, IsParity(out_value));

    return out_value;
  }

  static inline u8 ALUOp_Sub8(CPU::Registers* registers, u8 lhs, u8 rhs)
  {
    u16 old_value = lhs;
    u16 sub_value = rhs;
    u16 new_value = old_value - sub_value;
    u8 out_value = u8(new_value & 0xFF);

    SET_FLAG(registers, CF, ((new_value & 0xFF00) != 0));
    SET_FLAG(registers, OF, ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x80) == 0x80));
    SET_FLAG(registers, AF, (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10));
    SET_FLAG(registers, SF, IsSign(out_value));
    SET_FLAG(registers, ZF, IsZero(out_value));
    SET_FLAG(registers, PF, IsParity(out_value));

    return out_value;
  }

  static inline u8 ALUOp_Sbb8(CPU::Registers* registers, u8 lhs, u8 rhs)
  {
    u16 old_value = lhs;
    u16 sub_value = rhs;
    u16 carry_in = registers->FLAGS.CF ? 1 : 0;
    u16 new_value = old_value - sub_value - carry_in;
    u8 out_value = u8(new_value & 0xFF);

    SET_FLAG(registers, CF, ((new_value & 0xFF00) != 0));
    SET_FLAG(registers, OF, ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x80) == 0x80));
    SET_FLAG(registers, AF, (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10));
    SET_FLAG(registers, SF, IsSign(out_value));
    SET_FLAG(registers, ZF, IsZero(out_value));
    SET_FLAG(registers, PF, IsParity(out_value));

    return out_value;
  }

  static inline u16 ALUOp_Add16(CPU::Registers* registers, u16 lhs, u16 rhs)
  {
    u32 old_value = lhs;
    u32 add_value = rhs;
    u32 new_value = old_value + add_value;
    u16 out_value = u16(new_value & 0xFFFF);

    SET_FLAG(registers, CF, ((new_value & 0xFFFF0000) != 0));
    SET_FLAG(registers, OF, ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x8000) == 0x8000));
    SET_FLAG(registers, AF, (((old_value ^ add_value ^ new_value) & 0x10) == 0x10));
    SET_FLAG(registers, SF, IsSign(out_value));
    SET_FLAG(registers, ZF, IsZero(out_value));
    SET_FLAG(registers, PF, IsParity(out_value));

    return out_value;
  }

  static inline u16 ALUOp_Adc16(CPU::Registers* registers, u16 lhs, u16 rhs)
  {
    u32 old_value = lhs;
    u32 add_value = rhs;
    u32 carry_in = (registers->FLAGS.CF) ? 1 : 0;
    u32 new_value = old_value + add_value + carry_in;
    u16 out_value = u16(new_value & 0xFFFF);

    SET_FLAG(registers, CF, ((new_value & 0xFFFF0000) != 0));
    SET_FLAG(registers, OF, ((((new_value ^ old_value) & (new_value ^ add_value)) & 0x8000) == 0x8000));
    SET_FLAG(registers, AF, (((old_value ^ add_value ^ new_value) & 0x10) == 0x10));
    SET_FLAG(registers, SF, IsSign(out_value));
    SET_FLAG(registers, ZF, IsZero(out_value));
    SET_FLAG(registers, PF, IsParity(out_value));

    return out_value;
  }

  static inline u16 ALUOp_Sub16(CPU::Registers* registers, u16 lhs, u16 rhs)
  {
    u32 old_value = lhs;
    u32 sub_value = rhs;
    u32 new_value = old_value - sub_value;
    u16 out_value = u16(new_value & 0xFFFF);

    SET_FLAG(registers, CF, ((new_value & 0xFFFF0000) != 0));
    SET_FLAG(registers, OF, ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x8000) == 0x8000));
    SET_FLAG(registers, AF, (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10));
    SET_FLAG(registers, SF, IsSign(out_value));
    SET_FLAG(registers, ZF, IsZero(out_value));
    SET_FLAG(registers, PF, IsParity(out_value));

    return out_value;
  }

  static inline u16 ALUOp_Sbb16(CPU::Registers* registers, u16 lhs, u16 rhs)
  {
    u32 old_value = lhs;
    u32 sub_value = rhs;
    u32 carry_in = registers->FLAGS.CF ? 1 : 0;
    u32 new_value = old_value - sub_value - carry_in;
    u16 out_value = u16(new_value & 0xFFFF);

    SET_FLAG(registers, CF, ((new_value & 0xFFFF0000) != 0));
    SET_FLAG(registers, OF, ((((new_value ^ old_value) & (old_value ^ sub_value)) & 0x8000) == 0x8000));
    SET_FLAG(registers, AF, (((old_value ^ sub_value ^ new_value) & 0x10) == 0x10));
    SET_FLAG(registers, SF, IsSign(out_value));
    SET_FLAG(registers, ZF, IsZero(out_value));
    SET_FLAG(registers, PF, IsParity(out_value));

    return out_value;
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_ADD(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    if constexpr (dst_size == OperandSize_8)
    {
      u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
      u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
      u8 new_value = ALUOp_Add8(&cpu->m_registers, lhs, rhs);
      WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
      u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
      u16 new_value = ALUOp_Add16(&cpu->m_registers, lhs, rhs);
      WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
    }

    cpu->AddCycles(ALUCycles<dst_mode, src_mode>(cpu));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_ADC(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    if constexpr (dst_size == OperandSize_8)
    {
      u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
      u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
      u8 new_value = ALUOp_Adc8(&cpu->m_registers, lhs, rhs);
      WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
      u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
      u16 new_value = ALUOp_Adc16(&cpu->m_registers, lhs, rhs);
      WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
    }

    cpu->AddCycles(ALUCycles<dst_mode, src_mode>(cpu));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_SUB(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    if constexpr (dst_size == OperandSize_8)
    {
      u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
      u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
      u8 new_value = ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
      WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
      u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
      u16 new_value = ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
      WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
    }

    cpu->AddCycles(ALUCycles<dst_mode, src_mode>(cpu));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_SBB(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    if constexpr (dst_size == OperandSize_8)
    {
      u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
      u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
      u8 new_value = ALUOp_Sbb8(&cpu->m_registers, lhs, rhs);
      WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
      u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
      u16 new_value = ALUOp_Sbb16(&cpu->m_registers, lhs, rhs);
      WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);
    }

    cpu->AddCycles(ALUCycles<dst_mode, src_mode>(cpu));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_CMP(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    // Implemented as subtract but discarding the result
    if constexpr (dst_size == OperandSize_8)
    {
      u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
      u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
      ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
      u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
      ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
    }

    if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
      cpu->AddCycles(4);
    else if constexpr (dst_mode == OperandMode_ModRM_RM && src_mode == OperandMode_Immediate)
      cpu->AddCycles(RMCycles(cpu, 4, 10, 14));
    else if constexpr (dst_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 3, 9, 13));
    else if constexpr (src_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 3, 9, 13));
    else
      static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_AND(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    bool sf, zf, pf;
    if constexpr (dst_size == OperandSize_8)
    {
      u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
      u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
      u8 new_value = lhs & rhs;
      WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);

      sf = IsSign(new_value);
      zf = IsZero(new_value);
      pf = IsParity(new_value);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
      u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
      u16 new_value = lhs & rhs;
      WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

      sf = IsSign(new_value);
      zf = IsZero(new_value);
      pf = IsParity(new_value);
    }

    // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF
    // flag is undefined.
    SET_FLAG(&cpu->m_registers, OF, false);
    SET_FLAG(&cpu->m_registers, CF, false);
    SET_FLAG(&cpu->m_registers, SF, sf);
    SET_FLAG(&cpu->m_registers, ZF, zf);
    SET_FLAG(&cpu->m_registers, PF, pf);
    SET_FLAG(&cpu->m_registers, AF, false);

    cpu->AddCycles(ALUCycles<dst_mode, src_mode>(cpu));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_OR(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    bool sf, zf, pf;
    if constexpr (dst_size == OperandSize_8)
    {
      u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
      u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
      u8 new_value = lhs | rhs;
      WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);

      sf = IsSign(new_value);
      zf = IsZero(new_value);
      pf = IsParity(new_value);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
      u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
      u16 new_value = lhs | rhs;
      WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

      sf = IsSign(new_value);
      zf = IsZero(new_value);
      pf = IsParity(new_value);
    }

    // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF
    // flag is undefined.
    SET_FLAG(&cpu->m_registers, OF, false);
    SET_FLAG(&cpu->m_registers, CF, false);
    SET_FLAG(&cpu->m_registers, SF, sf);
    SET_FLAG(&cpu->m_registers, ZF, zf);
    SET_FLAG(&cpu->m_registers, PF, pf);
    SET_FLAG(&cpu->m_registers, AF, false);

    cpu->AddCycles(ALUCycles<dst_mode, src_mode>(cpu));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_XOR(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    bool sf, zf, pf;
    if constexpr (dst_size == OperandSize_8)
    {
      u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
      u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
      u8 new_value = lhs ^ rhs;
      WriteByteOperand<dst_mode, dst_constant>(cpu, new_value);

      sf = IsSign(new_value);
      zf = IsZero(new_value);
      pf = IsParity(new_value);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
      u16 rhs = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
      u16 new_value = lhs ^ rhs;
      WriteWordOperand<dst_mode, dst_constant>(cpu, new_value);

      sf = IsSign(new_value);
      zf = IsZero(new_value);
      pf = IsParity(new_value);
    }

    // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF
    // flag is undefined.
    SET_FLAG(&cpu->m_registers, OF, false);
    SET_FLAG(&cpu->m_registers, CF, false);
    SET_FLAG(&cpu->m_registers, SF, sf);
    SET_FLAG(&cpu->m_registers, ZF, zf);
    SET_FLAG(&cpu->m_registers, PF, pf);
    SET_FLAG(&cpu->m_registers, AF, false);

    cpu->AddCycles(ALUCycles<dst_mode, src_mode>(cpu));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_TEST(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    bool sf, zf, pf;
    if constexpr (dst_size == OperandSize_8)
    {
      u8 lhs = ReadByteOperand<dst_mode, dst_constant>(cpu);
      u8 rhs = ReadByteOperand<src_mode, src_constant>(cpu);
      u8 new_value = lhs & rhs;

      sf = IsSign(new_value);
      zf = IsZero(new_value);
      pf = IsParity(new_value);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 lhs = ReadWordOperand<dst_mode, dst_constant>(cpu);
      u16 rhs = ReadWordOperand<src_mode, src_constant>(cpu);
      u16 new_value = lhs & rhs;

      sf = IsSign(new_value);
      zf = IsZero(new_value);
      pf = IsParity(new_value);
    }

    // The OF and CF flags are cleared; the SF, ZF, and PF flags are set according to the result. The state of the AF
    // flag is undefined.
    SET_FLAG(&cpu->m_registers, OF, false);
    SET_FLAG(&cpu->m_registers, CF, false);
    SET_FLAG(&cpu->m_registers, SF, sf);
    SET_FLAG(&cpu->m_registers, ZF, zf);
    SET_FLAG(&cpu->m_registers, PF, pf);
    SET_FLAG(&cpu->m_registers, AF, false);

    if constexpr (src_mode == OperandMode_Immediate && dst_mode == OperandMode_Register && dst_constant == Reg16_AX)
      cpu->AddCycles(4);
    else if constexpr (src_mode == OperandMode_Immediate)
      cpu->AddCycles(RMCycles(cpu, 5, 11, 13));
    else if constexpr (dst_mode == OperandMode_ModRM_RM || src_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 3, 9, 13));
    else
      static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_MOV(CPU* cpu)
  {
    static_assert(dst_size == src_size, "dst_size == src_size");
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    if constexpr (dst_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<src_mode, src_constant>(cpu);
      WriteByteOperand<dst_mode, dst_constant>(cpu, value);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<src_mode, src_constant>(cpu);
      WriteWordOperand<dst_mode, dst_constant>(cpu, value);
    }

    if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Immediate)
      cpu->AddCycles(4);
    else if constexpr (dst_mode == OperandMode_Register && src_mode == OperandMode_Memory)
      cpu->AddCycles(10);
    else if constexpr (dst_mode == OperandMode_Memory && src_mode == OperandMode_Register)
      cpu->AddCycles(10);
    else if constexpr (dst_mode == OperandMode_ModRM_RM && src_mode == OperandMode_Immediate)
      cpu->AddCycles(RMCycles(cpu, 4, 10, 14));
    else if constexpr (dst_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 2, 9, 13));
    else if constexpr (src_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 2, 8, 12));
    else
      static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_MOV_Sreg(CPU* cpu)
  {
    static_assert(dst_size == OperandSize_16 && src_size == OperandSize_16, "Segment registers are 16-bits");
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    const u8 segreg = cpu->idata.GetModRM_Reg() & 3;

    // TODO: Loading the SS register with a MOV instruction inhibits all interrupts until after the execution
    // of the next instruction. This operation allows a stack pointer to be loaded into the ESP register with the next
    // instruction (MOV ESP, stack-pointer value) before an interrupt occurs1. Be aware that the LSS instruction offers
    // a more efficient method of loading the SS and ESP registers.
    if constexpr (dst_mode == OperandMode_ModRM_SegmentReg)
    {
      cpu->m_registers.segment_selectors[segreg] = ReadWordOperand<src_mode, src_constant>(cpu);
      cpu->AddCycles(RMCycles(cpu, 2, 8, 12));
    }
    else
    {
      WriteWordOperand<dst_mode, dst_constant>(cpu, cpu->m_registers.segment_selectors[segreg]);
      cpu->AddCycles(RMCycles(cpu, 2, 9, 13));
    }
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_XCHG(CPU* cpu)
  {
    static_assert(dst_size == src_size, "source and destination operands are of same size");
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    // In memory version, memory is op0, register is op1. Memory must be written first.
    if constexpr (dst_size == OperandSize_8)
    {
      u8 value0 = ReadByteOperand<dst_mode, dst_constant>(cpu);
      u8 value1 = ReadByteOperand<src_mode, src_constant>(cpu);

      WriteByteOperand<dst_mode, dst_constant>(cpu, value1);
      WriteByteOperand<src_mode, src_constant>(cpu, value0);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 value0 = ReadWordOperand<dst_mode, dst_constant>(cpu);
      u16 value1 = ReadWordOperand<src_mode, src_constant>(cpu);

      WriteWordOperand<dst_mode, dst_constant>(cpu, value1);
      WriteWordOperand<src_mode, src_constant>(cpu, value0);
    }

    cpu->AddCycles(RMCycles(cpu, 4, 17, 25));
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_SHL(CPU* cpu)
  {
    static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
    CalculateEffectiveAddress<val_mode>(cpu);
    CalculateEffectiveAddress<count_mode>(cpu);

    // Shift amounts will always be u8
    // The 8086 does not mask the shift count. However, all other IA-32 processors
    // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
    // resulting in a maximum count of 31.
    if constexpr (val_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
      u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (shift_amount > 0)
      {
        u16 shifted_value = ZeroExtend16(value) << shift_amount;
        u8 new_value = Truncate8(shifted_value);
        WriteByteOperand<val_mode, val_constant>(cpu, new_value);

        SET_FLAG(&cpu->m_registers, CF, ((shifted_value & 0x100) != 0));
        SET_FLAG(&cpu->m_registers, OF,
                 (shift_amount == 1 && (((shifted_value >> 7) & 1) ^ ((shifted_value >> 8) & 1)) != 0));
        SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
        SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
        SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
        SET_FLAG(&cpu->m_registers, AF, false);
      }
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
      u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (shift_amount > 0)
      {
        u32 shifted_value = ZeroExtend32(value) << shift_amount;
        u16 new_value = Truncate16(shifted_value);
        WriteWordOperand<val_mode, val_constant>(cpu, new_value);

        SET_FLAG(&cpu->m_registers, CF, ((shifted_value & 0x10000) != 0));
        SET_FLAG(&cpu->m_registers, OF,
                 (shift_amount == 1 && (((shifted_value >> 15) & 1) ^ ((shifted_value >> 16) & 1)) != 0));
        SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
        SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
        SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
        SET_FLAG(&cpu->m_registers, AF, false);
      }
    }

    cpu->AddCycles(ShiftCycles<val_mode>(cpu));
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_SHR(CPU* cpu)
  {
    static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
    CalculateEffectiveAddress<val_mode>(cpu);
    CalculateEffectiveAddress<count_mode>(cpu);

    // Shift amounts will always be u8
    // The 8086 does not mask the shift count. However, all other IA-32 processors
    // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
    // resulting in a maximum count of 31.
    if constexpr (val_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
      u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (shift_amount > 0)
      {
        u8 new_value = value >> shift_amount;
        WriteByteOperand<val_mode, val_constant>(cpu, new_value);

        SET_FLAG(&cpu->m_registers, CF, ((shift_amount ? (value >> (shift_amount - 1) & 1) : (value & 1)) != 0));
        SET_FLAG(&cpu->m_registers, OF, (shift_amount == 1 && (value & 0x80) != 0));
        SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
        SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
        SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
      }
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
      u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (shift_amount > 0)
      {
        u16 new_value = value >> shift_amount;
        WriteWordOperand<val_mode, val_constant>(cpu, new_value);

        SET_FLAG(&cpu->m_registers, CF, ((shift_amount ? (value >> (shift_amount - 1) & 1) : (value & 1)) != 0));
        SET_FLAG(&cpu->m_registers, OF, (shift_amount == 1 && (value & 0x8000) != 0));
        SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
        SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
        SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
      }
    }

    cpu->AddCycles(ShiftCycles<val_mode>(cpu));
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_SAR(CPU* cpu)
  {
    static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
    CalculateEffectiveAddress<val_mode>(cpu);
    CalculateEffectiveAddress<count_mode>(cpu);

    // Shift amounts will always be u8
    // The 8086 does not mask the shift count. However, all other IA-32 processors
    // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
    // resulting in a maximum count of 31.
    if constexpr (val_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
      u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (shift_amount > 0)
      {
        u8 new_value = u8(s8(value) >> shift_amount);
        WriteByteOperand<val_mode, val_constant>(cpu, new_value);

        SET_FLAG(&cpu->m_registers, CF, ((s8(value) >> (shift_amount - 1) & 1) != 0));
        SET_FLAG(&cpu->m_registers, OF, false);
        SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
        SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
        SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
      }
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
      u8 shift_amount = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (shift_amount > 0)
      {
        u16 new_value = u16(s16(value) >> shift_amount);
        WriteWordOperand<val_mode, val_constant>(cpu, new_value);

        SET_FLAG(&cpu->m_registers, CF, ((s16(value) >> (shift_amount - 1) & 1) != 0));
        SET_FLAG(&cpu->m_registers, OF, false);
        SET_FLAG(&cpu->m_registers, PF, IsParity(new_value));
        SET_FLAG(&cpu->m_registers, SF, IsSign(new_value));
        SET_FLAG(&cpu->m_registers, ZF, IsZero(new_value));
      }
    }

    cpu->AddCycles(ShiftCycles<val_mode>(cpu));
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_RCL(CPU* cpu)
  {
    static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
    CalculateEffectiveAddress<val_mode>(cpu);
    CalculateEffectiveAddress<count_mode>(cpu);

    // The processor restricts the count to a number between 0 and 31 by masking all the bits in the count operand
    // except the 5 least-significant bits.
    if constexpr (val_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
      u8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (rotate_count > 0)
      {
        u8 carry = (cpu->m_registers.FLAGS.CF) ? 1 : 0;
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
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
      u8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (rotate_count > 0)
      {
        u16 carry = (cpu->m_registers.FLAGS.CF) ? 1 : 0;
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
    }

    cpu->AddCycles(ShiftCycles<val_mode>(cpu));
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_RCR(CPU* cpu)
  {
    static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
    CalculateEffectiveAddress<val_mode>(cpu);
    CalculateEffectiveAddress<count_mode>(cpu);

    // The processor restricts the count to a number between 0 and 31 by masking all the bits in the count operand
    // except the 5 least-significant bits.
    if constexpr (val_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
      u8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (rotate_count > 0)
      {
        u8 carry = (cpu->m_registers.FLAGS.CF) ? 1 : 0;
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
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
      u8 rotate_count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (rotate_count > 0)
      {
        u16 carry = (cpu->m_registers.FLAGS.CF) ? 1 : 0;
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
    }

    cpu->AddCycles(ShiftCycles<val_mode>(cpu));
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_ROL(CPU* cpu)
  {
    static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
    CalculateEffectiveAddress<val_mode>(cpu);
    CalculateEffectiveAddress<count_mode>(cpu);

    // Hopefully this will compile down to a native ROL instruction
    if constexpr (val_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
      u8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (count > 0)
      {
        u8 new_value = value;
        if ((count & 0x7) != 0)
        {
          u8 masked_count = count & 0x7;
          new_value = (value << masked_count) | (value >> (8 - masked_count));
          WriteByteOperand<val_mode, val_constant>(cpu, new_value);
        }

        u8 b0 = (new_value & 1);
        u8 b7 = (new_value >> 7);
        SET_FLAG(&cpu->m_registers, CF, (b0 != 0));
        SET_FLAG(&cpu->m_registers, OF, ((b0 ^ b7) != 0));
      }
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
      u8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (count > 0)
      {
        u16 new_value = value;
        if ((count & 0xf) != 0)
        {
          u8 masked_count = count & 0xf;
          new_value = (value << masked_count) | (value >> (16 - masked_count));
          WriteWordOperand<val_mode, val_constant>(cpu, new_value);
        }

        u16 b0 = (new_value & 1);
        u16 b15 = (new_value >> 15);
        SET_FLAG(&cpu->m_registers, CF, (b0 != 0));
        SET_FLAG(&cpu->m_registers, OF, ((b0 ^ b15) != 0));
      }
    }

    cpu->AddCycles(ShiftCycles<val_mode>(cpu));
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant, OperandSize count_size, OperandMode count_mode,
           u32 count_constant>
  static inline void Execute_Operation_ROR(CPU* cpu)
  {
    static_assert(count_size == OperandSize_8, "count is a byte-sized operand");
    CalculateEffectiveAddress<val_mode>(cpu);
    CalculateEffectiveAddress<count_mode>(cpu);

    // Hopefully this will compile down to a native ROR instruction
    if constexpr (val_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
      u8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (count > 0)
      {
        u8 new_value = value;
        u8 masked_count = count & 0x7;
        if (masked_count != 0)
        {
          new_value = (value >> masked_count) | (value << (8 - masked_count));
          WriteByteOperand<val_mode, val_constant>(cpu, new_value);
        }

        u16 b6 = ((new_value >> 6) & 1);
        u16 b7 = ((new_value >> 7) & 1);
        SET_FLAG(&cpu->m_registers, CF, (b7 != 0));
        SET_FLAG(&cpu->m_registers, OF, ((b6 ^ b7) != 0));
      }
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
      u8 count = ReadByteOperand<count_mode, count_constant>(cpu) & 0x1F;
      if (count > 0)
      {
        u16 new_value = value;
        u8 masked_count = count & 0xf;
        if (masked_count != 0)
        {
          new_value = (value >> masked_count) | (value << (16 - masked_count));
          WriteWordOperand<val_mode, val_constant>(cpu, new_value);
        }

        u16 b14 = ((new_value >> 14) & 1);
        u16 b15 = ((new_value >> 15) & 1);
        SET_FLAG(&cpu->m_registers, CF, (b15 != 0));
        SET_FLAG(&cpu->m_registers, OF, ((b14 ^ b15) != 0));
      }
    }

    cpu->AddCycles(ShiftCycles<val_mode>(cpu));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_IN(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    const u16 port_number = ReadZeroExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    if constexpr (dst_size == OperandSize_8)
    {
      u8 value;
      cpu->m_bus->ReadIOPortByte(port_number, &value);
      WriteByteOperand<dst_mode, dst_constant>(cpu, value);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 value;
      cpu->m_bus->ReadIOPortWord(port_number, &value);
      WriteWordOperand<dst_mode, dst_constant>(cpu, value);
    }

    if constexpr (src_mode == OperandMode_Immediate)
      cpu->AddCycles(MemoryCycles(cpu, 10, 14));
    else if constexpr (src_mode == OperandMode_Register)
      cpu->AddCycles(MemoryCycles(cpu, 8, 12));
    else
      static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_OUT(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    const u16 port_number = ReadZeroExtendedWordOperand<dst_size, dst_mode, dst_constant>(cpu);
    if constexpr (dst_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<src_mode, src_constant>(cpu);
      cpu->m_bus->WriteIOPortByte(port_number, value);
    }
    else if constexpr (dst_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<src_mode, src_constant>(cpu);
      cpu->m_bus->WriteIOPortWord(port_number, value);
    }

    if constexpr (src_mode == OperandMode_Immediate)
      cpu->AddCycles(MemoryCycles(cpu, 10, 14));
    else if constexpr (src_mode == OperandMode_Register)
      cpu->AddCycles(MemoryCycles(cpu, 8, 12));
    else
      static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_INC(CPU* cpu)
  {
    CalculateEffectiveAddress<val_mode>(cpu);

    // Preserve CF
    bool cf = cpu->m_registers.FLAGS.CF;
    if constexpr (val_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
      u8 new_value = ALUOp_Add8(&cpu->m_registers, value, 1);
      WriteByteOperand<val_mode, val_constant>(cpu, new_value);
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
      u16 new_value = ALUOp_Add16(&cpu->m_registers, value, 1);
      WriteWordOperand<val_mode, val_constant>(cpu, new_value);
    }

    SET_FLAG(&cpu->m_registers, CF, cf);

    if constexpr (val_mode == OperandMode_Register)
      cpu->AddCycles(3);
    else if constexpr (val_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 3, 15, 23));
    else
      static_assert(dependent_int_false<val_mode>::value, "unknown mode");
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_DEC(CPU* cpu)
  {
    CalculateEffectiveAddress<val_mode>(cpu);

    // Preserve CF
    bool cf = cpu->m_registers.FLAGS.CF;
    if constexpr (val_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
      u8 new_value = ALUOp_Sub8(&cpu->m_registers, value, 1);
      WriteByteOperand<val_mode, val_constant>(cpu, new_value);
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
      u16 new_value = ALUOp_Sub16(&cpu->m_registers, value, 1);
      WriteWordOperand<val_mode, val_constant>(cpu, new_value);
    }

    SET_FLAG(&cpu->m_registers, CF, cf);

    if constexpr (val_mode == OperandMode_Register)
      cpu->AddCycles(3);
    else if constexpr (val_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 3, 15, 23));
    else
      static_assert(dependent_int_false<val_mode>::value, "unknown mode");
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_NOT(CPU* cpu)
  {
    CalculateEffectiveAddress<val_mode>(cpu);

    if constexpr (val_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
      u8 new_value = ~value;
      WriteByteOperand<val_mode, val_constant>(cpu, new_value);
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
      u16 new_value = ~value;
      WriteWordOperand<val_mode, val_constant>(cpu, new_value);
    }

    if constexpr (val_mode == OperandMode_Register)
      cpu->AddCycles(3);
    else if constexpr (val_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 3, 16, 24));
    else
      static_assert(dependent_int_false<val_mode>::value, "unknown mode");
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_NEG(CPU* cpu)
  {
    CalculateEffectiveAddress<val_mode>(cpu);

    if constexpr (val_size == OperandSize_8)
    {
      u8 value = ReadByteOperand<val_mode, val_constant>(cpu);
      u8 new_value = u8(-s8(value));
      WriteByteOperand<val_mode, val_constant>(cpu, new_value);

      ALUOp_Sub8(&cpu->m_registers, 0, value);
      SET_FLAG(&cpu->m_registers, CF, (new_value != 0));
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u16 value = ReadWordOperand<val_mode, val_constant>(cpu);
      u16 new_value = u16(-s16(value));
      WriteWordOperand<val_mode, val_constant>(cpu, new_value);

      ALUOp_Sub16(&cpu->m_registers, 0, value);
      SET_FLAG(&cpu->m_registers, CF, (new_value != 0));
    }

    if constexpr (val_mode == OperandMode_Register)
      cpu->AddCycles(3);
    else if constexpr (val_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 3, 16, 24));
    else
      static_assert(dependent_int_false<val_mode>::value, "unknown mode");
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_MUL(CPU* cpu)
  {
    CalculateEffectiveAddress<val_mode>(cpu);

    // The OF and CF flags are set to 0 if the upper half of the result is 0;
    // otherwise, they are set to 1. The SF, ZF, AF, and PF flags are undefined.
    if constexpr (val_size == OperandSize_8)
    {
      u16 lhs = u16(cpu->m_registers.AL);
      u16 rhs = u16(ReadByteOperand<val_mode, val_constant>(cpu));
      u16 result = lhs * rhs;
      cpu->m_registers.AX = result;
      SET_FLAG(&cpu->m_registers, OF, (cpu->m_registers.AH != 0));
      SET_FLAG(&cpu->m_registers, CF, (cpu->m_registers.AH != 0));
      SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.AL));
      SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.AL));
      SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.AL));
      cpu->AddCycles(RMCycles(cpu, 70, 76, 80));
    }
    else if constexpr (val_size == OperandSize_16)
    {
      u32 lhs = u32(cpu->m_registers.AX);
      u32 rhs = u32(ReadWordOperand<val_mode, val_constant>(cpu));
      u32 result = lhs * rhs;
      cpu->m_registers.AX = u16(result & 0xFFFF);
      cpu->m_registers.DX = u16(result >> 16);
      SET_FLAG(&cpu->m_registers, OF, (cpu->m_registers.DX != 0));
      SET_FLAG(&cpu->m_registers, CF, (cpu->m_registers.DX != 0));
      SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.AX));
      SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.AX));
      SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.AX));
      cpu->AddCycles(RMCycles(cpu, 113, 124, 129));
    }
  }

  template<OperandSize op1_size, OperandMode op1_mode, u32 op1_constant>
  static inline void Execute_Operation_IMUL(CPU* cpu)
  {
    CalculateEffectiveAddress<op1_mode>(cpu);

    if constexpr (op1_size == OperandSize_8)
    {
      u16 lhs = SignExtend16(cpu->m_registers.AL);
      u16 rhs = SignExtend16(ReadByteOperand<op1_mode, op1_constant>(cpu));
      u16 result = u16(s16(lhs) * s16(rhs));
      u8 truncated_result = Truncate8(result);

      cpu->m_registers.AX = result;

      cpu->m_registers.FLAGS.OF = cpu->m_registers.FLAGS.CF = (SignExtend16(truncated_result) != result);
      cpu->m_registers.FLAGS.SF = IsSign(truncated_result);
      cpu->m_registers.FLAGS.ZF = IsZero(truncated_result);
      cpu->m_registers.FLAGS.PF = IsParity(truncated_result);
      cpu->AddCycles(RMCycles(cpu, 80, 86, 90));
    }
    else if constexpr (op1_size == OperandSize_16)
    {
      u32 lhs = SignExtend32(cpu->m_registers.AX);
      u32 rhs = SignExtend32(ReadSignExtendedWordOperand<op1_size, op1_mode, op1_constant>(cpu));
      u32 result = u32(s32(lhs) * s32(rhs));
      u16 truncated_result = Truncate16(result);

      cpu->m_registers.DX = Truncate16(result >> 16);
      cpu->m_registers.AX = truncated_result;

      cpu->m_registers.FLAGS.OF = cpu->m_registers.FLAGS.CF = (SignExtend32(truncated_result) != result);
      cpu->m_registers.FLAGS.SF = IsSign(truncated_result);
      cpu->m_registers.FLAGS.ZF = IsZero(truncated_result);
      cpu->m_registers.FLAGS.PF = IsParity(truncated_result);
      cpu->AddCycles(RMCycles(cpu, 128, 150, 154));
    }
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_DIV(CPU* cpu)
  {
    CalculateEffectiveAddress<val_mode>(cpu);

    if constexpr (val_size == OperandSize_8)
    {
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
      cpu->AddCycles(RMCycles(cpu, 80, 86, 90));
    }
    else if constexpr (val_size == OperandSize_16)
    {
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
      cpu->AddCycles(RMCycles(cpu, 144, 150, 158));
    }
  }

  template<OperandSize val_size, OperandMode val_mode, u32 val_constant>
  static inline void Execute_Operation_IDIV(CPU* cpu)
  {
    CalculateEffectiveAddress<val_mode>(cpu);

    if constexpr (val_size == OperandSize_8)
    {
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
      u8 truncated_quotient = u8(u16(quotient) & 0xFFFF);
      u8 truncated_remainder = u8(u16(remainder) & 0xFFFF);
      if (s8(truncated_quotient) != quotient)
      {
        cpu->RaiseException(Interrupt_DivideError);
        return;
      }

      cpu->m_registers.AL = truncated_quotient;
      cpu->m_registers.AH = truncated_remainder;
      cpu->AddCycles(RMCycles(cpu, 101, 107, 111));
    }
    else if constexpr (val_size == OperandSize_16)
    {
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
      u16 truncated_quotient = u16(u32(quotient) & 0xFFFF);
      u16 truncated_remainder = u16(u32(remainder) & 0xFFFF);
      if (s16(truncated_quotient) != quotient)
      {
        cpu->RaiseException(Interrupt_DivideError);
        return;
      }

      cpu->m_registers.AX = truncated_quotient;
      cpu->m_registers.DX = truncated_remainder;
      cpu->AddCycles(RMCycles(cpu, 165, 171, 175));
    }
  }

  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_PUSH(CPU* cpu)
  {
    CalculateEffectiveAddress<src_mode>(cpu);

    const u16 value = ReadSignExtendedWordOperand<src_size, src_mode, src_constant>(cpu);
    cpu->PushWord(value);

    if constexpr (src_mode == OperandMode_Register)
      cpu->AddCycles(11);
    else if constexpr (src_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 15, 16, 24));
    else
      static_assert(dependent_int_false<src_mode>::value, "unknown mode");
  }

  template<OperandSize src_size, OperandMode src_mode, u32 src_constant>
  static inline void Execute_Operation_PUSH_Sreg(CPU* cpu)
  {
    static_assert(src_size == OperandSize_16 && src_mode == OperandMode_SegmentRegister && src_constant < Segment_Count,
                  "operands are of correct type and in range");

    const u16 selector = cpu->m_registers.segment_selectors[src_constant];
    cpu->PushWord(selector);
    cpu->AddCycles(MemoryCycles(cpu, 10, 14));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_POP_Sreg(CPU* cpu)
  {
    static_assert(dst_size == OperandSize_16 && dst_mode == OperandMode_SegmentRegister && dst_constant < Segment_Count,
                  "operands are of correct type and in range");

    u16 selector = cpu->PopWord();
    cpu->m_registers.segment_selectors[dst_constant] = selector;
    cpu->AddCycles(8);
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_POP(CPU* cpu)
  {
    // POP can use ESP in the address calculations, in this case the value of ESP
    // is that after the pop operation has occurred, not before.
    u16 value = cpu->PopWord();
    CalculateEffectiveAddress<dst_mode>(cpu);
    WriteWordOperand<dst_mode, dst_constant>(cpu, value);

    if constexpr (dst_mode == OperandMode_Register)
      cpu->AddCycles(8);
    else if constexpr (dst_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 8, 17, 17));
    else
      static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
  }

  static inline void Execute_Operation_PUSHA(CPU* cpu)
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

    cpu->AddCycles(19);
  }

  static inline void Execute_Operation_POPA(CPU* cpu)
  {
    u16 DI = cpu->PopWord();
    u16 SI = cpu->PopWord();
    u16 BP = cpu->PopWord();
    /*u16 SP = */ cpu->PopWord();
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

    cpu->AddCycles(19);
  }

  template<OperandSize sreg_size, OperandMode sreg_mode, u32 sreg_constant, OperandSize reg_size, OperandMode reg_mode,
           u32 reg_constant, OperandSize ptr_size, OperandMode ptr_mode, u32 ptr_constant>
  static inline void Execute_Operation_LXS(CPU* cpu)
  {
    static_assert(sreg_mode == OperandMode_SegmentRegister, "sreg_mode is Segment Register");
    static_assert(reg_mode == OperandMode_ModRM_Reg, "reg_mode is Register");
    static_assert(ptr_mode == OperandMode_ModRM_RM, "reg_mode is Pointer");
    CalculateEffectiveAddress<ptr_mode>(cpu);

    u16 segment_selector;
    VirtualMemoryAddress address;
    ReadFarAddressOperand<ptr_mode>(cpu, &segment_selector, &address);

    cpu->m_registers.segment_selectors[sreg_constant] = segment_selector;
    WriteWordOperand<reg_mode, reg_constant>(cpu, Truncate16(address));

    cpu->AddCycles(MemoryCycles(cpu, 7, 15)); // TODO: Probably wrong.
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_LEA(CPU* cpu)
  {
    static_assert(src_mode == OperandMode_ModRM_RM, "Source operand is a pointer");
    CalculateEffectiveAddress<dst_mode>(cpu);
    CalculateEffectiveAddress<src_mode>(cpu);

    // Calculate full address in instruction's address mode, truncate/extend to operand size.
    WriteWordOperand<dst_mode, dst_constant>(cpu, cpu->m_effective_address);

    cpu->AddCycles(5); // TODO: Probably wrong.
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_JMP_Near(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_size, dst_mode, dst_constant>(cpu);
    cpu->BranchTo(jump_address);

    if constexpr (dst_mode == OperandMode_Relative)
      cpu->AddCycles(15);
    else if constexpr (dst_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(RMCycles(cpu, 11, 18, 22));
    else
      static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
  }

  template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_Jcc(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    if (!TestJumpCondition<condition>(cpu))
    {
      cpu->AddCycles(6);
      return;
    }

    VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_size, dst_mode, dst_constant>(cpu);
    cpu->BranchTo(jump_address);
    cpu->AddCycles(18);
  }

  template<JumpCondition condition, OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_LOOP(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);

    u16 count = --cpu->m_registers.CX;
    bool branch = (count != 0) && TestJumpCondition<condition>(cpu);
    if (!branch)
    {
      cpu->AddCycles(4);
      return;
    }

    VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_size, dst_mode, dst_constant>(cpu);
    cpu->BranchTo(jump_address);
    cpu->AddCycles(8);
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_CALL_Near(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_size, dst_mode, dst_constant>(cpu);
    cpu->PushWord(cpu->m_registers.IP);
    cpu->BranchTo(jump_address);

    if constexpr (dst_mode == OperandMode_Relative)
      cpu->AddCycles(19);
    else if constexpr (dst_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(16);
    else
      static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
  }

  template<OperandSize dst_size = OperandSize_Count, OperandMode dst_mode = OperandMode_None, u32 dst_constant = 0>
  static inline void Execute_Operation_RET_Near(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);
    if constexpr (dst_mode != OperandMode_None)
    {
      u16 pop_count = ReadZeroExtendedWordOperand<dst_size, dst_mode, dst_constant>(cpu);
      u16 return_EIP = cpu->PopWord();
      cpu->m_registers.SP += Truncate16(pop_count);
      cpu->BranchTo(return_EIP);
      cpu->AddCycles(MemoryCycles(cpu, 20, 24));
    }
    else
    {
      u16 return_EIP = cpu->PopWord();
      cpu->BranchTo(return_EIP);
      cpu->AddCycles(MemoryCycles(cpu, 16, 20));
    }
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_JMP_Far(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);

    u16 segment_selector;
    VirtualMemoryAddress address;
    ReadFarAddressOperand<dst_mode>(cpu, &segment_selector, &address);

    if constexpr (dst_mode == OperandMode_FarAddress)
      cpu->AddCycles(20);
    else if constexpr (dst_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(29);
    else
      static_assert(dependent_int_false<dst_mode>::value, "unknown mode");

    cpu->BranchTo(segment_selector, address);
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_CALL_Far(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);

    u16 segment_selector;
    VirtualMemoryAddress address;
    ReadFarAddressOperand<dst_mode>(cpu, &segment_selector, &address);

    cpu->PushWord(cpu->m_registers.CS);
    cpu->PushWord(cpu->m_registers.IP);

    cpu->BranchTo(segment_selector, address);

    if constexpr (dst_mode == OperandMode_FarAddress)
      cpu->AddCycles(28);
    else if constexpr (dst_mode == OperandMode_ModRM_RM)
      cpu->AddCycles(37);
    else
      static_assert(dependent_int_false<dst_mode>::value, "unknown mode");
  }

  template<OperandSize dst_size = OperandSize_Count, OperandMode dst_mode = OperandMode_None, u32 dst_constant = 0>
  static inline void Execute_Operation_RET_Far(CPU* cpu)
  {
    CalculateEffectiveAddress<dst_mode>(cpu);

    if constexpr (dst_mode != OperandMode_None)
    {
      u16 pop_count = ReadZeroExtendedWordOperand<dst_size, dst_mode, dst_constant>(cpu);
      const u16 ip = cpu->PopWord();
      const u16 cs = cpu->PopWord();
      cpu->m_registers.SP += pop_count;
      cpu->BranchTo(cs, ip);
      cpu->AddCycles(MemoryCycles(cpu, 26, 34));
    }
    else
    {
      const u16 ip = cpu->PopWord();
      const u16 cs = cpu->PopWord();
      cpu->BranchTo(cs, ip);
      cpu->AddCycles(MemoryCycles(cpu, 25, 33));
    }
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_INT(CPU* cpu)
  {
    static_assert(dst_size == OperandSize_8, "size is 8 bits");
    static_assert(dst_mode == OperandMode_Constant || dst_mode == OperandMode_Immediate, "constant or immediate");
    u8 interrupt = ReadByteOperand<dst_mode, dst_constant>(cpu);

    // Return to IP after this instruction
    cpu->SetupInterruptCall(interrupt, cpu->m_registers.IP);
    if constexpr (dst_mode == OperandMode_Constant)
      cpu->AddCycles(MemoryCycles(cpu, 52, 72));
    else
      cpu->AddCycles(MemoryCycles(cpu, 51, 71));
  }

  static inline void Execute_Operation_INTO(CPU* cpu)
  {
    // Call overflow exception if OF is set
    if (!cpu->m_registers.FLAGS.OF)
    {
      cpu->AddCycles(4);
      return;
    }

    // Return address should not point to the faulting instruction.
    cpu->SetupInterruptCall(Interrupt_Overflow, cpu->m_registers.IP);
    cpu->AddCycles(MemoryCycles(cpu, 53, 73));
  }

  static inline void Execute_Operation_IRET(CPU* cpu)
  {
    u16 return_IP = cpu->PopWord();
    u16 return_CS = cpu->PopWord();
    u16 return_FLAGS = cpu->PopWord();
    cpu->BranchTo(return_CS, return_IP);
    cpu->SetFlags(return_FLAGS);
    cpu->AddCycles(MemoryCycles(cpu, 32, 44));
  }

  static inline void Execute_Operation_NOP(CPU* cpu) { cpu->AddCycles(3); }

  static inline void Execute_Operation_CLC(CPU* cpu)
  {
    SET_FLAG(&cpu->m_registers, CF, false);
    cpu->AddCycles(2);
  }

  static inline void Execute_Operation_CLD(CPU* cpu)
  {
    SET_FLAG(&cpu->m_registers, DF, false);
    cpu->AddCycles(2);
  }

  static inline void Execute_Operation_CLI(CPU* cpu)
  {
    // TODO: Delay of one instruction
    SET_FLAG(&cpu->m_registers, IF, false);
    cpu->AddCycles(2);
  }

  static inline void Execute_Operation_CMC(CPU* cpu)
  {
    SET_FLAG(&cpu->m_registers, CF, !cpu->m_registers.FLAGS.CF);
    cpu->AddCycles(2);
  }

  static inline void Execute_Operation_STC(CPU* cpu)
  {
    SET_FLAG(&cpu->m_registers, CF, true);
    cpu->AddCycles(2);
  }

  static inline void Execute_Operation_STD(CPU* cpu)
  {
    SET_FLAG(&cpu->m_registers, DF, true);
    cpu->AddCycles(2);
  }

  static inline void Execute_Operation_STI(CPU* cpu)
  {
    SET_FLAG(&cpu->m_registers, IF, true);
    cpu->AddCycles(2);
  }

  static inline void Execute_Operation_SALC(CPU* cpu)
  {
    // Undocumented instruction. Same as SBB AL, AL without modifying any flags.
    u16 old_flags = cpu->m_registers.FLAGS.bits;
    cpu->m_registers.AL = ALUOp_Sbb8(&cpu->m_registers, cpu->m_registers.AL, cpu->m_registers.AL);
    cpu->m_registers.FLAGS.bits = old_flags;
    cpu->AddCycles(3);
  }

  static inline void Execute_Operation_LAHF(CPU* cpu)
  {
    cpu->m_registers.AH = Truncate8(cpu->m_registers.FLAGS.bits);
    cpu->AddCycles(2);
  }

  static inline void Execute_Operation_SAHF(CPU* cpu)
  {
    const u32 change_mask = Flag_SF | Flag_ZF | Flag_AF | Flag_CF | Flag_PF;
    cpu->SetFlags((cpu->m_registers.FLAGS.bits & ~change_mask) | (ZeroExtend32(cpu->m_registers.AH) & change_mask));
    cpu->AddCycles(4);
  }

  static inline void Execute_Operation_PUSHF(CPU* cpu)
  {
    cpu->PushWord(Truncate16(cpu->m_registers.FLAGS.bits));
    cpu->AddCycles(MemoryCycles(cpu, 10, 14));
  }

  static inline void Execute_Operation_POPF(CPU* cpu)
  {
    const u16 flags = cpu->PopWord();
    cpu->SetFlags(flags);
    cpu->AddCycles(MemoryCycles(cpu, 8, 12));
  }

  static inline void Execute_Operation_HLT(CPU* cpu)
  {
    cpu->SetHalted(true);
    cpu->AddCycles(2);
  }

  static inline void Execute_Operation_CBW(CPU* cpu)
  {
    // Sign-extend AL to AH
    cpu->m_registers.AH = ((cpu->m_registers.AL & 0x80) != 0) ? 0xFF : 0x00;
    cpu->AddCycles(2);
  }

  static inline void Execute_Operation_CWD(CPU* cpu)
  {
    // Sign-extend AX to DX
    cpu->m_registers.DX = ((cpu->m_registers.AX & 0x8000) != 0) ? 0xFFFF : 0x0000;
    cpu->AddCycles(5);
  }

  static inline void Execute_Operation_XLAT(CPU* cpu)
  {
    u16 address = cpu->m_registers.BX + ZeroExtend16(cpu->m_registers.AL);
    u8 value = cpu->ReadMemoryByte(cpu->idata.segment, address);
    cpu->m_registers.AL = value;
    cpu->AddCycles(11);
  }

  static inline void Execute_Operation_AAA(CPU* cpu)
  {
    if ((cpu->m_registers.AL & 0xF) > 0x09 || cpu->m_registers.FLAGS.AF)
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
    cpu->AddCycles(8);
  }

  static inline void Execute_Operation_AAS(CPU* cpu)
  {
    if ((cpu->m_registers.AL & 0xF) > 0x09 || cpu->m_registers.FLAGS.AF)
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
    cpu->AddCycles(8);
  }

  template<OperandSize op_size, OperandMode op_mode, u32 op_constant>
  static inline void Execute_Operation_AAM(CPU* cpu)
  {
    CalculateEffectiveAddress<op_mode>(cpu);

    u8 operand = ReadByteOperand<op_mode, op_constant>(cpu);
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
    cpu->AddCycles(83);
  }

  template<OperandSize op_size, OperandMode op_mode, u32 op_constant>
  static inline void Execute_Operation_AAD(CPU* cpu)
  {
    CalculateEffectiveAddress<op_mode>(cpu);

    u8 operand = ReadByteOperand<op_mode, op_constant>(cpu);
    u16 result = u16(cpu->m_registers.AH) * u16(operand) + u16(cpu->m_registers.AL);

    cpu->m_registers.AL = u8(result & 0xFF);
    cpu->m_registers.AH = 0;

    SET_FLAG(&cpu->m_registers, AF, false);
    SET_FLAG(&cpu->m_registers, CF, false);
    SET_FLAG(&cpu->m_registers, OF, false);

    SET_FLAG(&cpu->m_registers, SF, IsSign(cpu->m_registers.AL));
    SET_FLAG(&cpu->m_registers, ZF, IsZero(cpu->m_registers.AL));
    SET_FLAG(&cpu->m_registers, PF, IsParity(cpu->m_registers.AL));
    cpu->AddCycles(60);
  }

  static inline void Execute_Operation_DAA(CPU* cpu)
  {
    u8 old_AL = cpu->m_registers.AL;
    bool old_CF = cpu->m_registers.FLAGS.CF;

    if ((old_AL & 0xF) > 0x9 || cpu->m_registers.FLAGS.AF)
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
    cpu->AddCycles(4);
  }

  static inline void Execute_Operation_DAS(CPU* cpu)
  {
    u8 old_AL = cpu->m_registers.AL;
    bool old_CF = cpu->m_registers.FLAGS.CF;

    if ((old_AL & 0xF) > 0x9 || cpu->m_registers.FLAGS.AF)
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
    cpu->AddCycles(4);
  }

  template<Operation operation, bool check_equal, typename callback>
  static inline void Execute_REP(CPU* cpu, callback cb)
  {
    const bool has_rep = cpu->idata.has_rep;

    // We can execute this instruction as a non-rep.
    if (!has_rep)
    {
      cb(cpu);
      return;
    }

    for (;;)
    {
      cpu->AddCycles(2);

      // Only the counter is checked at the beginning.
      if (cpu->m_registers.CX == 0)
        return;

      // Execute the actual instruction.
      cb(cpu);

      // Decrement the count register after the operation.
      bool branch = (--cpu->m_registers.CX != 0);

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

      // Add a cycle, for the next byte. This could cause an interrupt.
      // This way long-running REP instructions will be paused mid-way.
      cpu->CommitPendingCycles();

      // Interrupts should apparently be checked before the instruction.
      // We check them before dispatching, but since we're looping around, do it again.
      if (cpu->HasExternalInterrupt())
      {
        // If the interrupt line gets signaled, we need the return address set to the REP instruction.
        cpu->RestartCurrentInstruction();
        return;
      }
    }
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_SCAS(CPU* cpu)
  {
    static_assert(src_size == dst_size, "operand sizes are the same");
    Execute_REP<Operation_SCAS, true>(cpu, [](CPU* cpu) {
      // The ES segment cannot be overridden with a segment override prefix.
      u8 data_size;
      if constexpr (dst_size == OperandSize_8)
      {
        u8 lhs = cpu->m_registers.AL;
        u8 rhs = cpu->ReadMemoryByte(Segment_ES, cpu->m_registers.DI);
        ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
        data_size = sizeof(u8);
      }
      else if constexpr (dst_size == OperandSize_16)
      {
        u16 lhs = cpu->m_registers.AX;
        u16 rhs = cpu->ReadMemoryWord(Segment_ES, cpu->m_registers.DI);
        ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
        data_size = sizeof(u16);
      }

      if (!cpu->m_registers.FLAGS.DF)
        cpu->m_registers.DI += ZeroExtend16(data_size);
      else
        cpu->m_registers.DI -= ZeroExtend16(data_size);
    });

    cpu->AddCycles(MemoryCycles(cpu, 15, 19));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_LODS(CPU* cpu)
  {
    static_assert(src_size == dst_size, "operand sizes are the same");
    Execute_REP<Operation_LODS, false>(cpu, [](CPU* cpu) {
      u8 data_size;
      if constexpr (dst_size == OperandSize_8)
      {
        u8 value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->m_registers.SI);
        cpu->m_registers.AL = value;
        data_size = sizeof(u8);
      }
      else if constexpr (dst_size == OperandSize_16)
      {
        u16 value = cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_registers.SI);
        cpu->m_registers.AX = value;
        data_size = sizeof(u16);
      }

      if (!cpu->m_registers.FLAGS.DF)
        cpu->m_registers.SI += ZeroExtend16(data_size);
      else
        cpu->m_registers.SI -= ZeroExtend16(data_size);
    });

    cpu->AddCycles(MemoryCycles(cpu, 5, 9));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_STOS(CPU* cpu)
  {
    static_assert(src_size == dst_size, "operand sizes are the same");
    Execute_REP<Operation_STOS, false>(cpu, [](CPU* cpu) {
      u8 data_size;
      if constexpr (dst_size == OperandSize_8)
      {
        u8 value = cpu->m_registers.AL;
        cpu->WriteMemoryByte(Segment_ES, cpu->m_registers.DI, value);
        data_size = sizeof(u8);
      }
      else if constexpr (dst_size == OperandSize_16)
      {
        u16 value = cpu->m_registers.AX;
        cpu->WriteMemoryWord(Segment_ES, cpu->m_registers.DI, value);
        data_size = sizeof(u16);
      }

      if (!cpu->m_registers.FLAGS.DF)
        cpu->m_registers.DI += ZeroExtend16(data_size);
      else
        cpu->m_registers.DI -= ZeroExtend16(data_size);
    });

    cpu->AddCycles(MemoryCycles(cpu, 11, 15));
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_CMPS(CPU* cpu)
  {
    static_assert(src_size == dst_size, "operand sizes are the same");
    Execute_REP<Operation_CMPS, true>(cpu, [](CPU* cpu) {
      // The DS segment may be overridden with a segment override prefix, but the ES segment cannot be overridden.
      u8 data_size;
      if constexpr (dst_size == OperandSize_8)
      {
        u8 lhs = cpu->ReadMemoryByte(cpu->idata.segment, cpu->m_registers.SI);
        u8 rhs = cpu->ReadMemoryByte(Segment_ES, cpu->m_registers.DI);
        ALUOp_Sub8(&cpu->m_registers, lhs, rhs);
        data_size = sizeof(u8);
      }
      else if constexpr (dst_size == OperandSize_16)
      {
        u16 lhs = cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_registers.SI);
        u16 rhs = cpu->ReadMemoryWord(Segment_ES, cpu->m_registers.DI);
        ALUOp_Sub16(&cpu->m_registers, lhs, rhs);
        data_size = sizeof(u16);
      }

      if (!cpu->m_registers.FLAGS.DF)
      {
        cpu->m_registers.SI += ZeroExtend16(data_size);
        cpu->m_registers.DI += ZeroExtend16(data_size);
      }
      else
      {
        cpu->m_registers.SI -= ZeroExtend16(data_size);
        cpu->m_registers.DI -= ZeroExtend16(data_size);
      }

      cpu->AddCycles(MemoryCycles(cpu, 22, 30));
    });
  }

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant, OperandSize src_size, OperandMode src_mode,
           u32 src_constant>
  static inline void Execute_Operation_MOVS(CPU* cpu)
  {
    static_assert(src_size == dst_size, "operand sizes are the same");
    Execute_REP<Operation_MOVS, false>(cpu, [](CPU* cpu) {
      // The DS segment may be over-ridden with a segment override prefix, but the ES segment cannot be overridden.
      u8 data_size;
      if constexpr (dst_size == OperandSize_8)
      {
        u8 value = cpu->ReadMemoryByte(cpu->idata.segment, cpu->m_registers.SI);
        cpu->WriteMemoryByte(Segment_ES, cpu->m_registers.DI, value);
        data_size = sizeof(u8);
      }
      else if constexpr (dst_size == OperandSize_16)
      {
        u16 value = cpu->ReadMemoryWord(cpu->idata.segment, cpu->m_registers.SI);
        cpu->WriteMemoryWord(Segment_ES, cpu->m_registers.DI, value);
        data_size = sizeof(u16);
      }

      if (!cpu->m_registers.FLAGS.DF)
      {
        cpu->m_registers.SI += ZeroExtend16(data_size);
        cpu->m_registers.DI += ZeroExtend16(data_size);
      }
      else
      {
        cpu->m_registers.SI -= ZeroExtend16(data_size);
        cpu->m_registers.DI -= ZeroExtend16(data_size);
      }
      cpu->AddCycles(MemoryCycles(cpu, 18, 26));
    });
  }

  static inline void Execute_Operation_WAIT(CPU* cpu) {}

  template<OperandSize dst_size, OperandMode dst_mode, u32 dst_constant>
  static inline void Execute_Operation_Escape(CPU* cpu)
  {
  }

  static inline void DispatchInstruction(CPU* cpu);
};

void CPU::ExecuteInstruction()
{
  // The instruction that sets the trap flag should not trigger an interrupt.
  // To handle this, we store the trap flag state before processing the instruction.
  bool trap_after_instruction = m_registers.FLAGS.TF;

  // Store current instruction address in m_current_EIP.
  // The address of the current instruction is needed when exceptions occur.
  m_current_IP = m_registers.IP;

#if 0
  LinearMemoryAddress linear_address = CalculateLinearAddress(Segment_CS, m_registers.IP);
  if (linear_address == 0xF4000)
    TRACE_EXECUTION = true;
#endif

  if (TRACE_EXECUTION)
  {
    if (TRACE_EXECUTION_LAST_EIP != m_current_IP)
      PrintCurrentStateAndInstruction();
    TRACE_EXECUTION_LAST_EIP = m_current_IP;
  }

  // Initialize istate for this instruction
  std::memset(&idata, 0, sizeof(idata));
  idata.segment = Segment_DS;

  // Read and decode an instruction from the current IP.
  Instructions::DispatchInstruction(this);

  if (trap_after_instruction)
  {
    // We should push the next instruction pointer, not the instruction that's trapping,
    // since it has already executed. We also can't use m_cpu->RaiseException since this would
    // reset the stack pointer too (and it could be a stack-modifying instruction). We
    // also don't need to abort the current instruction since we're looping anyway.
    SetupInterruptCall(Interrupt_Debugger, m_registers.IP);
  }
}

#include "instructions_dispatch.inl"

} // namespace CPU_8086
