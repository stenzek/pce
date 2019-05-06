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

class CPU : public ::CPU
{
  DECLARE_OBJECT_TYPE_INFO(CPU, ::CPU);
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
  static constexpr u32 SERIALIZATION_ID = MakeSerializationID('C', 'P', 'U', 'C');
  static constexpr u32 PAGE_SIZE = 4096;
  static constexpr u32 PAGE_OFFSET_MASK = (PAGE_SIZE - 1);
  static constexpr u32 PAGE_MASK = ~PAGE_OFFSET_MASK;
  static constexpr u32 PAGE_SHIFT = 12;
  static constexpr size_t TLB_ENTRY_COUNT = 8192;

#pragma pack(push, 1)
  // Needed because the 8-bit register indices are all low bits -> all high bits
  struct Reg8Access
  {
    const u8& operator[](size_t index) const { return data[index % 4][index / 4]; }
    u8& operator[](size_t index) { return data[index % 4][index / 4]; }

    u8 data[Reg32_Count][4];
  };
  struct Reg16Access
  {
    const u16& operator[](size_t index) const { return data[index][0]; }
    u16& operator[](size_t index) { return data[index][0]; }

    u16 data[Reg32_Count][2];
  };

  template<u32 mask>
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

    u32 data;
  };
#pragma pack(pop)

  union Registers
  {
    struct
    {
      // Little-endian, so LSB first
      u8 AL;
      u8 AH;
      u8 __reserved8_0;
      u8 __reserved8_1;
      u8 CL;
      u8 CH;
      u8 __reserved8_2;
      u8 __reserved8_3;
      u8 DL;
      u8 DH;
      u8 __reserved8_4;
      u8 __reserved8_5;
      u8 BL;
      u8 BH;
      u8 __reserved8_6;
      u8 __reserved8_7;
    };

    struct
    {
      u16 AX;
      u16 __reserved16_0;
      u16 CX;
      u16 __reserved16_1;
      u16 DX;
      u16 __reserved16_2;
      u16 BX;
      u16 __reserved16_3;
      u16 SP;
      u16 __reserved16_4;
      u16 BP;
      u16 __reserved16_5;
      u16 SI;
      u16 __reserved16_6;
      u16 DI;
      u16 __reserved16_7;
      u16 IP;
      u16 __reserved16_8;
      u16 FLAGS;
      u16 __reserved16_9;
    };

    struct
    {
      u32 EAX;
      u32 ECX;
      u32 EDX;
      u32 EBX;
      u32 ESP;
      u32 EBP;
      u32 ESI;
      u32 EDI;

      u32 EIP;

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
        FlagAccess<Flag_VIF> VIF;
        FlagAccess<Flag_VIP> VIP;
        u32 bits;
      } EFLAGS;

      u32 CR0;
      u32 CR1;
      u32 CR2;
      u32 CR3;
      union
      {
        u32 bits;
        BitField<u32, bool, 0, 1> VME;
        BitField<u32, bool, 1, 1> PVI;
        BitField<u32, bool, 2, 1> TSD;
        BitField<u32, bool, 3, 1> DE;
      } CR4;

      u32 DR0;
      u32 DR1;
      u32 DR2;
      u32 DR3;
      u32 DR4;
      u32 DR5;
      u32 DR6;
      u32 DR7;

      u32 TR3;
      u32 TR4;
      u32 TR5;
      u32 TR6;
      u32 TR7;

      union
      {
        struct
        {
          u16 ES;
          u16 CS;
          u16 SS;
          u16 DS;
          u16 FS;
          u16 GS;
        };
        u16 segment_selectors[Segment_Count];
      };

      // uint48 GDTR;
      // uint48 IDTR;
      u16 LDTR;
      u16 TR;
    };

    Reg8Access reg8;
    Reg16Access reg16;
    u32 reg32[Reg32_Count];
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
    u32 ESP;
    u32 base_address;
    u32 limit_low;
    u32 limit_high;
    AddressSize address_size;
    u16 SS;

    TemporaryStack(CPU* cpu_, u32 ESP_, u16 SS_, u32 base_address_, u32 limit_low_, u32 limit_high_,
                   AddressSize address_size_);
    TemporaryStack(CPU* cpu_, u32 ESP_, u16 SS_, const DESCRIPTOR_ENTRY& dentry);
    TemporaryStack(CPU* cpu_, u32 ESP_, u16 SS_);

    bool CanPushBytes(u32 num_bytes) const;
    bool CanPushWords(u32 num_words) const;
    bool CanPushDWords(u32 num_dwords) const;

    void PushWord(u16 value);
    void PushDWord(u32 value);
    u16 PopWord();
    u32 PopDWord();

    void SwitchTo();
  };

  // Interrupt hook callback
  using InterruptHookCallback = std::function<bool(u32 interrupt, Registers* registers)>;

  CPU(const String& identifier, Model model, float frequency, BackendType backend_type = BackendType::Interpreter,
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
                              AccessFlags flags);

  // Checks if a given offset is valid into the specified segment.
  template<u32 size, AccessType access>
  bool CheckSegmentAccess(Segment segment, VirtualMemoryAddress offset, bool raise_gp_fault);

  // Linear memory reads/writes
  // These should only be used within instruction handlers, or jit code, as they raise exceptions.
  u8 ReadMemoryByte(LinearMemoryAddress address);
  u16 ReadMemoryWord(LinearMemoryAddress address);
  u32 ReadMemoryDWord(LinearMemoryAddress address);
  void WriteMemoryByte(LinearMemoryAddress address, u8 value);
  void WriteMemoryWord(LinearMemoryAddress address, u16 value);
  void WriteMemoryDWord(LinearMemoryAddress address, u32 value);

  // Reads/writes memory based on the specified segment and offset.
  // These should only be used within instruction handlers, or jit code, as they raise exceptions.
  u8 ReadMemoryByte(Segment segment, VirtualMemoryAddress address);
  u16 ReadMemoryWord(Segment segment, VirtualMemoryAddress address);
  u32 ReadMemoryDWord(Segment segment, VirtualMemoryAddress address);
  void WriteMemoryByte(Segment segment, VirtualMemoryAddress address, u8 value);
  void WriteMemoryWord(Segment segment, VirtualMemoryAddress address, u16 value);
  void WriteMemoryDWord(Segment segment, VirtualMemoryAddress address, u32 value);

  // Unchecked memory reads/writes (don't perform access checks, or raise exceptions).
  // Safe to use outside instruction handlers.
  bool SafeReadMemoryByte(LinearMemoryAddress address, u8* value, AccessFlags access_flags);
  bool SafeReadMemoryWord(LinearMemoryAddress address, u16* value, AccessFlags access_flags);
  bool SafeReadMemoryDWord(LinearMemoryAddress address, u32* value, AccessFlags access_flags);
  bool SafeWriteMemoryByte(VirtualMemoryAddress address, u8 value, AccessFlags access_flags);
  bool SafeWriteMemoryWord(VirtualMemoryAddress address, u16 value, AccessFlags access_flags);
  bool SafeWriteMemoryDWord(VirtualMemoryAddress address, u32 value, AccessFlags access_flags);

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
  bool SupportsBackend(BackendType mode) override;
  void SetBackend(BackendType mode) override;

  // Executes instructions/cycles.
  void ExecuteSlice(SimulationTime time) override;
  void StallExecution(SimulationTime time) override;
  void StopExecution() override;

  // Code cache flushing - for recompiler backends
  void FlushCodeCache() override;

protected:
  // Actually creates the backend.
  void CreateBackend();

  // Reads privilege level
  u8 GetCPL() const;

  // Checks for supervisor/user mode
  bool InSupervisorMode() const;
  bool InUserMode() const;
  bool IsPagingEnabled() const;

  // Reads i/o privilege level from flags
  u16 GetIOPL() const;

  // Helper to check for real mode
  // Returns false for V8086 mode
  bool InRealMode() const;

  // Helper to check for protected mode
  bool InProtectedMode() const;

  // Helper to check for V8086 mode
  bool InVirtual8086Mode() const;

  // Full page translation - page table lookup.
  bool LookupPageTable(PhysicalMemoryAddress* out_physical_address, LinearMemoryAddress linear_address,
                       AccessFlags flags);

  // Instruction fetching
  u8 FetchInstructionByte();
  u16 FetchInstructionWord();
  u32 FetchInstructionDWord();

  // Direct instruction fetching (bypass the prefetch queue)
  u8 FetchDirectInstructionByte(u32 address);
  u16 FetchDirectInstructionWord(u32 address);
  u32 FetchDirectInstructionDWord(u32 address);

  // Push/pop from stack
  // This can cause both a page fault, as well as a stack fault, in which case
  // these functions will never return.
  void PushWord(u16 value);
  void PushWord32(u16 value);
  void PushDWord(u32 value);
  u16 PopWord();
  u32 PopDWord();

  // Sets flags from a value, masking away bits that can't be changed
  void SetFlags(u32 value);
  void SetHalted(bool halt);
  void UpdateAlignmentCheckMask();
  void SetCPL(u8 cpl);

  // Loads 80386+ control/debug/test registers
  void LoadSpecialRegister(Reg32 reg, u32 value);

  // Reads a segment descriptor from memory, given a selector
  bool ReadDescriptorEntry(DESCRIPTOR_ENTRY* entry, const DescriptorTablePointer& table, u32 index);
  bool ReadDescriptorEntry(DESCRIPTOR_ENTRY* entry, const SEGMENT_SELECTOR_VALUE& selector);

  // Updates a segment descriptor
  bool WriteDescriptorEntry(const DESCRIPTOR_ENTRY& entry, const DescriptorTablePointer& table, u32 index);
  bool WriteDescriptorEntry(const DESCRIPTOR_ENTRY& entry, const SEGMENT_SELECTOR_VALUE& selector);

  // Updates the accessed bit in a descriptor.
  void SetDescriptorAccessedBit(DESCRIPTOR_ENTRY& entry, const DescriptorTablePointer& table, u32 index);
  void SetDescriptorAccessedBit(DESCRIPTOR_ENTRY& entry, const SEGMENT_SELECTOR_VALUE& selector);

  // Checks code segment is valid for far jump/call/iret.
  bool CheckTargetCodeSegment(u16 raw_selector, u8 check_rpl, u8 check_cpl, bool raise_exceptions);

  // Load descriptor tables.
  void LoadGlobalDescriptorTable(LinearMemoryAddress table_base_address, u32 table_limit);
  void LoadInterruptDescriptorTable(LinearMemoryAddress table_base_address, u32 table_limit);

  // Loads the visible portion of a segment register, updating the cached information
  void LoadSegmentRegister(Segment segment, u16 value);
  void LoadLocalDescriptorTable(u16 value);
  void LoadTaskSegment(u16 value);

  // Clears inaccessible segment selectors, based on the current privilege level.
  void ClearInaccessibleSegmentSelectors();

  // Throws an exception, leaving IP containing the address of the current instruction
  void RaiseException(u32 interrupt, u32 error_code = 0);

  // Raises a software interrupt (INT3, INTO, BOUND). Does not abort the current instruction.
  // EIP is set to the next instruction, not the excepting instruction. ESP is not reset.
  void RaiseSoftwareException(u32 interrupt);

  // Debugger exceptions, whether from the INT1 instruction, or from debug exceptions should
  // not be classed as software exceptions for the purpose of the EXT bit, or descriptor checks.
  void RaiseDebugException();

  // Raises page fault exception.
  void RaisePageFault(LinearMemoryAddress linear_address, AccessFlags flags, bool page_present);

  // Raises a software interrupt (INT n). Does not abort the current instruction, EIP is set to
  // the next instruction, and ESP is not reset. VME-style interrupts are checked and invoked.
  void SoftwareInterrupt(u8 interrupt);

  // Checking for external interrupts.
  bool HasExternalInterrupt() const;
  void DispatchExternalInterrupt();

  // Jump instructions
  void BranchTo(u32 new_EIP);
  void BranchFromException(u32 new_EIP);
  void AbortCurrentInstruction();
  void RestartCurrentInstruction();

  // Handle far calls/jumps
  void FarJump(u16 segment_selector, u32 offset, OperandSize operand_size);
  void FarCall(u16 segment_selector, u32 offset, OperandSize operand_size);
  void FarReturn(OperandSize operand_size, u32 pop_byte_count);
  void InterruptReturn(OperandSize operand_size);

  // Change processor state to execute interrupt handler
  void SetupInterruptCall(u32 interrupt, bool software_interrupt, bool push_error_code, u32 error_code, u32 return_EIP);
  void SetupRealModeInterruptCall(u32 interrupt, u32 return_EIP);
  void SetupV86ModeInterruptCall(u8 interrupt, u32 return_EIP);
  void SetupProtectedModeInterruptCall(u32 interrupt, bool software_interrupt, bool push_error_code, u32 error_code,
                                       u32 return_EIP);

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
  void SwitchToTask(u16 new_task, bool nested_task, bool from_iret, bool push_error_code, u32 error_code);

  // Checks permissions for the specified IO port against the IO permission bitmap
  bool HasIOPermissions(u32 port_number, u32 port_count, bool raise_exceptions);

  // Checks permissions for the specified interrupt against the interrupt bitmap in enhanced V8086 mode.
  bool IsVMEInterruptBitSet(u8 port_number);

  // Temporary: TODO move to debugger interface
  void DumpPageTable();
  void DumpMemory(LinearMemoryAddress start_address, u32 size);
  void DumpStack();

  // TLB emulation
  size_t GetTLBEntryIndex(u32 linear_address);
  void InvalidateAllTLBEntries(bool force_clear = false);
  void InvalidateTLBEntry(u32 linear_address);

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
  u32 m_current_EIP = 0;

  // Stores the value of ESP before the current instruction was executed.
  // Used to recover from half-executed instructions when exceptions are thrown.
  u32 m_current_ESP = 0;

  // All guest-visible registers
  Registers m_registers = {};
  FPURegisters m_fpu_registers = {};

  // Current address and operand sizes (operating mode)
  AddressSize m_current_address_size = AddressSize_16;
  OperandSize m_current_operand_size = OperandSize_16;

  // Current stack address size
  AddressSize m_stack_address_size = AddressSize_16;

  // EIP mask - 0xFFFF for 16-bit code, 0xFFFFFFFF for 32-bit code.
  u32 m_EIP_mask = 0xFFFF;

  // Locations of descriptor tables
  DescriptorTablePointer m_idt_location;
  DescriptorTablePointer m_gdt_location;
  DescriptorTablePointer m_ldt_location;
  TSSCache m_tss_location;

  // Descriptor caches for memory segments
  SegmentCache m_segment_cache[Segment_Count];

  // Current privilege level of executing code.
  u8 m_cpl = 0;

  // Used to speed up TLB lookups, 0 - supervisor, 1 - user
  u8 m_tlb_user_bit = 0;

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
  u32 m_current_exception = Interrupt_Count;

  // Timing data.
  u16 m_cycle_group_timings[NUM_CYCLE_GROUPS] = {};

#ifdef ENABLE_TLB_EMULATION
  // We use the lower 12 bits to represent a "counter" which is incremented each
  // time the TLB is flushed. This way, we don't need to wipe out the array every
  // flush, instead only when the counter wraps around. We still have 12 bits free
  // in the physical adress if we need to store anything else.
  struct TLBEntry
  {
    // Invalid TLB entries will be set to 0xFFFFFFFF, which has the lower-most
    // bits set, so it will never be confused for a real entry.
    LinearMemoryAddress linear_address;
    PhysicalMemoryAddress physical_address;
  };

  // Indexed by [user_supervisor][write_read]
  TLBEntry m_tlb_entries[2][3][TLB_ENTRY_COUNT] = {};
  u32 m_tlb_counter_bits = 0;
#endif

#ifdef ENABLE_PREFETCH_EMULATION
  byte m_prefetch_queue[PREFETCH_QUEUE_SIZE] = {};
  u32 m_prefetch_queue_position = 0;
  u32 m_prefetch_queue_size = 0;
#endif

  // Current execution state.
  VirtualMemoryAddress m_effective_address = 0;
  InstructionData idata = {};
};

template<u32 size, AccessType access>
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
  // This computation can overflow (e.g. address FFFFFFFF + size 4), so use 64-bit for the upper bounds.
  if (((segcache->access_mask & static_cast<AccessTypeMask>(1 << static_cast<u8>(access))) == AccessTypeMask::None) ||
      (offset < segcache->limit_low) ||
      ((static_cast<u64>(offset) + static_cast<u64>(size - 1)) > static_cast<u64>(segcache->limit_high)))
  {
    // For the SS selector we issue SF not GPF.
    if (raise_gp_fault)
      RaiseException((segment == Segment_SS) ? Interrupt_StackFault : Interrupt_GeneralProtectionFault, 0);

    return false;
  }

  return true;
}

} // namespace CPU_X86
