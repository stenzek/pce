#pragma once
#include "YBaseLib/Common.h"

// Circular buffer based on the Bip Buffer concept
// http://www.codeproject.com/Articles/3479/The-Bip-Buffer-The-Circular-Buffer-with-a-Twist

class CircularBuffer
{
  DeclareNonCopyable(CircularBuffer);

public:
  CircularBuffer();
  CircularBuffer(size_t bufferSize);
  CircularBuffer(byte* pBuffer, size_t bufferSize);
  ~CircularBuffer();

  // Total size of buffer
  size_t GetBufferSize() const { return m_bufferSize; }

  // Current space in the buffer (including wrap-around).
  size_t GetBufferSpace() const;

  // Current space in the buffer (excluding wrap-around).
  size_t GetContiguousBufferSpace() const;

  // Current data size in the buffer (including wrap-around)
  size_t GetBufferUsed() const;

  // Current data size in the buffer (excluding wrap-around)
  size_t GetContiguousUsedBytes() const;

  // Resize the buffer. Currently can only increase size and not decrease.
  void ResizeBuffer(size_t newBufferSize);

  // Read bytes from the buffer. Only will return true upon completing the entire read.
  bool Read(void* pDestination, size_t byteCount);

  // Write bytes to the buffer. Only will return true upon completing the entire write.
  bool Write(const void* pSource, size_t byteCount);

  // Obtains a pointer to the start of the data in the buffer.
  // When finished, call FlushBytes(length).
  bool GetReadPointer(const void** ppReadPointer, size_t* pByteCount) const;

  // Frees up buffer space.
  void MoveReadPointer(size_t byteCount);

  // Obtains a pointer to the start of writable data with at least byteCount bytes, contiguously.
  bool GetWritePointer(void** ppWritePointer, size_t* pByteCount);

  // After writing to ppWritePointer, moves it forward.
  void MoveWritePointer(size_t byteCount);

private:
  // backed storage
  byte* m_pBuffer;

  // region A - in this case, the head is the 'read' pointer, and the tail the 'write' pointer
  byte* m_pRegionAHead;
  byte* m_pRegionATail;

  // region B's head is always located at buffer base, so no need for a variable
  byte* m_pRegionBTail;

  // buffer size
  size_t m_bufferSize;
  bool m_ownsBuffer;
};
