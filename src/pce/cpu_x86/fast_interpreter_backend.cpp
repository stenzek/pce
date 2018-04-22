#include "pce/cpu_x86/fast_interpreter_backend.h"
#include "YBaseLib/Endian.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/String.h"
#include "pce/bus.h"
#include "pce/cpu_x86/interpreter.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cstdint>
#include <cstdio>

#ifdef Y_COMPILER_MSVC
#include <intrin.h>
#endif

Log_SetChannel(CPU_X86::FastInterpreter);

namespace CPU_X86 {

extern bool TRACE_EXECUTION;
extern uint32 TRACE_EXECUTION_LAST_EIP;

FastInterpreterBackend::FastInterpreterBackend(CPU* cpu) : m_cpu(cpu), m_system(cpu->GetSystem()), m_bus(cpu->GetBus())
{
}

FastInterpreterBackend::~FastInterpreterBackend() {}

void FastInterpreterBackend::Reset() {}

void FastInterpreterBackend::Execute()
{
  setjmp(m_jmp_buf);

  while (!m_cpu->IsHalted() && m_cpu->m_execution_downcount > 0)
  {
    // Check for external interrupts.
    if (m_cpu->HasExternalInterrupt())
      m_cpu->DispatchExternalInterrupt();

    // The instruction that sets the trap flag should not trigger an interrupt.
    // To handle this, we store the trap flag state before processing the instruction.
    bool trap_after_instruction = m_cpu->m_registers.EFLAGS.TF;

    // Store current instruction address in m_current_EIP.
    // The address of the current instruction is needed when exceptions occur.
    m_cpu->m_current_EIP = m_cpu->m_registers.EIP;
    m_cpu->m_current_ESP = m_cpu->m_registers.ESP;

    LinearMemoryAddress linear_address = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_registers.EIP);
    // if (m_cpu->m_registers.CS == 0x226F && (m_cpu->m_registers.EIP == 0x000002A3 || m_cpu->m_registers.EIP ==
    // 0x651C))  if (linear_address == 0x802214C3)  if (linear_address == 0x7C70)
    //__debugbreak();
    // if (linear_address == 0xC0003)
    // TRACE_EXECUTION = true;

    if (TRACE_EXECUTION)
    {
      if (TRACE_EXECUTION_LAST_EIP != m_cpu->m_current_EIP)
        m_cpu->PrintCurrentStateAndInstruction();
      TRACE_EXECUTION_LAST_EIP = m_cpu->m_current_EIP;
    }

    // Read and decode an instruction from the current IP.
    ExecuteInstruction();

    if (trap_after_instruction)
    {
      // We should push the next instruction pointer, not the instruction that's trapping,
      // since it has already executed. We also can't use m_cpu->RaiseException since this would
      // reset the stack pointer too (and it could be a stack-modifying instruction). We
      // also don't need to abort the current instruction since we're looping anyway.
      m_cpu->SetupInterruptCall(Interrupt_Debugger, true, false, 0, m_cpu->m_registers.EIP);
    }

    // Run events if needed.
    m_cpu->CommitPendingCycles();
  }
}

void FastInterpreterBackend::AbortCurrentInstruction()
{
  // Log_WarningPrintf("Executing longjmp()");
  m_cpu->CommitPendingCycles();
  longjmp(m_jmp_buf, 1);
}

void FastInterpreterBackend::BranchTo(uint32 new_EIP) {}

void FastInterpreterBackend::BranchFromException(uint32 new_EIP) {}

void FastInterpreterBackend::OnControlRegisterLoaded(Reg32 reg, uint32 old_value, uint32 new_value) {}

void FastInterpreterBackend::FlushCodeCache() {}

void FastInterpreterBackend::RaiseInvalidOpcode()
{
  // This set is only here because of the Print call. RaiseException will reset it itself.
  m_cpu->m_registers.EIP = m_cpu->m_current_EIP;
  m_cpu->PrintCurrentStateAndInstruction("Invalid opcode raised");
  m_cpu->RaiseException(Interrupt_InvalidOpcode);
}

void FastInterpreterBackend::FallbackToInterpreter()
{
  // Reset EIP, so that we start fetching from the beginning of the instruction again.
  m_cpu->RestartCurrentInstruction();
  // m_cpu->PrintCurrentStateAndInstruction("Fallback");

  // Fetch the instruction, raising exceptions as needed.
  struct Callback : public InstructionFetchCallback
  {
    Callback(CPU* cpu_) : cpu(cpu_) {}

    uint8 FetchByte() override { return cpu->FetchInstructionByte(); }

    uint16 FetchWord() override { return cpu->FetchInstructionWord(); }

    uint32 FetchDWord() override { return cpu->FetchInstructionDWord(); }

    CPU* cpu;
  };

  // Actually fetch the instruction.
  OldInstruction instruction;
  Callback callback(m_cpu);
  if (!DecodeInstruction(&instruction, m_cpu->m_current_address_size, m_cpu->m_current_operand_size, callback))
  {
    // The interpreter failed too.
    RaiseInvalidOpcode();
    return;
  }

  // Now execute it in the interpreter.
  // EIP should be up to date since we used our Fetch method(s).
  Interpreter::ExecuteInstruction(m_cpu, &instruction);
}

void FastInterpreterBackend::FetchModRM(InstructionState* istate)
{
  istate->modrm = m_cpu->FetchInstructionByte();
}

template<FIOperandMode op_mode, OperandSize op_size>
void FastInterpreterBackend::FetchImmediate(InstructionState* istate)
{
  switch (op_mode)
  {
    case FIOperandMode_Immediate:
    case FIOperandMode_Relative:
    {
      OperandSize actual_size = (op_size == OperandSize_Count) ? istate->operand_size : op_size;
      switch (actual_size)
      {
        case OperandSize_8:
          istate->immediate.value8 = m_cpu->FetchInstructionByte();
          break;
        case OperandSize_16:
          istate->immediate.value16 = m_cpu->FetchInstructionWord();
          break;
        case OperandSize_32:
          istate->immediate.value32 = m_cpu->FetchInstructionDWord();
          break;
      }
    }
    break;

    case FIOperandMode_Memory:
    {
      if (istate->address_size == AddressSize_16)
        istate->effective_address = m_cpu->FetchInstructionWord();
      else
        istate->effective_address = m_cpu->FetchInstructionDWord();
    }
    break;
  }
}

template<FIOperandMode op_mode>
void FastInterpreterBackend::CalculateEffectiveAddress(InstructionState* istate)
{
  switch (op_mode)
  {
    case FIOperandMode_ModRM_RM:
    {
      // NOTE: The uint16() cast here is needed otherwise the result is an int rather than uint16.
      uint8 index = ((istate->modrm >> 6) << 3) | (istate->modrm & 7);
      istate->modrm_rm_register = false;
      if (istate->address_size == AddressSize_16)
      {
        switch (index)
        {
          case 0:
            istate->effective_address = ZeroExtend32(uint16(m_cpu->m_registers.BX + m_cpu->m_registers.SI));
            break;
          case 1:
            istate->effective_address = ZeroExtend32(uint16(m_cpu->m_registers.BX + m_cpu->m_registers.DI));
            break;
          case 2:
            istate->effective_address = ZeroExtend32(uint16(m_cpu->m_registers.BP + m_cpu->m_registers.SI));
            if (!istate->has_segment_override)
              istate->segment = Segment_SS;
            break;
          case 3:
            istate->effective_address = ZeroExtend32(uint16(m_cpu->m_registers.BP + m_cpu->m_registers.DI));
            if (!istate->has_segment_override)
              istate->segment = Segment_SS;
            break;
          case 4:
            istate->effective_address = ZeroExtend32(m_cpu->m_registers.SI);
            break;
          case 5:
            istate->effective_address = ZeroExtend32(m_cpu->m_registers.DI);
            break;
          case 6:
            istate->effective_address = ZeroExtend32(m_cpu->FetchInstructionWord());
            break;
          case 7:
            istate->effective_address = ZeroExtend32(m_cpu->m_registers.BX);
            break;
          case 8:
            istate->effective_address = ZeroExtend32(
              uint16(m_cpu->m_registers.BX + m_cpu->m_registers.SI + SignExtend16(m_cpu->FetchInstructionByte())));
            break;
          case 9:
            istate->effective_address = ZeroExtend32(
              uint16(m_cpu->m_registers.BX + m_cpu->m_registers.DI + SignExtend16(m_cpu->FetchInstructionByte())));
            break;
          case 10:
            istate->effective_address = ZeroExtend32(
              uint16(m_cpu->m_registers.BP + m_cpu->m_registers.SI + SignExtend16(m_cpu->FetchInstructionByte())));
            if (!istate->has_segment_override)
              istate->segment = Segment_SS;
            break;
          case 11:
            istate->effective_address = ZeroExtend32(
              uint16(m_cpu->m_registers.BP + m_cpu->m_registers.DI + SignExtend16(m_cpu->FetchInstructionByte())));
            if (!istate->has_segment_override)
              istate->segment = Segment_SS;
            break;
          case 12:
            istate->effective_address =
              ZeroExtend32(uint16(m_cpu->m_registers.SI + SignExtend16(m_cpu->FetchInstructionByte())));
            break;
          case 13:
            istate->effective_address =
              ZeroExtend32(uint16(m_cpu->m_registers.DI + SignExtend16(m_cpu->FetchInstructionByte())));
            break;
          case 14:
            istate->effective_address =
              ZeroExtend32(uint16(m_cpu->m_registers.BP + SignExtend16(m_cpu->FetchInstructionByte())));
            if (!istate->has_segment_override)
              istate->segment = Segment_SS;
            break;
          case 15:
            istate->effective_address =
              ZeroExtend32(uint16(m_cpu->m_registers.BX + SignExtend16(m_cpu->FetchInstructionByte())));
            break;
          case 16:
            istate->effective_address = ZeroExtend32(
              uint16(m_cpu->m_registers.BX + m_cpu->m_registers.SI + SignExtend16(m_cpu->FetchInstructionWord())));
            break;
          case 17:
            istate->effective_address = ZeroExtend32(
              uint16(m_cpu->m_registers.BX + m_cpu->m_registers.DI + SignExtend16(m_cpu->FetchInstructionWord())));
            break;
          case 18:
            istate->effective_address = ZeroExtend32(
              uint16(m_cpu->m_registers.BP + m_cpu->m_registers.SI + SignExtend16(m_cpu->FetchInstructionWord())));
            if (!istate->has_segment_override)
              istate->segment = Segment_SS;
            break;
          case 19:
            istate->effective_address = ZeroExtend32(
              uint16(m_cpu->m_registers.BP + m_cpu->m_registers.DI + SignExtend16(m_cpu->FetchInstructionWord())));
            if (!istate->has_segment_override)
              istate->segment = Segment_SS;
            break;
          case 20:
            istate->effective_address =
              ZeroExtend32(uint16(m_cpu->m_registers.SI + SignExtend16(m_cpu->FetchInstructionWord())));
            break;
          case 21:
            istate->effective_address =
              ZeroExtend32(uint16(m_cpu->m_registers.DI + SignExtend16(m_cpu->FetchInstructionWord())));
            break;
          case 22:
            istate->effective_address =
              ZeroExtend32(uint16(m_cpu->m_registers.BP + SignExtend16(m_cpu->FetchInstructionWord())));
            if (!istate->has_segment_override)
              istate->segment = Segment_SS;
            break;
          case 23:
            istate->effective_address =
              ZeroExtend32(uint16(m_cpu->m_registers.BX + SignExtend16(m_cpu->FetchInstructionWord())));
            break;
          case 24:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg16_AX;
            break;
          case 25:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg16_CX;
            break;
          case 26:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg16_DX;
            break;
          case 27:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg16_BX;
            break;
          case 28:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg16_SP;
            break;
          case 29:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg16_BP;
            break;
          case 30:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg16_SI;
            break;
          case 31:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg16_DI;
            break;
        }
      }
      else
      {
        switch (index)
        {
          case 0:
            istate->effective_address = m_cpu->m_registers.EAX;
            break;
          case 1:
            istate->effective_address = m_cpu->m_registers.ECX;
            break;
          case 2:
            istate->effective_address = m_cpu->m_registers.EDX;
            break;
          case 3:
            istate->effective_address = m_cpu->m_registers.EBX;
            break;
          case 5:
            istate->effective_address = m_cpu->FetchInstructionDWord();
            break;
          case 6:
            istate->effective_address = m_cpu->m_registers.ESI;
            break;
          case 7:
            istate->effective_address = m_cpu->m_registers.EDI;
            break;
          case 8:
            istate->effective_address = m_cpu->m_registers.EAX + SignExtend32(m_cpu->FetchInstructionByte());
            break;
          case 9:
            istate->effective_address = m_cpu->m_registers.ECX + SignExtend32(m_cpu->FetchInstructionByte());
            break;
          case 10:
            istate->effective_address = m_cpu->m_registers.EDX + SignExtend32(m_cpu->FetchInstructionByte());
            break;
          case 11:
            istate->effective_address = m_cpu->m_registers.EBX + SignExtend32(m_cpu->FetchInstructionByte());
            break;
          case 13:
            istate->effective_address = m_cpu->m_registers.EBP + SignExtend32(m_cpu->FetchInstructionByte());
            if (!istate->has_segment_override)
              istate->segment = Segment_SS;
            break;
          case 14:
            istate->effective_address = m_cpu->m_registers.ESI + SignExtend32(m_cpu->FetchInstructionByte());
            break;
          case 15:
            istate->effective_address = m_cpu->m_registers.EDI + SignExtend32(m_cpu->FetchInstructionByte());
            break;
          case 16:
            istate->effective_address = m_cpu->m_registers.EAX + m_cpu->FetchInstructionDWord();
            break;
          case 17:
            istate->effective_address = m_cpu->m_registers.ECX + m_cpu->FetchInstructionDWord();
            break;
          case 18:
            istate->effective_address = m_cpu->m_registers.EDX + m_cpu->FetchInstructionDWord();
            break;
          case 19:
            istate->effective_address = m_cpu->m_registers.EBX + m_cpu->FetchInstructionDWord();
            break;
          case 21:
            istate->effective_address = m_cpu->m_registers.EBP + m_cpu->FetchInstructionDWord();
            if (!istate->has_segment_override)
              istate->segment = Segment_SS;
            break;
          case 22:
            istate->effective_address = m_cpu->m_registers.ESI + m_cpu->FetchInstructionDWord();
            break;
          case 23:
            istate->effective_address = m_cpu->m_registers.EDI + m_cpu->FetchInstructionDWord();
            break;
          case 24:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg32_EAX;
            break;
          case 25:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg32_ECX;
            break;
          case 26:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg32_EDX;
            break;
          case 27:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg32_EBX;
            break;
          case 28:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg32_ESP;
            break;
          case 29:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg32_EBP;
            break;
          case 30:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg32_ESI;
            break;
          case 31:
            istate->modrm_rm_register = true;
            istate->effective_address = Reg32_EDI;
            break;

          case 4:
          case 12:
          case 20:
          {
            // SIB modes
            uint8 sib = m_cpu->FetchInstructionByte();
            uint8 base_register = (sib & 0x7);
            uint8 index_register = ((sib >> 3) & 0x7);
            uint8 scaling_factor = ((sib >> 6) & 0x3);

            uint32 base_addr;
            uint32 displacement = 0;
            if (index == 12)
              displacement = SignExtend32(m_cpu->FetchInstructionByte());
            else if (index == 20)
              displacement = m_cpu->FetchInstructionDWord();

            if (base_register == Reg32_ESP)
            {
              // Default to SS segment
              base_addr = m_cpu->m_registers.ESP;
              if (!istate->has_segment_override)
                istate->segment = Segment_SS;
            }
            else if (base_register == Reg32_EBP)
            {
              // EBP means no base if mod == 00, EBP otherwise
              if (index == 4)
              {
                // Though we do have a displacement dword
                base_addr = 0;
                displacement = m_cpu->FetchInstructionDWord();
              }
              else
              {
                // EBP register also defaults to stack segment
                // This isn't documented in the Intel manual...
                base_addr = m_cpu->m_registers.EBP;
                if (!istate->has_segment_override)
                  istate->segment = Segment_SS;
              }
            }
            else
            {
              base_addr = m_cpu->m_registers.reg32[base_register];
            }
            uint32 index_addr;
            if (index_register == Reg32_ESP)
            {
              // ESP means no index
              index_addr = 0;
            }
            else
            {
              index_addr = m_cpu->m_registers.reg32[index_register];
            }

            istate->effective_address = base_addr;
            istate->effective_address += index_addr << scaling_factor;
            istate->effective_address += displacement;
          }
          break;
        }
      }
    }
    break;
  }
}

inline constexpr bool NeedsModRM(FIOperandMode op_mode, OperandSize op_size = OperandSize_16, uint32 constant = 0)
{
  return op_mode == FIOperandMode_ModRM_Reg || op_mode == FIOperandMode_ModRM_RM ||
         op_mode == FIOperandMode_ModRM_ControlRegister || op_mode == FIOperandMode_ModRM_DebugRegister ||
         op_mode == FIOperandMode_ModRM_TestRegister;
}

template<FIOperandMode mode, uint32 constant>
uint8 FastInterpreterBackend::ReadByteOperand(InstructionState* istate)
{
  switch (mode)
  {
    case FIOperandMode_Constant:
      return Truncate8(constant);
    case FIOperandMode_Register:
      return m_cpu->m_registers.reg8[constant];
    case FIOperandMode_Immediate:
      return istate->immediate.value8;
      //     case FIOperandMode_RegisterIndirect:
      //         {
      //             uint8 value;
      //             if (istate->address_size == AddressSize_16)
      //                 m_cpu->ReadMemoryByte(istate->segment, ZeroExtend32(m_cpu->m_registers.reg16[constant]),
      //                 &value);
      //             else
      //                 m_cpu->ReadMemoryByte(istate->segment, m_cpu->m_registers.reg32[constant], &value);
      //             return value;
      //         }
    case FIOperandMode_Memory:
      return m_cpu->ReadMemoryByte(istate->segment, istate->effective_address);
    case FIOperandMode_ModRM_RM:
    {
      if (istate->modrm_rm_register)
        return m_cpu->m_registers.reg8[istate->effective_address];
      else
        return m_cpu->ReadMemoryByte(istate->segment, istate->effective_address);
    }
    case FIOperandMode_ModRM_Reg:
      return m_cpu->m_registers.reg8[istate->GetModRM_Reg()];
    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<FIOperandMode mode, uint32 constant>
uint16 FastInterpreterBackend::ReadWordOperand(InstructionState* istate)
{
  switch (mode)
  {
    case FIOperandMode_Constant:
      return Truncate16(constant);
    case FIOperandMode_Register:
      return m_cpu->m_registers.reg16[constant];
    case FIOperandMode_Immediate:
      return istate->immediate.value16;
    case FIOperandMode_Memory:
      return m_cpu->ReadMemoryWord(istate->segment, istate->effective_address);
    case FIOperandMode_ModRM_RM:
    {
      if (istate->modrm_rm_register)
        return m_cpu->m_registers.reg16[istate->effective_address];
      else
        return m_cpu->ReadMemoryWord(istate->segment, istate->effective_address);
    }
    case FIOperandMode_ModRM_Reg:
      return m_cpu->m_registers.reg16[istate->GetModRM_Reg()];

    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<FIOperandMode mode, uint32 constant>
uint32 FastInterpreterBackend::ReadDWordOperand(InstructionState* istate)
{
  switch (mode)
  {
    case FIOperandMode_Constant:
      return constant;
    case FIOperandMode_Register:
      return m_cpu->m_registers.reg32[constant];
    case FIOperandMode_Immediate:
      return istate->immediate.value32;
    case FIOperandMode_Memory:
      return m_cpu->ReadMemoryDWord(istate->segment, istate->effective_address);
    case FIOperandMode_ModRM_RM:
    {
      if (istate->modrm_rm_register)
        return m_cpu->m_registers.reg32[istate->effective_address];
      else
        return m_cpu->ReadMemoryDWord(istate->segment, istate->effective_address);
    }
    case FIOperandMode_ModRM_Reg:
      return m_cpu->m_registers.reg32[istate->GetModRM_Reg()];
      //     case FIOperandMode_ModRM_ControlRegister:
      //         {
      //             uint8 reg = (istate->modrm >> 3) & 7;
      //             if (reg > 4)
      //                 RaiseInvalidOpcode();
      //             return m_cpu->m_registers.reg32[Reg32_CR0 + reg];
      //         }
    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<FIOperandMode mode, OperandSize size, uint32 constant>
uint16 CPU_X86::FastInterpreterBackend::ReadSignExtendedWordOperand(InstructionState* istate)
{
  OperandSize actual_size = (size == OperandSize_Count) ? istate->operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      uint8 value;
      switch (mode)
      {
        case FIOperandMode_Register:
          value = m_cpu->m_registers.reg8[constant];
          break;
        case FIOperandMode_Immediate:
          value = istate->immediate.value8;
          break;
        case FIOperandMode_Memory:
          value = m_cpu->ReadMemoryByte(istate->segment, istate->effective_address);
          break;
        case FIOperandMode_ModRM_Reg:
          value = m_cpu->m_registers.reg8[istate->GetModRM_Reg()];
          break;
        case FIOperandMode_ModRM_RM:
          if (istate->modrm_rm_register)
            value = m_cpu->m_registers.reg8[istate->effective_address];
          else
            value = m_cpu->ReadMemoryByte(istate->segment, istate->effective_address);
          break;
        default:
          DebugUnreachableCode();
          return 0;
      }
      return SignExtend16(value);
    }
    case OperandSize_16:
      return ReadWordOperand<mode, constant>(istate);
    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<FIOperandMode mode, OperandSize size, uint32 constant>
uint32 CPU_X86::FastInterpreterBackend::ReadSignExtendedDWordOperand(InstructionState* istate)
{
  OperandSize actual_size = (size == OperandSize_Count) ? istate->operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      uint8 value;
      switch (mode)
      {
        case FIOperandMode_Register:
          value = m_cpu->m_registers.reg8[constant];
          break;
        case FIOperandMode_Immediate:
          value = istate->immediate.value8;
          break;
        case FIOperandMode_Memory:
          value = m_cpu->ReadMemoryByte(istate->segment, istate->effective_address);
          break;
        case FIOperandMode_ModRM_Reg:
          value = m_cpu->m_registers.reg8[istate->GetModRM_Reg()];
          break;
        case FIOperandMode_ModRM_RM:
          if (istate->modrm_rm_register)
            value = m_cpu->m_registers.reg8[istate->effective_address];
          else
            value = m_cpu->ReadMemoryByte(istate->segment, istate->effective_address);
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
        case FIOperandMode_Register:
          value = m_cpu->m_registers.reg16[constant];
          break;
        case FIOperandMode_Immediate:
          value = istate->immediate.value16;
          break;
        case FIOperandMode_Memory:
          value = m_cpu->ReadMemoryWord(istate->segment);
          break;
        case FIOperandMode_ModRM_Reg:
          value = m_cpu->m_registers.reg16[istate->GetModRM_Reg()];
          break;
        case FIOperandMode_ModRM_RM:
          if (istate->modrm_rm_register)
            value = m_cpu->m_registers.reg16[istate->effective_address];
          else
            value = m_cpu->ReadMemoryWord(istate->segment, istate->effective_address);
          break;
        default:
          DebugUnreachableCode();
          return 0;
      }
      return SignExtend32(value);
    }
    case OperandSize_32:
      return ReadDWordOperand<mode, constant>(istate);
    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<FIOperandMode mode, OperandSize size, uint32 constant>
uint16 CPU_X86::FastInterpreterBackend::ReadZeroExtendedWordOperand(InstructionState* istate)
{
  OperandSize actual_size = (size == OperandSize_Count) ? istate->operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      uint8 value;
      switch (mode)
      {
        case FIOperandMode_Constant:
          value = Truncate8(constant);
          break;
        case FIOperandMode_Register:
          value = m_cpu->m_registers.reg8[constant];
          break;
        case FIOperandMode_Immediate:
          value = istate->immediate.value8;
          break;
        case FIOperandMode_Memory:
          value = m_cpu->ReadMemoryByte(istate->segment, istate->effective_address);
          break;
        case FIOperandMode_ModRM_Reg:
          value = m_cpu->m_registers.reg8[istate->GetModRM_Reg()];
          break;
        case FIOperandMode_ModRM_RM:
          if (istate->modrm_rm_register)
            value = m_cpu->m_registers.reg8[istate->effective_address];
          else
            value = m_cpu->ReadMemoryByte(istate->segment, istate->effective_address);
          break;
        default:
          DebugUnreachableCode();
          return 0;
      }
      return ZeroExtend16(value);
    }
    case OperandSize_16:
      return ReadWordOperand<mode, constant>(istate);
    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<FIOperandMode mode, OperandSize size, uint32 constant>
uint32 CPU_X86::FastInterpreterBackend::ReadZeroExtendedDWordOperand(InstructionState* istate)
{
  OperandSize actual_size = (size == OperandSize_Count) ? istate->operand_size : size;
  switch (actual_size)
  {
    case OperandSize_8:
    {
      uint8 value;
      switch (mode)
      {
        case FIOperandMode_Constant:
          value = Truncate8(constant);
          break;
        case FIOperandMode_Register:
          value = m_cpu->m_registers.reg8[constant];
          break;
        case FIOperandMode_Immediate:
          value = istate->immediate.value8;
          break;
        case FIOperandMode_Memory:
          value = m_cpu->ReadMemoryByte(istate->segment, istate->effective_address);
          break;
        case FIOperandMode_ModRM_Reg:
          value = m_cpu->m_registers.reg8[istate->GetModRM_Reg()];
          break;
        case FIOperandMode_ModRM_RM:
          if (istate->modrm_rm_register)
            value = m_cpu->m_registers.reg8[istate->effective_address];
          else
            value = m_cpu->ReadMemoryByte(istate->segment, istate->effective_address);
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
        case FIOperandMode_Constant:
          value = Truncate16(constant);
          break;
        case FIOperandMode_Register:
          value = m_cpu->m_registers.reg16[constant];
          break;
        case FIOperandMode_Immediate:
          value = istate->immediate.value16;
          break;
        case FIOperandMode_Memory:
          value = m_cpu->ReadMemoryWord(istate->segment);
          break;
        case FIOperandMode_ModRM_Reg:
          value = m_cpu->m_registers.reg16[istate->GetModRM_Reg()];
          break;
        case FIOperandMode_ModRM_RM:
          if (istate->modrm_rm_register)
            value = m_cpu->m_registers.reg16[istate->effective_address];
          else
            value = m_cpu->ReadMemoryWord(istate->segment, istate->effective_address);
          break;
        default:
          DebugUnreachableCode();
          return 0;
      }
      return ZeroExtend32(value);
    }
    case OperandSize_32:
      return ReadDWordOperand<mode, constant>(istate);
    default:
      DebugUnreachableCode();
      return 0;
  }
}

template<FIOperandMode mode, uint32 constant>
void CPU_X86::FastInterpreterBackend::WriteByteOperand(InstructionState* istate, uint8 value)
{
  switch (mode)
  {
    case FIOperandMode_Register:
      m_cpu->m_registers.reg8[constant] = value;
      break;
    case FIOperandMode_Memory:
      m_cpu->WriteMemoryByte(istate->segment, istate->effective_address, value);
      break;

    case FIOperandMode_ModRM_RM:
    {
      if (istate->modrm_rm_register)
        m_cpu->m_registers.reg8[istate->effective_address] = value;
      else
        m_cpu->WriteMemoryByte(istate->segment, istate->effective_address, value);
    }
    break;

    case FIOperandMode_ModRM_Reg:
      m_cpu->m_registers.reg8[istate->GetModRM_Reg()] = value;
      break;

    default:
      DebugUnreachableCode();
      break;
  }
}

template<FIOperandMode mode, uint32 constant>
void CPU_X86::FastInterpreterBackend::WriteWordOperand(InstructionState* istate, uint16 value)
{
  switch (mode)
  {
    case FIOperandMode_Register:
      m_cpu->m_registers.reg16[constant] = value;
      break;
    case FIOperandMode_Memory:
      m_cpu->WriteMemoryWord(istate->segment, istate->effective_address, value);
      break;

    case FIOperandMode_ModRM_RM:
    {
      if (istate->modrm_rm_register)
        m_cpu->m_registers.reg16[istate->effective_address] = value;
      else
        m_cpu->WriteMemoryWord(istate->segment, istate->effective_address, value);
    }
    break;

    case FIOperandMode_ModRM_Reg:
      m_cpu->m_registers.reg16[istate->GetModRM_Reg()] = value;
      break;

    default:
      DebugUnreachableCode();
      break;
  }
}

template<FIOperandMode mode, uint32 constant>
void CPU_X86::FastInterpreterBackend::WriteDWordOperand(InstructionState* istate, uint32 value)
{
  switch (mode)
  {
    case FIOperandMode_Register:
      m_cpu->m_registers.reg32[constant] = value;
      break;
    case FIOperandMode_Memory:
      m_cpu->WriteMemoryDWord(istate->segment, istate->effective_address, value);
      break;

    case FIOperandMode_ModRM_RM:
    {
      if (istate->modrm_rm_register)
        m_cpu->m_registers.reg32[istate->effective_address] = value;
      else
        m_cpu->WriteMemoryDWord(istate->segment, istate->effective_address, value);
    }
    break;

    case FIOperandMode_ModRM_Reg:
      m_cpu->m_registers.reg32[istate->GetModRM_Reg()] = value;
      break;

    default:
      DebugUnreachableCode();
      break;
  }
}

template<FIOperandMode mode>
void CPU_X86::FastInterpreterBackend::ReadFarAddressOperand(InstructionState* istate, OperandSize size,
                                                            uint16* segment_selector, VirtualMemoryAddress* address)
{
  // Can either be far immediate, or memory
  switch (mode)
  {
    case FIOperandMode_FarMemory:
    {
      if (size == OperandSize_16)
        *address = ZeroExtend32(m_cpu->FetchInstructionWord());
      else
        *address = m_cpu->FetchInstructionDWord();
      *segment_selector = m_cpu->FetchInstructionWord();
    }
    break;

    case FIOperandMode_Memory:
    case FIOperandMode_ModRM_RM:
    {
      if (size == OperandSize_16)
      {
        *address = ZeroExtend32(m_cpu->ReadMemoryWord(istate->segment, istate->effective_address));
        *segment_selector = m_cpu->ReadMemoryWord(istate->segment, istate->effective_address + 2);
      }
      else
      {
        *address = m_cpu->ReadMemoryDWord(istate->segment, istate->effective_address);
        *segment_selector = m_cpu->ReadMemoryWord(istate->segment, istate->effective_address + 4);
      }
    }
    break;
    default:
      DebugUnreachableCode();
      break;
  }
}

template<JumpCondition condition>
bool CPU_X86::FastInterpreterBackend::TestJumpCondition(InstructionState* istate)
{
  switch (condition)
  {
    case JumpCondition_Always:
      return true;

    case JumpCondition_Overflow:
      return m_cpu->m_registers.EFLAGS.OF;

    case JumpCondition_NotOverflow:
      return !m_cpu->m_registers.EFLAGS.OF;

    case JumpCondition_Sign:
      return m_cpu->m_registers.EFLAGS.SF;

    case JumpCondition_NotSign:
      return !m_cpu->m_registers.EFLAGS.SF;

    case JumpCondition_Equal:
      return m_cpu->m_registers.EFLAGS.ZF;

    case JumpCondition_NotEqual:
      return !m_cpu->m_registers.EFLAGS.ZF;

    case JumpCondition_Below:
      return m_cpu->m_registers.EFLAGS.CF;

    case JumpCondition_AboveOrEqual:
      return !m_cpu->m_registers.EFLAGS.CF;

    case JumpCondition_BelowOrEqual:
      return (m_cpu->m_registers.EFLAGS.CF | m_cpu->m_registers.EFLAGS.ZF);

    case JumpCondition_Above:
      return !(m_cpu->m_registers.EFLAGS.CF | m_cpu->m_registers.EFLAGS.ZF);

    case JumpCondition_Less:
      return (m_cpu->m_registers.EFLAGS.SF != m_cpu->m_registers.EFLAGS.OF);

    case JumpCondition_GreaterOrEqual:
      return (m_cpu->m_registers.EFLAGS.SF == m_cpu->m_registers.EFLAGS.OF);

    case JumpCondition_LessOrEqual:
      return (m_cpu->m_registers.EFLAGS.ZF || (m_cpu->m_registers.EFLAGS.SF != m_cpu->m_registers.EFLAGS.OF));

    case JumpCondition_Greater:
      return (!m_cpu->m_registers.EFLAGS.ZF && (m_cpu->m_registers.EFLAGS.SF == m_cpu->m_registers.EFLAGS.OF));

    case JumpCondition_Parity:
      return m_cpu->m_registers.EFLAGS.PF;

    case JumpCondition_NotParity:
      return !m_cpu->m_registers.EFLAGS.PF;

    case JumpCondition_CXZero:
    {
      if (istate->address_size == AddressSize_16)
        return (m_cpu->m_registers.CX == 0);
      else
        return (m_cpu->m_registers.ECX == 0);
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

inline uint8 ALUOp_Add8(CPU_X86::CPU::Registers* registers, uint8 lhs, uint8 rhs)
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

inline uint8 ALUOp_Adc8(CPU_X86::CPU::Registers* registers, uint8 lhs, uint8 rhs)
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

inline uint8 ALUOp_Sub8(CPU_X86::CPU::Registers* registers, uint8 lhs, uint8 rhs)
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

inline uint8 ALUOp_Sbb8(CPU_X86::CPU::Registers* registers, uint8 lhs, uint8 rhs)
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

inline uint16 ALUOp_Add16(CPU_X86::CPU::Registers* registers, uint16 lhs, uint16 rhs)
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

inline uint16 ALUOp_Adc16(CPU_X86::CPU::Registers* registers, uint16 lhs, uint16 rhs)
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

inline uint16 ALUOp_Sub16(CPU_X86::CPU::Registers* registers, uint16 lhs, uint16 rhs)
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

inline uint16 ALUOp_Sbb16(CPU_X86::CPU::Registers* registers, uint16 lhs, uint16 rhs)
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

inline uint32 ALUOp_Add32(CPU_X86::CPU::Registers* registers, uint32 lhs, uint32 rhs)
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

inline uint32 ALUOp_Adc32(CPU_X86::CPU::Registers* registers, uint32 lhs, uint32 rhs)
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

inline uint32 ALUOp_Sub32(CPU_X86::CPU::Registers* registers, uint32 lhs, uint32 rhs)
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

inline uint32 ALUOp_Sbb32(CPU_X86::CPU::Registers* registers, uint32 lhs, uint32 rhs)
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

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_ADD(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(istate);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(istate);
    uint8 new_value = ALUOp_Add8(&m_cpu->m_registers, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(istate);
    uint16 rhs = ReadSignExtendedWordOperand<src_mode, src_size, src_constant>(istate);
    uint16 new_value = ALUOp_Add16(&m_cpu->m_registers, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(istate);
    uint32 rhs = ReadSignExtendedDWordOperand<src_mode, src_size, src_constant>(istate);
    uint32 new_value = ALUOp_Add32(&m_cpu->m_registers, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(istate, new_value);
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_ADC(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(istate);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(istate);
    uint8 new_value = ALUOp_Adc8(&m_cpu->m_registers, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(istate);
    uint16 rhs = ReadSignExtendedWordOperand<src_mode, src_size, src_constant>(istate);
    uint16 new_value = ALUOp_Adc16(&m_cpu->m_registers, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(istate);
    uint32 rhs = ReadSignExtendedDWordOperand<src_mode, src_size, src_constant>(istate);
    uint32 new_value = ALUOp_Adc32(&m_cpu->m_registers, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(istate, new_value);
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_SUB(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(istate);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(istate);
    uint8 new_value = ALUOp_Sub8(&m_cpu->m_registers, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(istate);
    uint16 rhs = ReadSignExtendedWordOperand<src_mode, src_size, src_constant>(istate);
    uint16 new_value = ALUOp_Sub16(&m_cpu->m_registers, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(istate);
    uint32 rhs = ReadSignExtendedDWordOperand<src_mode, src_size, src_constant>(istate);
    uint32 new_value = ALUOp_Sub32(&m_cpu->m_registers, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(istate, new_value);
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_SBB(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(istate);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(istate);
    uint8 new_value = ALUOp_Sbb8(&m_cpu->m_registers, lhs, rhs);
    WriteByteOperand<dst_mode, dst_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(istate);
    uint16 rhs = ReadSignExtendedWordOperand<src_mode, src_size, src_constant>(istate);
    uint16 new_value = ALUOp_Sbb16(&m_cpu->m_registers, lhs, rhs);
    WriteWordOperand<dst_mode, dst_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(istate);
    uint32 rhs = ReadSignExtendedDWordOperand<src_mode, src_size, src_constant>(istate);
    uint32 new_value = ALUOp_Sbb32(&m_cpu->m_registers, lhs, rhs);
    WriteDWordOperand<dst_mode, dst_constant>(istate, new_value);
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_CMP(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  // Implemented as subtract but discarding the result
  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(istate);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(istate);
    ALUOp_Sub8(&m_cpu->m_registers, lhs, rhs);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(istate);
    uint16 rhs = ReadSignExtendedWordOperand<src_mode, src_size, src_constant>(istate);
    ALUOp_Sub16(&m_cpu->m_registers, lhs, rhs);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(istate);
    uint32 rhs = ReadSignExtendedDWordOperand<src_mode, src_size, src_constant>(istate);
    ALUOp_Sub32(&m_cpu->m_registers, lhs, rhs);
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_AND(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  bool sf, zf, pf;
  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(istate);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(istate);
    uint8 new_value = lhs & rhs;
    WriteByteOperand<dst_mode, dst_constant>(istate, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(istate);
    uint16 rhs = ReadSignExtendedWordOperand<src_mode, src_size, src_constant>(istate);
    uint16 new_value = lhs & rhs;
    WriteWordOperand<dst_mode, dst_constant>(istate, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(istate);
    uint32 rhs = ReadSignExtendedDWordOperand<src_mode, src_size, src_constant>(istate);
    uint32 new_value = lhs & rhs;
    WriteDWordOperand<dst_mode, dst_constant>(istate, new_value);

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
  SET_FLAG(&m_cpu->m_registers, OF, false);
  SET_FLAG(&m_cpu->m_registers, CF, false);
  SET_FLAG(&m_cpu->m_registers, SF, sf);
  SET_FLAG(&m_cpu->m_registers, ZF, zf);
  SET_FLAG(&m_cpu->m_registers, PF, pf);
  SET_FLAG(&m_cpu->m_registers, AF, false);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_OR(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  bool sf, zf, pf;
  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(istate);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(istate);
    uint8 new_value = lhs | rhs;
    WriteByteOperand<dst_mode, dst_constant>(istate, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(istate);
    uint16 rhs = ReadSignExtendedWordOperand<src_mode, src_size, src_constant>(istate);
    uint16 new_value = lhs | rhs;
    WriteWordOperand<dst_mode, dst_constant>(istate, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(istate);
    uint32 rhs = ReadSignExtendedDWordOperand<src_mode, src_size, src_constant>(istate);
    uint32 new_value = lhs | rhs;
    WriteDWordOperand<dst_mode, dst_constant>(istate, new_value);

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
  SET_FLAG(&m_cpu->m_registers, OF, false);
  SET_FLAG(&m_cpu->m_registers, CF, false);
  SET_FLAG(&m_cpu->m_registers, SF, sf);
  SET_FLAG(&m_cpu->m_registers, ZF, zf);
  SET_FLAG(&m_cpu->m_registers, PF, pf);
  SET_FLAG(&m_cpu->m_registers, AF, false);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_XOR(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  bool sf, zf, pf;
  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(istate);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(istate);
    uint8 new_value = lhs ^ rhs;
    WriteByteOperand<dst_mode, dst_constant>(istate, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(istate);
    uint16 rhs = ReadSignExtendedWordOperand<src_mode, src_size, src_constant>(istate);
    uint16 new_value = lhs ^ rhs;
    WriteWordOperand<dst_mode, dst_constant>(istate, new_value);

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(istate);
    uint32 rhs = ReadSignExtendedDWordOperand<src_mode, src_size, src_constant>(istate);
    uint32 new_value = lhs ^ rhs;
    WriteDWordOperand<dst_mode, dst_constant>(istate, new_value);

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
  SET_FLAG(&m_cpu->m_registers, OF, false);
  SET_FLAG(&m_cpu->m_registers, CF, false);
  SET_FLAG(&m_cpu->m_registers, SF, sf);
  SET_FLAG(&m_cpu->m_registers, ZF, zf);
  SET_FLAG(&m_cpu->m_registers, PF, pf);
  SET_FLAG(&m_cpu->m_registers, AF, false);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_TEST(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  bool sf, zf, pf;
  if (actual_size == OperandSize_8)
  {
    uint8 lhs = ReadByteOperand<dst_mode, dst_constant>(istate);
    uint8 rhs = ReadByteOperand<src_mode, src_constant>(istate);
    uint8 new_value = lhs & rhs;

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = ReadWordOperand<dst_mode, dst_constant>(istate);
    uint16 rhs = ReadWordOperand<src_mode, src_constant>(istate);
    uint16 new_value = lhs & rhs;

    sf = IsSign(new_value);
    zf = IsZero(new_value);
    pf = IsParity(new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = ReadDWordOperand<dst_mode, dst_constant>(istate);
    uint32 rhs = ReadDWordOperand<src_mode, src_constant>(istate);
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
  SET_FLAG(&m_cpu->m_registers, OF, false);
  SET_FLAG(&m_cpu->m_registers, CF, false);
  SET_FLAG(&m_cpu->m_registers, SF, sf);
  SET_FLAG(&m_cpu->m_registers, ZF, zf);
  SET_FLAG(&m_cpu->m_registers, PF, pf);
  SET_FLAG(&m_cpu->m_registers, AF, false);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_MOV(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  static_assert(dst_size == src_size, "dst_size == src_size");
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<src_mode, src_constant>(istate);
    WriteByteOperand<dst_mode, dst_constant>(istate, value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<src_mode, src_constant>(istate);
    WriteWordOperand<dst_mode, dst_constant>(istate, value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<src_mode, src_constant>(istate);
    WriteDWordOperand<dst_mode, dst_constant>(istate, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_MOVZX(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  if (actual_size == OperandSize_16)
  {
    uint16 value = ReadZeroExtendedWordOperand<src_mode, src_size, src_constant>(istate);
    WriteWordOperand<dst_mode, dst_constant>(istate, value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadZeroExtendedDWordOperand<src_mode, src_size, src_constant>(istate);
    WriteDWordOperand<dst_mode, dst_constant>(istate, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_MOVSX(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  if (actual_size == OperandSize_16)
  {
    uint16 value = ReadSignExtendedWordOperand<src_mode, src_size, src_constant>(istate);
    WriteWordOperand<dst_mode, dst_constant>(istate, value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadSignExtendedDWordOperand<src_mode, src_size, src_constant>(istate);
    WriteDWordOperand<dst_mode, dst_constant>(istate, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_MOV_Sreg(InstructionState* istate)
{
  static_assert(dst_size == OperandSize_16 && src_size == OperandSize_16, "Segment registers are 16-bits");
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  uint8 segreg = istate->GetModRM_Reg();
  if (segreg >= Segment_Count)
    RaiseInvalidOpcode();

  if constexpr (dst_mode == FIOperandMode_ModRM_SegmentReg)
  {
    // Loading segment register
    uint16 value = ReadWordOperand<src_mode, src_constant>(istate);
    m_cpu->LoadSegmentRegister(static_cast<CPU_X86::Segment>(segreg), value);
  }
  else
  {
    // Storing segment register - these are zero-extended when the operand size is 32-bit and the destination is a
    // register.
    uint16 value = m_cpu->m_registers.segment_selectors[segreg];
    if constexpr (dst_mode == FIOperandMode_ModRM_RM)
    {
      if (istate->operand_size == OperandSize_32 && istate->modrm_rm_register)
        WriteDWordOperand<dst_mode, dst_constant>(istate, ZeroExtend32(value));
      else
        WriteWordOperand<dst_mode, dst_constant>(istate, value);
    }
    else
    {
      WriteWordOperand<dst_mode, dst_constant>(istate, value);
    }
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_XCHG(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  // In memory version, memory is op0, register is op1. Memory must be written first.
  if (actual_size == OperandSize_8)
  {
    uint8 value0 = ReadByteOperand<dst_mode, dst_constant>(istate);
    uint8 value1 = ReadByteOperand<src_mode, src_constant>(istate);

    WriteByteOperand<dst_mode, dst_constant>(istate, value1);
    WriteByteOperand<src_mode, src_constant>(istate, value0);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value0 = ReadWordOperand<dst_mode, dst_constant>(istate);
    uint16 value1 = ReadWordOperand<src_mode, src_constant>(istate);

    WriteWordOperand<dst_mode, dst_constant>(istate, value1);
    WriteWordOperand<src_mode, src_constant>(istate, value0);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value0 = ReadDWordOperand<dst_mode, dst_constant>(istate);
    uint32 value1 = ReadDWordOperand<src_mode, src_constant>(istate);

    WriteDWordOperand<dst_mode, dst_constant>(istate, value1);
    WriteDWordOperand<src_mode, src_constant>(istate, value0);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
         OperandSize count_size, uint32 count_constant>
void CPU_X86::FastInterpreterBackend::Execute_SHL(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);
  FetchImmediate<count_mode, count_size>(istate);
  CalculateEffectiveAddress<count_mode>(istate);

  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(istate);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (shift_amount == 0)
      return;

    uint16 shifted_value = ZeroExtend16(value) << shift_amount;
    uint8 new_value = Truncate8(shifted_value);
    WriteByteOperand<val_mode, val_constant>(istate, new_value);

    SET_FLAG(&m_cpu->m_registers, CF, ((shifted_value & 0x100) != 0));
    SET_FLAG(&m_cpu->m_registers, OF,
             (shift_amount == 1 && (((shifted_value >> 7) & 1) ^ ((shifted_value >> 8) & 1)) != 0));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(new_value));
    SET_FLAG(&m_cpu->m_registers, AF, false);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(istate);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (shift_amount == 0)
      return;

    uint32 shifted_value = ZeroExtend32(value) << shift_amount;
    uint16 new_value = Truncate16(shifted_value);
    WriteWordOperand<val_mode, val_constant>(istate, new_value);

    SET_FLAG(&m_cpu->m_registers, CF, ((shifted_value & 0x10000) != 0));
    SET_FLAG(&m_cpu->m_registers, OF,
             (shift_amount == 1 && (((shifted_value >> 15) & 1) ^ ((shifted_value >> 16) & 1)) != 0));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(new_value));
    SET_FLAG(&m_cpu->m_registers, AF, false);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(istate);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (shift_amount == 0)
      return;

    uint64 shifted_value = ZeroExtend64(value) << shift_amount;
    uint32 new_value = Truncate32(shifted_value);
    WriteDWordOperand<val_mode, val_constant>(istate, new_value);

    SET_FLAG(&m_cpu->m_registers, CF, ((shifted_value & UINT64_C(0x100000000)) != 0));
    SET_FLAG(&m_cpu->m_registers, OF,
             (shift_amount == 1 && (((shifted_value >> 31) & 1) ^ ((shifted_value >> 32) & 1)) != 0));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(new_value));
    SET_FLAG(&m_cpu->m_registers, AF, false);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
         OperandSize count_size, uint32 count_constant>
void CPU_X86::FastInterpreterBackend::Execute_SHR(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);
  FetchImmediate<count_mode, count_size>(istate);
  CalculateEffectiveAddress<count_mode>(istate);

  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(istate);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (shift_amount == 0)
      return;

    uint8 new_value = value >> shift_amount;
    WriteByteOperand<val_mode, val_constant>(istate, new_value);

    SET_FLAG(&m_cpu->m_registers, CF, ((shift_amount ? (value >> (shift_amount - 1) & 1) : (value & 1)) != 0));
    SET_FLAG(&m_cpu->m_registers, OF, (shift_amount == 1 && (value & 0x80) != 0));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(new_value));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(istate);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (shift_amount == 0)
      return;

    uint16 new_value = value >> shift_amount;
    WriteWordOperand<val_mode, val_constant>(istate, new_value);

    SET_FLAG(&m_cpu->m_registers, CF, ((shift_amount ? (value >> (shift_amount - 1) & 1) : (value & 1)) != 0));
    SET_FLAG(&m_cpu->m_registers, OF, (shift_amount == 1 && (value & 0x8000) != 0));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(new_value));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(istate);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (shift_amount == 0)
      return;

    uint32 new_value = value >> shift_amount;
    WriteDWordOperand<val_mode, val_constant>(istate, new_value);

    SET_FLAG(&m_cpu->m_registers, CF, ((shift_amount ? (value >> (shift_amount - 1) & 1) : (value & 1)) != 0));
    SET_FLAG(&m_cpu->m_registers, OF, (shift_amount == 1 && (value & 0x80000000) != 0));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(new_value));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
         OperandSize count_size, uint32 count_constant>
void CPU_X86::FastInterpreterBackend::Execute_SAR(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);
  FetchImmediate<count_mode, count_size>(istate);
  CalculateEffectiveAddress<count_mode>(istate);

  // Shift amounts will always be uint8
  // The 8086 does not mask the shift count. However, all other IA-32 processors
  // (starting with the Intel 286 processor) do mask the shift count to 5 bits,
  // resulting in a maximum count of 31.
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(istate);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (shift_amount == 0)
      return;

    uint8 new_value = uint8(int8(value) >> shift_amount);
    WriteByteOperand<val_mode, val_constant>(istate, new_value);

    SET_FLAG(&m_cpu->m_registers, CF, ((int8(value) >> (shift_amount - 1) & 1) != 0));
    SET_FLAG(&m_cpu->m_registers, OF, false);
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(new_value));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(istate);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (shift_amount == 0)
      return;

    uint16 new_value = uint16(int16(value) >> shift_amount);
    WriteWordOperand<val_mode, val_constant>(istate, new_value);

    SET_FLAG(&m_cpu->m_registers, CF, ((int16(value) >> (shift_amount - 1) & 1) != 0));
    SET_FLAG(&m_cpu->m_registers, OF, false);
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(new_value));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(istate);
    uint8 shift_amount = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (shift_amount == 0)
      return;

    uint32 new_value = uint32(int32(value) >> shift_amount);
    WriteDWordOperand<val_mode, val_constant>(istate, new_value);

    SET_FLAG(&m_cpu->m_registers, CF, ((int32(value) >> (shift_amount - 1) & 1) != 0));
    SET_FLAG(&m_cpu->m_registers, OF, false);
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(new_value));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(new_value));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(new_value));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
         OperandSize count_size, uint32 count_constant>
void CPU_X86::FastInterpreterBackend::Execute_RCL(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);
  FetchImmediate<count_mode, count_size>(istate);
  CalculateEffectiveAddress<count_mode>(istate);

  // The processor restricts the count to a number between 0 and 31 by masking all the bits in the count operand except
  // the 5 least-significant bits.
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(istate);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (rotate_count == 0)
      return;

    uint8 carry = (m_cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint8 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 7);
    }
    WriteByteOperand<val_mode, val_constant>(istate, value);

    SET_FLAG(&m_cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&m_cpu->m_registers, OF, (((value >> 7) ^ carry) != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(istate);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (rotate_count == 0)
      return;

    uint16 carry = (m_cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint16 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 15);
    }
    WriteWordOperand<val_mode, val_constant>(istate, value);

    SET_FLAG(&m_cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&m_cpu->m_registers, OF, (((value >> 15) ^ carry) != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(istate);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (rotate_count == 0)
      return;

    uint32 carry = (m_cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint32 save_value = value;
      value = (save_value << 1) | carry;
      carry = (save_value >> 31);
    }
    WriteDWordOperand<val_mode, val_constant>(istate, value);

    SET_FLAG(&m_cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&m_cpu->m_registers, OF, (((value >> 31) ^ carry) != 0));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
         OperandSize count_size, uint32 count_constant>
void CPU_X86::FastInterpreterBackend::Execute_RCR(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);
  FetchImmediate<count_mode, count_size>(istate);
  CalculateEffectiveAddress<count_mode>(istate);

  // The processor restricts the count to a number between 0 and 31 by masking all the bits in the count operand except
  // the 5 least-significant bits.
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(istate);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (rotate_count == 0)
      return;

    uint8 carry = (m_cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint8 save_value = value;
      value = (save_value >> 1) | (carry << 7);
      carry = (save_value & 1);
    }
    WriteByteOperand<val_mode, val_constant>(istate, value);

    SET_FLAG(&m_cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&m_cpu->m_registers, OF, (((value >> 7) ^ ((value >> 6) & 1)) != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(istate);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (rotate_count == 0)
      return;

    uint16 carry = (m_cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint16 save_value = value;
      value = (save_value >> 1) | (carry << 15);
      carry = (save_value & 1);
    }
    WriteWordOperand<val_mode, val_constant>(istate, value);

    SET_FLAG(&m_cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&m_cpu->m_registers, OF, (((value >> 15) ^ ((value >> 14) & 1)) != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(istate);
    uint8 rotate_count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (rotate_count == 0)
      return;

    uint32 carry = (m_cpu->m_registers.EFLAGS.CF) ? 1 : 0;
    for (uint8 i = 0; i < rotate_count; i++)
    {
      uint32 save_value = value;
      value = (save_value >> 1) | (carry << 31);
      carry = (save_value & 1);
    }
    WriteDWordOperand<val_mode, val_constant>(istate, value);

    SET_FLAG(&m_cpu->m_registers, CF, (carry != 0));
    SET_FLAG(&m_cpu->m_registers, OF, (((value >> 31) ^ ((value >> 30) & 1)) != 0));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
         OperandSize count_size, uint32 count_constant>
void CPU_X86::FastInterpreterBackend::Execute_ROL(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);
  FetchImmediate<count_mode, count_size>(istate);
  CalculateEffectiveAddress<count_mode>(istate);

  // Hopefully this will compile down to a native ROL instruction
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(istate);
    uint8 count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (count == 0)
      return;

    uint8 new_value = value;
    if ((count & 0x7) != 0)
    {
      uint8 masked_count = count & 0x7;
      new_value = (value << masked_count) | (value >> (8 - masked_count));
      WriteByteOperand<val_mode, val_constant>(istate, new_value);
    }

    uint8 b0 = (new_value & 1);
    uint8 b7 = (new_value >> 7);
    SET_FLAG(&m_cpu->m_registers, CF, (b0 != 0));
    SET_FLAG(&m_cpu->m_registers, OF, ((b0 ^ b7) != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(istate);
    uint8 count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (count == 0)
      return;

    uint16 new_value = value;
    if ((count & 0xf) != 0)
    {
      uint8 masked_count = count & 0xf;
      new_value = (value << masked_count) | (value >> (16 - masked_count));
      WriteWordOperand<val_mode, val_constant>(istate, new_value);
    }

    uint16 b0 = (new_value & 1);
    uint16 b15 = (new_value >> 15);
    SET_FLAG(&m_cpu->m_registers, CF, (b0 != 0));
    SET_FLAG(&m_cpu->m_registers, OF, ((b0 ^ b15) != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(istate);
    uint8 count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (count == 0)
      return;

    uint32 new_value = value;
    uint8 masked_count = count & 0x1f;
    if (masked_count != 0)
    {
      new_value = (value << masked_count) | (value >> (32 - masked_count));
      WriteDWordOperand<val_mode, val_constant>(istate, new_value);
    }

    uint32 b0 = (new_value & 1);
    uint32 b31 = ((new_value >> 31) & 1);
    SET_FLAG(&m_cpu->m_registers, CF, (b0 != 0));
    SET_FLAG(&m_cpu->m_registers, OF, ((b0 ^ b31) != 0));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant, FIOperandMode count_mode,
         OperandSize count_size, uint32 count_constant>
void CPU_X86::FastInterpreterBackend::Execute_ROR(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);
  FetchImmediate<count_mode, count_size>(istate);
  CalculateEffectiveAddress<count_mode>(istate);

  // Hopefully this will compile down to a native ROR instruction
  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(istate);
    uint8 count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (count == 0)
      return;

    uint8 new_value = value;
    uint8 masked_count = count & 0x7;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (8 - masked_count));
      WriteByteOperand<val_mode, val_constant>(istate, new_value);
    }

    uint16 b6 = ((new_value >> 6) & 1);
    uint16 b7 = ((new_value >> 7) & 1);
    SET_FLAG(&m_cpu->m_registers, CF, (b7 != 0));
    SET_FLAG(&m_cpu->m_registers, OF, ((b6 ^ b7) != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(istate);
    uint8 count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (count == 0)
      return;

    uint16 new_value = value;
    uint8 masked_count = count & 0xf;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (16 - masked_count));
      WriteWordOperand<val_mode, val_constant>(istate, new_value);
    }

    uint16 b14 = ((new_value >> 14) & 1);
    uint16 b15 = ((new_value >> 15) & 1);
    SET_FLAG(&m_cpu->m_registers, CF, (b15 != 0));
    SET_FLAG(&m_cpu->m_registers, OF, ((b14 ^ b15) != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(istate);
    uint8 count = ReadByteOperand<count_mode, count_constant>(istate) & 0x1F;
    if (count == 0)
      return;

    uint32 new_value = value;
    uint8 masked_count = count & 0x1f;
    if (masked_count != 0)
    {
      new_value = (value >> masked_count) | (value << (32 - masked_count));
      WriteDWordOperand<val_mode, val_constant>(istate, new_value);
    }

    uint32 b30 = ((new_value >> 30) & 1);
    uint32 b31 = ((new_value >> 31) & 1);
    SET_FLAG(&m_cpu->m_registers, CF, (b31 != 0));
    SET_FLAG(&m_cpu->m_registers, OF, ((b30 ^ b31) != 0));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_IN(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  uint16 port_number = ReadZeroExtendedWordOperand<src_mode, src_size, src_constant>(istate);

  if (actual_size == OperandSize_8)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint8), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint8 value;
    m_bus->ReadIOPortByte(port_number, &value);
    WriteByteOperand<dst_mode, dst_constant>(istate, value);
  }
  else if (actual_size == OperandSize_16)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint16), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint16 value;
    m_bus->ReadIOPortWord(port_number, &value);
    WriteWordOperand<dst_mode, dst_constant>(istate, value);
  }
  else if (actual_size == OperandSize_32)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint32), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint32 value;
    m_bus->ReadIOPortDWord(port_number, &value);
    WriteDWordOperand<dst_mode, dst_constant>(istate, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_OUT(InstructionState* istate)
{
  OperandSize actual_size = (src_size == OperandSize_Count) ? istate->operand_size : src_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  uint16 port_number = ReadZeroExtendedWordOperand<dst_mode, dst_size, dst_constant>(istate);

  if (actual_size == OperandSize_8)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint8), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint8 value = ReadByteOperand<src_mode, src_constant>(istate);
    m_bus->WriteIOPortByte(port_number, value);
  }
  else if (actual_size == OperandSize_16)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint16), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint16 value = ReadWordOperand<src_mode, src_constant>(istate);
    m_bus->WriteIOPortWord(port_number, value);
  }
  else if (actual_size == OperandSize_32)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint32), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint32 value = ReadDWordOperand<src_mode, src_constant>(istate);
    m_bus->WriteIOPortDWord(port_number, value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
void CPU_X86::FastInterpreterBackend::Execute_INC(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);

  // Preserve CF
  bool cf = m_cpu->m_registers.EFLAGS.CF;

  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(istate);
    uint8 new_value = ALUOp_Add8(&m_cpu->m_registers, value, 1);
    WriteByteOperand<val_mode, val_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(istate);
    uint16 new_value = ALUOp_Add16(&m_cpu->m_registers, value, 1);
    WriteWordOperand<val_mode, val_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(istate);
    uint32 new_value = ALUOp_Add32(&m_cpu->m_registers, value, 1);
    WriteDWordOperand<val_mode, val_constant>(istate, new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  SET_FLAG(&m_cpu->m_registers, CF, cf);
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
void CPU_X86::FastInterpreterBackend::Execute_DEC(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);

  // Preserve CF
  bool cf = m_cpu->m_registers.EFLAGS.CF;

  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(istate);
    uint8 new_value = ALUOp_Sub8(&m_cpu->m_registers, value, 1);
    WriteByteOperand<val_mode, val_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(istate);
    uint16 new_value = ALUOp_Sub16(&m_cpu->m_registers, value, 1);
    WriteWordOperand<val_mode, val_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(istate);
    uint32 new_value = ALUOp_Sub32(&m_cpu->m_registers, value, 1);
    WriteDWordOperand<val_mode, val_constant>(istate, new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  SET_FLAG(&m_cpu->m_registers, CF, cf);
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
void CPU_X86::FastInterpreterBackend::Execute_NOT(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);

  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(istate);
    uint8 new_value = ~value;
    WriteByteOperand<val_mode, val_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(istate);
    uint16 new_value = ~value;
    WriteWordOperand<val_mode, val_constant>(istate, new_value);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(istate);
    uint32 new_value = ~value;
    WriteDWordOperand<val_mode, val_constant>(istate, new_value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
void CPU_X86::FastInterpreterBackend::Execute_NEG(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);

  if (actual_size == OperandSize_8)
  {
    uint8 value = ReadByteOperand<val_mode, val_constant>(istate);
    uint8 new_value = uint8(-int8(value));
    WriteByteOperand<val_mode, val_constant>(istate, new_value);

    ALUOp_Sub8(&m_cpu->m_registers, 0, value);
    SET_FLAG(&m_cpu->m_registers, CF, (new_value != 0));
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = ReadWordOperand<val_mode, val_constant>(istate);
    uint16 new_value = uint16(-int16(value));
    WriteWordOperand<val_mode, val_constant>(istate, new_value);

    ALUOp_Sub16(&m_cpu->m_registers, 0, value);
    SET_FLAG(&m_cpu->m_registers, CF, (new_value != 0));
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = ReadDWordOperand<val_mode, val_constant>(istate);
    uint32 new_value = uint32(-int32(value));
    WriteDWordOperand<val_mode, val_constant>(istate, new_value);

    ALUOp_Sub32(&m_cpu->m_registers, 0, value);
    SET_FLAG(&m_cpu->m_registers, CF, (new_value != 0));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
void CPU_X86::FastInterpreterBackend::Execute_MUL(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);

  // The OF and CF flags are set to 0 if the upper half of the result is 0;
  // otherwise, they are set to 1. The SF, ZF, AF, and PF flags are undefined.
  if (actual_size == OperandSize_8)
  {
    uint16 lhs = uint16(m_cpu->m_registers.AL);
    uint16 rhs = uint16(ReadByteOperand<val_mode, val_constant>(istate));
    uint16 result = lhs * rhs;
    m_cpu->m_registers.AX = result;
    SET_FLAG(&m_cpu->m_registers, OF, (m_cpu->m_registers.AH != 0));
    SET_FLAG(&m_cpu->m_registers, CF, (m_cpu->m_registers.AH != 0));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(m_cpu->m_registers.AL));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(m_cpu->m_registers.AL));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(m_cpu->m_registers.AL));
  }
  else if (actual_size == OperandSize_16)
  {
    uint32 lhs = uint32(m_cpu->m_registers.AX);
    uint32 rhs = uint32(ReadSignExtendedWordOperand<val_mode, val_size, val_constant>(istate));
    uint32 result = lhs * rhs;
    m_cpu->m_registers.AX = uint16(result & 0xFFFF);
    m_cpu->m_registers.DX = uint16(result >> 16);
    SET_FLAG(&m_cpu->m_registers, OF, (m_cpu->m_registers.DX != 0));
    SET_FLAG(&m_cpu->m_registers, CF, (m_cpu->m_registers.DX != 0));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(m_cpu->m_registers.AX));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(m_cpu->m_registers.AX));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(m_cpu->m_registers.AX));
  }
  else if (actual_size == OperandSize_32)
  {
    uint64 lhs = ZeroExtend64(m_cpu->m_registers.EAX);
    uint64 rhs = ZeroExtend64(ReadSignExtendedDWordOperand<val_mode, val_size, val_constant>(istate));
    uint64 result = lhs * rhs;
    m_cpu->m_registers.EAX = Truncate32(result);
    m_cpu->m_registers.EDX = Truncate32(result >> 32);
    SET_FLAG(&m_cpu->m_registers, OF, (m_cpu->m_registers.EDX != 0));
    SET_FLAG(&m_cpu->m_registers, CF, (m_cpu->m_registers.EDX != 0));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(m_cpu->m_registers.EAX));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(m_cpu->m_registers.EAX));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(m_cpu->m_registers.EAX));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
void CPU_X86::FastInterpreterBackend::Execute_IMUL1(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);

  if (actual_size == OperandSize_8)
  {
    int16 lhs = int8(m_cpu->m_registers.AL);
    int16 rhs = int8(ReadByteOperand<val_mode, val_constant>(istate));
    int16 result = lhs * rhs;
    uint8 truncated_result = uint8(uint16(result) & 0xFFFF);

    m_cpu->m_registers.AX = uint16(result);

    SET_FLAG(&m_cpu->m_registers, OF, (int16(int8(truncated_result)) != result));
    SET_FLAG(&m_cpu->m_registers, CF, (int16(int8(truncated_result)) != result));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(truncated_result));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(truncated_result));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(truncated_result));
  }
  else if (actual_size == OperandSize_16)
  {
    int32 lhs, rhs;
    int32 result;
    uint16 truncated_result;

    lhs = int16(m_cpu->m_registers.AX);
    rhs = int16(ReadSignExtendedWordOperand<val_mode, val_size, val_constant>(istate));
    result = lhs * rhs;
    truncated_result = uint16(uint32(result) & 0xFFFF);

    m_cpu->m_registers.DX = uint16((uint32(result) >> 16) & 0xFFFF);
    m_cpu->m_registers.AX = truncated_result;

    SET_FLAG(&m_cpu->m_registers, OF, (int32(int16(truncated_result)) != result));
    SET_FLAG(&m_cpu->m_registers, CF, (int32(int16(truncated_result)) != result));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(truncated_result));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(truncated_result));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(truncated_result));
  }
  else if (actual_size == OperandSize_32)
  {
    int64 lhs, rhs;
    int64 result;
    uint32 truncated_result;

    // One-operand form
    lhs = int32(m_cpu->m_registers.EAX);
    rhs = int32(ReadSignExtendedDWordOperand<val_mode, val_size, val_constant>(istate));
    result = lhs * rhs;
    truncated_result = Truncate32(result);

    m_cpu->m_registers.EDX = Truncate32(uint64(result) >> 32);
    m_cpu->m_registers.EAX = truncated_result;

    SET_FLAG(&m_cpu->m_registers, OF, (int64(SignExtend64(truncated_result)) != result));
    SET_FLAG(&m_cpu->m_registers, CF, (int64(SignExtend64(truncated_result)) != result));
    SET_FLAG(&m_cpu->m_registers, SF, IsSign(truncated_result));
    SET_FLAG(&m_cpu->m_registers, ZF, IsZero(truncated_result));
    SET_FLAG(&m_cpu->m_registers, PF, IsParity(truncated_result));
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
void CPU_X86::FastInterpreterBackend::Execute_DIV(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);

  if (actual_size == OperandSize_8)
  {
    // Eight-bit divides use AX as a source
    uint8 divisor = ReadByteOperand<val_mode, val_constant>(istate);
    if (divisor == 0)
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    uint16 source = m_cpu->m_registers.AX;
    uint16 quotient = source / divisor;
    uint16 remainder = source % divisor;
    if (quotient > 0xFF)
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    m_cpu->m_registers.AL = uint8(quotient);
    m_cpu->m_registers.AH = uint8(remainder);
  }
  else if (actual_size == OperandSize_16)
  {
    // 16-bit divides use DX:AX as a source
    uint16 divisor = ReadSignExtendedWordOperand<val_mode, val_size, val_constant>(istate);
    if (divisor == 0)
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    uint32 source = (uint32(m_cpu->m_registers.DX) << 16) | uint32(m_cpu->m_registers.AX);
    uint32 quotient = source / divisor;
    uint32 remainder = source % divisor;
    if (quotient > 0xFFFF)
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    m_cpu->m_registers.AX = uint16(quotient);
    m_cpu->m_registers.DX = uint16(remainder);
  }
  else if (actual_size == OperandSize_32)
  {
    // 32-bit divides use EDX:EAX as a source
    uint32 divisor = ReadSignExtendedDWordOperand<val_mode, val_size, val_constant>(istate);
    if (divisor == 0)
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    uint64 source = (ZeroExtend64(m_cpu->m_registers.EDX) << 32) | ZeroExtend64(m_cpu->m_registers.EAX);
    uint64 quotient = source / divisor;
    uint64 remainder = source % divisor;
    if (quotient > UINT64_C(0xFFFFFFFF))
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    m_cpu->m_registers.EAX = Truncate32(quotient);
    m_cpu->m_registers.EDX = Truncate32(remainder);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode val_mode, OperandSize val_size, uint32 val_constant>
void CPU_X86::FastInterpreterBackend::Execute_IDIV(InstructionState* istate)
{
  OperandSize actual_size = (val_size == OperandSize_Count) ? istate->operand_size : val_size;
  FetchImmediate<val_mode, val_size>(istate);
  CalculateEffectiveAddress<val_mode>(istate);

  if (actual_size == OperandSize_8)
  {
    // Eight-bit divides use AX as a source
    int8 divisor = int8(ReadByteOperand<val_mode, val_constant>(istate));
    if (divisor == 0)
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    int16 source = int16(m_cpu->m_registers.AX);
    int16 quotient = source / divisor;
    int16 remainder = source % divisor;
    uint8 truncated_quotient = uint8(uint16(quotient) & 0xFFFF);
    uint8 truncated_remainder = uint8(uint16(remainder) & 0xFFFF);
    if (int8(truncated_quotient) != quotient)
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    m_cpu->m_registers.AL = truncated_quotient;
    m_cpu->m_registers.AH = truncated_remainder;
  }
  else if (actual_size == OperandSize_16)
  {
    // 16-bit divides use DX:AX as a source
    int16 divisor = int16(ReadSignExtendedWordOperand<val_mode, val_size, val_constant>(istate));
    if (divisor == 0)
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    int32 source = int32((uint32(m_cpu->m_registers.DX) << 16) | uint32(m_cpu->m_registers.AX));
    int32 quotient = source / divisor;
    int32 remainder = source % divisor;
    uint16 truncated_quotient = uint16(uint32(quotient) & 0xFFFF);
    uint16 truncated_remainder = uint16(uint32(remainder) & 0xFFFF);
    if (int16(truncated_quotient) != quotient)
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    m_cpu->m_registers.AX = truncated_quotient;
    m_cpu->m_registers.DX = truncated_remainder;
  }
  else if (actual_size == OperandSize_32)
  {
    // 16-bit divides use DX:AX as a source
    int32 divisor = int32(ReadSignExtendedDWordOperand<val_mode, val_size, val_constant>(istate));
    if (divisor == 0)
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    int64 source = int64((ZeroExtend64(m_cpu->m_registers.EDX) << 32) | ZeroExtend64(m_cpu->m_registers.EAX));
    int64 quotient = source / divisor;
    int64 remainder = source % divisor;
    uint32 truncated_quotient = Truncate32(uint64(quotient));
    uint32 truncated_remainder = Truncate32(uint64(remainder));
    if (int32(truncated_quotient) != quotient)
    {
      m_cpu->RaiseException(Interrupt_DivideError);
      return;
    }

    m_cpu->m_registers.EAX = truncated_quotient;
    m_cpu->m_registers.EDX = truncated_remainder;
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode src_mode, OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_PUSH(InstructionState* istate)
{
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  if (istate->operand_size == OperandSize_16)
  {
    uint16 value = ReadSignExtendedWordOperand<src_mode, src_size, src_constant>(istate);
    m_cpu->PushWord(value);
  }
  else if (istate->operand_size == OperandSize_32)
  {
    uint32 value = ReadSignExtendedDWordOperand<src_mode, src_size, src_constant>(istate);
    m_cpu->PushDWord(value);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
void CPU_X86::FastInterpreterBackend::Execute_POP(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  static_assert(dst_size == OperandSize_Count || dst_size == OperandSize_16 || dst_size == OperandSize_32,
                "dst_size is not 8-bits");

  uint32 value;
  if (actual_size == OperandSize_16)
    value = ZeroExtend32(m_cpu->PopWord());
  else // if (actual_size == OperandSize_32)
    value = m_cpu->PopDWord();

  // POP can use ESP in the address calculations, in this case the value of ESP
  // is that after the pop operation has occurred, not before.
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);

  if (actual_size == OperandSize_16)
    WriteWordOperand<dst_mode, dst_constant>(istate, Truncate16(value));
  else
    WriteDWordOperand<dst_mode, dst_constant>(istate, value);
}

template<FIOperandMode src_mode, OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_PUSH_Sreg(InstructionState* istate)
{
  static_assert(src_mode == FIOperandMode_SegmentRegister, "Source is segment register");
  static_assert(src_constant <= Segment_Count, "Segment register is valid");
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  // TODO: Is this correct? bochs has it incrementing 4 bytes but only writing 2 of them.
  uint16 value = m_cpu->m_registers.segment_selectors[src_constant];
  if (istate->operand_size == OperandSize_16)
    m_cpu->PushWord(value);
  else
    m_cpu->PushDWord(ZeroExtend32(value));
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
void CPU_X86::FastInterpreterBackend::Execute_POP_Sreg(InstructionState* istate)
{
  static_assert(dst_mode == FIOperandMode_SegmentRegister, "Destination is segment register");
  static_assert(dst_constant <= Segment_Count, "Segment register is valid");
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);

  // TODO: Is this correct? bochs has it incrementing 4 bytes but only writing 2 of them.
  uint16 value;
  if (istate->operand_size == OperandSize_16)
    value = m_cpu->PopWord();
  else
    value = Truncate16(m_cpu->PopDWord());

  m_cpu->LoadSegmentRegister(static_cast<Segment>(dst_constant), value);
}

void FastInterpreterBackend::Execute_PUSHA(InstructionState* istate)
{
  if (istate->operand_size == OperandSize_16)
  {
    uint16 old_SP = m_cpu->m_registers.SP;
    m_cpu->PushWord(m_cpu->m_registers.AX);
    m_cpu->PushWord(m_cpu->m_registers.CX);
    m_cpu->PushWord(m_cpu->m_registers.DX);
    m_cpu->PushWord(m_cpu->m_registers.BX);
    m_cpu->PushWord(old_SP);
    m_cpu->PushWord(m_cpu->m_registers.BP);
    m_cpu->PushWord(m_cpu->m_registers.SI);
    m_cpu->PushWord(m_cpu->m_registers.DI);
  }
  else if (istate->operand_size == OperandSize_32)
  {
    uint32 old_ESP = m_cpu->m_registers.ESP;
    m_cpu->PushDWord(m_cpu->m_registers.EAX);
    m_cpu->PushDWord(m_cpu->m_registers.ECX);
    m_cpu->PushDWord(m_cpu->m_registers.EDX);
    m_cpu->PushDWord(m_cpu->m_registers.EBX);
    m_cpu->PushDWord(old_ESP);
    m_cpu->PushDWord(m_cpu->m_registers.EBP);
    m_cpu->PushDWord(m_cpu->m_registers.ESI);
    m_cpu->PushDWord(m_cpu->m_registers.EDI);
  }
  else
  {
    DebugUnreachableCode();
  }
}

void FastInterpreterBackend::Execute_POPA(InstructionState* istate)
{
  // Assignment split from reading in case of exception.
  if (istate->operand_size == OperandSize_16)
  {
    uint16 DI = m_cpu->PopWord();
    uint16 SI = m_cpu->PopWord();
    uint16 BP = m_cpu->PopWord();
    /*uint16 SP = */ m_cpu->PopWord();
    uint16 BX = m_cpu->PopWord();
    uint16 DX = m_cpu->PopWord();
    uint16 CX = m_cpu->PopWord();
    uint16 AX = m_cpu->PopWord();
    m_cpu->m_registers.DI = DI;
    m_cpu->m_registers.SI = SI;
    m_cpu->m_registers.BP = BP;
    m_cpu->m_registers.BX = BX;
    m_cpu->m_registers.DX = DX;
    m_cpu->m_registers.CX = CX;
    m_cpu->m_registers.AX = AX;
  }
  else if (istate->operand_size == OperandSize_32)
  {
    uint32 EDI = m_cpu->PopDWord();
    uint32 ESI = m_cpu->PopDWord();
    uint32 EBP = m_cpu->PopDWord();
    /*uint32 ESP = */ m_cpu->PopDWord();
    uint32 EBX = m_cpu->PopDWord();
    uint32 EDX = m_cpu->PopDWord();
    uint32 ECX = m_cpu->PopDWord();
    uint32 EAX = m_cpu->PopDWord();
    m_cpu->m_registers.EDI = EDI;
    m_cpu->m_registers.ESI = ESI;
    m_cpu->m_registers.EBP = EBP;
    m_cpu->m_registers.EBX = EBX;
    m_cpu->m_registers.EDX = EDX;
    m_cpu->m_registers.ECX = ECX;
    m_cpu->m_registers.EAX = EAX;
  }
  else
  {
    DebugUnreachableCode();
  }
}

template<FIOperandMode frame_mode, OperandSize frame_size, uint32 frame_constant, FIOperandMode level_mode,
         OperandSize level_size, uint32 level_constant>
void CPU_X86::FastInterpreterBackend::Execute_ENTER(InstructionState* istate)
{
  // Because these are both immediates, we need to read immediately after fetch (as we only store one immediate).
  FetchImmediate<frame_mode, frame_size>(istate);
  uint16 stack_frame_size = ReadWordOperand<frame_mode, frame_constant>(istate);

  FetchImmediate<level_mode, level_size>(istate);
  uint8 level = ReadByteOperand<level_mode, level_constant>(istate);

  // Push current frame pointer.
  if (istate->operand_size == OperandSize_16)
    m_cpu->PushWord(m_cpu->m_registers.BP);
  else
    m_cpu->PushDWord(m_cpu->m_registers.EBP);

  uint32 frame_pointer = m_cpu->m_registers.ESP;
  if (level > 0)
  {
    // Use our own local copy of EBP in case any of these fail.
    if (istate->operand_size == OperandSize_16)
    {
      uint16 BP = m_cpu->m_registers.BP;
      for (uint8 i = 1; i < level; i++)
      {
        BP -= sizeof(uint16);

        uint16 prev_ptr = m_cpu->ReadMemoryWord(Segment_SS, BP);
        m_cpu->PushWord(prev_ptr);
      }
      m_cpu->PushDWord(frame_pointer);
      m_cpu->m_registers.BP = BP;
    }
    else
    {
      uint32 EBP = m_cpu->m_registers.EBP;
      for (uint8 i = 1; i < level; i++)
      {
        EBP -= sizeof(uint32);

        uint32 prev_ptr = m_cpu->ReadMemoryDWord(Segment_SS, EBP);
        m_cpu->PushDWord(prev_ptr);
      }
      m_cpu->PushDWord(frame_pointer);
      m_cpu->m_registers.EBP = EBP;
    }
  }

  if (istate->operand_size == OperandSize_16)
    m_cpu->m_registers.BP = Truncate16(frame_pointer);
  else
    m_cpu->m_registers.EBP = frame_pointer;

  if (m_cpu->m_stack_address_size == AddressSize_16)
    m_cpu->m_registers.SP -= stack_frame_size;
  else
    m_cpu->m_registers.ESP -= stack_frame_size;
}

void FastInterpreterBackend::Execute_LEAVE(InstructionState* istate)
{
  if (m_cpu->m_stack_address_size == AddressSize_16)
    m_cpu->m_registers.SP = m_cpu->m_registers.BP;
  else
    m_cpu->m_registers.ESP = m_cpu->m_registers.EBP;

  if (istate->operand_size == OperandSize_16)
    m_cpu->m_registers.BP = m_cpu->PopWord();
  else if (istate->operand_size == OperandSize_32)
    m_cpu->m_registers.EBP = m_cpu->PopDWord();
  else
    DebugUnreachableCode();
}

template<FIOperandMode sreg_mode, OperandSize sreg_size, uint32 sreg_constant, FIOperandMode reg_mode,
         OperandSize reg_size, uint32 reg_constant, FIOperandMode ptr_mode, OperandSize ptr_size, uint32 ptr_constant>
void CPU_X86::FastInterpreterBackend::Execute_LXS(InstructionState* istate)
{
  static_assert(sreg_mode == FIOperandMode_SegmentRegister, "sreg_mode is Segment Register");
  static_assert(reg_mode == FIOperandMode_ModRM_Reg, "reg_mode is Register");
  static_assert(ptr_mode == FIOperandMode_ModRM_RM, "reg_mode is Pointer");
  FetchImmediate<ptr_mode, ptr_size>(istate);
  CalculateEffectiveAddress<ptr_mode>(istate);

  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand<ptr_mode>(istate, istate->operand_size, &segment_selector, &address);

  Segment sreg = static_cast<Segment>(sreg_constant);
  m_cpu->LoadSegmentRegister(sreg, segment_selector);

  if (istate->operand_size == OperandSize_16)
    WriteWordOperand<reg_mode, reg_constant>(istate, Truncate16(address));
  else if (istate->operand_size == OperandSize_32)
    WriteDWordOperand<reg_mode, reg_constant>(istate, address);
  else
    DebugUnreachableCode();
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void CPU_X86::FastInterpreterBackend::Execute_LEA(InstructionState* istate)
{
  static_assert(src_mode == FIOperandMode_ModRM_RM, "Source operand is a pointer");
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  FetchImmediate<src_mode, src_size>(istate);
  CalculateEffectiveAddress<src_mode>(istate);

  // Calculate full address in instruction's address mode, truncate/extend to operand size.
  if (istate->operand_size == OperandSize_16)
    WriteWordOperand<dst_mode, dst_constant>(istate, Truncate16(istate->effective_address));
  else
    WriteDWordOperand<dst_mode, dst_constant>(istate, istate->effective_address);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
VirtualMemoryAddress CPU_X86::FastInterpreterBackend::CalculateJumpTarget(InstructionState* istate)
{
  static_assert(dst_mode == FIOperandMode_Relative || dst_mode == FIOperandMode_ModRM_RM,
                "Operand mode is relative or indirect");

  if constexpr (dst_mode == FIOperandMode_Relative)
  {
    OperandSize displacement_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
    if (istate->operand_size == OperandSize_16)
    {
      // TODO: Should this be extended to addressing mode?
      uint16 address = Truncate16(m_cpu->m_registers.EIP);
      switch (displacement_size)
      {
        case OperandSize_8:
          address += SignExtend16(istate->immediate.value8);
          break;
        case OperandSize_16:
          address += istate->immediate.value16;
          break;
        default:
          DebugUnreachableCode();
          break;
      }

      return ZeroExtend32(address);
    }
    else
    {
      uint32 address = m_cpu->m_registers.EIP;
      switch (displacement_size)
      {
        case OperandSize_8:
          address += SignExtend32(istate->immediate.value8);
          break;
        case OperandSize_16:
          address += SignExtend32(istate->immediate.value16);
          break;
        case OperandSize_32:
          address += istate->immediate.value32;
          break;
        default:
          DebugUnreachableCode();
          break;
      }

      return address;
    }
  }
  else if constexpr (dst_mode == FIOperandMode_ModRM_RM)
  {
    OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
    switch (actual_size)
    {
      case OperandSize_16:
        return ZeroExtend32(ReadWordOperand<dst_mode, dst_constant>(istate));
      case OperandSize_32:
        return ReadDWordOperand<dst_mode, dst_constant>(istate);
      default:
        DebugUnreachableCode();
        return 0;
    }
  }
  else
  {
    DebugUnreachableCode();
    return 0;
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
void CPU_X86::FastInterpreterBackend::Execute_JMP_Near(InstructionState* istate)
{
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);

  VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_mode, dst_size, dst_constant>(istate);
  m_cpu->BranchTo(jump_address);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, JumpCondition condition>
void CPU_X86::FastInterpreterBackend::Execute_Jcc(InstructionState* istate)
{
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);
  if (!TestJumpCondition<condition>(istate))
    return;

  VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_mode, dst_size, dst_constant>(istate);
  m_cpu->BranchTo(jump_address);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, JumpCondition condition>
void CPU_X86::FastInterpreterBackend::Execute_LOOP(InstructionState* istate)
{
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);

  // This seems okay IRT promotion rules.
  uint32 count;
  if (istate->address_size == AddressSize_16)
    count = ZeroExtend32(--m_cpu->m_registers.CX);
  else
    count = ZeroExtend32(--m_cpu->m_registers.ECX);

  bool branch = (count != 0) && TestJumpCondition<condition>(istate);
  if (!branch)
    return;

  VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_mode, dst_size, dst_constant>(istate);
  m_cpu->BranchTo(jump_address);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
void CPU_X86::FastInterpreterBackend::Execute_CALL_Near(InstructionState* istate)
{
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);

  VirtualMemoryAddress jump_address = CalculateJumpTarget<dst_mode, dst_size, dst_constant>(istate);
  if (istate->operand_size == OperandSize_16)
    m_cpu->PushWord(Truncate16(m_cpu->m_registers.EIP));
  else
    m_cpu->PushDWord(m_cpu->m_registers.EIP);

  m_cpu->BranchTo(jump_address);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
void CPU_X86::FastInterpreterBackend::Execute_RET_Near(InstructionState* istate)
{
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);

  uint32 pop_count = 0;
  if constexpr (dst_mode != FIOperandMode_None)
    pop_count = ReadZeroExtendedDWordOperand<dst_mode, dst_size, dst_constant>(istate);

  uint32 return_EIP;
  if (istate->operand_size == OperandSize_16)
  {
    return_EIP = ZeroExtend32(m_cpu->PopWord());
  }
  else if (istate->operand_size == OperandSize_32)
  {
    return_EIP = m_cpu->PopDWord();
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (m_cpu->m_stack_address_size == AddressSize_16)
    m_cpu->m_registers.SP += Truncate16(pop_count);
  else
    m_cpu->m_registers.ESP += pop_count;

  m_cpu->BranchTo(return_EIP);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
void CPU_X86::FastInterpreterBackend::Execute_JMP_Far(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);

  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand<dst_mode>(istate, actual_size, &segment_selector, &address);

  m_cpu->FarJump(segment_selector, address, actual_size);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
void CPU_X86::FastInterpreterBackend::Execute_CALL_Far(InstructionState* istate)
{
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);

  uint16 segment_selector;
  VirtualMemoryAddress address;
  ReadFarAddressOperand<dst_mode>(istate, actual_size, &segment_selector, &address);

  m_cpu->FarCall(segment_selector, address, actual_size);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
void CPU_X86::FastInterpreterBackend::Execute_RET_Far(InstructionState* istate)
{
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);

  uint32 pop_count = 0;
  if constexpr (dst_mode != FIOperandMode_None)
    pop_count = ReadZeroExtendedDWordOperand<dst_mode, dst_size, dst_constant>(istate);

  m_cpu->FarReturn(istate->operand_size, pop_count);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant>
void CPU_X86::FastInterpreterBackend::Execute_INT(InstructionState* istate)
{
  FetchImmediate<dst_mode, dst_size>(istate);
  CalculateEffectiveAddress<dst_mode>(istate);

  uint32 interrupt = ReadZeroExtendedWordOperand<dst_mode, dst_size, dst_constant>(istate);
  m_cpu->SoftwareInterrupt(istate->operand_size, interrupt);
}

void FastInterpreterBackend::Execute_INTO(InstructionState* istate)
{
  // Call overflow exception if OF is set
  if (m_cpu->m_registers.EFLAGS.OF)
    m_cpu->RaiseException(Interrupt_Overflow);
}

void FastInterpreterBackend::Execute_IRET(InstructionState* istate)
{
  m_cpu->InterruptReturn(istate->operand_size);
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void FastInterpreterBackend::Execute_MOVS(InstructionState* istate)
{
  // The DS segment may be over-ridden with a segment override prefix, but the ES segment cannot be overridden.
  Segment src_segment = istate->segment;
  VirtualMemoryAddress src_address =
    (istate->address_size == AddressSize_16) ? ZeroExtend32(m_cpu->m_registers.SI) : m_cpu->m_registers.ESI;
  VirtualMemoryAddress dst_address =
    (istate->address_size == AddressSize_16) ? ZeroExtend32(m_cpu->m_registers.DI) : m_cpu->m_registers.EDI;
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  uint8 data_size;

  if (actual_size == OperandSize_8)
  {
    uint8 value = m_cpu->ReadMemoryByte(src_segment, src_address);
    m_cpu->WriteMemoryByte(Segment_ES, dst_address, value);
    data_size = sizeof(uint8);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = m_cpu->ReadMemoryWord(src_segment, src_address);
    m_cpu->WriteMemoryWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint16);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = m_cpu->ReadMemoryDWord(src_segment, src_address);
    m_cpu->WriteMemoryDWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (istate->address_size == AddressSize_16)
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
    {
      m_cpu->m_registers.SI += ZeroExtend16(data_size);
      m_cpu->m_registers.DI += ZeroExtend16(data_size);
    }
    else
    {
      m_cpu->m_registers.SI -= ZeroExtend16(data_size);
      m_cpu->m_registers.DI -= ZeroExtend16(data_size);
    }
  }
  else
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
    {
      m_cpu->m_registers.ESI += ZeroExtend32(data_size);
      m_cpu->m_registers.EDI += ZeroExtend32(data_size);
    }
    else
    {
      m_cpu->m_registers.ESI -= ZeroExtend32(data_size);
      m_cpu->m_registers.EDI -= ZeroExtend32(data_size);
    }
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void FastInterpreterBackend::Execute_CMPS(InstructionState* istate)
{
  // The DS segment may be overridden with a segment override prefix, but the ES segment cannot be overridden.
  Segment src_segment = istate->segment;
  VirtualMemoryAddress src_address =
    (istate->address_size == AddressSize_16) ? ZeroExtend32(m_cpu->m_registers.SI) : m_cpu->m_registers.ESI;
  VirtualMemoryAddress dst_address =
    (istate->address_size == AddressSize_16) ? ZeroExtend32(m_cpu->m_registers.DI) : m_cpu->m_registers.EDI;
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  uint8 data_size;

  if (actual_size == OperandSize_8)
  {
    uint8 lhs = m_cpu->ReadMemoryByte(src_segment, src_address);
    uint8 rhs = m_cpu->ReadMemoryByte(Segment_ES, dst_address);
    ALUOp_Sub8(&m_cpu->m_registers, lhs, rhs);
    data_size = sizeof(uint8);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = m_cpu->ReadMemoryWord(src_segment, src_address);
    uint16 rhs = m_cpu->ReadMemoryWord(Segment_ES, dst_address);
    ALUOp_Sub16(&m_cpu->m_registers, lhs, rhs);
    data_size = sizeof(uint16);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = m_cpu->ReadMemoryDWord(src_segment, src_address);
    uint32 rhs = m_cpu->ReadMemoryDWord(Segment_ES, dst_address);
    ALUOp_Sub32(&m_cpu->m_registers, lhs, rhs);
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (istate->address_size == AddressSize_16)
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
    {
      m_cpu->m_registers.SI += ZeroExtend16(data_size);
      m_cpu->m_registers.DI += ZeroExtend16(data_size);
    }
    else
    {
      m_cpu->m_registers.SI -= ZeroExtend16(data_size);
      m_cpu->m_registers.DI -= ZeroExtend16(data_size);
    }
  }
  else
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
    {
      m_cpu->m_registers.ESI += ZeroExtend32(data_size);
      m_cpu->m_registers.EDI += ZeroExtend32(data_size);
    }
    else
    {
      m_cpu->m_registers.ESI -= ZeroExtend32(data_size);
      m_cpu->m_registers.EDI -= ZeroExtend32(data_size);
    }
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void FastInterpreterBackend::Execute_STOS(InstructionState* istate)
{
  VirtualMemoryAddress dst_address =
    (istate->address_size == AddressSize_16) ? ZeroExtend32(m_cpu->m_registers.DI) : m_cpu->m_registers.EDI;
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  uint8 data_size;

  if (actual_size == OperandSize_8)
  {
    uint8 value = m_cpu->m_registers.AL;
    m_cpu->WriteMemoryByte(Segment_ES, dst_address, value);
    data_size = sizeof(uint8);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = m_cpu->m_registers.AX;
    m_cpu->WriteMemoryWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint16);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = m_cpu->m_registers.EAX;
    m_cpu->WriteMemoryDWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (istate->address_size == AddressSize_16)
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
      m_cpu->m_registers.DI += ZeroExtend16(data_size);
    else
      m_cpu->m_registers.DI -= ZeroExtend16(data_size);
  }
  else
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
      m_cpu->m_registers.EDI += ZeroExtend32(data_size);
    else
      m_cpu->m_registers.EDI -= ZeroExtend32(data_size);
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void FastInterpreterBackend::Execute_LODS(InstructionState* istate)
{
  Segment segment = istate->segment;
  VirtualMemoryAddress src_address =
    (istate->address_size == AddressSize_16) ? ZeroExtend32(m_cpu->m_registers.SI) : m_cpu->m_registers.ESI;
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  uint8 data_size;

  if (actual_size == OperandSize_8)
  {
    uint8 value = m_cpu->ReadMemoryByte(segment, src_address);
    m_cpu->m_registers.AL = value;
    data_size = sizeof(uint8);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 value = m_cpu->ReadMemoryWord(segment, src_address);
    m_cpu->m_registers.AX = value;
    data_size = sizeof(uint16);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 value = m_cpu->ReadMemoryDWord(segment, src_address);
    m_cpu->m_registers.EAX = value;
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (istate->address_size == AddressSize_16)
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
      m_cpu->m_registers.SI += ZeroExtend16(data_size);
    else
      m_cpu->m_registers.SI -= ZeroExtend16(data_size);
  }
  else
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
      m_cpu->m_registers.ESI += ZeroExtend32(data_size);
    else
      m_cpu->m_registers.ESI -= ZeroExtend32(data_size);
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void FastInterpreterBackend::Execute_SCAS(InstructionState* istate)
{
  // The ES segment cannot be overridden with a segment override prefix.
  VirtualMemoryAddress dst_address =
    (istate->address_size == AddressSize_16) ? ZeroExtend32(m_cpu->m_registers.DI) : m_cpu->m_registers.EDI;
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  uint8 data_size;
  if (actual_size == OperandSize_8)
  {
    uint8 lhs = m_cpu->m_registers.AL;
    uint8 rhs = m_cpu->ReadMemoryByte(Segment_ES, dst_address);
    ALUOp_Sub8(&m_cpu->m_registers, lhs, rhs);
    data_size = sizeof(uint8);
  }
  else if (actual_size == OperandSize_16)
  {
    uint16 lhs = m_cpu->m_registers.AX;
    uint16 rhs = m_cpu->ReadMemoryWord(Segment_ES, dst_address);
    ALUOp_Sub16(&m_cpu->m_registers, lhs, rhs);
    data_size = sizeof(uint16);
  }
  else if (actual_size == OperandSize_32)
  {
    uint32 lhs = m_cpu->m_registers.EAX;
    uint32 rhs = m_cpu->ReadMemoryDWord(Segment_ES, dst_address);
    ALUOp_Sub32(&m_cpu->m_registers, lhs, rhs);
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (istate->address_size == AddressSize_16)
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
      m_cpu->m_registers.DI += ZeroExtend16(data_size);
    else
      m_cpu->m_registers.DI -= ZeroExtend16(data_size);
  }
  else
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
      m_cpu->m_registers.EDI += ZeroExtend32(data_size);
    else
      m_cpu->m_registers.EDI -= ZeroExtend32(data_size);
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void FastInterpreterBackend::Execute_INS(InstructionState* istate)
{
  VirtualMemoryAddress dst_address =
    (istate->address_size == AddressSize_16) ? ZeroExtend32(m_cpu->m_registers.DI) : m_cpu->m_registers.EDI;
  OperandSize actual_size = (dst_size == OperandSize_Count) ? istate->operand_size : dst_size;
  uint16 port_number = m_cpu->m_registers.DX;
  uint8 data_size;

  if (actual_size == OperandSize_8)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint8), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint8 value;
    m_bus->ReadIOPortByte(port_number, &value);
    m_cpu->WriteMemoryByte(Segment_ES, dst_address, value);
    data_size = sizeof(uint8);
  }
  else if (actual_size == OperandSize_16)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint16), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint16 value;
    m_bus->ReadIOPortWord(port_number, &value);
    m_cpu->WriteMemoryWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint16);
  }
  else if (actual_size == OperandSize_32)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint32), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint32 value;
    m_bus->ReadIOPortDWord(port_number, &value);
    m_cpu->WriteMemoryDWord(Segment_ES, dst_address, value);
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (istate->address_size == AddressSize_16)
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
      m_cpu->m_registers.DI += ZeroExtend16(data_size);
    else
      m_cpu->m_registers.DI -= ZeroExtend16(data_size);
  }
  else
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
      m_cpu->m_registers.EDI += ZeroExtend32(data_size);
    else
      m_cpu->m_registers.EDI -= ZeroExtend32(data_size);
  }
}

template<FIOperandMode dst_mode, OperandSize dst_size, uint32 dst_constant, FIOperandMode src_mode,
         OperandSize src_size, uint32 src_constant>
void FastInterpreterBackend::Execute_OUTS(InstructionState* istate)
{
  Segment segment = istate->segment;
  VirtualMemoryAddress src_address =
    (istate->address_size == AddressSize_16) ? ZeroExtend32(m_cpu->m_registers.SI) : m_cpu->m_registers.ESI;
  OperandSize actual_size = (src_size == OperandSize_Count) ? istate->operand_size : src_size;
  uint16 port_number = m_cpu->m_registers.DX;
  uint8 data_size;

  if (actual_size == OperandSize_8)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint8), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint8 value = m_cpu->ReadMemoryByte(segment, src_address);
    m_bus->WriteIOPortByte(port_number, value);
    data_size = sizeof(uint8);
  }
  else if (actual_size == OperandSize_16)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint16), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint16 value = m_cpu->ReadMemoryWord(segment, src_address);
    m_bus->WriteIOPortWord(port_number, value);
    data_size = sizeof(uint16);
  }
  else if (actual_size == OperandSize_32)
  {
    if (!m_cpu->HasIOPermissions(port_number, sizeof(uint32), true))
    {
      m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
      return;
    }

    uint32 value = m_cpu->ReadMemoryDWord(segment, src_address);
    m_bus->WriteIOPortDWord(port_number, value);
    data_size = sizeof(uint32);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }

  if (istate->address_size == AddressSize_16)
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
      m_cpu->m_registers.SI += ZeroExtend16(data_size);
    else
      m_cpu->m_registers.SI -= ZeroExtend16(data_size);
  }
  else
  {
    if (!m_cpu->m_registers.EFLAGS.DF)
      m_cpu->m_registers.ESI += ZeroExtend32(data_size);
    else
      m_cpu->m_registers.ESI -= ZeroExtend32(data_size);
  }
}

void FastInterpreterBackend::Execute_CLC(InstructionState* istate)
{
  SET_FLAG(&m_cpu->m_registers, CF, false);
}

void FastInterpreterBackend::Execute_CLD(InstructionState* istate)
{
  SET_FLAG(&m_cpu->m_registers, DF, false);
}

void FastInterpreterBackend::Execute_CLI(InstructionState* istate)
{
  // TODO: Delay of one instruction
  if (m_cpu->InProtectedMode() && m_cpu->GetCPL() > m_cpu->GetIOPL())
  {
    m_cpu->RaiseException(Interrupt_GeneralProtectionFault);
    return;
  }

  SET_FLAG(&m_cpu->m_registers, IF, false);
}

void FastInterpreterBackend::Execute_CMC(InstructionState* istate)
{
  SET_FLAG(&m_cpu->m_registers, CF, !m_cpu->m_registers.EFLAGS.CF);
}

void FastInterpreterBackend::Execute_STC(InstructionState* istate)
{
  SET_FLAG(&m_cpu->m_registers, CF, true);
}

void FastInterpreterBackend::Execute_STD(InstructionState* istate)
{
  SET_FLAG(&m_cpu->m_registers, DF, true);
}

void FastInterpreterBackend::Execute_STI(InstructionState* istate)
{
  if (m_cpu->InProtectedMode() && m_cpu->GetCPL() > m_cpu->GetIOPL())
  {
    m_cpu->RaiseException(Interrupt_GeneralProtectionFault);
    return;
  }

  SET_FLAG(&m_cpu->m_registers, IF, true);
}

void FastInterpreterBackend::Execute_LAHF(InstructionState* istate)
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

  m_cpu->m_registers.AH = Truncate8(m_cpu->m_registers.EFLAGS.bits);
}

void FastInterpreterBackend::Execute_SAHF(InstructionState* istate)
{
  uint16 flags = Truncate16(m_cpu->m_registers.EFLAGS.bits & 0xFF00) | ZeroExtend16(m_cpu->m_registers.AH);
  m_cpu->SetFlags16(flags);
}

void FastInterpreterBackend::Execute_PUSHF(InstructionState* istate)
{
  // In V8086 mode if IOPL!=3, trap to V8086 monitor
  if (m_cpu->InVirtual8086Mode() && m_cpu->GetIOPL() != 3)
  {
    m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  // RF flag is cleared in the copy
  uint32 EFLAGS = m_cpu->m_registers.EFLAGS.bits;
  EFLAGS &= ~Flag_RF;

  // VM flag is never set from PUSHF
  EFLAGS &= ~Flag_VM;

  if (istate->operand_size == OperandSize_16)
    m_cpu->PushWord(Truncate16(EFLAGS));
  else if (istate->operand_size == OperandSize_32)
    m_cpu->PushDWord(EFLAGS);
  else
    DebugUnreachableCode();
}

void FastInterpreterBackend::Execute_POPF(InstructionState* istate)
{
  // If V8086 and IOPL!=3, trap to monitor
  if (m_cpu->InVirtual8086Mode() && m_cpu->GetIOPL() != 3)
  {
    m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  if (istate->operand_size == OperandSize_16)
  {
    uint16 flags = m_cpu->PopWord();
    m_cpu->SetFlags16(flags);
  }
  else if (istate->operand_size == OperandSize_32)
  {
    uint32 flags = m_cpu->PopDWord();
    m_cpu->SetFlags(flags);
  }
  else
  {
    DebugUnreachableCode();
  }
}

void FastInterpreterBackend::Execute_HLT(InstructionState* istate)
{
  // HLT is a privileged instruction
  if ((m_cpu->InProtectedMode() && m_cpu->GetCPL() != 0) || m_cpu->InVirtual8086Mode())
  {
    m_cpu->RaiseException(Interrupt_GeneralProtectionFault, 0);
    return;
  }

  m_cpu->SetHalted(true);
}

void FastInterpreterBackend::Execute_CBW(InstructionState* istate)
{
  if (istate->operand_size == OperandSize_16)
  {
    // Sign-extend AL to AH
    m_cpu->m_registers.AH = ((m_cpu->m_registers.AL & 0x80) != 0) ? 0xFF : 0x00;
  }
  else if (istate->operand_size == OperandSize_32)
  {
    // Sign-extend AX to EAX
    m_cpu->m_registers.EAX = SignExtend32(m_cpu->m_registers.AX);
  }
  else
  {
    DebugUnreachableCode();
  }
}

void FastInterpreterBackend::Execute_CWD(InstructionState* istate)
{
  if (istate->operand_size == OperandSize_16)
  {
    // Sign-extend AX to DX
    m_cpu->m_registers.DX = ((m_cpu->m_registers.AX & 0x8000) != 0) ? 0xFFFF : 0x0000;
  }
  else if (istate->operand_size == OperandSize_32)
  {
    // Sign-extend EAX to EDX
    m_cpu->m_registers.EDX = ((m_cpu->m_registers.EAX & 0x80000000) != 0) ? 0xFFFFFFFF : 0x00000000;
  }
  else
  {
    DebugUnreachableCode();
  }
}

void FastInterpreterBackend::Execute_XLAT(InstructionState* istate)
{
  uint8 value;
  if (istate->address_size == AddressSize_16)
  {
    uint16 address = m_cpu->m_registers.BX + ZeroExtend16(m_cpu->m_registers.AL);
    value = m_cpu->ReadMemoryByte(istate->segment, address);
  }
  else if (istate->address_size == AddressSize_32)
  {
    uint32 address = m_cpu->m_registers.EBX + ZeroExtend32(m_cpu->m_registers.AL);
    value = m_cpu->ReadMemoryByte(istate->segment, address);
  }
  else
  {
    DebugUnreachableCode();
    return;
  }
  m_cpu->m_registers.AL = value;
}

void FastInterpreterBackend::Execute_AAA(InstructionState* istate)
{
  if ((m_cpu->m_registers.AL & 0xF) > 0x09 || m_cpu->m_registers.EFLAGS.AF)
  {
    m_cpu->m_registers.AX += 0x0106;
    SET_FLAG(&m_cpu->m_registers, AF, true);
    SET_FLAG(&m_cpu->m_registers, CF, true);
  }
  else
  {
    SET_FLAG(&m_cpu->m_registers, AF, false);
    SET_FLAG(&m_cpu->m_registers, CF, false);
  }

  m_cpu->m_registers.AL &= 0x0F;

  SET_FLAG(&m_cpu->m_registers, SF, IsSign(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, ZF, IsZero(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, PF, IsParity(m_cpu->m_registers.AL));
}

void FastInterpreterBackend::Execute_AAS(InstructionState* istate)
{
  if ((m_cpu->m_registers.AL & 0xF) > 0x09 || m_cpu->m_registers.EFLAGS.AF)
  {
    m_cpu->m_registers.AX -= 0x0106;
    SET_FLAG(&m_cpu->m_registers, AF, true);
    SET_FLAG(&m_cpu->m_registers, CF, true);
  }
  else
  {
    SET_FLAG(&m_cpu->m_registers, AF, false);
    SET_FLAG(&m_cpu->m_registers, CF, false);
  }

  m_cpu->m_registers.AL &= 0x0F;

  SET_FLAG(&m_cpu->m_registers, SF, IsSign(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, ZF, IsZero(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, PF, IsParity(m_cpu->m_registers.AL));
}

template<FIOperandMode op_mode, OperandSize op_size, uint32 op_constant>
void CPU_X86::FastInterpreterBackend::Execute_AAM(InstructionState* istate)
{
  FetchImmediate<op_mode, op_size>(istate);
  CalculateEffectiveAddress<op_mode>(istate);

  uint8 operand = ReadByteOperand<op_mode, op_constant>(istate);
  if (operand == 0)
  {
    m_cpu->RaiseException(Interrupt_DivideError);
    return;
  }

  m_cpu->m_registers.AH = m_cpu->m_registers.AL / operand;
  m_cpu->m_registers.AL = m_cpu->m_registers.AL % operand;

  SET_FLAG(&m_cpu->m_registers, AF, false);
  SET_FLAG(&m_cpu->m_registers, CF, false);
  SET_FLAG(&m_cpu->m_registers, OF, false);

  SET_FLAG(&m_cpu->m_registers, SF, IsSign(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, ZF, IsZero(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, PF, IsParity(m_cpu->m_registers.AL));
}

template<FIOperandMode op_mode, OperandSize op_size, uint32 op_constant>
void CPU_X86::FastInterpreterBackend::Execute_AAD(InstructionState* istate)
{
  FetchImmediate<op_mode, op_size>(istate);
  CalculateEffectiveAddress<op_mode>(istate);

  uint8 operand = ReadByteOperand<op_mode, op_constant>(istate);
  uint16 result = uint16(m_cpu->m_registers.AH) * uint16(operand) + uint16(m_cpu->m_registers.AL);

  m_cpu->m_registers.AL = uint8(result & 0xFF);
  m_cpu->m_registers.AH = 0;

  SET_FLAG(&m_cpu->m_registers, AF, false);
  SET_FLAG(&m_cpu->m_registers, CF, false);
  SET_FLAG(&m_cpu->m_registers, OF, false);

  SET_FLAG(&m_cpu->m_registers, SF, IsSign(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, ZF, IsZero(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, PF, IsParity(m_cpu->m_registers.AL));
}

void FastInterpreterBackend::Execute_DAA(InstructionState* istate)
{
  uint8 old_AL = m_cpu->m_registers.AL;
  bool old_CF = m_cpu->m_registers.EFLAGS.CF;

  if ((old_AL & 0xF) > 0x9 || m_cpu->m_registers.EFLAGS.AF)
  {
    SET_FLAG(&m_cpu->m_registers, CF, ((old_AL > 0xF9) || old_CF));
    m_cpu->m_registers.AL += 0x6;
    SET_FLAG(&m_cpu->m_registers, AF, true);
  }
  else
  {
    SET_FLAG(&m_cpu->m_registers, AF, false);
  }

  if (old_AL > 0x99 || old_CF)
  {
    m_cpu->m_registers.AL += 0x60;
    SET_FLAG(&m_cpu->m_registers, CF, true);
  }
  else
  {
    SET_FLAG(&m_cpu->m_registers, CF, false);
  }

  SET_FLAG(&m_cpu->m_registers, OF, false);
  SET_FLAG(&m_cpu->m_registers, SF, IsSign(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, ZF, IsZero(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, PF, IsParity(m_cpu->m_registers.AL));
}

void FastInterpreterBackend::Execute_DAS(InstructionState* istate)
{
  uint8 old_AL = m_cpu->m_registers.AL;
  bool old_CF = m_cpu->m_registers.EFLAGS.CF;

  if ((old_AL & 0xF) > 0x9 || m_cpu->m_registers.EFLAGS.AF)
  {
    SET_FLAG(&m_cpu->m_registers, CF, ((old_AL < 0x06) || old_CF));
    m_cpu->m_registers.AL -= 0x6;
    SET_FLAG(&m_cpu->m_registers, AF, true);
  }
  else
  {
    SET_FLAG(&m_cpu->m_registers, AF, false);
  }

  if (old_AL > 0x99 || old_CF)
  {
    m_cpu->m_registers.AL -= 0x60;
    SET_FLAG(&m_cpu->m_registers, CF, true);
  }

  SET_FLAG(&m_cpu->m_registers, OF, false);
  SET_FLAG(&m_cpu->m_registers, SF, IsSign(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, ZF, IsZero(m_cpu->m_registers.AL));
  SET_FLAG(&m_cpu->m_registers, PF, IsParity(m_cpu->m_registers.AL));
}

void FastInterpreterBackend::Execute_REP(InstructionState* istate, bool is_REPNE)
{
  uint8 opcode = m_cpu->FetchInstructionByte();
  bool is_REPE = false;
  for (;;)
  {
    switch (opcode)
    {
      case 0x6C: // REP INS m8, DX
      case 0x6D: // REP INS m16, DX
      case 0xA4: // REP MOVS m8, m8
      case 0xA5: // REP MOVS m16, m16
      case 0x6E: // REP OUTS DX, r/m8
      case 0x6F: // REP OUTS DX, r/m16
      case 0xAC: // REP LODS AL
      case 0xAD: // REP LODS AX
      case 0xAA: // REP STOS m8
      case 0xAB: // REP STOS m16
      {
        // REPNE is only compatible with CMPS/SCAS
        is_REPNE = false;
      }
      break;

      case 0xA6: // REP CMPS m8, DX
      case 0xA7: // REP CMPS m16, DX
      case 0xAE: // REP SCAS m8
      case 0xAF: // REP SCAS m16
      {
        // Handle REPNE.
        is_REPE = !is_REPNE;
      }
      break;

        // Annoyingly, we have to handle the prefixes here too.
      case 0x26:
        istate->has_segment_override = true;
        istate->segment = Segment_ES;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x36:
        istate->has_segment_override = true;
        istate->segment = Segment_SS;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x2E:
        istate->has_segment_override = true;
        istate->segment = Segment_CS;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x3E:
        istate->has_segment_override = true;
        istate->segment = Segment_DS;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x64:
        istate->has_segment_override = true;
        istate->segment = Segment_FS;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x65:
        istate->has_segment_override = true;
        istate->segment = Segment_GS;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x66:
        istate->operand_size = (m_cpu->m_current_operand_size == OperandSize_16) ? OperandSize_32 : OperandSize_16;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x67:
        istate->address_size = (m_cpu->m_current_address_size == AddressSize_16) ? AddressSize_32 : AddressSize_16;
        opcode = m_cpu->FetchInstructionByte();
        continue;

      default:
      {
        // Any other combination of opcode with REP is undefined.
        // RaiseInvalidOpcode();
        // Some tests rely on the instruction still executing.
        DispatchInstruction(istate, opcode);
        return;
      }
    }

    break;
  }

  for (;;)
  {
    // Check the counter before the condition.
    // Use CX instead of ECX in 16-bit mode.
    // Don't execute a single iteration if it's zero.
    if (istate->address_size == AddressSize_16)
    {
      if (m_cpu->m_registers.CX == 0)
        return;
    }
    else
    {
      if (m_cpu->m_registers.ECX == 0)
        return;
    }

    // Only a few instructions are compatible with REP.
    // None of them require the modrm byte, so this makes dispatch here easier.
    // None of these instructions have any immediates, so no need to restore EIP after each iteration.
    switch (opcode)
    {
      case 0x6C: // REP INS m8, DX
        Execute_INS<FIOperandMode_None, OperandSize_8, 0, FIOperandMode_Register, OperandSize_16, Reg16_DX>(istate);
        break;
      case 0x6D: // REP INS m16, DX
        Execute_INS<FIOperandMode_None, OperandSize_Count, 0, FIOperandMode_Register, OperandSize_16, Reg16_DX>(istate);
        break;
      case 0xA4: // REP MOVS m8, m8
        Execute_MOVS<FIOperandMode_None, OperandSize_8, 0, FIOperandMode_None, OperandSize_8, 0>(istate);
        break;
      case 0xA5: // REP MOVS m16, m16
        Execute_MOVS<FIOperandMode_None, OperandSize_Count, 0, FIOperandMode_None, OperandSize_Count, 0>(istate);
        break;
      case 0x6E: // REP OUTS DX, r/m8
        Execute_OUTS<FIOperandMode_Register, OperandSize_16, Reg16_DX, FIOperandMode_None, OperandSize_8, 0>(istate);
        break;
      case 0x6F: // REP OUTS DX, r/m16
        Execute_OUTS<FIOperandMode_Register, OperandSize_16, Reg16_DX, FIOperandMode_None, OperandSize_Count, 0>(
          istate);
        break;
      case 0xAC: // REP LODS AL
        Execute_LODS<FIOperandMode_Register, OperandSize_8, Reg8_AL, FIOperandMode_None, OperandSize_8, 0>(istate);
        break;
      case 0xAD: // REP LODS AX
        Execute_LODS<FIOperandMode_Register, OperandSize_Count, Reg16_AX, FIOperandMode_None, OperandSize_Count, 0>(
          istate);
        break;
      case 0xAA: // REP STOS m8
        Execute_STOS<FIOperandMode_None, OperandSize_8, 0, FIOperandMode_Register, OperandSize_8, Reg8_AL>(istate);
        break;
      case 0xAB: // REP STOS m16
        Execute_STOS<FIOperandMode_None, OperandSize_Count, 0, FIOperandMode_Register, OperandSize_Count, Reg16_AX>(
          istate);
        break;
      case 0xA6: // REP CMPS m8, m8
        Execute_CMPS<FIOperandMode_None, OperandSize_8, 0, FIOperandMode_None, OperandSize_8, 0>(istate);
        break;
      case 0xA7: // REP CMPS m16, m16
        Execute_CMPS<FIOperandMode_None, OperandSize_Count, 0, FIOperandMode_None, OperandSize_Count, 0>(istate);
        break;
      case 0xAE: // REP SCAS m8
        Execute_SCAS<FIOperandMode_None, OperandSize_8, 0, FIOperandMode_Register, OperandSize_8, Reg8_AL>(istate);
        break;
      case 0xAF: // REP SCAS m16
        Execute_SCAS<FIOperandMode_None, OperandSize_Count, 0, FIOperandMode_Register, OperandSize_Count, Reg16_AX>(
          istate);
        break;
    }

    // Decrement the count register after the operation.
    bool branch = true;
    if (istate->address_size == AddressSize_16)
      branch = (--m_cpu->m_registers.CX != 0);
    else
      branch = (--m_cpu->m_registers.ECX != 0);

    // Finally test the post-condition.
    if (is_REPE)
      branch &= TestJumpCondition<JumpCondition_Equal>(istate);
    else if (is_REPNE)
      branch &= TestJumpCondition<JumpCondition_NotEqual>(istate);

    // Try to batch REP instructions together for speed.
    if (!branch)
      break;

    // If the trap flag is set, we can't do this.
    if (m_cpu->m_registers.EFLAGS.TF)
    {
      m_cpu->RestartCurrentInstruction();
      break;
    }

    // Add a cycle, for the next byte. This could cause an interrupt.
    // This way long-running REP instructions will be paused mid-way.
    m_cpu->CommitPendingCycles();

    // Check if this caused an interrupt.
    if (m_cpu->HasExternalInterrupt())
    {
      // If the interrupt line gets signaled, we need the return address set to the REP instruction.
      m_cpu->RestartCurrentInstruction();
      m_cpu->DispatchExternalInterrupt();
      break;
    }

    m_cpu->AddCycle();
  }
}

#define Eb FIOperandMode_ModRM_RM, OperandSize_8, 0
#define Ev FIOperandMode_ModRM_RM, OperandSize_Count, 0
#define Ew FIOperandMode_ModRM_RM, OperandSize_16, 0
#define Gb FIOperandMode_ModRM_Reg, OperandSize_8, 0
#define Gw FIOperandMode_ModRM_Reg, OperandSize_16, 0
#define Gv FIOperandMode_ModRM_Reg, OperandSize_Count, 0
#define Sw FIOperandMode_ModRM_SegmentReg, OperandSize_16, 0
#define Ib FIOperandMode_Immediate, OperandSize_8, 0
#define Iw FIOperandMode_Immediate, OperandSize_16, 0
#define Iv FIOperandMode_Immediate, OperandSize_Count, 0
#define M FIOperandMode_ModRM_RM, OperandSize_Count, 0
#define Ap FIOperandMode_FarMemory, OperandSize_Count, 0
#define Mp FIOperandMode_ModRM_RM, OperandSize_Count, 0
#define Ob FIOperandMode_Memory, OperandSize_8, 0
#define Ow FIOperandMode_Memory, OperandSize_16, 0
#define Ov FIOperandMode_Memory, OperandSize_Count, 0
#define Jb FIOperandMode_Relative, OperandSize_8, 0
#define Jw FIOperandMode_Relative, OperandSize_16, 0
#define Jv FIOperandMode_Relative, OperandSize_Count, 0
#define Cb(n) FIOperandMode_Constant, OperandSize_8, (n)
#define Cw(n) FIOperandMode_Constant, OperandSize_16, (n)
#define Cv(n) FIOperandMode_Constant, OperandSize_Count, (n)
#define Ms FIOperandMode_ModRM_RM, OperandSize_16, 0
#define Ma FIOperandMode_ModRM_RM, OperandSize_16, 0
#define Cd FIOperandMode_ModRM_ControlRegister, OperandSize_32, 0
#define Td FIOperandMode_ModRM_TestRegister, OperandSize_32, 0
#define Dd FIOperandMode_ModRM_DebugRegister, OperandSize_32, 0
#define Rd FIOperandMode_ModRM_RM, OperandSize_32, 0
#define Xb FIOperandMode_None, OperandSize_8, 0
#define Xv FIOperandMode_None, OperandSize_Count, 0
#define Yb FIOperandMode_None, OperandSize_8, 0
#define Yv FIOperandMode_None, OperandSize_Count, 0
#define AL FIOperandMode_Register, OperandSize_8, Reg8_AL
#define AH FIOperandMode_Register, OperandSize_8, Reg8_AH
#define CL FIOperandMode_Register, OperandSize_8, Reg8_CL
#define CH FIOperandMode_Register, OperandSize_8, Reg8_CH
#define DL FIOperandMode_Register, OperandSize_8, Reg8_DL
#define DH FIOperandMode_Register, OperandSize_8, Reg8_DH
#define BL FIOperandMode_Register, OperandSize_8, Reg8_BL
#define BH FIOperandMode_Register, OperandSize_8, Reg8_BH
#define AX FIOperandMode_Register, OperandSize_16, Reg16_AX
#define CX FIOperandMode_Register, OperandSize_16, Reg16_CX
#define DX FIOperandMode_Register, OperandSize_16, Reg16_DX
#define BX FIOperandMode_Register, OperandSize_16, Reg16_BX
#define SP FIOperandMode_Register, OperandSize_16, Reg16_SP
#define BP FIOperandMode_Register, OperandSize_16, Reg16_BP
#define SI FIOperandMode_Register, OperandSize_16, Reg16_SI
#define DI FIOperandMode_Register, OperandSize_16, Reg16_DI
#define EAX FIOperandMode_Register, OperandSize_32, Reg32_EAX
#define ECX FIOperandMode_Register, OperandSize_32, Reg32_ECX
#define EDX FIOperandMode_Register, OperandSize_32, Reg32_EDX
#define EBX FIOperandMode_Register, OperandSize_32, Reg32_EBX
#define ESP FIOperandMode_Register, OperandSize_32, Reg32_ESP
#define EBP FIOperandMode_Register, OperandSize_32, Reg32_EBP
#define ESI FIOperandMode_Register, OperandSize_32, Reg32_ESI
#define EDI FIOperandMode_Register, OperandSize_32, Reg32_EDI
#define eAX FIOperandMode_Register, OperandSize_Count, Reg32_EAX
#define eCX FIOperandMode_Register, OperandSize_Count, Reg32_ECX
#define eDX FIOperandMode_Register, OperandSize_Count, Reg32_EDX
#define eBX FIOperandMode_Register, OperandSize_Count, Reg32_EBX
#define eSP FIOperandMode_Register, OperandSize_Count, Reg32_ESP
#define eBP FIOperandMode_Register, OperandSize_Count, Reg32_EBP
#define eSI FIOperandMode_Register, OperandSize_Count, Reg32_ESI
#define eDI FIOperandMode_Register, OperandSize_Count, Reg32_EDI
#define CS FIOperandMode_SegmentRegister, OperandSize_16, Segment_CS
#define DS FIOperandMode_SegmentRegister, OperandSize_16, Segment_DS
#define ES FIOperandMode_SegmentRegister, OperandSize_16, Segment_ES
#define SS FIOperandMode_SegmentRegister, OperandSize_16, Segment_SS
#define FS FIOperandMode_SegmentRegister, OperandSize_16, Segment_FS
#define GS FIOperandMode_SegmentRegister, OperandSize_16, Segment_GS

void FastInterpreterBackend::ExecuteInstruction()
{
  InstructionState istate = {};
  istate.address_size = m_cpu->m_current_address_size;
  istate.operand_size = m_cpu->m_current_operand_size;
  istate.segment = Segment_DS;

  uint8 opcode = m_cpu->FetchInstructionByte();
  DispatchInstruction(&istate, opcode);
}

template<FIOperandMode op1_mode, OperandSize op1_size, uint32 op1_constant, FIOperandMode op2_mode,
         OperandSize op2_size, uint32 op2_constant>
void CPU_X86::FastInterpreterBackend::Execute_GRP1(InstructionState* istate)
{
#define FetchAndExecute(op)                                                                                            \
  Execute_##op<op1_mode, op1_size, op1_constant, op2_mode, op2_size, op2_constant>(istate);                            \
  break;

  FetchModRM(istate);
  uint8 sub_opcode = istate->GetModRM_Reg();
  switch (sub_opcode)
  {
    case 0x00:
      FetchAndExecute(ADD);
    case 0x01:
      FetchAndExecute(OR);
    case 0x02:
      FetchAndExecute(ADC);
    case 0x03:
      FetchAndExecute(SBB);
    case 0x04:
      FetchAndExecute(AND);
    case 0x05:
      FetchAndExecute(SUB);
    case 0x06:
      FetchAndExecute(XOR);
    case 0x07:
      FetchAndExecute(CMP);
  }

#undef FetchAndExecute
}

template<FIOperandMode op1_mode, OperandSize op1_size, uint32 op1_constant, FIOperandMode op2_mode,
         OperandSize op2_size, uint32 op2_constant>
void CPU_X86::FastInterpreterBackend::Execute_GRP2(InstructionState* istate)
{
#define FetchAndExecute(op)                                                                                            \
  Execute_##op<op1_mode, op1_size, op1_constant, op2_mode, op2_size, op2_constant>(istate);                            \
  break;

  FetchModRM(istate);
  uint8 sub_opcode = istate->GetModRM_Reg();
  switch (sub_opcode)
  {
    case 0x00:
      FetchAndExecute(ROL);
    case 0x01:
      FetchAndExecute(ROR);
    case 0x02:
      FetchAndExecute(RCL);
    case 0x03:
      FetchAndExecute(RCR);
    case 0x04:
      FetchAndExecute(SHL);
    case 0x05:
      FetchAndExecute(SHR);
    case 0x07:
      FetchAndExecute(SAR);
    default:
      RaiseInvalidOpcode();
      break;
  }

#undef FetchAndExecute
}

template<FIOperandMode op1_mode, OperandSize op1_size, uint32 op1_constant, FIOperandMode op2_mode,
         OperandSize op2_size, uint32 op2_constant>
void CPU_X86::FastInterpreterBackend::Execute_GRP3(InstructionState* istate)
{
#define FetchAndExecute1(op)                                                                                           \
  Execute_##op<op1_mode, op1_size, op1_constant>(istate);                                                              \
  break;

#define FetchAndExecute2(op)                                                                                           \
  Execute_##op<op1_mode, op1_size, op1_constant, op2_mode, op2_size, op2_constant>(istate);                            \
  break;

  FetchModRM(istate);
  uint8 sub_opcode = istate->GetModRM_Reg();
  switch (sub_opcode)
  {
    case 0x00:
      FetchAndExecute2(TEST);
    case 0x02:
      FetchAndExecute1(NOT);
    case 0x03:
      FetchAndExecute1(NEG);
    case 0x04:
      FetchAndExecute1(MUL);
    case 0x05:
      FetchAndExecute1(IMUL1);
    case 0x06:
      FetchAndExecute1(DIV);
    case 0x07:
      FetchAndExecute1(IDIV);
    default:
      RaiseInvalidOpcode();
      break;
  }

#undef FetchAndExecute1
#undef FetchAndExecute2
}

template<FIOperandMode op1_mode, OperandSize op1_size, uint32 op1_constant>
void CPU_X86::FastInterpreterBackend::Execute_GRP4(InstructionState* istate)
{
  FetchModRM(istate);
  uint8 sub_opcode = istate->GetModRM_Reg();
  switch (sub_opcode)
  {
    case 0x00:
      Execute_INC<op1_mode, op1_size, op1_constant>(istate);
      break;
    case 0x01:
      Execute_DEC<op1_mode, op1_size, op1_constant>(istate);
      break;
    default:
      RaiseInvalidOpcode();
      break;
  }
}

template<FIOperandMode op1_mode, OperandSize op1_size, uint32 op1_constant>
void CPU_X86::FastInterpreterBackend::Execute_GRP5(InstructionState* istate)
{
#define FetchAndExecute(op)                                                                                            \
  Execute_##op<op1_mode, op1_size, op1_constant>(istate);                                                              \
  break;
#define FetchAndExecute1(op, op1)                                                                                      \
  Execute_##op<op1>(istate);                                                                                           \
  break;

  FetchModRM(istate);
  uint8 sub_opcode = istate->GetModRM_Reg();
  switch (sub_opcode)
  {
    case 0x00:
      FetchAndExecute(INC);
    case 0x01:
      FetchAndExecute(DEC);
    case 0x02:
      FetchAndExecute(CALL_Near);
    case 0x03:
      FetchAndExecute1(CALL_Far, Mp);
    case 0x04:
      FetchAndExecute(JMP_Near);
    case 0x05:
      FetchAndExecute1(JMP_Far, Mp);
    case 0x06:
      FetchAndExecute(PUSH);

    default:
      RaiseInvalidOpcode();
      break;
  }

#undef FetchAndExecute1
#undef FetchAndExecute
}

void FastInterpreterBackend::DispatchInstruction(InstructionState* istate, uint8 opcode)
{
#define Execute(op) Execute_##op(istate);

#define FetchAndExecute1(op, op1)                                                                                      \
  if constexpr (NeedsModRM(op1))                                                                                       \
  {                                                                                                                    \
    FetchModRM(istate);                                                                                                \
  }                                                                                                                    \
  Execute_##op<op1>(istate);

#define FetchAndExecute2(op, op1, op2)                                                                                 \
  if constexpr (NeedsModRM(op1) || NeedsModRM(op2))                                                                    \
  {                                                                                                                    \
    FetchModRM(istate);                                                                                                \
  }                                                                                                                    \
  Execute_##op<op1, op2>(istate);

#define FetchAndExecute3(op, op1, op2, op3)                                                                            \
  if constexpr (NeedsModRM(op1) || NeedsModRM(op2) || NeedsModRM(op3))                                                 \
  {                                                                                                                    \
    FetchModRM(istate);                                                                                                \
  }                                                                                                                    \
  Execute_##op<op1, op2, op3>(istate);

#define PrefixSegmentOverride(seg)                                                                                     \
  istate->has_segment_override = true;                                                                                 \
  istate->segment = Segment_##seg;                                                                                     \
  opcode = m_cpu->FetchInstructionByte();

#define DoJcc(op1, condition)                                                                                          \
  if constexpr (NeedsModRM(op1))                                                                                       \
  {                                                                                                                    \
    FetchModRM(istate);                                                                                                \
  }                                                                                                                    \
  Execute_Jcc<op1, condition>(istate);

#define DoLoop(op1, condition)                                                                                         \
  if constexpr (NeedsModRM(op1))                                                                                       \
  {                                                                                                                    \
    FetchModRM(istate);                                                                                                \
  }                                                                                                                    \
  Execute_LOOP<op1, condition>(istate);

  // Currently prefix bytes can be allowed in the middle (or after a REP).
  // TODO: This is wrong.
  m_cpu->AddCycle();
  for (;;)
  {
    switch (opcode)
    {
      case 0x26:
        istate->has_segment_override = true;
        istate->segment = Segment_ES;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x36:
        istate->has_segment_override = true;
        istate->segment = Segment_SS;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x2E:
        istate->has_segment_override = true;
        istate->segment = Segment_CS;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x3E:
        istate->has_segment_override = true;
        istate->segment = Segment_DS;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x64: // 80286+
        istate->has_segment_override = true;
        istate->segment = Segment_FS;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x65: // 80286+
        istate->has_segment_override = true;
        istate->segment = Segment_GS;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x66: // 80286+
        istate->operand_size = (m_cpu->m_current_operand_size == OperandSize_16) ? OperandSize_32 : OperandSize_16;
        opcode = m_cpu->FetchInstructionByte();
        continue;
      case 0x67: // 80286+
        istate->address_size = (m_cpu->m_current_address_size == AddressSize_16) ? AddressSize_32 : AddressSize_16;
        opcode = m_cpu->FetchInstructionByte();
        continue;

        // Coprocessor Escape
      case 0xD8:
      case 0xD9:
      case 0xDA:
      case 0xDB:
      case 0xDC:
      case 0xDD:
      case 0xDE:
      case 0xDF:
        // FetchModRM(istate);
        // FetchImmediate<FIOperandMode_ModRM_RM, OperandSize_8>(istate);
        // CalculateEffectiveAddress<FIOperandMode_ModRM_RM>(istate);
        FallbackToInterpreter();
        break;

      case 0x0F: // 80286+
        Execute_0F(istate);
        break;

      case 0x00:
        FetchAndExecute2(ADD, Eb, Gb);
        break;
      case 0x01:
        FetchAndExecute2(ADD, Ev, Gv);
        break;
      case 0x02:
        FetchAndExecute2(ADD, Gb, Eb);
        break;
      case 0x03:
        FetchAndExecute2(ADD, Gv, Ev);
        break;
      case 0x04:
        FetchAndExecute2(ADD, AL, Ib);
        break;
      case 0x05:
        FetchAndExecute2(ADD, eAX, Iv);
        break;
      case 0x06:
        FetchAndExecute1(PUSH_Sreg, ES);
        break;
      case 0x07:
        FetchAndExecute1(POP_Sreg, ES);
        break;
      case 0x08:
        FetchAndExecute2(OR, Eb, Gb);
        break;
      case 0x09:
        FetchAndExecute2(OR, Ev, Gv);
        break;
      case 0x0A:
        FetchAndExecute2(OR, Gb, Eb);
        break;
      case 0x0B:
        FetchAndExecute2(OR, Gv, Ev);
        break;
      case 0x0C:
        FetchAndExecute2(OR, AL, Ib);
        break;
      case 0x0D:
        FetchAndExecute2(OR, eAX, Iv);
        break;
      case 0x0E:
        FetchAndExecute1(PUSH_Sreg, CS);
        break;
        ///* 0x0F */ { Operation_Extension, {}, JumpCondition_Always, DefaultSegment_DS, extension_table_0f },    //
        /// 80286+
      case 0x10:
        FetchAndExecute2(ADC, Eb, Gb);
        break;
      case 0x11:
        FetchAndExecute2(ADC, Ev, Gv);
        break;
      case 0x12:
        FetchAndExecute2(ADC, Gb, Eb);
        break;
      case 0x13:
        FetchAndExecute2(ADC, Gv, Ev);
        break;
      case 0x14:
        FetchAndExecute2(ADC, AL, Ib);
        break;
      case 0x15:
        FetchAndExecute2(ADC, eAX, Iv);
        break;
      case 0x16:
        FetchAndExecute1(PUSH_Sreg, SS);
        break;
      case 0x17:
        FetchAndExecute1(POP_Sreg, SS);
        break;
      case 0x18:
        FetchAndExecute2(SBB, Eb, Gb);
        break;
      case 0x19:
        FetchAndExecute2(SBB, Ev, Gv);
        break;
      case 0x1A:
        FetchAndExecute2(SBB, Gb, Eb);
        break;
      case 0x1B:
        FetchAndExecute2(SBB, Gv, Ev);
        break;
      case 0x1C:
        FetchAndExecute2(SBB, AL, Ib);
        break;
      case 0x1D:
        FetchAndExecute2(SBB, eAX, Iv);
        break;
      case 0x1E:
        FetchAndExecute1(PUSH_Sreg, DS);
        break;
      case 0x1F:
        FetchAndExecute1(POP_Sreg, DS);
        break;
      case 0x20:
        FetchAndExecute2(AND, Eb, Gb);
        break;
      case 0x21:
        FetchAndExecute2(AND, Ev, Gv);
        break;
      case 0x22:
        FetchAndExecute2(AND, Gb, Eb);
        break;
      case 0x23:
        FetchAndExecute2(AND, Gv, Ev);
        break;
      case 0x24:
        FetchAndExecute2(AND, AL, Ib);
        break;
      case 0x25:
        FetchAndExecute2(AND, eAX, Iv);
        break;
      case 0x27:
        Execute(DAA);
        break;
      case 0x28:
        FetchAndExecute2(SUB, Eb, Gb);
        break;
      case 0x29:
        FetchAndExecute2(SUB, Ev, Gv);
        break;
      case 0x2A:
        FetchAndExecute2(SUB, Gb, Eb);
        break;
      case 0x2B:
        FetchAndExecute2(SUB, Gv, Ev);
        break;
      case 0x2C:
        FetchAndExecute2(SUB, AL, Ib);
        break;
      case 0x2D:
        FetchAndExecute2(SUB, eAX, Iv);
        break;
      case 0x2F:
        Execute(DAS);
        break;
      case 0x30:
        FetchAndExecute2(XOR, Eb, Gb);
        break;
      case 0x31:
        FetchAndExecute2(XOR, Ev, Gv);
        break;
      case 0x32:
        FetchAndExecute2(XOR, Gb, Eb);
        break;
      case 0x33:
        FetchAndExecute2(XOR, Gv, Ev);
        break;
      case 0x34:
        FetchAndExecute2(XOR, AL, Ib);
        break;
      case 0x35:
        FetchAndExecute2(XOR, eAX, Iv);
        break;
      case 0x37:
        Execute(AAA);
        break;
      case 0x38:
        FetchAndExecute2(CMP, Eb, Gb);
        break;
      case 0x39:
        FetchAndExecute2(CMP, Ev, Gv);
        break;
      case 0x3A:
        FetchAndExecute2(CMP, Gb, Eb);
        break;
      case 0x3B:
        FetchAndExecute2(CMP, Gv, Ev);
        break;
      case 0x3C:
        FetchAndExecute2(CMP, AL, Ib);
        break;
      case 0x3D:
        FetchAndExecute2(CMP, eAX, Iv);
        break;
      case 0x3F:
        Execute(AAS);
        break;
      case 0x40:
        FetchAndExecute1(INC, eAX);
        break;
      case 0x41:
        FetchAndExecute1(INC, eCX);
        break;
      case 0x42:
        FetchAndExecute1(INC, eDX);
        break;
      case 0x43:
        FetchAndExecute1(INC, eBX);
        break;
      case 0x44:
        FetchAndExecute1(INC, eSP);
        break;
      case 0x45:
        FetchAndExecute1(INC, eBP);
        break;
      case 0x46:
        FetchAndExecute1(INC, eSI);
        break;
      case 0x47:
        FetchAndExecute1(INC, eDI);
        break;
      case 0x48:
        FetchAndExecute1(DEC, eAX);
        break;
      case 0x49:
        FetchAndExecute1(DEC, eCX);
        break;
      case 0x4A:
        FetchAndExecute1(DEC, eDX);
        break;
      case 0x4B:
        FetchAndExecute1(DEC, eBX);
        break;
      case 0x4C:
        FetchAndExecute1(DEC, eSP);
        break;
      case 0x4D:
        FetchAndExecute1(DEC, eBP);
        break;
      case 0x4E:
        FetchAndExecute1(DEC, eSI);
        break;
      case 0x4F:
        FetchAndExecute1(DEC, eDI);
        break;
      case 0x50:
        FetchAndExecute1(PUSH, eAX);
        break;
      case 0x51:
        FetchAndExecute1(PUSH, eCX);
        break;
      case 0x52:
        FetchAndExecute1(PUSH, eDX);
        break;
      case 0x53:
        FetchAndExecute1(PUSH, eBX);
        break;
      case 0x54:
        FetchAndExecute1(PUSH, eSP);
        break;
      case 0x55:
        FetchAndExecute1(PUSH, eBP);
        break;
      case 0x56:
        FetchAndExecute1(PUSH, eSI);
        break;
      case 0x57:
        FetchAndExecute1(PUSH, eDI);
        break;
      case 0x58:
        FetchAndExecute1(POP, eAX);
        break;
      case 0x59:
        FetchAndExecute1(POP, eCX);
        break;
      case 0x5A:
        FetchAndExecute1(POP, eDX);
        break;
      case 0x5B:
        FetchAndExecute1(POP, eBX);
        break;
      case 0x5C:
        FetchAndExecute1(POP, eSP);
        break;
      case 0x5D:
        FetchAndExecute1(POP, eBP);
        break;
      case 0x5E:
        FetchAndExecute1(POP, eSI);
        break;
      case 0x5F:
        FetchAndExecute1(POP, eDI);
        break;
      case 0x60:
        Execute(PUSHA);
        break; // 80286+
      case 0x61:
        Execute(POPA);
        break; // 80286+
               //     case 0x62: FetchAndExecute2(BOUND, Gv, Ma); break;     // 80286+
               //     case 0x63: FetchAndExecute2(ARPL, Ev, Gv); break;
      case 0x68:
        FetchAndExecute1(PUSH, Iv);
        break; // 80286+
               //     case 0x69: FetchAndExecute2(IMUL, Gv, Ev, Iv); break;  // 80286+
      case 0x6A:
        FetchAndExecute1(PUSH, Ib);
        break; // 80286+
               //     case 0x6B: FetchAndExecute2(IMUL, Gv, Ev, Ib); break;  // 80286+
      case 0x6C:
        FetchAndExecute2(INS, Yb, DX);
        break;
      case 0x6D:
        FetchAndExecute2(INS, Yv, DX);
        break;
      case 0x6E:
        FetchAndExecute2(OUTS, DX, Xb);
        break;
      case 0x6F:
        FetchAndExecute2(OUTS, DX, Yv);
        break;
      case 0x70:
        DoJcc(Jb, JumpCondition_Overflow);
        break;
      case 0x71:
        DoJcc(Jb, JumpCondition_NotOverflow);
        break;
      case 0x72:
        DoJcc(Jb, JumpCondition_Below);
        break;
      case 0x73:
        DoJcc(Jb, JumpCondition_AboveOrEqual);
        break;
      case 0x74:
        DoJcc(Jb, JumpCondition_Equal);
        break;
      case 0x75:
        DoJcc(Jb, JumpCondition_NotEqual);
        break;
      case 0x76:
        DoJcc(Jb, JumpCondition_BelowOrEqual);
        break;
      case 0x77:
        DoJcc(Jb, JumpCondition_Above);
        break;
      case 0x78:
        DoJcc(Jb, JumpCondition_Sign);
        break;
      case 0x79:
        DoJcc(Jb, JumpCondition_NotSign);
        break;
      case 0x7A:
        DoJcc(Jb, JumpCondition_Parity);
        break;
      case 0x7B:
        DoJcc(Jb, JumpCondition_NotParity);
        break;
      case 0x7C:
        DoJcc(Jb, JumpCondition_Less);
        break;
      case 0x7D:
        DoJcc(Jb, JumpCondition_GreaterOrEqual);
        break;
      case 0x7E:
        DoJcc(Jb, JumpCondition_LessOrEqual);
        break;
      case 0x7F:
        DoJcc(Jb, JumpCondition_Greater);
        break;
      case 0x80:
        Execute_GRP1<Eb, Ib>(istate);
        break;
      case 0x81:
        Execute_GRP1<Ev, Iv>(istate);
        break;
      case 0x82:
        Execute_GRP1<Eb, Ib>(istate);
        break;
      case 0x83:
        Execute_GRP1<Ev, Ib>(istate);
        break;
      case 0x84:
        FetchAndExecute2(TEST, Gb, Eb);
        break;
      case 0x85:
        FetchAndExecute2(TEST, Gv, Ev);
        break;
      case 0x86:
        FetchAndExecute2(XCHG, Eb, Gb);
        break;
      case 0x87:
        FetchAndExecute2(XCHG, Ev, Gv);
        break;
      case 0x88:
        FetchAndExecute2(MOV, Eb, Gb);
        break;
      case 0x89:
        FetchAndExecute2(MOV, Ev, Gv);
        break;
      case 0x8A:
        FetchAndExecute2(MOV, Gb, Eb);
        break;
      case 0x8B:
        FetchAndExecute2(MOV, Gv, Ev);
        break;
      case 0x8C:
        FetchAndExecute2(MOV_Sreg, Ew, Sw);
        break;
      case 0x8D:
        FetchAndExecute2(LEA, Gv, M);
        break;
      case 0x8E:
        FetchAndExecute2(MOV_Sreg, Sw, Ew);
        break;
      case 0x8F:
        FetchAndExecute1(POP, Ev);
        break;
      case 0x90:
        break;
      case 0x91:
        FetchAndExecute2(XCHG, eCX, eAX);
        break;
      case 0x92:
        FetchAndExecute2(XCHG, eDX, eAX);
        break;
      case 0x93:
        FetchAndExecute2(XCHG, eBX, eAX);
        break;
      case 0x94:
        FetchAndExecute2(XCHG, eSP, eAX);
        break;
      case 0x95:
        FetchAndExecute2(XCHG, eBP, eAX);
        break;
      case 0x96:
        FetchAndExecute2(XCHG, eSI, eAX);
        break;
      case 0x97:
        FetchAndExecute2(XCHG, eDI, eAX);
        break;
      case 0x98:
        Execute(CBW);
        break;
      case 0x99:
        Execute(CWD);
        break;
      case 0x9A:
        FetchAndExecute1(CALL_Far, Ap);
        break;
        //     case 0x9B: Execute_WAIT(&istate); break;
      case 0x9C:
        Execute(PUSHF);
        break;
      case 0x9D:
        Execute(POPF);
        break;
      case 0x9E:
        Execute(SAHF);
        break;
      case 0x9F:
        Execute(LAHF);
        break;
      case 0xA0:
        FetchAndExecute2(MOV, AL, Ob);
        break;
      case 0xA1:
        FetchAndExecute2(MOV, eAX, Ov);
        break;
      case 0xA2:
        FetchAndExecute2(MOV, Ob, AL);
        break;
      case 0xA3:
        FetchAndExecute2(MOV, Ov, eAX);
        break;
      case 0xA4:
        FetchAndExecute2(MOVS, Yb, Xb);
        break;
      case 0xA5:
        FetchAndExecute2(MOVS, Yv, Xv);
        break;
      case 0xA6:
        FetchAndExecute2(CMPS, Xb, Yb);
        break;
      case 0xA7:
        FetchAndExecute2(CMPS, Xv, Yv);
        break;
      case 0xA8:
        FetchAndExecute2(TEST, AL, Ib);
        break;
      case 0xA9:
        FetchAndExecute2(TEST, eAX, Iv);
        break;
      case 0xAA:
        FetchAndExecute2(STOS, Yb, AL);
        break;
      case 0xAB:
        FetchAndExecute2(STOS, Yv, eAX);
        break;
      case 0xAC:
        FetchAndExecute2(LODS, AL, Xb);
        break;
      case 0xAD:
        FetchAndExecute2(LODS, eAX, Xv);
        break;
      case 0xAE:
        FetchAndExecute2(SCAS, AL, Xb);
        break;
      case 0xAF:
        FetchAndExecute2(SCAS, eAX, Xv);
        break;
      case 0xB0:
        FetchAndExecute2(MOV, AL, Ib);
        break;
      case 0xB1:
        FetchAndExecute2(MOV, CL, Ib);
        break;
      case 0xB2:
        FetchAndExecute2(MOV, DL, Ib);
        break;
      case 0xB3:
        FetchAndExecute2(MOV, BL, Ib);
        break;
      case 0xB4:
        FetchAndExecute2(MOV, AH, Ib);
        break;
      case 0xB5:
        FetchAndExecute2(MOV, CH, Ib);
        break;
      case 0xB6:
        FetchAndExecute2(MOV, DH, Ib);
        break;
      case 0xB7:
        FetchAndExecute2(MOV, BH, Ib);
        break;
      case 0xB8:
        FetchAndExecute2(MOV, eAX, Iv);
        break;
      case 0xB9:
        FetchAndExecute2(MOV, eCX, Iv);
        break;
      case 0xBA:
        FetchAndExecute2(MOV, eDX, Iv);
        break;
      case 0xBB:
        FetchAndExecute2(MOV, eBX, Iv);
        break;
      case 0xBC:
        FetchAndExecute2(MOV, eSP, Iv);
        break;
      case 0xBD:
        FetchAndExecute2(MOV, eBP, Iv);
        break;
      case 0xBE:
        FetchAndExecute2(MOV, eSI, Iv);
        break;
      case 0xBF:
        FetchAndExecute2(MOV, eDI, Iv);
        break;
      case 0xC0:
        Execute_GRP2<Eb, Ib>(istate);
        break; // 80286+
      case 0xC1:
        Execute_GRP2<Ev, Ib>(istate);
        break; // 80286+
      case 0xC2:
        FetchAndExecute1(RET_Near, Iw);
        break;
      case 0xC3:
        Execute(RET_Near);
        break;
      case 0xC4:
        FetchAndExecute3(LXS, ES, Gv, Mp);
        break;
      case 0xC5:
        FetchAndExecute3(LXS, DS, Gv, Mp);
        break;
      case 0xC6:
        FetchAndExecute2(MOV, Eb, Ib);
        break;
      case 0xC7:
        FetchAndExecute2(MOV, Ev, Iv);
        break;
      case 0xC8:
        FetchAndExecute2(ENTER, Iw, Ib);
        break;
      case 0xC9:
        Execute(LEAVE);
        break;
      case 0xCA:
        FetchAndExecute1(RET_Far, Iw);
        break;
      case 0xCB:
        Execute(RET_Far);
        break;
      case 0xCC:
        FetchAndExecute1(INT, Cb(3));
        break;
      case 0xCD:
        FetchAndExecute1(INT, Ib);
        break;
      case 0xCE:
        Execute(INTO);
        break;
      case 0xCF:
        Execute(IRET);
        break;
      case 0xD0:
        Execute_GRP2<Eb, Cb(1)>(istate);
        break;
      case 0xD1:
        Execute_GRP2<Ev, Cv(1)>(istate);
        break;
      case 0xD2:
        Execute_GRP2<Eb, CL>(istate);
        break;
      case 0xD3:
        Execute_GRP2<Ev, CL>(istate);
        break;
      case 0xD4:
        FetchAndExecute1(AAM, Ib);
        break;
      case 0xD5:
        FetchAndExecute1(AAD, Ib);
        break;
        //     case 0xD6: Execute_Invalid(&istate); break;
      case 0xD7:
        Execute(XLAT);
        break;
      case 0xE0:
        DoLoop(Jb, JumpCondition_NotEqual);
        break;
      case 0xE1:
        DoLoop(Jb, JumpCondition_Equal);
        break;
      case 0xE2:
        DoLoop(Jb, JumpCondition_Always);
        break;
      case 0xE3:
        DoJcc(Jb, JumpCondition_CXZero);
        break;
      case 0xE4:
        FetchAndExecute2(IN, AL, Ib);
        break;
      case 0xE5:
        FetchAndExecute2(IN, eAX, Ib);
        break;
      case 0xE6:
        FetchAndExecute2(OUT, Ib, AL);
        break;
      case 0xE7:
        FetchAndExecute2(OUT, Ib, eAX);
        break;
      case 0xE8:
        FetchAndExecute1(CALL_Near, Jv);
        break;
      case 0xE9:
        FetchAndExecute1(JMP_Near, Jv);
        break;
      case 0xEA:
        FetchAndExecute1(JMP_Far, Ap);
        break;
      case 0xEB:
        FetchAndExecute1(JMP_Near, Jb);
        break;
      case 0xEC:
        FetchAndExecute2(IN, AL, DX);
        break;
      case 0xED:
        FetchAndExecute2(IN, eAX, DX);
        break;
      case 0xEE:
        FetchAndExecute2(OUT, DX, AL);
        break;
      case 0xEF:
        FetchAndExecute2(OUT, DX, eAX);
        break;
        //     case 0xF0: Execute_Lock_Prefix(&istate); break;
        //     case 0xF1: Execute_Invalid(&istate); break;
      case 0xF2:
        Execute_REP(istate, true);
        break;
      case 0xF3:
        Execute_REP(istate, false);
        break;
      case 0xF4:
        Execute(HLT);
        break;
      case 0xF5:
        Execute(CMC);
        break;
      case 0xF6:
        Execute_GRP3<Eb, Ib>(istate);
        break;
      case 0xF7:
        Execute_GRP3<Ev, Iv>(istate);
        break;
      case 0xF8:
        Execute(CLC);
        break;
      case 0xF9:
        Execute(STC);
        break;
      case 0xFA:
        Execute(CLI);
        break;
      case 0xFB:
        Execute(STI);
        break;
      case 0xFC:
        Execute(CLD);
        break;
      case 0xFD:
        Execute(STD);
        break;
      case 0xFE:
        Execute_GRP4<Eb>(istate);
        break;
      case 0xFF:
        Execute_GRP5<Ev>(istate);
        break;

      default:
        // Panic("Unhandled opcode");
        // m_cpu->m_registers.EIP = m_cpu->m_current_EIP;
        // m_cpu->PrintCurrentStateAndInstruction("Unhandled opcode, falling back to interpreter");
        // Log_WarningPrintf("Unhandled opcode 0x%02X, falling back to interpreter", ZeroExtend32(opcode));
        FallbackToInterpreter();
        break;
    }

    // Exit the for loop because the instruction is complete.
    break;
  }
}

void FastInterpreterBackend::Execute_0F(InstructionState* istate)
{
  uint8 opcode = m_cpu->FetchInstructionByte();
  switch (opcode)
  {
    case 0x80:
      DoJcc(Jv, JumpCondition_Overflow);
      break;
    case 0x81:
      DoJcc(Jv, JumpCondition_NotOverflow);
      break;
    case 0x82:
      DoJcc(Jv, JumpCondition_Below);
      break;
    case 0x83:
      DoJcc(Jv, JumpCondition_AboveOrEqual);
      break;
    case 0x84:
      DoJcc(Jv, JumpCondition_Equal);
      break;
    case 0x85:
      DoJcc(Jv, JumpCondition_NotEqual);
      break;
    case 0x86:
      DoJcc(Jv, JumpCondition_BelowOrEqual);
      break;
    case 0x87:
      DoJcc(Jv, JumpCondition_Above);
      break;
    case 0x88:
      DoJcc(Jv, JumpCondition_Sign);
      break;
    case 0x89:
      DoJcc(Jv, JumpCondition_NotSign);
      break;
    case 0x8A:
      DoJcc(Jv, JumpCondition_Parity);
      break;
    case 0x8B:
      DoJcc(Jv, JumpCondition_NotParity);
      break;
    case 0x8C:
      DoJcc(Jv, JumpCondition_Less);
      break;
    case 0x8D:
      DoJcc(Jv, JumpCondition_GreaterOrEqual);
      break;
    case 0x8E:
      DoJcc(Jv, JumpCondition_LessOrEqual);
      break;
    case 0x8F:
      DoJcc(Jv, JumpCondition_Greater);
      break;

    case 0xA0:
      FetchAndExecute1(PUSH_Sreg, FS);
      break;
    case 0xA1:
      FetchAndExecute1(POP_Sreg, FS);
      break;
    case 0xA8:
      FetchAndExecute1(PUSH_Sreg, GS);
      break;
    case 0xA9:
      FetchAndExecute1(POP_Sreg, GS);
      break;

    case 0xB6:
      FetchAndExecute2(MOVZX, Gv, Eb);
      break; // 80386+
    case 0xB7:
      FetchAndExecute2(MOVZX, Gv, Ew);
      break; // 80386+
    case 0xBE:
      FetchAndExecute2(MOVSX, Gv, Eb);
      break; // 80386+
    case 0xBF:
      FetchAndExecute2(MOVSX, Gv, Ew);
      break; // 80386+

    default:
      // m_cpu->m_registers.EIP = m_cpu->m_current_EIP;
      // m_cpu->PrintCurrentStateAndInstruction("Unhandled 0F opcode, falling back to interpreter");
      // Log_WarningPrintf("Unhandled 0F opcode 0x%02X, falling back to interpreter", ZeroExtend32(opcode));
      FallbackToInterpreter();
      break;
  }
}
} // namespace CPU_X86
