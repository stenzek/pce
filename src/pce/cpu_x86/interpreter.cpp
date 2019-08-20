#include "interpreter.h"

// clang-format off
#include "interpreter.inl"
#include "interpreter_x87.inl"
#include "interpreter_dispatch.inl"
// clang-format on

namespace CPU_X86 {

void Interpreter::ExecuteInstruction(CPU* cpu)
{
  // The instruction that sets the trap flag should not trigger an interrupt.
  // To handle this, we store the trap flag state before processing the instruction.
  cpu->m_trap_after_instruction = cpu->m_registers.EFLAGS.TF;

  // Store current instruction address in m_current_EIP.
  // The address of the current instruction is needed when exceptions occur.
  cpu->m_current_EIP = cpu->m_registers.EIP;
  cpu->m_current_ESP = cpu->m_registers.ESP;
  cpu->m_execution_stats.instructions_interpreted++;

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

Interpreter::HandlerFunction Interpreter::GetInterpreterHandlerForInstruction(const Instruction* instruction)
{
  const u64 key = HandlerFunctionKey::Build(
    instruction->operation, instruction->operands[0].size, instruction->operands[0].mode, instruction->operands[0].data,
    instruction->operands[1].size, instruction->operands[1].mode, instruction->operands[1].data,
    instruction->operands[2].size, instruction->operands[2].mode, instruction->operands[2].data);

  auto iter = s_handler_functions.find(key);
  return (iter != s_handler_functions.end()) ? iter->second : nullptr;
}
} // namespace CPU_X86