#pragma once
#include "YBaseLib/BinaryWriter.h"

class BinaryWriteBuffer : public BinaryWriter
{
public:
  BinaryWriteBuffer();
  ~BinaryWriteBuffer();

  const byte* GetBufferPointer() const;
  const uint32 GetBufferSize() const;
};
