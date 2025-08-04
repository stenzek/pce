#include "YBaseLib/BinaryWriteBuffer.h"
#include "YBaseLib/ByteStream.h"

BinaryWriteBuffer::BinaryWriteBuffer() : BinaryWriter(nullptr)
{
  // override the BinaryWriter value
  m_pStream = ByteStream_CreateGrowableMemoryStream();
}

BinaryWriteBuffer::~BinaryWriteBuffer()
{
  m_pStream->Release();
}

const byte* BinaryWriteBuffer::GetBufferPointer() const
{
  return static_cast<GrowableMemoryByteStream*>(m_pStream)->GetMemoryPointer();
}

const uint32 BinaryWriteBuffer::GetBufferSize() const
{
  return static_cast<GrowableMemoryByteStream*>(m_pStream)->GetMemorySize();
}
