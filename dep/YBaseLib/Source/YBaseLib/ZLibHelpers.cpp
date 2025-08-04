#include "YBaseLib/ZLibHelpers.h"
#include "YBaseLib/Memory.h"

#ifdef HAVE_ZLIB

// Ensure zlib gets pulled in
#if Y_COMPILER_MSVC
#pragma comment(lib, "zdll.lib")
#endif

namespace ZLibHelpers {

#if 0

static ZCALLBACK void *__zlib_filefuncs_open64(void *pUserData, const void *filename, int mode) { return pUserData; }
static ZCALLBACK uLong __zlib_filefuncs_read(void *pUserData, void *pStream, void *buf, uLong size) { return reinterpret_cast<ByteStream *>(pUserData)->Read(buf, size); }
static ZCALLBACK uLong __zlib_filefuncs_write(void *pUserData, void *pStream, const void *buf, uLong size) { return reinterpret_cast<ByteStream *>(pUserData)->Write(buf, size); }
static ZCALLBACK ZPOS64_T __zlib_filefuncs_tell64(void *pUserData, void *pStream) { return reinterpret_cast<ByteStream *>(pUserData)->GetPosition(); }
static ZCALLBACK int __zlib_filefuncs_close(void *pUserData, void *pStream) { reinterpret_cast<ByteStream *>(pUserData)->Flush(); return 0; }
static ZCALLBACK int __zlib_filefuncs_testerror(void *pUserData, void *pStream) { return (reinterpret_cast<ByteStream *>(pUserData)->InErrorState()) ? -1 : 0; }
static ZCALLBACK long __zlib_filefuncs_seek64(void *pUserData, void *pStream, ZPOS64_T offset, int mode)
{
    if (mode == ZLIB_FILEFUNC_SEEK_CUR) { return reinterpret_cast<ByteStream *>(pUserData)->SeekRelative(offset) ? 0 : -1; }
    else if (mode == ZLIB_FILEFUNC_SEEK_END) { return reinterpret_cast<ByteStream *>(pUserData)->SeekToEnd() ? 0 : -1; }
    else if (mode == ZLIB_FILEFUNC_SEEK_SET) { return reinterpret_cast<ByteStream *>(pUserData)->SeekAbsolute(offset) ? 0 : -1; }
    else { return -1; }
}

void FillZLibFileFuncsForExistingByteStream(zlib_filefunc64_def *pFileFuncs, ByteStream *pStream)
{   
    pFileFuncs->zopen64_file = __zlib_filefuncs_open64;
    pFileFuncs->zread_file = __zlib_filefuncs_read;
    pFileFuncs->zwrite_file = __zlib_filefuncs_write;
    pFileFuncs->ztell64_file = __zlib_filefuncs_tell64;
    pFileFuncs->zseek64_file = __zlib_filefuncs_seek64;
    pFileFuncs->zclose_file = __zlib_filefuncs_close;
    pFileFuncs->zerror_file = __zlib_filefuncs_testerror;
    pFileFuncs->opaque = reinterpret_cast<void *>(pStream);
}

bool LocateAndExtractFile(void *pUnzFile, const char *FileName, void **ppBufferPointer, uint32 *pBufferSize)
{
    unzFile realUnzFile = reinterpret_cast<unzFile>(pUnzFile);

    if (unzLocateFile(realUnzFile, FileName, 2) != UNZ_OK)
        return false;

    unz_file_info64 fileInfo;
    if (unzGetCurrentFileInfo64(realUnzFile, &fileInfo, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK)
        return false;

    if (fileInfo.uncompressed_size > 0xFFFFFFFFULL)
        return false;

    if (unzOpenCurrentFile(realUnzFile) != UNZ_OK)
        return false;

    uint32 fileSize = (uint32)fileInfo.uncompressed_size;
    byte *pBuffer = (byte *)malloc(fileSize);
    if ((uint32)unzReadCurrentFile(realUnzFile, pBuffer, fileSize) != fileSize)
    {
        free(pBuffer);
        unzCloseCurrentFile(realUnzFile);
        return false;
    }

    unzCloseCurrentFile(realUnzFile);

    *ppBufferPointer = pBuffer;
    *pBufferSize = fileSize;
    return true;
}

ByteStream *LocateAndExtractFileStream(void *pUnzFile, const char *FileName)
{
    unzFile realUnzFile = reinterpret_cast<unzFile>(pUnzFile);

    if (unzLocateFile(realUnzFile, FileName, 2) != UNZ_OK)
        return NULL;

    unz_file_info64 fileInfo;
    if (unzGetCurrentFileInfo64(realUnzFile, &fileInfo, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK)
        return NULL;

    if (fileInfo.uncompressed_size > 0xFFFFFFFFULL)
        return NULL;

    if (unzOpenCurrentFile(realUnzFile) != UNZ_OK)
        return NULL;

    uint32 fileSize = (uint32)fileInfo.uncompressed_size;
    ByteStream *pStream = ByteStream_CreateGrowableMemoryStream(NULL, fileSize);
    uint32 position = 0;
    while (position < fileSize)
    {
        static const uint32 CHUNKSIZE = 4096;
        byte chunkData[CHUNKSIZE];

        uint32 readSize = Min(CHUNKSIZE, (fileSize - position));
        if ((uint32)unzReadCurrentFile(realUnzFile, chunkData, readSize) != readSize)
        {
            pStream->Release();
            unzCloseCurrentFile(realUnzFile);
            return NULL;
        }

        // no point in checking return code here, memory will always succeed
        pStream->Write(chunkData, readSize);
        position += readSize;
    }

    pStream->SeekAbsolute(0);
    unzCloseCurrentFile(realUnzFile);
    return pStream;
}

ByteStream *LocateAndExtractFileStreamProgress(void *pUnzFile, const char *FileName, ProgressCallbacks *pProgressCallbacks)
{
    unzFile realUnzFile = reinterpret_cast<unzFile>(pUnzFile);

    if (unzLocateFile(realUnzFile, FileName, 2) != UNZ_OK)
        return NULL;

    unz_file_info64 fileInfo;
    if (unzGetCurrentFileInfo64(realUnzFile, &fileInfo, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK)
        return NULL;

    if (fileInfo.uncompressed_size > 0xFFFFFFFFULL)
        return NULL;

    if (unzOpenCurrentFile(realUnzFile) != UNZ_OK)
        return NULL;

    uint32 fileSize = (uint32)fileInfo.uncompressed_size;
    pProgressCallbacks->SetProgressRange(fileSize);
    pProgressCallbacks->SetProgressValue(0);

    ByteStream *pStream = ByteStream_CreateGrowableMemoryStream(NULL, fileSize);
    uint32 position = 0;
    while (position < fileSize)
    {
        static const uint32 CHUNKSIZE = 4096;
        byte chunkData[CHUNKSIZE];

        uint32 readSize = Min(CHUNKSIZE, (fileSize - position));
        if ((uint32)unzReadCurrentFile(realUnzFile, chunkData, readSize) != readSize)
        {
            pStream->Release();
            unzCloseCurrentFile(realUnzFile);
            return NULL;
        }

        // no point in checking return code here, memory will always succeed
        pStream->Write(chunkData, readSize);
        position += readSize;

        // update progress
        pProgressCallbacks->SetProgressValue(position);
        if (pProgressCallbacks->IsCancelled())
        {
            pStream->Release();
            unzCloseCurrentFile(realUnzFile);
            return NULL;
        }
    }

    pStream->SeekAbsolute(0);
    unzCloseCurrentFile(realUnzFile);
    pProgressCallbacks->SetProgressValue(position);
    return pStream;
}

bool WriteFileStreamToZip(void *pZipFile, const char *FileName, ByteStream *pStream)
{
    zipFile pRealZipFile = reinterpret_cast<unzFile>(pZipFile);

    // open file in zip file
    zip_fileinfo zfi;
    Y_memzero(&zfi, sizeof(zfi));
    if (zipOpenNewFileInZip64(pRealZipFile, FileName, &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0) != ZIP_OK)
        return false;

    // actually write (compress) it
    uint32 streamSize = (uint32)pStream->GetSize();
    uint32 streamPosition = 0;
    pStream->SeekAbsolute(0);

    while (streamPosition < streamSize)
    {
        static const uint32 CHUNKSIZE = 4096;
        byte chunkData[CHUNKSIZE];

        uint32 readSize = Min(CHUNKSIZE, (streamSize - streamPosition));
        if (pStream->Read(chunkData, readSize) != readSize ||
            zipWriteInFileInZip(pRealZipFile, chunkData, readSize) != ZIP_OK)
        {
            zipCloseFileInZip(pRealZipFile);
            return false;
        }

        streamPosition += readSize;
    }

    // close file
    zipCloseFileInZip(pRealZipFile);
    return true;
}

bool WriteFileStreamToZipProgress(void *pZipFile, const char *FileName, ByteStream *pStream, ProgressCallbacks *pProgressCallbacks)
{
    zipFile pRealZipFile = reinterpret_cast<unzFile>(pZipFile);

    // open file in zip file
    zip_fileinfo zfi;
    Y_memzero(&zfi, sizeof(zfi));
    if (zipOpenNewFileInZip64(pRealZipFile, FileName, &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0) != ZIP_OK)
        return false;

    // actually write (compress) it
    uint32 streamSize = (uint32)pStream->GetSize();
    uint32 streamPosition = 0;

    pStream->SeekAbsolute(0);
    pProgressCallbacks->SetProgressRange(streamSize);
    pProgressCallbacks->SetProgressValue(0);

    while (streamPosition < streamSize)
    {
        static const uint32 CHUNKSIZE = 4096;
        byte chunkData[CHUNKSIZE];

        uint32 readSize = Min(CHUNKSIZE, (streamSize - streamPosition));
        if (pStream->Read(chunkData, readSize) != readSize ||
            zipWriteInFileInZip(pRealZipFile, chunkData, readSize) != ZIP_OK)
        {
            zipCloseFileInZip(pRealZipFile);
            return false;
        }

        streamPosition += readSize;

        // update progress
        pProgressCallbacks->SetProgressValue(streamPosition);

        if (pProgressCallbacks->IsCancelled())
        {
            zipCloseFileInZip(pRealZipFile);
            return false;
        }
    }

    // close file
    zipCloseFileInZip(pRealZipFile);
    pProgressCallbacks->SetProgressValue(streamPosition);
    return true;
}

#endif

uint32 GetDeflatedBufferUpperBounds(uint32 cbSourceBuffer, const int compressionLevel /*= Z_DEFAULT_COMPRESSION*/)
{
  z_stream zStream;
  Y_memzero(&zStream, sizeof(zStream));
  if (deflateInit(&zStream, compressionLevel) != Z_OK)
    return cbSourceBuffer * 4;

  uint32 nBytes = deflateBound(&zStream, cbSourceBuffer);
  deflateEnd(&zStream);
  return nBytes;
}

bool WriteDeflatedDataToBuffer(void* pDestinationBuffer, uint32 cbDestinationBuffer, uint32* pCompressedSize,
                               const void* pSourceBuffer, uint32 cbSourceBuffer,
                               const int compressionLevel /*= Z_DEFAULT_COMPRESSION*/)
{
  z_stream zStream;
  Y_memzero(&zStream, sizeof(zStream));
  if (deflateInit(&zStream, compressionLevel) != Z_OK)
    return false;

  zStream.avail_in = cbSourceBuffer;
  zStream.next_in = (Bytef*)pSourceBuffer;
  zStream.avail_out = cbDestinationBuffer;
  zStream.next_out = (Bytef*)pDestinationBuffer;
  while (zStream.avail_in > 0)
  {
    int ret = deflate(&zStream, Z_FINISH);
    if (ret == Z_STREAM_END)
    {
      break;
    }
    else if (ret == Z_OK && zStream.avail_out > 0)
    {
      continue;
    }
    else
    {
      deflateEnd(&zStream);
      return false;
    }
  }

  if (pCompressedSize != NULL)
    *pCompressedSize = (uint32)zStream.total_out;

  deflateEnd(&zStream);
  return true;
}

// bool WriteDeflatedDataToStream(ByteStream *pDestinationStream, const void *pSourceBuffer, uint32 cbSourceBuffer,
// const int compressionLevel /*= Z_DEFAULT_COMPRESSION*/)
// {
//
// }

bool ReadDeflatedDataFromBuffer(void* pDestinationBuffer, uint32 cbDestinationBuffer, uint32* pDecompressedSize,
                                const void* pSourceBuffer, uint32 cbSourceBuffer)
{
  z_stream zStream;
  Y_memzero(&zStream, sizeof(zStream));
  if (inflateInit(&zStream) != Z_OK)
    return false;

  zStream.avail_in = cbSourceBuffer;
  zStream.next_in = (Bytef*)pSourceBuffer;
  zStream.avail_out = cbDestinationBuffer;
  zStream.next_out = (Bytef*)pDestinationBuffer;
  while (zStream.avail_in > 0)
  {
    int ret = inflate(&zStream, Z_FINISH);
    if (ret == Z_STREAM_END)
    {
      break;
    }
    else if (ret == Z_OK && zStream.avail_out > 0)
    {
      continue;
    }
    else
    {
      deflateEnd(&zStream);
      return false;
    }
  }

  if (pDecompressedSize != NULL)
    *pDecompressedSize = (uint32)zStream.total_out;

  deflateEnd(&zStream);
  return true;
}

// bool ReadDeflatedDataFromStream(void *pDestinationBuffer, uint32 cbDestinationBuffer, ByteStream *pSourceStream)
// {
//
// }

} // namespace ZipHelpers

#endif // HAVE_ZLIB
