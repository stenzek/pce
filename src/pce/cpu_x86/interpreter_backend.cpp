#include "pce/cpu_x86/interpreter_backend.h"
#include "YBaseLib/Log.h"
#include "pce/cpu_x86/debugger_interface.h"
#include "pce/cpu_x86/interpreter.h"
#include "pce/system.h"
#include <cstdint>
#include <cstdio>
Log_SetChannel(CPUX86::Interpreter);

namespace CPU_X86 {

extern bool TRACE_EXECUTION;
extern uint32 TRACE_EXECUTION_LAST_EIP;

InterpreterBackend::InterpreterBackend(CPU* cpu) : m_cpu(cpu), m_system(cpu->GetSystem()) {}

InterpreterBackend::~InterpreterBackend() {}

void InterpreterBackend::Reset() {}

void InterpreterBackend::Execute()
{
  //     // When the debugger is stepping, we ignore the external simulation time
  //     if (m_cpu->m_debugger_interface && m_cpu->m_debugger_interface->IsStepping())
  //     {
  //         // TODO: Handle case where CPU is halted
  //         m_cycles_remaining = m_cpu->m_debugger_interface->GetSteppingInstructionCount();
  //
  //         // Switch to paused once we hit our number of steps
  //         if (m_cycles_remaining == 0)
  //         {
  //             m_cpu->m_system->SetState(System::State::Paused);
  //             return 0;
  //         }
  //     }
  //     else
  //     {
  //         m_cycles_remaining = cycles;
  //     }

  // We'll jump back here when an instruction is aborted.
  setjmp(m_jmp_buf);

  // Assume each instruction takes a single cycle
  // This is totally wrong, but whatever
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

    //         if (m_cpu->InProtectedMode() && (m_cpu->m_registers.EIP == 0xC017FDD5 || m_cpu->m_registers.EIP ==
    //         0xC017FE9A))
    //             trace_execution = true;

    //         LinearMemoryAddress linear_address = m_cpu->CalculateLinearAddress(Segment_CS, m_cpu->m_current_EIP);
    //         if (linear_address == 0x103106 || linear_address == 0)
    //             TRACE_EXECUTION = true;

    //         if (m_cpu->InProtectedMode() && (linear_address == 0xBFF7228A))
    //         {
    //             Log_DevPrintf("ESP=%08X EAX=%08X EBX=%08X", m_cpu->m_registers.ESP, m_cpu->m_registers.EAX,
    //             m_cpu->m_registers.EBX); DumpStack();
    //
    //             //trace_execution = true;
    //             //__debugbreak();
    //             if (m_debugger_interface)
    //                 m_debugger_interface->SetStepping(true, 0);
    //         }

    //         if (m_current_address_size == AddressSize_16 && m_cpu->InProtectedMode() && !m_cpu->InVirtual8086Mode()
    //         && m_cpu->m_registers.EIP == 0x2c5e)
    //             __debugbreak();

    //         if (m_cpu->m_registers.EIP == 0x84B5)
    //             __debugbreak();

    //         if (m_cpu->m_registers.CS == 0x0117 && (m_cpu->m_registers.EIP == 0x00006522 || m_cpu->m_registers.EIP ==
    //         0x651C))
    //             __debugbreak();

    if (TRACE_EXECUTION)
    {
      if (TRACE_EXECUTION_LAST_EIP != m_cpu->m_current_EIP)
        m_cpu->PrintCurrentStateAndInstruction();
      TRACE_EXECUTION_LAST_EIP = m_cpu->m_current_EIP;
    }

    // Read and decode an instruction from the current IP.
    OldInstruction instruction;
    struct Callback : public InstructionFetchCallback
    {
      Callback(CPU* cpu_) : cpu(cpu_) {}

      uint8 FetchByte() override { return cpu->FetchInstructionByte(); }

      uint16 FetchWord() override { return cpu->FetchInstructionWord(); }

      uint32 FetchDWord() override { return cpu->FetchInstructionDWord(); }

      CPU* cpu;
    };
    Callback fetch_callback(m_cpu);

    if (!DecodeInstruction(&instruction, m_cpu->m_current_address_size, m_cpu->m_current_operand_size, fetch_callback))
    {
      // Decode fail, raise trap.
      m_cpu->PrintCurrentStateAndInstruction("Failed to decode instruction");
      m_cpu->RaiseException(Interrupt_InvalidOpcode);
      continue;
    }

    // Actually execute the instruction.
    m_cpu->AddCycle();
    Interpreter::ExecuteInstruction(m_cpu, &instruction);

    if (trap_after_instruction)
    {
      // We should push the next instruction pointer, not the instruction that's trapping,
      // since it has already executed. We also can't use cpu->RaiseException since this would
      // reset the stack pointer too (and it could be a stack-modifying instruction). We
      // also don't need to abort the current instruction since we're looping anyway.
      m_cpu->SetupInterruptCall(Interrupt_Debugger, true, false, 0, m_cpu->m_registers.EIP);
    }

    // Run external events.
    m_cpu->CommitPendingCycles();
  }
}

void InterpreterBackend::AbortCurrentInstruction()
{
  // Log_WarningPrintf("Executing longjmp()");
  longjmp(m_jmp_buf, 1);
}

void InterpreterBackend::BranchTo(uint32 new_EIP) {}

void InterpreterBackend::BranchFromException(uint32 new_EIP) {}

void InterpreterBackend::OnControlRegisterLoaded(Reg32 reg, uint32 old_value, uint32 new_value) {}

void InterpreterBackend::OnLockedMemoryAccess(PhysicalMemoryAddress address, PhysicalMemoryAddress range_start,
                                              PhysicalMemoryAddress range_end, MemoryLockAccess access)
{
}

void InterpreterBackend::FlushCodeCache() {}
} // namespace CPU_X86