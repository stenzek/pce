#pragma once
#include "common/object.h"
#include "pce/types.h"

class BinaryReader;
class BinaryWriter;

class Bus;
class System;

class Component : public Object
{
  DECLARE_OBJECT_TYPE_INFO(Component, Object);
  DECLARE_OBJECT_NO_PROPERTIES(Component);
  DECLARE_OBJECT_NO_FACTORY(Component);

public:
  virtual ~Component() {}

  virtual bool Initialize(System* system, Bus* bus) { return true; }
  virtual void Reset() {}

  virtual bool LoadState(BinaryReader& reader) { return true; }
  virtual bool SaveState(BinaryWriter& writer) { return true; }

  // Creates a serialization ID for identifying components in save states
  static constexpr uint32 MakeSerializationID(uint8 a = 0, uint8 b = 0, uint8 c = 0, uint8 d = 0)
  {
    return uint32(d) | (uint32(c) << 8) | (uint32(b) << 16) | (uint32(a) << 24);
  }
};
