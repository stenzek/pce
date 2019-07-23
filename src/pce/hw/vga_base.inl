#pragma once
#include "../types.h"

namespace HW {
constexpr u32 VGABase::Convert6BitColorTo8Bit(u32 color)
{
  // Convert 6-bit to 8-bit color by shifting low to high bits, and duplicating the low bits (00123456 -> 12345656).
  return ((color << 2) & UINT32_C(0x00FCFCFC)) | (color & UINT32_C(0xFF030303));
}

constexpr u32 VGABase::Convert8BitColorTo6Bit(u32 color)
{
  // Truncate low bits.
  return UINT32_C(0xFF000000) | (color & UINT32_C(0x00FCFCFC)) >> 2;
}
} // namespace HW