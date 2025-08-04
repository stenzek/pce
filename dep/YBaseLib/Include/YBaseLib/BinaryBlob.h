#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/ReferenceCounted.h"

class ByteStream;

class BinaryBlob : public ReferenceCounted
{
public:
  static BinaryBlob* Allocate(uint32 DataSize);
  static BinaryBlob* CreateFromPointer(const void* pData, uint32 dataSize);
  static BinaryBlob* CreateFromStream(ByteStream* pStream, bool seekToStart = true, int32 byteCount = -1);

  // inline const byte *GetDataPointer() const { return reinterpret_cast<const byte *>(this + 1); }
  // inline byte *GetDataPointer() { return reinterpret_cast<byte *>(this + 1); }

  inline const byte* GetDataPointer() const { return m_pDataPointer; }
  inline byte* GetDataPointer() { return m_pDataPointer; }
  inline uint32 GetDataSize() const { return m_iDataSize; }

  // creates a read-only stream from this blob
  ByteStream* CreateReadOnlyStream() const;

  // Detaches the buffer from this stream, rendering the blob itself useless. Free this buffer with Y_free.
  void DetachMemory(void** ppMemory, uint32* pSize);

protected:
  BinaryBlob(uint32 iDataSize);
  ~BinaryBlob();

  byte* m_pDataPointer;
  uint32 m_iDataSize;
};
