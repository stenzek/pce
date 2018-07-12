#define XXH_STATIC_LINKING_ONLY

#include "pce/bus.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
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
  for (auto& it : m_rom_regions)
    SAFE_RELEASE(it.mmio_handler);

  delete[] m_physical_memory_pages;
  delete[] m_ram_ptr;
}

bool Bus::Initialize(System* system, Bus* bus)
{
  m_system = system;
  return true;
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

void Bus::CheckForMemoryBreakpoint(PhysicalMemoryAddress address, uint32 size, bool is_write, uint32 value)
{
#if 0
  static const uint32 check_addresses[] = {0x00000f34, 2, 0xF96, 2};

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

void Bus::ReadMemoryBlock(PhysicalMemoryAddress address, uint32 length, void* destination)
{
  byte* destination_ptr = reinterpret_cast<byte*>(destination);

  while (length > 0)
  {
    uint32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
    uint32 page_offset = (address & m_physical_memory_address_mask) % MEMORY_PAGE_SIZE;
    DebugAssert(page_number < m_num_physical_memory_pages);

    // Fast path?
    const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
    const uint32 size_in_page = std::min(length, MEMORY_PAGE_SIZE);
    if (page.type & PhysicalMemoryPage::kReadableMemory)
    {
      std::memcpy(destination_ptr, page.ram_ptr + page_offset, size_in_page);
      destination_ptr += size_in_page;
      address += size_in_page;
      length -= size_in_page;
      continue;
    }

    if (page.type & PhysicalMemoryPage::kReadableMMIO)
    {
      // Slow path for MMIO.
      MMIO* const handler = page.mmio_handler;
      const uint32 page_base_address = page_number * MEMORY_PAGE_SIZE;
      const uint32 start_address = handler->GetStartAddress();
      const uint32 end_address = handler->GetEndAddress();
      if (address >= start_address && (address + size_in_page) <= end_address)
      {
        handler->ReadBlock(address, size_in_page, destination_ptr);
        destination_ptr += size_in_page;
        address += size_in_page;
        length -= size_in_page;
        continue;
      }

      // Super slow path - when the MMIO doesn't cover the entire page.
      const uint32 mmio_size_in_page = end_address - page_base_address;
      const uint32 mmio_usable_size = mmio_size_in_page - page_offset;
      const uint32 padding_before = (start_address >= address) ? (start_address - address) : 0;
      const uint32 padding_after = (mmio_usable_size < size_in_page) ? (size_in_page - mmio_usable_size) : 0;
      const uint32 copy_size = std::min(mmio_usable_size, size_in_page);
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

void Bus::WriteMemoryBlock(PhysicalMemoryAddress address, uint32 length, const void* source)
{
  const byte* source_ptr = reinterpret_cast<const byte*>(source);

  while (length > 0)
  {
    uint32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
    uint32 page_offset = (address & m_physical_memory_address_mask) % MEMORY_PAGE_SIZE;
    DebugAssert(page_number < m_num_physical_memory_pages);

    // Fast path?
    const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
    const uint32 size_in_page = std::min(length, MEMORY_PAGE_SIZE);
    if (page.type & PhysicalMemoryPage::kWritableMemory)
    {
      std::memcpy(page.ram_ptr + page_offset, source_ptr, size_in_page);
      source_ptr += size_in_page;
      address += size_in_page;
      length -= size_in_page;
      continue;
    }

    if (page.type & PhysicalMemoryPage::kWritableMMIO)
    {
      // Slow path for MMIO.
      MMIO* const handler = page.mmio_handler;
      const uint32 page_base_address = page_number * MEMORY_PAGE_SIZE;
      const uint32 start_address = handler->GetStartAddress();
      const uint32 end_address = handler->GetEndAddress();
      if (address >= start_address && (address + size_in_page) <= end_address)
      {
        handler->WriteBlock(address, size_in_page, source_ptr);
        source_ptr += size_in_page;
        address += size_in_page;
        length -= size_in_page;
        continue;
      }

      // Super slow path - when the MMIO doesn't cover the entire page.
      const uint32 mmio_size_in_page = end_address - page_base_address;
      const uint32 mmio_usable_size = mmio_size_in_page - page_offset;
      const uint32 padding_before = (start_address >= address) ? (start_address - address) : 0;
      const uint32 padding_after = (mmio_usable_size < size_in_page) ? (size_in_page - mmio_usable_size) : 0;
      const uint32 copy_size = std::min(mmio_usable_size, size_in_page);
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

byte* Bus::GetRAMPointer(PhysicalMemoryAddress address)
{
  const uint32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
  const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
  return (page.type & PhysicalMemoryPage::kReadableMemory) ? (page.ram_ptr + (address & MEMORY_PAGE_OFFSET_MASK)) :
                                                             nullptr;
}

void Bus::AllocateMemoryPages(uint32 memory_address_bits)
{
  uint32 num_pages = uint32((uint64(1) << memory_address_bits) / uint64(MEMORY_PAGE_SIZE));

  // Allocate physical pages
  DebugAssert(num_pages > 0);
  m_physical_memory_pages = new PhysicalMemoryPage[num_pages];
  std::memset(m_physical_memory_pages, 0, sizeof(PhysicalMemoryPage) * num_pages);
  m_physical_memory_address_mask = uint32((uint64(1) << memory_address_bits) - 1);
  m_num_physical_memory_pages = num_pages;
}

PhysicalMemoryAddress Bus::GetTotalRAMInPageRange(uint32 start_page, uint32 end_page) const
{
  PhysicalMemoryAddress size = 0;
  for (uint32 i = start_page; i < end_page; i++)
  {
    const PhysicalMemoryPage& page = m_physical_memory_pages[i];
    if (page.type & PhysicalMemoryPage::kRAMRegion)
      size += MEMORY_PAGE_SIZE;
  }

  return size;
}

void Bus::AllocateRAM(uint32 size)
{
  DebugAssert(size > 0 && !m_ram_ptr);
  Assert((size % MEMORY_PAGE_SIZE) == 0);
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
    Assert(!page->ram_ptr && remaining_ram >= MEMORY_PAGE_SIZE);
    page->ram_ptr = m_ram_ptr + m_ram_assigned;
    page->type =
      PhysicalMemoryPage::kReadableMemory | PhysicalMemoryPage::kWritableMemory | PhysicalMemoryPage::kRAMRegion;
    m_ram_assigned += MEMORY_PAGE_SIZE;
    allocated_ram += MEMORY_PAGE_SIZE;
    remaining_ram -= MEMORY_PAGE_SIZE;
  }

  return allocated_ram;
}

byte* Bus::CreateROMRegion(PhysicalMemoryAddress address, uint32 size)
{
  // Round size up to next page boundary.
  Assert((address % MEMORY_PAGE_SIZE) == 0);
  ROMRegion rr;
  rr.mapped_address = address;
  rr.size = (size + (MEMORY_PAGE_SIZE - 1)) & ~(MEMORY_PAGE_SIZE - 1);
  rr.data = std::make_unique<byte[]>(rr.size);

  // Map to address space.
  uint32 start_page = rr.mapped_address / MEMORY_PAGE_SIZE;
  uint32 end_page = Truncate32((uint64(rr.mapped_address) + uint64(rr.size) + uint64(1)) / MEMORY_PAGE_SIZE);
  byte* rom_start_ptr = rr.data.get();
  byte* rom_current_ptr = rr.data.get();
  for (uint32 current_page = start_page; current_page < end_page; current_page++)
  {
    PhysicalMemoryPage* page = &m_physical_memory_pages[current_page];
    page->ram_ptr = rom_current_ptr;
    page->type = PhysicalMemoryPage::kReadableMemory | PhysicalMemoryPage::kROMRegion;
    rom_current_ptr += MEMORY_PAGE_SIZE;
  }

  m_rom_regions.push_back(std::move(rr));
  return rom_start_ptr;
}

bool Bus::CreateROMRegionFromFile(const char* filename, PhysicalMemoryAddress address, uint32 expected_size)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &stream))
  {
    Log_ErrorPrintf("Failed to open ROM file: %s", filename);
    return false;
  }

  const uint32 size = Truncate32(stream->GetSize());
  if (expected_size != 0 && stream->GetSize() != expected_size)
  {
    Log_ErrorPrintf("ROM file %s mismatch - expected %u bytes, got %u bytes", filename, expected_size, size);
    stream->Release();
    return false;
  }

  byte* ptr = CreateROMRegion(address, size);
  AssertMsg(ptr, "allocating ROM region");

  if (!stream->Read2(ptr, size))
  {
    Log_ErrorPrintf("Failed to read %u bytes from ROM file %s", size, filename);
    stream->Release();
    return false;
  }

  stream->Release();
  return true;
}

bool Bus::CreateROMRegionFromBuffer(const void* buffer, uint32 size, PhysicalMemoryAddress address)
{
  byte* ptr = CreateROMRegion(address, size);
  AssertMsg(ptr, "allocating ROM region");
  std::memcpy(ptr, buffer, size);
  return true;
}

bool Bus::CreateMMIOROMRegionFromFile(const char* filename, PhysicalMemoryAddress address, uint32 expected_size /*= 0*/)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &stream))
  {
    Log_ErrorPrintf("Failed to open ROM file: %s", filename);
    return false;
  }

  const uint32 size = Truncate32(stream->GetSize());
  if (expected_size != 0 && stream->GetSize() != expected_size)
  {
    Log_ErrorPrintf("ROM file %s mismatch - expected %u bytes, got %u bytes", filename, expected_size, size);
    stream->Release();
    return false;
  }

  ROMRegion rr;
  rr.data = std::make_unique<byte[]>(size);
  rr.size = size;
  rr.mapped_address = address;
  if (!stream->Read2(rr.data.get(), size))
  {
    Log_ErrorPrintf("Failed to read %u bytes from ROM file %s", size, filename);
    stream->Release();
    return false;
  }

  rr.mmio_handler = MMIO::CreateDirect(address, size, rr.data.get(), true, false, true);
  RegisterMMIO(rr.mmio_handler);
  m_rom_regions.push_back(std::move(rr));

  stream->Release();
  return true;
}

bool Bus::CreateMMIOROMRegionFromBuffer(const void* buffer, uint32 size, PhysicalMemoryAddress address)
{
  ROMRegion rr;
  rr.data = std::make_unique<byte[]>(size);
  rr.size = size;
  rr.mapped_address = address;
  std::memcpy(rr.data.get(), buffer, size);

  rr.mmio_handler = MMIO::CreateDirect(address, size, rr.data.get(), true, false, true);
  RegisterMMIO(rr.mmio_handler);

  m_rom_regions.push_back(std::move(rr));
  return true;
}

void Bus::MirrorRegion(PhysicalMemoryAddress start, uint32 size, PhysicalMemoryAddress mirror_start)
{
  uint32 start_page = start / MEMORY_PAGE_SIZE;
  uint32 end_page = Truncate32((uint64(start) + uint64(size) + uint64(1)) / MEMORY_PAGE_SIZE);
  uint32 mirror_start_page = mirror_start / MEMORY_PAGE_SIZE;

  for (uint32 current_src_page = start_page, current_dst_page = mirror_start_page; current_src_page < end_page;
       current_src_page++, current_dst_page++)
  {
    PhysicalMemoryPage* src_page = &m_physical_memory_pages[current_src_page];
    PhysicalMemoryPage* dst_page = &m_physical_memory_pages[current_dst_page];
    if ((src_page->type & (PhysicalMemoryPage::kRAMRegion | PhysicalMemoryPage::kROMRegion)) == 0)
      continue;

    Assert(dst_page->type == 0);
    dst_page->type = src_page->type;
    dst_page->ram_ptr = src_page->ram_ptr;
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

static MMIO* CreateMMIOSplitter(MMIO* mmio1, MMIO* mmio2)
{
  Assert("implement me");
  return nullptr;
}

void Bus::RegisterMMIO(MMIO* mmio)
{
  // MMIO size should be 4 byte aligned, that way we don't have to split reads/writes.
  Assert((mmio->GetSize() & uint32(sizeof(uint32) - 1)) == 0);

  auto callback = [this, mmio](uint32 page_number, PhysicalMemoryPage* page) {
    if (page->mmio_handler)
    {
      MMIO* splitter = CreateMMIOSplitter(page->mmio_handler, mmio);
      page->mmio_handler->Release();
      page->mmio_handler = splitter;
      return;
    }

    if (page->type & (PhysicalMemoryPage::kReadableMemory | PhysicalMemoryPage::kWritableMemory))
    {
      Log_WarningPrintf("Unmapping RAM page at address 0x%08X for MMIO page", unsigned(page_number * MEMORY_PAGE_SIZE));
      page->type &= ~(PhysicalMemoryPage::kReadableMemory | PhysicalMemoryPage::kWritableMemory);
    }

    page->type |= PhysicalMemoryPage::kReadableMMIO | PhysicalMemoryPage::kWritableMMIO;
    page->mmio_handler = mmio;
    mmio->AddRef();
  };

  EnumeratePagesForRange(mmio->GetStartAddress(), mmio->GetEndAddress(), callback);
}

bool Bus::IsCachablePage(const PhysicalMemoryPage& page)
{
  if (page.type & PhysicalMemoryPage::kReadableMemory)
    return true;

  if (page.type & PhysicalMemoryPage::kReadableMMIO)
    return page.mmio_handler->IsCachable();

  return false;
}

bool Bus::IsCachablePage(PhysicalMemoryAddress address) const
{
  uint32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);
  return IsCachablePage(m_physical_memory_pages[page_number]);
}

bool Bus::IsWritablePage(const PhysicalMemoryPage& page)
{
  if (page.type & (PhysicalMemoryPage::kWritableMemory | PhysicalMemoryPage::kWritableMMIO))
    return true;

  return false;
}

bool Bus::IsWritablePage(PhysicalMemoryAddress address) const
{
  uint32 page_number = address / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);
  return IsWritablePage(m_physical_memory_pages[page_number]);
}

void Bus::MarkPageAsCode(PhysicalMemoryAddress address)
{
  uint32 page_number = address / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);
  m_physical_memory_pages[page_number].type |= PhysicalMemoryPage::kCachedCode;
}

void Bus::UnmarkPageAsCode(PhysicalMemoryAddress address)
{
  uint32 page_number = address / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);
  m_physical_memory_pages[page_number].type &= ~PhysicalMemoryPage::kCachedCode;
}

void Bus::ClearPageCodeFlags()
{
  for (uint32 i = 0; i < m_num_physical_memory_pages; i++)
    m_physical_memory_pages[i].type &= ~PhysicalMemoryPage::kCachedCode;
}

void Bus::SetCodeInvalidationCallback(CodeInvalidateCallback callback)
{
  m_code_invalidate_callback = std::move(callback);
}

void Bus::ClearCodeInvalidationCallback()
{
  m_code_invalidate_callback = [](PhysicalMemoryAddress) {};
}

void Bus::SetPageMemoryState(PhysicalMemoryAddress page_address, bool readable_memory, bool writable_memory)
{
  PhysicalMemoryPage& page = m_physical_memory_pages[page_address / MEMORY_PAGE_SIZE];
  DebugAssert((page_address % MEMORY_PAGE_SIZE) == 0);

  // If it's code, we need to invalidate it.
  // TODO: This is only really required if we change states..
  if (page.type & PhysicalMemoryPage::kCachedCode)
    m_code_invalidate_callback(page_address & MEMORY_PAGE_MASK);

  if (readable_memory)
  {
    page.type &= ~(PhysicalMemoryPage::kReadableMMIO);
    if (page.ram_ptr && page.type & (PhysicalMemoryPage::kRAMRegion | PhysicalMemoryPage::kROMRegion))
      page.type |= PhysicalMemoryPage::kReadableMemory;
  }
  else
  {
    page.type &= ~(PhysicalMemoryPage::kReadableMemory);
    if (page.mmio_handler)
      page.type |= PhysicalMemoryPage::kReadableMMIO;
  }

  if (writable_memory)
  {
    // Only enable writes if it's RAM, not virtual ROM.
    page.type &= ~(PhysicalMemoryPage::kWritableMMIO);
    if (page.ram_ptr && page.type & PhysicalMemoryPage::kRAMRegion)
      page.type |= PhysicalMemoryPage::kWritableMemory;
  }
  else
  {
    page.type &= ~(PhysicalMemoryPage::kWritableMemory);
    if (page.mmio_handler)
      page.type |= PhysicalMemoryPage::kWritableMMIO;
  }
}

void Bus::SetPagesMemoryState(PhysicalMemoryAddress start_address, uint32 size, bool readable_memory,
                              bool writable_memory)
{
  const uint32 num_pages = (size + (MEMORY_PAGE_SIZE - 1)) / MEMORY_PAGE_SIZE;
  DebugAssert((start_address % MEMORY_PAGE_SIZE) == 0);

  uint32 current_page = start_address & MEMORY_PAGE_MASK;
  for (uint32 i = 0; i < num_pages; i++, current_page += MEMORY_PAGE_SIZE)
    SetPageMemoryState(current_page, readable_memory, writable_memory);
}

Bus::CodeHashType Bus::GetCodeHash(PhysicalMemoryAddress address, uint32 length)
{
  XXH64_state_t state;
  XXH64_reset(&state, 0x42);

  std::array<byte, MEMORY_PAGE_SIZE> buf;
  while (length > 0)
  {
    uint32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
    uint32 page_offset = (address & m_physical_memory_address_mask) % MEMORY_PAGE_SIZE;
    DebugAssert(page_number < m_num_physical_memory_pages);

    // Fast path?
    const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
    const uint32 hash_size = std::min(length, MEMORY_PAGE_SIZE);
    if (page.type & PhysicalMemoryPage::kReadableMemory)
    {
      XXH64_update(&state, page.ram_ptr + page_offset, hash_size);
      address += hash_size;
      length -= hash_size;
      continue;
    }

    if (page.type & PhysicalMemoryPage::kReadableMMIO)
    {
      // Slow path for MMIO.
      MMIO* const handler = page.mmio_handler;
      const uint32 page_base_address = page_number * MEMORY_PAGE_SIZE;
      const uint32 start_address = handler->GetStartAddress();
      const uint32 end_address = handler->GetEndAddress();
      if (address >= start_address && (address + hash_size) <= end_address)
      {
        handler->ReadBlock(address, hash_size, buf.data());
        XXH64_update(&state, buf.data(), hash_size);
        address += hash_size;
        length -= hash_size;
        continue;
      }

      // Super slow path - when the MMIO doesn't cover the entire page.
      const uint32 mmio_size_in_page = end_address - page_base_address;
      const uint32 mmio_usable_size = mmio_size_in_page - page_offset;
      const uint32 padding_before = (start_address >= address) ? (start_address - address) : 0;
      const uint32 padding_after = (mmio_usable_size < hash_size) ? (hash_size - mmio_usable_size) : 0;
      const uint32 copy_size = std::min(mmio_usable_size, hash_size);
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

bool Bus::IsReadableAddress(PhysicalMemoryAddress address, uint32 size) const
{
  const uint32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);

  const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
  if (page.type & PhysicalMemoryPage::kReadableMemory)
    return true;

  if (page.type & PhysicalMemoryPage::kReadableMMIO && address >= page.mmio_handler->GetStartAddress() &&
      (address + size - 1) <= page.mmio_handler->GetEndAddress())
  {
    return true;
  }

  return false;
}

bool Bus::IsWritableAddress(PhysicalMemoryAddress address, uint32 size) const
{
  const uint32 page_number = (address & m_physical_memory_address_mask) / MEMORY_PAGE_SIZE;
  DebugAssert(page_number < m_num_physical_memory_pages);

  const PhysicalMemoryPage& page = m_physical_memory_pages[page_number];
  if (page.type & PhysicalMemoryPage::kWritableMemory)
    return true;

  if (page.type & PhysicalMemoryPage::kWritableMMIO && address >= page.mmio_handler->GetStartAddress() &&
      (address + size - 1) <= page.mmio_handler->GetEndAddress())
  {
    return true;
  }

  return false;
}

bool Bus::CheckedReadMemoryByte(PhysicalMemoryAddress address, uint8* value)
{
  if (!IsReadableAddress(address, sizeof(*value)))
  {
    *value = UINT8_C(0xFF);
    return false;
  }

  *value = ReadMemoryByte(address);
  return true;
}

bool Bus::CheckedWriteMemoryByte(PhysicalMemoryAddress address, uint8 value)
{
  if (!IsWritableAddress(address, sizeof(value)))
    return false;

  WriteMemoryByte(address, value);
  return true;
}

bool Bus::CheckedReadMemoryWord(PhysicalMemoryAddress address, uint16* value)
{
  if ((address & MEMORY_PAGE_MASK) != ((address + (sizeof(*value) - 1)) & MEMORY_PAGE_MASK))
  {
    uint8 b0, b1;
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

bool Bus::CheckedWriteMemoryWord(PhysicalMemoryAddress address, uint16 value)
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

bool Bus::CheckedReadMemoryDWord(PhysicalMemoryAddress address, uint32* value)
{
  if ((address & MEMORY_PAGE_MASK) != ((address + (sizeof(*value) - 1)) & MEMORY_PAGE_MASK))
  {
    uint16 d0, d1;
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

bool Bus::CheckedWriteMemoryDWord(PhysicalMemoryAddress address, uint32 value)
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

bool Bus::CheckedReadMemoryQWord(PhysicalMemoryAddress address, uint64* value)
{
  if ((address & MEMORY_PAGE_MASK) != ((address + (sizeof(*value) - 1)) & MEMORY_PAGE_MASK))
  {
    uint32 d0, d1;
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

bool Bus::CheckedWriteMemoryQWord(PhysicalMemoryAddress address, uint64 value)
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
