#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"

BinaryReader::BinaryReader(ByteStream* pStream, ENDIAN_TYPE streamByteOrder /* = Y_HOST_ENDIAN_TYPE */,
                           bool ignoreErrors /* = false */)
  : m_pStream(pStream), m_eStreamByteOrder(streamByteOrder), m_ignoreErrors(ignoreErrors), m_errorState(false)
{
}

void BinaryReader::SeekAbsolute(uint64 position)
{
  if (m_errorState || !m_pStream->SeekAbsolute(position))
  {
    m_errorState = true;

    if (!m_ignoreErrors)
      Panic("BinaryReader::SeekAbsolute() failed");
  }
}

void BinaryReader::SeekRelative(int64 offset)
{
  if (m_errorState || !m_pStream->SeekRelative(offset))
  {
    m_errorState = true;

    if (!m_ignoreErrors)
      Panic("BinaryReader::SeekRelative() failed");
  }
}

void BinaryReader::SeekToEnd()
{
  if (m_errorState || !m_pStream->SeekToEnd())
  {
    m_errorState = true;

    if (!m_ignoreErrors)
      Panic("BinaryReader::SeekToEnd() failed");
  }
}

bool BinaryReader::SafeSeekAbsolute(uint64 position)
{
  if (m_errorState)
    return false;

  return m_pStream->SeekAbsolute(position);
}

bool BinaryReader::SafeSeekRelative(int64 offset)
{
  if (m_errorState)
    return false;

  return m_pStream->SeekRelative(offset);
}

bool BinaryReader::SafeSeekToEnd()
{
  if (m_errorState)
    return false;

  return m_pStream->SeekToEnd();
}

void BinaryReader::InternalReadBytes(void* pDestination, uint32 cbDestination)
{
  if (!cbDestination)
    return;

  if (m_errorState)
  {
    if (!m_ignoreErrors)
      Panic("BinaryReader::InternalReadBytes() failed");

    Y_memzero(pDestination, cbDestination);
    return;
  }

  uint32 bytesRead;
  if (cbDestination == 1)
    bytesRead = m_pStream->ReadByte((byte*)pDestination) ? 1 : 0;
  else
    bytesRead = m_pStream->Read(pDestination, cbDestination);

  if (bytesRead != cbDestination)
  {
    if (!m_ignoreErrors)
      Panic("BinaryReader::InternalReadBytes() failed");

    m_errorState = true;
    Y_memzero(reinterpret_cast<byte*>(pDestination) + bytesRead, cbDestination - bytesRead);
  }
}

bool BinaryReader::SafeInternalReadBytes(void* pDestination, uint32 cbDestination)
{
  if (!cbDestination)
    return true;

  if (m_errorState)
    return false;

  uint32 bytesRead;
  if (cbDestination == 1)
    bytesRead = m_pStream->ReadByte((byte*)pDestination) ? 1 : 0;
  else
    bytesRead = m_pStream->Read(pDestination, cbDestination);

  if (bytesRead == cbDestination)
    return true;

  m_errorState = true;
  return false;
}

bool BinaryReader::ReadBool()
{
  byte ret;
  InternalReadBytes(&ret, 1);
  return (ret != 0);
}

bool BinaryReader::SafeReadBool(bool* pValue)
{
  return SafeInternalReadBytes(reinterpret_cast<void*>(pValue), 1);
}

byte BinaryReader::ReadByte()
{
  byte ret;
  InternalReadBytes(&ret, 1);
  return ret;
}

bool BinaryReader::SafeReadByte(byte* pValue)
{
  return SafeInternalReadBytes(reinterpret_cast<void*>(pValue), 1);
}

int8 BinaryReader::ReadInt8()
{
  int8 ret;
  InternalReadBytes(&ret, 1);
  return ret;
}

bool BinaryReader::SafeReadInt8(int8* pValue)
{
  return SafeInternalReadBytes(reinterpret_cast<void*>(pValue), 1);
}

uint8 BinaryReader::ReadUInt8()
{
  int8 ret;
  InternalReadBytes(&ret, 1);
  return ret;
}

bool BinaryReader::SafeReadUInt8(uint8* pValue)
{
  return SafeInternalReadBytes(reinterpret_cast<void*>(pValue), 1);
}

// string functions
String BinaryReader::ReadCString()
{
  String ret;
  ReadCString(ret);
  ret.Shrink();
  return ret;
}

bool BinaryReader::SafeReadCString(String* pValue)
{
  pValue->Clear();

  char ch;
  for (;;)
  {
    if (!SafeInternalReadBytes(&ch, 1))
      return false;

    if (ch == 0)
      break;

    pValue->AppendCharacter(ch);
  }

  return true;
}

uint32 BinaryReader::ReadCString(char* dest, uint32 maxSize)
{
  uint32 curSize = 0;

  char c;
  for (;;)
  {
    InternalReadBytes(&c, 1);
    if (c == 0)
      break;

    if (curSize < maxSize)
      dest[curSize++] = c;
  }

  // ensure dest is null-terminated
  if (curSize == maxSize)
  {
    dest[curSize - 1] = 0;
    return curSize - 1;
  }
  else
  {
    dest[curSize] = 0;
    return curSize;
  }
}

void BinaryReader::ReadCString(String& dest)
{
  dest.Clear();

  char c;
  for (;;)
  {
    InternalReadBytes(&c, 1);
    if (c == 0)
      break;

    dest.AppendCharacter(c);
  }
}

String BinaryReader::ReadFixedString(uint32 fixedLength)
{
  String ret;
  ReadFixedString(fixedLength, ret);
  return ret;
}

bool BinaryReader::SafeReadFixedString(uint32 fixedLength, String* pValue)
{
  pValue->Clear();
  pValue->Resize(fixedLength);
  if (!SafeInternalReadBytes(pValue->GetWriteableCharArray(), fixedLength))
  {
    pValue->Clear();
    return false;
  }

  pValue->UpdateSize();
  return true;
}

void BinaryReader::ReadFixedString(uint32 fixedLength, char* dest)
{
  InternalReadBytes(dest, fixedLength);
  dest[fixedLength] = 0;
}

void BinaryReader::ReadFixedString(uint32 fixedLength, String& dest)
{
  dest.Clear();
  dest.Resize(fixedLength);
  InternalReadBytes(dest.GetWriteableCharArray(), fixedLength);
  dest.UpdateSize();
}

String BinaryReader::ReadSizePrefixedString()
{
  String ret;
  ReadSizePrefixedString(ret);
  return ret;
}

uint32 BinaryReader::ReadSizePrefixedString(char* dest, uint32 maxSize)
{
  uint32 stringLength = ReadUInt32();
  uint32 curSize = 0;

  char c;
  uint32 i;
  for (i = 0; i < stringLength; i++)
  {
    InternalReadBytes(&c, 1);
    if (c == 0)
      break;

    if (curSize < maxSize)
      dest[curSize++] = c;
  }

  // ensure dest is null-terminated
  if (curSize == maxSize)
  {
    dest[curSize - 1] = 0;
    return curSize - 1;
  }
  else
  {
    dest[curSize] = 0;
    return curSize;
  }
}

void BinaryReader::ReadSizePrefixedString(String& dest)
{
  uint32 stringLength = ReadUInt32();
  if (stringLength == 0)
  {
    dest.Clear();
    return;
  }

  dest.Resize(stringLength);
  InternalReadBytes(dest.GetWriteableCharArray(), stringLength);
}

bool BinaryReader::SafeReadSizePrefixedString(String* pValue)
{
  uint32 stringLength;
  if (!SafeReadUInt32(&stringLength))
    return false;

  pValue->Clear();
  if (stringLength == 0)
    return true;

  pValue->Resize(stringLength);
  if (!SafeInternalReadBytes(pValue->GetWriteableCharArray(), stringLength))
  {
    pValue->Clear();
    return false;
  }

  pValue->UpdateSize();
  return true;
}

// endian-specific
int16 BinaryReader::ReadInt16()
{
  int16 ret;
  InternalReadBytes(&ret, 2);
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint16((uint16*)&ret);

  return ret;
}

bool BinaryReader::SafeReadInt16(int16* pValue)
{
  if (!SafeInternalReadBytes(pValue, sizeof(int16)))
    return false;

  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint16(reinterpret_cast<uint16*>(pValue));

  return true;
}

uint16 BinaryReader::ReadUInt16()
{
  uint16 ret;
  InternalReadBytes(&ret, 2);
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint16(&ret);

  return ret;
}

bool BinaryReader::SafeReadUInt16(uint16* pValue)
{
  if (!SafeInternalReadBytes(pValue, sizeof(uint16)))
    return false;

  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint16(reinterpret_cast<uint16*>(pValue));

  return true;
}

int32 BinaryReader::ReadInt32()
{
  int32 ret;
  InternalReadBytes(&ret, 4);
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32((uint32*)&ret);

  return ret;
}

bool BinaryReader::SafeReadInt32(int32* pValue)
{
  if (!SafeInternalReadBytes(pValue, sizeof(int32)))
    return false;

  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32(reinterpret_cast<uint32*>(pValue));

  return true;
}

uint32 BinaryReader::ReadUInt32()
{
  uint32 ret;
  InternalReadBytes(&ret, 4);
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32(&ret);

  return ret;
}

bool BinaryReader::SafeReadUInt32(uint32* pValue)
{
  if (!SafeInternalReadBytes(pValue, sizeof(uint32)))
    return false;

  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32(reinterpret_cast<uint32*>(pValue));

  return true;
}

int64 BinaryReader::ReadInt64()
{
  int64 ret;
  InternalReadBytes(&ret, 8);
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64((uint64*)&ret);

  return ret;
}

bool BinaryReader::SafeReadInt64(int64* pValue)
{
  if (!SafeInternalReadBytes(pValue, sizeof(int64)))
    return false;

  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64(reinterpret_cast<uint64*>(pValue));

  return true;
}

uint64 BinaryReader::ReadUInt64()
{
  uint64 ret;
  InternalReadBytes(&ret, 8);
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64(&ret);

  return ret;
}

bool BinaryReader::SafeReadUInt64(uint64* pValue)
{
  if (!SafeInternalReadBytes(pValue, sizeof(uint64)))
    return false;

  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64(reinterpret_cast<uint64*>(pValue));

  return true;
}

float BinaryReader::ReadFloat()
{
  float ret;
  InternalReadBytes(&ret, 4);
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32((uint32*)&ret);

  return ret;
}

bool BinaryReader::SafeReadFloat(float* pValue)
{
  if (!SafeInternalReadBytes(pValue, sizeof(float)))
    return false;

  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32(reinterpret_cast<uint32*>(pValue));

  return true;
}

double BinaryReader::ReadDouble()
{
  double ret;
  InternalReadBytes(&ret, 8);
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64((uint64*)&ret);

  return ret;
}

bool BinaryReader::SafeReadDouble(double* pValue)
{
  if (!SafeInternalReadBytes(pValue, sizeof(double)))
    return false;

  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64(reinterpret_cast<uint64*>(pValue));

  return true;
}

void BinaryReader::ReadBytes(void* dst, uint32 dstSize)
{
  InternalReadBytes(dst, dstSize);
}

bool BinaryReader::SafeReadBytes(void* dst, uint32 dstSize)
{
  return SafeInternalReadBytes(dst, dstSize);
}
