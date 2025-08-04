#pragma once
#include "YBaseLib/Common.h"

#ifdef HAVE_ZLIB
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/ProgressCallbacks.h"

// from zlib/minizip
//#include <zip.h>
//#include <unzip.h>
#include <zlib.h>

namespace ZLibHelpers {

#if 0

// common helpers
void FillZLibFileFuncsForExistingByteStream(zlib_filefunc64_def *pFileFuncs, ByteStream *pStream);

// unzip helpers
bool LocateAndExtractFile(unzFile pUnzFile, const char *FileName, void **ppBufferPointer, uint32 *pBufferSize);
ByteStream *LocateAndExtractFileStream(unzFile pUnzFile, const char *FileName);
ByteStream *LocateAndExtractFileStreamProgress(unzFile pUnzFile, const char *FileName, ProgressCallbacks *pProgressCallbacks);

// zip helpers
bool WriteFileStreamToZip(zipFile pZipFile, const char *FileName, ByteStream *pStream);
bool WriteFileStreamToZipProgress(zipFile pZipFile, const char *FileName, ByteStream *pStream, ProgressCallbacks *pProgressCallbacks);

#endif

// deflate helpers. buffer should be at least nbytes * 2 long
uint32 GetDeflatedBufferUpperBounds(uint32 cbSourceBuffer, const int compressionLevel = Z_DEFAULT_COMPRESSION);
bool WriteDeflatedDataToBuffer(void* pDestinationBuffer, uint32 cbDestinationBuffer, uint32* pCompressedSize,
                               const void* pSourceBuffer, uint32 cbSourceBuffer,
                               const int compressionLevel = Z_DEFAULT_COMPRESSION);
// bool WriteDeflatedDataToStream(ByteStream *pDestinationStream, const void *pSourceBuffer, uint32 cbSourceBuffer,
// const int compressionLevel = Z_DEFAULT_COMPRESSION);
bool ReadDeflatedDataFromBuffer(void* pDestinationBuffer, uint32 cbDestinationBuffer, uint32* pDecompressedSize,
                                const void* pSourceBuffer, uint32 cbSourceBuffer);
// bool ReadDeflatedDataFromStream(void *pDestinationBuffer, uint32 cbDestinationBuffer, ByteStream *pSourceStream);

} // namespace ZipHelpers

#endif // HAVE_ZLIB
