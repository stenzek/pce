#include "pce/cpu_x86/jitx64_backend.h"
#include "YBaseLib/Log.h"
#include "pce/cpu_x86/debugger_interface.h"
#include "pce/cpu_x86/jitx64_code.h"
#include "pce/cpu_x86/jitx64_codegen.h"
#include "pce/system.h"
#include "xbyak.h"
#include <array>
#include <cstdint>
#include <cstdio>
Log_SetChannel(CPUX86::Interpreter);

// TODO: Block leaking on invalidation
// TODO: Remove physical references when block is destroyed

namespace CPU_X86 {

extern bool TRACE_EXECUTION;
extern uint32 TRACE_EXECUTION_LAST_EIP;

JitX64Backend::JitX64Backend(CPU* cpu) : CodeCacheBackend(cpu), m_code_space(std::make_unique<JitX64Code>()) {}

JitX64Backend::~JitX64Backend() {}

void JitX64Backend::Reset()
{
  CodeCacheBackend::Reset();
}

void JitX64Backend::Execute()
{
  // We'll jump back here when an instruction is aborted.
  fastjmp_set(&m_jmp_buf);

  // Assume each instruction takes a single cycle
  // This is totally wrong, but whatever
  while (!m_cpu->IsHalted() && m_cpu->m_execution_downcount > 0)
  {
    // Check for external interrupts.
    if (m_cpu->HasExternalInterrupt())
      m_cpu->DispatchExternalInterrupt();

    Dispatch();

    m_cpu->CommitPendingCycles();
  }
}

void JitX64Backend::AbortCurrentInstruction()
{
  // Since we won't return to the dispatcher, clean up the block here.
  if (m_current_block_flushed)
  {
    Log_WarningPrintf("Current block invalidated while executing");
    delete m_current_block;
    m_current_block = nullptr;
  }

  // Log_WarningPrintf("Executing longjmp()");
  m_cpu->CommitPendingCycles();
  fastjmp_jmp(&m_jmp_buf);
}

void JitX64Backend::BranchTo(uint32 new_EIP) {}

void JitX64Backend::BranchFromException(uint32 new_EIP) {}

void JitX64Backend::FlushCodeCache()
{
  // Prevent the current block from being flushed.
  if (m_current_block)
  {
    auto iter = m_blocks.find(m_current_block->key);
    Assert(iter != m_blocks.end());
    m_blocks.erase(iter);
    UnlinkBlockBase(m_current_block);
    m_current_block_flushed = true;
  }

  CodeCacheBackend::FlushCodeCache();
  m_code_space->Reset();
}

CodeCacheBackend::BlockBase* JitX64Backend::AllocateBlock(const BlockKey key)
{
  return new Block(key);
}

bool JitX64Backend::CompileBlock(BlockBase* block)
{
  if (!CompileBlockBase(block))
    return false;

  size_t code_size = 128 * block->instructions.size() + 64;
  // if ((code_size % 4096) != 0)
  // code_size = code_size - (code_size % 4096) + 4096;
  // block->AllocCode(code_size);

  if (code_size > m_code_space->GetFreeCodeSpace())
  {
    m_code_buffer_overflow = true;
    return false;
  }

  // JitX64CodeGenerator codegen(this, reinterpret_cast<void*>(block->code_pointer), block->code_size);
  JitX64CodeGenerator codegen(this, m_code_space->GetFreeCodePointer(), m_code_space->GetFreeCodeSpace());
  // for (const Instruction& instruction : block->instructions)
  for (size_t i = 0; i < block->instructions.size(); i++)
  {
    bool last_instr = (i == block->instructions.size() - 1);
    if (!codegen.CompileInstruction(&block->instructions[i], last_instr))
      return false;
  }

  auto code = codegen.FinishBlock();
  m_code_space->CommitCode(code.second);

  static_cast<Block*>(block)->code_pointer = reinterpret_cast<const Block::CodePointer>(code.first);
  static_cast<Block*>(block)->code_size = code.second;
  return true;
}

void JitX64Backend::ResetBlock(BlockBase* block)
{
  static_cast<Block*>(block)->code_pointer = nullptr;
  static_cast<Block*>(block)->code_size = 0;
  CodeCacheBackend::ResetBlock(block);
}

void JitX64Backend::FlushBlock(BlockBase* block)
{
  // Defer flush to after execution.
  if (m_current_block == block)
    m_current_block_flushed = true;
  else
    CodeCacheBackend::FlushBlock(block);
}

void JitX64Backend::DestroyBlock(BlockBase* block)
{
  delete static_cast<Block*>(block);
}

void JitX64Backend::Dispatch()
{
  // Block flush pending?
  if (m_code_buffer_overflow)
  {
    Log_ErrorPrint("Out of code space, flushing all blocks.");
    m_code_buffer_overflow = false;
    m_current_block = nullptr;
    FlushCodeCache();
  }

  m_current_block = static_cast<Block*>(GetNextBlock());

  if (m_current_block != nullptr)
    // InterpretCachedBlock(m_current_block);
    m_current_block->code_pointer(m_cpu);
  else
    InterpretUncachedBlock();

  Block* previous_block = m_current_block;
  m_current_block = nullptr;

  // Fix up delayed block destroying.
  if (m_current_block_flushed)
  {
    Log_WarningPrintf("Current block invalidated while executing");
    FlushBlock(previous_block);
  }
}

} // namespace CPU_X86