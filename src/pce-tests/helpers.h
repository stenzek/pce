#pragma once

#include <string>

#include "YBaseLib/ByteStream.h"
#include "YBaseLib/PODArray.h"

#include "pce/system.h"
#include "pce/types.h"

std::string StringFromFormat(const char* fmt, ...);

bool ReadFileToArray(PODArray<byte>* dest_array, const char* filename);
bool LoadFileToRam(System* system, const char* filename, PhysicalMemoryAddress base_address);
bool MapFileToRam(System* system, const char* filename, PhysicalMemoryAddress base_address);
