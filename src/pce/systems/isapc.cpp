#include "pce/systems/isapc.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "pce/bus.h"
#include "pce/mmio.h"
Log_SetChannel(Systems::ISAPC);

namespace Systems {
DEFINE_OBJECT_TYPE_INFO(ISAPC);
BEGIN_OBJECT_PROPERTY_MAP(ISAPC)
END_OBJECT_PROPERTY_MAP()

ISAPC::ISAPC(const ObjectTypeInfo* type_info /* = &s_type_info */) : BaseClass(type_info) {}

ISAPC::~ISAPC() = default;

bool ISAPC::LoadInterleavedROM(PhysicalMemoryAddress address, const char* low_filename, const char* high_filename)
{
  ByteStream* low_stream;
  ByteStream* high_stream;
  if (!ByteStream_OpenFileStream(low_filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &low_stream))
  {
    Log_ErrorPrintf("Failed to open code file %s", low_filename);
    return false;
  }
  if (!ByteStream_OpenFileStream(high_filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &high_stream))
  {
    Log_ErrorPrintf("Failed to open code file %s", high_filename);
    low_stream->Release();
    return false;
  }

  const u32 low_length = u32(low_stream->GetSize());
  const u32 high_length = u32(high_stream->GetSize());
  if (low_length != high_length)
  {
    Log_ErrorPrintf("Invalid BIOS image, differing sizes (%u and %u bytes)", low_length, high_length);
    return false;
  }

  // Interleave even and odd bytes, this compensates for the 8-bit data bus per chip
  std::vector<byte> data(low_length + high_length);
  for (u32 i = 0; i < low_length; i++)
  {
    byte even_byte, odd_byte;
    if (!low_stream->Read(&even_byte, sizeof(even_byte)) || !high_stream->Read(&odd_byte, sizeof(odd_byte)))
    {
      Log_ErrorPrintf("Failed to read BIOS image");
      return false;
    }

    data[i * 2 + 0] = even_byte;
    data[i * 2 + 1] = odd_byte;
  }

  high_stream->Release();
  low_stream->Release();

  return m_bus->CreateROMRegionFromBuffer(data.data(), low_length + high_length, address);
}

PhysicalMemoryAddress ISAPC::GetBaseMemorySize() const
{
  u32 start_page = 0x00000000 / Bus::MEMORY_PAGE_SIZE;
  u32 end_page = 0x000A0000 / Bus::MEMORY_PAGE_SIZE;

  return m_bus->GetTotalRAMInPageRange(start_page, end_page);
}

PhysicalMemoryAddress ISAPC::GetExtendedMemorySize() const
{
  u32 start_page = 0x00100000 / Bus::MEMORY_PAGE_SIZE;
  u32 end_page = m_bus->GetMemoryPageCount();

  return m_bus->GetTotalRAMInPageRange(start_page, end_page);
}

PhysicalMemoryAddress ISAPC::GetTotalMemorySize() const
{
  return m_bus->GetTotalRAMInPageRange(0, m_bus->GetMemoryPageCount());
}

void ISAPC::AllocatePhysicalMemory(u32 ram_size, bool reserve_isa_memory, bool reserve_uma, bool reserve_rom)
{
  // Allocate RAM
  DebugAssert(ram_size > 0);
  m_bus->AllocateRAM(ram_size);

#define MAKE_RAM_REGION(start, end) m_bus->CreateRAMRegion((start), (end))

  // Allocate 640KiB conventional memory at 0x00000000-0x0009FFFF
  MAKE_RAM_REGION(0x00000000, 0x0009FFFF);

  // Is UMA reserved?
  if (!reserve_uma)
    MAKE_RAM_REGION(0x000A0000, 0x000BFFFF);
  if (!reserve_rom)
    MAKE_RAM_REGION(0x000C0000, 0x000FFFFF);

  // High memory area from 0x00100000 - 0x00EFFFFF (14MiB)
  MAKE_RAM_REGION(0x00100000, 0x00EFFFFF);

  // Reserve ISA memory hole (0x00F00000 - 0x00FFFFFF - 1MiB)?
  if (!reserve_isa_memory)
    MAKE_RAM_REGION(0x00F00000, 0x00FFFFFF);

  // Remaining extended memory up to PCI MMIO range - 0x01000000
  MAKE_RAM_REGION(0x01000000, 0x7FFFFFFFu);

#undef MAKE_RAM_REGION
}

bool ISAPC::GetA20State() const
{
  return (m_bus->GetMemoryAddressMask() & A20_BIT) != 0;
}

void ISAPC::SetA20State(bool state)
{
  if (GetA20State() == state)
    return;

  Log_DebugPrintf("A20 line is %s", state ? "ON" : "OFF");

  PhysicalMemoryAddress mask = m_bus->GetMemoryAddressMask();
  if (state)
    mask |= A20_BIT;
  else
    mask &= ~A20_BIT;
  m_bus->SetMemoryAddressMask(mask);
}

} // namespace Systems
