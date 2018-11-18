#include "pce/cpu_x86/interpreter_backend.h"
#include "YBaseLib/Endian.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/String.h"
#include "pce/bus.h"
#include "pce/cpu_x86/decoder.h"
#include "pce/interrupt_controller.h"
#include "pce/system.h"
#include <cstdint>
#include <cstdio>

namespace CPU_X86 {

extern bool TRACE_EXECUTION;
extern uint32 TRACE_EXECUTION_LAST_EIP;

InterpreterBackend::InterpreterBackend(CPU* cpu) : m_cpu(cpu), m_system(cpu->GetSystem()), m_bus(cpu->GetBus()) {}

InterpreterBackend::~InterpreterBackend() {}

void InterpreterBackend::Reset() {}

void InterpreterBackend::Execute()
{
  fastjmp_set(&m_jmp_buf);

  while (!m_cpu->IsHalted() && m_cpu->m_execution_downcount > 0)
  {
    // Check for external interrupts.
    if (m_cpu->HasExternalInterrupt())
      m_cpu->DispatchExternalInterrupt();

    ExecuteInstruction(m_cpu);

    // Run events if needed.
    m_cpu->CommitPendingCycles();
  }
}

void InterpreterBackend::AbortCurrentInstruction()
{
  m_cpu->CommitPendingCycles();
  fastjmp_jmp(&m_jmp_buf);
}

void InterpreterBackend::BranchTo(uint32 new_EIP) {}

void InterpreterBackend::BranchFromException(uint32 new_EIP) {}

void InterpreterBackend::OnControlRegisterLoaded(Reg32 reg, uint32 old_value, uint32 new_value) {}

void InterpreterBackend::FlushCodeCache() {}

void InterpreterBackend::ExecuteInstruction(CPU* cpu)
{
  // The instruction that sets the trap flag should not trigger an interrupt.
  // To handle this, we store the trap flag state before processing the instruction.
  cpu->m_trap_after_instruction = cpu->m_registers.EFLAGS.TF;

  // Store current instruction address in m_current_EIP.
  // The address of the current instruction is needed when exceptions occur.
  cpu->m_current_EIP = cpu->m_registers.EIP;
  cpu->m_current_ESP = cpu->m_registers.ESP;

#if 0
  LinearMemoryAddress linear_address = cpu->CalculateLinearAddress(Segment_CS, cpu->m_registers.EIP);
  if (linear_address == 0xFFE53DC6)
    TRACE_EXECUTION = true;
#endif

  if (TRACE_EXECUTION)
  {
    if (TRACE_EXECUTION_LAST_EIP != cpu->m_current_EIP)
      cpu->PrintCurrentStateAndInstruction();
    TRACE_EXECUTION_LAST_EIP = cpu->m_current_EIP;
  }

  // Cycle for this execution
  cpu->AddCycle();

  // Initialize istate for this instruction
  std::memset(&cpu->idata, 0, sizeof(cpu->idata));
  cpu->idata.address_size = cpu->m_current_address_size;
  cpu->idata.operand_size = cpu->m_current_operand_size;
  cpu->idata.segment = Segment_DS;

  // Read and decode an instruction from the current IP.
  Dispatch(cpu);

  if (cpu->m_trap_after_instruction)
    cpu->RaiseDebugException();
}

void InterpreterBackend::RaiseInvalidOpcode(CPU* cpu)
{
  // This set is only here because of the Print call. RaiseException will reset it itself.
  // cpu->m_registers.EIP = cpu->m_current_EIP;
  // cpu->PrintCurrentStateAndInstruction("Invalid opcode raised");
  cpu->RaiseException(Interrupt_InvalidOpcode);
}

void InterpreterBackend::FetchModRM(CPU* cpu)
{
  cpu->idata.modrm = cpu->FetchInstructionByte();
}

template<OperandSize op_size, OperandMode op_mode, uint32 op_constant>
void CPU_X86::InterpreterBackend::FetchImmediate(CPU* cpu)
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
        uint8 displacement_size = addr->displacement_size;
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

} // namespace CPU_X86

#include "interpreter.h"
#include "interpreter.inl"
#include "interpreter_dispatch.inl"
#include "interpreter_x87.inl"
