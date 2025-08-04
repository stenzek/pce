#include "YBaseLib/ZipArchive.h"

#ifdef HAVE_ZLIB
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Timestamp.h"
Log_SetChannel(ZipArchive);

#include <zlib.h>

// Ensure zlib gets pulled in
#if Y_COMPILER_MSVC
#pragma comment(lib, "zdll.lib")
#endif

// zip file constants
static const uint32 ZIP_ARCHIVE_VERSION_MADE_BY = 0;
static const uint32 ZIP_ARCHIVE_VERSION_TO_EXTRACT = 20;
static const uint32 ZIP_ARCHIVE_LOCAL_FILE_HEADER_SIGNTURE = 0x04034b50;
static const uint32 ZIP_ARCHIVE_DATA_DESCRIPTOR_SIGNATURE = 0x08074b50;
static const uint32 ZIP_ARCHIVE_CENTRAL_DIRECTORY_FILE_HEADER_SIGNATURE = 0x02014b50;
static const uint32 ZIP_ARCHIVE_END_OF_CENTRAL_DIRECTORY_SIGNATURE = 0x06054b50;

static const uint32 ZIP_STREAM_BUFFER_SIZE = 16384;
static const uint32 ZIP_BUFFERED_IO_CHUNK_SIZE = 4096;

struct ZIP_ARCHIVE_LOCAL_FILE_HEADER
{
  uint32 Signature;
  uint16 VersionRequiredToExtract;
  uint16 Flags;
  uint16 CompressionMethod;
  uint32 ModificationTimestamp;
  uint32 CRC32;
  uint64 CompressedSize;
  uint64 DecompressedSize;
  uint16 FilenameLength;
  uint16 ExtraFieldLength;
};

static Timestamp ZipTimeToTimestamp(const uint32 ziptime)
{
  uint32 datePart = ziptime >> 16;
  uint32 timePart = ziptime & 0xFFFF;

  Timestamp::ExpandedTime expandedTime;
  expandedTime.Year = ((datePart & 0xFE00) / 0x200) + 1980;
  expandedTime.Month = (datePart & 0x1E0) / 0x20;
  expandedTime.DayOfMonth = datePart & 0x1F;
  expandedTime.Hour = (timePart & 0xF800) / 0x800;
  expandedTime.Minute = (timePart & 0x7E0) / 0x20;
  expandedTime.Second = (timePart & 0x1F) * 2;
  expandedTime.Milliseconds = 0;

  return Timestamp::FromExpandedTime(expandedTime);
}

static uint32 TimestampToZipTime(const Timestamp& ts)
{
  Timestamp::ExpandedTime expandedTime = ts.AsExpandedTime();

  uint32 year = expandedTime.Year;
  if (year >= 1980)
    year -= 1980;
  else if (year >= 80)
    year -= 80;

  return (((expandedTime.DayOfMonth) + (32 * expandedTime.Month) + (512 * year)) << 16) |
         ((expandedTime.Second / 2) + (32 * expandedTime.Minute) + (2048 * expandedTime.Hour));
}

class ZipArchiveStreamedReadByteStream : public ByteStream
{
public:
  ZipArchiveStreamedReadByteStream(ZipArchive* pZipArchive, ByteStream* pArchiveStream, uint64 baseOffset,
                                   ZIP_ARCHIVE_LOCAL_FILE_HEADER* pLocalFileHeader)
    : m_pZipArchive(pZipArchive), m_pArchiveStream(pArchiveStream), m_baseOffset(baseOffset),
      m_currentFileOffset(baseOffset), m_inBufferBytes(0), m_inBufferPosition(0), m_currentDecompressedOffset(0),
      m_currentCRC32(0)
  {
    std::memcpy(&m_localFileHeader, pLocalFileHeader, sizeof(m_localFileHeader));

    switch (m_localFileHeader.CompressionMethod)
    {
      case Z_DEFLATED:
      {
        Y_memzero(&m_zStream, sizeof(m_zStream));
        if (inflateInit2(&m_zStream, -MAX_WBITS) != Z_OK)
          Panic("inflateInit2 failed");
      }
      break;
    }

    m_pZipArchive->m_nOpenStreamedReads++;
  }

  ~ZipArchiveStreamedReadByteStream()
  {
    m_pZipArchive->m_nOpenStreamedReads--;

    switch (m_localFileHeader.CompressionMethod)
    {
      case Z_DEFLATED:
      {
        inflateEnd(&m_zStream);
      }
      break;
    }
  }

  bool FillInputBuffer()
  {
    if (m_errorState)
      return false;

    if (m_currentFileOffset == (m_baseOffset + m_localFileHeader.CompressedSize))
      return false;

    if (!m_pArchiveStream->SeekAbsolute(m_currentFileOffset))
    {
      m_errorState = true;
      return false;
    }

    // needs more input buffer FIXME
    uint32 readSize =
      (uint32)Min(m_localFileHeader.CompressedSize - (m_currentFileOffset - m_baseOffset), (uint64)sizeof(m_pInBuffer));
    uint32 nInputBytes = m_pArchiveStream->Read(m_pInBuffer, readSize);
    if (nInputBytes == 0 || m_pArchiveStream->InErrorState())
    {
      m_errorState = true;
      return false;
    }

    m_currentFileOffset += (uint64)nInputBytes;
    m_inBufferBytes = nInputBytes;
    return true;
  }

  virtual uint32 Read(void* pDestination, uint32 ByteCount)
  {
    if (m_errorState)
      return 0;

    switch (m_localFileHeader.CompressionMethod)
    {
        // uncompressed
      case 0:
      {
        byte* pCurrentPtr = reinterpret_cast<byte*>(pDestination);
        uint32 remaining = ByteCount;

        while (remaining > 0)
        {
          if (m_inBufferPosition == m_inBufferBytes)
          {
            if (!FillInputBuffer())
              break;

            m_inBufferPosition = 0;
          }

          uint32 copyLength = Min(remaining, m_inBufferBytes - m_inBufferPosition);
          std::memcpy(pCurrentPtr, m_pInBuffer + m_inBufferPosition, copyLength);
          m_currentCRC32 = crc32(m_currentCRC32, pCurrentPtr, copyLength);
          m_currentDecompressedOffset += (uint64)copyLength;
          m_inBufferPosition += copyLength;
          pCurrentPtr += copyLength;
          remaining -= copyLength;
        }

        if (m_currentDecompressedOffset == m_localFileHeader.DecompressedSize &&
            m_currentCRC32 != m_localFileHeader.CRC32)
        {
          Log_ErrorPrintf("ZipArchiveStreamedReadByteStream::Read: CRC mismatch: %u / %u", m_currentCRC32,
                          m_localFileHeader.CRC32);
          m_errorState = true;
          return 0;
        }

        return (ByteCount - remaining);
      }
      break;

        // deflate
      case Z_DEFLATED:
      {
        m_zStream.avail_out = ByteCount;
        m_zStream.next_out = (Bytef*)pDestination;

        while (m_zStream.avail_out > 0)
        {
          if (m_zStream.avail_in == 0)
          {
            // if there is no input bytes, there can still be output bytes if they're repeating
            // this logic could probably be written a bit better though (maybe Z_BUF_ERROR)
            if (FillInputBuffer())
            {
              m_zStream.avail_in = m_inBufferBytes;
              m_zStream.next_in = (Bytef*)m_pInBuffer;
            }
          }

          // save the buffer pointer
          Bytef* pOldPointer = m_zStream.next_out;

          // inflate it
          int err = inflate(&m_zStream, Z_SYNC_FLUSH);
          if (err != Z_OK && err != Z_STREAM_END)
          {
            // some other error
            Log_ErrorPrintf("ZipArchiveStreamedReadByteStream::Read: zlib returned error: %d", err);
            m_errorState = true;
            break;
          }

          // update crc32
          if (m_zStream.next_out != pOldPointer)
            m_currentCRC32 = crc32(m_currentCRC32, pOldPointer, static_cast<uInt>(m_zStream.next_out - pOldPointer));

          // handle return
          if (err == Z_OK)
          {
            // done this block, so continue with the next one if required
            continue;
          }
          else if (err == Z_STREAM_END)
          {
            // check crc32
            if (m_currentCRC32 != m_localFileHeader.CRC32)
            {
              // error
              Log_ErrorPrintf("ZipArchiveStreamedReadByteStream::Read: CRC mismatch: %u / %u", m_currentCRC32,
                              m_localFileHeader.CRC32);
              m_errorState = true;
              return 0;
            }

            // end of stream, so exit the loop
            break;
          }
        }

        uint32 bytesRead = (ByteCount - (uint32)m_zStream.avail_out);
        m_currentDecompressedOffset += (uint64)bytesRead;
        m_zStream.avail_out = 0;
        m_zStream.next_out = NULL;
        return bytesRead;
      }
      break;

        // unknown method
      default:
      {
        Log_ErrorPrintf("ZipArchiveStreamedReadByteStream::Read: Unknown compression method: %u",
                        (uint32)m_localFileHeader.CompressionMethod);
        m_errorState = true;
        return 0;
      }
      break;
    }
  }

  virtual bool ReadByte(byte* pDestByte)
  {
    return (Read(reinterpret_cast<void*>(pDestByte), sizeof(byte)) == sizeof(byte));
  }

  virtual bool Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead = nullptr)
  {
    uint32 nBytes = Read(pDestination, ByteCount);
    if (pNumberOfBytesRead != nullptr)
      *pNumberOfBytesRead = nBytes;

    return (nBytes == ByteCount);
  }

  virtual bool WriteByte(byte SourceByte) { return false; }

  virtual uint32 Write(const void* pSource, uint32 ByteCount) { return false; }

  virtual bool Write2(const void* pSource, uint32 ByteCount, uint32* pNumberOfBytesWritten = nullptr) { return false; }

  virtual bool SeekAbsolute(uint64 Offset)
  {
    // any change?
    if (Offset == m_currentDecompressedOffset)
      return true;

    // allow forward seeks
    if (Offset > m_currentDecompressedOffset)
      return SeekRelative(int64(Offset - m_currentDecompressedOffset));

    m_errorState = true;
    return false;
  }

  virtual bool SeekRelative(int64 Offset)
  {
    if (Offset >= 0)
    {
      // read in data until the position is correct
      uint64 remaining = (uint64)Offset;
      byte data[ZIP_STREAM_BUFFER_SIZE];
      while (remaining > 0)
      {
        uint32 readSize = (uint32)Min(remaining, (uint64)ZIP_STREAM_BUFFER_SIZE);
        if (Read(data, readSize) != readSize)
          return false;

        remaining -= (uint64)readSize;
      }

      return true;
    }

    m_errorState = true;
    return false;
  }

  virtual bool SeekToEnd()
  {
    m_errorState = true;
    return false;
  }

  virtual uint64 GetPosition() const { return m_currentDecompressedOffset; }

  virtual uint64 GetSize() const { return m_localFileHeader.DecompressedSize; }

  virtual bool Flush() { return false; }

  virtual bool Discard() { return false; }

  virtual bool Commit() { return false; }

private:
  ZipArchive* m_pZipArchive;
  ByteStream* m_pArchiveStream;
  uint64 m_baseOffset;
  uint64 m_currentFileOffset;
  ZIP_ARCHIVE_LOCAL_FILE_HEADER m_localFileHeader;

  byte m_pInBuffer[ZIP_STREAM_BUFFER_SIZE];
  uint32 m_inBufferBytes;
  uint32 m_inBufferPosition;
  uint64 m_currentDecompressedOffset;
  uint32 m_currentCRC32;

  z_stream m_zStream;
};

class ZipArchiveBufferedReadByteStream : public ByteStream
{
public:
  ZipArchiveBufferedReadByteStream(ZipArchive* pZipArchive)
    : m_pZipArchive(pZipArchive), m_pData(NULL), m_position(0), m_size(0)
  {
    m_pZipArchive->m_nOpenBufferedReads++;
  }

  ~ZipArchiveBufferedReadByteStream() { m_pZipArchive->m_nOpenBufferedReads--; }

  bool FillBuffer(ByteStream* pArchiveStream, uint64 baseOffset, ZIP_ARCHIVE_LOCAL_FILE_HEADER* pLocalFileHeader)
  {
    byte ioBuffer[ZIP_BUFFERED_IO_CHUNK_SIZE];
    bool decompressError = false;
    uint32 currentCRC32 = 0;

    if (m_errorState || pLocalFileHeader->DecompressedSize > 0xFFFFFFFFULL)
      return false;

    m_size = (uint32)pLocalFileHeader->DecompressedSize;
    m_pData = new byte[m_size];

    switch (pLocalFileHeader->CompressionMethod)
    {
      case 0:
      {
        if (pArchiveStream->Read(m_pData, m_size) != m_size)
        {
          decompressError = true;
          m_errorState = true;
        }

        currentCRC32 = crc32(0, (const Bytef*)m_pData, m_size);
      }
      break;

      case Z_DEFLATED:
      {
        z_stream zStream;
        Y_memzero(&zStream, sizeof(zStream));
        if (inflateInit2(&zStream, -MAX_WBITS) != Z_OK)
          Panic("inflateInit2 failed");

        zStream.next_in = NULL;
        zStream.avail_in = 0;
        zStream.next_out = (Bytef*)m_pData;
        zStream.avail_out = m_size;

        uint32 remainingInputBytes = (uint32)pLocalFileHeader->CompressedSize;
        while (zStream.avail_out > 0)
        {
          // in buffer exhausted?
          if (zStream.avail_in == 0)
          {
            // fill it
            uint32 readSize = Min(ZIP_BUFFERED_IO_CHUNK_SIZE, remainingInputBytes);
            if (pArchiveStream->Read(ioBuffer, readSize) != readSize)
            {
              decompressError = true;
              m_errorState = true;
              break;
            }

            // update pointers
            remainingInputBytes -= readSize;
            zStream.next_in = (Bytef*)ioBuffer;
            zStream.avail_in = readSize;
          }

          // save the buffer pointer
          Bytef* pOldPointer = zStream.next_out;

          // decompress the next block
          int err = inflate(&zStream, Z_SYNC_FLUSH);
          if (err != Z_OK && err != Z_STREAM_END)
          {
            // some other error
            decompressError = true;
            break;
          }

          // update crc32
          if (zStream.next_out != pOldPointer)
            currentCRC32 = crc32(currentCRC32, pOldPointer, static_cast<uInt>(zStream.next_out - pOldPointer));

          // handle return
          if (err == Z_OK)
          {
            // done this block, so continue with the next one if required
            continue;
          }
          else if (err == Z_STREAM_END)
          {
            // end of stream, so exit the loop
            break;
          }
        }

        inflateEnd(&zStream);
      }
      break;

      default:
        decompressError = true;
        m_errorState = true;
        break;
    }

    // check crc32
    if (currentCRC32 != pLocalFileHeader->CRC32)
    {
      // corrupt data
      decompressError = true;
      m_errorState = true;
    }

    return !decompressError;
  }

  virtual uint32 Read(void* pDestination, uint32 ByteCount)
  {
    uint32 sz = ByteCount;
    if ((m_position + ByteCount) > m_size)
      sz = m_size - m_position;

    if (sz > 0)
    {
      std::memcpy(pDestination, m_pData + m_position, sz);
      m_position += sz;
    }

    return sz;
  }

  virtual bool ReadByte(byte* pDestByte)
  {
    if (m_position < m_size)
    {
      *pDestByte = m_pData[m_position++];
      return true;
    }

    m_errorState = true;
    return false;
  }

  virtual bool Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead = nullptr)
  {
    uint32 nBytes = Read(pDestination, ByteCount);
    if (pNumberOfBytesRead != nullptr)
      *pNumberOfBytesRead = nBytes;

    if (nBytes != ByteCount)
    {
      m_errorState = true;
      return false;
    }

    return true;
  }

  virtual bool WriteByte(byte SourceByte) { return false; }

  virtual uint32 Write(const void* pSource, uint32 ByteCount) { return false; }

  virtual bool Write2(const void* pSource, uint32 ByteCount, uint32* pNumberOfBytesWritten = NULL) { return false; }

  virtual bool SeekAbsolute(uint64 Offset)
  {
    uint32 Offset32 = (uint32)Offset;
    if (Offset32 > m_size)
    {
      m_errorState = true;
      return false;
    }

    m_position = Offset32;
    return true;
  }

  virtual bool SeekRelative(int64 Offset)
  {
    int32 Offset32 = (int32)Offset;
    if ((Offset32 < 0 && -Offset32 > (int32)m_position) || (uint32)((int32)m_position + Offset32) > m_size)
    {
      m_errorState = true;
      return false;
    }

    m_position += Offset32;
    return true;
  }

  virtual bool SeekToEnd()
  {
    m_position = m_size;
    return true;
  }

  virtual uint64 GetPosition() const { return (uint64)m_position; }

  virtual uint64 GetSize() const { return (uint64)m_size; }

  virtual bool Flush() { return false; }

  virtual bool Discard() { return false; }

  virtual bool Commit() { return false; }

private:
  ZipArchive* m_pZipArchive;

  byte* m_pData;
  uint32 m_position;
  uint32 m_size;
};

class ZipArchiveStreamedWriteByteStream : public ByteStream
{
public:
  ZipArchiveStreamedWriteByteStream(ZipArchive* pZipArchive, ByteStream* pArchiveStream,
                                    ZipArchive::FileEntry* pFileEntry, uint64 baseOffset, uint32 compressionMethod,
                                    uint32 compressionLevel)
    : m_pZipArchive(pZipArchive), m_pArchiveStream(pArchiveStream), m_pFileEntry(pFileEntry), m_baseOffset(baseOffset),
      m_currentFileOffset(baseOffset), m_currentCompressedSize(0), m_currentDecompressedSize(0), m_outBufferBytes(0),
      m_currentCRC32(0), m_compressionMethod(compressionMethod)
  {
    Y_memzero(&m_zStream, sizeof(m_zStream));

    switch (compressionMethod)
    {
      case Z_DEFLATED:
      {
        int err = deflateInit2(&m_zStream, compressionLevel, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
        if (err != Z_OK)
          Panic("deflateInit2 failed");

        m_zStream.avail_out = sizeof(m_pOutBuffer);
        m_zStream.next_out = (Bytef*)m_pOutBuffer;
      }
      break;
    }

    m_pZipArchive->m_nOpenStreamedWrites++;
    m_pFileEntry->WriteOpenCount++;
  }

  ~ZipArchiveStreamedWriteByteStream()
  {
    DebugAssert(m_pFileEntry->WriteOpenCount == 1);

    if (!m_errorState)
      Finalize();

    switch (m_compressionMethod)
    {
      case Z_DEFLATED:
      {
        deflateEnd(&m_zStream);
      }
      break;
    }

    if (m_errorState)
      m_pFileEntry->IsDeleted = true;

    m_pFileEntry->WriteOpenCount--;
    m_pZipArchive->m_nOpenStreamedWrites--;
  }

  bool WriteHeader()
  {
    if (!m_pArchiveStream->SeekAbsolute(m_baseOffset))
    {
      SetErrorState();
      return false;
    }

    BinaryWriter binaryWriter(m_pArchiveStream, ENDIAN_TYPE_LITTLE);
    uint32 filenameLength = Min(m_pFileEntry->FileName.GetLength(), (uint32)0xFFFF);

    if (!binaryWriter.SafeWriteUInt32(~ZIP_ARCHIVE_LOCAL_FILE_HEADER_SIGNTURE) ||   // Signature
        !binaryWriter.SafeWriteUInt16(ZIP_ARCHIVE_VERSION_TO_EXTRACT) ||            // VersionRequiredToExtract
        !binaryWriter.SafeWriteUInt16(0) ||                                         // Flags
        !binaryWriter.SafeWriteUInt16((uint16)m_compressionMethod) ||               // CompressionMethod
        !binaryWriter.SafeWriteUInt32(0) ||                                         // ModificationTimestamp
        !binaryWriter.SafeWriteUInt32(m_currentCRC32) ||                            // CRC32
        !binaryWriter.SafeWriteUInt32(0) ||                                         // CompressedSize
        !binaryWriter.SafeWriteUInt32(0) ||                                         // DecompressedSize
        !binaryWriter.SafeWriteUInt16((uint16)filenameLength) ||                    // FilenameLength
        !binaryWriter.SafeWriteUInt16(0) ||                                         // ExtraFieldLength
        !binaryWriter.SafeWriteFixedString(m_pFileEntry->FileName, filenameLength)) // Filename
    {

      SetErrorState();
      return false;
    }

    m_currentFileOffset = m_pArchiveStream->GetPosition();
    return true;
  }

  bool FlushOutputBuffer()
  {
    if (m_errorState)
      return false;

    if (m_outBufferBytes > 0)
    {
      if (!m_pArchiveStream->SeekAbsolute(m_currentFileOffset) ||
          m_pArchiveStream->Write(m_pOutBuffer, m_outBufferBytes) != m_outBufferBytes)
      {
        SetErrorState();
        return false;
      }

      m_currentCompressedSize += (uint64)m_outBufferBytes;
      m_currentFileOffset += (uint64)m_outBufferBytes;
      m_outBufferBytes = 0;

      m_zStream.avail_out = sizeof(m_pOutBuffer);
      m_zStream.next_out = (Bytef*)m_pOutBuffer;
    }

    return true;
  }

  bool Finalize()
  {
    switch (m_compressionMethod)
    {
      case Z_DEFLATED:
      {
        DebugAssert(m_zStream.avail_in == 0);

        for (;;)
        {
          int err = deflate(&m_zStream, Z_FINISH);
          m_outBufferBytes = sizeof(m_pOutBuffer) - m_zStream.avail_out;

          if (err == Z_OK)
          {
            // possibly the output buffer is full
            if (!FlushOutputBuffer())
              return false;

            // retry
            continue;
          }
          else if (err == Z_STREAM_END)
          {
            // completed
            break;
          }
          else
          {
            // some other error
            SetErrorState();
            return false;
          }
        }
      }
      break;
    }

    // flush all buffers
    if (!FlushOutputBuffer())
      return false;

    // seek to the header
    if (!m_pArchiveStream->SeekAbsolute(m_baseOffset))
    {
      SetErrorState();
      return false;
    }

    // get the timestamp
    Timestamp modifiedTimestamp(Timestamp::Now());

    // rewrite the local header
    BinaryWriter binaryWriter(m_pArchiveStream, ENDIAN_TYPE_LITTLE);
    if (!binaryWriter.SafeWriteUInt32(ZIP_ARCHIVE_LOCAL_FILE_HEADER_SIGNTURE) || // Signature
        !binaryWriter.SafeWriteUInt16(ZIP_ARCHIVE_VERSION_TO_EXTRACT) ||         // VersionRequiredToExtract
        !binaryWriter.SafeWriteUInt16(0) ||                                      // Flags
        !binaryWriter.SafeWriteUInt16((uint16)m_compressionMethod) ||            // CompressionMethod
        !binaryWriter.SafeWriteUInt32(TimestampToZipTime(Timestamp::Now())) ||   // ModificationTimestamp
        !binaryWriter.SafeWriteUInt32(m_currentCRC32) ||                         // CRC32
        !binaryWriter.SafeWriteUInt32((uint32)m_currentCompressedSize) ||        // CompressedSize
        !binaryWriter.SafeWriteUInt32((uint32)m_currentDecompressedSize))        // DecompressedSize
    {
      SetErrorState();
      return false;
    }

    // update the file entry
    m_pFileEntry->CompressionMethod = m_compressionMethod;
    m_pFileEntry->ModifiedTime = modifiedTimestamp;
    m_pFileEntry->CRC32 = m_currentCRC32;
    m_pFileEntry->OffsetToFileHeader = m_baseOffset;
    m_pFileEntry->CompressedFileSize = m_currentCompressedSize;
    m_pFileEntry->DecompressedFileSize = m_currentDecompressedSize;
    m_pFileEntry->IsDeleted = false;
    return true;
  }

  virtual uint32 Read(void* pDestination, uint32 ByteCount) { return 0; }

  virtual bool ReadByte(byte* pDestByte) { return false; }

  virtual bool Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead = NULL) { return false; }

  virtual uint32 Write(const void* pSource, uint32 ByteCount)
  {
    switch (m_compressionMethod)
    {
      case 0:
      {
        const byte* pCurrentPtr = reinterpret_cast<const byte*>(pSource);
        uint32 remaining = 0;

        while (remaining > 0)
        {
          if (m_outBufferBytes == sizeof(m_pOutBuffer) && !FlushOutputBuffer())
            break;

          uint32 copyLength = Min(remaining, uint32(sizeof(m_pOutBuffer) - m_outBufferBytes));
          std::memcpy(m_pOutBuffer + m_outBufferBytes, pCurrentPtr, copyLength);
          m_currentCRC32 = crc32(m_currentCRC32, pCurrentPtr, copyLength);
          pCurrentPtr += copyLength;
          remaining -= copyLength;
        }

        uint32 bytesWritten = (ByteCount - remaining);
        m_currentDecompressedSize += (uint64)bytesWritten;
        return (ByteCount - remaining);
      }
      break;

      case Z_DEFLATED:
      {
        // set up pointers
        m_zStream.avail_in = ByteCount;
        m_zStream.next_in = (Bytef*)pSource;

        // loop
        while (m_zStream.avail_in > 0)
        {
          if (m_zStream.avail_out == 0)
          {
            if (!FlushOutputBuffer())
            {
              m_errorState = true;
              break;
            }

            m_zStream.avail_out = sizeof(m_pOutBuffer);
            m_zStream.next_out = (Bytef*)m_pOutBuffer;
          }

          const Bytef* pOldPointer = m_zStream.next_in;
          int err = deflate(&m_zStream, Z_NO_FLUSH);
          m_outBufferBytes = sizeof(m_pOutBuffer) - m_zStream.avail_out;

          // update crc
          if (m_zStream.next_in != pOldPointer)
            m_currentCRC32 = crc32(m_currentCRC32, pOldPointer, static_cast<uInt>(m_zStream.next_in - pOldPointer));

          // test return
          if (err == Z_OK)
          {
            // more data to be processed
            continue;
          }
          else
          {
            Log_ErrorPrintf("ZipArchiveStreamedWriteByteStream::Write: zlib returned error %d", err);

            // some error occured
            SetErrorState();
            break;
          }
        }

        uint32 bytesWritten = ByteCount - (m_zStream.avail_in);
        m_zStream.avail_in = 0;
        m_zStream.next_in = NULL;
        m_currentDecompressedSize += (uint64)bytesWritten;
        return bytesWritten;
      }
      break;
    }

    return 0;
  }

  virtual bool WriteByte(byte SourceByte) { return (Write(&SourceByte, sizeof(byte)) == sizeof(byte)); }

  virtual bool Write2(const void* pSource, uint32 ByteCount, uint32* pNumberOfBytesWritten = NULL)
  {
    uint32 bytesWritten = Write(pSource, ByteCount);
    if (pNumberOfBytesWritten != NULL)
      *pNumberOfBytesWritten = bytesWritten;

    return (bytesWritten == ByteCount);
  }

  virtual bool SeekAbsolute(uint64 Offset)
  {
    // allow seeks to current position
    if (Offset == m_currentDecompressedSize)
      return true;

    m_errorState = true;
    return false;
  }

  virtual bool SeekRelative(int64 Offset)
  {
    if (Offset == 0)
      return true;

    m_errorState = true;
    return false;
  }

  virtual bool SeekToEnd() { return true; }

  virtual uint64 GetPosition() const { return m_currentDecompressedSize; }

  virtual uint64 GetSize() const { return m_currentDecompressedSize; }

  virtual bool Flush() { return FlushOutputBuffer(); }

  virtual bool Discard() { return false; }

  virtual bool Commit() { return true; }

private:
  ZipArchive* m_pZipArchive;
  ByteStream* m_pArchiveStream;
  ZipArchive::FileEntry* m_pFileEntry;
  uint64 m_baseOffset;
  uint64 m_currentFileOffset;
  uint64 m_currentCompressedSize;
  uint64 m_currentDecompressedSize;

  byte m_pOutBuffer[ZIP_STREAM_BUFFER_SIZE];
  uint32 m_outBufferBytes;
  uint32 m_currentCRC32;
  uint32 m_compressionMethod;

  z_stream m_zStream;
};

class ZipArchiveBufferedWriteByteStream : public ByteStream
{
public:
  ZipArchiveBufferedWriteByteStream(ZipArchive* pZipArchive, ByteStream* pWriteStream,
                                    ZipArchive::FileEntry* pFileEntry, uint32 compressionMethod,
                                    uint32 compressionLevel)
  {
    pFileEntry->WriteOpenCount++;

    m_pArchive = pZipArchive;
    m_pWriteStream = pWriteStream;
    m_pFileEntry = pFileEntry;
    m_compressionMethod = compressionMethod;
    m_compressionLevel = compressionLevel;
    m_pMemory = NULL;
    m_position = 0;
    m_size = 0;
    m_memorySize = 0;
  }

  ~ZipArchiveBufferedWriteByteStream()
  {
    DebugAssert(m_pFileEntry->WriteOpenCount == 1);

    if (!Finalize())
    {
      // maybe fix this in the future by setting an archive error flag
      // Panic("Zip buffered write finalize failed");

      // flag file as deleted
      m_pFileEntry->IsDeleted = true;
    }

    if (m_pMemory != NULL)
      std::free(m_pMemory);

    m_pFileEntry->WriteOpenCount--;
  }

  bool Finalize()
  {
    Assert(m_pArchive->m_nOpenStreamedWrites == 0);

    // setup zlib
    z_stream zStream;
    if (m_compressionMethod == Z_DEFLATED)
    {
      Y_memzero(&zStream, sizeof(zStream));
      int err = deflateInit2(&zStream, m_compressionLevel, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
      if (err != Z_OK)
        return false;
    }

    // seek to the header
    if (!m_pWriteStream->SeekToEnd())
    {
      SetErrorState();
      return false;
    }

    // get the timestamp
    BinaryWriter binaryWriter(m_pWriteStream, ENDIAN_TYPE_LITTLE);
    Timestamp modifiedTimestamp(Timestamp::Now());
    uint64 offsetToLocalFileHeader = m_pWriteStream->GetPosition();
    uint32 filenameLength = Min(m_pFileEntry->FileName.GetLength(), (uint32)0xFFFF);
    if (!binaryWriter.SafeWriteUInt32(~ZIP_ARCHIVE_LOCAL_FILE_HEADER_SIGNTURE) ||   // Signature
        !binaryWriter.SafeWriteUInt16(ZIP_ARCHIVE_VERSION_TO_EXTRACT) ||            // VersionRequiredToExtract
        !binaryWriter.SafeWriteUInt16(0) ||                                         // Flags
        !binaryWriter.SafeWriteUInt16((uint16)m_compressionMethod) ||               // CompressionMethod
        !binaryWriter.SafeWriteUInt32(0) ||                                         // ModificationTimestamp
        !binaryWriter.SafeWriteUInt32(0) ||                                         // CRC32
        !binaryWriter.SafeWriteUInt32(0) ||                                         // CompressedSize
        !binaryWriter.SafeWriteUInt32(0) ||                                         // DecompressedSize
        !binaryWriter.SafeWriteUInt16((uint16)filenameLength) ||                    // FilenameLength
        !binaryWriter.SafeWriteUInt16(0) ||                                         // ExtraFieldLength
        !binaryWriter.SafeWriteFixedString(m_pFileEntry->FileName, filenameLength)) // Filename
    {
      SetErrorState();
      return false;
    }

    uint32 crc = 0;
    uint32 compressedFileSize = 0;
    uint32 decompressedFileSize = 0;
    if (m_size > 0)
    {
      // calculate crc32
      crc = crc32(0, (const Bytef*)m_pMemory, m_size);

      // compress?
      if (m_compressionMethod == Z_DEFLATED)
      {
        byte compressionBuffer[ZIP_BUFFERED_IO_CHUNK_SIZE];
        zStream.avail_in = m_size;
        zStream.next_in = (Bytef*)m_pMemory;
        zStream.avail_out = sizeof(compressionBuffer);
        zStream.next_out = (Bytef*)compressionBuffer;

        while (zStream.avail_in > 0)
        {
          if (zStream.avail_out == 0)
          {
            binaryWriter.WriteBytes(compressionBuffer, sizeof(compressionBuffer));
            zStream.avail_out = sizeof(compressionBuffer);
            zStream.next_out = (Bytef*)compressionBuffer;
          }

          int err = deflate(&zStream, Z_NO_FLUSH);
          if (err == Z_OK)
          {
            // ok, more data to go
            continue;
          }
          else
          {
            // error of some sort
            deflateEnd(&zStream);
            return false;
          }
        }

        // complete compression
        for (;;)
        {
          if (zStream.avail_out == 0)
          {
            binaryWriter.WriteBytes(compressionBuffer, sizeof(compressionBuffer));
            zStream.avail_out = sizeof(compressionBuffer);
            zStream.next_out = (Bytef*)compressionBuffer;
          }

          int err = deflate(&zStream, Z_FINISH);
          if (err == Z_STREAM_END)
          {
            // complete
            break;
          }
          else if (err == Z_OK)
          {
            // more data to go
            continue;
          }
          else
          {
            // error of some sort
            deflateEnd(&zStream);
            return false;
          }
        }

        // write any remaining bytes
        if (zStream.avail_out != sizeof(compressionBuffer))
          binaryWriter.WriteBytes(compressionBuffer, sizeof(compressionBuffer) - zStream.avail_out);

        // fill fields
        compressedFileSize = zStream.total_out;
        decompressedFileSize = zStream.total_in;
      }
      else
      {
        // write data out as-is
        binaryWriter.WriteBytes(m_pMemory, m_size);

        // fill fields
        compressedFileSize = m_size;
        decompressedFileSize = m_size;
      }
    }

    // seek back to local header position
    if (!m_pWriteStream->SeekAbsolute(offsetToLocalFileHeader))
    {
      SetErrorState();
      return false;
    }

    // write the local header
    if (!binaryWriter.SafeWriteUInt32(ZIP_ARCHIVE_LOCAL_FILE_HEADER_SIGNTURE) || // Signature
        !binaryWriter.SafeWriteUInt16(ZIP_ARCHIVE_VERSION_TO_EXTRACT) ||         // VersionRequiredToExtract
        !binaryWriter.SafeWriteUInt16(0) ||                                      // Flags
        !binaryWriter.SafeWriteUInt16((uint16)m_compressionMethod) ||            // CompressionMethod
        !binaryWriter.SafeWriteUInt32(TimestampToZipTime(Timestamp::Now())) ||   // ModificationTimestamp
        !binaryWriter.SafeWriteUInt32(crc) ||                                    // CRC32
        !binaryWriter.SafeWriteUInt32(compressedFileSize) ||                     // CompressedSize
        !binaryWriter.SafeWriteUInt32(decompressedFileSize))                     // DecompressedSize
    {
      SetErrorState();
      return false;
    }

    // update the file entry
    m_pFileEntry->CompressionMethod = m_compressionMethod;
    m_pFileEntry->ModifiedTime = modifiedTimestamp;
    m_pFileEntry->CRC32 = crc;
    m_pFileEntry->OffsetToFileHeader = offsetToLocalFileHeader;
    m_pFileEntry->CompressedFileSize = compressedFileSize;
    m_pFileEntry->DecompressedFileSize = decompressedFileSize;
    m_pFileEntry->IsDeleted = false;
    return true;
  }

  bool ReadByte(byte* pDestByte)
  {
    if (m_position < m_size)
    {
      *pDestByte = m_pMemory[m_position++];
      return true;
    }

    return false;
  }

  uint32 Read(void* pDestination, uint32 ByteCount)
  {
    uint32 sz = ByteCount;
    if ((m_position + ByteCount) > m_size)
      sz = m_size - m_position;

    if (sz > 0)
    {
      std::memcpy(pDestination, m_pMemory + m_position, sz);
      m_position += sz;
    }

    return sz;
  }

  bool Read2(void* pDestination, uint32 ByteCount, uint32* pNumberOfBytesRead /* = NULL */)
  {
    uint32 r = Read(pDestination, ByteCount);
    if (pNumberOfBytesRead != NULL)
      *pNumberOfBytesRead = r;

    return (r == ByteCount);
  }

  bool WriteByte(byte SourceByte)
  {
    if (m_position == m_memorySize)
      Grow(1);

    m_pMemory[m_position++] = SourceByte;
    m_size = Max(m_size, m_position);
    return true;
  }

  uint32 Write(const void* pSource, uint32 ByteCount)
  {
    if ((m_position + ByteCount) > m_memorySize)
      Grow(ByteCount);

    std::memcpy(m_pMemory + m_position, pSource, ByteCount);
    m_position += ByteCount;
    m_size = Max(m_size, m_position);
    return ByteCount;
  }

  bool Write2(const void* pSource, uint32 ByteCount, uint32* pNumberOfBytesWritten /* = NULL */)
  {
    uint32 r = Write(pSource, ByteCount);
    if (pNumberOfBytesWritten != NULL)
      *pNumberOfBytesWritten = r;

    return (r == ByteCount);
  }

  bool SeekAbsolute(uint64 Offset)
  {
    uint32 Offset32 = (uint32)Offset;
    if (Offset32 > m_size)
      return false;

    m_position = Offset32;
    return true;
  }

  bool SeekRelative(int64 Offset)
  {
    int32 Offset32 = (int32)Offset;
    if ((Offset32 < 0 && -Offset32 > (int32)m_position) || (uint32)((int32)m_position + Offset32) > m_size)
      return false;

    m_position += Offset32;
    return true;
  }

  bool SeekToEnd()
  {
    m_position = m_size;
    return true;
  }

  uint64 GetSize() const { return (uint64)m_size; }

  uint64 GetPosition() const { return (uint64)m_position; }

  bool Flush() { return true; }

  bool Commit() { return true; }

  bool Discard() { return false; }

private:
  void Grow(uint32 minimumGrowth)
  {
    uint32 newSize = Max(m_memorySize + minimumGrowth, m_memorySize * 2);
    if (m_pMemory == NULL)
    {
      m_pMemory = (byte*)std::malloc(newSize);
      std::memcpy(m_pMemory, m_pMemory, m_size);
      m_memorySize = newSize;
    }
    else
    {
      m_pMemory = (byte*)std::realloc(m_pMemory, newSize);
      m_memorySize = newSize;
    }
  }

  ZipArchive* m_pArchive;
  ZipArchive::FileEntry* m_pFileEntry;
  ByteStream* m_pWriteStream;
  uint32 m_compressionMethod;
  uint32 m_compressionLevel;
  byte* m_pMemory;
  uint32 m_position;
  uint32 m_size;
  uint32 m_memorySize;
};

ZipArchive::ZipArchive(ByteStream* pReadStream, ByteStream* pWriteStream)
  : m_pReadStream(pReadStream), m_pWriteStream(pWriteStream), m_nOpenStreamedReads(0), m_nOpenStreamedWrites(0),
    m_nOpenBufferedReads(0), m_nOpenBufferedWrites(0)
{
  if (pReadStream != NULL)
    pReadStream->AddRef();
  if (pWriteStream != NULL)
    pWriteStream->AddRef();
}

ZipArchive::~ZipArchive()
{
  if (m_pReadStream != NULL)
    m_pReadStream->Release();
  if (m_pWriteStream != NULL)
    m_pWriteStream->Release();
}

bool ZipArchive::StatFile(const char* filename, FILESYSTEM_STAT_DATA* pStatData) const
{
  const FileHashTable::Member* pMember;
  if (((pMember = m_writeFileHashTable.Find(filename)) != NULL && !pMember->Value->IsDeleted) ||
      ((pMember = m_readFileHashTable.Find(filename)) != NULL && !pMember->Value->IsDeleted))
  {
    const FileEntry* pFileEntry = pMember->Value;

    // attributes
    pStatData->Attributes = 0;
    {
      // compressed
      if (pFileEntry->CompressionMethod != 0)
        pStatData->Attributes |= FILESYSTEM_FILE_ATTRIBUTE_COMPRESSED;
    }

    // modification time
    pStatData->ModificationTime = pFileEntry->ModifiedTime;

    // size (uncompressed)
    pStatData->Size = pFileEntry->DecompressedFileSize;

    // ok
    return true;
  }

  return false;
}

ByteStream* ZipArchive::OpenFile(const char* filename, uint32 openMode, uint32 compressionLevel /* = 6 */)
{
  // zip archives do not support reading and writing at the same time
  if ((openMode & (BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_WRITE)) == (BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_WRITE))
    return NULL;

  // reading?
  if (openMode & BYTESTREAM_OPEN_READ)
  {
    const FileHashTable::Member* pMember;
    ByteStream* pStreamToReadFrom = NULL;
    if ((pMember = m_writeFileHashTable.Find(filename)) != NULL && !pMember->Value->IsDeleted)
    {
      // file already open for reading?
      if (pMember->Value->WriteOpenCount > 0)
        return NULL;

      pStreamToReadFrom = m_pWriteStream;
    }
    else if ((pMember = m_readFileHashTable.Find(filename)) != NULL && !pMember->Value->IsDeleted)
    {
      pStreamToReadFrom = m_pReadStream;
    }
    else
      return NULL;

    // read the local file header
    const FileEntry* pFileEntry = pMember->Value;
    BinaryReader binaryReader(pStreamToReadFrom, ENDIAN_TYPE_LITTLE);
    if (!pStreamToReadFrom->SeekAbsolute(pFileEntry->OffsetToFileHeader))
      return NULL;

    ZIP_ARCHIVE_LOCAL_FILE_HEADER localFileHeader;
    uint32 compressedSize32;
    uint32 decompressedSize32;
    if (!binaryReader.SafeReadUInt32(&localFileHeader.Signature) ||
        !binaryReader.SafeReadUInt16(&localFileHeader.VersionRequiredToExtract) ||
        !binaryReader.SafeReadUInt16(&localFileHeader.Flags) ||
        !binaryReader.SafeReadUInt16(&localFileHeader.CompressionMethod) ||
        !binaryReader.SafeReadUInt32(&localFileHeader.ModificationTimestamp) ||
        !binaryReader.SafeReadUInt32(&localFileHeader.CRC32) || !binaryReader.SafeReadUInt32(&compressedSize32) ||
        !binaryReader.SafeReadUInt32(&decompressedSize32) ||
        !binaryReader.SafeReadUInt16(&localFileHeader.FilenameLength) ||
        !binaryReader.SafeReadUInt16(&localFileHeader.ExtraFieldLength))
    {
      return NULL;
    }

    localFileHeader.CompressedSize = (uint64)compressedSize32;
    localFileHeader.DecompressedSize = (uint64)decompressedSize32;

    if (localFileHeader.Signature != ZIP_ARCHIVE_LOCAL_FILE_HEADER_SIGNTURE)
      return NULL;

    // only handling deflate at the moment
    if (localFileHeader.CompressionMethod != 0 && localFileHeader.CompressionMethod != Z_DEFLATED)
      return NULL;

    // ugh, streamed files. have to handle these carefully
    // otherwise, should be accurate
    if (localFileHeader.Flags & 8)
    {
      if (pFileEntry->CRC32 == 0)
        return NULL;

      // pull from central directory
      localFileHeader.CRC32 = pFileEntry->CRC32;
      localFileHeader.CompressedSize = pFileEntry->CompressedFileSize;
      localFileHeader.DecompressedSize = pFileEntry->DecompressedFileSize;
    }
    else if (localFileHeader.CompressedSize != pFileEntry->CompressedFileSize ||
             localFileHeader.DecompressedSize != pFileEntry->DecompressedFileSize)
    {
      return NULL;
    }

    uint32 seekDistance = (uint32)localFileHeader.FilenameLength + (uint32)localFileHeader.ExtraFieldLength;
    if (!pStreamToReadFrom->SeekRelative(seekDistance))
      return NULL;

    if (openMode & BYTESTREAM_OPEN_STREAMED || (openMode & BYTESTREAM_OPEN_SEEKABLE) == 0)
    {
      // create stream
      ZipArchiveStreamedReadByteStream* pReaderStream = new ZipArchiveStreamedReadByteStream(
        this, pStreamToReadFrom, pStreamToReadFrom->GetPosition(), &localFileHeader);
      return pReaderStream;
    }
    else
    {
      // create buffered stream
      ZipArchiveBufferedReadByteStream* pReaderStream = new ZipArchiveBufferedReadByteStream(this);
      if (!pReaderStream->FillBuffer(pStreamToReadFrom, pStreamToReadFrom->GetPosition(), &localFileHeader))
      {
        pReaderStream->Release();
        pReaderStream = NULL;
      }
      return pReaderStream;
    }
  }
  else if (openMode & BYTESTREAM_OPEN_WRITE)
  {
    if (m_pWriteStream == NULL)
      return NULL;

    bool streamed = (openMode & BYTESTREAM_OPEN_STREAMED || (openMode & BYTESTREAM_OPEN_SEEKABLE) == 0);
    if (streamed)
    {
      // can only have one streamed write open at once
      if (m_nOpenBufferedWrites > 0 || m_nOpenStreamedWrites > 0)
        return NULL;

      // seek to the end of the write stream
      if (!m_pWriteStream->SeekToEnd())
        return NULL;
    }
    else
    {
      // disallow buffered write when a streamed write is open
      if (m_nOpenStreamedWrites > 0)
        return NULL;
    }

    // determine compression method
    uint32 compressionMethod = (compressionLevel == 0) ? 0 : Z_DEFLATED;

    // see if the file is already present
    FileHashTable::Member* pMember = m_writeFileHashTable.Find(filename);
    if (pMember == NULL)
    {
      // create a new file entry
      FileEntry* pFileEntry = new FileEntry;
      pFileEntry->FileName = filename;
      pFileEntry->CompressionMethod = compressionMethod;
      pFileEntry->ModifiedTime = Timestamp::Now();
      pFileEntry->CRC32 = 0;
      pFileEntry->OffsetToFileHeader = 0;
      pFileEntry->CompressedFileSize = 0;
      pFileEntry->DecompressedFileSize = 0;
      pFileEntry->IsDeleted = false;
      pFileEntry->WriteOpenCount = 0;
      pMember = m_writeFileHashTable.Insert(pFileEntry->FileName, pFileEntry);
    }

    // create the stream object
    if (streamed)
    {
      ZipArchiveStreamedWriteByteStream* pWriterStream = new ZipArchiveStreamedWriteByteStream(
        this, m_pWriteStream, pMember->Value, m_pWriteStream->GetPosition(), compressionMethod, compressionLevel);
      if (!pWriterStream->WriteHeader())
      {
        pWriterStream->Release();
        pMember->Value->IsDeleted = true;
        return NULL;
      }

      return pWriterStream;
    }
    else
    {
      ZipArchiveBufferedWriteByteStream* pWriterStream = new ZipArchiveBufferedWriteByteStream(
        this, m_pWriteStream, pMember->Value, compressionMethod, compressionLevel);
      return pWriterStream;
    }
  }

  return NULL;
}

bool ZipArchive::DeleteFile(const char* filename)
{
  FileHashTable::Member* pMember;
  bool result = false;

  // remove from both read and write streams
  pMember = m_readFileHashTable.Find(filename);
  if (pMember != NULL)
  {
    pMember->Value->IsDeleted = true;
    result = true;
  }

  pMember = m_writeFileHashTable.Find(filename);
  if (pMember != NULL)
  {
    pMember->Value->IsDeleted = true;
    result = true;
  }

  return result;
}

bool ZipArchive::CopyFile(ZipArchive* pOtherArchive, const char* filename)
{
  if (m_pWriteStream == NULL)
    return false;

  const FileHashTable::Member* pSourceMember;
  ByteStream* pSourceStream;

  pSourceMember = pOtherArchive->m_writeFileHashTable.Find(filename);
  if (pSourceMember == NULL)
  {
    if ((pSourceMember = pOtherArchive->m_readFileHashTable.Find(filename)) != NULL)
    {
      pSourceStream = pOtherArchive->m_pReadStream;
    }
    else
    {
      return false;
    }
  }
  else
  {
    pSourceStream = pOtherArchive->m_pWriteStream;
  }

  const FileEntry* pSourceEntry = pSourceMember->Value;

  // seek read stream to the file header offset
  if (!pSourceStream->SeekAbsolute(pSourceEntry->OffsetToFileHeader))
    return false;

  // find offset that we are writing to
  if (!m_pWriteStream->SeekToEnd())
    return false;

  // create in file table
  FileHashTable::Member* pDestinationMember = m_writeFileHashTable.Find(pSourceEntry->FileName);
  if (pDestinationMember != NULL)
  {
    if (pDestinationMember->Value->WriteOpenCount > 0)
      return false;
  }
  else
  {
    FileEntry* pFileEntry = new FileEntry;
    pDestinationMember = m_writeFileHashTable.Insert(pSourceEntry->FileName, pFileEntry);
  }

  // fill file entry
  FileEntry* pDestinationEntry = pDestinationMember->Value;
  pDestinationEntry->FileName = pSourceEntry->FileName;
  pDestinationEntry->CompressionMethod = pSourceEntry->CompressionMethod;
  pDestinationEntry->ModifiedTime = pSourceEntry->ModifiedTime;
  pDestinationEntry->CRC32 = pSourceEntry->CRC32;
  pDestinationEntry->OffsetToFileHeader = m_pWriteStream->GetPosition();
  pDestinationEntry->CompressedFileSize = pSourceEntry->CompressedFileSize;
  pDestinationEntry->DecompressedFileSize = pSourceEntry->DecompressedFileSize;
  pDestinationEntry->IsDeleted = false;
  pDestinationEntry->WriteOpenCount = 0;

  // write local header
  {
    BinaryReader binaryReader(pSourceStream);

    ZIP_ARCHIVE_LOCAL_FILE_HEADER localFileHeader;
    uint32 compressedSize32;
    uint32 decompressedSize32;
    if (!binaryReader.SafeReadUInt32(&localFileHeader.Signature) ||
        !binaryReader.SafeReadUInt16(&localFileHeader.VersionRequiredToExtract) ||
        !binaryReader.SafeReadUInt16(&localFileHeader.Flags) ||
        !binaryReader.SafeReadUInt16(&localFileHeader.CompressionMethod) ||
        !binaryReader.SafeReadUInt32(&localFileHeader.ModificationTimestamp) ||
        !binaryReader.SafeReadUInt32(&localFileHeader.CRC32) || !binaryReader.SafeReadUInt32(&compressedSize32) ||
        !binaryReader.SafeReadUInt32(&decompressedSize32) ||
        !binaryReader.SafeReadUInt16(&localFileHeader.FilenameLength) ||
        !binaryReader.SafeReadUInt16(&localFileHeader.ExtraFieldLength))
    {
      pDestinationEntry->IsDeleted = true;
      return false;
    }

    localFileHeader.CompressedSize = (uint64)compressedSize32;
    localFileHeader.DecompressedSize = (uint64)decompressedSize32;

    if (localFileHeader.Signature != ZIP_ARCHIVE_LOCAL_FILE_HEADER_SIGNTURE)
    {
      pDestinationEntry->IsDeleted = true;
      return false;
    }

    // only handling deflate at the moment
    if (localFileHeader.CompressionMethod != 0 && localFileHeader.CompressionMethod != Z_DEFLATED)
    {
      pDestinationEntry->IsDeleted = true;
      return false;
    }

    // ugh, streamed files. have to handle these carefully
    // otherwise, should be accurate
    if (localFileHeader.Flags & 8)
    {
      if (pSourceEntry->CRC32 == 0)
      {
        pDestinationEntry->IsDeleted = true;
        return false;
      }

      // pull from central directory, and strip the streamed flag from it, since we're not going to append the
      // post-header
      localFileHeader.CRC32 = pSourceEntry->CRC32;
      localFileHeader.CompressedSize = pSourceEntry->CompressedFileSize;
      localFileHeader.DecompressedSize = pSourceEntry->DecompressedFileSize;
      localFileHeader.Flags &= ~(uint16)8;
    }
    else if (localFileHeader.CompressedSize != pSourceEntry->CompressedFileSize ||
             localFileHeader.DecompressedSize != pSourceEntry->DecompressedFileSize)
    {
      pDestinationEntry->IsDeleted = true;
      return false;
    }

    uint32 seekDistance = (uint32)localFileHeader.FilenameLength + (uint32)localFileHeader.ExtraFieldLength;
    if (!pSourceStream->SeekRelative(seekDistance))
    {
      pDestinationEntry->IsDeleted = true;
      return false;
    }

    // write the local file header to the write stream
    BinaryWriter binaryWriter(m_pWriteStream, ENDIAN_TYPE_LITTLE);
    uint32 filenameLength = Min(pSourceEntry->FileName.GetLength(), (uint32)0xFFFF);
    if (!binaryWriter.SafeWriteUInt32(localFileHeader.Signature) ||                 // Signature
        !binaryWriter.SafeWriteUInt16(localFileHeader.VersionRequiredToExtract) ||  // VersionRequiredToExtract
        !binaryWriter.SafeWriteUInt16(localFileHeader.Flags) ||                     // Flags
        !binaryWriter.SafeWriteUInt16(localFileHeader.CompressionMethod) ||         // CompressionMethod
        !binaryWriter.SafeWriteUInt32(localFileHeader.ModificationTimestamp) ||     // ModificationTimestamp
        !binaryWriter.SafeWriteUInt32(localFileHeader.CRC32) ||                     // CRC32
        !binaryWriter.SafeWriteUInt32((uint32)localFileHeader.CompressedSize) ||    // CompressedSize
        !binaryWriter.SafeWriteUInt32((uint32)localFileHeader.DecompressedSize) ||  // DecompressedSize
        !binaryWriter.SafeWriteUInt16((uint16)filenameLength) ||                    // FilenameLength
        !binaryWriter.SafeWriteUInt16(0) ||                                         // ExtraFieldLength
        !binaryWriter.SafeWriteFixedString(pSourceEntry->FileName, filenameLength)) // Filename
    {
      pDestinationEntry->IsDeleted = true;
      return false;
    }
  }

  // read+write chunks
  static const uint32 CHUNKSIZE = 4096;
  byte chunkBuffer[CHUNKSIZE];
  uint64 remaining = pSourceEntry->CompressedFileSize;
  while (remaining > 0)
  {
    uint32 copySize = (uint32)Min(remaining, (uint64)CHUNKSIZE);

    // read it
    uint32 bytesRead = pSourceStream->Read(chunkBuffer, copySize);
    if (bytesRead != copySize)
    {
      // io error
      return false;
    }

    // write it
    uint32 bytesWritten = m_pWriteStream->Write(chunkBuffer, copySize);
    if (bytesWritten != copySize)
    {
      // io error
      return false;
    }

    remaining -= (uint64)copySize;
  }

  // all good
  return true;
}

bool ZipArchive::UpgradeToWritableArchive(ByteStream* pWriteStream)
{
  if (m_pWriteStream != NULL)
    return false;

  m_pWriteStream = pWriteStream;
  m_pWriteStream->AddRef();
  return true;
}

bool ZipArchive::ParseZip()
{
  BinaryReader binaryReader(m_pReadStream, ENDIAN_TYPE_LITTLE);

  // start at the end of the file - 4
  if (!m_pReadStream->SeekToEnd() || !m_pReadStream->SeekRelative(-4))
    return false;

  // find the end of central directory record
  uint32 signature;
  for (;;)
  {
    signature = binaryReader.ReadUInt32();
    if (signature == ZIP_ARCHIVE_END_OF_CENTRAL_DIRECTORY_SIGNATURE)
      break;

    if (!m_pReadStream->SeekRelative(-5))
      return false;
  }

  // got it, so pull out the fields
  uint16 diskNumber;
  uint16 centralDirectoryDisk;
  uint16 centralDirectoryRecordsOnThisDisk;
  uint16 centralDirectoryTotalRecords;
  uint32 centralDirectorySize;
  uint32 centralDirectoryOffset;
  uint16 commentLength;

  if (!binaryReader.SafeReadUInt16(&diskNumber) || !binaryReader.SafeReadUInt16(&centralDirectoryDisk) ||
      !binaryReader.SafeReadUInt16(&centralDirectoryRecordsOnThisDisk) ||
      !binaryReader.SafeReadUInt16(&centralDirectoryTotalRecords) ||
      !binaryReader.SafeReadUInt32(&centralDirectorySize) || !binaryReader.SafeReadUInt32(&centralDirectoryOffset) ||
      !binaryReader.SafeReadUInt16(&commentLength))
  {
    return false;
  }

  // multi-disk archives are not supported
  if (centralDirectoryDisk != 0 || centralDirectoryRecordsOnThisDisk != centralDirectoryTotalRecords)
  {
    Log_ErrorPrintf("ZipArchive::ParseZip: Multi-disk archives are unsupported.");
    return false;
  }

  // go to the central directory
  if (!m_pReadStream->SeekAbsolute(centralDirectoryOffset))
    return false;

  // parse the central directory
  for (uint16 i = 0; i < centralDirectoryTotalRecords; i++)
  {
    uint32 fileSignature;
    uint16 versionMadeBy;
    uint16 versionExtract;
    uint16 bitFlags;
    uint16 compressionMethod;
    uint32 modificationTimestamp;
    uint32 crc;
    uint32 compressedSize;
    uint32 uncompressedSize;
    uint16 filenameLength;
    uint16 extraFieldLength;
    uint16 fileCommentLength;
    uint16 diskNumberCD;
    uint16 internalFileAttributes;
    uint32 externalFileAttributes;
    uint32 fileHeaderOffset;
    String fileName;

    if (!binaryReader.SafeReadUInt32(&fileSignature) || !binaryReader.SafeReadUInt16(&versionMadeBy) ||
        !binaryReader.SafeReadUInt16(&versionExtract) || !binaryReader.SafeReadUInt16(&bitFlags) ||
        !binaryReader.SafeReadUInt16(&compressionMethod) || !binaryReader.SafeReadUInt32(&modificationTimestamp) ||
        !binaryReader.SafeReadUInt32(&crc) || !binaryReader.SafeReadUInt32(&compressedSize) ||
        !binaryReader.SafeReadUInt32(&uncompressedSize) || !binaryReader.SafeReadUInt16(&filenameLength) ||
        !binaryReader.SafeReadUInt16(&extraFieldLength) || !binaryReader.SafeReadUInt16(&fileCommentLength) ||
        !binaryReader.SafeReadUInt16(&diskNumberCD) || !binaryReader.SafeReadUInt16(&internalFileAttributes) ||
        !binaryReader.SafeReadUInt32(&externalFileAttributes) || !binaryReader.SafeReadUInt32(&fileHeaderOffset) ||
        !binaryReader.SafeReadFixedString(filenameLength, &fileName))
    {
      return false;
    }

    if (fileSignature != ZIP_ARCHIVE_CENTRAL_DIRECTORY_FILE_HEADER_SIGNATURE)
      return false;

    if (((uint32)extraFieldLength + (uint32)fileCommentLength) > 0 &&
        !m_pReadStream->SeekRelative((uint32)extraFieldLength + (uint32)fileCommentLength))
      return false;

    if (m_readFileHashTable.Find(fileName) != NULL)
    {
      Log_WarningPrintf("ZipArchive::ParseZip: Duplicate file name in zip: '%s'.", fileName.GetCharArray());
      continue;
    }

    FileEntry* pFileEntry = new FileEntry;
    pFileEntry->FileName = fileName;
    pFileEntry->CompressionMethod = compressionMethod;
    pFileEntry->ModifiedTime = ZipTimeToTimestamp(modificationTimestamp);
    pFileEntry->CRC32 = crc;
    pFileEntry->OffsetToFileHeader = (uint64)fileHeaderOffset;
    pFileEntry->CompressedFileSize = (uint64)compressedSize;
    pFileEntry->DecompressedFileSize = (uint64)uncompressedSize;
    pFileEntry->IsDeleted = false;
    pFileEntry->WriteOpenCount = 0;
    m_readFileHashTable.Insert(pFileEntry->FileName, pFileEntry);
  }

  return true;
}

bool ZipArchive::CommitChanges()
{
  DebugAssert(m_pWriteStream != NULL);

  // ensure there are no open writes
  if ((m_nOpenStreamedReads + m_nOpenStreamedWrites + m_nOpenBufferedWrites) > 0)
    return false;

  // start writing at the end
  if (!m_pWriteStream->SeekToEnd())
    return false;

  // any files that are located in the read stream, but not in the write stream, now need to be written to the write
  // stream
  for (FileHashTable::Iterator itr = m_readFileHashTable.Begin(); !itr.AtEnd(); itr.Forward())
  {
    const FileEntry* pFileEntry = itr->Value;
    if (pFileEntry->IsDeleted || m_writeFileHashTable.Find(itr->Key) != NULL)
      continue;

    if (!m_pReadStream->SeekAbsolute(pFileEntry->OffsetToFileHeader))
      return false;

    // store the starting offset
    uint64 newStartingOffset = m_pWriteStream->GetPosition();
    {
      BinaryReader binaryReader(m_pReadStream);

      ZIP_ARCHIVE_LOCAL_FILE_HEADER localFileHeader;
      uint32 compressedSize32;
      uint32 decompressedSize32;
      if (!binaryReader.SafeReadUInt32(&localFileHeader.Signature) ||
          !binaryReader.SafeReadUInt16(&localFileHeader.VersionRequiredToExtract) ||
          !binaryReader.SafeReadUInt16(&localFileHeader.Flags) ||
          !binaryReader.SafeReadUInt16(&localFileHeader.CompressionMethod) ||
          !binaryReader.SafeReadUInt32(&localFileHeader.ModificationTimestamp) ||
          !binaryReader.SafeReadUInt32(&localFileHeader.CRC32) || !binaryReader.SafeReadUInt32(&compressedSize32) ||
          !binaryReader.SafeReadUInt32(&decompressedSize32) ||
          !binaryReader.SafeReadUInt16(&localFileHeader.FilenameLength) ||
          !binaryReader.SafeReadUInt16(&localFileHeader.ExtraFieldLength))
      {
        return false;
      }

      localFileHeader.CompressedSize = (uint64)compressedSize32;
      localFileHeader.DecompressedSize = (uint64)decompressedSize32;

      if (localFileHeader.Signature != ZIP_ARCHIVE_LOCAL_FILE_HEADER_SIGNTURE)
        return false;

      // only handling deflate at the moment
      if (localFileHeader.CompressionMethod != 0 && localFileHeader.CompressionMethod != Z_DEFLATED)
        return false;

      // ugh, streamed files. have to handle these carefully
      // otherwise, should be accurate
      if (localFileHeader.Flags & 8)
      {
        if (pFileEntry->CRC32 == 0)
          return false;

        // pull from central directory, and strip the streamed flag from it, since we're not going to append the
        // post-header
        localFileHeader.CRC32 = pFileEntry->CRC32;
        localFileHeader.CompressedSize = pFileEntry->CompressedFileSize;
        localFileHeader.DecompressedSize = pFileEntry->DecompressedFileSize;
        localFileHeader.Flags &= ~(uint16)8;
      }
      else if (localFileHeader.CompressedSize != pFileEntry->CompressedFileSize ||
               localFileHeader.DecompressedSize != pFileEntry->DecompressedFileSize)
      {
        return false;
      }

      uint32 seekDistance = (uint32)localFileHeader.FilenameLength + (uint32)localFileHeader.ExtraFieldLength;
      if (!m_pReadStream->SeekRelative(seekDistance))
        return false;

      // write the local file header to the write stream
      BinaryWriter binaryWriter(m_pWriteStream, ENDIAN_TYPE_LITTLE);
      uint32 filenameLength = Min(pFileEntry->FileName.GetLength(), (uint32)0xFFFF);
      if (!binaryWriter.SafeWriteUInt32(localFileHeader.Signature) ||                // Signature
          !binaryWriter.SafeWriteUInt16(localFileHeader.VersionRequiredToExtract) || // VersionRequiredToExtract
          !binaryWriter.SafeWriteUInt16(localFileHeader.Flags) ||                    // Flags
          !binaryWriter.SafeWriteUInt16(localFileHeader.CompressionMethod) ||        // CompressionMethod
          !binaryWriter.SafeWriteUInt32(localFileHeader.ModificationTimestamp) ||    // ModificationTimestamp
          !binaryWriter.SafeWriteUInt32(localFileHeader.CRC32) ||                    // CRC32
          !binaryWriter.SafeWriteUInt32((uint32)localFileHeader.CompressedSize) ||   // CompressedSize
          !binaryWriter.SafeWriteUInt32((uint32)localFileHeader.DecompressedSize) || // DecompressedSize
          !binaryWriter.SafeWriteUInt16((uint16)filenameLength) ||                   // FilenameLength
          !binaryWriter.SafeWriteUInt16(0) ||                                        // ExtraFieldLength
          !binaryWriter.SafeWriteFixedString(pFileEntry->FileName, filenameLength))  // Filename
      {
        return false;
      }
    }

    // read+write chunks
    static const uint32 CHUNKSIZE = 4096;
    byte chunkBuffer[CHUNKSIZE];
    uint64 remaining = pFileEntry->CompressedFileSize;
    while (remaining > 0)
    {
      uint32 copySize = (uint32)Min(remaining, (uint64)CHUNKSIZE);

      // read it
      uint32 bytesRead = m_pReadStream->Read(chunkBuffer, copySize);
      if (bytesRead != copySize)
      {
        // io error
        return false;
      }

      // write it
      uint32 bytesWritten = m_pWriteStream->Write(chunkBuffer, copySize);
      if (bytesWritten != copySize)
      {
        // io error
        return false;
      }

      remaining -= (uint64)copySize;
    }

    // create a copy of the entry, and store it in the write table
    FileEntry* pNewFileEntry = new FileEntry;
    pNewFileEntry->FileName = pFileEntry->FileName;
    pNewFileEntry->CompressionMethod = pFileEntry->CompressionMethod;
    pNewFileEntry->ModifiedTime = pFileEntry->ModifiedTime;
    pNewFileEntry->CRC32 = pFileEntry->CRC32;
    pNewFileEntry->OffsetToFileHeader = newStartingOffset;
    pNewFileEntry->CompressedFileSize = pFileEntry->CompressedFileSize;
    pNewFileEntry->DecompressedFileSize = pFileEntry->DecompressedFileSize;
    pNewFileEntry->IsDeleted = false;
    pNewFileEntry->WriteOpenCount = 0;
    m_writeFileHashTable.Insert(pNewFileEntry->FileName, pNewFileEntry);
  }

  // store offset to the start of the central directory
  uint64 centralDirectoryOffset = m_pWriteStream->GetPosition();
  uint32 nCentralDirectoryEntries = 0;

  // write the central directory to the write stream
  {
    BinaryWriter binaryWriter(m_pWriteStream, ENDIAN_TYPE_LITTLE);

    // write each entry
    for (FileHashTable::Iterator itr = m_writeFileHashTable.Begin(); !itr.AtEnd(); itr.Forward())
    {
      const FileEntry* pFileEntry = itr->Value;
      if (pFileEntry->IsDeleted)
        continue;

      // filenames are limited to 65535 characters
      uint32 filenameLength = Min(pFileEntry->FileName.GetLength(), (uint32)0xFFFF);

      // write header
      if (!binaryWriter.SafeWriteUInt32(ZIP_ARCHIVE_CENTRAL_DIRECTORY_FILE_HEADER_SIGNATURE) || // Signature
          !binaryWriter.SafeWriteUInt16(ZIP_ARCHIVE_VERSION_MADE_BY) ||                         // Version Made By
          !binaryWriter.SafeWriteUInt16(ZIP_ARCHIVE_VERSION_TO_EXTRACT) ||                      // Version To Extract
          !binaryWriter.SafeWriteUInt16(0) ||                                                   // Flags
          !binaryWriter.SafeWriteUInt16((uint16)pFileEntry->CompressionMethod) ||               // Compression Method
          !binaryWriter.SafeWriteUInt32(TimestampToZipTime(pFileEntry->ModifiedTime)) ||        // Modified Timestamp
          !binaryWriter.SafeWriteUInt32(pFileEntry->CRC32) ||                                   // CRC32
          !binaryWriter.SafeWriteUInt32((uint32)pFileEntry->CompressedFileSize) ||              // Compressed Size
          !binaryWriter.SafeWriteUInt32((uint32)pFileEntry->DecompressedFileSize) ||            // Decompressed Size
          !binaryWriter.SafeWriteUInt16((uint16)filenameLength) ||                              // Filename Length
          !binaryWriter.SafeWriteUInt16(0) ||                                                   // Extra Field Length
          !binaryWriter.SafeWriteUInt16(0) ||                                                   // File Comment Length
          !binaryWriter.SafeWriteUInt16(0) ||                                       // Disk number where file starts
          !binaryWriter.SafeWriteUInt16(0) ||                                       // Internal File Attributes
          !binaryWriter.SafeWriteUInt32(0) ||                                       // External File Attributes
          !binaryWriter.SafeWriteUInt32((uint32)pFileEntry->OffsetToFileHeader) ||  // Offset to file header
          !binaryWriter.SafeWriteFixedString(pFileEntry->FileName, filenameLength)) // Filename
      {
        return false;
      }

      nCentralDirectoryEntries++;
    }

    // can only write 65536 files to a zip archive
    uint64 centralDirectorySize = (uint64)(m_pWriteStream->GetPosition() - centralDirectoryOffset);
    nCentralDirectoryEntries = Min(nCentralDirectoryEntries, (uint32)0xFFFF);

    // write the end of central directory marker
    if (!binaryWriter.SafeWriteUInt32(ZIP_ARCHIVE_END_OF_CENTRAL_DIRECTORY_SIGNATURE) || // Signature
        !binaryWriter.SafeWriteUInt16(0) ||                                              // Disk Number
        !binaryWriter.SafeWriteUInt16(0) || // Disk where central directory starts
        !binaryWriter.SafeWriteUInt16(
          (uint16)nCentralDirectoryEntries) || // Central directory record count on this disk
        !binaryWriter.SafeWriteUInt16((uint16)nCentralDirectoryEntries) || // Central directory record count
        !binaryWriter.SafeWriteUInt32((uint32)centralDirectorySize) ||     // Central directory size
        !binaryWriter.SafeWriteUInt32((uint32)centralDirectoryOffset) ||   // Central directory offset
        !binaryWriter.SafeWriteUInt16(0))                                  // Comment length
    {
      return false;
    }
  }

  // close the read stream
  if (m_pReadStream != NULL)
  {
    m_pReadStream->Release();
    m_pReadStream = NULL;
  }

  // flush the write stream, commit it for atomic updated files
  m_pWriteStream->Flush();

  // empty the read hash table, and move everything from the write table into it
  for (FileHashTable::Iterator itr = m_readFileHashTable.Begin(); !itr.AtEnd(); itr.Forward())
    delete itr->Value;
  m_readFileHashTable.Clear();
  for (FileHashTable::Iterator itr = m_writeFileHashTable.Begin(); !itr.AtEnd(); itr.Forward())
  {
    FileEntry* pFileEntry = itr->Value;
    if (pFileEntry->IsDeleted)
      delete pFileEntry;
    else
      m_readFileHashTable.Insert(pFileEntry->FileName, pFileEntry);
  }
  m_writeFileHashTable.Clear();

  // replace stream pointers
  m_pReadStream = m_pWriteStream;
  m_pWriteStream = NULL;
  return true;
}

bool ZipArchive::DiscardChanges()
{
  // empty the write files table
  m_writeFileHashTable.Clear();

  // close it
  m_pWriteStream->Release();
  m_pWriteStream = NULL;
  return true;
}

ZipArchive* ZipArchive::CreateArchive(ByteStream* pWriteStream)
{
  ZipArchive* pZipArchive = new ZipArchive(NULL, pWriteStream);
  return pZipArchive;
}

ZipArchive* ZipArchive::OpenArchiveReadOnly(ByteStream* pReadStream)
{
  ZipArchive* pZipArchive = new ZipArchive(pReadStream, NULL);
  if (!pZipArchive->ParseZip())
  {
    delete pZipArchive;
    return NULL;
  }

  return pZipArchive;
}

ZipArchive* ZipArchive::OpenArchiveReadWrite(ByteStream* pReadStream, ByteStream* pWriteStream)
{
  DebugAssert(pReadStream != pWriteStream);

  ZipArchive* pZipArchive = new ZipArchive(pReadStream, NULL);
  if (!pZipArchive->ParseZip())
  {
    delete pZipArchive;
    return NULL;
  }

  if (!pZipArchive->UpgradeToWritableArchive(pWriteStream))
  {
    delete pZipArchive;
    return NULL;
  }

  return pZipArchive;
}

bool ZipArchive::FindFiles(const char* path, const char* pattern, uint32 flags,
                           FileSystem::FindResultsArray* pResults) const
{
  // has a path
  if (path[0] == '\0')
    return false;

  // clear result array
  if (!(flags & FILESYSTEM_FIND_KEEP_ARRAY))
    pResults->Clear();

  uint32 count = FindFilesInTable(m_writeFileHashTable, path, pattern, flags, pResults);
  count += FindFilesInTable(m_readFileHashTable, path, pattern, flags, pResults);

  return (count > 0);
}

uint32 ZipArchive::FindFilesInTable(const FileHashTable& fileTable, const char* path, const char* pattern, uint32 flags,
                                    FileSystem::FindResultsArray* pResults) const
{
  // small speed optimization for '*' case
  bool hasWildCards = false;
  bool wildCardMatchAll = false;
  if (Y_strpbrk(pattern, "*?") != NULL)
  {
    hasWildCards = true;
    wildCardMatchAll = !(Y_strcmp(pattern, "*"));
  }

  // ignore leading /
  if (path[0] == '/')
    path++;

  // get length of path
  uint32 pathLength = Y_strlen(path);
  uint32 count = 0;

  // iterate over files
  for (FileHashTable::ConstIterator itr = fileTable.Begin(); !itr.AtEnd(); itr.Forward())
  {
    const FileEntry* pFileEntry = itr->Value;
    if (pFileEntry->IsDeleted)
      continue;

    if (Y_strnicmp(pFileEntry->FileName, path, pathLength) == 0 && pFileEntry->FileName[pathLength] == '/')
    {
      // path matches
      const char* afterPathPart = pFileEntry->FileName.GetCharArray() + pathLength + 1;

      // check if it contains another directory
      if (!(flags & FILESYSTEM_FIND_RECURSIVE) && Y_strchr(afterPathPart, '/') != NULL)
        continue;

      // match the filename part
      const char* fileNamePart = Y_strrchr(afterPathPart, '/');
      if (fileNamePart == NULL)
        fileNamePart = afterPathPart;
      else
        fileNamePart++;

      if (hasWildCards)
      {
        if (!wildCardMatchAll && !Y_strwildcmp(fileNamePart, pattern))
          continue;
      }
      else
      {
        if (Y_stricmp(fileNamePart, pattern) != 0)
          continue;
      }

      // create data
      FILESYSTEM_FIND_DATA outData;
      if (flags & FILESYSTEM_FIND_RELATIVE_PATHS)
        Y_strncpy(outData.FileName, countof(outData.FileName), afterPathPart);
      else
        Y_strncpy(outData.FileName, countof(outData.FileName), pFileEntry->FileName);

      // remaining fields
      outData.ModificationTime = pFileEntry->ModifiedTime;
      outData.Attributes = 0;
      outData.Size = pFileEntry->DecompressedFileSize;

      // check it does not exist
      uint32 i;
      for (i = 0; i < pResults->GetSize(); i++)
      {
        if (Y_stricmp(outData.FileName, pResults->GetElement(i).FileName) == 0)
          break;
      }
      if (i == pResults->GetSize())
      {
        pResults->Add(outData);
        count++;
      }
    }
  }

  return count;
}

#endif // HAVE_ZLIB
