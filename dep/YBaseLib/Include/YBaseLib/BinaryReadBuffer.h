#pragma once
#include "YBaseLib/BinaryReader.h"

class BinaryReadBuffer : public BinaryReader
{
public:
  BinaryReadBuffer(uint32 size);
  ~BinaryReadBuffer();

  const byte* GetBufferPointer() const;
  byte* GetBufferPointer();

  uint32 GetBufferSize() const;

private:
  byte* m_pMemory;
  uint32 m_size;
};
