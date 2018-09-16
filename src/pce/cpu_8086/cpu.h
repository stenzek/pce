#pragma once

#include "common/clock.h"
#include "common/fastjmp.h"
#include "pce/cpu.h"
#include "pce/cpu_8086/types.h"
#include <functional>
#include <memory>

//#define ENABLE_PREFETCH_EMULATION 1
#define PREFETCH_QUEUE_SIZE 32

class DebuggerInterface;
class InterruptController;

namespace CPU_8086 {

class Backend;
class DebuggerInterface;
class Instructions;

class CPU : public CPUBase
{
  DECLARE_OBJECT_TYPE_INFO(CPU, CPUBase);
  DECLARE_OBJECT_NO_FACTORY(CPU);
  DECLARE_OBJECT_PROPERTY_MAP(CPU);

  friend DebuggerInterface;
  friend Instructions;

public:
  static const uint32 SERIALIZATION_ID = MakeSerializationID('8', '0', '8', '6');

#pragma pack(push, 1)
  // Needed because the 8-bit register indices are all low bits -> all high bits
  struct Reg8Access
  {
    const u8& operator[](size_t index) const { return data[index % 4][index / 4]; }
    u8& operator[](size_t index) { return data[index % 4][index / 4]; }

    u8 data[Reg16_Count][2];
  };

  template<u16 mask>
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

    u16 data;
  };
#pragma pack(pop)

  union Registers
  {
    struct
    {
      // Little-endian, so LSB first
      uint8 AL;
      uint8 AH;
      uint8 CL;
      uint8 CH;
      uint8 DL;
      uint8 DH;
      uint8 BL;
      uint8 BH;
    };

    struct
    {
      uint16 AX;
      uint16 CX;
      uint16 DX;
      uint16 BX;
      uint16 SP;
      uint16 BP;
      uint16 SI;
      uint16 DI;
      uint16 IP;
      union
      {
        u16 bits;
        FlagAccess<Flag_CF> CF;
        FlagAccess<Flag_PF> PF;
        FlagAccess<Flag_AF> AF;
        FlagAccess<Flag_ZF> ZF;
        FlagAccess<Flag_SF> SF;
        FlagAccess<Flag_TF> TF;
        FlagAccess<Flag_IF> IF;
        FlagAccess<Flag_DF> DF;
        FlagAccess<Flag_OF> OF;
      } FLAGS;

      union
      {
        struct
        {
          uint16 ES;
          uint16 CS;
          uint16 SS;
          uint16 DS;
        };
        uint16 segment_selectors[Segment_Count];
      };
    };

    Reg8Access reg8;
    uint16 reg16[Reg16_Count];
  };

  CPU(const String& identifier, Model model, float frequency, const ObjectTypeInfo* type_info = &s_type_info);
  ~CPU();

  const char* GetModelString() const;

  const Registers* GetRegisters() const { return &m_registers; }
  Registers* GetRegisters() { return &m_registers; }
  bool IsHalted() const { return m_halted; }

  // Cycle tracking when executing.
  void AddCycle() { m_pending_cycles++; }
  void AddCycles(CycleCount cycles) { m_pending_cycles += cycles; }
  void AddMemoryCycle() { /*m_pending_cycles++;*/}
  void CommitPendingCycles();

  // Calculates the physical address of memory with the specified segment and offset.
  // If code is set, it is assumed to be reading instructions, otherwise data.
  PhysicalMemoryAddress CalculateLinearAddress(Segment segment, VirtualMemoryAddress offset);

  // Linear memory reads/writes
  u8 ReadMemoryByte(LinearMemoryAddress address);
  u16 ReadMemoryWord(LinearMemoryAddress address);
  void WriteMemoryByte(LinearMemoryAddress address, u8 value);
  void WriteMemoryWord(LinearMemoryAddress address, u16 value);

  // Reads/writes memory based on the specified segment and offset.
  u8 ReadMemoryByte(Segment segment, VirtualMemoryAddress address);
  u16 ReadMemoryWord(Segment segment, VirtualMemoryAddress address);
  void WriteMemoryByte(Segment segment, VirtualMemoryAddress address, u8 value);
  void WriteMemoryWord(Segment segment, VirtualMemoryAddress address, u16 value);

  // Unchecked memory reads/writes (don't perform access checks, or raise exceptions).
  // Safe to use outside instruction handlers.
  bool SafeReadMemoryByte(LinearMemoryAddress address, u8* value);
  bool SafeReadMemoryWord(LinearMemoryAddress address, u16* value);
  bool SafeWriteMemoryByte(LinearMemoryAddress address, u8 value);
  bool SafeWriteMemoryWord(LinearMemoryAddress address, u16 value);

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

protected:
  // Instruction fetching
  u8 FetchInstructionByte();
  u16 FetchInstructionWord();

  // Direct instruction fetching (bypass the prefetch queue)
  u8 FetchDirectInstructionByte(u16 address);
  u16 FetchDirectInstructionWord(u16 address);

  // Push/pop from stack
  void PushWord(u16 value);
  u16 PopWord();

  // Sets flags from a value, masking away bits that can't be changed
  void SetFlags(u16 value);
  void SetHalted(bool halt);

  // Throws an exception, leaving IP containing the address of the current instruction
  void RaiseException(uint32 interrupt);

  // Checking for external interrupts.
  bool HasExternalInterrupt() const;
  void DispatchExternalInterrupt();

  // Instruction execution.
  void ExecuteInstruction();

  // Jump instructions
  void BranchTo(u16 new_IP);
  void BranchTo(u16 new_CS, u16 new_IP);
  void AbortCurrentInstruction();
  void RestartCurrentInstruction();

  // Change processor state to execute interrupt handler
  void SetupInterruptCall(u32 interrupt, u16 return_IP);

  // Prefetch queue emulation
  void FlushPrefetchQueue();
  bool FillPrefetchQueue();

  InterruptController* m_interrupt_controller = nullptr;
  std::unique_ptr<DebuggerInterface> m_debugger_interface;

  // Pending cycles, used for some jit backends.
  // Pending time is added at the start of the block, then committed at the next block execution.
  CycleCount m_pending_cycles = 0;
  CycleCount m_execution_downcount = 0;

  // CPU model that determines behavior.
  Model m_model = MODEL_8086;
  bool m_data_bus_is_8bit = false;

  // Two instruction pointers are kept, m_registers.IP contains the address of the next
  // instruction to be executed, and m_current_IP contains the address of the instruction
  // currently being executed. These are kept at the beginning of the class because the
  // jit addresses these, and this way the offset can be stored in a signed byte.
  u16 m_current_IP = 0;

  // All guest-visible registers
  Registers m_registers = {};

  // Halt state, when an interrupt request or nmi comes in this is reset
  bool m_halted = false;

  // Non-maskable interrupt line
  bool m_nmi_state = false;

  // External interrupt request line
  bool m_irq_state = false;

#ifdef ENABLE_PREFETCH_EMULATION
  byte m_prefetch_queue[PREFETCH_QUEUE_SIZE] = {};
  u32 m_prefetch_queue_position = 0;
  u32 m_prefetch_queue_size = 0;
#endif

  // Current execution state.
  VirtualMemoryAddress m_effective_address = 0;
  InstructionData idata = {};

#ifdef Y_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4324)
  fastjmp_buf m_jmp_buf = {};
#pragma warning(pop)
#endif
};

} // namespace CPU_8086
