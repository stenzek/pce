#pragma once
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Endian.h"
#include "YBaseLib/String.h"

// implements a class that can read/write common types
// it will automatically perform endianness conversion for multibyte types, if needed.
class BinaryReader
{
public:
  // constructs a reader using the specified stream and the endianness of the stream.
  // if throwExceptions is set to false, and it goes past EOF/errors, 0's will be returned.
  BinaryReader(ByteStream* pStream, ENDIAN_TYPE streamByteOrder = Y_HOST_ENDIAN_TYPE, bool ignoreErrors = false);

  // returns the pointer to the underlying stream of this reader
  ByteStream* GetStream() { return m_pStream; }

  // returns the error state of the reader
  bool GetErrorState() const { return m_errorState || m_pStream->InErrorState(); }
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
  bool ReadBool();
  byte ReadByte();
  int8 ReadInt8();
  uint8 ReadUInt8();

  // safe variants
  bool SafeReadBool(bool* pValue);
  bool SafeReadByte(byte* pValue);
  bool SafeReadInt8(int8* pValue);
  bool SafeReadUInt8(uint8* pValue);

  // strings have multiple readers based on performance the user requires
  // "C" strings are a series of characters terminated by a NULL (0) character.
  String ReadCString();
  void ReadCString(String& dest);
  uint32 ReadCString(char* dest, uint32 maxSize);
  bool SafeReadCString(String* pValue);

  // "fixed" strings are fixed in length in the stream, optionally padded by zeros if the length is shorter.
  // if using the char * version, dest must be at least fixedLength + 1 bytes long.
  String ReadFixedString(uint32 fixedLength);
  void ReadFixedString(uint32 fixedLength, String& dest);
  void ReadFixedString(uint32 fixedLength, char* dest);
  bool SafeReadFixedString(uint32 fixedLength, String* pValue);

  // "size prefixed" strings have the length of the string appended before them as a uint16.
  String ReadSizePrefixedString();
  void ReadSizePrefixedString(String& dest);
  uint32 ReadSizePrefixedString(char* dest, uint32 maxSize);
  bool SafeReadSizePrefixedString(String* pValue);

  // endian-specific type readers
  int16 ReadInt16();
  uint16 ReadUInt16();
  int32 ReadInt32();
  uint32 ReadUInt32();
  int64 ReadInt64();
  uint64 ReadUInt64();
  float ReadFloat();
  double ReadDouble();

  // safe variants
  bool SafeReadInt16(int16* pValue);
  bool SafeReadUInt16(uint16* pValue);
  bool SafeReadInt32(int32* pValue);
  bool SafeReadUInt32(uint32* pValue);
  bool SafeReadInt64(int64* pValue);
  bool SafeReadUInt64(uint64* pValue);
  bool SafeReadFloat(float* pValue);
  bool SafeReadDouble(double* pValue);

  // reads a variable number of bytes from the stream, no endian conversion is done
  void ReadBytes(void* dst, uint32 dstSize);
  bool SafeReadBytes(void* dst, uint32 dstSize);

  // reads a user-specified type from the stream, no endian conversion is done.
  template<typename T>
  T ReadType()
  {
    T ret;
    InternalReadBytes(&ret, sizeof(T));
    return ret;
  }

  // same, but using a user-supplied pointer
  template<typename T>
  bool SafeReadType(T* dst)
  {
    return SafeInternalReadBytes(dst, sizeof(T));
  }

  // stream operators for reading to local variables
  // at the moment doing this provides no speed boost over the readX functions.
  inline BinaryReader& operator>>(int8& dest)
  {
    dest = ReadInt8();
    return *this;
  }
  inline BinaryReader& operator>>(uint8& dest)
  {
    dest = ReadUInt8();
    return *this;
  }
  inline BinaryReader& operator>>(int16& dest)
  {
    dest = ReadInt16();
    return *this;
  }
  inline BinaryReader& operator>>(uint16& dest)
  {
    dest = ReadUInt16();
    return *this;
  }
  inline BinaryReader& operator>>(int32& dest)
  {
    dest = ReadInt32();
    return *this;
  }
  inline BinaryReader& operator>>(uint32& dest)
  {
    dest = ReadUInt32();
    return *this;
  }
  inline BinaryReader& operator>>(int64& dest)
  {
    dest = ReadInt64();
    return *this;
  }
  inline BinaryReader& operator>>(uint64& dest)
  {
    dest = ReadUInt64();
    return *this;
  }
  inline BinaryReader& operator>>(float& dest)
  {
    dest = ReadFloat();
    return *this;
  }
  inline BinaryReader& operator>>(double& dest)
  {
    dest = ReadDouble();
    return *this;
  }
  inline BinaryReader& operator>>(String& dest)
  {
    ReadCString(dest);
    return *this;
  }

protected:
  // internal function that reads the actual data from the stream, and does error checking
  void InternalReadBytes(void* pDestination, uint32 cbDestination);
  bool SafeInternalReadBytes(void* pDestination, uint32 cbDestination);

  ByteStream* m_pStream;
  ENDIAN_TYPE m_eStreamByteOrder;
  bool m_ignoreErrors;
  bool m_errorState;
};
