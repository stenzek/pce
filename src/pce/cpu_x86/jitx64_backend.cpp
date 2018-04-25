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
  // When we reset, we assume we're not in a block.
  m_current_block = nullptr;
  FlushAllBlocks();
}

void JitX64Backend::Execute()
{
  // We'll jump back here when an instruction is aborted.
  setjmp(m_jmp_buf);

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
  longjmp(m_jmp_buf, 1);
}

void JitX64Backend::BranchTo(uint32 new_EIP) {}

void JitX64Backend::BranchFromException(uint32 new_EIP) {}

void JitX64Backend::FlushAllBlocks()
{
  for (auto& it : m_blocks)
  {
    if (it.second == m_current_block)
    {
      m_current_block_flushed = true;
      continue;
    }

    delete it.second;
  }

  m_blocks.clear();
  m_physical_page_blocks.clear();
  m_current_block = nullptr;
  m_code_space->Reset();
}

void JitX64Backend::FlushBlock(const BlockKey& key, bool was_invalidated /* = false */)
{
  auto block_iter = m_blocks.find(key);
  if (block_iter == m_blocks.end())
    return;

  if (was_invalidated)
  {
    block_iter->second->invalidated = true;
    return;
  }

  if (m_current_block == block_iter->second)
    m_current_block_flushed = true;
  else
    delete block_iter->second;

  m_blocks.erase(block_iter);
}

const JitX64Backend::Block* JitX64Backend::LookupBlock()
{
  BlockKey key;
  if (!GetBlockKeyForCurrentState(&key))
  {
    // CPU is in a really bad shape, and should fault.
    Log_PerfPrintf("Falling back to interpreter due to paging error.");
    return nullptr;
  }

  auto lookup_iter = m_blocks.find(key);
  if (lookup_iter != m_blocks.end())
  {
    Block* block = lookup_iter->second;
    if (!block->invalidated || RevalidateCachedBlockForCurrentState(block))
    {
      // Block is valid again.
      return block;
    }

    // Block is no longer valid, so recompile it.
    m_blocks.erase(lookup_iter);
    delete block;
  }

  Block* block = CompileBlock();
  if (!block)
  {
    Log_PerfPrintf("Falling back to interpreter for block %08X due to compile error.", key.eip_physical_address);
    return nullptr;
  }

  m_blocks.emplace(std::make_pair(key, block));
  return block;
}

JitX64Backend::Block* JitX64Backend::CompileBlock()
{
  Block* block = new Block;
  if (!CompileBlockBase(block))
  {
    delete block;
    return nullptr;
  }

  size_t code_size = 128 * block->instructions.size() + 64;
  // if ((code_size % 4096) != 0)
  // code_size = code_size - (code_size % 4096) + 4096;
  // block->AllocCode(code_size);

  if (code_size > m_code_space->GetFreeCodeSpace())
  {
    m_code_buffer_overflow = true;
    delete block;
    return nullptr;
  }

#if 0
  // JitX64CodeGenerator codegen(this, reinterpret_cast<void*>(block->code_pointer), block->code_size);
  JitX64CodeGenerator codegen(this, m_code_space->GetFreeCodePointer(), m_code_space->GetFreeCodeSpace());
  // for (const Instruction& instruction : block->instructions)
  for (size_t i = 0; i < block->instructions.size(); i++)
  {
    bool last_instr = (i == block->instructions.size() - 1);
    if (!codegen.CompileInstruction(&block->instructions[i], last_instr))
    {
      delete block;
      return nullptr;
    }
  }

  auto code = codegen.FinishBlock();
  m_code_space->CommitCode(code.second);

  block->code_pointer = reinterpret_cast<const Block::CodePointer>(code.first);
  block->code_size = code.second;
#endif
  return block;
}

void JitX64Backend::Dispatch()
{
  // Block flush pending?
  if (m_code_buffer_overflow)
  {
    Log_ErrorPrint("Out of code space, flushing all blocks.");
    m_code_buffer_overflow = false;
    m_current_block = nullptr;
    FlushAllBlocks();
  }

  m_current_block = LookupBlock();
  m_current_block_flushed = false;

  if (m_current_block != nullptr)
    // InterpretCachedBlock(m_current_block);
    m_current_block->code_pointer(m_cpu);
  else
    InterpretUncachedBlock();

  // Fix up delayed block destroying.
  if (m_current_block_flushed)
  {
    Log_WarningPrintf("Current block invalidated while executing");
    delete m_current_block;
    m_current_block = nullptr;
  }
}
} // namespace CPU_X86