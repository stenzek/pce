#include "pce/bus.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "pce/cpu.h"
#include "pce/mmio.h"
#include "pce/system.h"
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
Log_SetChannel(Bus);

// Fix link errors on GCC.
const uint32 Bus::SERIALIZATION_ID;
const uint32 Bus::MEMORY_PAGE_SIZE;
const uint32 Bus::MEMORY_PAGE_OFFSET_MASK;
const uint32 Bus::MEMORY_PAGE_MASK;

Bus::Bus(uint32 memory_address_bits)
{
  AllocateMemoryPages(memory_address_bits);
}

Bus::~Bus()
{
  delete[] m_ram_lookup;
  delete[] m_physical_memory_pages;
  delete[] m_ram_ptr;
}

void Bus::Initialize(System* system, Bus* bus)
{
  m_system = system;
}

void Bus::Reset()
{
  // Reset RAM?
}

bool Bus::LoadState(BinaryReader& reader)
{
  if (reader.ReadUInt32() != SERIALIZATION_ID)
    return false;

  uint32 physical_page_count = reader.ReadUInt32();
  if (physical_page_count != m_num_physical_memory_pages)
  {
    Log_ErrorPrintf("Incorrect number of physical memory pages");
    return false;
  }

  m_physical_memory_address_mask = reader.ReadUInt32();

  uint32 ram_size = reader.ReadUInt32();
  if (ram_size != m_ram_size)
  {
    Log_ErrorPrintf("Incorrect RAM size");
    return false;
  }

  reader.ReadBytes(m_ram_ptr, m_ram_size);
  return true;
}

bool Bus::SaveState(BinaryWriter& writer)
{
  writer.WriteUInt32(SERIALIZATION_ID);

  writer.WriteUInt32(m_num_physical_memory_pages);
  writer.WriteUInt32(m_physical_memory_address_mask);
  writer.WriteUInt32(m_ram_size);
  writer.WriteBytes(m_ram_ptr, m_ram_size);
  return true;
}

Bus::IOPortConnection* Bus::GetIOPortConnectionForOwner(uint32 port, void* owner)
{
  auto range = m_ioport_handlers.equal_range(port);
  for (auto iter = range.first; iter != range.second; iter++)
  {
    if (iter->second.owner == owner)
      return &iter->second;
  }

  return nullptr;
}

Bus::IOPortConnection* Bus::CreateIOPortConnectionEntry(uint32 port, void* owner)
{
  auto result = m_ioport_handlers.emplace(port, IOPortConnection());
  result->second.owner = owner;
  return &result->second;
}

void Bus::ConnectIOPortRead(uint32 port, void* owner, IOPortReadByteHandler read_callback)
{
  IOPortConnection* connection = GetIOPortConnectionForOwner(port, owner);
  if (!connection)
    connection = CreateIOPortConnectionEntry(port, owner);

  connection->read_byte_handler = std::move(read_callback);
}

void Bus::ConnectIOPortReadWord(uint32 port, void* owner, IOPortReadWordHandler read_callback)
{
  IOPortConnection* connection = GetIOPortConnectionForOwner(port, owner);
  if (!connection)
    connection = CreateIOPortConnectionEntry(port, owner);

  connection->read_word_handler = std::move(read_callback);
}

void Bus::ConnectIOPortReadDWord(uint32 port, void* owner, IOPortReadDWordHandler read_callback)
{
  IOPortConnection* connection = GetIOPortConnectionForOwner(port, owner);
  if (!connection)
    connection = CreateIOPortConnectionEntry(port, owner);

  connection->read_dword_handler = std::move(read_callback);
}

void Bus::ConnectIOPortWrite(uint32 port, void* owner, IOPortWriteByteHandler write_callback)
{
  IOPortConnection* connection = GetIOPortConnectionForOwner(port, owner);
  if (!connection)
    connection = CreateIOPortConnectionEntry(port, owner);

  connection->write_byte_handler = std::move(write_callback);
}

void Bus::ConnectIOPortWriteWord(uint32 port, void* owner, IOPortWriteWordHandler write_callback)
{
  IOPortConnection* connection = GetIOPortConnectionForOwner(port, owner);
  if (!connection)
    connection = CreateIOPortConnectionEntry(port, owner);

  connection->write_word_handler = std::move(write_callback);
}

void Bus::ConnectIOPortWriteDWord(uint32 port, void* owner, IOPortWriteDWordHandler write_callback)
{
  IOPortConnection* connection = GetIOPortConnectionForOwner(port, owner);
  if (!connection)
    connection = CreateIOPortConnectionEntry(port, owner);

  connection->write_dword_handler = std::move(write_callback);
}

void Bus::DisconnectIOPorts(void* owner)
{
  for (auto iter = m_ioport_handlers.begin(); iter != m_ioport_handlers.end();)
  {
    if (iter->second.owner != owner)
    {
      iter++;
      continue;
    }

    m_ioport_handlers.erase(iter++);
  }
}

bool Bus::ReadIOPortByte(uint32 port, uint8* value)
{
  // Start with a value of 0.
  // Should bits not set by devices left as floating/1 though?
  *value = 0;

  auto range = m_ioport_handlers.equal_range(port);
  bool found_handler = false;
  for (auto iter = range.first; iter != range.second; iter++)
  {
    if (!iter->second.read_byte_handler)
      continue;

    iter->second.read_byte_handler(port, value);
    found_handler = true;
  }

  if (!found_handler)
  {
    Log_DevPrintf("Unknown IO port 0x%04X (read)", port);
    *value = 0xFF;
    return false;
  }

  Log_TracePrintf("Read from ioport 0x%04X: 0x%02X", port, *value);
  return true;
}

bool Bus::ReadIOPortWord(uint32 port, uint16* value)
{
  // Start with a value of 0.
  // Should bits not set by devices left as floating/1 though?
  *value = 0;

  auto range = m_ioport_handlers.equal_range(port);
  bool found_handler = false;
  for (auto iter = range.first; iter != range.second; iter++)
  {
    if (!iter->second.read_word_handler)
      continue;

    iter->second.read_word_handler(port, value);
    found_handler = true;
  }

  // If this port does not support 16-bit IO, write as two 8-bit ports.
  if (!found_handler)
  {
    uint8 b0, b1;
    if (!ReadIOPortByte(port + 0, &b0))
      b0 = 0xFF;
    if (!ReadIOPortByte(port + 1, &b1))
      b1 = 0xFF;
    *value = ZeroExtend16(b0) | (ZeroExtend16(b1) << 8);
    return true;
  }

  Log_TracePrintf("Read from ioport 0x%04X: 0x%04X", port, ZeroExtend32(*value));
  return true;
}

bool Bus::ReadIOPortDWord(uint32 port, uint32* value)
{
  // Start with a value of 0.
  // Should bits not set by devices left as floating/1 though?
  *value = 0;

  auto range = m_ioport_handlers.equal_range(port);
  bool found_handler = false;
  for (auto iter = range.first; iter != range.second; iter++)
  {
    if (!iter->second.read_dword_handler)
      continue;

    iter->second.read_dword_handler(port, value);
    found_handler = true;
  }

  // If this port does not support 32-bit IO, write as two 16-bit ports, which will
  // turn into 2 8-bit ports.
  if (!found_handler)
  {
    uint16 b0, b1;
    if (!ReadIOPortWord(port + 0, &b0))
      b0 = 0xFFFF;
    if (!ReadIOPortWord(port + 2, &b1))
      b1 = 0xFFFF;
    *value = ZeroExtend32(b0) | (ZeroExtend32(b1) << 16);
    return true;
  }

  Log_TracePrintf("Read from ioport 0x%04X: 0x%04X", port, ZeroExtend32(*value));
  return true;
}

bool Bus::WriteIOPortByte(uint32 port, uint8 value)
{
  auto range = m_ioport_handlers.equal_range(port);
  bool found_handler = false;
  for (auto iter = range.first; iter != range.second; iter++)
  {
    if (!iter->second.write_byte_handler)
      continue;

    if (!found_handler)
      Log_TracePrintf("Write to ioport 0x%04X: 0x%02X", port, value);

    iter->second.write_byte_handler(port, value);
    found_handler = true;
  }

  if (!found_handler)
  {
    Log_DevPrintf("Unknown IO port 0x%04X (write), value = %04X", port, value);
    return false;
  }

  return true;
}

bool Bus::WriteIOPortWord(uint32 port, uint16 value)
{
  auto range = m_ioport_handlers.equal_range(port);
  bool found_handler = false;
  for (auto iter = range.first; iter != range.second; iter++)
  {
    if (!iter->second.write_word_handler)
      continue;

    if (!found_handler)
      Log_TracePrintf("Write to ioport 0x%04X: 0x%04X", port, ZeroExtend32(value));

    iter->second.write_word_handler(port, value);
    found_handler = true;
  }

  // If this port does not support 16-bit IO, write as two 8-bit ports.
  if (!found_handler)
  {
    bool result = WriteIOPortByte(port + 0, Truncate8(value >> 0));
    result |= WriteIOPortByte(port + 1, Truncate8(value >> 8));
    return result;
  }

  return true;
}

bool Bus::WriteIOPortDWord(uint32 port, uint32 value)
{
  auto range = m_ioport_handlers.equal_range(port);
  bool found_handler = false;
  for (auto iter = range.first; iter != range.second; iter++)
  {
    if (!iter->second.write_dword_handler)
      continue;

    if (!found_handler)
      Log_TracePrintf("Write to ioport 0x%04X: 0x%04X", port, ZeroExtend32(value));

    iter->second.write_dword_handler(port, value);
    found_handler = true;
  }

  // If this port does not support 32-bit IO, write as two 16-bit ports
  // (which will turn into 2 8-bit ports).
  if (!found_handler)
  {
    bool result = WriteIOPortWord(port + 0, Truncate16(value >> 0));
    result |= WriteIOPortWord(port + 2, Truncate16(value >> 16));
    return result;
  }

  return true;
}

void Bus::ConnectIOPortReadToPointer(uint32 port, void* owner, const uint8* var)
{
  IOPortReadByteHandler read_handler = [var](uint32 cb_port, uint8* value) { *value = *var; };

  ConnectIOPortRead(port, owner, std::move(read_handler));
}

void Bus::ConnectIOPortWriteToPointer(uint32 port, void* owner, uint8* var)
{
  IOPortWriteByteHandler write_handler = [var](uint32 cb_port, uint8 cb_value) { *var = cb_value; };

  ConnectIOPortWrite(port, owner, std::move(write_handler));
}

void Bus::LockPhysicalMemory(PhysicalMemoryAddress address, uint32 size, MemoryLockAccess access)
{
  DebugAssert(size > 0);

  uint32 start_page = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
  uint32 end_page = ((address + (size - 1)) & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
  DebugAssert(start_page < m_num_physical_memory_pages && end_page < m_num_physical_memory_pages);

  for (uint32 page_number = start_page; page_number <= end_page; page_number++)
  {
    PhysicalMemoryPage* page = &m_physical_memory_pages[page_number];
    if ((page->lock_type & access) != MemoryLockAccess::None)
      continue;

    page->lock_type |= access;
    UpdatePageFastMemoryLookup(page_number, page);
  }
}

void Bus::UnlockPhysicalMemory(PhysicalMemoryAddress address, uint32 size, MemoryLockAccess access)
{
  DebugAssert(size > 0);

  uint32 start_page = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
  uint32 end_page = ((address + (size - 1)) & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
  DebugAssert(start_page < m_num_physical_memory_pages && end_page < m_num_physical_memory_pages);

  for (uint32 page_number = start_page; page_number <= end_page; page_number++)
  {
    PhysicalMemoryPage* page = &m_physical_memory_pages[page_number];
    if ((page->lock_type & access) == MemoryLockAccess::None)
      continue;

    page->lock_type &= ~access;
    UpdatePageFastMemoryLookup(page_number, page);
  }
}

static bool CrossesPage(PhysicalMemoryAddress address, uint32 size)
{
  DebugAssert(size > 0);
  PhysicalMemoryAddress start_address = address;
  PhysicalMemoryAddress end_address = address + size - 1;

  return (start_address & ~(Bus::MEMORY_PAGE_SIZE - 1)) != (end_address & ~(Bus::MEMORY_PAGE_SIZE - 1));
}

#ifdef Y_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#pragma warning(disable : 4333) // warning C4333: '>>': right shift by too large amount, data loss
#pragma warning(disable : 4244) // warning C4244: '=': conversion from 'int' to 'uint8', possible loss of data
#endif

template<typename T, bool aligned>
#ifdef Y_COMPILER_MSVC
__forceinline
#elif Y_COMPILER_GCC || Y_COMPILER_CLANG
__attribute__((always_inline))
#endif
  bool
  Bus::ReadMemoryT(PhysicalMemoryAddress address, T* value)
{
  if (aligned || ((address & MEMORY_PAGE_MASK) == ((address + sizeof(T) - 1) & MEMORY_PAGE_MASK)))
  {
    address &= m_physical_memory_address_mask;

    uint32 page_number = address / MEMORY_PAGE_SIZE;
    uint32 page_offset = address % MEMORY_PAGE_SIZE;

    // Since we allocate the page array based on the address mask, this should never happen.
    DebugAssert(page_number <= m_num_physical_memory_pages);

    // Fast path.
    if (m_ram_lookup[page_number])
    {
      std::memcpy(value, m_ram_lookup[page_number] + page_offset, sizeof(T));
      return true;
    }

    // Check for memory locks
    PhysicalMemoryPage* page = &m_physical_memory_pages[page_number];
    if ((page->lock_type & MemoryLockAccess::Read) != MemoryLockAccess::None)
    {
      // Notify and remove the lock for this type.
      m_system->GetCPU()->OnLockedMemoryAccess(address, address & MEMORY_PAGE_MASK,
                                               (address & MEMORY_PAGE_MASK) + MEMORY_PAGE_SIZE, MemoryLockAccess::Read);
      page->lock_type &= ~MemoryLockAccess::Read;
      UpdatePageFastMemoryLookup(page_number, page);
    }

    // Check if we're out of the MMIO range first, this lets us exit early
    if (page_offset < page->mmio_start_offset || page_offset > page->mmio_end_offset)
    {
      // Check if we're within the RAM range.
      if (page_offset >= page->ram_start_offset && page_offset <= page->ram_end_offset)
      {
        uint32 ram_offset = page_offset - page->ram_start_offset;
        DebugAssert((ram_offset + sizeof(T)) <= page->ram_end_offset);
        std::memcpy(value, page->ram_ptr + ram_offset, sizeof(*value));
        return true;
      }
    }

    // Check for MMIO handlers within the page
    for (MMIO* mmio : page->mmio_handlers)
    {
      // The < here saves checking for partial writes on bytes.
      if (address >= mmio->GetStartAddress() && address <= mmio->GetEndAddress())
      {
        // Check where the write is a partial MMIO write and partial RAM write
        if (std::is_same<T, uint8>::value || (address + sizeof(T) - 1) <= mmio->GetEndAddress())
        {
          // Pass to MMIO
          if (std::is_same<T, uint8>::value)
            mmio->ReadByte(address, reinterpret_cast<uint8*>(value));
          else if (std::is_same<T, uint16>::value)
            mmio->ReadWord(address, reinterpret_cast<uint16*>(value));
          else if (std::is_same<T, uint32>::value)
            mmio->ReadDWord(address, reinterpret_cast<uint32*>(value));
          else if (std::is_same<T, uint64>::value)
            mmio->ReadQWord(address, reinterpret_cast<uint64*>(value));

          return true;
        }
        else
        {
          AssertMsg(false, "Fix me");
        }
      }
    }

    *value = T(-1);
    // Log_WarningPrintf("Failed physical memory read of address 0x%08X", address);
    return false;
  }

  if (std::is_same<T, uint16>::value)
  {
    // Do as two byte reads instead.
    uint8 lsb = 0, msb = 0;
    bool result = CheckedReadMemoryByte(address, &lsb);
    result &= CheckedReadMemoryByte(address + 1, &msb);
    *value = (ZeroExtend16(msb) << 8) | ZeroExtend16(lsb);
    return result;
  }
  else if (std::is_same<T, uint32>::value)
  {
    // Do as two word reads instead.
    uint16 lsb = 0, msb = 0;
    bool result = CheckedReadMemoryWord(address, &lsb);
    result &= CheckedReadMemoryWord(address + 2, &msb);
    *value = (ZeroExtend32(msb) << 16) | ZeroExtend32(lsb);
    return result;
  }
  else
  {
    // Not reached, but needed otherwise compile warnings.
    return false;
  }
}

template<typename T, bool aligned>
#ifdef Y_COMPILER_MSVC
__forceinline
#elif Y_COMPILER_GCC || Y_COMPILER_CLANG
__attribute__((always_inline))
#endif
  bool
  Bus::WriteMemoryT(PhysicalMemoryAddress address, T value)
{
#if 0
    static const uint32 check_addresses[] = {
        //0x3AFCC, 4,
        //0x2e9f4, 4
        //0x12420, 4
        //0xB3B528, 4
        0x0015a8d0, 4
    };
    uint32 v_start = address;
    uint32 v_end = address + sizeof(T);
    for (uint32 i = 0; i < countof(check_addresses); i += 2)
    {
        uint32 a_start = check_addresses[i];
        uint32 a_end = a_start + check_addresses[i + 1];
        if ((v_start >= a_start && v_end <= a_end) ||
            (a_start >= v_start && a_end <= v_end))
        {
            Log_WarningPrintf("Mem BP %08X while writing %08X (value %X)", a_start, v_start, ZeroExtend32(value));
        }
    }
#endif

  if (aligned || ((address & MEMORY_PAGE_MASK) == ((address + sizeof(T) - 1) & MEMORY_PAGE_MASK)))
  {
    address &= m_physical_memory_address_mask;

    uint32 page_number = address / MEMORY_PAGE_SIZE;
    uint32 page_offset = address % MEMORY_PAGE_SIZE;

    // Since we allocate the page array based on the address mask, this should never happen.
    DebugAssert(page_number <= m_num_physical_memory_pages);

    // Fast path.
    if (m_ram_lookup[page_number])
    {
      std::memcpy(m_ram_lookup[page_number] + page_offset, &value, sizeof(value));
      return true;
    }

    // Check for memory locks
    PhysicalMemoryPage* page = &m_physical_memory_pages[page_number];
    if ((page->lock_type & MemoryLockAccess::Write) != MemoryLockAccess::None)
    {
      // Notify and remove the lock for this type.
      m_system->GetCPU()->OnLockedMemoryAccess(
        address, address & MEMORY_PAGE_MASK, (address & MEMORY_PAGE_MASK) + MEMORY_PAGE_SIZE, MemoryLockAccess::Write);
      page->lock_type &= ~MemoryLockAccess::Write;
      UpdatePageFastMemoryLookup(page_number, page);
    }

    // Check if we're out of the MMIO range first, this lets us exit early
    if (page_offset < page->mmio_start_offset || page_offset > page->mmio_end_offset)
    {
      // Check if we're within the RAM range.
      if (page_offset >= page->ram_start_offset && page_offset <= page->ram_end_offset)
      {
        uint32 ram_offset = page_offset - page->ram_start_offset;
        DebugAssert((ram_offset + sizeof(T)) <= page->ram_end_offset);
        std::memcpy(page->ram_ptr + ram_offset, &value, sizeof(T));
        return true;
      }
    }

    // Check for MMIO handlers within the page
    for (MMIO* mmio : page->mmio_handlers)
    {
      // The < here saves checking for partial writes on bytes.
      if (address >= mmio->GetStartAddress() && address <= mmio->GetEndAddress())
      {
        // Check where the write is a partial MMIO write and partial RAM write
        if (std::is_same<T, uint8>::value || (address + sizeof(T) - 1) <= mmio->GetEndAddress())
        {
          // Pass to MMIO
          if (std::is_same<T, uint8>::value)
            mmio->WriteByte(address, static_cast<uint8>(value));
          else if (std::is_same<T, uint16>::value)
            mmio->WriteWord(address, static_cast<uint16>(value));
          else if (std::is_same<T, uint32>::value)
            mmio->WriteDWord(address, static_cast<uint32>(value));
          else if (std::is_same<T, uint64>::value)
            mmio->WriteQWord(address, static_cast<uint64>(value));

          return true;
        }
        else
        {
          AssertMsg(false, "Fix me");
        }
      }
    }

    Log_DevPrintf("Failed physical memory write of address 0x%08X", address);
    return false;
  }

  if (std::is_same<T, uint16>::value)
  {
    // Do as two byte writes instead.
    return CheckedWriteMemoryByte(address + 0, Truncate8(value)) &
           CheckedWriteMemoryByte(address + 1, Truncate8(value >> 8));
  }
  else if (std::is_same<T, uint32>::value)
  {
    // Do as two word writes instead.
    return CheckedWriteMemoryWord(address + 0, Truncate16(value)) &
           CheckedWriteMemoryWord(address + 2, Truncate16(value >> 16));
  }
  else
  {
    // Not reached, but needed otherwise compile warnings.
    return false;
  }
}

#ifdef Y_COMPILER_MSVC
#pragma warning(pop)
#endif

uint8 Bus::ReadMemoryByte(PhysicalMemoryAddress address)
{
  uint8 value;
  ReadMemoryT<uint8, true>(address, &value);
  return value;
}

void Bus::WriteMemoryByte(PhysicalMemoryAddress address, uint8 value)
{
  WriteMemoryT<uint8, true>(address, value);
}

uint16 Bus::ReadMemoryWord(PhysicalMemoryAddress address)
{
  uint16 value;
  ReadMemoryT<uint16, true>(address, &value);
  return value;
}

void Bus::WriteMemoryWord(PhysicalMemoryAddress address, uint16 value)
{
  WriteMemoryT<uint16, true>(address, value);
}

uint32 Bus::ReadMemoryDWord(PhysicalMemoryAddress address)
{
  uint32 value;
  ReadMemoryT<uint32, true>(address, &value);
  return value;
}

void Bus::WriteMemoryDWord(PhysicalMemoryAddress address, uint32 value)
{
  WriteMemoryT<uint32, true>(address, value);
}

uint64 Bus::ReadMemoryQWord(PhysicalMemoryAddress address)
{
  uint64 value;
  ReadMemoryT<uint64, true>(address, &value);
  return value;
}

void Bus::WriteMemoryQWord(PhysicalMemoryAddress address, uint64 value)
{
  WriteMemoryT<uint64, true>(address, value);
}

bool Bus::CheckedReadMemoryByte(PhysicalMemoryAddress address, uint8* value)
{
  return ReadMemoryT<uint8, false>(address, value);
}

bool Bus::CheckedWriteMemoryByte(PhysicalMemoryAddress address, uint8 value)
{
  return WriteMemoryT<uint8, false>(address, value);
}

bool Bus::CheckedReadMemoryWord(PhysicalMemoryAddress address, uint16* value)
{
  return ReadMemoryT<uint16, false>(address, value);
}

bool Bus::CheckedWriteMemoryWord(PhysicalMemoryAddress address, uint16 value)
{
  return WriteMemoryT<uint16, false>(address, value);
}

bool Bus::CheckedReadMemoryDWord(PhysicalMemoryAddress address, uint32* value)
{
  return ReadMemoryT<uint32, false>(address, value);
}

bool Bus::CheckedWriteMemoryDWord(PhysicalMemoryAddress address, uint32 value)
{
  return WriteMemoryT<uint32, false>(address, value);
}

bool Bus::CheckedReadMemoryQWord(PhysicalMemoryAddress address, uint64* value)
{
  return ReadMemoryT<uint64, false>(address, value);
}

bool Bus::CheckedWriteMemoryQWord(PhysicalMemoryAddress address, uint64 value)
{
  return WriteMemoryT<uint64, false>(address, value);
}

void* Bus::GetRAMPointer(PhysicalMemoryAddress address, uint32 size)
{
  // Can't get a pointer when it crosses a page
  if (CrossesPage(address, size))
    return nullptr;

  // Get the page
  uint32 page_number = address / MEMORY_PAGE_SIZE;
  uint32 page_offset = address % MEMORY_PAGE_SIZE;
  if (page_number >= m_num_physical_memory_pages)
    return nullptr;

  // If it's range spans outside the ram region, can't get it
  PhysicalMemoryPage* page = &m_physical_memory_pages[page_number];
  if (page_offset < page->ram_start_offset || (page_offset + size) > page->ram_end_offset)
    return nullptr;

  // Everything checks out
  return page->ram_ptr + (page_offset - page->ram_start_offset);
}

void Bus::AllocateMemoryPages(uint32 memory_address_bits)
{
  uint32 num_pages = uint32((uint64(1) << memory_address_bits) / uint64(MEMORY_PAGE_SIZE));

  // Allocate physical pages
  DebugAssert(num_pages > 0);
  m_physical_memory_pages = new PhysicalMemoryPage[num_pages];
  m_physical_memory_address_mask = uint32((uint64(1) << memory_address_bits) - 1);
  m_num_physical_memory_pages = num_pages;

  m_ram_lookup = new byte*[num_pages];
  std::fill_n(m_ram_lookup, num_pages, nullptr);
}

PhysicalMemoryAddress Bus::GetTotalRAMInPageRange(uint32 start_page, uint32 end_page) const
{
  PhysicalMemoryAddress size = 0;
  for (uint32 i = start_page; i < end_page; i++)
  {
    const PhysicalMemoryPage* page = &m_physical_memory_pages[i];
    size += (page->ram_end_offset - page->ram_start_offset);
  }

  return size;
}

void Bus::AllocateRAM(uint32 size)
{
  DebugAssert(size > 0 && !m_ram_ptr);
  m_ram_ptr = new byte[size];
  m_ram_size = size;
  m_ram_assigned = 0;
  std::memset(m_ram_ptr, 0x00, m_ram_size);
}

uint32 Bus::CreateRAMRegion(PhysicalMemoryAddress start, PhysicalMemoryAddress end)
{
  Assert((start % MEMORY_PAGE_SIZE) == 0 && (uint64(end + 1) % MEMORY_PAGE_SIZE) == 0);

  uint32 allocated_ram = 0;
  uint32 remaining_ram = GetUnassignedRAMSize();
  uint32 start_page = start / MEMORY_PAGE_SIZE;
  uint32 end_page = Truncate32((uint64(end) + 1) / MEMORY_PAGE_SIZE);
  for (uint32 current_page = start_page; current_page < end_page && remaining_ram > 0; current_page++)
  {
    PhysicalMemoryPage* page = &m_physical_memory_pages[current_page];
    uint32 ram_in_page = std::min(remaining_ram, MEMORY_PAGE_SIZE);
    Assert(!page->ram_ptr);
    page->ram_ptr = m_ram_ptr + m_ram_assigned;
    page->ram_start_offset = 0;
    page->ram_end_offset = ram_in_page;
    m_ram_assigned += ram_in_page;
    allocated_ram += ram_in_page;
    remaining_ram -= ram_in_page;
    UpdatePageFastMemoryLookup(current_page, page);
  }

  return allocated_ram;
}

void Bus::UpdatePageMMIORange(uint32 page_number, PhysicalMemoryPage* page)
{
  if (page->mmio_handlers.empty())
  {
    page->mmio_start_offset = ~0u;
    page->mmio_end_offset = ~0u;
    return;
  }

  // Sort in ascending order, this may enable optimizations when I'm not lazy later
  // TODO: Check for overlapping entries
  auto sort_callback = [](MMIO*& lhs, MMIO*& rhs) { return (lhs->GetStartAddress() > rhs->GetStartAddress()); };
  std::sort(page->mmio_handlers.begin(), page->mmio_handlers.end(), sort_callback);

  // Find min/max address, this could be replaced with first/last now it's sorted
  PhysicalMemoryAddress start_address = ~PhysicalMemoryAddress(0);
  PhysicalMemoryAddress end_address = 0;
  for (MMIO* mmio : page->mmio_handlers)
  {
    start_address = std::min(start_address, mmio->GetStartAddress());
    end_address = std::max(end_address, mmio->GetEndAddress());
  }

  PhysicalMemoryAddress page_start = PhysicalMemoryAddress(page_number * MEMORY_PAGE_SIZE);
  if (start_address < page_start)
    page->mmio_start_offset = 0;
  else
    page->mmio_start_offset = start_address - page_start;

  if (end_address >= (page_start + (MEMORY_PAGE_SIZE - 1)))
    page->mmio_end_offset = MEMORY_PAGE_SIZE - 1;
  else
    page->mmio_end_offset = end_address - page_start;
}

void Bus::UpdatePageFastMemoryLookup(uint32 page_number, PhysicalMemoryPage* page)
{
  if (!page->mmio_handlers.empty() || page->ram_start_offset != 0 || page->ram_end_offset != MEMORY_PAGE_SIZE ||
      page->lock_type != MemoryLockAccess::None)
  {
    m_ram_lookup[page_number] = nullptr;
  }
  else
  {
    m_ram_lookup[page_number] = page->ram_ptr;
  }
}

template<typename T>
void Bus::EnumeratePagesForRange(PhysicalMemoryAddress start_address, PhysicalMemoryAddress end_address, T callback)
{
  DebugAssert(end_address > start_address);

  uint32 first_page = start_address / MEMORY_PAGE_SIZE;
  uint32 last_page = end_address / MEMORY_PAGE_SIZE;

  for (uint32 i = first_page; i <= last_page; i++)
  {
    if (i < m_num_physical_memory_pages)
      callback(i, &m_physical_memory_pages[i]);
  }
}

void Bus::RegisterMMIO(MMIO* mmio)
{
  // MMIO size should be 4 byte aligned, that way we don't have to split reads/writes.
  Assert((mmio->GetSize() & uint32(sizeof(uint32) - 1)) == 0);

  auto callback = [this, mmio](uint32 page_number, PhysicalMemoryPage* page) {
    mmio->AddRef();
    page->mmio_handlers.push_back(mmio);
    UpdatePageMMIORange(page_number, page);
    UpdatePageFastMemoryLookup(page_number, page);
  };

  EnumeratePagesForRange(mmio->GetStartAddress(), mmio->GetEndAddress(), callback);
}

void Bus::UnregisterMMIO(MMIO* mmio)
{
  auto callback = [this, mmio](uint32 page_number, PhysicalMemoryPage* page) {
    auto iter = std::find(page->mmio_handlers.begin(), page->mmio_handlers.end(), mmio);
    if (iter != page->mmio_handlers.end())
    {
      page->mmio_handlers.erase(iter);
      mmio->AddRef();
    }
    UpdatePageMMIORange(page_number, page);
    UpdatePageFastMemoryLookup(page_number, page);
  };

  EnumeratePagesForRange(mmio->GetStartAddress(), mmio->GetEndAddress(), callback);
}
