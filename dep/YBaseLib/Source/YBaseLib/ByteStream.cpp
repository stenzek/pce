#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/String.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#if defined(Y_PLATFORM_WINDOWS)
#include <direct.h>
#include <io.h>
#include <share.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

Log_SetChannel(ByteStream);

class FileByteStream : public ByteStream
{
public:
  FileByteStream(FILE* pFile) : m_pFile(pFile) { DebugAssert(m_pFile != nullptr); }

  virtual ~FileByteStream() { fclose(m_pFile); }

  virtual bool ReadByte(byte* pDestByte) override
  {
    if (m_errorState)
      return false;

    if (fread(pDestByte, 1, 1, m_pFile) != 1)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

  virtual uint32 Read(void* pDestination, uint32 ByteCount) override
  {
    if (m_errorState)
      return 0;

    uint32 readCount = (uint32)fread(pDestination, 1, ByteCount, m_pFile);
    if (readCount != ByteCount && ferror(m_pFile) != 0)
      m_errorState = true;

    return readCount;
  }

  virtual bool Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead /* = nullptr */) override
  {
    if (m_errorState)
      return false;

    uint32 bytesRead = Read(pDestination, ByteCount);

    if (pNumberOfBytesRead != nullptr)
      *pNumberOfBytesRead = bytesRead;

    if (bytesRead != ByteCount)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

  virtual bool WriteByte(byte SourceByte) override
  {
    if (m_errorState)
      return false;

    if (fwrite(&SourceByte, 1, 1, m_pFile) != 1)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

  virtual uint32 Write(const void* pSource, uint32 ByteCount) override
  {
    if (m_errorState)
      return 0;

    uint32 writeCount = (uint32)fwrite(pSource, 1, ByteCount, m_pFile);
    if (writeCount != ByteCount)
      m_errorState = true;

    return writeCount;
  }

  virtual bool Write2(const void* pSource, uint32 ByteCount, uint32* pNumberOfBytesWritten /* = nullptr */) override
  {
    if (m_errorState)
      return false;

    uint32 bytesWritten = Write(pSource, ByteCount);

    if (pNumberOfBytesWritten != nullptr)
      *pNumberOfBytesWritten = bytesWritten;

    if (bytesWritten != ByteCount)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

#if defined(Y_PLATFORM_WINDOWS)

  virtual bool SeekAbsolute(uint64 Offset) override
  {
    if (m_errorState)
      return false;

    if (_fseeki64(m_pFile, Offset, SEEK_SET) != 0)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

  virtual bool SeekRelative(int64 Offset) override
  {
    if (m_errorState)
      return false;

    if (_fseeki64(m_pFile, Offset, SEEK_CUR) != 0)
    {
      m_errorState = true;
      return true;
    }

    return true;
  }

  virtual bool SeekToEnd() override
  {
    if (m_errorState)
      return false;

    if (_fseeki64(m_pFile, 0, SEEK_END) != 0)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

  virtual uint64 GetPosition() const override { return _ftelli64(m_pFile); }

  virtual uint64 GetSize() const override
  {
    int64 OldPos = _ftelli64(m_pFile);
    _fseeki64(m_pFile, 0, SEEK_END);
    int64 Size = _ftelli64(m_pFile);
    _fseeki64(m_pFile, OldPos, SEEK_SET);
    return (uint64)Size;
  }

#elif defined(Y_PLATFORM_POSIX) || defined(Y_PLATFORM_HTML5) || defined(Y_PLATFORM_ANDROID)

  virtual bool SeekAbsolute(uint64 Offset) override
  {
    if (m_errorState)
      return false;

    if (fseeko(m_pFile, static_cast<off_t>(Offset), SEEK_SET) != 0)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

  virtual bool SeekRelative(int64 Offset) override
  {
    if (m_errorState)
      return false;

    if (fseeko(m_pFile, static_cast<off_t>(Offset), SEEK_CUR) != 0)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

  virtual bool SeekToEnd() override
  {
    if (m_errorState)
      return false;

    if (fseeko(m_pFile, 0, SEEK_END) != 0)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

  virtual uint64 GetPosition() const override { return static_cast<uint64>(ftello(m_pFile)); }

  virtual uint64 GetSize() const override
  {
    off_t OldPos = ftello(m_pFile);
    fseeko(m_pFile, 0, SEEK_END);
    off_t Size = ftello(m_pFile);
    fseeko(m_pFile, OldPos, SEEK_SET);
    return (uint64)Size;
  }

#endif

  virtual bool Flush() override
  {
    if (m_errorState)
      return false;

    if (fflush(m_pFile) != 0)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

  virtual bool Commit() override { return true; }

  virtual bool Discard() override { return false; }

protected:
  FILE* m_pFile;
};

class AtomicUpdatedFileByteStream : public FileByteStream
{
public:
  AtomicUpdatedFileByteStream(FILE* pFile, const char* originalFileName, const char* temporaryFileName)
    : FileByteStream(pFile), m_committed(false), m_discarded(false), m_originalFileName(originalFileName),
      m_temporaryFileName(temporaryFileName)
  {
  }

  virtual ~AtomicUpdatedFileByteStream()
  {
    if (m_discarded)
    {
#if Y_PLATFORM_WINDOWS
      // delete the temporary file
      if (!DeleteFileA(m_temporaryFileName))
        Log_WarningPrintf(
          "AtomicUpdatedFileByteStream::~AtomicUpdatedFileByteStream(): Failed to delete temporary file '%s'",
          m_temporaryFileName.GetCharArray());
#else
      // delete the temporary file
      if (remove(m_temporaryFileName) < 0)
        Log_WarningPrintf(
          "AtomicUpdatedFileByteStream::~AtomicUpdatedFileByteStream(): Failed to delete temporary file '%s'",
          m_temporaryFileName.GetCharArray());
#endif
    }
    else if (!m_committed)
    {
      Commit();
    }

    // fclose called by FileByteStream destructor
  }

  virtual bool Flush() override
  {
    if (fflush(m_pFile) != 0)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

  virtual bool Commit() override
  {
    Assert(!m_discarded);
    if (m_committed)
      return Flush();

    fflush(m_pFile);

#ifdef Y_PLATFORM_WINDOWS
    // move the atomic file name to the original file name
    if (!MoveFileExA(m_temporaryFileName, m_originalFileName, MOVEFILE_REPLACE_EXISTING))
    {
      Log_WarningPrintf("AtomicUpdatedFileByteStream::Commit(): Failed to rename temporary file '%s' to '%s'",
                        m_temporaryFileName.GetCharArray(), m_originalFileName.GetCharArray());
      m_discarded = true;
    }
    else
    {
      m_committed = true;
    }
#else
    // move the atomic file name to the original file name
    if (rename(m_temporaryFileName, m_originalFileName) < 0)
    {
      Log_WarningPrintf("AtomicUpdatedFileByteStream::Commit(): Failed to rename temporary file '%s' to '%s'",
                        m_temporaryFileName.GetCharArray(), m_originalFileName.GetCharArray());
      m_discarded = true;
    }
    else
    {
      m_committed = true;
    }
#endif

    return (!m_discarded);
  }

  virtual bool Discard() override
  {
    Assert(!m_committed);
    m_discarded = true;
    return true;
  }

private:
  bool m_committed;
  bool m_discarded;
  String m_originalFileName;
  String m_temporaryFileName;
};

NullByteStream::NullByteStream() {}

NullByteStream::~NullByteStream() {}

bool NullByteStream::ReadByte(byte* pDestByte)
{
  *pDestByte = 0;
  return true;
}

uint32 NullByteStream::Read(void* pDestination, uint32 ByteCount)
{
  if (ByteCount > 0)
    Y_memzero(pDestination, ByteCount);

  return ByteCount;
}

bool NullByteStream::Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead /* = nullptr */)
{
  if (ByteCount > 0)
    Y_memzero(pDestination, ByteCount);

  if (pNumberOfBytesRead)
    *pNumberOfBytesRead = ByteCount;

  return true;
}

bool NullByteStream::WriteByte(byte SourceByte)
{
  return true;
}

uint32 NullByteStream::Write(const void* pSource, uint32 ByteCount)
{
  return ByteCount;
}

bool NullByteStream::Write2(const void* pSource, uint32 ByteCount, uint32* pNumberOfBytesWritten /* = nullptr */)
{
  return true;
}

bool NullByteStream::SeekAbsolute(uint64 Offset)
{
  return true;
}

bool NullByteStream::SeekRelative(int64 Offset)
{
  return true;
}

bool NullByteStream::SeekToEnd()
{
  return true;
}

uint64 NullByteStream::GetSize() const
{
  return 0;
}

uint64 NullByteStream::GetPosition() const
{
  return 0;
}

bool NullByteStream::Flush()
{
  return true;
}

bool NullByteStream::Commit()
{
  return true;
}

bool NullByteStream::Discard()
{
  return true;
}

MemoryByteStream::MemoryByteStream(void* pMemory, uint32 MemSize)
{
  m_iPosition = 0;
  m_iSize = MemSize;
  m_pMemory = (byte*)pMemory;
}

MemoryByteStream::~MemoryByteStream() {}

bool MemoryByteStream::ReadByte(byte* pDestByte)
{
  if (m_iPosition < m_iSize)
  {
    *pDestByte = m_pMemory[m_iPosition++];
    return true;
  }

  return false;
}

uint32 MemoryByteStream::Read(void* pDestination, uint32 ByteCount)
{
  uint32 sz = ByteCount;
  if ((m_iPosition + ByteCount) > m_iSize)
    sz = m_iSize - m_iPosition;

  if (sz > 0)
  {
    std::memcpy(pDestination, m_pMemory + m_iPosition, sz);
    m_iPosition += sz;
  }

  return sz;
}

bool MemoryByteStream::Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead /* = nullptr */)
{
  uint32 r = Read(pDestination, ByteCount);
  if (pNumberOfBytesRead != NULL)
    *pNumberOfBytesRead = r;

  return (r == ByteCount);
}

bool MemoryByteStream::WriteByte(byte SourceByte)
{
  if (m_iPosition < m_iSize)
  {
    m_pMemory[m_iSize++] = SourceByte;
    return true;
  }

  return false;
}

uint32 MemoryByteStream::Write(const void* pSource, uint32 ByteCount)
{
  uint32 sz = ByteCount;
  if ((m_iPosition + ByteCount) > m_iSize)
    sz = m_iSize - m_iPosition;

  if (sz > 0)
  {
    std::memcpy(m_pMemory + m_iPosition, pSource, sz);
    m_iPosition += sz;
  }

  return sz;
}

bool MemoryByteStream::Write2(const void* pSource, uint32 ByteCount, uint32* pNumberOfBytesWritten /* = nullptr */)
{
  uint32 r = Write(pSource, ByteCount);
  if (pNumberOfBytesWritten != nullptr)
    *pNumberOfBytesWritten = r;

  return (r == ByteCount);
}

bool MemoryByteStream::SeekAbsolute(uint64 Offset)
{
  uint32 Offset32 = (uint32)Offset;
  if (Offset32 > m_iSize)
    return false;

  m_iPosition = Offset32;
  return true;
}

bool MemoryByteStream::SeekRelative(int64 Offset)
{
  int32 Offset32 = (int32)Offset;
  if ((Offset32 < 0 && -Offset32 > (int32)m_iPosition) || (uint32)((int32)m_iPosition + Offset32) > m_iSize)
    return false;

  m_iPosition += Offset32;
  return true;
}

bool MemoryByteStream::SeekToEnd()
{
  m_iPosition = m_iSize;
  return true;
}

uint64 MemoryByteStream::GetSize() const
{
  return (uint64)m_iSize;
}

uint64 MemoryByteStream::GetPosition() const
{
  return (uint64)m_iPosition;
}

bool MemoryByteStream::Flush()
{
  return true;
}

bool MemoryByteStream::Commit()
{
  return true;
}

bool MemoryByteStream::Discard()
{
  return false;
}

ReadOnlyMemoryByteStream::ReadOnlyMemoryByteStream(const void* pMemory, uint32 MemSize)
{
  m_iPosition = 0;
  m_iSize = MemSize;
  m_pMemory = reinterpret_cast<const byte*>(pMemory);
}

ReadOnlyMemoryByteStream::~ReadOnlyMemoryByteStream() {}

bool ReadOnlyMemoryByteStream::ReadByte(byte* pDestByte)
{
  if (m_iPosition < m_iSize)
  {
    *pDestByte = m_pMemory[m_iPosition++];
    return true;
  }

  return false;
}

uint32 ReadOnlyMemoryByteStream::Read(void* pDestination, uint32 ByteCount)
{
  uint32 sz = ByteCount;
  if ((m_iPosition + ByteCount) > m_iSize)
    sz = m_iSize - m_iPosition;

  if (sz > 0)
  {
    std::memcpy(pDestination, m_pMemory + m_iPosition, sz);
    m_iPosition += sz;
  }

  return sz;
}

bool ReadOnlyMemoryByteStream::Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead /* = nullptr */)
{
  uint32 r = Read(pDestination, ByteCount);
  if (pNumberOfBytesRead != nullptr)
    *pNumberOfBytesRead = r;

  return (r == ByteCount);
}

bool ReadOnlyMemoryByteStream::WriteByte(byte SourceByte)
{
  return false;
}

uint32 ReadOnlyMemoryByteStream::Write(const void* pSource, uint32 ByteCount)
{
  return 0;
}

bool ReadOnlyMemoryByteStream::Write2(const void* pSource, uint32 ByteCount,
                                      uint32* pNumberOfBytesWritten /* = nullptr */)
{
  return false;
}

bool ReadOnlyMemoryByteStream::SeekAbsolute(uint64 Offset)
{
  uint32 Offset32 = (uint32)Offset;
  if (Offset32 > m_iSize)
    return false;

  m_iPosition = Offset32;
  return true;
}

bool ReadOnlyMemoryByteStream::SeekRelative(int64 Offset)
{
  int32 Offset32 = (int32)Offset;
  if ((Offset32 < 0 && -Offset32 > (int32)m_iPosition) || (uint32)((int32)m_iPosition + Offset32) > m_iSize)
    return false;

  m_iPosition += Offset32;
  return true;
}

bool ReadOnlyMemoryByteStream::SeekToEnd()
{
  m_iPosition = m_iSize;
  return true;
}

uint64 ReadOnlyMemoryByteStream::GetSize() const
{
  return (uint64)m_iSize;
}

uint64 ReadOnlyMemoryByteStream::GetPosition() const
{
  return (uint64)m_iPosition;
}

bool ReadOnlyMemoryByteStream::Flush()
{
  return false;
}

bool ReadOnlyMemoryByteStream::Commit()
{
  return false;
}

bool ReadOnlyMemoryByteStream::Discard()
{
  return false;
}

GrowableMemoryByteStream::GrowableMemoryByteStream(void* pInitialMem, uint32 InitialMemSize)
{
  m_iPosition = 0;
  m_iSize = 0;

  if (pInitialMem != nullptr)
  {
    m_iMemorySize = InitialMemSize;
    m_pPrivateMemory = nullptr;
    m_pMemory = (byte*)pInitialMem;
  }
  else
  {
    m_iMemorySize = Max(InitialMemSize, (uint32)64);
    m_pPrivateMemory = m_pMemory = (byte*)std::malloc(m_iMemorySize);
  }
}

GrowableMemoryByteStream::~GrowableMemoryByteStream()
{
  if (m_pPrivateMemory != nullptr)
    std::free(m_pPrivateMemory);
}

bool GrowableMemoryByteStream::ReadByte(byte* pDestByte)
{
  if (m_iPosition < m_iSize)
  {
    *pDestByte = m_pMemory[m_iPosition++];
    return true;
  }

  return false;
}

uint32 GrowableMemoryByteStream::Read(void* pDestination, uint32 ByteCount)
{
  uint32 sz = ByteCount;
  if ((m_iPosition + ByteCount) > m_iSize)
    sz = m_iSize - m_iPosition;

  if (sz > 0)
  {
    std::memcpy(pDestination, m_pMemory + m_iPosition, sz);
    m_iPosition += sz;
  }

  return sz;
}

bool GrowableMemoryByteStream::Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead /* = nullptr */)
{
  uint32 r = Read(pDestination, ByteCount);
  if (pNumberOfBytesRead != NULL)
    *pNumberOfBytesRead = r;

  return (r == ByteCount);
}

bool GrowableMemoryByteStream::WriteByte(byte SourceByte)
{
  if (m_iPosition == m_iMemorySize)
    Grow(1);

  m_pMemory[m_iPosition++] = SourceByte;
  m_iSize = Max(m_iSize, m_iPosition);
  return true;
}

uint32 GrowableMemoryByteStream::Write(const void* pSource, uint32 ByteCount)
{
  if ((m_iPosition + ByteCount) > m_iMemorySize)
    Grow(ByteCount);

  std::memcpy(m_pMemory + m_iPosition, pSource, ByteCount);
  m_iPosition += ByteCount;
  m_iSize = Max(m_iSize, m_iPosition);
  return ByteCount;
}

bool GrowableMemoryByteStream::Write2(const void* pSource, uint32 ByteCount,
                                      uint32* pNumberOfBytesWritten /* = nullptr */)
{
  uint32 r = Write(pSource, ByteCount);
  if (pNumberOfBytesWritten != nullptr)
    *pNumberOfBytesWritten = r;

  return (r == ByteCount);
}

bool GrowableMemoryByteStream::SeekAbsolute(uint64 Offset)
{
  uint32 Offset32 = (uint32)Offset;
  if (Offset32 > m_iSize)
    return false;

  m_iPosition = Offset32;
  return true;
}

bool GrowableMemoryByteStream::SeekRelative(int64 Offset)
{
  int32 Offset32 = (int32)Offset;
  if ((Offset32 < 0 && -Offset32 > (int32)m_iPosition) || (uint32)((int32)m_iPosition + Offset32) > m_iSize)
    return false;

  m_iPosition += Offset32;
  return true;
}

bool GrowableMemoryByteStream::SeekToEnd()
{
  m_iPosition = m_iSize;
  return true;
}

uint64 GrowableMemoryByteStream::GetSize() const
{
  return (uint64)m_iSize;
}

uint64 GrowableMemoryByteStream::GetPosition() const
{
  return (uint64)m_iPosition;
}

bool GrowableMemoryByteStream::Flush()
{
  return true;
}

bool GrowableMemoryByteStream::Commit()
{
  return true;
}

bool GrowableMemoryByteStream::Discard()
{
  return false;
}

void GrowableMemoryByteStream::Grow(uint32 MinimumGrowth)
{
  uint32 NewSize = Max(m_iMemorySize + MinimumGrowth, m_iMemorySize * 2);
  if (m_pPrivateMemory == nullptr)
  {
    m_pPrivateMemory = (byte*)std::malloc(NewSize);
    std::memcpy(m_pPrivateMemory, m_pMemory, m_iSize);
    m_pMemory = m_pPrivateMemory;
    m_iMemorySize = NewSize;
  }
  else
  {
    m_pPrivateMemory = m_pMemory = (byte*)std::realloc(m_pPrivateMemory, NewSize);
    m_iMemorySize = NewSize;
  }
}

#if defined(Y_PLATFORM_WINDOWS)

bool ByteStream_OpenFileStream(const char* fileName, uint32 openMode, ByteStream** ppReturnPointer)
{
  if ((openMode & (BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE)) == BYTESTREAM_OPEN_WRITE)
  {
    // if opening with write but not create, the path must exist.
    if (GetFileAttributes(fileName) == INVALID_FILE_ATTRIBUTES)
      return false;
  }

  char modeString[16];
  uint32 modeStringLength = 0;

  if (openMode & BYTESTREAM_OPEN_WRITE)
  {
    // if the file exists, use r+, otherwise w+
    // HACK: if we're not truncating, and the file exists (we want to only update it), we still have to use r+
    if ((openMode & BYTESTREAM_OPEN_TRUNCATE) || GetFileAttributes(fileName) == INVALID_FILE_ATTRIBUTES)
    {
      modeString[modeStringLength++] = 'w';
      if (openMode & BYTESTREAM_OPEN_READ)
        modeString[modeStringLength++] = '+';
    }
    else
    {
      modeString[modeStringLength++] = 'r';
      modeString[modeStringLength++] = '+';
    }

    modeString[modeStringLength++] = 'b';
  }
  else if (openMode & BYTESTREAM_OPEN_READ)
  {
    modeString[modeStringLength++] = 'r';
    modeString[modeStringLength++] = 'b';
  }

  // doesn't work with _fdopen
  if (!(openMode & BYTESTREAM_OPEN_ATOMIC_UPDATE))
  {
    if (openMode & BYTESTREAM_OPEN_STREAMED)
      modeString[modeStringLength++] = 'S';
    else if (openMode & BYTESTREAM_OPEN_SEEKABLE)
      modeString[modeStringLength++] = 'R';
  }

  modeString[modeStringLength] = 0;

  if (openMode & BYTESTREAM_OPEN_CREATE_PATH)
  {
    uint32 i;
    uint32 fileNameLength = Y_strlen(fileName);
    char* tempStr = (char*)alloca(fileNameLength + 1);

    // check if it starts with a drive letter. if so, skip ahead
    if (fileNameLength >= 2 && fileName[1] == ':')
    {
      if (fileNameLength <= 3)
      {
        // create a file called driveletter: or driveletter:\ ? you must be crazy
        i = fileNameLength;
      }
      else
      {
        std::memcpy(tempStr, fileName, 3);
        i = 3;
      }
    }
    else
    {
      // start at beginning
      i = 0;
    }

    // step through each path component, create folders as necessary
    for (; i < fileNameLength; i++)
    {
      if (i > 0 && (fileName[i] == '\\' || fileName[i] == '/'))
      {
        // terminate the string
        tempStr[i] = '\0';

        // check if it exists
        struct stat s;
        if (stat(tempStr, &s) < 0)
        {
          if (errno == ENOENT)
          {
            // try creating it
            if (_mkdir(tempStr) < 0)
            {
              // no point trying any further down the chain
              break;
            }
          }
          else // if (errno == ENOTDIR)
          {
            // well.. someone's trying to open a fucking weird path that is comprised of both directories and files...
            // I aint sticking around here to find out what disaster awaits... let fopen deal with it
            break;
          }
        }

// append platform path seperator
#if defined(Y_PLATFORM_WINDOWS)
        tempStr[i] = '\\';
#else
        tempStr[i] = '/';
#endif
      }
      else
      {
        // append character to temp string
        tempStr[i] = fileName[i];
      }
    }
  }

  if (openMode & BYTESTREAM_OPEN_ATOMIC_UPDATE)
  {
    DebugAssert(openMode & (BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE));

    // generate the temporary file name
    uint32 fileNameLength = Y_strlen(fileName);
    char* temporaryFileName = (char*)alloca(fileNameLength + 8);
    Y_snprintf(temporaryFileName, fileNameLength + 8, "%s.XXXXXX", fileName);

    // fill in random characters
    _mktemp_s(temporaryFileName, fileNameLength + 8);

    // open the file
    errno_t err;
    FILE* pTemporaryFile;

    // massive hack here
    DWORD desiredAccess = GENERIC_WRITE;
    if (openMode & BYTESTREAM_OPEN_READ)
      desiredAccess |= GENERIC_READ;
    HANDLE hFile = CreateFileA(temporaryFileName, desiredAccess, FILE_SHARE_DELETE, NULL, CREATE_NEW, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
      return false;

    // get fd from this
    int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hFile), 0);
    if (fd < 0)
    {
      CloseHandle(hFile);
      DeleteFileA(temporaryFileName);
      return false;
    }

    // convert to a stream
    pTemporaryFile = _fdopen(fd, modeString);
    if (pTemporaryFile == nullptr)
    {
      close(fd);
      DeleteFileA(temporaryFileName);
      return false;
    }

    // create the stream pointer
    AtomicUpdatedFileByteStream* pStream = new AtomicUpdatedFileByteStream(pTemporaryFile, fileName, temporaryFileName);

    // do we need to copy the existing file into this one?
    if (!(openMode & BYTESTREAM_OPEN_TRUNCATE))
    {
      FILE* pOriginalFile;
      err = fopen_s(&pOriginalFile, fileName, "rb");
      if (err != 0 || pOriginalFile == nullptr)
      {
        // this will delete the temporary file
        pStream->Discard();
        pStream->Release();
        return false;
      }

      static const size_t BUFFERSIZE = 4096;
      byte buffer[BUFFERSIZE];
      while (!feof(pOriginalFile))
      {
        size_t nBytes = fread(buffer, BUFFERSIZE, sizeof(byte), pOriginalFile);
        if (nBytes == 0)
          break;

        if (pStream->Write(buffer, (uint32)nBytes) != (uint32)nBytes)
        {
          pStream->Discard();
          pStream->Release();
          fclose(pOriginalFile);
          return false;
        }
      }

      // close original file
      fclose(pOriginalFile);
    }

    // return pointer
    *ppReturnPointer = pStream;
    return true;
  }
  else
  {
    // forward through
    FILE* pFile;
    errno_t err = fopen_s(&pFile, fileName, modeString);
    if (err != 0 || pFile == NULL)
      return false;

    *ppReturnPointer = new FileByteStream(pFile);
    return true;
  }
}

#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG) || defined(Y_COMPILER_EMSCRIPTEN)

bool ByteStream_OpenFileStream(const char* fileName, uint32 openMode, ByteStream** ppReturnPointer)
{
  if ((openMode & (BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE)) == BYTESTREAM_OPEN_WRITE)
  {
    // if opening with write but not create, the path must exist.
    struct stat s;
    if (stat(fileName, &s) < 0)
      return false;
  }

  char modeString[16];
  uint32 modeStringLength = 0;

  if (openMode & BYTESTREAM_OPEN_WRITE)
  {
    if (openMode & BYTESTREAM_OPEN_TRUNCATE)
      modeString[modeStringLength++] = 'w';
    else
      modeString[modeStringLength++] = 'a';

    modeString[modeStringLength++] = 'b';

    if (openMode & BYTESTREAM_OPEN_READ)
      modeString[modeStringLength++] = '+';
  }
  else if (openMode & BYTESTREAM_OPEN_READ)
  {
    modeString[modeStringLength++] = 'r';
    modeString[modeStringLength++] = 'b';
  }

  modeString[modeStringLength] = 0;

  if (openMode & BYTESTREAM_OPEN_CREATE_PATH)
  {
    uint32 i;
    uint32 fileNameLength = Y_strlen(fileName);
    char* tempStr = (char*)alloca(fileNameLength + 1);

#if defined(Y_PLATFORM_WINDOWS)
    // check if it starts with a drive letter. if so, skip ahead
    if (fileNameLength >= 2 && fileName[1] == ':')
    {
      if (fileNameLength <= 3)
      {
        // create a file called driveletter: or driveletter:\ ? you must be crazy
        i = fileNameLength;
      }
      else
      {
        std::memcpy(tempStr, fileName, 3);
        i = 3;
      }
    }
    else
    {
      // start at beginning
      i = 0;
    }
#endif

    // step through each path component, create folders as necessary
    for (i = 0; i < fileNameLength; i++)
    {
      if (i > 0 && (fileName[i] == '\\' || fileName[i] == '/') && fileName[i] != ':')
      {
        // terminate the string
        tempStr[i] = '\0';

        // check if it exists
        struct stat s;
        if (stat(tempStr, &s) < 0)
        {
          if (errno == ENOENT)
          {
          // try creating it
#if defined(Y_PLATFORM_WINDOWS)
            if (mkdir(tempStr) < 0)
#else
            if (mkdir(tempStr, 0777) < 0)
#endif
            {
              // no point trying any further down the chain
              break;
            }
          }
          else // if (errno == ENOTDIR)
          {
            // well.. someone's trying to open a fucking weird path that is comprised of both directories and files...
            // I aint sticking around here to find out what disaster awaits... let fopen deal with it
            break;
          }
        }

// append platform path seperator
#if defined(Y_PLATFORM_WINDOWS)
        tempStr[i] = '\\';
#else
        tempStr[i] = '/';
#endif
      }
      else
      {
        // append character to temp string
        tempStr[i] = fileName[i];
      }
    }
  }

  if (openMode & BYTESTREAM_OPEN_ATOMIC_UPDATE)
  {
    DebugAssert(openMode & (BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE));

    // generate the temporary file name
    uint32 fileNameLength = Y_strlen(fileName);
    char* temporaryFileName = (char*)alloca(fileNameLength + 8);
    Y_snprintf(temporaryFileName, fileNameLength + 8, "%s.XXXXXX", fileName);

    // fill in random characters
    mktemp(temporaryFileName);

    // open the file
    FILE* pTemporaryFile = fopen(temporaryFileName, modeString);
    if (pTemporaryFile == nullptr)
      return false;

    // create the stream pointer
    AtomicUpdatedFileByteStream* pStream = new AtomicUpdatedFileByteStream(pTemporaryFile, fileName, temporaryFileName);

    // do we need to copy the existing file into this one?
    if (!(openMode & BYTESTREAM_OPEN_TRUNCATE))
    {
      FILE* pOriginalFile = fopen(fileName, "rb");
      if (pOriginalFile == nullptr)
      {
        // this will delete the temporary file
        pStream->SetErrorState();
        pStream->Release();
        return false;
      }

      static const size_t BUFFERSIZE = 4096;
      byte buffer[BUFFERSIZE];
      while (!feof(pOriginalFile))
      {
        size_t nBytes = fread(buffer, BUFFERSIZE, sizeof(byte), pOriginalFile);
        if (nBytes == 0)
          break;

        if (pStream->Write(buffer, (uint32)nBytes) != (uint32)nBytes)
        {
          pStream->SetErrorState();
          pStream->Release();
          fclose(pOriginalFile);
          return false;
        }
      }

      // close original file
      fclose(pOriginalFile);
    }

    // return pointer
    *ppReturnPointer = pStream;
    return true;
  }
  else
  {
    FILE* pFile = fopen(fileName, modeString);
    if (pFile == nullptr)
      return false;

    *ppReturnPointer = new FileByteStream(pFile);
    return true;
  }
}

#endif

MemoryByteStream* ByteStream_CreateMemoryStream(void* pMemory, uint32 Size)
{
  DebugAssert(pMemory != nullptr && Size > 0);
  return new MemoryByteStream(pMemory, Size);
}

ReadOnlyMemoryByteStream* ByteStream_CreateReadOnlyMemoryStream(const void* pMemory, uint32 Size)
{
  DebugAssert(pMemory != nullptr && Size > 0);
  return new ReadOnlyMemoryByteStream(pMemory, Size);
}

NullByteStream* ByteStream_CreateNullStream()
{
  return new NullByteStream();
}

GrowableMemoryByteStream* ByteStream_CreateGrowableMemoryStream(void* pInitialMemory, uint32 InitialSize)
{
  return new GrowableMemoryByteStream(pInitialMemory, InitialSize);
}

GrowableMemoryByteStream* ByteStream_CreateGrowableMemoryStream()
{
  return new GrowableMemoryByteStream(nullptr, 0);
}

bool ByteStream_CopyStream(ByteStream* pDestinationStream, ByteStream* pSourceStream)
{
  const uint32 chunkSize = 4096;
  byte chunkData[chunkSize];

  uint64 oldSourcePosition = pSourceStream->GetPosition();
  if (!pSourceStream->SeekAbsolute(0) || !pDestinationStream->SeekAbsolute(0))
    return false;

  bool success = false;
  for (;;)
  {
    uint32 nBytes = pSourceStream->Read(chunkData, chunkSize);
    if (nBytes == 0)
    {
      success = true;
      break;
    }

    if (pDestinationStream->Write(chunkData, nBytes) != nBytes)
      break;
  }

  return (pSourceStream->SeekAbsolute(oldSourcePosition) && success);
}

bool ByteStream_AppendStream(ByteStream* pSourceStream, ByteStream* pDestinationStream)
{
  const uint32 chunkSize = 4096;
  byte chunkData[chunkSize];

  uint64 oldSourcePosition = pSourceStream->GetPosition();
  if (!pSourceStream->SeekAbsolute(0))
    return false;

  bool success = false;
  for (;;)
  {
    uint32 nBytes = pSourceStream->Read(chunkData, chunkSize);
    if (nBytes == 0)
    {
      success = true;
      break;
    }

    if (pDestinationStream->Write(chunkData, nBytes) != nBytes)
      break;
  }

  return (pSourceStream->SeekAbsolute(oldSourcePosition) && success);
}

uint32 ByteStream_CopyBytes(ByteStream* pSourceStream, uint32 byteCount, ByteStream* pDestinationStream)
{
  const uint32 chunkSize = 4096;
  byte chunkData[chunkSize];

  uint32 remaining = byteCount;
  while (remaining > 0)
  {
    uint32 toCopy = Min(remaining, chunkSize);
    uint32 bytesRead = pSourceStream->Read(chunkData, toCopy);
    if (bytesRead == 0)
      break;

    uint32 bytesWritten = pDestinationStream->Write(chunkData, bytesRead);
    if (bytesWritten == 0)
      break;

    remaining -= bytesWritten;
  }

  return byteCount - remaining;
}
