#include "pce/systems/pcbase.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "pce/bus.h"
#include "pce/mmio.h"
Log_SetChannel(Systems::PCBase);

namespace Systems {

PCBase::PCBase(HostInterface* host_interface) : System(host_interface) {}

PCBase::~PCBase()
{
  for (const auto& it : m_roms)
    it.mmio->Release();
}

bool PCBase::AddMMIOROMFromStream(PhysicalMemoryAddress address, ByteStream* stream)
{
  uint32 length = uint32(stream->GetSize());
  ROMBlock* rom = AllocateROM(address, length);

  if (!stream->SeekAbsolute(0) || !stream->Read2(rom->data.get(), length))
  {
    Log_ErrorPrintf("Failed to read BIOS image");
    return false;
  }

  return true;
}

bool PCBase::AddMMIOROMFromFile(PhysicalMemoryAddress address, const char* filename, uint32 expected_size /* = 0 */)
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

  bool result = AddMMIOROMFromStream(address, stream);
  stream->Release();
  return result;
}

bool PCBase::AddInterleavedMMIOROMFromFile(PhysicalMemoryAddress address, ByteStream* low_stream,
                                           ByteStream* high_stream)
{
  uint32 low_length = uint32(low_stream->GetSize());
  uint32 high_length = uint32(high_stream->GetSize());
  if (low_length != high_length)
  {
    Log_ErrorPrintf("Invalid BIOS image, differing sizes (%u and %u bytes)", low_length, high_length);
    return false;
  }

  ROMBlock* rom = AllocateROM(address, low_length + high_length);

  if (!high_stream->SeekAbsolute(0) || !high_stream->SeekAbsolute(0))
  {
    Log_ErrorPrintf("Failed to read BIOS image");
    return false;
  }

  // Interleave even and odd bytes, this compensates for the 8-bit data bus per chip
  for (uint32 i = 0; i < low_length; i++)
  {
    byte even_byte, odd_byte;
    if (!low_stream->Read(&even_byte, sizeof(even_byte)) || !high_stream->Read(&odd_byte, sizeof(odd_byte)))
    {
      Log_ErrorPrintf("Failed to read BIOS image");
      return false;
    }

    rom->data[i * 2 + 0] = even_byte;
    rom->data[i * 2 + 1] = odd_byte;
  }

  return true;
}

PhysicalMemoryAddress PCBase::GetBaseMemorySize() const
{
  uint32 start_page = 0x00000000 / Bus::MEMORY_PAGE_SIZE;
  uint32 end_page = 0x000A0000 / Bus::MEMORY_PAGE_SIZE;

  return m_bus->GetTotalRAMInPageRange(start_page, end_page);
}

PhysicalMemoryAddress PCBase::GetExtendedMemorySize() const
{
  uint32 start_page = 0x00100000 / Bus::MEMORY_PAGE_SIZE;
  uint32 end_page = m_bus->GetMemoryPageCount();

  return m_bus->GetTotalRAMInPageRange(start_page, end_page);
}

PhysicalMemoryAddress PCBase::GetTotalMemorySize() const
{
  return m_bus->GetTotalRAMInPageRange(0, m_bus->GetMemoryPageCount());
}

void PCBase::AllocatePhysicalMemory(uint32 ram_size, bool reserve_isa_memory, bool reserve_uma)
{
  // Allocate RAM
  DebugAssert(ram_size > 0);
  m_bus->AllocateRAM(ram_size);

#define MAKE_RAM_REGION(start, end) m_bus->CreateRAMRegion((start), (end))

  // Allocate 640KiB conventional memory at 0x00000000-0x0009FFFF
  MAKE_RAM_REGION(0x00000000, 0x0009FFFF);

  // Is UMA reserved?
  if (!reserve_uma)
    MAKE_RAM_REGION(0x000A0000, 0x000FFFFF);

  // High memory area from 0x00100000 - 0x00EFFFFF (14MiB)
  MAKE_RAM_REGION(0x00100000, 0x00EFFFFF);

  // Reserve ISA memory hole (0x00F00000 - 0x00FFFFFF - 1MiB)?
  if (!reserve_isa_memory)
    MAKE_RAM_REGION(0x00F00000, 0x00FFFFFF);

  // Remaining extended memory up to PCI MMIO range - 0x01000000
  MAKE_RAM_REGION(0x01000000, 0x7FFFFFFFu);

#undef MAKE_RAM_REGION
}

bool PCBase::LoadSystemState(BinaryReader& reader)
{
  if (!System::LoadSystemState(reader))
    return false;

  return true;
}

bool PCBase::SaveSystemState(BinaryWriter& writer)
{
  if (!System::SaveSystemState(writer))
    return false;

  return true;
}

PCBase::ROMBlock* PCBase::AllocateROM(PhysicalMemoryAddress address, uint32 size)
{
  m_roms.emplace_back(ROMBlock());

  ROMBlock& rom = m_roms.back();
  rom.data = std::make_unique<byte[]>(size);
  Y_memzero(rom.data.get(), size);

  rom.mmio = MMIO::CreateDirect(address, size, rom.data.get(), true, false);
  m_bus->RegisterMMIO(rom.mmio);

  return &rom;
}

bool PCBase::GetA20State() const
{
  return (m_bus->GetMemoryAddressMask() & A20_BIT) != 0;
}

void PCBase::SetA20State(bool state)
{
  if (GetA20State() == state)
    return;

  Log_DevPrintf("A20 line is %s", state ? "ON" : "OFF");

  PhysicalMemoryAddress mask = m_bus->GetMemoryAddressMask();
  if (state)
    mask |= A20_BIT;
  else
    mask &= ~A20_BIT;
  m_bus->SetMemoryAddressMask(mask);
}

} // namespace Systems
