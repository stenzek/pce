#include "YBaseLib/BinaryBlob.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Memory.h"

BinaryBlob::BinaryBlob(uint32 iDataSize) : m_iDataSize(iDataSize)
{
  m_pDataPointer = (byte*)std::malloc(iDataSize);
}

BinaryBlob::~BinaryBlob()
{
  std::free(m_pDataPointer);
}

BinaryBlob* BinaryBlob::Allocate(uint32 DataSize)
{
  DebugAssert(DataSize > 0);
  // return new (Y_malloc(sizeof(BinaryBlob) + DataSize)) BinaryBlob(DataSize);
  return new BinaryBlob(DataSize);
}

BinaryBlob* BinaryBlob::CreateFromStream(ByteStream* pStream, bool seekToStart /* = true */, int32 byteCount /* = -1 */)
{
  uint32 blobSize = (byteCount < 0) ? (uint32)pStream->GetSize() : (uint32)byteCount;
  uint64 currentPosition = pStream->GetPosition();

  if (!seekToStart && byteCount < 0)
    blobSize -= (uint32)currentPosition;
  else
    pStream->SeekAbsolute(0);

  Assert(blobSize < 0xFFFFFFFFUL);

  BinaryBlob* pBlob = BinaryBlob::Allocate(blobSize);
  if (!pStream->Read(pBlob->GetDataPointer(), blobSize))
  {
    pBlob->Release();
    return nullptr;
  }

  if (seekToStart)
    pStream->SeekAbsolute(currentPosition);

  return pBlob;
}

BinaryBlob* BinaryBlob::CreateFromPointer(const void* pData, uint32 dataSize)
{
  BinaryBlob* pBlob = BinaryBlob::Allocate(dataSize);
  if (dataSize > 0)
    std::memcpy(pBlob->GetDataPointer(), pData, dataSize);

  return pBlob;
}

ByteStream* BinaryBlob::CreateReadOnlyStream() const
{
  // todo: seperate class here for tracking references back to the blob
  return ByteStream_CreateReadOnlyMemoryStream(m_pDataPointer, m_iDataSize);
}

void BinaryBlob::DetachMemory(void** ppMemory, uint32* pSize)
{
  DebugAssert(m_pDataPointer != nullptr);
  *ppMemory = m_pDataPointer;
  *pSize = m_iDataSize;
  m_pDataPointer = nullptr;
}
