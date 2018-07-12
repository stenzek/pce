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
bool ENABLE_CI_BLOCK_CHAINING = true;
static void* last_killed;

CachedInterpreterBackend::CachedInterpreterBackend(CPU* cpu) : CodeCacheBackend(cpu) {}

CachedInterpreterBackend::~CachedInterpreterBackend() {}

void CachedInterpreterBackend::Reset()
{
  // When we reset, we assume we're not in a block.
  m_current_block = nullptr;
  m_current_block_flushed = false;
  FlushAllBlocks();
}

void CachedInterpreterBackend::Execute()
{
  // We'll jump back here when an instruction is aborted.
  fastjmp_set(&m_jmp_buf);
  while (!m_cpu->IsHalted() && m_cpu->m_execution_downcount > 0)
  {
    // Check for external interrupts.
    if (m_cpu->HasExternalInterrupt())
    {
      m_cpu->DispatchExternalInterrupt();
      m_current_block = nullptr;
      m_current_block_flushed = false;
    }

    // Execute code.
    if (!m_current_block)
    {
      // No next block, try to get one.
      m_current_block = LookupBlock();
      m_current_block_flushed = false;
    }
    else if (m_current_block_flushed)
    {
      // Something destroyed our block while we weren't executing.
      DestroyBlock(m_current_block);
      m_current_block = LookupBlock();
      m_current_block_flushed = false;
    }

    if (m_current_block != nullptr)
    {
      InterpretCachedBlock(m_current_block);

      // Fix up delayed block destroying.
      if (m_current_block_flushed)
      {
        DestroyBlock(m_current_block);
        m_current_block = nullptr;
        m_current_block_flushed = false;
      }
      else
      {
        Block* previous_block = m_current_block;
        m_current_block = nullptr;

        if (previous_block->IsLinkable() && ENABLE_CI_BLOCK_CHAINING)
        {
          BlockKey key;
          if (GetBlockKeyForCurrentState(&key))
          {
            // Block points to itself?
            if (previous_block->key == key)
            {
              if (previous_block->invalidated && !RevalidateCachedBlockForCurrentState(previous_block))
              {
                // Self-linking block invalidated itself.
                FlushBlock(key, false);
              }
              else
              {
                // Execute self again.
                m_current_block = previous_block;
              }
            }
            else
            {
              // Try to find an already-linked block.
              for (BlockBase* linked_block : previous_block->link_successors)
              {
                if (linked_block->key == key)
                {
                  if (linked_block->invalidated && !RevalidateCachedBlockForCurrentState(linked_block))
                  {
                    // This will invalidate the list we're looping through.
                    FlushBlock(key, false);
                    break;
                  }

                  // Block is okay.
                  m_current_block = static_cast<Block*>(linked_block);
                  break;
                }
              }

              // No acceptable blocks found in the successor list, try a new one.
              if (!m_current_block)
              {
                // Link the previous block to this new block if we find a new block.
                m_current_block = LookupBlock(key);
                if (m_current_block)
                  LinkBlockBase(previous_block, m_current_block);
              }
            }
          }
        }
        else
        {
          m_current_block = nullptr;
          m_current_block_flushed = false;
        }
      }
    }
    else
    {
      InterpretUncachedBlock();
    }

    // Execute events.
    m_cpu->CommitPendingCycles();
  }

  // Clear our current block info. This could change while we're not executing.
  m_current_block = nullptr;
  m_current_block_flushed = false;
}

void CachedInterpreterBackend::AbortCurrentInstruction()
{
  // Since we won't return to the dispatcher, clean up the block here.
  if (m_current_block_flushed)
    DestroyBlock(m_current_block);

  // No longer executing the block we were on.
  m_current_block = nullptr;
  m_current_block_flushed = false;

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

void CachedInterpreterBackend::FlushAllBlocks()
{
  for (auto& it : m_blocks)
  {
    if (!it.second)
      continue;
    else if (it.second == m_current_block)
      m_current_block_flushed = true;
    else
      DestroyBlock(it.second);
  }

  m_blocks.clear();
  ClearPhysicalPageBlockMapping();
}

void CachedInterpreterBackend::FlushBlock(const BlockKey& key, bool was_invalidated /* = false */)
{
  auto block_iter = m_blocks.find(key);
  if (block_iter == m_blocks.end())
    return;

  Block* block = block_iter->second;
  if (was_invalidated)
  {
    block->invalidated = true;
    return;
  }

  m_blocks.erase(block_iter);
  if (!block)
    return;

  if (m_current_block == block)
    m_current_block_flushed = true;
  else
    DestroyBlock(block);
}

void CachedInterpreterBackend::DestroyBlock(Block* block)
{
  UnlinkBlockBase(block);
  delete block;
}

CachedInterpreterBackend::Block* CachedInterpreterBackend::LookupBlock()
{
  BlockKey key;
  if (!GetBlockKeyForCurrentState(&key))
  {
    // CPU is in a really bad shape, and should fault.
    Log_PerfPrintf("Falling back to interpreter due to paging error.");
    return nullptr;
  }

  return LookupBlock(key);
}

CachedInterpreterBackend::Block* CachedInterpreterBackend::LookupBlock(const BlockKey& key)
{
  auto lookup_iter = m_blocks.find(key);
  if (lookup_iter != m_blocks.end())
  {
    Block* block = lookup_iter->second;
    if (!block)
    {
      // Block not cachable.
      return nullptr;
    }

    if (!block->invalidated || RevalidateCachedBlockForCurrentState(block))
    {
      // Block is valid again.
      return block;
    }

    // Block is no longer valid, so recompile it.
    last_killed = block;
    m_blocks.erase(lookup_iter);
    DestroyBlock(block);
  }

  Block* block = CompileBlock(key);
  m_blocks.emplace(std::make_pair(key, block));
  return block;
}

CachedInterpreterBackend::Block* CachedInterpreterBackend::CompileBlock(const BlockKey& key)
{
  Block* block = new Block();
  block->key = key;
  if (!CompileBlockBase(block))
  {
    delete block;
    return nullptr;
  }

  return block;
}
} // namespace CPU_X86