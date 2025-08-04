#pragma once

#include "YBaseLib/Common.h"

struct Y_CPUID_RESULT
{
  uint32 MaxBasicLevel;
  uint32 MaxExtendedLevel;
  uint32 ProcessorCount; // 1
  uint32 ThreadCount;
  char VendorString[12 + 1]; // GenuineIntel
  uint32 ClockSpeed;         // 4000
  char BrandString[48 + 1];  //
  uint8 Stepping;
  uint8 Model;
  uint8 Family;
  uint8 Type;
  uint8 BrandId;
  char SummaryString[192];

  bool SupportsCMOV;
  bool SupportsMMX;
  bool SupportsSSE;
  bool SupportsSSE2;
  bool SupportsSSE3;
  bool SupportsSSSE3;
  bool SupportsSSE4A;
  bool SupportsSSE41;
  bool SupportsSSE42;
  bool SupportsSSE5A;
  bool SupportsAVX;
  bool SupportsAES;
  bool SupportsHTT;
  bool Supports3DNow;
  bool Supports3DNow2;
};

void Y_ReadCPUID(Y_CPUID_RESULT* pResult);
