#pragma once
#include "YBaseLib/ReferenceCounted.h"
#include "pce/system.h"

class MMIO : public ReferenceCounted
{
public:
  struct Handlers
  {
    using ReadByteHandler = std::function<u8(u32 offset_from_base)>;
    using ReadWordHandler = std::function<u16(u32 offset_from_base)>;
    using ReadDWordHandler = std::function<u32(u32 offset_from_base)>;
    using ReadQWordHandler = std::function<u64(u32 offset_from_base)>;
    using WriteByteHandler = std::function<void(u32 offset_from_base, u8 value)>;
    using WriteWordHandler = std::function<void(u32 offset_from_base, u16 value)>;
    using WriteDWordHandler = std::function<void(u32 offset_from_base, u32 value)>;
    using WriteQWordHandler = std::function<void(u32 offset_from_base, u64 value)>;
    using ReadBlockHandler = std::function<void(u32 offset_from_base, u32 length, void* destination)>;
    using WriteBlockHandler = std::function<void(u32 offset_from_base, u32 length, const void* source)>;

    void IgnoreReads();
    void IgnoreWrites();

    ReadByteHandler read_byte;
    ReadWordHandler read_word;
    ReadDWordHandler read_dword;
    ReadQWordHandler read_qword;
    WriteByteHandler write_byte;
    WriteWordHandler write_word;
    WriteDWordHandler write_dword;
    WriteQWordHandler write_qword;

    ReadBlockHandler read_block;
    WriteBlockHandler write_block;
  };

public:
  MMIO(PhysicalMemoryAddress start_address, u32 size, Handlers&& handlers, bool cacheable);
  ~MMIO();

  PhysicalMemoryAddress GetStartAddress() const { return m_start_address; }
  PhysicalMemoryAddress GetEndAddress() const { return m_end_address; }
  u32 GetSize() const { return m_size; }
  bool IsCachable() const { return m_cachable; }

  u8 ReadByte(PhysicalMemoryAddress address) { return m_handlers.read_byte(address - m_start_address); }
  u16 ReadWord(PhysicalMemoryAddress address) { return m_handlers.read_word(address - m_start_address); }
  u32 ReadDWord(PhysicalMemoryAddress address) { return m_handlers.read_dword(address - m_start_address); }
  u64 ReadQWord(PhysicalMemoryAddress address) { return m_handlers.read_qword(address - m_start_address); }
  void WriteByte(PhysicalMemoryAddress address, u8 source) { m_handlers.write_byte(address - m_start_address, source); }
  void WriteWord(PhysicalMemoryAddress address, u16 source)
  {
    m_handlers.write_word(address - m_start_address, source);
  }
  void WriteDWord(PhysicalMemoryAddress address, u32 source)
  {
    m_handlers.write_dword(address - m_start_address, source);
  }
  void WriteQWord(PhysicalMemoryAddress address, u64 source)
  {
    m_handlers.write_qword(address - m_start_address, source);
  }
  void ReadBlock(PhysicalMemoryAddress address, u32 length, void* destination)
  {
    m_handlers.read_block(address - m_start_address, length, destination);
  }
  void WriteBlock(PhysicalMemoryAddress address, u32 length, const void* source)
  {
    m_handlers.write_block(address - m_start_address, length, source);
  }

  // Factory methods
  static MMIO* CreateDirect(PhysicalMemoryAddress start_address, u32 size, void* data, bool allow_read = true,
                            bool allow_write = true, bool cacheable = true);
  static MMIO* CreateComplex(PhysicalMemoryAddress start_address, u32 size, Handlers&& handlers,
                             bool cacheable = false);
  static MMIO* CreateMirror(PhysicalMemoryAddress start_address, u32 size, const MMIO* existing_handler);

private:
  static u8 IgnoreReadByteHandler(u32 offset_from_base);
  static u16 IgnoreReadWordHandler(u32 offset_from_base);
  static u32 IgnoreReadDWordHandler(u32 offset_from_base);
  static u64 IgnoreReadQWordHandler(u32 offset_from_base);
  static void IgnoreReadBlockHandler(u32 offset_from_base, u32 length, void* destination);
  static void IgnoreWriteByteHandler(u32 offset_from_base, u8 value);
  static void IgnoreWriteWordHandler(u32 offset_from_base, u16 value);
  static void IgnoreWriteDWordHandler(u32 offset_from_base, u32 value);
  static void IgnoreWriteQWordHandler(u32 offset_from_base, u64 value);
  static void IgnoreWriteBlockHandler(u32 offset_from_base, u32 length, const void* source);
  u16 DefaultReadWordHandler(u32 offset_from_base);
  u32 DefaultReadDWordHandler(u32 offset_from_base);
  u64 DefaultReadQWordHandler(u32 offset_from_base);
  void DefaultReadBlockHandler(u32 offset_from_base, u32 length, void* destination);
  void DefaultWriteWordHandler(u32 offset_from_base, u16 value);
  void DefaultWriteDWordHandler(u32 offset_from_base, u32 value);
  void DefaultWriteQWordHandler(u32 offset_from_base, u64 value);
  void DefaultWriteBlockHandler(u32 offset_from_base, u32 length, const void* source);

  PhysicalMemoryAddress m_start_address;
  PhysicalMemoryAddress m_end_address;
  u32 m_size;

  Handlers m_handlers;
  bool m_cachable;
};