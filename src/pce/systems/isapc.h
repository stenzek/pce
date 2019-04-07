#pragma once
#include "pce/system.h"
#include <list>
#include <memory>

namespace Systems {

class ISAPC : public System
{
  DECLARE_OBJECT_TYPE_INFO(ISAPC, System);
  DECLARE_OBJECT_NO_FACTORY(ISAPC);
  DECLARE_OBJECT_PROPERTY_MAP(ISAPC);

public:
  ISAPC(const ObjectTypeInfo* type_info = &s_type_info);
  virtual ~ISAPC();

  bool LoadInterleavedROM(PhysicalMemoryAddress address, const char* low_filename, const char* high_filename);

  PhysicalMemoryAddress GetBaseMemorySize() const;
  PhysicalMemoryAddress GetExtendedMemorySize() const;
  PhysicalMemoryAddress GetTotalMemorySize() const;

protected:
  static constexpr PhysicalMemoryAddress A20_BIT = (1 << 20);

  /// Allocates RAM to physical addresses.
  /// @param reserve_isa_memory Disables RAM allocation in the 15-16MB hole
  /// @param reserve_uma Disables RAM allocation in the start of the upper memory area (A0000 - BFFFF)
  /// @param reserve_rom Disables RAM allocation in the ROM UMA area (C0000 - FFFFF)
  void AllocatePhysicalMemory(u32 ram_size, bool reserve_isa_memory, bool reserve_uma, bool reserve_rom);

  // Helper for A20
  bool GetA20State() const;
  void SetA20State(bool state);
};

} // namespace Systems