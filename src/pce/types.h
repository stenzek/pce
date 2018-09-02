#pragma once
#include "common/types.h"

// Physical memory addresses are 32-bits wide
using PhysicalMemoryAddress = uint32;
using LinearMemoryAddress = uint32;

// IO port read/write sizes
enum IOPortDataSize : uint32
{
  IOPortDataSize_8,
  IOPortDataSize_16
};

enum class CPUBackendType
{
  Interpreter,
  CachedInterpreter,
  Recompiler
};

// TODO: Put this in a namespace somewhere...
void RegisterAllTypes();