#include "pce/cpu_x86/cached_interpreter_backend.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/cpu_x86/debugger_interface.h"
#include "pce/system.h"
#include <array>
#include <cstdint>
#include <cstdio>
Log_SetChannel(CPUX86::Interpreter);

namespace CPU_X86 {

extern bool TRACE_EXECUTION;
extern uint32 TRACE_EXECUTION_LAST_EIP;

CachedInterpreterBackend::CachedInterpreterBackend(CPU* cpu) : CodeCacheBackend(cpu) {}

CachedInterpreterBackend::~CachedInterpreterBackend() {}

void CachedInterpreterBackend::Reset()
{
  CodeCacheBackend::Reset();
}

void CachedInterpreterBackend::Execute()
{
  BlockKey key;

  // We'll jump back here when an instruction is aborted.
  fastjmp_set(&m_jmp_buf);
  m_current_block = nullptr;

  while (!m_cpu->IsHalted() && m_cpu->m_execution_downcount > 0)
  {
    // Check for external interrupts.
    if (m_cpu->HasExternalInterrupt())
    {
      DebugAssert(m_current_block == nullptr);
      m_cpu->DispatchExternalInterrupt();
    }

    // Execute code.
    if (!m_current_block)
    {
      // No next block, try to get one.
      m_current_block = GetNextBlock();
      if (!m_current_block)
      {
        // No luck. Fall back to interpreter.
        InterpretUncachedBlock();
        m_cpu->CommitPendingCycles();
        continue;
      }
    }

    // Execute the block.
    InterpretCachedBlock(m_current_block);
    m_cpu->CommitPendingCycles();

    // Fix up delayed block destroying.
    Block* previous_block = m_current_block;
    m_current_block = nullptr;
    if (previous_block->destroy_pending)
    {
      FlushBlock(previous_block);
      continue;
    }

    // Block chaining?
    if (!m_cpu->HasExternalInterrupt() && previous_block->IsLinkable())
    {
      if (GetBlockKeyForCurrentState(&key))
      {
        // Block points to itself?
        if (previous_block->key == key && CanExecuteBlock(previous_block))
        {
          // Execute self again.
          m_current_block = previous_block;
        }
        else
        {
          // Try to find an already-linked block.
          for (BlockBase* linked_block : previous_block->link_successors)
          {
            if (linked_block->key == key)
            {
              if (!CanExecuteBlock(linked_block))
              {
                // CanExecuteBlock can result in a block flush, so stop iterating here.
                break;
              }

              // Execute the
              m_current_block = static_cast<Block*>(linked_block);
              continue;
            }
          }

          // No acceptable blocks found in the successor list, try a new one.
          if (!m_current_block)
          {
            // Link the previous block to this new block if we find a new block.
            m_current_block = GetNextBlock();
            if (m_current_block)
              LinkBlockBase(previous_block, m_current_block);
          }
        }
      }
    }
  }
}

void CachedInterpreterBackend::AbortCurrentInstruction()
{
  // Since we won't return to the dispatcher, clean up the block here.
  if (m_current_block && m_current_block->destroy_pending)
  {
    DestroyBlock(m_current_block);
    m_current_block = nullptr;
  }

  // Log_WarningPrintf("Executing longjmp()");
  m_cpu->CommitPendingCycles();
  fastjmp_jmp(&m_jmp_buf);
}

void CachedInterpreterBackend::BranchTo(uint32 new_EIP)
{
  CodeCacheBackend::BranchTo(new_EIP);
}

void CachedInterpreterBackend::BranchFromException(uint32 new_EIP)
{
  CodeCacheBackend::BranchFromException(new_EIP);
}

void CachedInterpreterBackend::FlushCodeCache()
{
  // Prevent the current block from being flushed.
  if (m_current_block)
    FlushBlock(m_current_block, true);

  CodeCacheBackend::FlushCodeCache();
}

CodeCacheBackend::BlockBase* CachedInterpreterBackend::AllocateBlock(const BlockKey key)
{
  return new Block(key);
}

bool CachedInterpreterBackend::CompileBlock(BlockBase* block)
{
  return CompileBlockBase(block);
}

void CachedInterpreterBackend::ResetBlock(BlockBase* block)
{
  CodeCacheBackend::ResetBlock(block);
}

void CachedInterpreterBackend::FlushBlock(BlockBase* block, bool defer_destroy /* = false */)
{
  // Defer flush to after execution.
  if (m_current_block == block)
    defer_destroy = true;

  CodeCacheBackend::FlushBlock(block, defer_destroy);
}

void CachedInterpreterBackend::DestroyBlock(BlockBase* block)
{
  delete static_cast<Block*>(block);
}
} // namespace CPU_X86