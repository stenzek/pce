#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/String.h"

class Error;

namespace Platform {

// returns a temporary filename with the X's replaced with random characters
void MakeTempFileName(char* filename, uint32 len);
void MakeTempFileName(String& filename);

// gets this program's file name
bool GetProgramFileName(String& destination);

// get memory usage of the process in bytes
size_t GetProgramMemoryUsage();

// initialize socket support.
bool InitializeSocketSupport(Error* pError);

}; // namespace Platform
