#include "pce/mmio.h"
#include <cstring>

MMIO::MMIO(PhysicalMemoryAddress start_address, uint32 size, Handlers&& handlers, bool cacheable)
  : m_start_address(start_address), m_end_address(start_address + (size - 1)), m_size(size), m_handlers(handlers),
    m_cachable(cacheable)
{
}

MMIO::~MMIO() {}

void MMIO::ReadByte(PhysicalMemoryAddress address, uint8* destination)
{
  DebugAssert(address >= GetStartAddress() && (address + 1 - 1) <= GetEndAddress());
  m_handlers.read_byte(address - m_start_address, destination);
}

void MMIO::ReadWord(PhysicalMemoryAddress address, uint16* destination)
{
  DebugAssert(address >= GetStartAddress() && (address + 2 - 1) <= GetEndAddress());
  m_handlers.read_word(address - m_start_address, destination);
}

void MMIO::ReadDWord(PhysicalMemoryAddress address, uint32* destination)
{
  DebugAssert(address >= GetStartAddress() && (address + 4 - 1) <= GetEndAddress());
  m_handlers.read_dword(address - m_start_address, destination);
}

void MMIO::ReadQWord(PhysicalMemoryAddress address, uint64* destination)
{
  DebugAssert(address >= GetStartAddress() && (address + 8 - 1) <= GetEndAddress());
  m_handlers.read_qword(address - m_start_address, destination);
}

void MMIO::ReadBlock(PhysicalMemoryAddress address, uint32 length, void* destination)
{
  DebugAssert(address >= GetStartAddress() && (address + length - 1) <= GetEndAddress());
  m_handlers.read_block(address - m_start_address, length, destination);
}

void MMIO::WriteByte(PhysicalMemoryAddress address, uint8 source)
{
  DebugAssert(address >= GetStartAddress() && (address + 1 - 1) <= GetEndAddress());
  m_handlers.write_byte(address - m_start_address, source);
}

void MMIO::WriteWord(PhysicalMemoryAddress address, uint16 source)
{
  DebugAssert(address >= GetStartAddress() && (address + 2 - 1) <= GetEndAddress());
  m_handlers.write_word(address - m_start_address, source);
}

void MMIO::WriteDWord(PhysicalMemoryAddress address, uint32 source)
{
  DebugAssert(address >= GetStartAddress() && (address + 4 - 1) <= GetEndAddress());
  m_handlers.write_dword(address - m_start_address, source);
}

void MMIO::WriteQWord(PhysicalMemoryAddress address, uint64 source)
{
  DebugAssert(address >= GetStartAddress() && (address + 8 - 1) <= GetEndAddress());
  m_handlers.write_qword(address - m_start_address, source);
}

void MMIO::WriteBlock(PhysicalMemoryAddress address, uint32 length, const void* source)
{
  DebugAssert(address >= GetStartAddress() && (address + length - 1) <= GetEndAddress());
  m_handlers.write_block(address - m_start_address, length, source);
}

MMIO* MMIO::CreateDirect(PhysicalMemoryAddress start_address, uint32 size, void* data, bool allow_read,
                         bool allow_write, bool cacheable)
{
  char* data_base = reinterpret_cast<char*>(data);
  DebugAssert(size > 0);

  Handlers handlers;
  if (allow_read)
  {
    handlers.read_byte = [data_base](uint32 offset_from_base, uint8* value) {
      std::memcpy(value, data_base + offset_from_base, sizeof(uint8));
    };
    handlers.read_word = [data_base](uint32 offset_from_base, uint16* value) {
      std::memcpy(value, data_base + offset_from_base, sizeof(uint16));
    };
    handlers.read_dword = [data_base](uint32 offset_from_base, uint32* value) {
      std::memcpy(value, data_base + offset_from_base, sizeof(uint32));
    };
    handlers.read_qword = [data_base](uint32 offset_from_base, uint64* value) {
      std::memcpy(value, data_base + offset_from_base, sizeof(uint64));
    };
    handlers.read_block = [data_base](uint32 offset_from_base, uint32 length, void* destination) {
      std::memcpy(destination, data_base + offset_from_base, length);
    };
  }
  else
  {
    handlers.IgnoreReads();
  }

  if (allow_write)
  {
    handlers.write_byte = [data_base](uint32 offset_from_base, uint8 value) {
      std::memcpy(data_base + offset_from_base, &value, sizeof(uint8));
    };
    handlers.write_word = [data_base](uint32 offset_from_base, uint16 value) {
      std::memcpy(data_base + offset_from_base, &value, sizeof(uint16));
    };
    handlers.write_dword = [data_base](uint32 offset_from_base, uint32 value) {
      std::memcpy(data_base + offset_from_base, &value, sizeof(uint32));
    };
    handlers.write_qword = [data_base](uint32 offset_from_base, uint64 value) {
      std::memcpy(data_base + offset_from_base, &value, sizeof(uint64));
    };
    handlers.write_block = [data_base](uint32 offset_from_base, uint32 length, const void* source) {
      std::memcpy(data_base + offset_from_base, source, length);
    };
  }
  else
  {
    handlers.IgnoreWrites();
  }

  return new MMIO(start_address, size, std::move(handlers), cacheable);
}

MMIO* MMIO::CreateComplex(PhysicalMemoryAddress start_address, uint32 size, Handlers&& handlers, bool cacheable)
{
  DebugAssert(size > 0);

  MMIO* mmio = new MMIO(start_address, size, std::move(handlers), cacheable);

  // Hook up unregistered width reads/writes.
  if (!mmio->m_handlers.read_word)
    mmio->m_handlers.read_word =
      std::bind(&MMIO::DefaultReadWordHandler, mmio, std::placeholders::_1, std::placeholders::_2);
  if (!mmio->m_handlers.read_dword)
    mmio->m_handlers.read_dword =
      std::bind(&MMIO::DefaultReadDWordHandler, mmio, std::placeholders::_1, std::placeholders::_2);
  if (!mmio->m_handlers.read_qword)
    mmio->m_handlers.read_qword =
      std::bind(&MMIO::DefaultReadQWordHandler, mmio, std::placeholders::_1, std::placeholders::_2);
  if (!mmio->m_handlers.read_block)
    mmio->m_handlers.read_block = std::bind(&MMIO::DefaultReadBlockHandler, mmio, std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3);
  if (!mmio->m_handlers.write_word)
    mmio->m_handlers.write_word =
      std::bind(&MMIO::DefaultWriteWordHandler, mmio, std::placeholders::_1, std::placeholders::_2);
  if (!mmio->m_handlers.write_dword)
    mmio->m_handlers.write_dword =
      std::bind(&MMIO::DefaultWriteDWordHandler, mmio, std::placeholders::_1, std::placeholders::_2);
  if (!mmio->m_handlers.write_qword)
    mmio->m_handlers.write_qword =
      std::bind(&MMIO::DefaultWriteQWordHandler, mmio, std::placeholders::_1, std::placeholders::_2);
  if (!mmio->m_handlers.write_block)
    mmio->m_handlers.write_block = std::bind(&MMIO::DefaultWriteBlockHandler, mmio, std::placeholders::_1,
                                             std::placeholders::_2, std::placeholders::_3);

  return mmio;
}

void MMIO::Handlers::IgnoreReads()
{
  read_byte = std::bind(&IgnoreReadByteHandler, std::placeholders::_1, std::placeholders::_2);
  read_word = std::bind(&IgnoreReadWordHandler, std::placeholders::_1, std::placeholders::_2);
  read_dword = std::bind(&IgnoreReadDWordHandler, std::placeholders::_1, std::placeholders::_2);
  read_qword = std::bind(&IgnoreReadQWordHandler, std::placeholders::_1, std::placeholders::_2);
  read_block = std::bind(&IgnoreReadBlockHandler, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void MMIO::Handlers::IgnoreWrites()
{
  write_byte = std::bind(&IgnoreWriteByteHandler, std::placeholders::_1, std::placeholders::_2);
  write_word = std::bind(&IgnoreWriteWordHandler, std::placeholders::_1, std::placeholders::_2);
  write_dword = std::bind(&IgnoreWriteDWordHandler, std::placeholders::_1, std::placeholders::_2);
  write_qword = std::bind(&IgnoreWriteQWordHandler, std::placeholders::_1, std::placeholders::_2);
  write_block =
    std::bind(&IgnoreWriteBlockHandler, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void MMIO::IgnoreReadByteHandler(uint32 offset_from_base, uint8* value)
{
  *value = UINT8_C(0xFF);
}

void MMIO::IgnoreReadWordHandler(uint32 offset_from_base, uint16* value)
{
  *value = UINT16_C(0xFFFF);
}

void MMIO::IgnoreReadDWordHandler(uint32 offset_from_base, uint32* value)
{
  *value = UINT32_C(0xFFFFFFFF);
}

void MMIO::IgnoreReadQWordHandler(uint32 offset_from_base, uint64* value)
{
  *value = UINT64_C(0xFFFFFFFFFFFFFFFF);
}

void MMIO::IgnoreReadBlockHandler(uint32 offset_from_base, uint32 length, void* destination)
{
  std::memset(destination, 0xFF, length);
}

void MMIO::IgnoreWriteByteHandler(uint32 offset_from_base, uint8 value) {}

void MMIO::IgnoreWriteWordHandler(uint32 offset_from_base, uint16 value) {}

void MMIO::IgnoreWriteDWordHandler(uint32 offset_from_base, uint32 value) {}

void MMIO::IgnoreWriteQWordHandler(uint32 offset_from_base, uint64 value) {}

void MMIO::IgnoreWriteBlockHandler(uint32 offset_from_base, uint32 length, const void* source) {}

void MMIO::DefaultReadWordHandler(uint32 offset_from_base, uint16* value)
{
  uint8 b0, b1;
  m_handlers.read_byte(offset_from_base + 0, &b0);
  m_handlers.read_byte(offset_from_base + 1, &b1);
  *value = ZeroExtend16(b0) | (ZeroExtend16(b1) << 8);
}

void MMIO::DefaultReadDWordHandler(uint32 offset_from_base, uint32* value)
{
  uint16 w0, w1;
  m_handlers.read_word(offset_from_base + 0, &w0);
  m_handlers.read_word(offset_from_base + 2, &w1);
  *value = ZeroExtend32(w0) | (ZeroExtend32(w1) << 16);
}

void MMIO::DefaultReadQWordHandler(uint32 offset_from_base, uint64* value)
{
  uint32 w0, w1;
  m_handlers.read_dword(offset_from_base + 0, &w0);
  m_handlers.read_dword(offset_from_base + 4, &w1);
  *value = ZeroExtend64(w0) | (ZeroExtend64(w1) << 32);
}

void MMIO::DefaultReadBlockHandler(uint32 offset_from_base, uint32 length, void* destination)
{
  byte* destination_ptr = reinterpret_cast<byte*>(destination);

  // Align to DWORD.
  while ((offset_from_base & 3) != 0 && length > 0)
  {
    byte value;
    m_handlers.read_byte(offset_from_base++, &value);
    *(destination_ptr++) = value;
    length--;
  }

  // Issue DWORD reads.
  while (length > sizeof(uint32))
  {
    uint32 value;
    m_handlers.read_dword(offset_from_base, &value);
    std::memcpy(destination_ptr, &value, sizeof(value));
    destination_ptr += sizeof(value);
    offset_from_base += sizeof(value);
    length -= sizeof(value);
  }

  // Issue byte reads until the end.
  while (length > 0)
  {
    byte value;
    m_handlers.read_byte(offset_from_base++, &value);
    *(destination_ptr++) = value;
    length--;
  }
}

void MMIO::DefaultWriteWordHandler(uint32 offset_from_base, uint16 value)
{
  m_handlers.write_byte(offset_from_base + 0, Truncate8(value));
  m_handlers.write_byte(offset_from_base + 1, Truncate8(value >> 8));
}

void MMIO::DefaultWriteDWordHandler(uint32 offset_from_base, uint32 value)
{
  m_handlers.write_word(offset_from_base + 0, Truncate16(value));
  m_handlers.write_word(offset_from_base + 2, Truncate16(value >> 16));
}

void MMIO::DefaultWriteQWordHandler(uint32 offset_from_base, uint64 value)
{
  m_handlers.write_qword(offset_from_base + 0, Truncate32(value));
  m_handlers.write_qword(offset_from_base + 4, Truncate32(value >> 32));
}

void MMIO::DefaultWriteBlockHandler(uint32 offset_from_base, uint32 length, const void* source)
{
  const byte* source_ptr = reinterpret_cast<const byte*>(source);

  // Align to DWORD.
  while ((offset_from_base & 3) != 0 && length > 0)
  {
    m_handlers.write_byte(offset_from_base++, *(source_ptr++));
    length--;
  }

  // Issue DWORD writes.
  while (length > sizeof(uint32))
  {
    uint32 value;
    std::memcpy(&value, source_ptr, sizeof(value));
    m_handlers.write_dword(offset_from_base, value);
    source_ptr += sizeof(value);
    offset_from_base += sizeof(value);
    length -= sizeof(value);
  }

  // Issue byte writes until the end.
  while (length > 0)
  {
    m_handlers.write_byte(offset_from_base++, *(source_ptr++));
    length--;
  }
}
