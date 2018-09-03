#pragma once
#include "pce/systems/i430fx.h"

class ByteStream;

namespace Systems {

class Bochs : public i430FX
{
  DECLARE_OBJECT_TYPE_INFO(Bochs, i430FX);
  DECLARE_OBJECT_GENERIC_FACTORY(Bochs);
  DECLARE_OBJECT_PROPERTY_MAP(Bochs);

public:
  Bochs(CPU_X86::Model model = CPU_X86::MODEL_PENTIUM, float cpu_frequency = 8000000.0f,
        uint32 memory_size = 16 * 1024 * 1024, const ObjectTypeInfo* type_info = &s_type_info);
  ~Bochs();

  bool Initialize() override;
  void Reset() override;

  bool LoadSystemState(BinaryReader& reader) override;
  bool SaveSystemState(BinaryWriter& writer) override;

protected:
  void SetCMOSVariables();
  void ConnectSystemIOPorts();
};

} // namespace Systems