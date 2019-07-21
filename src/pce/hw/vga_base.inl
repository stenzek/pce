#pragma once
#include "../types.h"

namespace HW {

u32 VGABase::Convert6BitColorTo8Bit(uint32 color)
{
  u8 r = Truncate8(color);
  u8 g = Truncate8(color >> 8);
  u8 b = Truncate8(color >> 16);

  // Convert 6-bit color to 8-bit color by shifting low bits to high bits (00123456 -> 12345612).
  r = (r << 2) | (r >> 4);
  g = (g << 2) | (g >> 4);
  b = (b << 2) | (b >> 4);

  return (color & 0xFF000000) | ZeroExtend32(r) | (ZeroExtend32(g) << 8) | (ZeroExtend32(b) << 16);
}

u32 VGABase::ConvertBGR555ToRGB24(uint16 color)
{
  u8 b = Truncate8(color & 31);
  u8 g = Truncate8((color >> 5) & 31);
  u8 r = Truncate8((color >> 10) & 31);

  // 00012345 -> 1234545
  b = (b << 3) | (b >> 3);
  g = (g << 3) | (g >> 3);
  r = (r << 3) | (r >> 3);

  return (color & 0xFF000000) | ZeroExtend32(r) | (ZeroExtend32(g) << 8) | (ZeroExtend32(b) << 16);
}

u32 VGABase::ConvertBGR565ToRGB24(uint16 color)
{
  u8 b = Truncate8(color & 31);
  u8 g = Truncate8((color >> 5) & 63);
  u8 r = Truncate8((color >> 11) & 31);

  // 00012345 -> 1234545 / 00123456 -> 12345656
  b = (b << 3) | (b >> 3);
  g = (g << 2) | (g >> 4);
  r = (r << 3) | (r >> 3);

  return (color & 0xFF000000) | ZeroExtend32(r) | (ZeroExtend32(g) << 8) | (ZeroExtend32(b) << 16);
}

} // namespace HW