#pragma once
#include "../types.h"

/***************************************************************************/
/*        Portion of this software comes with the following license:       */
/***************************************************************************/
/*

    Copyright Aaron Giles
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in
          the documentation and/or other materials provided with the
          distribution.
        * Neither the name 'MAME' nor the names of its contributors may be
          used to endorse or promote products derived from this software
          without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

/* poly_param_extent describes information for a single parameter in an extent */
struct poly_param_extent
{
  float start; /* parameter value at starting X,Y */
  float dpdx;  /* dp/dx relative to starting X */
};

#define MAX_VERTEX_PARAMS 6

/* poly_extent describes start/end points for a scanline, along with per-scanline parameters */
struct poly_extent
{
  s32 startx;                                 /* starting X coordinate (inclusive) */
  s32 stopx;                                  /* ending X coordinate (exclusive) */
  poly_param_extent param[MAX_VERTEX_PARAMS]; /* starting and dx values for each parameter */
};

/* an rgb_t is a single combined R,G,B (and optionally alpha) value */
typedef u32 rgb_t;

/* an rgb15_t is a single combined 15-bit R,G,B value */
typedef u16 rgb15_t;

/* macros to assemble rgb_t values */
#define MAKE_ARGB(a, r, g, b)                                                                                          \
  ((((rgb_t)(a)&0xff) << 24) | (((rgb_t)(r)&0xff) << 16) | (((rgb_t)(g)&0xff) << 8) | ((rgb_t)(b)&0xff))
#define MAKE_RGB(r, g, b) (MAKE_ARGB(255, r, g, b))

/* macros to extract components from rgb_t values */
#define RGB_ALPHA(rgb) (((rgb) >> 24) & 0xff)
#define RGB_RED(rgb) (((rgb) >> 16) & 0xff)
#define RGB_GREEN(rgb) (((rgb) >> 8) & 0xff)
#define RGB_BLUE(rgb) ((rgb)&0xff)

/* common colors */
#define RGB_BLACK (MAKE_ARGB(255, 0, 0, 0))
#define RGB_WHITE (MAKE_ARGB(255, 255, 255, 255))

/***************************************************************************
    inline FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    rgb_to_rgb15 - convert an RGB triplet to
    a 15-bit OSD-specified RGB value
-------------------------------------------------*/

inline rgb15_t rgb_to_rgb15(rgb_t rgb)
{
  return ((RGB_RED(rgb) >> 3) << 10) | ((RGB_GREEN(rgb) >> 3) << 5) | ((RGB_BLUE(rgb) >> 3) << 0);
}

/*-------------------------------------------------
    rgb_clamp - clamp an RGB component to 0-255
-------------------------------------------------*/

inline u8 rgb_clamp(s32 value)
{
  if (value < 0)
    return 0;
  if (value > 255)
    return 255;
  return (u8)value;
}

/*-------------------------------------------------
    pal1bit - convert a 1-bit value to 8 bits
-------------------------------------------------*/

inline u8 pal1bit(u8 bits)
{
  return (bits & 1) ? 0xff : 0x00;
}

/*-------------------------------------------------
    pal2bit - convert a 2-bit value to 8 bits
-------------------------------------------------*/

inline u8 pal2bit(u8 bits)
{
  bits &= 3;
  return (bits << 6) | (bits << 4) | (bits << 2) | bits;
}

/*-------------------------------------------------
    pal3bit - convert a 3-bit value to 8 bits
-------------------------------------------------*/

inline u8 pal3bit(u8 bits)
{
  bits &= 7;
  return (bits << 5) | (bits << 2) | (bits >> 1);
}

/*-------------------------------------------------
    pal4bit - convert a 4-bit value to 8 bits
-------------------------------------------------*/

inline u8 pal4bit(u8 bits)
{
  bits &= 0xf;
  return (bits << 4) | bits;
}

/*-------------------------------------------------
    pal5bit - convert a 5-bit value to 8 bits
-------------------------------------------------*/

inline u8 pal5bit(u8 bits)
{
  bits &= 0x1f;
  return (bits << 3) | (bits >> 2);
}

/*-------------------------------------------------
    pal6bit - convert a 6-bit value to 8 bits
-------------------------------------------------*/

inline u8 pal6bit(u8 bits)
{
  bits &= 0x3f;
  return (bits << 2) | (bits >> 4);
}

/*-------------------------------------------------
    pal7bit - convert a 7-bit value to 8 bits
-------------------------------------------------*/

inline u8 pal7bit(u8 bits)
{
  bits &= 0x7f;
  return (bits << 1) | (bits >> 6);
}

/* rectangles describe a bitmap portion */
typedef struct _rectangle rectangle;
struct _rectangle
{
  int min_x; /* minimum X, or left coordinate */
  int max_x; /* maximum X, or right coordinate (inclusive) */
  int min_y; /* minimum Y, or top coordinate */
  int max_y; /* maximum Y, or bottom coordinate (inclusive) */
};

/* Standard MIN/MAX macros */
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

inline float u2f(u32 v)
{
  union
  {
    float ff;
    u32 vv;
  } u;
  u.vv = v;
  return u.ff;
}

/* Macros for normalizing data into big or little endian formats */
#define FLIPENDIAN_INT16(x) (((((u16)(x)) >> 8) | ((x) << 8)) & 0xffff)
#define FLIPENDIAN_INT32(x)                                                                                            \
  ((((u32)(x)) << 24) | (((u32)(x)) >> 24) | ((((u32)(x)) & 0x0000ff00) << 8) | ((((u32)(x)) & 0x00ff0000) >> 8))
#define FLIPENDIAN_INT64(x)                                                                                            \
  ((((((u64)(x)) >> 56) & ((u64)0xFF)) << 0) | (((((u64)(x)) >> 48) & ((u64)0xFF)) << 8) |                             \
   (((((u64)(x)) >> 40) & ((u64)0xFF)) << 16) | (((((u64)(x)) >> 32) & ((u64)0xFF)) << 24) |                           \
   (((((u64)(x)) >> 24) & ((u64)0xFF)) << 32) | (((((u64)(x)) >> 16) & ((u64)0xFF)) << 40) |                           \
   (((((u64)(x)) >> 8) & ((u64)0xFF)) << 48) | (((((u64)(x)) >> 0) & ((u64)0xFF)) << 56))

#define ACCESSING_BITS_0_15 ((mem_mask & 0x0000ffff) != 0)
#define ACCESSING_BITS_16_31 ((mem_mask & 0xffff0000) != 0)

// constants for expression endianness
enum endianness_t
{
  ENDIANNESS_LITTLE,
  ENDIANNESS_BIG
};
const endianness_t ENDIANNESS_NATIVE = ENDIANNESS_LITTLE;
#define ENDIAN_VALUE_LE_BE(endian, leval, beval) (((endian) == ENDIANNESS_LITTLE) ? (leval) : (beval))
#define NATIVE_ENDIAN_VALUE_LE_BE(leval, beval) ENDIAN_VALUE_LE_BE(ENDIANNESS_NATIVE, leval, beval)
#define BYTE4_XOR_LE(a) ((a) ^ NATIVE_ENDIAN_VALUE_LE_BE(0, 3))

#define BYTE_XOR_LE(a) ((a) ^ NATIVE_ENDIAN_VALUE_LE_BE(0, 1))

#define profiler_mark_start(x)                                                                                         \
  do                                                                                                                   \
  {                                                                                                                    \
  } while (0)
#define profiler_mark_end()                                                                                            \
  do                                                                                                                   \
  {                                                                                                                    \
  } while (0)

/* Highly useful macro for compile-time knowledge of an array size */
#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))

inline s32 mul_32x32_shift(s32 a, s32 b, s8 shift)
{
  return static_cast<s32>((static_cast<s64>(a) * static_cast<s64>(b)) >> shift);
}

typedef void (*poly_draw_scanline_func)(void* dest, s32 scanline, const poly_extent* extent, const void* extradata);

inline rgb_t rgba_bilinear_filter(rgb_t rgb00, rgb_t rgb01, rgb_t rgb10, rgb_t rgb11, u8 u, u8 v)
{
  u32 ag0, ag1, rb0, rb1;

  rb0 = (rgb00 & 0x00ff00ff) + ((((rgb01 & 0x00ff00ff) - (rgb00 & 0x00ff00ff)) * u) >> 8);
  rb1 = (rgb10 & 0x00ff00ff) + ((((rgb11 & 0x00ff00ff) - (rgb10 & 0x00ff00ff)) * u) >> 8);
  rgb00 >>= 8;
  rgb01 >>= 8;
  rgb10 >>= 8;
  rgb11 >>= 8;
  ag0 = (rgb00 & 0x00ff00ff) + ((((rgb01 & 0x00ff00ff) - (rgb00 & 0x00ff00ff)) * u) >> 8);
  ag1 = (rgb10 & 0x00ff00ff) + ((((rgb11 & 0x00ff00ff) - (rgb10 & 0x00ff00ff)) * u) >> 8);

  rb0 = (rb0 & 0x00ff00ff) + ((((rb1 & 0x00ff00ff) - (rb0 & 0x00ff00ff)) * v) >> 8);
  ag0 = (ag0 & 0x00ff00ff) + ((((ag1 & 0x00ff00ff) - (ag0 & 0x00ff00ff)) * v) >> 8);

  return ((ag0 << 8) & 0xff00ff00) | (rb0 & 0x00ff00ff);
}

struct poly_vertex
{
  float x;                    /* X coordinate */
  float y;                    /* Y coordinate */
  float p[MAX_VERTEX_PARAMS]; /* interpolated parameter values */
};

struct tri_extent
{
  s16 startx; /* starting X coordinate (inclusive) */
  s16 stopx;  /* ending X coordinate (exclusive) */
};

/* tri_work_unit is a triangle-specific work-unit */
struct tri_work_unit
{
  tri_extent extent[8]; /* array of scanline extents */
};
