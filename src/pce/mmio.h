#pragma once
#include "YBaseLib/ReferenceCounted.h"
#include "pce/system.h"

class MMIO : public ReferenceCounted
{
public:
  struct Handlers
  {
    using ReadByteHandler = std::function<void(uint32 offset_from_base, uint8* value)>;
    using ReadWordHandler = std::function<void(uint32 offset_from_base, uint16* value)>;
    using ReadDWordHandler = std::function<void(uint32 offset_from_base, uint32* value)>;
    using ReadQWordHandler = std::function<void(uint32 offset_from_base, uint64* value)>;
    using ReadBlockHandler = std::function<void(uint32 offset_from_base, uint32 length, void* destination)>;
    using WriteByteHandler = std::function<void(uint32 offset_from_base, uint8 value)>;
    using WriteWordHandler = std::function<void(uint32 offset_from_base, uint16 value)>;
    using WriteDWordHandler = std::function<void(uint32 offset_from_base, uint32 value)>;
    using WriteQWordHandler = std::function<void(uint32 offset_from_base, uint64 value)>;
    using WriteBlockHandler = std::function<void(uint32 offset_from_base, uint32 length, const void* source)>;

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
  MMIO(PhysicalMemoryAddress start_address, uint32 size, Handlers&& handlers, bool cacheable);
  ~MMIO();

  PhysicalMemoryAddress GetStartAddress() const { return m_start_address; }
  PhysicalMemoryAddress GetEndAddress() const { return m_end_address; }
  uint32 GetSize() const { return m_size; }
  bool IsCachable() const { return m_cachable; }

  void ReadByte(PhysicalMemoryAddress address, uint8* destination);
  void ReadWord(PhysicalMemoryAddress address, uint16* destination);
  void ReadDWord(PhysicalMemoryAddress address, uint32* destination);
  void ReadQWord(PhysicalMemoryAddress address, uint64* destination);
  void ReadBlock(PhysicalMemoryAddress address, uint32 length, void* destination);
  void WriteByte(PhysicalMemoryAddress address, uint8 source);
  void WriteWord(PhysicalMemoryAddress address, uint16 source);
  void WriteDWord(PhysicalMemoryAddress address, uint32 source);
  void WriteQWord(PhysicalMemoryAddress address, uint64 source);
  void WriteBlock(PhysicalMemoryAddress address, uint32 length, const void* source);

  // Factory methods
  static MMIO* CreateDirect(PhysicalMemoryAddress start_address, uint32 size, void* data, bool allow_read = true,
                            bool allow_write = true, bool cacheable = true);
  static MMIO* CreateComplex(PhysicalMemoryAddress start_address, uint32 size, Handlers&& handlers,
                             bool cacheable = false);

private:
  static void IgnoreReadByteHandler(uint32 offset_from_base, uint8* value);
  static void IgnoreReadWordHandler(uint32 offset_from_base, uint16* value);
  static void IgnoreReadDWordHandler(uint32 offset_from_base, uint32* value);
  static void IgnoreReadQWordHandler(uint32 offset_from_base, uint64* value);
  static void IgnoreReadBlockHandler(uint32 offset_from_base, uint32 length, void* destination);
  static void IgnoreWriteByteHandler(uint32 offset_from_base, uint8 value);
  static void IgnoreWriteWordHandler(uint32 offset_from_base, uint16 value);
  static void IgnoreWriteDWordHandler(uint32 offset_from_base, uint32 value);
  static void IgnoreWriteQWordHandler(uint32 offset_from_base, uint64 value);
  static void IgnoreWriteBlockHandler(uint32 offset_from_base, uint32 length, const void* source);
  void DefaultReadWordHandler(uint32 offset_from_base, uint16* value);
  void DefaultReadDWordHandler(uint32 offset_from_base, uint32* value);
  void DefaultReadQWordHandler(uint32 offset_from_base, uint64* value);
  void DefaultReadBlockHandler(uint32 offset_from_base, uint32 length, void* destination);
  void DefaultWriteWordHandler(uint32 offset_from_base, uint16 value);
  void DefaultWriteDWordHandler(uint32 offset_from_base, uint32 value);
  void DefaultWriteQWordHandler(uint32 offset_from_base, uint64 value);
  void DefaultWriteBlockHandler(uint32 offset_from_base, uint32 length, const void* source);

  PhysicalMemoryAddress m_start_address;
  PhysicalMemoryAddress m_end_address;
  uint32 m_size;

  Handlers m_handlers;
  bool m_cachable;
};