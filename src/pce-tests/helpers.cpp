#include "pce-tests/helpers.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/mmio.h"
#include <cstdio>
Log_SetChannel(TestHelpers);

std::string StringFromFormat(const char* fmt, ...)
{
  std::va_list ap;
  va_start(ap, fmt);

  int count = vsnprintf(nullptr, 0, fmt, ap);
  if (count <= 0)
    return std::string();

  std::string ret;
  ret.resize(count);
  std::vsnprintf(const_cast<char*>(ret.data()), ret.size() + 1, fmt, ap);
  va_end(ap);

  return ret;
}

bool ReadFileToArray(PODArray<byte>* dest_array, const char* filename)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &stream))
  {
    Log_ErrorPrintf("Failed to open code file %s", filename);
    return false;
  }

  dest_array->Resize(static_cast<u32>(stream->GetSize()));
  if (!stream->Read2(dest_array->GetBasePointer(), dest_array->GetSize()))
  {
    Log_ErrorPrintf("Failed to read code file %s", filename);
    stream->Release();
    return false;
  }
  stream->Release();

  return true;
}

bool LoadFileToRam(System* system, const char* filename, PhysicalMemoryAddress base_address)
{
  PODArray<byte> data;
  if (!ReadFileToArray(&data, filename))
    return false;

  for (u32 i = 0; i < data.GetSize(); i++)
    system->GetBus()->WriteMemoryByte(base_address + i, data[i]);

  return true;
}

bool MapFileToRam(System* system, const char* filename, PhysicalMemoryAddress base_address)
{
  PODArray<byte> data;
  if (!ReadFileToArray(&data, filename))
    return false;

  // TODO: Fix memory leak here.. somehow
  byte* data_ptr;
  u32 length;
  data.DetachArray(&data_ptr, &length);

  MMIO* mmio = MMIO::CreateDirect(base_address, length, data_ptr, true, false);
  system->GetBus()->ConnectMMIO(mmio);
  mmio->Release();
  return true;
}
