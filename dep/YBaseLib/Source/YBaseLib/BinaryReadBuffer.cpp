#include "YBaseLib/BinaryReadBuffer.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Memory.h"

BinaryReadBuffer::BinaryReadBuffer(uint32 size) : BinaryReader(nullptr)
{
  m_pMemory = (byte*)std::malloc(size);
  m_size = size;

  // override the BinaryWriter value
  m_pStream = ByteStream_CreateMemoryStream(m_pMemory, m_size);
}

BinaryReadBuffer::~BinaryReadBuffer()
{
  m_pStream->Release();
}

const byte* BinaryReadBuffer::GetBufferPointer() const
{
  return m_pMemory;
}

byte* BinaryReadBuffer::GetBufferPointer()
{
  return m_pMemory;
}

uint32 BinaryReadBuffer::GetBufferSize() const
{
  return m_size;
}
