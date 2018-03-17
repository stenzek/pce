#pragma once

#include <atomic>
#include <cstring>
#include <functional>
#include <thread>
#include <unordered_map>

#include "YBaseLib/Barrier.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Event.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/TaskQueue.h"
#include "YBaseLib/Timer.h"

#include "pce/component.h"
#include "pce/types.h"

class ByteStream;
class BinaryReader;
class BinaryWriter;
class MMIO;

class Bus : public Component
{
public:
  static const uint32 SERIALIZATION_ID = Component::MakeSerializationID('B', 'U', 'S');
  static const uint32 MEMORY_PAGE_SIZE = 0x1000; // 4KiB
  static const uint32 MEMORY_PAGE_OFFSET_MASK = PhysicalMemoryAddress(MEMORY_PAGE_SIZE - 1);
  static const uint32 MEMORY_PAGE_MASK = ~MEMORY_PAGE_OFFSET_MASK;

  Bus(uint32 memory_address_bits);
  ~Bus();

  void Initialize(System* system, Bus* bus) override;
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
  void MirrorRAMRegion(PhysicalMemoryAddress start, PhysicalMemoryAddress end, PhysicalMemoryAddress mirror_start);

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

  // Memory locks, size is in bytes (converted to pages)
  void LockPhysicalMemory(PhysicalMemoryAddress address, uint32 size, MemoryLockAccess access);
  void UnlockPhysicalMemory(PhysicalMemoryAddress address, uint32 size, MemoryLockAccess access);

  // Reads/writes memory. Words must be within the same 4KiB page.
  // Reads of unmapped memory return -1.
  uint8 ReadMemoryByte(PhysicalMemoryAddress address);
  void WriteMemoryByte(PhysicalMemoryAddress address, uint8 value);
  uint16 ReadMemoryWord(PhysicalMemoryAddress address);
  void WriteMemoryWord(PhysicalMemoryAddress address, uint16 value);
  uint32 ReadMemoryDWord(PhysicalMemoryAddress address);
  void WriteMemoryDWord(PhysicalMemoryAddress address, uint32 value);
  uint64 ReadMemoryQWord(PhysicalMemoryAddress address);
  void WriteMemoryQWord(PhysicalMemoryAddress address, uint64 value);

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

  // Get pointer to memory. Must lie within the same 64KiB page and be RAM not MMIO.
  void* GetRAMPointer(PhysicalMemoryAddress address, uint32 size);

  // MMIO handlers
  void RegisterMMIO(MMIO* mmio);
  void UnregisterMMIO(MMIO* mmio);

protected:
  struct PhysicalMemoryPage
  {
    byte* ram_ptr = nullptr;
    PhysicalMemoryAddress ram_start_offset = ~0u;
    PhysicalMemoryAddress ram_end_offset = ~0u;

    std::vector<MMIO*> mmio_handlers;
    PhysicalMemoryAddress mmio_start_offset = ~0u;
    PhysicalMemoryAddress mmio_end_offset = ~0u;

    MemoryLockAccess lock_type = MemoryLockAccess::None;
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

  static void UpdatePageMMIORange(uint32 page_number, PhysicalMemoryPage* page);
  void UpdatePageFastMemoryLookup(uint32 page_number, PhysicalMemoryPage* page);
  template<typename T>
  void EnumeratePagesForRange(PhysicalMemoryAddress start_address, PhysicalMemoryAddress end_address, T callback);

  // Generic memory read/write handler
  template<typename T, bool aligned>
  bool ReadMemoryT(PhysicalMemoryAddress address, T* value);
  template<typename T, bool aligned>
  bool WriteMemoryT(PhysicalMemoryAddress address, T value);

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

  // Fast RAM memory access, indexed by page number. If null, needs slow lookup.
  byte** m_ram_lookup;

  // Amount of RAM allocated overall
  // Do not access this pointer directly
  byte* m_ram_ptr = nullptr;
  uint32 m_ram_size = 0;
  uint32 m_ram_assigned = 0;
};
