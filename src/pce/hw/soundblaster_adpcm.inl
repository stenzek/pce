/*
 *  Copyright (C) 2002-2015  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <algorithm>
#include <cstdint>

inline int16_t DecodeADPCM_2(uint8_t sample, uint8_t& reference, int32_t& scale)
{
  static const int32_t scaleMap[24] = {0, 1,  0,  -1,  1, 3,  -1, -3,  2, 6,  -2,  -6,
                                       4, 12, -4, -12, 8, 24, -8, -24, 6, 48, -16, -48};
  static const int32_t adjustMap[24] = {0,   4, 0,   4, 252, 4, 252, 4, 252, 4, 252, 4,
                                        252, 4, 252, 4, 252, 4, 252, 4, 252, 0, 252, 0};

  int32_t samp = int32_t(sample) + scale;
  samp = std::max(0, std::min(23, samp));

  int32_t ref = int32_t(reference) + scaleMap[samp];
  reference = uint8_t(std::max(0, std::min(255, ref)));
  scale = (scale + adjustMap[samp]) & 0xFF;

  uint16 out_16_unsigned = (uint16_t(reference) << 8) | uint16_t(reference);
  return int16_t(int32(ZeroExtend32(out_16_unsigned)) - 32768);
}

inline int16_t DecodeADPCM_3(uint8_t sample, uint8_t& reference, int32_t& scale)
{
  static const int32_t scaleMap[40] = {0,  1,   2,   3,   0,  -1, -2, -3, 1,   3,   5,   7,  -1, -3,
                                       -5, -7,  2,   6,   10, 14, -2, -6, -10, -14, 4,   12, 20, 28,
                                       -4, -12, -20, -28, 5,  15, 25, 35, -5,  -15, -25, -35};
  static const int32_t adjustMap[40] = {0,   0, 0, 8, 0,   0, 0, 8, 248, 0, 0, 8, 248, 0, 0, 8, 248, 0, 0, 8,
                                        248, 0, 0, 8, 248, 0, 0, 8, 248, 0, 0, 8, 248, 0, 0, 0, 248, 0, 0, 0};

  int32_t samp = int32_t(sample) + scale;
  samp = std::max(0, std::min(39, samp));

  int32_t ref = int32_t(reference) + scaleMap[samp];
  reference = uint8_t(std::max(0, std::min(255, ref)));
  scale = (scale + adjustMap[samp]) & 0xFF;

  uint16 out_16_unsigned = (uint16_t(sample) << 8) | uint16_t(sample);
  return int16_t(int32(ZeroExtend32(out_16_unsigned)) - 32768);
}

inline int16_t DecodeADPCM_4(uint8_t sample, uint8_t& reference, int32_t& scale)
{
  static const int32_t scaleMap[64] = {0, 1,  2,  3,  4,  5,  6,  7,  0,  -1,  -2,  -3,  -4,  -5,  -6,  -7,
                                       1, 3,  5,  7,  9,  11, 13, 15, -1, -3,  -5,  -7,  -9,  -11, -13, -15,
                                       2, 6,  10, 14, 18, 22, 26, 30, -2, -6,  -10, -14, -18, -22, -26, -30,
                                       4, 12, 20, 28, 36, 44, 52, 60, -4, -12, -20, -28, -36, -44, -52, -60};
  static const int32_t adjustMap[64] = {
    0,   0, 0, 0, 0, 16, 16, 16, 0,   0, 0, 0, 0, 16, 16, 16, 240, 0, 0, 0, 0, 16, 16, 16, 240, 0, 0, 0, 0, 16, 16, 16,
    240, 0, 0, 0, 0, 16, 16, 16, 240, 0, 0, 0, 0, 16, 16, 16, 240, 0, 0, 0, 0, 0,  0,  0,  240, 0, 0, 0, 0, 0,  0,  0};

  int32_t samp = int32_t(sample) + scale;
  samp = std::max(0, std::min(63, samp));

  int32_t ref = int32_t(reference) + scaleMap[samp];
  reference = uint8_t(std::max(0, std::min(255, ref)));
  scale = (scale + adjustMap[samp]) & 0xFF;

  uint16 out_16_unsigned = (uint16_t(reference) << 8) | uint16_t(reference);
  return int16_t(int32(ZeroExtend32(out_16_unsigned)) - 32768);
}
