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

  void AllocatePhysicalMemory(uint32 ram_size, bool reserve_isa_memory, bool reserve_uma);

  // Helper for A20
  bool GetA20State() const;
  void SetA20State(bool state);
};

} // namespace Systems