#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/ReferenceCounted.h"
#include "YBaseLib/String.h"

// base byte stream creation functions
enum BYTESTREAM_OPEN_MODE
{
  BYTESTREAM_OPEN_READ = 1,           // open stream for writing
  BYTESTREAM_OPEN_WRITE = 2,          // open stream for writing
  BYTESTREAM_OPEN_APPEND = 4,         // seek to the end
  BYTESTREAM_OPEN_TRUNCATE = 8,       // truncate the file, seek to start
  BYTESTREAM_OPEN_CREATE = 16,        // if the file does not exist, create it
  BYTESTREAM_OPEN_CREATE_PATH = 32,   // if the file parent directories don't exist, create them
  BYTESTREAM_OPEN_ATOMIC_UPDATE = 64, //
  BYTESTREAM_OPEN_SEEKABLE = 128,
  BYTESTREAM_OPEN_STREAMED = 256,
};

// interface class used by readers, writers, etc.
class ByteStream : public ReferenceCounted
{
public:
  // reads a single byte from the stream.
  virtual bool ReadByte(byte* pDestByte) = 0;

  // read bytes from this stream. returns the number of bytes read, if this isn't equal to the requested size, an error
  // or EOF occurred.
  virtual uint32 Read(void* pDestination, uint32 ByteCount) = 0;

  // read bytes from this stream, optionally returning the number of bytes read.
  virtual bool Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead = nullptr) = 0;

  // writes a single byte to the stream.
  virtual bool WriteByte(byte SourceByte) = 0;

  // write bytes to this stream, returns the number of bytes written. if this isn't equal to the requested size, a
  // buffer overflow, or write error occurred.
  virtual uint32 Write(const void* pSource, uint32 ByteCount) = 0;

  // write bytes to this stream, optionally returning the number of bytes written.
  virtual bool Write2(const void* pSource, uint32 ByteCount, uint32* pNumberOfBytesWritten = nullptr) = 0;

  // seeks to the specified position in the stream
  // if seek failed, returns false.
  virtual bool SeekAbsolute(uint64 Offset) = 0;
  virtual bool SeekRelative(int64 Offset) = 0;
  virtual bool SeekToEnd() = 0;

  // gets the current offset in the stream
  virtual uint64 GetPosition() const = 0;

  // gets the size of the stream
  virtual uint64 GetSize() const = 0;

  // flush any changes to the stream to disk
  virtual bool Flush() = 0;

  // if the file was opened in atomic update mode, discards any changes made to the file
  virtual bool Discard() = 0;

  // if the file was opened in atomic update mode, commits the file and replaces the temporary file
  virtual bool Commit() = 0;

  // state accessors
  inline bool InErrorState() const { return m_errorState; }
  inline void SetErrorState() { m_errorState = true; }
  inline void ClearErrorState() { m_errorState = false; }

protected:
  // hide the destructor, so that a user cannot delete it without using close()
  ByteStream() : m_errorState(false) {}
  virtual ~ByteStream() {}

  // state bits
  bool m_errorState;

  // make it noncopyable
  DeclareNonCopyable(ByteStream);
};

class NullByteStream : public ByteStream
{
public:
  NullByteStream();
  ~NullByteStream();

  virtual bool ReadByte(byte* pDestByte) override final;
  virtual uint32 Read(void* pDestination, uint32 ByteCount) override final;
  virtual bool Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead /* = nullptr */) override final;
  virtual bool WriteByte(byte SourceByte) override final;
  virtual uint32 Write(const void* pSource, uint32 ByteCount) override final;
  virtual bool Write2(const void* pSource, uint32 ByteCount,
                      uint32* pNumberOfBytesWritten /* = nullptr */) override final;
  virtual bool SeekAbsolute(uint64 Offset) override final;
  virtual bool SeekRelative(int64 Offset) override final;
  virtual bool SeekToEnd() override final;
  virtual uint64 GetSize() const override final;
  virtual uint64 GetPosition() const override final;
  virtual bool Flush() override final;
  virtual bool Commit() override final;
  virtual bool Discard() override final;
};

class MemoryByteStream : public ByteStream
{
public:
  MemoryByteStream(void* pMemory, uint32 MemSize);
  virtual ~MemoryByteStream();

  byte* GetMemoryPointer() const { return m_pMemory; }
  uint32 GetMemorySize() const { return m_iSize; }

  virtual bool ReadByte(byte* pDestByte) override;
  virtual uint32 Read(void* pDestination, uint32 ByteCount) override;
  virtual bool Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead /* = nullptr */) override;
  virtual bool WriteByte(byte SourceByte) override;
  virtual uint32 Write(const void* pSource, uint32 ByteCount) override;
  virtual bool Write2(const void* pSource, uint32 ByteCount, uint32* pNumberOfBytesWritten /* = nullptr */) override;
  virtual bool SeekAbsolute(uint64 Offset) override;
  virtual bool SeekRelative(int64 Offset) override;
  virtual bool SeekToEnd() override;
  virtual uint64 GetSize() const override;
  virtual uint64 GetPosition() const override;
  virtual bool Flush() override;
  virtual bool Commit() override;
  virtual bool Discard() override;

private:
  byte* m_pMemory;
  uint32 m_iPosition;
  uint32 m_iSize;
};

class ReadOnlyMemoryByteStream : public ByteStream
{
public:
  ReadOnlyMemoryByteStream(const void* pMemory, uint32 MemSize);
  virtual ~ReadOnlyMemoryByteStream();

  const byte* GetMemoryPointer() const { return m_pMemory; }
  uint32 GetMemorySize() const { return m_iSize; }

  virtual bool ReadByte(byte* pDestByte) override;
  virtual uint32 Read(void* pDestination, uint32 ByteCount) override;
  virtual bool Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead /* = nullptr */) override;
  virtual bool WriteByte(byte SourceByte) override;
  virtual uint32 Write(const void* pSource, uint32 ByteCount) override;
  virtual bool Write2(const void* pSource, uint32 ByteCount, uint32* pNumberOfBytesWritten /* = nullptr */) override;
  virtual bool SeekAbsolute(uint64 Offset) override;
  virtual bool SeekRelative(int64 Offset) override;
  virtual bool SeekToEnd() override;
  virtual uint64 GetSize() const override;
  virtual uint64 GetPosition() const override;
  virtual bool Flush() override;
  virtual bool Commit() override;
  virtual bool Discard() override;

private:
  const byte* m_pMemory;
  uint32 m_iPosition;
  uint32 m_iSize;
};

class GrowableMemoryByteStream : public ByteStream
{
public:
  GrowableMemoryByteStream(void* pInitialMem, uint32 InitialMemSize);
  virtual ~GrowableMemoryByteStream();

  byte* GetMemoryPointer() const { return m_pMemory; }
  uint32 GetMemorySize() const { return m_iSize; }

  virtual bool ReadByte(byte* pDestByte) override;
  virtual uint32 Read(void* pDestination, uint32 ByteCount) override;
  virtual bool Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead /* = nullptr */) override;
  virtual bool WriteByte(byte SourceByte) override;
  virtual uint32 Write(const void* pSource, uint32 ByteCount) override;
  virtual bool Write2(const void* pSource, uint32 ByteCount, uint32* pNumberOfBytesWritten /* = nullptr */) override;
  virtual bool SeekAbsolute(uint64 Offset) override;
  virtual bool SeekRelative(int64 Offset) override;
  virtual bool SeekToEnd() override;
  virtual uint64 GetSize() const override;
  virtual uint64 GetPosition() const override;
  virtual bool Flush() override;
  virtual bool Commit() override;
  virtual bool Discard() override;

private:
  void Grow(uint32 MinimumGrowth);

  byte* m_pPrivateMemory;
  byte* m_pMemory;
  uint32 m_iPosition;
  uint32 m_iSize;
  uint32 m_iMemorySize;
};

// base byte stream creation functions
// opens a local file-based stream. fills in error if passed, and returns false if the file cannot be opened.
bool ByteStream_OpenFileStream(const char* FileName, uint32 OpenMode, ByteStream** ppReturnPointer);

// memory byte stream, caller is responsible for management, therefore it can be located on either the stack or on the
// heap.
MemoryByteStream* ByteStream_CreateMemoryStream(void* pMemory, uint32 Size);

// a growable memory byte stream will automatically allocate its own memory if the provided memory is overflowed.
// a "pure heap" buffer, i.e. a buffer completely managed by this implementation, can be created by supplying a NULL
// pointer and initialSize of zero.
GrowableMemoryByteStream* ByteStream_CreateGrowableMemoryStream(void* pInitialMemory, uint32 InitialSize);
GrowableMemoryByteStream* ByteStream_CreateGrowableMemoryStream();

// readable memory stream
ReadOnlyMemoryByteStream* ByteStream_CreateReadOnlyMemoryStream(const void* pMemory, uint32 Size);

// null memory stream
NullByteStream* ByteStream_CreateNullStream();

// copies one stream's contents to another. rewinds source streams automatically, and returns it back to its old
// position.
bool ByteStream_CopyStream(ByteStream* pDestinationStream, ByteStream* pSourceStream);

// appends one stream's contents to another.
bool ByteStream_AppendStream(ByteStream* pSourceStream, ByteStream* pDestinationStream);

// copies a number of bytes from one to another
uint32 ByteStream_CopyBytes(ByteStream* pSourceStream, uint32 byteCount, ByteStream* pDestinationStream);
