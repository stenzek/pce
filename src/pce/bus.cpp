#define XXH_STATIC_LINKING_ONLY

#include "pce/bus.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "common/state_wrapper.h"
#include "pce/cpu.h"
#include "pce/mmio.h"
#include "pce/system.h"
#include "xxhash.h"
#include <array>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
Log_SetChannel(Bus);

DEFINE_OBJECT_TYPE_INFO(Bus);

Bus::Bus(u32 memory_address_bits, const ObjectTypeInfo* type_info /* = &s_type_info */) : BaseClass(type_info)
{
  AllocateMemoryPages(memory_address_bits);
  m_ioport_handlers = new IOPortConnection*[NUM_IOPORTS];
  std::memset(m_ioport_handlers, 0, sizeof(IOPortConnection*) * NUM_IOPORTS);
}

Bus::~Bus()
{
  for (auto& it : m_rom_regions)
    SAFE_RELEASE(it.mmio_handler);

  for (u32 i = 0; i < NUM_IOPORTS; i++)
  {
    IOPortConnection* conn = m_ioport_handlers[i];
    while (conn)
    {
      IOPortConnection* temp = conn;
      conn = conn->next;
      delete temp;
    }
  }

  for (u32 i = 0; i < m_num_physical_memory_pages; i++)
  {
    if (m_physical_memory_pages[i].mmio_handler)
      m_physical_memory_pages[i].mmio_handler->Release();
  }

  delete[] m_physical_memory_pages;
  delete[] m_ram_ptr;
}

bool Bus::Initialize(System* system)
{
  m_system = system;
  return true;
}

void Bus::Reset()
{
  // Reset RAM
  if (m_ram_ptr)
    std::memset(m_ram_ptr, 0, m_ram_size);
}

bool Bus::DoState(StateWrapper& sw)
{
  u32 physical_page_count = m_num_physical_memory_pages;
  sw.Do(&physical_page_count);
  if (physical_page_count != m_num_physical_memory_pages)
  {
    Log_ErrorPrintf("Incorrect number of physical memory pages");
    return false;
  }

  u32 ram_size = m_ram_size;
  sw.Do(&ram_size);
  if (ram_size != m_ram_size)
  {
    Log_ErrorPrintf("Incorrect RAM size");
    return false;
  }

  sw.Do(&m_physical_memory_address_mask);
  sw.DoBytes(m_ram_ptr, m_ram_size);
  return !sw.HasError();
}

void Bus::CheckForMemoryBreakpoint(PhysicalMemoryAddress address, u32 size, bool is_write, u32 value)
{
#if 0
  static const uint32 check_addresses[] = { 0x0001c16a, 4, 0x0001c21a, 4};

  uint32 v_start = address;
  uint32 v_end = address + size;

  for (uint32 i = 0; i < countof(check_addresses); i += 2)
  {
    uint32 a_start = check_addresses[i];
    uint32 a_end = a_start + check_addresses[i + 1];

    if ((v_start >= a_start && v_end <= a_end) || (a_start >= v_start && a_end <= v_end))
    {
      if (is_write)
        Log_WarningPrintf("Mem BP %08X while writing %08X (value 0x%08X)", a_start, v_start, value);
      else
        Log_WarningPrintf("Mem BP %08X while reading %08X (value 0x%08X)", a_start, v_start, value);
      // m_system->SetState(System::State::Paused);
      break;
    }
  }

#endif
}

Bus::IOPortConnection* Bus::GetIOPortConnection(u16 port, const void* owner)
{
  IOPortConnection* conn = m_ioport_handlers[port];
  while (conn)
  {
    if (conn->owner == owner)
      return conn;

    conn = conn->next;
  }

  return conn;
}

Bus::IOPortConnection* Bus::CreateIOPortConnection(u16 port, const void* owner)
{
  // Add to tracking list.
  auto iter = m_ioport_owners.find(owner);
  if (iter == m_ioport_owners.end())
    iter = m_ioport_owners.emplace(owner, std::vector<u16>()).first;
  if (std::find(iter->second.begin(), iter->second.end(), port) == iter->second.end())
    iter->second.push_back(port);

  IOPortConnection* conn = new IOPortConnection();
  conn->owner = owner;
  conn->next = nullptr;

  IOPortConnection* exiting_conn = m_ioport_handlers[port];
  if (!exiting_conn)
  {
    m_ioport_handlers[port] = conn;
    return conn;
  }

  // Add to end of linked list.
  while (exiting_conn->next)
    exiting_conn = exiting_conn->next;

  exiting_conn->next = conn;
  return conn;
}

void Bus::RemoveIOPortConnection(u16 port, const void* owner)
{
  IOPortConnection* conn = m_ioport_handlers[port];
  IOPortConnection* prev = nullptr;
  while (conn)
  {
    if (conn->owner != owner)
    {
      prev = conn;
      conn = conn->next;
      continue;
    }

    // Remove from linked list.
    if (prev)
      prev->next = conn->next;
    else
      m_ioport_handlers[port] = conn->next;

    IOPortConnection* temp = conn;
    conn = conn->next;
    delete temp;
  }
}

void Bus::ConnectIOPortRead(u16 port, const void* owner, IOPortReadByteHandler read_callback)
{
  IOPortConnection* connection = GetIOPortConnection(port, owner);
  if (!connection)
    connection = CreateIOPortConnection(port, owner);

  connection->read_byte_handler = std::move(read_callback);
}

void Bus::ConnectIOPortReadWord(u16 port, const void* owner, IOPortReadWordHandler read_callback)
{
  IOPortConnection* connection = GetIOPortConnection(port, owner);
  if (!connection)
    connection = CreateIOPortConnection(port, owner);

  connection->read_word_handler = std::move(read_callback);
}

void Bus::ConnectIOPortReadDWord(u16 port, const void* owner, IOPortReadDWordHandler read_callback)
{
  IOPortConnection* connection = GetIOPortConnection(port, owner);
  if (!connection)
    connection = CreateIOPortConnection(port, owner);

  connection->read_dword_handler = std::move(read_callback);
}

void Bus::ConnectIOPortWrite(u16 port, const void* owner, IOPortWriteByteHandler write_callback)
{
  IOPortConnection* connection = GetIOPortConnection(port, owner);
  if (!connection)
    connection = CreateIOPortConnection(port, owner);

  connection->write_byte_handler = std::move(write_callback);
}

void Bus::ConnectIOPortWriteWord(u16 port, const void* owner, IOPortWriteWordHandler write_callback)
{
  IOPortConnection* connection = GetIOPortConnection(port, owner);
  if (!connection)
    connection = CreateIOPortConnection(port, owner);

  connection->write_word_handler = std::move(write_callback);
}

void Bus::ConnectIOPortWriteDWord(u16 port, const void* owner, IOPortWriteDWordHandler write_callback)
{
  IOPortConnection* connection = GetIOPortConnection(port, owner);
  if (!connection)
    connection = CreateIOPortConnection(port, owner);

  connection->write_dword_handler = std::move(write_callback);
}

void Bus::DisconnectIOPort(u16 port, const void* owner)
{
  RemoveIOPortConnection(port, owner);

  auto iter = m_ioport_owners.find(owner);
  if (iter != m_ioport_owners.end())
  {
    // Remove from port tracking list.
    auto iter2 = std::find(iter->second.begin(), iter->second.end(), port);
    if (iter2 != iter->second.end())
      iter->second.erase(iter2);

    // Remove from list when the last one is done.
    if (iter->second.empty())
      m_ioport_owners.erase(owner);
  }
}

void Bus::DisconnectIOPorts(const void* owner)
{
  auto iter = m_ioport_owners.find(owner);
  if (iter == m_ioport_owners.end())
    return;

  for (const u16 port : iter->second)
    RemoveIOPortConnection(port, owner);

  m_ioport_owners.erase(iter);
}

u8 Bus::ReadIOPortByte(u16 port)
{
  const IOPortConnection* conn = m_ioport_handlers[port];
  while (conn)
  {
    const IOPortConnection* current = conn;
    conn = conn->next;
    if (current->read_byte_handler)
      return current->read_byte_handler(port);
  }

  Log_DebugPrintf("Unknown IO port 0x%04X (read)", port);
  return 0xFF;
}

u16 Bus::ReadIOPortWord(u16 port)
{
  const IOPortConnection* conn = m_ioport_handlers[port];
  while (conn)
  {
    const IOPortConnection* current = conn;
    conn = conn->next;
    if (current->read_word_handler)
      return current->read_word_handler(port);
  }

  // If this port does not support 16-bit IO, write as two 8-bit ports.
  const u8 b0 = ReadIOPortByte(port + 0);
  const u8 b1 = ReadIOPortByte(port + 1);
  return ZeroExtend16(b0) | (ZeroExtend16(b1) << 8);
}

u32 Bus::ReadIOPortDWord(u16 port)
{
  const IOPortConnection* conn = m_ioport_handlers[port];
  while (conn)
  {
    const IOPortConnection* current = conn;
    conn = conn->next;
    if (current->read_dword_handler)
      return current->read_dword_handler(port);
  }

  // If this port does not support 32-bit IO, write as two 16-bit ports, which will
  // turn into 2 8-bit ports.
  const u16 b0 = ReadIOPortWord(port + 0);
  const u16 b1 = ReadIOPortWord(port + 2);
  return ZeroExtend32(b0) | (ZeroExtend32(b1) << 16);
}

void Bus::WriteIOPortByte(u16 port, u8 value)
{
  const IOPortConnection* conn = m_ioport_handlers[port];
  if (!conn)
  {
    Log_DebugPrintf("Unknown IO port 0x%04X (write), value = %04X", port, value);
    return;
  }

  do
  {
    const IOPortConnection* current = conn;
    conn = conn->next;
    if (current->write_byte_handler)
      current->write_byte_handler(port, value);
  } while (conn);

  // Log_TracePrintf("Write to ioport 0x%04X: 0x%02X", port, value);
}

void Bus::WriteIOPortWord(u16 port, u16 value)
{
  // If this port does not support 16-bit IO, write as two 8-bit ports.
  const IOPortConnection* conn = m_ioport_handlers[port];
  if (!conn || !conn->write_word_handler)
  {
    WriteIOPortByte(port + 0, Truncate8(value >> 0));
    WriteIOPortByte(port + 1, Truncate8(value >> 8));
    return;
  }

  do
  {
    const IOPortConnection* current = conn;
    conn = conn->next;
    if (current->write_word_handler)
      current->write_word_handler(port, value);
  } while (conn);

  // Log_TracePrintf("Write to ioport 0x%04X: 0x%04X", port, ZeroExtend32(value));
}

void Bus::WriteIOPortDWord(u16 port, u32 value)
{
  // If this port does not support 32-bit IO, write as two 16-bit ports
  // (which will turn into 2 8-bit ports).
  const IOPortConnection* conn = m_ioport_handlers[port];
  if (!conn || !conn->write_dword_handler)
  {
    WriteIOPortWord(port + 0, Truncate16(value >> 0));
    WriteIOPortWord(port + 2, Truncate16(value >> 16));
    return;
  }

  do
  {
    const IOPortConnection* current = conn;
    conn = conn->next;
    if (current->write_dword_handler)
      current->write_dword_handler(port, value);
  } while (conn);

  // Log_TracePrintf("Write to ioport 0x%04X: 0x%04X", port, ZeroExtend32(value));
}

void Bus::ConnectIOPortReadToPointer(u16 port, const void* owner, const u8* var)
{
  IOPortReadByteHandler read_handler = [var](u16 cb_port) { return *var; };

  ConnectIOPortRead(port, owner, std::move(read_handler));
}

void Bus::ConnectIOPortWriteToPointer(u16 port, const void* owner, u8* var)
{
  IOPortWriteByteHandler write_handler = [var](u32 cb_port, u8 cb_value) { *var = cb_value; };

  ConnectIOPortWrite(port, owner, std::move(write_handler));
}

void Bus::ReadMemoryBlock(PhysicalMemoryAddress address, u32 length, void* destination)
{
  byte* destination_ptr = reinterpret_cast<byte*>(destination);

  while (length > 0)
  {
    u32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
    u32 page_offset = (address & m_physical_memory_address_mask) % MEMORY_PAGE_SIZE;
    DebugAssert(page_number < m_num_physical_memory_pages);

    // Fast path?
    const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
    const u32 size_in_page = std::min(length, MEMORY_PAGE_SIZE);
    if (page.type & PhysicalMemoryPage::kReadableRAM)
    {
      std::memcpy(destination_ptr, page.ram_ptr + page_offset, size_in_page);
      destination_ptr += size_in_page;
      address += size_in_page;
      length -= size_in_page;
      continue;
    }

    if (page.mmio_handler)
    {
      // Slow path for MMIO.
      MMIO* const handler = page.mmio_handler;
      const u32 page_base_address = page_number * MEMORY_PAGE_SIZE;
      const u32 start_address = handler->GetStartAddress();
      const u32 end_address = handler->GetEndAddress();
      if (address >= start_address && (address + size_in_page) <= end_address)
      {
        handler->ReadBlock(address, size_in_page, destination_ptr);
        destination_ptr += size_in_page;
        address += size_in_page;
        length -= size_in_page;
        continue;
      }

      // Super slow path - when the MMIO doesn't cover the entire page.
      const u32 mmio_size_in_page = end_address - page_base_address;
      const u32 mmio_usable_size = mmio_size_in_page - page_offset;
      const u32 padding_before = (start_address >= address) ? (start_address - address) : 0;
      const u32 padding_after = (mmio_usable_size < size_in_page) ? (size_in_page - mmio_usable_size) : 0;
      const u32 copy_size = std::min(mmio_usable_size, size_in_page);
      if (padding_before > 0)
      {
        std::memset(destination_ptr, 0xFF, padding_before);
        destination_ptr += padding_before;
        address += padding_before;
        length -= padding_before;
      }
      if (copy_size > 0)
      {
        handler->ReadBlock(address, copy_size, destination_ptr);
        destination_ptr += copy_size;
        address += copy_size;
        length -= copy_size;
      }
      if (padding_after > 0)
      {
        std::memset(destination_ptr, 0xFF, padding_after);
        destination_ptr += padding_after;
        address += padding_after;
        length -= padding_after;
      }
    }
    else
    {
      // Not valid memory.
      std::memset(destination_ptr, 0xFF, size_in_page);
      destination_ptr += size_in_page;
      address += size_in_page;
      length -= size_in_page;
    }
  }
}

void Bus::WriteMemoryBlock(PhysicalMemoryAddress address, u32 length, const void* source)
{
  const byte* source_ptr = reinterpret_cast<const byte*>(source);

  while (length > 0)
  {
    u32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
    u32 page_offset = (address & m_physical_memory_address_mask) % MEMORY_PAGE_SIZE;
    DebugAssert(page_number < m_num_physical_memory_pages);

    // Fast path?
    const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
    const u32 size_in_page = std::min(length, MEMORY_PAGE_SIZE);
    if (page.type & PhysicalMemoryPage::kWritableRAM)
    {
      std::memcpy(page.ram_ptr + page_offset, source_ptr, size_in_page);
      source_ptr += size_in_page;
      address += size_in_page;
      length -= size_in_page;
      continue;
    }

    if (page.mmio_handler)
    {
      // Slow path for MMIO.
      MMIO* const handler = page.mmio_handler;
      const u32 page_base_address = page_number * MEMORY_PAGE_SIZE;
      const u32 start_address = handler->GetStartAddress();
      const u32 end_address = handler->GetEndAddress();
      if (address >= start_address && (address + size_in_page) <= end_address)
      {
        handler->WriteBlock(address, size_in_page, source_ptr);
        source_ptr += size_in_page;
        address += size_in_page;
        length -= size_in_page;
        continue;
      }

      // Super slow path - when the MMIO doesn't cover the entire page.
      const u32 mmio_size_in_page = end_address - page_base_address;
      const u32 mmio_usable_size = mmio_size_in_page - page_offset;
      const u32 padding_before = (start_address >= address) ? (start_address - address) : 0;
      const u32 padding_after = (mmio_usable_size < size_in_page) ? (size_in_page - mmio_usable_size) : 0;
      const u32 copy_size = std::min(mmio_usable_size, size_in_page);
      if (padding_before > 0)
      {
        source_ptr += padding_before;
        address += padding_before;
        length -= padding_before;
      }
      if (copy_size > 0)
      {
        handler->WriteBlock(address, copy_size, source_ptr);
        source_ptr += copy_size;
        address += copy_size;
        length -= copy_size;
      }
      if (padding_after > 0)
      {
        source_ptr += padding_after;
        address += padding_after;
        length -= padding_after;
      }
    }
    else
    {
      // Not valid memory.
      source_ptr += size_in_page;
      address += size_in_page;
      length -= size_in_page;
    }
  }
}

void Bus::AllocateMemoryPages(u32 memory_address_bits)
{
  u32 num_pages = u32((u64(1) << memory_address_bits) / u64(MEMORY_PAGE_SIZE));

  // Allocate physical pages
  DebugAssert(num_pages > 0);
  m_physical_memory_pages = new PhysicalMemoryPage[num_pages];
  m_physical_memory_page_ram_index = new byte*[num_pages];
  std::memset(m_physical_memory_pages, 0, sizeof(PhysicalMemoryPage) * num_pages);
  std::fill_n(m_physical_memory_page_ram_index, num_pages, nullptr);
  m_physical_memory_address_mask = u32((u64(1) << memory_address_bits) - 1);
  m_num_physical_memory_pages = num_pages;
}

PhysicalMemoryAddress Bus::GetTotalRAMInPageRange(u32 start_page, u32 end_page) const
{
  PhysicalMemoryAddress size = 0;
  for (u32 i = start_page; i < end_page; i++)
  {
    const PhysicalMemoryPage& page = m_physical_memory_pages[i];
    if (page.ram_ptr && !(page.type & PhysicalMemoryPage::kMirror))
      size += MEMORY_PAGE_SIZE;
  }

  return size;
}

void Bus::AllocateRAM(u32 size)
{
  DebugAssert(size > 0 && !m_ram_ptr);
  Assert((size % MEMORY_PAGE_SIZE) == 0);
  m_ram_ptr = new byte[size];
  m_ram_size = size;
  m_ram_assigned = 0;
  std::memset(m_ram_ptr, 0x00, m_ram_size);
}

u32 Bus::CreateRAMRegion(PhysicalMemoryAddress start, PhysicalMemoryAddress end)
{
  Assert((start % MEMORY_PAGE_SIZE) == 0 && (u64(end + 1) % MEMORY_PAGE_SIZE) == 0);

  u32 allocated_ram = 0;
  u32 remaining_ram = GetUnassignedRAMSize();
  u32 start_page = start / MEMORY_PAGE_SIZE;
  u32 end_page = Truncate32((u64(end) + 1) / MEMORY_PAGE_SIZE);
  for (u32 current_page = start_page; current_page < end_page && remaining_ram > 0; current_page++)
  {
    PhysicalMemoryPage* page = &m_physical_memory_pages[current_page];
    Assert(!page->ram_ptr && remaining_ram >= MEMORY_PAGE_SIZE);
    page->ram_ptr = m_ram_ptr + m_ram_assigned;
    page->type = PhysicalMemoryPage::kReadableRAM | PhysicalMemoryPage::kWritableRAM;
    m_physical_memory_page_ram_index[current_page] = page->ram_ptr;
    m_ram_assigned += MEMORY_PAGE_SIZE;
    allocated_ram += MEMORY_PAGE_SIZE;
    remaining_ram -= MEMORY_PAGE_SIZE;
  }

  return allocated_ram;
}

bool Bus::CreateROMRegionFromFile(const char* filename, u32 file_offset, PhysicalMemoryAddress address,
                                  u32 expected_size /* = 0 */)
{
  auto data = System::ReadFileToBuffer(filename, file_offset, expected_size);
  if (!data.first)
    return false;

  ROMRegion rr;
  rr.data = std::move(data.first);
  rr.size = data.second;
  rr.mapped_address = address;
  rr.mmio_handler = MMIO::CreateDirect(address, rr.size, rr.data.get(), true, false, true);
  ConnectMMIO(rr.mmio_handler);
  m_rom_regions.push_back(std::move(rr));
  return true;
}

bool Bus::CreateROMRegionFromBuffer(const void* buffer, u32 size, PhysicalMemoryAddress address)
{
  ROMRegion rr;
  rr.data = std::make_unique<byte[]>(size);
  rr.size = size;
  rr.mapped_address = address;
  std::memcpy(rr.data.get(), buffer, size);

  rr.mmio_handler = MMIO::CreateDirect(address, size, rr.data.get(), true, false, true);
  ConnectMMIO(rr.mmio_handler);

  m_rom_regions.push_back(std::move(rr));
  return true;
}

void Bus::MirrorRegion(PhysicalMemoryAddress start, u32 size, PhysicalMemoryAddress mirror_start)
{
  u32 start_page = start / MEMORY_PAGE_SIZE;
  u32 end_page = Truncate32((u64(start) + u64(size) + u64(1)) / MEMORY_PAGE_SIZE);
  u32 mirror_start_page = mirror_start / MEMORY_PAGE_SIZE;

  for (u32 current_src_page = start_page, current_dst_page = mirror_start_page; current_src_page < end_page;
       current_src_page++, current_dst_page++)
  {
    PhysicalMemoryPage* src_page = &m_physical_memory_pages[current_src_page];
    PhysicalMemoryPage* dst_page = &m_physical_memory_pages[current_dst_page];
    if (src_page->ram_ptr)
    {
      // Mirror RAM pointer.
      dst_page->ram_ptr = src_page->ram_ptr;
      dst_page->type |= (src_page->type & (PhysicalMemoryPage::kReadableRAM | PhysicalMemoryPage::kWritableRAM)) |
                        PhysicalMemoryPage::kMirror;
    }

    if (src_page->mmio_handler)
    {
      // Mirror MMIO handler.
      const MMIO* existing_mmio_handler = src_page->mmio_handler;

      // Offset the mirrored handler into the existing handler.
      const PhysicalMemoryAddress src_page_start_address = current_src_page * MEMORY_PAGE_SIZE;
      const PhysicalMemoryAddress dst_page_start_address = current_dst_page * MEMORY_PAGE_SIZE;
      PhysicalMemoryAddress mmio_start_address;
      if (existing_mmio_handler->GetStartAddress() >= src_page_start_address)
      {
        // Starts past the beginning of this page, e.g. C400 on page C000. We should map it to DST:OFFSET, e.g. F400.
        const PhysicalMemoryAddress offset = existing_mmio_handler->GetStartAddress() - src_page_start_address;
        mmio_start_address = dst_page_start_address + offset;
      }
      else
      {
        // Page-aligned, or this is not the first page.
        const PhysicalMemoryAddress offset = src_page_start_address - existing_mmio_handler->GetStartAddress();
        mmio_start_address = dst_page_start_address - offset;
      }

      if (dst_page->mmio_handler)
        dst_page->mmio_handler->Release();

      dst_page->mmio_handler =
        MMIO::CreateMirror(mmio_start_address, existing_mmio_handler->GetSize(), existing_mmio_handler);
      dst_page->type |= PhysicalMemoryPage::kMirror;
    }
  }
}

template<typename T>
void Bus::EnumeratePagesForRange(PhysicalMemoryAddress start_address, PhysicalMemoryAddress end_address, T callback)
{
  DebugAssert(end_address > start_address);

  u32 first_page = start_address / MEMORY_PAGE_SIZE;
  u32 last_page = end_address / MEMORY_PAGE_SIZE;

  for (u32 i = first_page; i <= last_page; i++)
  {
    if (i < m_num_physical_memory_pages)
      callback(i, &m_physical_memory_pages[i]);
  }
}

void Bus::ConnectMMIO(MMIO* mmio)
{
  // MMIO size should be 4 byte aligned, that way we don't have to split reads/writes.
  Assert((mmio->GetSize() & u32(sizeof(u32) - 1)) == 0);

  auto callback = [this, mmio](u32 page_number, PhysicalMemoryPage* page) {
    if (page->IsReadableRAM() || page->IsWritableRAM())
    {
      Log_WarningPrintf("Registering MMIO region 0x%08X in RAM area, MMIO will be ignored",
                        page_number * MEMORY_PAGE_SIZE);
    }
    if (page->mmio_handler)
    {
      Log_WarningPrintf("Page %08X already has a MMIO handler, ignoring new handler",
                        u32(page_number * MEMORY_PAGE_SIZE));
      return;
    }

    page->mmio_handler = mmio;
    mmio->AddRef();
  };

  EnumeratePagesForRange(mmio->GetStartAddress(), mmio->GetEndAddress(), std::move(callback));
}

void Bus::DisconnectMMIO(MMIO* mmio)
{
  auto callback = [this, mmio](u32 page_number, PhysicalMemoryPage* page) {
    if (page->mmio_handler != mmio)
      return;

    page->mmio_handler = nullptr;
    mmio->Release();
  };

  EnumeratePagesForRange(mmio->GetStartAddress(), mmio->GetEndAddress(), std::move(callback));
}

bool Bus::IsCachablePage(const PhysicalMemoryPage& page)
{
  if (page.IsReadableRAM())
    return true;

  if (page.IsReadableMMIO())
    return page.mmio_handler->IsCachable();

  return false;
}

bool Bus::IsCachablePage(PhysicalMemoryAddress address) const
{
  u32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);
  return IsCachablePage(m_physical_memory_pages[page_number]);
}

bool Bus::IsWritablePage(const PhysicalMemoryPage& page)
{
  if (page.IsWritableRAM() || page.IsWritableMMIO())
    return true;

  return false;
}

bool Bus::IsWritablePage(PhysicalMemoryAddress address) const
{
  u32 page_number = address / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);
  return IsWritablePage(m_physical_memory_pages[page_number]);
}

void Bus::MarkPageAsCode(PhysicalMemoryAddress address)
{
  u32 page_number = address / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);
  m_physical_memory_pages[page_number].type |= PhysicalMemoryPage::kCachedCode;
  m_physical_memory_page_ram_index[page_number] = nullptr;
}

void Bus::UnmarkPageAsCode(PhysicalMemoryAddress address)
{
  u32 page_number = address / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);

  PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
  page.type &= ~PhysicalMemoryPage::kCachedCode;
  if (page.IsReadableWritableRAM())
    m_physical_memory_page_ram_index[page_number] = page.ram_ptr;
}

void Bus::ClearPageCodeFlags()
{
  for (u32 i = 0; i < m_num_physical_memory_pages; i++)
  {
    PhysicalMemoryPage& page = m_physical_memory_pages[i];
    if (!(page.type & PhysicalMemoryPage::kCachedCode))
      continue;

    page.type &= ~PhysicalMemoryPage::kCachedCode;
    if (page.IsReadableWritableRAM())
      m_physical_memory_page_ram_index[i] = page.ram_ptr;
  }
}

void Bus::SetCodeInvalidationCallback(CodeInvalidateCallback callback)
{
  m_code_invalidate_callback = std::move(callback);
}

void Bus::ClearCodeInvalidationCallback()
{
  m_code_invalidate_callback = [](PhysicalMemoryAddress) {};
}

void Bus::SetPageRAMState(PhysicalMemoryAddress page_address, bool readable_memory, bool writable_memory)
{
  const u32 page_number = page_address / MEMORY_PAGE_SIZE;
  PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
  DebugAssert((page_address % MEMORY_PAGE_SIZE) == 0);

  // RAM flags are ignored when there is no RAM.
  if (!page.ram_ptr)
    return;

  // If it's code, we need to invalidate it.
  // TODO: This is only really required if we change states..
  if (page.type & PhysicalMemoryPage::kCachedCode)
    m_code_invalidate_callback(page_address & MEMORY_PAGE_MASK);

  if (readable_memory)
    page.type |= PhysicalMemoryPage::kReadableRAM;
  else
    page.type &= ~PhysicalMemoryPage::kReadableRAM;
  if (writable_memory)
    page.type |= PhysicalMemoryPage::kWritableRAM;
  else
    page.type &= ~PhysicalMemoryPage::kWritableRAM;

  m_physical_memory_page_ram_index[page_number] = page.IsReadableWritableRAM() ? page.ram_ptr : nullptr;
}

void Bus::SetPagesRAMState(PhysicalMemoryAddress start_address, u32 size, bool readable_memory, bool writable_memory)
{
  const u32 num_pages = (size + (MEMORY_PAGE_SIZE - 1)) / MEMORY_PAGE_SIZE;
  DebugAssert((start_address % MEMORY_PAGE_SIZE) == 0);

  u32 current_page = start_address & MEMORY_PAGE_MASK;
  for (u32 i = 0; i < num_pages; i++, current_page += MEMORY_PAGE_SIZE)
    SetPageRAMState(current_page, readable_memory, writable_memory);
}

void Bus::Stall(SimulationTime time)
{
  Log_DebugPrintf("Stalling bus for %" PRId64 " ns", time);
  m_system->AddSimulationTime(time);
  m_system->UpdateCPUDowncount();
}

Bus::CodeHashType Bus::GetCodeHash(PhysicalMemoryAddress address, u32 length)
{
  XXH64_state_t state;
  XXH64_reset(&state, 0x42);

  std::array<byte, MEMORY_PAGE_SIZE> buf;
  while (length > 0)
  {
    u32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
    u32 page_offset = (address & m_physical_memory_address_mask) % MEMORY_PAGE_SIZE;
    DebugAssert(page_number < m_num_physical_memory_pages);

    // Fast path?
    const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
    const u32 hash_size = std::min(length, MEMORY_PAGE_SIZE);
    if (page.type & PhysicalMemoryPage::kReadableRAM)
    {
      XXH64_update(&state, page.ram_ptr + page_offset, hash_size);
      address += hash_size;
      length -= hash_size;
      continue;
    }

    if (page.mmio_handler)
    {
      // Slow path for MMIO.
      MMIO* const handler = page.mmio_handler;
      const u32 page_base_address = page_number * MEMORY_PAGE_SIZE;
      const u32 start_address = handler->GetStartAddress();
      const u32 end_address = handler->GetEndAddress();
      if (address >= start_address && (address + hash_size) <= end_address)
      {
        handler->ReadBlock(address, hash_size, buf.data());
        XXH64_update(&state, buf.data(), hash_size);
        address += hash_size;
        length -= hash_size;
        continue;
      }

      // Super slow path - when the MMIO doesn't cover the entire page.
      const u32 mmio_size_in_page = end_address - page_base_address;
      const u32 mmio_usable_size = mmio_size_in_page - page_offset;
      const u32 padding_before = (start_address >= address) ? (start_address - address) : 0;
      const u32 padding_after = (mmio_usable_size < hash_size) ? (hash_size - mmio_usable_size) : 0;
      const u32 copy_size = std::min(mmio_usable_size, hash_size);
      if (padding_before > 0)
      {
        std::memset(buf.data(), 0xFF, padding_before);
        address += padding_before;
        length -= padding_before;
      }
      if (copy_size > 0)
      {
        handler->ReadBlock(address, copy_size, buf.data() + padding_before);
        address += copy_size;
        length -= copy_size;
      }
      if (padding_after > 0)
      {
        std::memset(buf.data() + padding_before + copy_size, 0xFF, padding_after);
        address += padding_after;
        length -= padding_after;
      }
    }
    else
    {
      // Not valid memory.
      std::memset(buf.data(), 0xFF, hash_size);
      address += hash_size;
      length -= hash_size;
    }

    XXH64_update(&state, buf.data(), hash_size);
  }

  return XXH64_digest(&state);
}

bool Bus::IsReadableAddress(PhysicalMemoryAddress address, u32 size) const
{
  const u32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);

  const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
  if (page.type & PhysicalMemoryPage::kReadableRAM)
    return true;

  if (page.mmio_handler && address >= page.mmio_handler->GetStartAddress() &&
      (address + size - 1) <= page.mmio_handler->GetEndAddress())
  {
    return true;
  }

  return false;
}

bool Bus::IsWritableAddress(PhysicalMemoryAddress address, u32 size) const
{
  const u32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);

  const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
  if (page.type & PhysicalMemoryPage::kReadableRAM)
    return true;

  if (page.mmio_handler && address >= page.mmio_handler->GetStartAddress() &&
      (address + size - 1) <= page.mmio_handler->GetEndAddress())
  {
    return true;
  }

  return false;
}

bool Bus::CheckedReadMemoryByte(PhysicalMemoryAddress address, u8* value)
{
  if (!IsReadableAddress(address, sizeof(*value)))
  {
    *value = UINT8_C(0xFF);
    return false;
  }

  *value = ReadMemoryByte(address);
  return true;
}

bool Bus::CheckedWriteMemoryByte(PhysicalMemoryAddress address, u8 value)
{
  if (!IsWritableAddress(address, sizeof(value)))
    return false;

  WriteMemoryByte(address, value);
  return true;
}

bool Bus::CheckedReadMemoryWord(PhysicalMemoryAddress address, u16* value)
{
  if ((address & MEMORY_PAGE_MASK) != ((address + (sizeof(*value) - 1)) & MEMORY_PAGE_MASK))
  {
    u8 b0, b1;
    bool result = CheckedReadMemoryByte(address, &b0) & CheckedReadMemoryByte(address + 1, &b1);
    *value = (ZeroExtend16(b1) << 8) | (ZeroExtend16(b0));
    return result;
  }

  if (!IsReadableAddress(address, sizeof(*value)))
  {
    *value = UINT16_C(0xFFFF);
    return false;
  }

  *value = ReadMemoryWord(address);
  return true;
}

bool Bus::CheckedWriteMemoryWord(PhysicalMemoryAddress address, u16 value)
{
  if ((address & MEMORY_PAGE_MASK) != ((address + (sizeof(value) - 1)) & MEMORY_PAGE_MASK))
  {
    return CheckedWriteMemoryByte(address, Truncate8(value)) &
           CheckedWriteMemoryByte(address + 1, Truncate8(value >> 8));
  }

  if (!IsWritableAddress(address, sizeof(value)))
    return false;

  WriteMemoryWord(address, value);
  return true;
}

bool Bus::CheckedReadMemoryDWord(PhysicalMemoryAddress address, u32* value)
{
  if ((address & MEMORY_PAGE_MASK) != ((address + (sizeof(*value) - 1)) & MEMORY_PAGE_MASK))
  {
    u16 d0, d1;
    bool result = CheckedReadMemoryWord(address, &d0) & CheckedReadMemoryWord(address + 2, &d1);
    *value = (ZeroExtend32(d1) << 16) | (ZeroExtend32(d0));
    return result;
  }

  if (!IsReadableAddress(address, sizeof(*value)))
  {
    *value = UINT32_C(0xFFFFFFFF);
    return false;
  }

  *value = ReadMemoryDWord(address);
  return true;
}

bool Bus::CheckedWriteMemoryDWord(PhysicalMemoryAddress address, u32 value)
{
  if ((address & MEMORY_PAGE_MASK) != ((address + (sizeof(value) - 1)) & MEMORY_PAGE_MASK))
  {
    return CheckedWriteMemoryWord(address, Truncate16(value)) &
           CheckedWriteMemoryWord(address + 2, Truncate16(value >> 16));
  }

  if (!IsWritableAddress(address, sizeof(value)))
    return false;

  WriteMemoryDWord(address, value);
  return true;
}

bool Bus::CheckedReadMemoryQWord(PhysicalMemoryAddress address, u64* value)
{
  if ((address & MEMORY_PAGE_MASK) != ((address + (sizeof(*value) - 1)) & MEMORY_PAGE_MASK))
  {
    u32 d0, d1;
    bool result = CheckedReadMemoryDWord(address, &d0) & CheckedReadMemoryDWord(address + 4, &d1);
    *value = (ZeroExtend64(d1) << 32) | (ZeroExtend32(d0));
    return result;
  }

  if (!IsReadableAddress(address, sizeof(*value)))
  {
    *value = UINT64_C(0xFFFFFFFFFFFFFFFF);
    return false;
  }

  *value = ReadMemoryQWord(address);
  return true;
}

bool Bus::CheckedWriteMemoryQWord(PhysicalMemoryAddress address, u64 value)
{
  if ((address & MEMORY_PAGE_MASK) != ((address + (sizeof(value) - 1)) & MEMORY_PAGE_MASK))
  {
    return CheckedWriteMemoryDWord(address, Truncate32(value)) &
           CheckedWriteMemoryDWord(address + 4, Truncate32(value >> 32));
  }

  if (!IsWritableAddress(address, sizeof(value)))
    return false;

  WriteMemoryQWord(address, value);
  return true;
}
