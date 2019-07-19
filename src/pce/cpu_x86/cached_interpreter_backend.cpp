#include "cached_interpreter_backend.h"
#include "../bus.h"
#include "../system.h"
#include "YBaseLib/Log.h"
#include "debugger_interface.h"
#include "decoder.h"
#include "interpreter.h"
Log_SetChannel(CPU_X86::CachedInterpreterBackend);

namespace CPU_X86 {

extern bool TRACE_EXECUTION;
extern u32 TRACE_EXECUTION_LAST_EIP;

CachedInterpreterBackend::CachedInterpreterBackend(CPU* cpu) : CodeCacheBackend(cpu) {}

CachedInterpreterBackend::~CachedInterpreterBackend() {}

void CachedInterpreterBackend::Reset()
{
  CodeCacheBackend::Reset();
}

void CachedInterpreterBackend::Execute()
{
  // We'll jump back here when an instruction is aborted.
  fastjmp_set(&m_jmp_buf);
  m_current_block = nullptr;

  BlockKey block_key;
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
      // Check for external interrupts.
      if (m_cpu->HasExternalInterrupt())
      {
        DebugAssert(m_current_block == nullptr);
        m_cpu->DispatchExternalInterrupt();
      }

      // Execute code.
      if (!m_current_block)
      {
        if (GetBlockKeyForCurrentState(&block_key))
        {
          // Look for block in the cache.
          auto iter = m_blocks.find(block_key);
          if (iter != m_blocks.end())
          {
            // Can it be executed as-is?
            m_current_block = static_cast<Block*>(iter->second);
            if (!m_current_block)
            {
              // Not a valid block.
              InterpretUncachedBlock();
              m_cpu->CommitPendingCycles();
              continue;
            }
            else if (!CanExecuteBlock(m_current_block))
            {
              // Block needs to be recompiled.
              if (!RecompileBlock(m_current_block))
              {
                // Recompiling failed.
                InterpretUncachedBlock();
                m_cpu->CommitPendingCycles();
                continue;
              }
            }
            else
            {
              // Block can be executed.
              ExecuteBlock(m_current_block);
            }
          }
          else
          {
            // Block needs to be compiled.
            if (!CompileBlock(block_key))
            {
              // No luck. Fall back to interpreter.
              InterpretUncachedBlock();
              m_cpu->CommitPendingCycles();
              continue;
            }
          }
        }
        else
        {
          // We need to fault or something. Not likely.
          InterpretUncachedBlock();
          m_cpu->CommitPendingCycles();
          continue;
        }
      }

      m_cpu->CommitPendingCycles();

      // Fix up delayed block destroying.
      Block* previous_block = m_current_block;
      m_current_block = nullptr;
      if (previous_block->IsDestroyPending())
      {
        FlushBlock(previous_block);
        continue;
      }

#if 0
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
              m_current_block = static_cast<Block*>(LookupNextBlock());
              if (m_current_block)
                LinkBlockBase(previous_block, m_current_block);
            }
          }
        }
      }
#endif
    }

    // Current block should be cleared after executing, in case of external reset.
    m_current_block = nullptr;

    // Run pending events.
    m_system->RunEvents();
  }
}

void CachedInterpreterBackend::AbortCurrentInstruction()
{
  // Since we won't return to the dispatcher, clean up the block here.
  if (m_current_block)
  {
    if (m_current_block->IsDestroyPending())
    {
      DestroyBlock(m_current_block);
    }
    else if (!m_current_block->IsCompiled())
    {
      if (m_current_block->PreviouslyFailedCompilation())
      {
        // The block previously failed compilation, and failed again.
        // No point trying any more.
        m_blocks[m_current_block->key] = nullptr;
        UnlinkBlockBase(m_current_block);
        RemoveBlockPhysicalMappings(m_current_block);
        DestroyBlock(m_current_block);
      }
      else
      {
        // If the block wasn't compiled, set the invalid flag.
        // We want to try recompiling it again the next time it's executed, as it might be the case where the block spans
        // a page, and after faulting the next page will be resident.
        m_current_block->flags |= BlockFlags::Invalidated;
      }
    }

    m_current_block = nullptr;
  }

  m_cpu->CommitPendingCycles();
  fastjmp_jmp(&m_jmp_buf);
}

void CachedInterpreterBackend::BranchTo(u32 new_EIP)
{
  CodeCacheBackend::BranchTo(new_EIP);
}

void CachedInterpreterBackend::BranchFromException(u32 new_EIP)
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

BlockBase* CachedInterpreterBackend::AllocateBlock(const BlockKey key)
{
  return new Block(key);
}

bool CachedInterpreterBackend::CompileBlock(BlockBase* block)
{
  if (!CompileBlockBase(block))
    return false;

  Block* cblock = static_cast<Block*>(block);
  cblock->entries.reserve(cblock->instructions.size());
  for (const BlockBase::InstructionEntry& entry : cblock->instructions)
  {
    cblock->entries.push_back(
      {entry.interpreter_handler, entry.instruction.data, static_cast<u8>(entry.instruction.length)});
  }

  cblock->flags |= BlockFlags::Compiled;
  return true;
}

void CachedInterpreterBackend::ResetBlock(BlockBase* block)
{
  Block* cblock = static_cast<Block*>(block);
  CodeCacheBackend::ResetBlock(cblock);
  cblock->entries.clear();
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

bool CachedInterpreterBackend::CompileBlock(const BlockKey key)
{
  // Insert the partially-compiled block in the map, so we can invalidate it for SMC.
  Block* block = new Block(key);
  InsertBlock(block);
  m_current_block = block;

  // Try to compile the block.
  Log_DebugPrintf("Attempting to compile block %08X", key.eip_physical_address);
  if (CompileBlock(block))
    return true;

  Log_WarningPrintf("Failed to compile block %08X", key.eip_physical_address);

  // Replace the entry in the map with a null pointer, so we don't compile it again.
  m_current_block = nullptr;
  m_blocks[key] = nullptr;
  RemoveBlockPhysicalMappings(block);
  UnlinkBlockBase(block);
  DestroyBlock(block);
  return false;
}

void CachedInterpreterBackend::ExecuteBlock(BlockBase* block)
{
  m_cpu->m_execution_stats.code_cache_blocks_executed++;
  m_cpu->m_execution_stats.code_cache_instructions_executed += static_cast<Block*>(block)->entries.size();
  for (const Block::Entry& instruction : static_cast<Block*>(block)->entries)
  {
#if 0
    if (TRACE_EXECUTION && m_cpu->m_registers.EIP != TRACE_EXECUTION_LAST_EIP)
    {
      m_cpu->PrintCurrentStateAndInstruction(m_cpu->m_registers.EIP, nullptr);
      TRACE_EXECUTION_LAST_EIP = m_cpu->m_registers.EIP;
    }
#endif

    m_cpu->AddCycle();
    m_cpu->m_current_EIP = m_cpu->m_registers.EIP;
    m_cpu->m_current_ESP = m_cpu->m_registers.ESP;
    m_cpu->m_registers.EIP = (m_cpu->m_registers.EIP + instruction.length) & m_cpu->m_EIP_mask;
    std::memcpy(&m_cpu->idata, &instruction.data, sizeof(m_cpu->idata));
    instruction.handler(m_cpu);
    // Interpreter::ExecuteInstruction(m_cpu);
  }
}

} // namespace CPU_X86