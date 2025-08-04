#pragma once
#include "YBaseLib/Common.h"

class ByteStream;
class String;

class CRC32
{
public:
  CRC32(uint32 start = 0);

  uint32 GetCRC() const { return m_currentCRC; }
  void HashBytes(const void* buf, size_t len);
  void HashString(const String& str);
  bool HashStream(ByteStream* pStream, bool seekToStart = false, bool restorePosition = false);
  bool HashStreamPartial(ByteStream* pStream, uint64 count);
  void Reset();

private:
  uint32 m_currentCRC;
};
