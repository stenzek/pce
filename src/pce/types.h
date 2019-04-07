#pragma once
#include "common/types.h"

// Physical memory addresses are 32-bits wide
using PhysicalMemoryAddress = u32;
using LinearMemoryAddress = u32;

// IO port read/write sizes
enum IOPortDataSize : u32
{
  IOPortDataSize_8,
  IOPortDataSize_16
};

// TODO: Put this in a namespace somewhere...
void RegisterAllTypes();