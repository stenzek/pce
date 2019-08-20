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

void CachedInterpreterBackend::AbortCurrentInstruction()
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
  for (const Instruction& instruction : cblock->instructions)
  {
    auto handler = Interpreter::GetInterpreterHandlerForInstruction(&instruction);
    if (!handler)
    {
      String disassembled;
      Decoder::DisassembleToString(&instruction, &disassembled);
      Log_ErrorPrintf("Failed to get handler for instruction '%s'", disassembled.GetCharArray());
      return false;
    }

    cblock->entries.push_back({handler, instruction.data, static_cast<u8>(instruction.length)});
  }

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

void CachedInterpreterBackend::ExecuteBlock()
{
  // m_cpu->PrintCurrentStateAndInstruction(m_cpu->m_registers.EIP);
  m_cpu->m_execution_stats.code_cache_blocks_executed++;
  m_cpu->m_execution_stats.code_cache_instructions_executed += m_current_block->entries.size();
  for (const Block::Entry& instruction : m_current_block->entries)
  {
#if 0
    if (TRACE_EXECUTION && m_cpu->m_registers.EIP != TRACE_EXECUTION_LAST_EIP)
    {
      m_cpu->PrintCurrentStateAndInstruction(m_cpu->m_registers.EIP, nullptr);
      TRACE_EXECUTION_LAST_EIP = m_cpu->m_registers.EIP;
    }
#endif

    m_cpu->m_current_EIP = m_cpu->m_registers.EIP;
    m_cpu->m_current_ESP = m_cpu->m_registers.ESP;
    m_cpu->m_registers.EIP = (m_cpu->m_registers.EIP + instruction.length) & m_cpu->m_EIP_mask;
    std::memcpy(&m_cpu->idata, &instruction.data, sizeof(m_cpu->idata));
    instruction.handler(m_cpu);
    // Interpreter::ExecuteInstruction(m_cpu);
  }
}

} // namespace CPU_X86