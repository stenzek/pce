#pragma once

#include <atomic>
#include <cstring>
#include <functional>
#include <unordered_map>

#include "YBaseLib/Barrier.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/TaskQueue.h"
#include "YBaseLib/Timer.h"

#include "common/object.h"
#include "pce/mmio.h"
#include "pce/types.h"

class StateWrapper;
class TimingManager;

class Bus : public Object
{
  DECLARE_OBJECT_TYPE_INFO(Bus, Object);
  DECLARE_OBJECT_NO_FACTORY(Bus);
  DECLARE_OBJECT_NO_PROPERTIES(Bus);

public:
  using CodeHashType = u64;
  using CodeInvalidateCallback = std::function<void(PhysicalMemoryAddress)>;

  static constexpr u32 MEMORY_PAGE_SIZE = 0x1000; // 4KiB
  static constexpr u32 MEMORY_PAGE_OFFSET_MASK = PhysicalMemoryAddress(MEMORY_PAGE_SIZE - 1);
  static constexpr u32 MEMORY_PAGE_MASK = ~MEMORY_PAGE_OFFSET_MASK;
  static constexpr u32 NUM_IOPORTS = 0x10000;

  Bus(u32 memory_address_bits, const ObjectTypeInfo* type_info = &s_type_info);
  ~Bus();

  virtual bool Initialize(System* system);
  virtual void Reset();
  virtual bool DoState(StateWrapper& sw);

  PhysicalMemoryAddress GetMemoryAddressMask() const { return m_physical_memory_address_mask; }
  void SetMemoryAddressMask(PhysicalMemoryAddress mask) { m_physical_memory_address_mask = mask; }
  u32 GetMemoryPageCount() const { return m_num_physical_memory_pages; }
  u32 GetUnassignedRAMSize() const { return m_ram_size - m_ram_assigned; }

  // Obtained by walking the memory page table. end_page is not included in the count.
  PhysicalMemoryAddress GetTotalRAMInPageRange(u32 start_page, u32 end_page) const;

  void AllocateRAM(u32 size);

  // Returns the amount of RAM allocated to this region.
  // Start and end have to be page-aligned.
  u32 CreateRAMRegion(PhysicalMemoryAddress start, PhysicalMemoryAddress end);

  // Creates a MMIO ROM region from an external file.
  bool CreateROMRegionFromFile(const char* filename, u32 file_offset, PhysicalMemoryAddress address,
                               u32 expected_size = 0);

  // Creates a MMIO ROM region from a buffer.
  bool CreateROMRegionFromBuffer(const void* buffer, u32 size, PhysicalMemoryAddress address);

  // Creates a mirror of RAM/ROM.
  void MirrorRegion(PhysicalMemoryAddress start, u32 size, PhysicalMemoryAddress mirror_start);

  // IO port read/write callbacks
  using IOPortReadByteHandler = std::function<u8(u16 port)>;
  using IOPortReadWordHandler = std::function<u16(u16 port)>;
  using IOPortReadDWordHandler = std::function<u32(u16 port)>;
  using IOPortWriteByteHandler = std::function<void(u16 port, u8 value)>;
  using IOPortWriteWordHandler = std::function<void(u16 port, u16 value)>;
  using IOPortWriteDWordHandler = std::function<void(u16 port, u32 value)>;

  // IO port connections
  void ConnectIOPortRead(u16 port, const void* owner, IOPortReadByteHandler read_callback);
  void ConnectIOPortWrite(u16 port, const void* owner, IOPortWriteByteHandler write_callback);
  void DisconnectIOPort(u16 port, const void* owner);
  void DisconnectIOPorts(const void* owner);

  // Multi-byte IO reads/writes
  // If a port is not configured for multi-byte IO, the ports following it will be used instead.
  void ConnectIOPortReadWord(u16 port, const void* owner, IOPortReadWordHandler read_callback);
  void ConnectIOPortReadDWord(u16 port, const void* owner, IOPortReadDWordHandler read_callback);
  void ConnectIOPortWriteWord(u16 port, const void* owner, IOPortWriteWordHandler write_callback);
  void ConnectIOPortWriteDWord(u16 port, const void* owner, IOPortWriteDWordHandler write_callback);

  // Connecting an IO port to a single variable
  void ConnectIOPortReadToPointer(u16 port, const void* owner, const u8* var);
  void ConnectIOPortWriteToPointer(u16 port, const void* owner, u8* var);

  // IO port handler accessors (mainly for CPU)
  u8 ReadIOPortByte(u16 port);
  u16 ReadIOPortWord(u16 port);
  u32 ReadIOPortDWord(u16 port);
  void WriteIOPortByte(u16 port, u8 value);
  void WriteIOPortWord(u16 port, u16 value);
  void WriteIOPortDWord(u16 port, u32 value);

  // Reads/writes memory. Words must be within the same 4KiB page.
  // Reads of unmapped memory return -1.
  template<typename T>
  T ReadMemoryTyped(PhysicalMemoryAddress address);
  template<typename T>
  void WriteMemoryTyped(PhysicalMemoryAddress address, T value);

  // Reading/writing page-aligned memory.
  u8 ReadMemoryByte(PhysicalMemoryAddress address) { return ReadMemoryTyped<u8>(address); }
  void WriteMemoryByte(PhysicalMemoryAddress address, u8 value) { return WriteMemoryTyped<u8>(address, value); }
  u16 ReadMemoryWord(PhysicalMemoryAddress address) { return ReadMemoryTyped<u16>(address); }
  void WriteMemoryWord(PhysicalMemoryAddress address, u16 value) { return WriteMemoryTyped<u16>(address, value); }
  u32 ReadMemoryDWord(PhysicalMemoryAddress address) { return ReadMemoryTyped<u32>(address); }
  void WriteMemoryDWord(PhysicalMemoryAddress address, u32 value) { return WriteMemoryTyped<u32>(address, value); }
  u64 ReadMemoryQWord(PhysicalMemoryAddress address) { return ReadMemoryTyped<u64>(address); }
  void WriteMemoryQWord(PhysicalMemoryAddress address, u64 value) { return WriteMemoryTyped<u64>(address, value); }

  // Testing for readable/writable addresses.
  bool IsReadableAddress(PhysicalMemoryAddress address, u32 size) const;
  bool IsWritableAddress(PhysicalMemoryAddress address, u32 size) const;

  // Unaligned memory access is fine, can crosses a physical page.
  // Checked memory access, where bus errors and such are required.
  // In this case, we need to split it up into multiple words/bytes.
  bool CheckedReadMemoryByte(PhysicalMemoryAddress address, u8* value);
  bool CheckedWriteMemoryByte(PhysicalMemoryAddress address, u8 value);
  bool CheckedReadMemoryWord(PhysicalMemoryAddress address, u16* value);
  bool CheckedWriteMemoryWord(PhysicalMemoryAddress address, u16 value);
  bool CheckedReadMemoryDWord(PhysicalMemoryAddress address, u32* value);
  bool CheckedWriteMemoryDWord(PhysicalMemoryAddress address, u32 value);
  bool CheckedReadMemoryQWord(PhysicalMemoryAddress address, u64* value);
  bool CheckedWriteMemoryQWord(PhysicalMemoryAddress address, u64 value);

  // Read/write block of memory.
  void ReadMemoryBlock(PhysicalMemoryAddress address, u32 length, void* destination);
  void WriteMemoryBlock(PhysicalMemoryAddress address, u32 length, const void* source);

  // Get pointer to memory. Must lie within the same 64KiB page and be RAM not MMIO.
  byte* GetRAMPointer(PhysicalMemoryAddress address);

  // MMIO handlers
  void ConnectMMIO(MMIO* mmio);
  void DisconnectMMIO(MMIO* mmio);

  // Checks if a page contains only RAM.
  // RAM-only pages can be cached, with changes detected through the bitmask.
  bool IsCachablePage(PhysicalMemoryAddress address) const;
  bool IsWritablePage(PhysicalMemoryAddress address) const;

  // Hashes a block of code for use in backend code caches.
  CodeHashType GetCodeHash(PhysicalMemoryAddress address, u32 length);
  void MarkPageAsCode(PhysicalMemoryAddress address);
  void UnmarkPageAsCode(PhysicalMemoryAddress address);
  void ClearPageCodeFlags();

  // Code invalidate callback - executed when pages marked as code are modified.
  void SetCodeInvalidationCallback(CodeInvalidateCallback callback);
  void ClearCodeInvalidationCallback();

  // Change page types.
  void SetPageRAMState(PhysicalMemoryAddress page_address, bool readable_memory, bool writable_memory);
  void SetPagesRAMState(PhysicalMemoryAddress start_address, u32 size, bool readable_memory, bool writable_memory);

  // Hold the bus, stalling the main CPU for the specified amount of time.
  void Stall(SimulationTime time);

protected:
  struct PhysicalMemoryPage
  {
    enum Type : u8
    {
      kReadableRAM = 1,
      kWritableRAM = 2,
      kCachedCode = 4,
      kMirror = 8,
    };

    byte* ram_ptr;
    MMIO* mmio_handler;
    u8 type;

    bool IsReadableRAM() const { return (type & kReadableRAM) != 0; }
    bool IsWritableRAM() const { return (type & kWritableRAM) != 0; }
    bool HasCachedCode() const { return (type & kCachedCode) != 0; }
    bool IsMirror() const { return (type & kMirror) != 0; }
    bool IsMMIO() const { return (mmio_handler != nullptr); }
    bool IsReadableMMIO() const { return IsMMIO() && !IsReadableRAM(); }
    bool IsWritableMMIO() const { return IsMMIO() && !IsWritableRAM(); }
  };

  struct IOPortConnection
  {
    const void* owner;
    IOPortConnection* next;
    IOPortReadByteHandler read_byte_handler;
    IOPortReadWordHandler read_word_handler;
    IOPortReadDWordHandler read_dword_handler;
    IOPortWriteByteHandler write_byte_handler;
    IOPortWriteWordHandler write_word_handler;
    IOPortWriteDWordHandler write_dword_handler;
  };

  void AllocateMemoryPages(u32 memory_address_bits);

  template<typename T>
  void EnumeratePagesForRange(PhysicalMemoryAddress start_address, PhysicalMemoryAddress end_address, T callback);
  static bool IsCachablePage(const PhysicalMemoryPage& page);
  static bool IsWritablePage(const PhysicalMemoryPage& page);

  // Generic memory read/write handler
  template<typename T, bool aligned>
  bool ReadMemoryT(PhysicalMemoryAddress address, T* value);
  template<typename T, bool aligned>
  bool WriteMemoryT(PhysicalMemoryAddress address, T value);

  // Memory breakpoints
  void CheckForMemoryBreakpoint(PhysicalMemoryAddress address, u32 size, bool is_write, u32 value);

  // Obtain IO port connection for owner.
  IOPortConnection* GetIOPortConnection(u16 port, const void* owner);
  IOPortConnection* CreateIOPortConnection(u16 port, const void* owner);
  void RemoveIOPortConnection(u16 port, const void* owner);

  System* m_system = nullptr;

  // IO ports
  IOPortConnection** m_ioport_handlers = nullptr;
  std::unordered_map<const void*, std::vector<u16>> m_ioport_owners;

  // System memory map
  PhysicalMemoryPage* m_physical_memory_pages = nullptr;
  u32 m_num_physical_memory_pages = 0;

  // Physical address mask, by default this is set to the maximum address
  PhysicalMemoryAddress m_physical_memory_address_mask = ~0u;

  // Code invalidate callback - executed when pages marked as code are modified.
  CodeInvalidateCallback m_code_invalidate_callback;

  // Amount of RAM allocated overall
  // Do not access this pointer directly
  byte* m_ram_ptr = nullptr;
  u32 m_ram_size = 0;
  u32 m_ram_assigned = 0;

  // List of ROM regions allocated.
  // This does not include mirrors.
  struct ROMRegion
  {
    std::unique_ptr<byte[]> data;
    MMIO* mmio_handler;
    PhysicalMemoryAddress mapped_address;
    u32 size;
  };
  std::vector<ROMRegion> m_rom_regions;
};

#include "pce/bus.inl"
