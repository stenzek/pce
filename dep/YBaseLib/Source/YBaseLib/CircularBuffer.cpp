#include "YBaseLib/CircularBuffer.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"

CircularBuffer::CircularBuffer()
  : m_pBuffer(nullptr), m_pRegionAHead(m_pBuffer), m_pRegionATail(m_pBuffer), m_pRegionBTail(nullptr), m_bufferSize(0),
    m_ownsBuffer(true)
{
}

CircularBuffer::CircularBuffer(size_t bufferSize)
  : m_pBuffer((byte*)std::malloc(bufferSize)), m_pRegionAHead(m_pBuffer), m_pRegionATail(m_pBuffer),
    m_pRegionBTail(nullptr), m_bufferSize(bufferSize), m_ownsBuffer(true)
{
  // @TODO safe malloc
  DebugAssert(bufferSize > 0);
}

CircularBuffer::CircularBuffer(byte* pBuffer, size_t bufferSize)
  : m_pBuffer(pBuffer), m_pRegionAHead(m_pBuffer), m_pRegionATail(m_pBuffer), m_pRegionBTail(nullptr),
    m_bufferSize(bufferSize), m_ownsBuffer(false)
{
}

CircularBuffer::~CircularBuffer()
{
  if (m_ownsBuffer)
    std::free(m_pBuffer);
}

void CircularBuffer::ResizeBuffer(size_t newBufferSize)
{
  DebugAssert(m_ownsBuffer && newBufferSize >= m_bufferSize);
  byte* pNewBuffer = (byte*)std::realloc(m_pBuffer, newBufferSize);

  // re-align pointers to new buffer
  DebugAssert(pNewBuffer != nullptr);
  m_pRegionAHead = (m_pRegionAHead - m_pBuffer) + pNewBuffer;
  m_pRegionATail = (m_pRegionATail - m_pBuffer) + pNewBuffer;
  if (m_pRegionBTail != nullptr)
    m_pRegionBTail = (m_pRegionBTail - m_pBuffer) + pNewBuffer;

  // swap base pointer
  m_pBuffer = pNewBuffer;
  m_bufferSize = newBufferSize;
}

size_t CircularBuffer::GetBufferSpace() const
{
  // prefer using buffer B
  if (m_pRegionBTail != nullptr)
    return m_pRegionAHead - m_pRegionBTail;
  else
    return m_bufferSize - (m_pRegionATail - m_pRegionAHead);
}

size_t CircularBuffer::GetContiguousBufferSpace() const
{
  // prefer using buffer B
  if (m_pRegionBTail != nullptr)
    return m_pRegionAHead - m_pRegionBTail;
  else
  {
    size_t spaceA = (m_pBuffer + m_bufferSize) - m_pRegionATail;
    size_t spaceB = m_pRegionAHead - m_pBuffer;
    return Max(spaceA, spaceB);
  }
}

size_t CircularBuffer::GetBufferUsed() const
{
  // only include B's figures if it exists
  if (m_pRegionBTail != nullptr)
    return (m_pRegionBTail - m_pBuffer) + (m_pRegionATail - m_pRegionAHead);
  else
    return (m_pRegionATail - m_pRegionAHead);
}

size_t CircularBuffer::GetContiguousUsedBytes() const
{
  return m_pRegionATail - m_pRegionAHead;
}

bool CircularBuffer::GetReadPointer(const void** ppReadPointer, size_t* pByteCount) const
{
  // check for empty buffer
  if (m_pRegionATail == m_pRegionAHead)
    return false;

  // read from A first, keep in mind this is a contiguous pointer,
  // so even if B exists, we can't use it yet. we'll sort that out
  // in MoveReadPointer afterwards.
  *ppReadPointer = m_pRegionAHead;
  *pByteCount = m_pRegionATail - m_pRegionAHead;
  return true;
}

void CircularBuffer::MoveReadPointer(size_t byteCount)
{
  DebugAssert(byteCount <= static_cast<size_t>(m_pRegionATail - m_pRegionAHead));

  // walk all over the bytes - only in debug mode
#ifdef Y_BUILD_CONFIG_DEBUG
  std::memset(m_pRegionAHead, 0xDE, byteCount);
#endif

  // move A's read pointer forward
  m_pRegionAHead += byteCount;

  // consumed region A?
  if (m_pRegionAHead == m_pRegionATail)
  {
    // if region B exists, "rename" it to region A
    if (m_pRegionBTail != nullptr)
    {
      m_pRegionAHead = m_pBuffer;
      m_pRegionATail = m_pRegionBTail;
      m_pRegionBTail = nullptr;
    }
    // otherwise just move the pointers back to the start
    else
    {
      m_pRegionAHead = m_pBuffer;
      m_pRegionATail = m_pBuffer;
    }
  }
}

bool CircularBuffer::GetWritePointer(void** ppWritePointer, size_t* pByteCount)
{
  size_t requiredBytes = *pByteCount;
  size_t freeSpace;

  // always allocate to B if it exists
  if (m_pRegionBTail != nullptr)
  {
    freeSpace = (m_pRegionAHead - m_pRegionBTail);
    if (requiredBytes > freeSpace)
      return false;

    *ppWritePointer = m_pBuffer;
    *pByteCount = freeSpace;
    return true;
  }

  // calculate space should B be created
  size_t spaceA = (m_pBuffer + m_bufferSize) - m_pRegionATail;
  size_t spaceB = m_pRegionAHead - m_pBuffer;

  // would creating region B allow more space than after A?
  if (spaceB > spaceA)
  {
    // size check
    if (requiredBytes > spaceB)
      return false;

    // allocate B
    m_pRegionBTail = m_pBuffer;
    *ppWritePointer = m_pRegionBTail;
    *pByteCount = spaceB;
    return true;
  }
  else
  {
    // leaves everything left in A
    if (requiredBytes > spaceA)
      return false;

    *ppWritePointer = m_pRegionATail;
    *pByteCount = spaceA;
    return true;
  }
}

void CircularBuffer::MoveWritePointer(size_t byteCount)
{
  if (m_pRegionBTail != nullptr)
  {
    // even if B was just allocated, it'll be non-null from above
    DebugAssert(byteCount <= static_cast<size_t>(m_pRegionAHead - m_pRegionBTail));
    m_pRegionBTail += byteCount;
  }
  else
  {
    // add to A size
    DebugAssert(byteCount <= static_cast<size_t>(m_pBuffer + m_bufferSize - m_pRegionATail));
    m_pRegionATail += byteCount;
  }
}

bool CircularBuffer::Read(void* pDestination, size_t byteCount)
{
  if (byteCount > GetBufferUsed())
    return false;

  byte* pDestinationPtr = reinterpret_cast<byte*>(pDestination);
  while (byteCount > 0)
  {
    const void* pReadPointer;
    size_t availableBytes;
    if (!GetReadPointer(&pReadPointer, &availableBytes))
      Panic("Buffer failed mid-read.");

    // copy data
    size_t copyCount = Min(byteCount, availableBytes);
    std::memcpy(pDestinationPtr, pReadPointer, copyCount);
    MoveReadPointer(copyCount);
    pDestinationPtr += copyCount;
    byteCount -= copyCount;
  }

  return true;
}

bool CircularBuffer::Write(const void* pSource, size_t byteCount)
{
  if (byteCount > GetBufferSpace())
    return false;

  const byte* pSourcePtr = reinterpret_cast<const byte*>(pSource);
  while (byteCount > 0)
  {
    void* pWritePointer;
    size_t availableBytes;
    if (!GetWritePointer(&pWritePointer, &availableBytes))
      Panic("Buffer failed mid-write.");

    // copy data
    size_t copyCount = Min(byteCount, availableBytes);
    std::memcpy(pWritePointer, pSourcePtr, copyCount);
    MoveWritePointer(copyCount);
    pSourcePtr += copyCount;
    byteCount -= copyCount;
  }

  return true;
}
