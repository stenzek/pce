#include "dasm_proto.h"
#include "dasm_x86.h"
#include "interpreter.h"
#include "recompiler_code_generator.h"

|.arch x64 |.section code |.globals GLOB |.actionlist g_action_list

  |.if ABI_WIN64 |.define RTEMP8A,
  al |.define RTEMP8B, cl |.define RTEMP8C, dl |.define RTEMP16A, ax |.define RTEMP16B, cx |.define RTEMP16C,
  dx |.define RTEMP32A, eax |.define RTEMP32B, ecx |.define RTEMP32C, edx |.define RTEMP64A, rax |.define RTEMP64B,
  rcx |.define RTEMP64C, rdx |.define RTEMPADDR, r8 |.define RSTORE8A, bl |.define RSTORE8B, r12b |.define RSTORE8C,
  r13b |.define RSTORE16A, bx |.define RSTORE16B, r12w |.define RSTORE16C, r13w |.define RSTORE32A,
  ebx |.define RSTORE32B, r12d |.define RSTORE32C, r13d |.define RSTORE64A, rbx |.define RSTORE64B,
  r12 |.define RSTORE64C, r13 |.define READDR16, r14w |.define READDR32, r14d |.define READDR64, r14 |.define RCPUPTR,
  rsi |.define RSCRATCH64, r11 |.define RSCRATCH32, r11d |.define RSCRATCH16, r11w |.define RSCRATCH8,
  r11b |.define RPARAM1_8, cl |.define RPARAM2_8, dl |.define RPARAM3_8, r8b |.define RPARAM4_8, r9b |.define RRET_8,
  al |.define RPARAM1_16, cx |.define RPARAM2_16, dx |.define RPARAM3_16, r8w |.define RPARAM4_16, r9w |.define RRET_16,
  ax |.define RPARAM1_32, ecx |.define RPARAM2_32, edx |.define RPARAM3_32, r8d |.define RPARAM4_32,
  r9d |.define RRET_32, eax |.define RPARAM1_64, rcx |.define RPARAM2_64, rdx |.define RPARAM3_64,
  r8 |.define RPARAM4_64, r9 |.define RRET_64, rax |.else |.define RTEMP8A, al |.define RTEMP8B, cl |.define RTEMP8C,
  dl |.define RTEMP16A, ax |.define RTEMP16B, cx |.define RTEMP16C, dx |.define RTEMP32A, eax |.define RTEMP32B,
  ecx |.define RTEMP32C, edx |.define RTEMP64A, rax |.define RTEMP64B, rcx |.define RTEMP64C, rdx |.define RTEMPADDR,
  r8 |.define RSTORE8A, bl |.define RSTORE8B, r12b |.define RSTORE8C, r13b |.define RSTORE16A, bx |.define RSTORE16B,
  r12w |.define RSTORE16C, r13w |.define RSTORE32A, ebx |.define RSTORE32B, r12d |.define RSTORE32C,
  r13d |.define RSTORE64A, rbx |.define RSTORE64B, r12 |.define RSTORE64C, r13 |.define READDR16,
  r14w |.define READDR32, r14d |.define READDR64, r14 |.define RCPUPTR, rbp |.define RSCRATCH64,
  r11 |.define RSCRATCH32, r11d |.define RSCRATCH16, r11w |.define RSCRATCH8, r11b |.define RPARAM1_8,
  dil |.define RPARAM2_8, sil |.define RPARAM3_8, dl |.define RPARAM4_8, cl |.define RRET_8, al |.define RPARAM1_16,
  di |.define RPARAM2_16, si |.define RPARAM3_16, dx |.define RPARAM4_16, cx |.define RRET_16, ax |.define RPARAM1_32,
  edi |.define RPARAM2_32, esi |.define RPARAM3_32, edx |.define RPARAM4_32, ecx |.define RRET_32,
  eax |.define RPARAM1_64, rdi |.define RPARAM2_64, rsi |.define RPARAM3_64, rdx |.define RPARAM4_64,
  rcx |.define RRET_64,
  rax |.endif

    |.type CPU,
  CPU,
  RCPUPTR

  namespace CPU_X86::Recompiler
{

#define Dst &(m_dasm_state)

  void CodeGenerator::InitHostRegs() {}

  void CodeGenerator::BeginBlock()
  {
    // TODO: Only push these if they're actually used..
    |.if ABI_WIN64 | push rbx | push r12 | push r13 | push r14 | push rsi | sub rsp, 0x20 |.endif
  }

  void CodeGenerator::EndBlock()
  {
    |.if ABI_WIN64 | add rsp, 0x20 | pop rsi | pop r14 | pop r13 | pop r12 | pop rbx |.endif
  }

  void CodeGenerator::StartInstruction(const Instruction* instruction)
  {
    if (!CodeCacheBackend::CanInstructionFault(instruction))
    {
      // Defer updates for non-faulting instructions.
      m_delayed_eip_add += instruction->length;
      m_delayed_cycles_add++;
      return;
    }

    // Update EIP to point to the next instruction.
    uint32 inst_len = instruction->length + m_delayed_eip_add;
    if (m_cpu->m_current_address_size == AddressSize_16)
    {
      // Add pending EndInstruction(), since we clear delayed_eip_add
      if (m_delayed_eip_add > 1)
        | add dword CPU->m_current_EIP,
          m_delayed_eip_add else if (m_delayed_eip_add == 1) | inc dword CPU
                                                                 ->m_current_EIP

                                                               if (inst_len > 1) add dword CPU->m_registers.EIP,
          inst_len else inc dword CPU->m_registers.EIP
    }
    else
    {
      // Add pending EndInstruction(), since we clear delayed_eip_add
      if (m_delayed_eip_add > 1)
        | add dword CPU->m_current_EIP,
          m_delayed_eip_add else if (m_delayed_eip_add == 1) |
            inc dword CPU->m_current_EIP

            if (inst_len > 1) |
            add dword CPU->m_registers.EIP,
          inst_len else | inc dword CPU->m_registers.EIP
    }
    m_delayed_eip_add = 0;

    // Add pending cycles for this instruction.
    uint32 cycles = m_delayed_cycles_add + 1;
    if (cycles > 1)
      add qword CPU->m_pending_cycles, cycles else inc qword CPU->m_pending_cycles m_delayed_cycles_add = 0;
  }

  bool CodeGenerator::Compile_Fallback(const Instruction* instruction)
  {
    Interpreter::HandlerFunction interpreter_handler = Interpreter::GetInterpreterHandlerForInstruction(instruction);
    if (!interpreter_handler)
      return false;

    StartInstruction(instruction);

    // TODO: REP

    | mov RPARAM1_64, RCPUPTR | mov64 RSCRATCH64, instruction->data.bits64[0] | mov CPU->idata.bits64[0],
      RSCRATCH64 | mov64 RSCRATCH64, instruction->data.bits64[1] | mov CPU->idata.bits64[1],
      RSCRATCH64 | mov64 RSCRATCH64,
      reinterpret_cast<size_t>(interpreter_handler) | call RSCRATCH64

                                                        EndInstruction(instruction);
  }

  void CodeGenerator::Compile_MOV(const Instruction* instruction)
  {
    // TODO: Get cycles
    Value value = ReadOperand(instruction, 1, instruction->operands[0].size, false);
    WriteOperand(instruction, 0, std::move(value));
  }

} // namespace CPU_X86::Recompiler