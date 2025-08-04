#pragma once
#include "YBaseLib/Common.h"

#ifdef HAVE_ZLIB
#include "YBaseLib/CIStringHashTable.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/String.h"
#include "YBaseLib/Timestamp.h"

class ByteStream;

class ZipArchive
{
  friend class ZipArchiveStreamedReadByteStream;
  friend class ZipArchiveStreamedWriteByteStream;
  friend class ZipArchiveBufferedReadByteStream;
  friend class ZipArchiveBufferedWriteByteStream;

public:
  ~ZipArchive();

  const bool IsWritable() const { return (m_pWriteStream != NULL); }

  // archive operations
  bool StatFile(const char* filename, FILESYSTEM_STAT_DATA* pStatData) const;
  bool FindFiles(const char* path, const char* pattern, uint32 flags, FileSystem::FindResultsArray* pResults) const;
  ByteStream* OpenFile(const char* filename, uint32 openMode, uint32 compressionLevel = 6);
  bool DeleteFile(const char* filename);

  // copy a file from another zip archive
  bool CopyFile(ZipArchive* pOtherArchive, const char* filename);

  // upgrades a read-only archive to a read-write archive by providing a new write stream.
  bool UpgradeToWritableArchive(ByteStream* pWriteStream);

  // commit all changes, completing the write stream, replacing the read pointer with the write pointer.
  // this function expects that the write stream was also opened in write mode.
  bool CommitChanges();

  // discard all changes.
  // this function will remove the write stream, UpgradeToWritableArchive must be called before it can be re-called
  bool DiscardChanges();

  // opens a new archive
  static ZipArchive* CreateArchive(ByteStream* pWriteStream);

  // open the archive read-only.
  static ZipArchive* OpenArchiveReadOnly(ByteStream* pReadStream);

  // opens the archive read-write.
  static ZipArchive* OpenArchiveReadWrite(ByteStream* pReadStream, ByteStream* pWriteStream);

  // todo: defragment methods

private:
  ZipArchive(ByteStream* pReadStream, ByteStream* pWriteStream);
  bool ParseZip();

  ByteStream* m_pReadStream;
  ByteStream* m_pWriteStream;
  uint32 m_nOpenStreamedReads;
  uint32 m_nOpenStreamedWrites;
  uint32 m_nOpenBufferedReads;
  uint32 m_nOpenBufferedWrites;

  struct FileEntry
  {
    String FileName;
    uint32 CompressionMethod;
    Timestamp ModifiedTime;
    uint32 CRC32;
    uint64 OffsetToFileHeader;
    uint64 CompressedFileSize;
    uint64 DecompressedFileSize;
    bool IsDeleted;
    uint32 WriteOpenCount;
  };

  typedef CIStringHashTable<FileEntry*> FileHashTable;
  FileHashTable m_readFileHashTable;
  FileHashTable m_writeFileHashTable;

  uint32 FindFilesInTable(const FileHashTable& fileTable, const char* path, const char* pattern, uint32 flags,
                          FileSystem::FindResultsArray* pResults) const;
};

#endif // HAVE_ZLIB
