#pragma once
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Endian.h"
#include "YBaseLib/String.h"

// implements a class that can read/write common types
// it will automatically perform endianness conversion for multibyte types, if needed.
class BinaryWriter
{
public:
  // constructs a reader using the specified stream and the endianness of the stream.
  // if throwExceptions is set to false, and it goes past EOF/errors, 0's will be returned.
  BinaryWriter(ByteStream* pStream, ENDIAN_TYPE streamByteOrder = Y_HOST_ENDIAN_TYPE, bool ignoreErrors = false);

  // returns the pointer to the underlying stream of this reader
  ByteStream* GetStream() { return m_pStream; }

  // returns the error state of the writer
  bool InErrorState() const { return m_errorState || m_pStream->InErrorState(); }
  void ClearErrorState()
  {
    m_errorState = false;
    m_pStream->ClearErrorState();
  }

  // these alter the underlying stream
  uint64 GetStreamPosition() { return m_pStream->GetPosition(); }
  void SeekAbsolute(uint64 position);
  void SeekRelative(int64 offset);
  void SeekToEnd();
  bool SafeSeekAbsolute(uint64 position);
  bool SafeSeekRelative(int64 offset);
  bool SafeSeekToEnd();

  // these types are non-endian-specific
  void WriteBool(const bool b)
  {
    byte bb = b ? 1 : 0;
    InternalWriteBytes(&bb, sizeof(bb));
  }
  void WriteByte(const byte b) { InternalWriteBytes(&b, sizeof(b)); }
  void WriteInt8(const int8 v) { InternalWriteBytes(&v, sizeof(v)); }
  void WriteUInt8(const uint8 v) { InternalWriteBytes(&v, sizeof(v)); }
  bool SafeWriteBool(const bool b)
  {
    byte bb = b ? 1 : 0;
    return SafeInternalWriteBytes(&bb, sizeof(bb));
  }
  bool SafeWriteByte(const byte b) { return SafeInternalWriteBytes(&b, sizeof(b)); }
  bool SafeWriteInt8(const int8 v) { return SafeInternalWriteBytes(&v, sizeof(v)); }
  bool SafeWriteUInt8(const uint8 v) { return SafeInternalWriteBytes(&v, sizeof(v)); }

  // strings have multiple readers based on performance the user requires
  // "C" strings are a series of characters terminated by a NULL (0) character.
  void WriteCString(const String& str);
  void WriteCString(const char* str);
  void WriteCString(const char* str, uint32 len);
  bool SafeWriteCString(const String& str);
  bool SafeWriteCString(const char* str);
  bool SafeWriteCString(const char* str, uint32 len);

  // "fixed" strings are fixed in length in the stream, optionally padded by zeros if the length is shorter.
  // if using the char * version, dest must be at least fixedLength + 1 bytes long.
  void WriteFixedString(const String& str, uint32 fixedLength);
  void WriteFixedString(const char* str, uint32 fixedLength);
  void WriteFixedString(const char* str, uint32 len, uint32 fixedLength);
  bool SafeWriteFixedString(const String& str, uint32 fixedLength);
  bool SafeWriteFixedString(const char* str, uint32 fixedLength);
  bool SafeWriteFixedString(const char* str, uint32 len, uint32 fixedLength);

  // "size prefixed" strings have the length of the string appended before them as a uint16.
  void WriteSizePrefixedString(const String& str);
  void WriteSizePrefixedString(const char* str);
  void WriteSizePrefixedString(const char* str, uint32 len);
  bool SafeWriteSizePrefixedString(const String& str);
  bool SafeWriteSizePrefixedString(const char* str);
  bool SafeWriteSizePrefixedString(const char* str, uint32 len);

  // endian-specific type readers
  void WriteInt16(const int16 v);
  void WriteUInt16(const uint16 v);
  void WriteInt32(const int32 v);
  void WriteUInt32(const uint32 v);
  void WriteInt64(const int64& v);
  void WriteUInt64(const uint64& v);
  void WriteFloat(const float& v);
  void WriteDouble(const double& v);

  // safe variants
  bool SafeWriteInt16(const int16 v);
  bool SafeWriteUInt16(const uint16 v);
  bool SafeWriteInt32(const int32 v);
  bool SafeWriteUInt32(const uint32 v);
  bool SafeWriteInt64(const int64& v);
  bool SafeWriteUInt64(const uint64& v);
  bool SafeWriteFloat(const float& v);
  bool SafeWriteDouble(const double& v);

  // reads a variable number of bytes from the stream, no endian conversion is done
  void WriteBytes(const void* src, uint32 len);
  bool SafeWriteBytes(const void* src, uint32 len);

  // reads a user-specified type from the stream, no endian conversion is done.
  template<typename T>
  void WriteType(const T* v)
  {
    InternalWriteBytes(v, sizeof(T));
  }
  template<typename T>
  bool SafeWriteType(const T* v)
  {
    return SafeInternalWriteBytes(v, sizeof(T));
  }

  // stream operators for reading to local variables
  // at the moment doing this provides no speed boost over the readX functions.
  inline BinaryWriter& operator<<(const int8 src)
  {
    WriteInt8(src);
    return *this;
  }
  inline BinaryWriter& operator<<(const uint8 src)
  {
    WriteUInt8(src);
    return *this;
  }
  inline BinaryWriter& operator<<(const int16 src)
  {
    WriteInt16(src);
    return *this;
  }
  inline BinaryWriter& operator<<(const uint16 src)
  {
    WriteUInt16(src);
    return *this;
  }
  inline BinaryWriter& operator<<(const int32 src)
  {
    WriteInt32(src);
    return *this;
  }
  inline BinaryWriter& operator<<(const uint32 src)
  {
    WriteUInt32(src);
    return *this;
  }
  inline BinaryWriter& operator<<(const int64& src)
  {
    WriteInt64(src);
    return *this;
  }
  inline BinaryWriter& operator<<(const uint64& src)
  {
    WriteUInt64(src);
    return *this;
  }
  inline BinaryWriter& operator<<(const float& src)
  {
    WriteFloat(src);
    return *this;
  }
  inline BinaryWriter& operator<<(const double& src)
  {
    WriteDouble(src);
    return *this;
  }
  inline BinaryWriter& operator<<(const String& src)
  {
    WriteCString(src);
    return *this;
  }
  inline BinaryWriter& operator<<(const char* src)
  {
    WriteCString(src);
    return *this;
  }

protected:
  // internal function that reads the actual data from the stream, and does error checking
  void InternalWriteBytes(const void* pSource, uint32 cbSource);
  bool SafeInternalWriteBytes(const void* pSource, uint32 cbSource);

  ByteStream* m_pStream;
  ENDIAN_TYPE m_eStreamByteOrder;
  bool m_ignoreErrors;
  bool m_errorState;
};
