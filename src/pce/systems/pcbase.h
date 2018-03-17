#pragma once
#include "pce/system.h"
#include <list>
#include <memory>

namespace Systems {

class PCBase : public System
{
public:
  PCBase(HostInterface* host_interface);
  virtual ~PCBase();

  bool AddROM(PhysicalMemoryAddress address, ByteStream* stream);
  bool AddInterleavedROM(PhysicalMemoryAddress address, ByteStream* low_stream, ByteStream* high_stream);

  PhysicalMemoryAddress GetBaseMemorySize() const;
  PhysicalMemoryAddress GetExtendedMemorySize() const;
  PhysicalMemoryAddress GetTotalMemorySize() const;

protected:
  static constexpr PhysicalMemoryAddress A20_BIT = (1 << 20);

  virtual void AllocatePhysicalMemory(uint32 ram_size, bool reserve_isa_memory);
  virtual bool LoadSystemState(BinaryReader& reader) override;
  virtual bool SaveSystemState(BinaryWriter& writer) override;

  // ROM space
  struct ROMBlock
  {
    std::unique_ptr<byte[]> data;
    MMIO* mmio = nullptr;
  };

  ROMBlock* AllocateROM(PhysicalMemoryAddress address, uint32 size);

  std::list<ROMBlock> m_roms;

  // Helper for A20
  bool GetA20State() const;
  void SetA20State(bool state);
};

} // namespace Systems