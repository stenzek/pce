#pragma once

#include "common/clock.h"
#include "pce/cpu.h"
#include "pce/cpu_x86/cycles.h"
#include "pce/cpu_x86/types.h"
#include <functional>
#include <memory>

// Enable TLB emulation?
#define ENABLE_TLB_EMULATION 1
#define ENABLE_PREFETCH_EMULATION 1
#define PREFETCH_QUEUE_SIZE 32

class DebuggerInterface;
class InterruptController;

namespace CPU_X86 {

class Backend;
class DebuggerInterface;

class CPU : public CPUBase
{
  DECLARE_OBJECT_TYPE_INFO(CPU, CPUBase);
  DECLARE_OBJECT_NO_FACTORY(CPU);
  DECLARE_OBJECT_PROPERTY_MAP(CPU);

  friend class Backend;
  friend class DebuggerInterface;
  friend class CodeCacheBackend;
  friend class CachedInterpreterBackend;
  friend class Interpreter;
  friend class InterpreterBackend;
  friend class JitX64Backend;
  friend class JitX64CodeGenerator;

public:
  static const uint32 SERIALIZATION_ID = MakeSerializationID('C', 'P', 'U', 'C');
  static const uint32 PAGE_SIZE = 4096;
  static const uint32 PAGE_OFFSET_MASK = (PAGE_SIZE - 1);
  static const uint32 PAGE_MASK = ~PAGE_OFFSET_MASK;
  static const size_t TLB_ENTRY_COUNT = 8192;

#pragma pack(push, 1)
  // Needed because the 8-bit register indices are all low bits -> all high bits
  struct Reg8Access
  {
    const uint8& operator[](size_t index) const { return data[index % 4][index / 4]; }
    uint8& operator[](size_t index) { return data[index % 4][index / 4]; }

    uint8 data[Reg32_Count][4];
  };
  struct Reg16Access
  {
    const uint16& operator[](size_t index) const { return data[index][0]; }
    uint16& operator[](size_t index) { return data[index][0]; }

    uint16 data[Reg32_Count][2];
  };

  template<uint32 mask>
  struct FlagAccess
  {
    inline bool IsSet() const { return !!(data & mask); }
    inline operator bool() const { return IsSet(); }

    inline FlagAccess& operator=(bool value)
    {
      if (value)
        data |= mask;
      else
        data &= ~mask;

      return *this;
    }

    uint32 data;
  };
#pragma pack(pop)

  union Registers
  {
    struct
    {
      // Little-endian, so LSB first
      uint8 AL;
      uint8 AH;
      uint8 __reserved8_0;
      uint8 __reserved8_1;
      uint8 CL;
      uint8 CH;
      uint8 __reserved8_2;
      uint8 __reserved8_3;
      uint8 DL;
      uint8 DH;
      uint8 __reserved8_4;
      uint8 __reserved8_5;
      uint8 BL;
      uint8 BH;
      uint8 __reserved8_6;
      uint8 __reserved8_7;
    };

    struct
    {
      uint16 AX;
      uint16 __reserved16_0;
      uint16 CX;
      uint16 __reserved16_1;
      uint16 DX;
      uint16 __reserved16_2;
      uint16 BX;
      uint16 __reserved16_3;
      uint16 SP;
      uint16 __reserved16_4;
      uint16 BP;
      uint16 __reserved16_5;
      uint16 SI;
      uint16 __reserved16_6;
      uint16 DI;
      uint16 __reserved16_7;
      uint16 IP;
      uint16 __reserved16_8;
      uint16 FLAGS;
      uint16 __reserved16_9;
    };

    struct
    {
      uint32 EAX;
      uint32 ECX;
      uint32 EDX;
      uint32 EBX;
      uint32 ESP;
      uint32 EBP;
      uint32 ESI;
      uint32 EDI;

      uint32 EIP;

      union
      {
        FlagAccess<Flag_CF> CF;
        FlagAccess<Flag_PF> PF;
        FlagAccess<Flag_AF> AF;
        FlagAccess<Flag_ZF> ZF;
        FlagAccess<Flag_SF> SF;
        FlagAccess<Flag_TF> TF;
        FlagAccess<Flag_IF> IF;
        FlagAccess<Flag_DF> DF;
        FlagAccess<Flag_OF> OF;
        FlagAccess<Flag_NT> NT;
        FlagAccess<Flag_RF> RF;
        FlagAccess<Flag_VM> VM;
        FlagAccess<Flag_AC> AC;
        uint32 bits;
      } EFLAGS;

      uint32 CR0;
      uint32 CR1;
      uint32 CR2;
      uint32 CR3;
      union
      {
        uint32 bits;
        BitField<uint32, bool, 0, 1> VME;
        BitField<uint32, bool, 1, 1> PVI;
        BitField<uint32, bool, 2, 1> TSD;
        BitField<uint32, bool, 3, 1> DE;
      } CR4;

      uint32 DR0;
      uint32 DR1;
      uint32 DR2;
      uint32 DR3;
      uint32 DR4;
      uint32 DR5;
      uint32 DR6;
      uint32 DR7;

      uint32 TR3;
      uint32 TR4;
      uint32 TR5;
      uint32 TR6;
      uint32 TR7;

      union
      {
        struct
        {
          uint16 ES;
          uint16 CS;
          uint16 SS;
          uint16 DS;
          uint16 FS;
          uint16 GS;
        };
        uint16 segment_selectors[Segment_Count];
      };

      // uint48 GDTR;
      // uint48 IDTR;
      uint16 LDTR;
      uint16 TR;
    };

    Reg8Access reg8;
    Reg16Access reg16;
    uint32 reg32[Reg32_Count];
  };

  struct FPURegisters
  {
    float80 ST[8];
    FPUControlWord CW;
    FPUStatusWord SW;
    FPUTagWord TW;
  };

  struct DescriptorTablePointer
  {
    LinearMemoryAddress base_address;
    VirtualMemoryAddress limit;
  };

  struct SegmentCache
  {
    // limit_low/limit_high are _inclusive_ of these addresses
    LinearMemoryAddress base_address;
    VirtualMemoryAddress limit_low;
    VirtualMemoryAddress limit_high;
    SEGMENT_DESCRIPTOR_ACCESS_BITS access;
    AccessTypeMask access_mask;
  };

  struct TSSCache
  {
    LinearMemoryAddress base_address;
    VirtualMemoryAddress limit;
    DESCRIPTOR_TYPE type;
  };

  struct TemporaryStack
  {
    CPU* cpu;
    uint32 ESP;
    uint32 base_address;
    uint32 limit_low;
    uint32 limit_high;
    AddressSize address_size;
    uint16 SS;

    TemporaryStack(CPU* cpu_, uint32 ESP_, uint16 SS_, uint32 base_address_, uint32 limit_low_, uint32 limit_high_,
                   AddressSize address_size_);
    TemporaryStack(CPU* cpu_, uint32 ESP_, uint16 SS_, const DESCRIPTOR_ENTRY& dentry);
    TemporaryStack(CPU* cpu_, uint32 ESP_, uint16 SS_);

    bool CanPushBytes(uint32 num_bytes) const;
    bool CanPushWords(uint32 num_words) const;
    bool CanPushDWords(uint32 num_dwords) const;

    void PushWord(uint16 value);
    void PushDWord(uint32 value);
    uint16 PopWord();
    uint32 PopDWord();

    void SwitchTo();
  };

  // Interrupt hook callback
  using InterruptHookCallback = std::function<bool(uint32 interrupt, Registers* registers)>;

  CPU(const String& identifier, Model model, float frequency, CPUBackendType backend_type = CPUBackendType::Interpreter,
      const ObjectTypeInfo* type_info = &s_type_info);
  ~CPU();

  const char* GetModelString() const;

  const Registers* GetRegisters() const { return &m_registers; }
  Registers* GetRegisters() { return &m_registers; }

  AddressSize GetCurrentAddressingSize() const { return m_current_address_size; }
  OperandSize GetCurrentOperandSize() const { return m_current_operand_size; }
  AddressSize GetStackAddressSize() const { return m_stack_address_size; }
  VirtualMemoryAddress GetCurrentAddressMask() const
  {
    return (m_current_address_size == AddressSize_16) ? 0xFFFF : 0xFFFFFFFF;
  }
  VirtualMemoryAddress GetInstructionAddressMask() const
  {
    return (idata.address_size == AddressSize_16) ? 0xFFFF : 0xFFFFFFFF;
  }

  bool IsHalted() const { return m_halted; }

  // Cycle tracking when executing.
  void AddCycle() { m_pending_cycles++; }
  void AddMemoryCycle() { /*m_pending_cycles++;*/}
  void AddCycles(CYCLE_GROUP group) { m_pending_cycles += ZeroExtend64(m_cycle_group_timings[group]); }
  void AddCyclesPMode(CYCLE_GROUP group)
  {
    m_pending_cycles += ZeroExtend64(m_cycle_group_timings[group + (m_registers.CR0 & 0x01)]);
  }
  void AddCyclesRM(CYCLE_GROUP group, bool rm_reg)
  {
    m_pending_cycles += ZeroExtend64(m_cycle_group_timings[group + static_cast<int>(rm_reg)]);
  }
  void CommitPendingCycles();
  u64 ReadTSC() const;

  // Calculates the physical address of memory with the specified segment and offset.
  // If code is set, it is assumed to be reading instructions, otherwise data.
  PhysicalMemoryAddress CalculateLinearAddress(Segment segment, VirtualMemoryAddress offset);

  // Translates linear address -> physical address if paging is enabled.
  bool TranslateLinearAddress(PhysicalMemoryAddress* out_physical_address, LinearMemoryAddress linear_address,
                              bool access_check, AccessType access_type, bool raise_page_fault);

  // Checks if a given offset is valid into the specified segment.
  template<uint32 size, AccessType access>
  bool CheckSegmentAccess(Segment segment, VirtualMemoryAddress offset, bool raise_gp_fault);

  // Linear memory reads/writes
  // These should only be used within instruction handlers, or jit code, as they raise exceptions.
  uint8 ReadMemoryByte(LinearMemoryAddress address);
  uint16 ReadMemoryWord(LinearMemoryAddress address);
  uint32 ReadMemoryDWord(LinearMemoryAddress address);
  void WriteMemoryByte(LinearMemoryAddress address, uint8 value);
  void WriteMemoryWord(LinearMemoryAddress address, uint16 value);
  void WriteMemoryDWord(LinearMemoryAddress address, uint32 value);

  // Reads/writes memory based on the specified segment and offset.
  // These should only be used within instruction handlers, or jit code, as they raise exceptions.
  uint8 ReadMemoryByte(Segment segment, VirtualMemoryAddress address);
  uint16 ReadMemoryWord(Segment segment, VirtualMemoryAddress address);
  uint32 ReadMemoryDWord(Segment segment, VirtualMemoryAddress address);
  void WriteMemoryByte(Segment segment, VirtualMemoryAddress address, uint8 value);
  void WriteMemoryWord(Segment segment, VirtualMemoryAddress address, uint16 value);
  void WriteMemoryDWord(Segment segment, VirtualMemoryAddress address, uint32 value);

  // Unchecked memory reads/writes (don't perform access checks, or raise exceptions).
  // Safe to use outside instruction handlers.
  bool SafeReadMemoryByte(LinearMemoryAddress address, uint8* value, bool access_check, bool raise_page_fault);
  bool SafeReadMemoryWord(LinearMemoryAddress address, uint16* value, bool access_check, bool raise_page_fault);
  bool SafeReadMemoryDWord(LinearMemoryAddress address, uint32* value, bool access_check, bool raise_page_fault);
  bool SafeWriteMemoryByte(VirtualMemoryAddress address, uint8 value, bool access_check, bool raise_page_fault);
  bool SafeWriteMemoryWord(VirtualMemoryAddress address, uint16 value, bool access_check, bool raise_page_fault);
  bool SafeWriteMemoryDWord(VirtualMemoryAddress address, uint32 value, bool access_check, bool raise_page_fault);

  // Prints the current state and instruction the CPU is sitting at.
  void PrintCurrentStateAndInstruction(const char* prefix_message = nullptr);

  // Component functions
  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;
  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;
  void SetIRQState(bool state) override;
  void SignalNMI() override;
  ::DebuggerInterface* GetDebuggerInterface() override;
  bool SupportsBackend(CPUBackendType mode) override;
  void SetBackend(CPUBackendType mode) override;

  // Executes instructions/cycles.
  void ExecuteCycles(CycleCount cycles) override;
  void StopExecution() override;

  // Code cache flushing - for recompiler backends
  void FlushCodeCache() override;

protected:
  // Actually creates the backend.
  void CreateBackend();

  // Reads privilege level
  uint8 GetCPL() const;

  // Checks for supervisor/user mode
  bool InSupervisorMode() const;
  bool InUserMode() const;
  bool IsPagingEnabled() const;

  // Reads i/o privilege level from flags
  uint16 GetIOPL() const;

  // Helper to check for real mode
  // Returns false for V8086 mode
  bool InRealMode() const;

  // Helper to check for protected mode
  bool InProtectedMode() const;

  // Helper to check for V8086 mode
  bool InVirtual8086Mode() const;

  // Full page translation - page table lookup.
  bool LookupPageTable(PhysicalMemoryAddress* out_physical_address, LinearMemoryAddress linear_address,
                       bool access_check, AccessType access_type, bool raise_page_fault);

  // Instruction fetching
  uint8 FetchInstructionByte();
  uint16 FetchInstructionWord();
  uint32 FetchInstructionDWord();

  // Direct instruction fetching (bypass the prefetch queue)
  uint8 FetchDirectInstructionByte(uint32 address, bool raise_exceptions);
  uint16 FetchDirectInstructionWord(uint32 address, bool raise_exceptions);
  uint32 FetchDirectInstructionDWord(uint32 address, bool raise_exceptions);

  // Push/pop from stack
  // This can cause both a page fault, as well as a stack fault, in which case
  // these functions will never return.
  void PushWord(uint16 value);
  void PushDWord(uint32 value);
  uint16 PopWord();
  uint32 PopDWord();

  // Sets flags from a value, masking away bits that can't be changed
  void SetFlags(uint32 value);
  void SetHalted(bool halt);
  void UpdateAlignmentCheckMask();
  void SetCPL(uint8 cpl);

  // Loads 80386+ control/debug/test registers
  void LoadSpecialRegister(Reg32 reg, uint32 value);

  // Reads a segment descriptor from memory, given a selector
  bool ReadDescriptorEntry(DESCRIPTOR_ENTRY* entry, const DescriptorTablePointer& table, uint32 index);
  bool ReadDescriptorEntry(DESCRIPTOR_ENTRY* entry, const SEGMENT_SELECTOR_VALUE& selector);

  // Updates a segment descriptor
  bool WriteDescriptorEntry(const DESCRIPTOR_ENTRY& entry, const DescriptorTablePointer& table, uint32 index);
  bool WriteDescriptorEntry(const DESCRIPTOR_ENTRY& entry, const SEGMENT_SELECTOR_VALUE& selector);

  // Checks code segment is valid for far jump/call/iret.
  bool CheckTargetCodeSegment(uint16 raw_selector, uint8 check_rpl, uint8 check_cpl, bool raise_exceptions);

  // Load descriptor tables.
  void LoadGlobalDescriptorTable(LinearMemoryAddress table_base_address, uint32 table_limit);
  void LoadInterruptDescriptorTable(LinearMemoryAddress table_base_address, uint32 table_limit);

  // Loads the visible portion of a segment register, updating the cached information
  void LoadSegmentRegister(Segment segment, uint16 value);
  void LoadLocalDescriptorTable(uint16 value);
  void LoadTaskSegment(uint16 value);

  // Clears inaccessible segment selectors, based on the current privilege level.
  void ClearInaccessibleSegmentSelectors();

  // Throws an exception, leaving IP containing the address of the current instruction
  void RaiseException(uint32 interrupt, uint32 error_code = 0);

  // Raises page fault exception.
  void RaisePageFault(LinearMemoryAddress linear_address, AccessType access_type, bool page_present);

  // Checking for external interrupts.
  bool HasExternalInterrupt() const;
  void DispatchExternalInterrupt();

  // Jump instructions
  void BranchTo(uint32 new_EIP);
  void BranchFromException(uint32 new_EIP);
  void AbortCurrentInstruction();
  void RestartCurrentInstruction();

  // Handle far calls/jumps
  void FarJump(uint16 segment_selector, uint32 offset, OperandSize operand_size);
  void FarCall(uint16 segment_selector, uint32 offset, OperandSize operand_size);
  void FarReturn(OperandSize operand_size, uint32 pop_byte_count);
  void InterruptReturn(OperandSize operand_size);

  // Change processor state to execute interrupt handler
  void SetupInterruptCall(uint32 interrupt, bool software_interrupt, bool push_error_code, uint32 error_code,
                          uint32 return_EIP);
  void SetupRealModeInterruptCall(uint32 interrupt, uint32 return_EIP);
  void SetupProtectedModeInterruptCall(uint32 interrupt, bool software_interrupt, bool push_error_code,
                                       uint32 error_code, uint32 return_EIP);

  // Check pending exceptions (WAIT instruction).
  void CheckFloatingPointException();

  // Store/load floating-point environment.
  void LoadFPUState(Segment seg, VirtualMemoryAddress addr, VirtualMemoryAddress addr_mask, bool is_32bit,
                    bool load_registers);
  void StoreFPUState(Segment seg, VirtualMemoryAddress addr, VirtualMemoryAddress addr_mask, bool is_32bit,
                     bool store_registers);

  // Update IR/B flags in status word when a new status word is loaded.
  void UpdateFPUSummaryException();

  // Handle CPUID instruction.
  void ExecuteCPUIDInstruction();

  // Switches to the task segment selected by the parameter.
  // Optionally sets nested task flag and backlink field.
  // This assumes that new_task is a valid descriptor and is not busy.
  void SwitchToTask(uint16 new_task, bool nested_task, bool from_iret, bool push_error_code, uint32 error_code);

  // Checks permissions for the specified IO port against the IO permission bitmap
  bool HasIOPermissions(uint32 port_number, uint32 port_count, bool raise_exceptions);

  // Temporary: TODO move to debugger interface
  void DumpPageTable();
  void DumpMemory(LinearMemoryAddress start_address, uint32 size);
  void DumpStack();

  // TLB emulation
  size_t GetTLBEntryIndex(uint32 linear_address);
  void InvalidateAllTLBEntries();
  void InvalidateTLBEntry(uint32 linear_address);

  // Prefetch queue emulation
  void FlushPrefetchQueue();
  bool FillPrefetchQueue();

  InterruptController* m_interrupt_controller = nullptr;
  std::unique_ptr<Backend> m_backend;
  std::unique_ptr<DebuggerInterface> m_debugger_interface;

  // Pending cycles, used for some jit backends.
  // Pending time is added at the start of the block, then committed at the next block execution.
  CycleCount m_pending_cycles = 0;
  CycleCount m_execution_downcount = 0;
  CycleCount m_tsc_cycles = 0;

  // CPU model that determines behavior.
  Model m_model = MODEL_386;

  // Two instruction pointers are kept, m_registers.IP contains the address of the next
  // instruction to be executed, and m_current_IP contains the address of the instruction
  // currently being executed. These are kept at the beginning of the class because the
  // jit addresses these, and this way the offset can be stored in a signed byte.
  uint32 m_current_EIP = 0;

  // Stores the value of ESP before the current instruction was executed.
  // Used to recover from half-executed instructions when exceptions are thrown.
  uint32 m_current_ESP = 0;

  // All guest-visible registers
  Registers m_registers = {};
  FPURegisters m_fpu_registers = {};

  // Current address and operand sizes (operating mode)
  AddressSize m_current_address_size = AddressSize_16;
  OperandSize m_current_operand_size = OperandSize_16;

  // Current stack address size
  AddressSize m_stack_address_size = AddressSize_16;

  // EIP mask - 0xFFFF for 16-bit code, 0xFFFFFFFF for 32-bit code.
  uint32 m_EIP_mask = 0xFFFF;

  // Locations of descriptor tables
  DescriptorTablePointer m_idt_location;
  DescriptorTablePointer m_gdt_location;
  DescriptorTablePointer m_ldt_location;
  TSSCache m_tss_location;

  // Descriptor caches for memory segments
  SegmentCache m_segment_cache[Segment_Count];

  // Current privilege level of executing code.
  uint8 m_cpl = 0;

  // Used to speed up TLB lookups, 0 - supervisor, 1 - user
  uint8 m_tlb_user_bit = 0;

  // Whether alignment checking is enabled (AM bit of CR0 and AC bit of EFLAGS).
  bool m_alignment_check_enabled = false;

  // Halt state, when an interrupt request or nmi comes in this is reset
  bool m_halted = false;

  // Non-maskable interrupt line
  bool m_nmi_state = false;

  // External interrupt request line
  bool m_irq_state = false;

  // State of the trap flag at the beginning of an instruction. Not saved to state.
  bool m_trap_after_instruction = false;

  // Exception currently being thrown. Interrupt_Count at all other times. Not saved to state.
  uint32 m_current_exception = Interrupt_Count;

  // Timing data.
  uint16 m_cycle_group_timings[NUM_CYCLE_GROUPS] = {};

#ifdef ENABLE_TLB_EMULATION
  struct TLBEntry
  {
    // Invalid TLB entries will be set to 0xFFFFFFFF, which has the lower-most
    // bits set, so it will never be confused for a real entry.
    LinearMemoryAddress linear_address;
    PhysicalMemoryAddress physical_address;
  };

  // Indexed by [user_supervisor][write_read]
  TLBEntry m_tlb_entries[2][3][TLB_ENTRY_COUNT] = {};
#endif

#ifdef ENABLE_PREFETCH_EMULATION
  byte m_prefetch_queue[PREFETCH_QUEUE_SIZE] = {};
  uint32 m_prefetch_queue_position = 0;
  uint32 m_prefetch_queue_size = 0;
#endif

  // Current execution state.
  VirtualMemoryAddress m_effective_address = 0;
  InstructionData idata = {};
};

template<uint32 size, AccessType access>
bool CPU_X86::CPU::CheckSegmentAccess(Segment segment, VirtualMemoryAddress offset, bool raise_gp_fault)
{
  const SegmentCache* segcache = &m_segment_cache[segment];
  DebugAssert(segment < Segment_Count && size > 0);

  // These read/write checks should be done at segment load time.
  // Non-CS segments should be data or code+readable
  // SS segments should be data+writable
  // Everything else should be code or writable
  // TODO: Add a flag for 4G segments so we can skip the limit check.

  // First we check if we have read/write/execute access.
  // Then check against the segment limit (can be expand up or down, but calculated at load time).
  if (((segcache->access_mask & static_cast<AccessTypeMask>(1 << static_cast<uint8>(access))) ==
       AccessTypeMask::None) ||
      (offset < segcache->limit_low) || ((offset + (size - 1)) > segcache->limit_high))
  {
    // For the SS selector we issue SF not GPF.
    if (raise_gp_fault)
      RaiseException((segment == Segment_SS) ? Interrupt_StackFault : Interrupt_GeneralProtectionFault, 0);

    return false;
  }

  return true;
}

} // namespace CPU_X86
