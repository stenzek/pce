#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"

BinaryWriter::BinaryWriter(ByteStream* pStream, ENDIAN_TYPE streamByteOrder /* = Y_HOST_ENDIAN_TYPE */,
                           bool ignoreErrors /* = false */)
  : m_pStream(pStream), m_eStreamByteOrder(streamByteOrder), m_ignoreErrors(ignoreErrors), m_errorState(false)
{
}

void BinaryWriter::SeekAbsolute(uint64 position)
{
  if (m_errorState || !m_pStream->SeekAbsolute(position))
  {
    m_errorState = true;

    if (!m_ignoreErrors)
      Panic("BinaryWriter::SeekAbsolute() failed");
  }
}

void BinaryWriter::SeekRelative(int64 offset)
{
  if (m_errorState || !m_pStream->SeekRelative(offset))
  {
    m_errorState = true;

    if (!m_ignoreErrors)
      Panic("BinaryWriter::SeekRelative() failed");
  }
}

void BinaryWriter::SeekToEnd()
{
  if (m_errorState || !m_pStream->SeekToEnd())
  {
    m_errorState = true;

    if (!m_ignoreErrors)
      Panic("BinaryWriter::SeekToEnd() failed");
  }
}

bool BinaryWriter::SafeSeekAbsolute(uint64 position)
{
  if (m_errorState)
    return false;

  return m_pStream->SeekAbsolute(position);
}

bool BinaryWriter::SafeSeekRelative(int64 offset)
{
  if (m_errorState)
    return false;

  return m_pStream->SeekRelative(offset);
}

bool BinaryWriter::SafeSeekToEnd()
{
  if (m_errorState)
    return false;

  return m_pStream->SeekToEnd();
}

void BinaryWriter::InternalWriteBytes(const void* pSource, uint32 cbSource)
{
  if (m_errorState || cbSource == 0)
    return;

  uint32 bytesWritten;
  if (cbSource == 1)
    bytesWritten = (m_pStream->WriteByte(*(byte*)pSource)) ? 1 : 0;
  else
    bytesWritten = m_pStream->Write(pSource, cbSource);

  if (bytesWritten == cbSource)
    return;

  if (!m_ignoreErrors)
    m_errorState = true;
}

bool BinaryWriter::SafeInternalWriteBytes(const void* pSource, uint32 cbSource)
{
  if (cbSource == 0)
    return true;

  uint32 bytesWritten;
  if (cbSource == 1)
    bytesWritten = (m_pStream->WriteByte(*(byte*)pSource)) ? 1 : 0;
  else
    bytesWritten = m_pStream->Write(pSource, cbSource);

  if (bytesWritten == cbSource)
    return true;

  if (!m_ignoreErrors)
    m_errorState = true;

  return false;
}

void BinaryWriter::WriteCString(const String& str)
{
  if (str.GetLength() > 0)
    InternalWriteBytes(str.GetCharArray(), str.GetLength());

  // terminating zero
  WriteByte(0);
}

void BinaryWriter::WriteCString(const char* str)
{
  uint32 strLength = Y_strlen(str);
  if (strLength > 0)
    InternalWriteBytes(str, strLength);

  // terminating zero
  WriteByte(0);
}

void BinaryWriter::WriteCString(const char* str, uint32 len)
{
  if (len > 0)
    InternalWriteBytes(str, len);

  // terminating zero
  WriteByte(0);
}

bool BinaryWriter::SafeWriteCString(const String& str)
{
  if (str.GetLength() > 0 && !SafeInternalWriteBytes(str.GetCharArray(), str.GetLength()))
    return false;

  // terminating zero
  return SafeWriteByte(0);
}

bool BinaryWriter::SafeWriteCString(const char* str)
{
  uint32 strLength = Y_strlen(str);
  if (strLength > 0 && !SafeInternalWriteBytes(str, strLength))
    return false;

  // terminating zero
  return SafeWriteByte(0);
}

bool BinaryWriter::SafeWriteCString(const char* str, uint32 len)
{
  if (len > 0 && !SafeInternalWriteBytes(str, len))
    return false;

  // terminating zero
  return SafeWriteByte(0);
}

void BinaryWriter::WriteFixedString(const String& str, uint32 fixedLength)
{
  uint32 charsToWrite = Min(str.GetLength(), fixedLength);
  if (charsToWrite > 0)
    InternalWriteBytes(str.GetCharArray(), charsToWrite);

  for (; charsToWrite < fixedLength; charsToWrite++)
    WriteByte(0);
}

void BinaryWriter::WriteFixedString(const char* str, uint32 fixedLength)
{
  uint32 charsToWrite = Min(Y_strlen(str), fixedLength);
  if (charsToWrite > 0)
    InternalWriteBytes(str, charsToWrite);

  for (; charsToWrite < fixedLength; charsToWrite++)
    WriteByte(0);
}

void BinaryWriter::WriteFixedString(const char* str, uint32 len, uint32 fixedLength)
{
  uint32 charsToWrite = Min(len, fixedLength);
  if (charsToWrite > 0)
    InternalWriteBytes(str, charsToWrite);

  for (; charsToWrite < fixedLength; charsToWrite++)
    WriteByte(0);
}

bool BinaryWriter::SafeWriteFixedString(const String& str, uint32 fixedLength)
{
  uint32 charsToWrite = Min(str.GetLength(), fixedLength);
  if (charsToWrite > 0 && !SafeInternalWriteBytes(str.GetCharArray(), charsToWrite))
    return false;

  for (; charsToWrite < fixedLength; charsToWrite++)
  {
    if (!SafeWriteByte(0))
      return false;
  }

  return true;
}

bool BinaryWriter::SafeWriteFixedString(const char* str, uint32 fixedLength)
{
  uint32 charsToWrite = Min(Y_strlen(str), fixedLength);
  if (charsToWrite > 0 && !SafeInternalWriteBytes(str, charsToWrite))
    return false;

  for (; charsToWrite < fixedLength; charsToWrite++)
  {
    if (!SafeWriteByte(0))
      return false;
  }

  return true;
}

bool BinaryWriter::SafeWriteFixedString(const char* str, uint32 len, uint32 fixedLength)
{
  uint32 charsToWrite = Min(len, fixedLength);
  if (charsToWrite > 0 && !SafeInternalWriteBytes(str, charsToWrite))
    return false;

  for (; charsToWrite < fixedLength; charsToWrite++)
  {
    if (!SafeWriteByte(0))
      return false;
  }

  return true;
}

void BinaryWriter::WriteSizePrefixedString(const String& str)
{
  uint32 strLength = str.GetLength();
  WriteUInt32(strLength);
  if (strLength > 0)
    InternalWriteBytes(str.GetCharArray(), strLength);
}

void BinaryWriter::WriteSizePrefixedString(const char* str)
{
  uint32 strLength = Y_strlen(str);
  WriteUInt32(strLength);
  if (strLength > 0)
    InternalWriteBytes(str, strLength);
}

void BinaryWriter::WriteSizePrefixedString(const char* str, uint32 len)
{
  WriteUInt32(len);
  if (len > 0)
    InternalWriteBytes(str, len);
}

bool BinaryWriter::SafeWriteSizePrefixedString(const String& str)
{
  uint32 strLength = str.GetLength();

  if (!SafeWriteUInt32(strLength))
    return false;

  if (strLength > 0)
    return SafeInternalWriteBytes(str.GetCharArray(), strLength);
  else
    return true;
}

bool BinaryWriter::SafeWriteSizePrefixedString(const char* str)
{
  uint32 strLength = Y_strlen(str);

  if (!SafeWriteUInt32(strLength))
    return false;

  if (strLength > 0)
    return SafeInternalWriteBytes(str, strLength);
  else
    return true;
}

bool BinaryWriter::SafeWriteSizePrefixedString(const char* str, uint32 len)
{
  if (!SafeWriteUInt32(len))
    return false;

  if (len > 0)
    return SafeInternalWriteBytes(str, len);
  else
    return true;
}

// endian-specific
void BinaryWriter::WriteInt16(const int16 v)
{
  int16 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint16((uint16*)&valueToWrite);

  InternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

void BinaryWriter::WriteUInt16(const uint16 v)
{
  uint16 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint16(&valueToWrite);

  InternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

void BinaryWriter::WriteInt32(const int32 v)
{
  int32 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32((uint32*)&valueToWrite);

  InternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

void BinaryWriter::WriteUInt32(const uint32 v)
{
  uint32 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32(&valueToWrite);

  InternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

void BinaryWriter::WriteInt64(const int64& v)
{
  int64 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64((uint64*)&valueToWrite);

  InternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

void BinaryWriter::WriteUInt64(const uint64& v)
{
  uint64 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64(&valueToWrite);

  InternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

void BinaryWriter::WriteFloat(const float& v)
{
  float valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32((uint32*)&valueToWrite);

  InternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

void BinaryWriter::WriteDouble(const double& v)
{
  double valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64((uint64*)&valueToWrite);

  InternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

bool BinaryWriter::SafeWriteInt16(const int16 v)
{
  int16 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint16((uint16*)&valueToWrite);

  return SafeInternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

bool BinaryWriter::SafeWriteUInt16(const uint16 v)
{
  uint16 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint16(&valueToWrite);

  return SafeInternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

bool BinaryWriter::SafeWriteInt32(const int32 v)
{
  int32 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32((uint32*)&valueToWrite);

  return SafeInternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

bool BinaryWriter::SafeWriteUInt32(const uint32 v)
{
  uint32 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32(&valueToWrite);

  return SafeInternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

bool BinaryWriter::SafeWriteInt64(const int64& v)
{
  int64 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64((uint64*)&valueToWrite);

  return SafeInternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

bool BinaryWriter::SafeWriteUInt64(const uint64& v)
{
  uint64 valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64(&valueToWrite);

  return SafeInternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

bool BinaryWriter::SafeWriteFloat(const float& v)
{
  float valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint32((uint32*)&valueToWrite);

  return SafeInternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

bool BinaryWriter::SafeWriteDouble(const double& v)
{
  double valueToWrite = v;
  if (m_eStreamByteOrder != Y_HOST_ENDIAN_TYPE)
    Y_byteswap_uint64((uint64*)&valueToWrite);

  return SafeInternalWriteBytes(&valueToWrite, sizeof(valueToWrite));
}

void BinaryWriter::WriteBytes(const void* src, uint32 len)
{
  InternalWriteBytes(src, len);
}

bool BinaryWriter::SafeWriteBytes(const void* src, uint32 len)
{
  return SafeInternalWriteBytes(src, len);
}
