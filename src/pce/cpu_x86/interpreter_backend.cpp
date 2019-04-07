#include "interpreter_backend.h"
#include "interpreter.h"

namespace CPU_X86 {

extern bool TRACE_EXECUTION;
extern u32 TRACE_EXECUTION_LAST_EIP;

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

#if 0
    LinearMemoryAddress linear_address = cpu->CalculateLinearAddress(Segment_CS, cpu->m_registers.EIP);
    if (linear_address == 0xFFE53DC6)
      TRACE_EXECUTION = true;
#endif

    if (TRACE_EXECUTION)
    {
      if (TRACE_EXECUTION_LAST_EIP != m_cpu->m_current_EIP)
        m_cpu->PrintCurrentStateAndInstruction();
      TRACE_EXECUTION_LAST_EIP = m_cpu->m_current_EIP;
    }

    Interpreter::ExecuteInstruction(m_cpu);

    // Run events if needed.
    m_cpu->CommitPendingCycles();
  }
}

void InterpreterBackend::AbortCurrentInstruction()
{
  m_cpu->CommitPendingCycles();
  fastjmp_jmp(&m_jmp_buf);
}

void InterpreterBackend::BranchTo(u32 new_EIP) {}

void InterpreterBackend::BranchFromException(u32 new_EIP) {}

void InterpreterBackend::OnControlRegisterLoaded(Reg32 reg, u32 old_value, u32 new_value) {}

void InterpreterBackend::FlushCodeCache() {}

} // namespace CPU_X86
