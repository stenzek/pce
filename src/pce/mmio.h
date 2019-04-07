#pragma once
#include "YBaseLib/ReferenceCounted.h"
#include "pce/system.h"

class MMIO : public ReferenceCounted
{
public:
  struct Handlers
  {
    using ReadByteHandler = std::function<void(u32 offset_from_base, u8* value)>;
    using ReadWordHandler = std::function<void(u32 offset_from_base, u16* value)>;
    using ReadDWordHandler = std::function<void(u32 offset_from_base, u32* value)>;
    using ReadQWordHandler = std::function<void(u32 offset_from_base, u64* value)>;
    using ReadBlockHandler = std::function<void(u32 offset_from_base, u32 length, void* destination)>;
    using WriteByteHandler = std::function<void(u32 offset_from_base, u8 value)>;
    using WriteWordHandler = std::function<void(u32 offset_from_base, u16 value)>;
    using WriteDWordHandler = std::function<void(u32 offset_from_base, u32 value)>;
    using WriteQWordHandler = std::function<void(u32 offset_from_base, u64 value)>;
    using WriteBlockHandler = std::function<void(u32 offset_from_base, u32 length, const void* source)>;

    void IgnoreReads();
    void IgnoreWrites();

    ReadByteHandler read_byte;
    ReadWordHandler read_word;
    ReadDWordHandler read_dword;
    ReadQWordHandler read_qword;
    ReadBlockHandler read_block;
    WriteByteHandler write_byte;
    WriteWordHandler write_word;
    WriteDWordHandler write_dword;
    WriteQWordHandler write_qword;
    WriteBlockHandler write_block;
  };

public:
  MMIO(PhysicalMemoryAddress start_address, u32 size, Handlers&& handlers, bool cacheable);
  ~MMIO();

  PhysicalMemoryAddress GetStartAddress() const { return m_start_address; }
  PhysicalMemoryAddress GetEndAddress() const { return m_end_address; }
  u32 GetSize() const { return m_size; }
  bool IsCachable() const { return m_cachable; }

  void ReadByte(PhysicalMemoryAddress address, u8* destination);
  void ReadWord(PhysicalMemoryAddress address, u16* destination);
  void ReadDWord(PhysicalMemoryAddress address, u32* destination);
  void ReadQWord(PhysicalMemoryAddress address, u64* destination);
  void ReadBlock(PhysicalMemoryAddress address, u32 length, void* destination);
  void WriteByte(PhysicalMemoryAddress address, u8 source);
  void WriteWord(PhysicalMemoryAddress address, u16 source);
  void WriteDWord(PhysicalMemoryAddress address, u32 source);
  void WriteQWord(PhysicalMemoryAddress address, u64 source);
  void WriteBlock(PhysicalMemoryAddress address, u32 length, const void* source);

  // Factory methods
  static MMIO* CreateDirect(PhysicalMemoryAddress start_address, u32 size, void* data, bool allow_read = true,
                            bool allow_write = true, bool cacheable = true);
  static MMIO* CreateComplex(PhysicalMemoryAddress start_address, u32 size, Handlers&& handlers,
                             bool cacheable = false);
  static MMIO* CreateMirror(PhysicalMemoryAddress start_address, u32 size, const MMIO* existing_handler);

private:
  static void IgnoreReadByteHandler(u32 offset_from_base, u8* value);
  static void IgnoreReadWordHandler(u32 offset_from_base, u16* value);
  static void IgnoreReadDWordHandler(u32 offset_from_base, u32* value);
  static void IgnoreReadQWordHandler(u32 offset_from_base, u64* value);
  static void IgnoreReadBlockHandler(u32 offset_from_base, u32 length, void* destination);
  static void IgnoreWriteByteHandler(u32 offset_from_base, u8 value);
  static void IgnoreWriteWordHandler(u32 offset_from_base, u16 value);
  static void IgnoreWriteDWordHandler(u32 offset_from_base, u32 value);
  static void IgnoreWriteQWordHandler(u32 offset_from_base, u64 value);
  static void IgnoreWriteBlockHandler(u32 offset_from_base, u32 length, const void* source);
  void DefaultReadWordHandler(u32 offset_from_base, u16* value);
  void DefaultReadDWordHandler(u32 offset_from_base, u32* value);
  void DefaultReadQWordHandler(u32 offset_from_base, u64* value);
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