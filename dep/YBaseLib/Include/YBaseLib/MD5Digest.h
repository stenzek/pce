#pragma once
#include "YBaseLib/Common.h"

// based heavily on this implementation:
// http://www.fourmilab.ch/md5/

class MD5Digest
{
public:
  MD5Digest();

  void Update(const void* pData, uint32 cbData);
  void Final(byte Digest[16]);
  void Reset();

private:
  uint32 buf[4];
  uint32 bits[2];
  byte in[64];
};
