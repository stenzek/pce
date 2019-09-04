#include "interpreter_backend.h"
#include "interpreter.h"

namespace CPU_X86 {

extern bool TRACE_EXECUTION;
extern u32 TRACE_EXECUTION_LAST_EIP;

InterpreterBackend::InterpreterBackend(CPU* cpu) : m_cpu(cpu), m_system(cpu->GetSystem()), m_bus(cpu->GetBus()) {}

InterpreterBackend::~InterpreterBackend() {}

void InterpreterBackend::Execute()
{
  fastjmp_set(&m_jmp_buf);

  while (m_system->ShouldRunCPU())
  {
    if (m_cpu->m_halted)
    {
      m_cpu->m_pending_cycles += m_cpu->m_execution_downcount;
      m_cpu->CommitPendingCycles();
      m_system->RunEvents();
      continue;
    }

    while (m_cpu->m_execution_downcount > 0)
    {
      // Check for external interrupts.
      if (m_cpu->HasExternalInterrupt())
        m_cpu->DispatchExternalInterrupt();

#if 0
      LinearMemoryAddress linear_address = cpu->CalculateLinearAddress(Segment_CS, cpu->m_registers.EIP);
      if (linear_address == 0xFFE53DC6)
        TRACE_EXECUTION = true;
#endif
#if 0
      if (m_cpu->ReadTSC() == 0x1A2BF1E2)
        TRACE_EXECUTION = true;
#endif

      if (TRACE_EXECUTION)
      {
        if (TRACE_EXECUTION_LAST_EIP != m_cpu->m_registers.EIP)
          m_cpu->PrintCurrentStateAndInstruction(m_cpu->m_registers.EIP);
        TRACE_EXECUTION_LAST_EIP = m_cpu->m_registers.EIP;
      }

      Interpreter::ExecuteInstruction(m_cpu);
      m_cpu->CommitPendingCycles();
    }

    // Run events if needed.
    m_system->RunEvents();
  }
}

void InterpreterBackend::AbortCurrentInstruction()
{
  m_cpu->CommitPendingCycles();
  fastjmp_jmp(&m_jmp_buf);
}

size_t InterpreterBackend::GetCodeBlockCount() const
{
  return 0;
}

void InterpreterBackend::FlushCodeCache() {}

} // namespace CPU_X86
