#include "recompiler_backend.h"
#include "../bus.h"
#include "../system.h"
#include "YBaseLib/Log.h"
#include "debugger_interface.h"
#include "decoder.h"
#include "interpreter.h"
#include "recompiler_code_generator.h"
Log_SetChannel(CPU_X86::Recompiler);

namespace CPU_X86::Recompiler {

extern bool TRACE_EXECUTION;
extern u32 TRACE_EXECUTION_LAST_EIP;

Backend::Backend(CPU* cpu) : CodeCacheBackend(cpu), m_code_space(std::make_unique<JitCodeBuffer>()) {}

Backend::~Backend() {}

void Backend::Reset()
{
  CodeCacheBackend::Reset();
}

void Backend::Execute()
{
  BlockKey key;

  // We'll jump back here when an instruction is aborted.
  fastjmp_set(&m_jmp_buf);
  m_current_block = nullptr;

  while (m_system->ShouldRunCPU())
  {
    if (m_cpu->m_halted)
    {
      m_cpu->m_pending_cycles += m_cpu->m_execution_downcount;
      m_cpu->m_execution_downcount = 0;
      m_cpu->CommitPendingCycles();
      m_system->RunEvents();
      continue;
    }

    while (m_cpu->m_execution_downcount > 0)
    {
      // Block flush pending?
      if (m_code_buffer_overflow)
      {
        Log_ErrorPrint("Out of code space, flushing all blocks.");
        m_code_buffer_overflow = false;
        m_current_block = nullptr;
        FlushCodeCache();
      }

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
        m_current_block = static_cast<Block*>(GetNextBlock());
        if (!m_current_block)
        {
          // No luck. Fall back to interpreter.
          InterpretUncachedBlock();
          m_cpu->CommitPendingCycles();
          continue;
        }
      }

      // Execute the block.
      ExecuteBlock();
      m_cpu->CommitPendingCycles();

      // Fix up delayed block destroying.
      Block* previous_block = m_current_block;
      m_current_block = nullptr;
      if (previous_block->IsDestroyPending())
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
              m_current_block = static_cast<Block*>(GetNextBlock());
              if (m_current_block)
                LinkBlockBase(previous_block, m_current_block);
            }
          }
        }
      }
    }

    // Current block should be cleared after executing, in case of external reset.
    m_current_block = nullptr;

    // Run pending events.
    m_system->RunEvents();
  }
}

void Backend::AbortCurrentInstruction()
{
  // Since we won't return to the dispatcher, clean up the block here.
  if (m_current_block && m_current_block->IsDestroyPending())
  {
    DestroyBlock(m_current_block);
    m_current_block = nullptr;
  }

  m_cpu->CommitPendingCycles();
  fastjmp_jmp(&m_jmp_buf);
}

void Backend::BranchTo(u32 new_EIP)
{
  CodeCacheBackend::BranchTo(new_EIP);
}

void Backend::BranchFromException(u32 new_EIP)
{
  CodeCacheBackend::BranchFromException(new_EIP);
}

void Backend::FlushCodeCache()
{
  // Prevent the current block from being flushed.
  if (m_current_block)
    FlushBlock(m_current_block, true);

  CodeCacheBackend::FlushCodeCache();
  m_code_space->Reset();
}

BlockBase* Backend::AllocateBlock(const BlockKey key)
{
  return new Block(key);
}

bool Backend::CompileBlock(BlockBase* block)
{
  if (!CompileBlockBase(block))
    return false;

  Block* cblock = static_cast<Block*>(block);
  if (m_code_space->GetFreeCodeSpace() < (cblock->instructions.size() * MaximumBytesPerInstruction))
  {
    Log_WarningPrintf("Code space is possibly insufficient for block %08X (%zu instructions), flushing",
                      cblock->GetPhysicalAddress(), cblock->instructions.size());
    m_code_buffer_overflow = true;
    return false;
  }

  CodeGenerator codegen(m_code_space.get());
  if (!codegen.CompileBlock(block, &cblock->code_pointer, &cblock->code_size))
  {
    Log_WarningPrintf("Failed to compile block at paddr %08X", block->key.eip_physical_address);
    return false;
  }

  return true;
}

void Backend::ResetBlock(BlockBase* block)
{
  Block* cblock = static_cast<Block*>(block);
  CodeCacheBackend::ResetBlock(cblock);
  cblock->code_pointer = nullptr;
  cblock->code_size = 0;
}

void Backend::FlushBlock(BlockBase* block, bool defer_destroy /* = false */)
{
  // Defer flush to after execution.
  if (m_current_block == block)
    defer_destroy = true;

  CodeCacheBackend::FlushBlock(block, defer_destroy);
}

void Backend::DestroyBlock(BlockBase* block)
{
  delete static_cast<Block*>(block);
}

void Backend::ExecuteBlock()
{
  m_cpu->m_execution_stats.code_cache_blocks_executed++;
  m_cpu->m_execution_stats.code_cache_instructions_executed += m_current_block->instructions.size();
  m_current_block->code_pointer(m_cpu);
}

} // namespace CPU_X86::Recompiler