// Has to come first due to LLVM includes.
// clang-format off
#include "pce/cpu_x86/recompiler_translator.h"
// clang-format on
#include "pce/cpu_x86/recompiler_backend.h"
#include "YBaseLib/Log.h"
#include "pce/cpu_x86/debugger_interface.h"
#include "pce/cpu_x86/recompiler_code_space.h"
#include "pce/system.h"
#include <array>
#include <cstdint>
#include <cstdio>
Log_SetChannel(CPU_X86::Recompiler);

#ifdef _MSC_VER
#pragma comment(lib, "LLVMAnalysis.lib")
#pragma comment(lib, "LLVMAsmParser.lib")
#pragma comment(lib, "LLVMAsmPrinter.lib")
#pragma comment(lib, "LLVMBinaryFormat.lib")
#pragma comment(lib, "LLVMBitReader.lib")
#pragma comment(lib, "LLVMBitWriter.lib")
#pragma comment(lib, "LLVMCodeGen.lib")
#pragma comment(lib, "LLVMCore.lib")
#pragma comment(lib, "LLVMCoroutines.lib")
#pragma comment(lib, "LLVMCoverage.lib")
#pragma comment(lib, "LLVMDebugInfoCodeView.lib")
#pragma comment(lib, "LLVMDebugInfoDWARF.lib")
#pragma comment(lib, "LLVMDebugInfoMSF.lib")
#pragma comment(lib, "LLVMDebugInfoPDB.lib")
#pragma comment(lib, "LLVMDemangle.lib")
#pragma comment(lib, "LLVMDlltoolDriver.lib")
#pragma comment(lib, "LLVMExecutionEngine.lib")
#pragma comment(lib, "LLVMFuzzMutate.lib")
#pragma comment(lib, "LLVMGlobalISel.lib")
#pragma comment(lib, "LLVMInstCombine.lib")
#pragma comment(lib, "LLVMInstrumentation.lib")
#pragma comment(lib, "LLVMInterpreter.lib")
#pragma comment(lib, "LLVMipo.lib")
#pragma comment(lib, "LLVMIRReader.lib")
#pragma comment(lib, "LLVMLibDriver.lib")
#pragma comment(lib, "LLVMLineEditor.lib")
#pragma comment(lib, "LLVMLinker.lib")
#pragma comment(lib, "LLVMLTO.lib")
#pragma comment(lib, "LLVMMC.lib")
#pragma comment(lib, "LLVMMCDisassembler.lib")
#pragma comment(lib, "LLVMMCJIT.lib")
#pragma comment(lib, "LLVMMCParser.lib")
#pragma comment(lib, "LLVMMIRParser.lib")
#pragma comment(lib, "LLVMObjCARCOpts.lib")
#pragma comment(lib, "LLVMObject.lib")
#pragma comment(lib, "LLVMObjectYAML.lib")
#pragma comment(lib, "LLVMOption.lib")
#pragma comment(lib, "LLVMOrcJIT.lib")
#pragma comment(lib, "LLVMPasses.lib")
#pragma comment(lib, "LLVMProfileData.lib")
#pragma comment(lib, "LLVMRuntimeDyld.lib")
#pragma comment(lib, "LLVMScalarOpts.lib")
#pragma comment(lib, "LLVMSelectionDAG.lib")
#pragma comment(lib, "LLVMSupport.lib")
#pragma comment(lib, "LLVMSymbolize.lib")
#pragma comment(lib, "LLVMTableGen.lib")
#pragma comment(lib, "LLVMTarget.lib")
#pragma comment(lib, "LLVMTransformUtils.lib")
#pragma comment(lib, "LLVMVectorize.lib")
#pragma comment(lib, "LLVMWindowsManifest.lib")
#pragma comment(lib, "LLVMX86AsmParser.lib")
#pragma comment(lib, "LLVMX86AsmPrinter.lib")
#pragma comment(lib, "LLVMX86CodeGen.lib")
#pragma comment(lib, "LLVMX86Desc.lib")
#pragma comment(lib, "LLVMX86Disassembler.lib")
#pragma comment(lib, "LLVMX86Info.lib")
#pragma comment(lib, "LLVMX86Utils.lib")
#pragma comment(lib, "LLVMXRay.lib")
#endif

// TODO: Block leaking on invalidation
// TODO: Remove physical references when block is destroyed

namespace CPU_X86 {

extern bool TRACE_EXECUTION;
extern uint32 TRACE_EXECUTION_LAST_EIP;

static void InitializeLLVM()
{
  static bool llvm_initialized = false;
  if (!llvm_initialized)
  {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    // llvm::InitializeNativeTargetAsmParser();
    llvm_initialized = true;
  }
}

RecompilerBackend::RecompilerBackend(CPU* cpu) : CodeCacheBackend(cpu)
{
  m_code_space = std::make_unique<RecompilerCodeSpace>();
  m_llvm_context = std::make_unique<llvm::LLVMContext>();
  m_translation_block_function_type =
    llvm::FunctionType::get(llvm::Type::getVoidTy(GetLLVMContext()), {llvm::Type::getInt8Ty(GetLLVMContext())}, false);
  InitializeLLVM();
  CreateExecutionEngine();
}

RecompilerBackend::~RecompilerBackend() {}

void RecompilerBackend::Reset()
{
  // When we reset, we assume we're not in a block.
  m_current_block = nullptr;
  m_current_block_flushed = false;
  FlushAllBlocks();
}

void RecompilerBackend::Execute()
{
  // We'll jump back here when an instruction is aborted.
  setjmp(m_jmp_buf);
  while (!m_cpu->IsHalted() && m_cpu->m_execution_downcount > 0)
  {
    // Check for external interrupts.
    if (m_cpu->HasExternalInterrupt())
    {
      m_cpu->DispatchExternalInterrupt();
      m_current_block = nullptr;
      m_current_block_flushed = false;
    }

    Dispatch();

    // Execute events.
    m_cpu->CommitPendingCycles();
  }

  // Clear our current block info. This could change while we're not executing.
  m_current_block = nullptr;
  m_current_block_flushed = false;
}

void RecompilerBackend::AbortCurrentInstruction()
{
  // Since we won't return to the dispatcher, clean up the block here.
  if (m_current_block_flushed)
    DestroyBlock(m_current_block);

  // No longer executing the block we were on.
  m_current_block = nullptr;
  m_current_block_flushed = false;

  // Log_WarningPrintf("Executing longjmp()");
  m_cpu->CommitPendingCycles();
  longjmp(m_jmp_buf, 1);
}

void RecompilerBackend::BranchTo(uint32 new_EIP)
{
  CodeCacheBackend::BranchTo(new_EIP);
}

void RecompilerBackend::BranchFromException(uint32 new_EIP)
{
  CodeCacheBackend::BranchFromException(new_EIP);
}

void RecompilerBackend::FlushAllBlocks()
{
  for (auto& it : m_blocks)
  {
    if (it.second == m_current_block)
      m_current_block_flushed = true;
    else
      DestroyBlock(it.second);
  }

  m_blocks.clear();
  ClearPhysicalPageBlockMapping();
  m_code_space->Reset();
  CreateExecutionEngine();
}

void RecompilerBackend::FlushBlock(const BlockKey& key, bool was_invalidated /* = false */)
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
  if (m_current_block == block)
    m_current_block_flushed = true;
  else
    DestroyBlock(block);
}

void RecompilerBackend::DestroyBlock(Block* block)
{
  UnlinkBlockBase(block);
  delete block;
}

RecompilerBackend::Block* RecompilerBackend::LookupBlock()
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

RecompilerBackend::Block* RecompilerBackend::LookupBlock(const BlockKey& key)
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
    m_blocks.erase(lookup_iter);
    DestroyBlock(block);
  }

  Block* block = CompileBlock(key);
  if (!block)
    Log_WarningPrintf("Falling back to interpreter for block %08X due to compile error.", key.eip_physical_address);

  m_blocks.emplace(std::make_pair(key, block));
  return block;
}

TinyString RecompilerBackend::GetBlockModuleName(const Block* block)
{
  return TinyString::FromFormat("block_%08x", block->key.eip_physical_address, Truncate32(block->key.qword >> 32));
}

void RecompilerBackend::CreateExecutionEngine()
{
  m_execution_engine.reset();

  llvm::EngineBuilder eb;
  m_execution_engine = std::unique_ptr<llvm::ExecutionEngine>(eb.create());
  if (!m_execution_engine)
    Panic("Failed to create LLVM execution engine.");
}

RecompilerBackend::Block* RecompilerBackend::CompileBlock(const BlockKey& key)
{
  Block* block = new Block();
  block->key = key;
  if (!CompileBlockBase(block))
  {
    delete block;
    return nullptr;
  }

  size_t code_size = 128 * block->instructions.size() + 64;
  if (code_size > m_code_space->GetFreeCodeSpace())
  {
    m_code_buffer_overflow = true;
    delete block;
    return nullptr;
  }

  // Create LLVM module and function.
  TinyString module_name(GetBlockModuleName(block));
  std::unique_ptr<llvm::Module> module = std::make_unique<llvm::Module>(module_name.GetCharArray(), GetLLVMContext());
  llvm::Function* function = llvm::Function::Create(
    m_translation_block_function_type, llvm::GlobalValue::ExternalLinkage, module_name.GetCharArray(), module.get());
  DebugAssert(function);

  // Translate the block to LLVM IR.
  RecompilerTranslator translator(this, block, module.get(), function);
  if (!translator.TranslateBlock())
  {
    Log_WarningPrintf("Failed to translate block 0x%08X to LLVM IR.", key.eip_physical_address);
    delete block;
    return nullptr;
  }

  // Compile the LLVM IR to native code.
  m_execution_engine->addModule(std::move(module));
  uint64_t func_addr = m_execution_engine->getFunctionAddress(std::string(module_name));
  if (!func_addr)
  {
    Log_WarningPrintf("Failed to compile block 0x%08X to native code.", key.eip_physical_address);
    delete block;
    return nullptr;
  }

  block->function = function;
  block->code_pointer = reinterpret_cast<Block::CodePointer>(func_addr);
  return block;
}

void RecompilerBackend::Dispatch()
{
  // Block flush pending?
  if (m_code_buffer_overflow)
  {
    Log_ErrorPrint("Out of code space, flushing all blocks.");
    m_code_buffer_overflow = false;
    m_current_block = nullptr;
    m_current_block_flushed = false;
    FlushAllBlocks();
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
    if (m_current_block->code_pointer)
      m_current_block->code_pointer(reinterpret_cast<uint8*>(m_cpu));
    else
      InterpretCachedBlock(m_current_block);

    // Fix up delayed block destroying.
    if (m_current_block_flushed)
    {
      Log_WarningPrintf("Current block invalidated while executing");
      DestroyBlock(m_current_block);
      m_current_block = nullptr;
      m_current_block_flushed = false;
    }
    else
    {
      m_current_block = nullptr;
    }
  }
  else
  {
    InterpretUncachedBlock();
  }
}
} // namespace CPU_X86