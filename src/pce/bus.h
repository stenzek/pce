#pragma once

#include <atomic>
#include <cstring>
#include <functional>
#include <unordered_map>

#include "YBaseLib/Barrier.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Event.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/TaskQueue.h"
#include "YBaseLib/Timer.h"

#include "pce/component.h"
#include "pce/mmio.h"
#include "pce/types.h"

class ByteStream;
class BinaryReader;
class BinaryWriter;

class Bus : public Component
{
public:
  using CodeHashType = uint64;
  using CodeInvalidateCallback = std::function<void(PhysicalMemoryAddress)>;

  static const uint32 SERIALIZATION_ID = Component::MakeSerializationID('B', 'U', 'S');
  static const uint32 MEMORY_PAGE_SIZE = 0x1000; // 4KiB
  static const uint32 MEMORY_PAGE_OFFSET_MASK = PhysicalMemoryAddress(MEMORY_PAGE_SIZE - 1);
  static const uint32 MEMORY_PAGE_MASK = ~MEMORY_PAGE_OFFSET_MASK;

  Bus(uint32 memory_address_bits);
  ~Bus();

  bool Initialize(System* system, Bus* bus) override;
  void Reset() override;

  bool LoadState(BinaryReader& reader) override;
  bool SaveState(BinaryWriter& writer) override;

  PhysicalMemoryAddress GetMemoryAddressMask() const { return m_physical_memory_address_mask; }
  void SetMemoryAddressMask(PhysicalMemoryAddress mask) { m_physical_memory_address_mask = mask; }
  uint32 GetMemoryPageCount() const { return m_num_physical_memory_pages; }
  uint32 GetUnassignedRAMSize() const { return m_ram_size - m_ram_assigned; }

  // Obtained by walking the memory page table. end_page is not included in the count.
  PhysicalMemoryAddress GetTotalRAMInPageRange(uint32 start_page, uint32 end_page) const;

  void AllocateRAM(uint32 size);

  // Returns the amount of RAM allocated to this region.
  // Start and end have to be page-aligned.
  uint32 CreateRAMRegion(PhysicalMemoryAddress start, PhysicalMemoryAddress end);

  // Creates a MMIO ROM region from an external file.
  bool CreateROMRegionFromFile(const char* filename, PhysicalMemoryAddress address, uint32 expected_size = 0);

  // Creates a MMIO ROM region from a buffer.
  bool CreateROMRegionFromBuffer(const void* buffer, uint32 size, PhysicalMemoryAddress address);

  // Creates a mirror of RAM/ROM.
  void MirrorRegion(PhysicalMemoryAddress start, uint32 size, PhysicalMemoryAddress mirror_start);

  // IO port read/write callbacks
  using IOPortReadByteHandler = std::function<void(uint32 port, uint8* value)>;
  using IOPortReadWordHandler = std::function<void(uint32 port, uint16* value)>;
  using IOPortReadDWordHandler = std::function<void(uint32 port, uint32* value)>;
  using IOPortWriteByteHandler = std::function<void(uint32 port, uint8 value)>;
  using IOPortWriteWordHandler = std::function<void(uint32 port, uint16 value)>;
  using IOPortWriteDWordHandler = std::function<void(uint32 port, uint32 value)>;

  // IO port connections
  void ConnectIOPortRead(uint32 port, void* owner, IOPortReadByteHandler read_callback);
  void ConnectIOPortWrite(uint32 port, void* owner, IOPortWriteByteHandler write_callback);
  void DisconnectIOPorts(void* owner);

  // Multi-byte IO reads/writes
  // If a port is not configured for multi-byte IO, the ports following it will be used instead.
  void ConnectIOPortReadWord(uint32 port, void* owner, IOPortReadWordHandler read_callback);
  void ConnectIOPortReadDWord(uint32 port, void* owner, IOPortReadDWordHandler read_callback);
  void ConnectIOPortWriteWord(uint32 port, void* owner, IOPortWriteWordHandler write_callback);
  void ConnectIOPortWriteDWord(uint32 port, void* owner, IOPortWriteDWordHandler write_callback);

  // IO port handler accessors (mainly for CPU)
  bool ReadIOPortByte(uint32 port, uint8* value);
  bool ReadIOPortWord(uint32 port, uint16* value);
  bool ReadIOPortDWord(uint32 port, uint32* value);
  bool WriteIOPortByte(uint32 port, uint8 value);
  bool WriteIOPortWord(uint32 port, uint16 value);
  bool WriteIOPortDWord(uint32 port, uint32 value);

  // Connecting an IO port to a single variable
  void ConnectIOPortReadToPointer(uint32 port, void* owner, const uint8* var);
  void ConnectIOPortWriteToPointer(uint32 port, void* owner, uint8* var);

  // Reads/writes memory. Words must be within the same 4KiB page.
  // Reads of unmapped memory return -1.
  template<typename T>
  T ReadMemoryTyped(PhysicalMemoryAddress address);
  template<typename T>
  void WriteMemoryTyped(PhysicalMemoryAddress address, T value);

  // Reading/writing page-aligned memory.
  uint8 ReadMemoryByte(PhysicalMemoryAddress address) { return ReadMemoryTyped<uint8>(address); }
  void WriteMemoryByte(PhysicalMemoryAddress address, uint8 value) { return WriteMemoryTyped<uint8>(address, value); }
  uint16 ReadMemoryWord(PhysicalMemoryAddress address) { return ReadMemoryTyped<uint16>(address); }
  void WriteMemoryWord(PhysicalMemoryAddress address, uint16 value) { return WriteMemoryTyped<uint16>(address, value); }
  uint32 ReadMemoryDWord(PhysicalMemoryAddress address) { return ReadMemoryTyped<uint32>(address); }
  void WriteMemoryDWord(PhysicalMemoryAddress address, uint32 value)
  {
    return WriteMemoryTyped<uint32>(address, value);
  }
  uint64 ReadMemoryQWord(PhysicalMemoryAddress address) { return ReadMemoryTyped<uint64>(address); }
  void WriteMemoryQWord(PhysicalMemoryAddress address, uint64 value)
  {
    return WriteMemoryTyped<uint64>(address, value);
  }

  // Testing for readable/writable addresses.
  bool IsReadableAddress(PhysicalMemoryAddress address, uint32 size) const;
  bool IsWritableAddress(PhysicalMemoryAddress address, uint32 size) const;

  // Unaligned memory access is fine, can crosses a physical page.
  // Checked memory access, where bus errors and such are required.
  // In this case, we need to split it up into multiple words/bytes.
  bool CheckedReadMemoryByte(PhysicalMemoryAddress address, uint8* value);
  bool CheckedWriteMemoryByte(PhysicalMemoryAddress address, uint8 value);
  bool CheckedReadMemoryWord(PhysicalMemoryAddress address, uint16* value);
  bool CheckedWriteMemoryWord(PhysicalMemoryAddress address, uint16 value);
  bool CheckedReadMemoryDWord(PhysicalMemoryAddress address, uint32* value);
  bool CheckedWriteMemoryDWord(PhysicalMemoryAddress address, uint32 value);
  bool CheckedReadMemoryQWord(PhysicalMemoryAddress address, uint64* value);
  bool CheckedWriteMemoryQWord(PhysicalMemoryAddress address, uint64 value);

  // Read/write block of memory.
  void ReadMemoryBlock(PhysicalMemoryAddress address, uint32 length, void* destination);
  void WriteMemoryBlock(PhysicalMemoryAddress address, uint32 length, const void* source);

  // Get pointer to memory. Must lie within the same 64KiB page and be RAM not MMIO.
  byte* GetRAMPointer(PhysicalMemoryAddress address);

  // MMIO handlers
  void RegisterMMIO(MMIO* mmio);

  // Checks if a page contains only RAM.
  // RAM-only pages can be cached, with changes detected through the bitmask.
  bool IsCachablePage(PhysicalMemoryAddress address) const;
  bool IsWritablePage(PhysicalMemoryAddress address) const;

  // Hashes a block of code for use in backend code caches.
  CodeHashType GetCodeHash(PhysicalMemoryAddress address, uint32 length);
  void MarkPageAsCode(PhysicalMemoryAddress address);
  void UnmarkPageAsCode(PhysicalMemoryAddress address);
  void ClearPageCodeFlags();

  // Code invalidate callback - executed when pages marked as code are modified.
  void SetCodeInvalidationCallback(CodeInvalidateCallback callback);
  void ClearCodeInvalidationCallback();

  // Change page types.
  void SetPageRAMState(PhysicalMemoryAddress page_address, bool readable_memory, bool writable_memory);
  void SetPagesRAMState(PhysicalMemoryAddress start_address, uint32 size, bool readable_memory, bool writable_memory);

protected:
  struct PhysicalMemoryPage
  {
    enum Type : uint8
    {
      kReadableRAM = 1,
      kWritableRAM = 2,
      kCachedCode = 4,
      kMirror = 8,
    };

    byte* ram_ptr;
    MMIO* mmio_handler;
    uint8 type;

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
    void* owner = nullptr;
    IOPortReadByteHandler read_byte_handler;
    IOPortReadWordHandler read_word_handler;
    IOPortReadDWordHandler read_dword_handler;
    IOPortWriteByteHandler write_byte_handler;
    IOPortWriteWordHandler write_word_handler;
    IOPortWriteDWordHandler write_dword_handler;
  };

  void AllocateMemoryPages(uint32 memory_address_bits);

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
  void CheckForMemoryBreakpoint(PhysicalMemoryAddress address, uint32 size, bool is_write, uint32 value);

  // Obtain IO port connection for owner.
  IOPortConnection* GetIOPortConnectionForOwner(uint32 port, void* owner);
  IOPortConnection* CreateIOPortConnectionEntry(uint32 port, void* owner);

  System* m_system = nullptr;

  // IO ports
  std::unordered_multimap<uint32, IOPortConnection> m_ioport_handlers;

  // System memory map
  PhysicalMemoryPage* m_physical_memory_pages = nullptr;
  uint32 m_num_physical_memory_pages = 0;

  // Physical address mask, by default this is set to the maximum address
  PhysicalMemoryAddress m_physical_memory_address_mask = ~0u;

  // Code invalidate callback - executed when pages marked as code are modified.
  CodeInvalidateCallback m_code_invalidate_callback;

  // Amount of RAM allocated overall
  // Do not access this pointer directly
  byte* m_ram_ptr = nullptr;
  uint32 m_ram_size = 0;
  uint32 m_ram_assigned = 0;

  // List of ROM regions allocated.
  // This does not include mirrors.
  struct ROMRegion
  {
    std::unique_ptr<byte[]> data;
    MMIO* mmio_handler;
    PhysicalMemoryAddress mapped_address;
    uint32 size;
  };
  std::vector<ROMRegion> m_rom_regions;
};

#include "pce/bus.inl"
