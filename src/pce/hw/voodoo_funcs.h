#pragma once
#include "common/display_timing.h"
#include "voodoo_regs.h"
#include "voodoo_types.h"

class Display;
class TimingEvent;
class TimingManager;

/*************************************
 *
 *  Misc. constants
 *
 *************************************/

/* enumeration specifying which model of Voodoo we are emulating */
enum
{
  VOODOO_1,
  VOODOO_1_DTMU,
  VOODOO_2,
  MAX_VOODOO_TYPES
};

/* maximum number of TMUs */
#define MAX_TMU 2

/* maximum number of rasterizers */
#define MAX_RASTERIZERS 1024

/* size of the rasterizer hash table */
#define RASTER_HASH_SIZE 97

/* flags for LFB writes */
#define LFB_RGB_PRESENT 1
#define LFB_ALPHA_PRESENT 2
#define LFB_DEPTH_PRESENT 4
#define LFB_DEPTH_PRESENT_MSW 8

/* lookup bits is the log2 of the size of the reciprocal/log table */
#define RECIPLOG_LOOKUP_BITS 9

/* input precision is how many fraction bits the input value has; this is a 64-bit number */
#define RECIPLOG_INPUT_PREC 32

/* lookup precision is how many fraction bits each table entry contains */
#define RECIPLOG_LOOKUP_PREC 22

/* output precision is how many fraction bits the result should have */
#define RECIP_OUTPUT_PREC 15
#define LOG_OUTPUT_PREC 8

/*************************************
 *
 *  Dithering tables
 *
 *************************************/

static const u8 dither_matrix_4x4[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

static const u8 dither_matrix_2x2[16] = {2, 10, 2, 10, 14, 6, 14, 6, 2, 10, 2, 10, 14, 6, 14, 6};

/*************************************
 *
 *  Macros for extracting pixels
 *
 *************************************/

#define EXTRACT_565_TO_888(val, a, b, c)                                                                               \
  (a) = (int)((((unsigned int)(val) >> 8u) & 0xf8u) | (((unsigned int)(val) >> 13u) & 0x07u));                         \
  (b) = (int)((((unsigned int)(val) >> 3u) & 0xfcu) | (((unsigned int)(val) >> 9u) & 0x03u));                          \
  (c) = (int)((((unsigned int)(val) << 3u) & 0xf8u) | (((unsigned int)(val) >> 2u) & 0x07u));

#define EXTRACT_x555_TO_888(val, a, b, c)                                                                              \
  (a) = (int)((((unsigned int)(val) >> 7u) & 0xf8u) | (((unsigned int)(val) >> 12u) & 0x07u));                         \
  (b) = (int)((((unsigned int)(val) >> 2u) & 0xf8u) | (((unsigned int)(val) >> 7u) & 0x07u));                          \
  (c) = (int)((((unsigned int)(val) << 3u) & 0xf8u) | (((unsigned int)(val) >> 2u) & 0x07u));

#define EXTRACT_555x_TO_888(val, a, b, c)                                                                              \
  (a) = (int)((((unsigned int)(val) >> 8u) & 0xf8u) | (((unsigned int)(val) >> 13u) & 0x07u));                         \
  (b) = (int)((((unsigned int)(val) >> 3u) & 0xf8u) | (((unsigned int)(val) >> 8u) & 0x07u));                          \
  (c) = (int)((((unsigned int)(val) << 2u) & 0xf8u) | (((unsigned int)(val) >> 3u) & 0x07u));

#define EXTRACT_1555_TO_8888(val, a, b, c, d)                                                                          \
  (a) = ((s16)(val) >> 15) & 0xff;                                                                                     \
  EXTRACT_x555_TO_888(val, b, c, d)

#define EXTRACT_5551_TO_8888(val, a, b, c, d)                                                                          \
  EXTRACT_555x_TO_888(val, a, b, c)(d) = (int)(((unsigned int)(val)&0x0001u) ? 0xffu : 0x00u);

#define EXTRACT_x888_TO_888(val, a, b, c)                                                                              \
  (a) = (int)(((unsigned int)(val) >> 16u) & 0xffu);                                                                   \
  (b) = (int)(((unsigned int)(val) >> 8u) & 0xffu);                                                                    \
  (c) = (int)(((unsigned int)(val) >> 0u) & 0xffu);

#define EXTRACT_888x_TO_888(val, a, b, c)                                                                              \
  (a) = (int)(((unsigned int)(val) >> 24u) & 0xffu);                                                                   \
  (b) = (int)(((unsigned int)(val) >> 16u) & 0xffu);                                                                   \
  (c) = (int)(((unsigned int)(val) >> 8u) & 0xffu);

#define EXTRACT_8888_TO_8888(val, a, b, c, d)                                                                          \
  (a) = (int)(((unsigned int)(val) >> 24u) & 0xffu);                                                                   \
  (b) = (int)(((unsigned int)(val) >> 16u) & 0xffu);                                                                   \
  (c) = (int)(((unsigned int)(val) >> 8u) & 0xffu);                                                                    \
  (d) = (int)(((unsigned int)(val) >> 0u) & 0xffu);

#define EXTRACT_4444_TO_8888(val, a, b, c, d)                                                                          \
  (a) = (int)((((unsigned int)(val) >> 8u) & 0xf0u) | (((unsigned int)(val) >> 12u) & 0x0fu));                         \
  (b) = (int)((((unsigned int)(val) >> 4u) & 0xf0u) | (((unsigned int)(val) >> 8u) & 0x0fu));                          \
  (c) = (int)((((unsigned int)(val) >> 0u) & 0xf0u) | (((unsigned int)(val) >> 4u) & 0x0fu));                          \
  (d) = (int)((((unsigned int)(val) << 4u) & 0xf0u) | (((unsigned int)(val) >> 0u) & 0x0fu));

#define EXTRACT_332_TO_888(val, a, b, c)                                                                               \
  (a) = (int)((((unsigned int)(val) >> 0u) & 0xe0u) | (((unsigned int)(val) >> 3u) & 0x1cu) |                          \
              (((unsigned int)(val) >> 6u) & 0x03u));                                                                  \
  (b) = (int)((((unsigned int)(val) << 3u) & 0xe0u) | (((unsigned int)(val) >> 0u) & 0x1cu) |                          \
              (((unsigned int)(val) >> 3u) & 0x03u));                                                                  \
  (c) = (int)((((unsigned int)(val) << 6u) & 0xc0u) | (((unsigned int)(val) << 4u) & 0x30u) |                          \
              (((unsigned int)(val) << 2u) & 0x0cu) | (((unsigned int)(val) << 0u) & 0x03u));

/*************************************
 *
 *  Misc. macros
 *
 *************************************/

/* macro for clamping a value between minimum and maximum values */
#define CLAMP(val, min, max)                                                                                           \
  do                                                                                                                   \
  {                                                                                                                    \
    if ((val) < (min))                                                                                                 \
    {                                                                                                                  \
      (val) = (min);                                                                                                   \
    }                                                                                                                  \
    else if ((val) > (max))                                                                                            \
    {                                                                                                                  \
      (val) = (max);                                                                                                   \
    }                                                                                                                  \
  } while (0)

/* macro to compute the base 2 log for LOD calculations */
#define LOGB2(x) (log((double)(x)) / log(2.0))

/*************************************
 *
 *  Core types
 *
 *************************************/

typedef u32 rgb_t;

typedef struct _rgba rgba;
#define LSB_FIRST
struct _rgba
{
#ifdef LSB_FIRST
#else
  u8 a, r, g, b;
#endif
};

union voodoo_reg
{
  s32 i;
  u32 u;
  float f;
  struct
  {
    // Assumes little-endian.
    u8 b, g, r, a;
  } rgb;
};

typedef voodoo_reg rgb_union;

/* note that this structure is an even 64 bytes long */
struct stats_block
{
  s32 pixels_in;          /* pixels in statistic */
  s32 pixels_out;         /* pixels out statistic */
  s32 chroma_fail;        /* chroma test fail statistic */
  s32 zfunc_fail;         /* z function test fail statistic */
  s32 afunc_fail;         /* alpha function test fail statistic */
  s32 clip_fail;          /* clipping fail statistic */
  s32 stipple_count;      /* stipple statistic */
  s32 filler[64 / 4 - 7]; /* pad this structure to 64 bytes */
};

struct fifo_state
{
  s32 size; /* size of the FIFO */
};

struct pci_state
{
  fifo_state fifo; /* PCI FIFO */
  u32 init_enable; /* initEnable value */
  bool op_pending; /* true if an operation is pending */
};

struct ncc_table
{
  bool dirty;              /* is the texel lookup dirty? */
  voodoo_reg* reg;         /* pointer to our registers */
  s32 ir[4], ig[4], ib[4]; /* I values for R,G,B */
  s32 qr[4], qg[4], qb[4]; /* Q values for R,G,B */
  s32 y[16];               /* Y values */
  rgb_t* palette;          /* pointer to associated RGB palette */
  rgb_t* palettea;         /* pointer to associated ARGB palette */
  rgb_t texel[256];        /* texel lookup */
};

struct tmu_state
{
  u8* ram;         /* pointer to our RAM */
  u32 mask;        /* mask to apply to pointers */
  voodoo_reg* reg; /* pointer to our register base */
  bool regdirty;   /* true if the LOD/mode/base registers have changed */

  u32 texaddr_mask; /* mask for texture address */
  u8 texaddr_shift; /* shift for texture address */

  s64 starts, startt; /* starting S,T (14.18) */
  s64 startw;         /* starting W (2.30) */
  s64 dsdx, dtdx;     /* delta S,T per X */
  s64 dwdx;           /* delta W per X */
  s64 dsdy, dtdy;     /* delta S,T per Y */
  s64 dwdy;           /* delta W per Y */

  s32 lodmin, lodmax; /* min, max LOD values */
  s32 lodbias;        /* LOD bias */
  u32 lodmask;        /* mask of available LODs */
  u32 lodoffset[9];   /* offset of texture base for each LOD */
  s32 detailmax;      /* detail clamp */
  s32 detailbias;     /* detail bias */
  u8 detailscale;     /* detail scale */

  u32 wmask; /* mask for the current texture width */
  u32 hmask; /* mask for the current texture height */

  u8 bilinear_mask; /* mask for bilinear resolution (0xf0 for V1, 0xff for V2) */

  ncc_table ncc[2]; /* two NCC tables */

  rgb_t* lookup;    /* currently selected lookup */
  rgb_t* texel[16]; /* texel lookups for each format */

  rgb_t palette[256];  /* palette lookup table */
  rgb_t palettea[256]; /* palette+alpha lookup table */
};

struct tmu_shared_state
{
  rgb_t rgb332[256]; /* RGB 3-3-2 lookup table */
  rgb_t alpha8[256]; /* alpha 8-bit lookup table */
  rgb_t int8[256];   /* intensity 8-bit lookup table */
  rgb_t ai44[256];   /* alpha, intensity 4-4 lookup table */

  rgb_t rgb565[65536];   /* RGB 5-6-5 lookup table */
  rgb_t argb1555[65536]; /* ARGB 1-5-5-5 lookup table */
  rgb_t argb4444[65536]; /* ARGB 4-4-4-4 lookup table */
};

struct setup_vertex
{
  float x, y;       /* X, Y coordinates */
  float a, r, g, b; /* A, R, G, B values */
  float z, wb;      /* Z and broadcast W values */
  float w0, s0, t0; /* W, S, T for TMU 0 */
  float w1, s1, t1; /* W, S, T for TMU 1 */
};

struct fbi_state
{
  u8* ram;        /* pointer to frame buffer RAM */
  u32 mask;       /* mask to apply to pointers */
  u32 rgboffs[3]; /* word offset to 3 RGB buffers */
  u32 auxoffs;    /* word offset to 1 aux buffer */

  u8 frontbuf; /* front buffer index */
  u8 backbuf;  /* back buffer index */

  u32 yorigin; /* Y origin subtract value */

  u32 width;       /* width of current frame buffer */
  u32 height;      /* height of current frame buffer */
                   //	u32				xoffs;					/* horizontal offset (back porch) */
                   //	u32				yoffs;					/* vertical offset (back porch) */
                   //	u32				vsyncscan;				/* vertical sync scanline */
  u32 rowpixels;   /* pixels per row */
  u32 tile_width;  /* width of video tiles */
  u32 tile_height; /* height of video tiles */
  u32 x_tiles;     /* number of tiles in the X direction */

  u8 vblank;             /* VBLANK state */
  bool vblank_dont_swap; /* don't actually swap when we hit this point */

  /* triangle setup info */
  s16 ax, ay;                         /* vertex A x,y (12.4) */
  s16 bx, by;                         /* vertex B x,y (12.4) */
  s16 cx, cy;                         /* vertex C x,y (12.4) */
  s32 startr, startg, startb, starta; /* starting R,G,B,A (12.12) */
  s32 startz;                         /* starting Z (20.12) */
  s64 startw;                         /* starting W (16.32) */
  s32 drdx, dgdx, dbdx, dadx;         /* delta R,G,B,A per X */
  s32 dzdx;                           /* delta Z per X */
  s64 dwdx;                           /* delta W per X */
  s32 drdy, dgdy, dbdy, dady;         /* delta R,G,B,A per Y */
  s32 dzdy;                           /* delta Z per Y */
  s64 dwdy;                           /* delta W per Y */

  stats_block lfb_stats; /* LFB-access statistics */

  u8 sverts;             /* number of vertices ready */
  setup_vertex svert[3]; /* 3 setup vertices */

  fifo_state fifo; /* framebuffer memory fifo */

  u8 fogblend[64];  /* 64-entry fog table */
  u8 fogdelta[64];  /* 64-entry fog table */
  u8 fogdelta_mask; /* mask for for delta (0xff for V1, 0xfc for V2) */

  //	rgb_t				clut[512];				/* clut gamma data */
};

struct dac_state
{
  u8 reg[8];      /* 8 registers */
  u8 read_result; /* pending read result */
};

struct raster_info
{
  struct raster_info* next;         /* pointer to next entry with the same hash */
  poly_draw_scanline_func callback; /* callback pointer */
  bool is_generic;                  /* true if this is one of the generic rasterizers */
  u8 display;                       /* display index */
  u32 hits;                         /* how many hits (pixels) we've used this for */
  u32 polys;                        /* how many polys we've used this for */
  u32 eff_color_path;               /* effective fbzColorPath value */
  u32 eff_alpha_mode;               /* effective alphaMode value */
  u32 eff_fog_mode;                 /* effective fogMode value */
  u32 eff_fbz_mode;                 /* effective fbzMode value */
  u32 eff_tex_mode_0;               /* effective textureMode value for TMU #0 */
  u32 eff_tex_mode_1;               /* effective textureMode value for TMU #1 */
};

struct poly_extra_data
{
  voodoo_state* state; /* pointer back to the voodoo state */
  raster_info* info;   /* pointer to rasterizer information */

  s16 ax, ay;                         /* vertex A x,y (12.4) */
  s32 startr, startg, startb, starta; /* starting R,G,B,A (12.12) */
  s32 startz;                         /* starting Z (20.12) */
  s64 startw;                         /* starting W (16.32) */
  s32 drdx, dgdx, dbdx, dadx;         /* delta R,G,B,A per X */
  s32 dzdx;                           /* delta Z per X */
  s64 dwdx;                           /* delta W per X */
  s32 drdy, dgdy, dbdy, dady;         /* delta R,G,B,A per Y */
  s32 dzdy;                           /* delta Z per Y */
  s64 dwdy;                           /* delta W per Y */

  s64 starts0, startt0; /* starting S,T (14.18) */
  s64 startw0;          /* starting W (2.30) */
  s64 ds0dx, dt0dx;     /* delta S,T per X */
  s64 dw0dx;            /* delta W per X */
  s64 ds0dy, dt0dy;     /* delta S,T per Y */
  s64 dw0dy;            /* delta W per Y */
  s32 lodbase0;         /* used during rasterization */

  s64 starts1, startt1; /* starting S,T (14.18) */
  s64 startw1;          /* starting W (2.30) */
  s64 ds1dx, dt1dx;     /* delta S,T per X */
  s64 dw1dx;            /* delta W per X */
  s64 ds1dy, dt1dy;     /* delta S,T per Y */
  s64 dw1dy;            /* delta W per Y */
  s32 lodbase1;         /* used during rasterization */

  u16 dither[16]; /* dither matrix, for fastfill */

  u32 texcount;
  u32 r_fbzColorPath;
  u32 r_fbzMode;
  u32 r_alphaMode;
  u32 r_fogMode;
  s32 r_textureMode0;
  s32 r_textureMode1;
};

struct voodoo_state
{
  u8 type;     /* type of system */
  u8 chipmask; /* mask for which chips are available */

  voodoo_reg reg[0x400];       /* raw registers */
  const u8* regaccess;         /* register access array */
  const char* const* regnames; /* register names array */
  bool alt_regmap;             /* enable alternate register map? */

  pci_state pci; /* PCI state */
  dac_state dac; /* DAC state */

  fbi_state fbi;             /* FBI states */
  tmu_state tmu[MAX_TMU];    /* TMU states */
  tmu_shared_state tmushare; /* TMU shared state */

  stats_block* thread_stats; /* per-thread statistics */

  int next_rasterizer;                        /* next rasterizer index */
  raster_info rasterizer[MAX_RASTERIZERS];    /* array of rasterizers */
  raster_info* raster_hash[RASTER_HASH_SIZE]; /* hash table of rasterizers */

  bool send_config;
  u32 tmu_config;

  bool clock_enabled;
  bool output_on;

  Display* display;
  DisplayTiming display_timing;
  TimingManager* timing_manager;
  TimingEvent* retrace_event;
};

/*************************************
 *
 *  Inline FIFO management
 *
 *************************************/

inline s32 fifo_space(fifo_state* f)
{
  s32 items = 0;
  if (items < 0)
    items += f->size;
  return f->size - 1 - items;
}

inline u8 count_leading_zeros(u32 value)
{
  s32 result = 32;
  while (value > 0)
  {
    result--;
    value >>= 1;
  }
  return (u8)result;
}

/*************************************
 *
 *  Computes a fast 16.16 reciprocal
 *  of a 16.32 value; used for
 *  computing 1/w in the rasterizer.
 *
 *  Since it is trivial to also
 *  compute log2(1/w) = -log2(w) at
 *  the same time, we do that as well
 *  to 16.8 precision for LOD
 *  calculations.
 *
 *  On a Pentium M, this routine is
 *  20% faster than a 64-bit integer
 *  divide and also produces the log
 *  for free.
 *
 *************************************/

inline s64 fast_reciplog(s64 value, s32* log2)
{
  extern u32 voodoo_reciplog[];
  u32 temp, rlog;
  u32 interp;
  u32* table;
  u64 recip;
  bool neg = false;
  int lz, exp = 0;

  /* always work with unsigned numbers */
  if (value < 0)
  {
    value = -value;
    neg = true;
  }

  /* if we've spilled out of 32 bits, push it down under 32 */
  if (value & INT64_C(0xffff00000000))
  {
    temp = (u32)(value >> 16);
    exp -= 16;
  }
  else
    temp = (u32)value;

  /* if the resulting value is 0, the reciprocal is infinite */
  if (temp == 0)
  {
    *log2 = 1000 << LOG_OUTPUT_PREC;
    return neg ? 0x80000000 : 0x7fffffff;
  }

  /* determine how many leading zeros in the value and shift it up high */
  lz = count_leading_zeros(temp);
  temp <<= lz;
  exp += lz;

  /* compute a pointer to the table entries we want */
  /* math is a bit funny here because we shift one less than we need to in order */
  /* to account for the fact that there are two u32's per table entry */
  table = &voodoo_reciplog[(temp >> (31 - RECIPLOG_LOOKUP_BITS - 1)) & ((2 << RECIPLOG_LOOKUP_BITS) - 2)];

  /* compute the interpolation value */
  interp = (temp >> (31 - RECIPLOG_LOOKUP_BITS - 8)) & 0xff;

  /* do a linear interpolatation between the two nearest table values */
  /* for both the log and the reciprocal */
  rlog = (table[1] * (0x100 - interp) + table[3] * interp) >> 8;
  recip = (table[0] * (0x100 - interp) + table[2] * interp) >> 8;

  /* the log result is the fractional part of the log; round it to the output precision */
  rlog = (rlog + (1 << (RECIPLOG_LOOKUP_PREC - LOG_OUTPUT_PREC - 1))) >> (RECIPLOG_LOOKUP_PREC - LOG_OUTPUT_PREC);

  /* the exponent is the non-fractional part of the log; normally, we would subtract it from rlog */
  /* but since we want the log(1/value) = -log(value), we subtract rlog from the exponent */
  *log2 = (s32)((((int)exp - ((int)31 - (int)RECIPLOG_INPUT_PREC)) << (int)LOG_OUTPUT_PREC) - (int)rlog);

  /* adjust the exponent to account for all the reciprocal-related parameters to arrive at a final shift amount */
  exp += (RECIP_OUTPUT_PREC - RECIPLOG_LOOKUP_PREC) - (31 - RECIPLOG_INPUT_PREC);

  /* shift by the exponent */
  if (exp < 0)
    recip >>= -exp;
  else
    recip <<= exp;

  /* on the way out, apply the original sign to the reciprocal */
  return neg ? -(s64)recip : (s64)recip;
}

/*************************************
 *
 *  Float-to-int conversions
 *
 *************************************/

inline s32 float_to_int32(u32 data, int fixedbits)
{
  int exponent = ((int)((data >> 23u) & 0xffu) - 127 - 23 + fixedbits);
  s32 result = (s32)(data & 0x7fffff) | 0x800000;
  if (exponent < 0)
  {
    if (exponent > -32)
      result >>= -exponent;
    else
      result = 0;
  }
  else
  {
    if (exponent < 32)
      result <<= exponent;
    else
      result = 0x7fffffff;
  }
  if (data & 0x80000000)
    result = -result;
  return result;
}

inline s64 float_to_int64(u32 data, int fixedbits)
{
  int exponent = (int)((data >> 23) & 0xff) - 127 - 23 + fixedbits;
  s64 result = (s64)(data & 0x7fffff) | 0x800000;
  if (exponent < 0)
  {
    if (exponent > -64)
      result >>= -exponent;
    else
      result = 0;
  }
  else
  {
    if (exponent < 64)
      result <<= exponent;
    else
      result = INT64_C(0x7fffffffffffffff);
  }
  if (data & 0x80000000)
    result = -result;
  return result;
}

/*************************************
 *
 *  Rasterizer inlines
 *
 *************************************/

inline u32 normalize_color_path(u32 eff_color_path)
{
  /* ignore the subpixel adjust and texture enable flags */
  eff_color_path &= ~((1u << 26u) | (1u << 27u));

  return eff_color_path;
}

inline u32 normalize_alpha_mode(u32 eff_alpha_mode)
{
  /* always ignore alpha ref value */
  eff_alpha_mode &= ~(0xffu << 24u);

  /* if not doing alpha testing, ignore the alpha function and ref value */
  if (!ALPHAMODE_ALPHATEST(eff_alpha_mode))
    eff_alpha_mode &= ~(7u << 1u);

  /* if not doing alpha blending, ignore the source and dest blending factors */
  if (!ALPHAMODE_ALPHABLEND(eff_alpha_mode))
    eff_alpha_mode &= ~((15u << 8u) | (15u << 12u) | (15u << 16u) | (15u << 20u));

  return eff_alpha_mode;
}

inline u32 normalize_fog_mode(u32 eff_fog_mode)
{
  /* if not doing fogging, ignore all the other fog bits */
  if (!FOGMODE_ENABLE_FOG(eff_fog_mode))
    eff_fog_mode = 0;

  return eff_fog_mode;
}

inline u32 normalize_fbz_mode(u32 eff_fbz_mode)
{
  /* ignore the draw buffer */
  eff_fbz_mode &= ~(3u << 14u);

  return eff_fbz_mode;
}

inline u32 normalize_tex_mode(u32 eff_tex_mode)
{
  /* ignore the NCC table and seq_8_downld flags */
  eff_tex_mode &= ~((1u << 5u) | (1u << 31u));

  /* classify texture formats into 3 format categories */
  if (TEXMODE_FORMAT(eff_tex_mode) < 8)
    eff_tex_mode = (eff_tex_mode & ~(0xfu << 8u)) | (0u << 8u);
  else if (TEXMODE_FORMAT(eff_tex_mode) >= 10 && TEXMODE_FORMAT(eff_tex_mode) <= 12)
    eff_tex_mode = (eff_tex_mode & ~(0xfu << 8u)) | (10u << 8u);
  else
    eff_tex_mode = (eff_tex_mode & ~(0xfu << 8u)) | (8u << 8u);

  return eff_tex_mode;
}

inline u32 compute_raster_hash(const raster_info* info)
{
  u32 hash;

  /* make a hash */
  hash = info->eff_color_path;
  hash = (hash << 1u) | (hash >> 31u);
  hash ^= info->eff_fbz_mode;
  hash = (hash << 1u) | (hash >> 31u);
  hash ^= info->eff_alpha_mode;
  hash = (hash << 1u) | (hash >> 31u);
  hash ^= info->eff_fog_mode;
  hash = (hash << 1u) | (hash >> 31u);
  hash ^= info->eff_tex_mode_0;
  hash = (hash << 1u) | (hash >> 31u);
  hash ^= info->eff_tex_mode_1;

  return hash % RASTER_HASH_SIZE;
}

/*************************************
 *
 *  Dithering macros
 *
 *************************************/

/* note that these equations and the dither matrixes have
   been confirmed to be exact matches to the real hardware */
#define DITHER_RB(val, dith) ((((val) << 1) - ((val) >> 4) + ((val) >> 7) + (dith)) >> 1)
#define DITHER_G(val, dith) ((((val) << 2) - ((val) >> 4) + ((val) >> 6) + (dith)) >> 2)

#define DECLARE_DITHER_POINTERS                                                                                        \
  const u8* dither_lookup = NULL;                                                                                      \
  const u8* dither4 = NULL;                                                                                            \
  const u8* dither = NULL

#define DECLARE_DITHER_POINTERS_NO_DITHER_VAR const u8* dither_lookup = NULL;

#define COMPUTE_DITHER_POINTERS(FBZMODE, YY)                                                                           \
  do                                                                                                                   \
  {                                                                                                                    \
    /* compute the dithering pointers */                                                                               \
    if (FBZMODE_ENABLE_DITHERING(FBZMODE))                                                                             \
    {                                                                                                                  \
      dither4 = &dither_matrix_4x4[((YY)&3) * 4];                                                                      \
      if (FBZMODE_DITHER_TYPE(FBZMODE) == 0)                                                                           \
      {                                                                                                                \
        dither = dither4;                                                                                              \
        dither_lookup = &dither4_lookup[(YY & 3) << 11];                                                               \
      }                                                                                                                \
      else                                                                                                             \
      {                                                                                                                \
        dither = &dither_matrix_2x2[((YY)&3) * 4];                                                                     \
        dither_lookup = &dither2_lookup[(YY & 3) << 11];                                                               \
      }                                                                                                                \
    }                                                                                                                  \
  } while (0)

#define COMPUTE_DITHER_POINTERS_NO_DITHER_VAR(FBZMODE, YY)                                                             \
  do                                                                                                                   \
  {                                                                                                                    \
    /* compute the dithering pointers */                                                                               \
    if (FBZMODE_ENABLE_DITHERING(FBZMODE))                                                                             \
    {                                                                                                                  \
      if (FBZMODE_DITHER_TYPE(FBZMODE) == 0)                                                                           \
      {                                                                                                                \
        dither_lookup = &dither4_lookup[(YY & 3) << 11];                                                               \
      }                                                                                                                \
      else                                                                                                             \
      {                                                                                                                \
        dither_lookup = &dither2_lookup[(YY & 3) << 11];                                                               \
      }                                                                                                                \
    }                                                                                                                  \
  } while (0)

#define APPLY_DITHER(FBZMODE, XX, DITHER_LOOKUP, RR, GG, BB)                                                           \
  do                                                                                                                   \
  {                                                                                                                    \
    /* apply dithering */                                                                                              \
    if (FBZMODE_ENABLE_DITHERING(FBZMODE))                                                                             \
    {                                                                                                                  \
      /* look up the dither value from the appropriate matrix */                                                       \
      const u8* dith = &DITHER_LOOKUP[((XX)&3) << 1];                                                                  \
                                                                                                                       \
      /* apply dithering to R,G,B */                                                                                   \
      (RR) = dith[((RR) << 3) + 0];                                                                                    \
      (GG) = dith[((GG) << 3) + 1];                                                                                    \
      (BB) = dith[((BB) << 3) + 0];                                                                                    \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      (RR) >>= 3;                                                                                                      \
      (GG) >>= 2;                                                                                                      \
      (BB) >>= 3;                                                                                                      \
    }                                                                                                                  \
  } while (0)

  /*************************************
   *
   *  Clamping macros
   *
   *************************************/

#define CLAMPED_ARGB(ITERR, ITERG, ITERB, ITERA, FBZCP, RESULT)                                                        \
  do                                                                                                                   \
  {                                                                                                                    \
    r = (s32)(ITERR) >> 12;                                                                                            \
    g = (s32)(ITERG) >> 12;                                                                                            \
    b = (s32)(ITERB) >> 12;                                                                                            \
    a = (s32)(ITERA) >> 12;                                                                                            \
                                                                                                                       \
    if (FBZCP_RGBZW_CLAMP(FBZCP) == 0)                                                                                 \
    {                                                                                                                  \
      r &= 0xfff;                                                                                                      \
      RESULT.rgb.r = r;                                                                                                \
      if (r == 0xfff)                                                                                                  \
        RESULT.rgb.r = 0;                                                                                              \
      else if (r == 0x100)                                                                                             \
        RESULT.rgb.r = 0xff;                                                                                           \
                                                                                                                       \
      g &= 0xfff;                                                                                                      \
      RESULT.rgb.g = g;                                                                                                \
      if (g == 0xfff)                                                                                                  \
        RESULT.rgb.g = 0;                                                                                              \
      else if (g == 0x100)                                                                                             \
        RESULT.rgb.g = 0xff;                                                                                           \
                                                                                                                       \
      b &= 0xfff;                                                                                                      \
      RESULT.rgb.b = b;                                                                                                \
      if (b == 0xfff)                                                                                                  \
        RESULT.rgb.b = 0;                                                                                              \
      else if (b == 0x100)                                                                                             \
        RESULT.rgb.b = 0xff;                                                                                           \
                                                                                                                       \
      a &= 0xfff;                                                                                                      \
      RESULT.rgb.a = a;                                                                                                \
      if (a == 0xfff)                                                                                                  \
        RESULT.rgb.a = 0;                                                                                              \
      else if (a == 0x100)                                                                                             \
        RESULT.rgb.a = 0xff;                                                                                           \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      RESULT.rgb.r = (r < 0) ? 0 : (r > 0xff) ? 0xff : (u8)r;                                                          \
      RESULT.rgb.g = (g < 0) ? 0 : (g > 0xff) ? 0xff : (u8)g;                                                          \
      RESULT.rgb.b = (b < 0) ? 0 : (b > 0xff) ? 0xff : (u8)b;                                                          \
      RESULT.rgb.a = (a < 0) ? 0 : (a > 0xff) ? 0xff : (u8)a;                                                          \
    }                                                                                                                  \
  } while (0)

#define CLAMPED_Z(ITERZ, FBZCP, RESULT)                                                                                \
  do                                                                                                                   \
  {                                                                                                                    \
    (RESULT) = (s32)(ITERZ) >> 12;                                                                                     \
    if (FBZCP_RGBZW_CLAMP(FBZCP) == 0)                                                                                 \
    {                                                                                                                  \
      (RESULT) &= 0xfffff;                                                                                             \
      if ((RESULT) == 0xfffff)                                                                                         \
        (RESULT) = 0;                                                                                                  \
      else if ((RESULT) == 0x10000)                                                                                    \
        (RESULT) = 0xffff;                                                                                             \
      else                                                                                                             \
        (RESULT) &= 0xffff;                                                                                            \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      CLAMP((RESULT), 0, 0xffff);                                                                                      \
    }                                                                                                                  \
  } while (0)

#define CLAMPED_W(ITERW, FBZCP, RESULT)                                                                                \
  do                                                                                                                   \
  {                                                                                                                    \
    (RESULT) = (s16)((ITERW) >> 32);                                                                                   \
    if (FBZCP_RGBZW_CLAMP(FBZCP) == 0)                                                                                 \
    {                                                                                                                  \
      (RESULT) &= 0xffff;                                                                                              \
      if ((RESULT) == 0xffff)                                                                                          \
        (RESULT) = 0;                                                                                                  \
      else if ((RESULT) == 0x100)                                                                                      \
        (RESULT) = 0xff;                                                                                               \
      (RESULT) &= 0xff;                                                                                                \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      CLAMP((RESULT), 0, 0xff);                                                                                        \
    }                                                                                                                  \
  } while (0)

  /*************************************
   *
   *  Chroma keying macro
   *
   *************************************/

#define APPLY_CHROMAKEY(VV, STATS, FBZMODE, COLOR)                                                                     \
  do                                                                                                                   \
  {                                                                                                                    \
    if (FBZMODE_ENABLE_CHROMAKEY(FBZMODE))                                                                             \
    {                                                                                                                  \
      /* non-range version */                                                                                          \
      if (!CHROMARANGE_ENABLE((VV)->reg[chromaRange].u))                                                               \
      {                                                                                                                \
        if (((COLOR.u ^ (VV)->reg[chromaKey].u) & 0xffffff) == 0)                                                      \
        {                                                                                                              \
          (STATS)->chroma_fail++;                                                                                      \
          goto skipdrawdepth;                                                                                          \
        }                                                                                                              \
      }                                                                                                                \
                                                                                                                       \
      /* tricky range version */                                                                                       \
      else                                                                                                             \
      {                                                                                                                \
        s32 low, high, test;                                                                                           \
        int results = 0;                                                                                               \
                                                                                                                       \
        /* check blue */                                                                                               \
        low = (VV)->reg[chromaKey].rgb.b;                                                                              \
        high = (VV)->reg[chromaRange].rgb.b;                                                                           \
        test = COLOR.rgb.b;                                                                                            \
        results = (test >= low && test <= high);                                                                       \
        results ^= (int)(CHROMARANGE_BLUE_EXCLUSIVE((VV)->reg[chromaRange].u));                                        \
        results <<= 1;                                                                                                 \
                                                                                                                       \
        /* check green */                                                                                              \
        low = (VV)->reg[chromaKey].rgb.g;                                                                              \
        high = (VV)->reg[chromaRange].rgb.g;                                                                           \
        test = COLOR.rgb.g;                                                                                            \
        results |= (test >= low && test <= high);                                                                      \
        results ^= (int)(CHROMARANGE_GREEN_EXCLUSIVE((VV)->reg[chromaRange].u));                                       \
        results <<= 1;                                                                                                 \
                                                                                                                       \
        /* check red */                                                                                                \
        low = (VV)->reg[chromaKey].rgb.r;                                                                              \
        high = (VV)->reg[chromaRange].rgb.r;                                                                           \
        test = COLOR.rgb.r;                                                                                            \
        results |= (test >= low && test <= high);                                                                      \
        results ^= (int)(CHROMARANGE_RED_EXCLUSIVE((VV)->reg[chromaRange].u));                                         \
                                                                                                                       \
        /* final result */                                                                                             \
        if (CHROMARANGE_UNION_MODE((VV)->reg[chromaRange].u))                                                          \
        {                                                                                                              \
          if (results != 0)                                                                                            \
          {                                                                                                            \
            (STATS)->chroma_fail++;                                                                                    \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
          if (results == 7)                                                                                            \
          {                                                                                                            \
            (STATS)->chroma_fail++;                                                                                    \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
        }                                                                                                              \
      }                                                                                                                \
    }                                                                                                                  \
  } while (0)

  /*************************************
   *
   *  Alpha masking macro
   *
   *************************************/

#define APPLY_ALPHAMASK(VV, STATS, FBZMODE, AA)                                                                        \
  do                                                                                                                   \
  {                                                                                                                    \
    if (FBZMODE_ENABLE_ALPHA_MASK(FBZMODE))                                                                            \
    {                                                                                                                  \
      if (((AA)&1) == 0)                                                                                               \
      {                                                                                                                \
        (STATS)->afunc_fail++;                                                                                         \
        goto skipdrawdepth;                                                                                            \
      }                                                                                                                \
    }                                                                                                                  \
  } while (0)

  /*************************************
   *
   *  Alpha testing macro
   *
   *************************************/

#define APPLY_ALPHATEST(VV, STATS, ALPHAMODE, AA)                                                                      \
  do                                                                                                                   \
  {                                                                                                                    \
    if (ALPHAMODE_ALPHATEST(ALPHAMODE))                                                                                \
    {                                                                                                                  \
      u8 alpharef = (VV)->reg[alphaMode].rgb.a;                                                                        \
      switch (ALPHAMODE_ALPHAFUNCTION(ALPHAMODE))                                                                      \
      {                                                                                                                \
        case 0: /* alphaOP = never */                                                                                  \
          (STATS)->afunc_fail++;                                                                                       \
          goto skipdrawdepth;                                                                                          \
                                                                                                                       \
        case 1: /* alphaOP = less than */                                                                              \
          if ((AA) >= alpharef)                                                                                        \
          {                                                                                                            \
            (STATS)->afunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 2: /* alphaOP = equal */                                                                                  \
          if ((AA) != alpharef)                                                                                        \
          {                                                                                                            \
            (STATS)->afunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 3: /* alphaOP = less than or equal */                                                                     \
          if ((AA) > alpharef)                                                                                         \
          {                                                                                                            \
            (STATS)->afunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 4: /* alphaOP = greater than */                                                                           \
          if ((AA) <= alpharef)                                                                                        \
          {                                                                                                            \
            (STATS)->afunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 5: /* alphaOP = not equal */                                                                              \
          if ((AA) == alpharef)                                                                                        \
          {                                                                                                            \
            (STATS)->afunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 6: /* alphaOP = greater than or equal */                                                                  \
          if ((AA) < alpharef)                                                                                         \
          {                                                                                                            \
            (STATS)->afunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 7: /* alphaOP = always */                                                                                 \
          break;                                                                                                       \
      }                                                                                                                \
    }                                                                                                                  \
  } while (0)

  /*************************************
   *
   *  Alpha blending macro
   *
   *************************************/

#define APPLY_ALPHA_BLEND(FBZMODE, ALPHAMODE, XX, DITHER, RR, GG, BB, AA)                                              \
  do                                                                                                                   \
  {                                                                                                                    \
    if (ALPHAMODE_ALPHABLEND(ALPHAMODE))                                                                               \
    {                                                                                                                  \
      int dpix = dest[XX];                                                                                             \
      int dr, dg, db;                                                                                                  \
      EXTRACT_565_TO_888(dpix, dr, dg, db);                                                                            \
      int da = (FBZMODE_ENABLE_ALPHA_PLANES(FBZMODE) && depth) ? depth[XX] : 0xff;                                     \
      int sr = (RR);                                                                                                   \
      int sg = (GG);                                                                                                   \
      int sb = (BB);                                                                                                   \
      int sa = (AA);                                                                                                   \
      int ta;                                                                                                          \
                                                                                                                       \
      /* apply dither subtraction */                                                                                   \
      if ((FBZMODE_ALPHA_DITHER_SUBTRACT(FBZMODE)) && DITHER)                                                          \
      {                                                                                                                \
        /* look up the dither value from the appropriate matrix */                                                     \
        int dith = DITHER[(XX)&3];                                                                                     \
                                                                                                                       \
        /* subtract the dither value */                                                                                \
        dr = ((dr << 1) + 15 - dith) >> 1;                                                                             \
        dg = ((dg << 2) + 15 - dith) >> 2;                                                                             \
        db = ((db << 1) + 15 - dith) >> 1;                                                                             \
      }                                                                                                                \
                                                                                                                       \
      /* compute source portion */                                                                                     \
      switch (ALPHAMODE_SRCRGBBLEND(ALPHAMODE))                                                                        \
      {                                                                                                                \
        default: /* reserved */                                                                                        \
        case 0:  /* AZERO */                                                                                           \
          (RR) = (GG) = (BB) = 0;                                                                                      \
          break;                                                                                                       \
                                                                                                                       \
        case 1: /* ASRC_ALPHA */                                                                                       \
          (RR) = (sr * (sa + 1)) >> 8;                                                                                 \
          (GG) = (sg * (sa + 1)) >> 8;                                                                                 \
          (BB) = (sb * (sa + 1)) >> 8;                                                                                 \
          break;                                                                                                       \
                                                                                                                       \
        case 2: /* A_COLOR */                                                                                          \
          (RR) = (sr * (dr + 1)) >> 8;                                                                                 \
          (GG) = (sg * (dg + 1)) >> 8;                                                                                 \
          (BB) = (sb * (db + 1)) >> 8;                                                                                 \
          break;                                                                                                       \
                                                                                                                       \
        case 3: /* ADST_ALPHA */                                                                                       \
          (RR) = (sr * (da + 1)) >> 8;                                                                                 \
          (GG) = (sg * (da + 1)) >> 8;                                                                                 \
          (BB) = (sb * (da + 1)) >> 8;                                                                                 \
          break;                                                                                                       \
                                                                                                                       \
        case 4: /* AONE */                                                                                             \
          break;                                                                                                       \
                                                                                                                       \
        case 5: /* AOMSRC_ALPHA */                                                                                     \
          (RR) = (sr * (0x100 - sa)) >> 8;                                                                             \
          (GG) = (sg * (0x100 - sa)) >> 8;                                                                             \
          (BB) = (sb * (0x100 - sa)) >> 8;                                                                             \
          break;                                                                                                       \
                                                                                                                       \
        case 6: /* AOM_COLOR */                                                                                        \
          (RR) = (sr * (0x100 - dr)) >> 8;                                                                             \
          (GG) = (sg * (0x100 - dg)) >> 8;                                                                             \
          (BB) = (sb * (0x100 - db)) >> 8;                                                                             \
          break;                                                                                                       \
                                                                                                                       \
        case 7: /* AOMDST_ALPHA */                                                                                     \
          (RR) = (sr * (0x100 - da)) >> 8;                                                                             \
          (GG) = (sg * (0x100 - da)) >> 8;                                                                             \
          (BB) = (sb * (0x100 - da)) >> 8;                                                                             \
          break;                                                                                                       \
                                                                                                                       \
        case 15: /* ASATURATE */                                                                                       \
          ta = (sa < (0x100 - da)) ? sa : (0x100 - da);                                                                \
          (RR) = (sr * (ta + 1)) >> 8;                                                                                 \
          (GG) = (sg * (ta + 1)) >> 8;                                                                                 \
          (BB) = (sb * (ta + 1)) >> 8;                                                                                 \
          break;                                                                                                       \
      }                                                                                                                \
                                                                                                                       \
      /* add in dest portion */                                                                                        \
      switch (ALPHAMODE_DSTRGBBLEND(ALPHAMODE))                                                                        \
      {                                                                                                                \
        default: /* reserved */                                                                                        \
        case 0:  /* AZERO */                                                                                           \
          break;                                                                                                       \
                                                                                                                       \
        case 1: /* ASRC_ALPHA */                                                                                       \
          (RR) += (dr * (sa + 1)) >> 8;                                                                                \
          (GG) += (dg * (sa + 1)) >> 8;                                                                                \
          (BB) += (db * (sa + 1)) >> 8;                                                                                \
          break;                                                                                                       \
                                                                                                                       \
        case 2: /* A_COLOR */                                                                                          \
          (RR) += (dr * (sr + 1)) >> 8;                                                                                \
          (GG) += (dg * (sg + 1)) >> 8;                                                                                \
          (BB) += (db * (sb + 1)) >> 8;                                                                                \
          break;                                                                                                       \
                                                                                                                       \
        case 3: /* ADST_ALPHA */                                                                                       \
          (RR) += (dr * (da + 1)) >> 8;                                                                                \
          (GG) += (dg * (da + 1)) >> 8;                                                                                \
          (BB) += (db * (da + 1)) >> 8;                                                                                \
          break;                                                                                                       \
                                                                                                                       \
        case 4: /* AONE */                                                                                             \
          (RR) += dr;                                                                                                  \
          (GG) += dg;                                                                                                  \
          (BB) += db;                                                                                                  \
          break;                                                                                                       \
                                                                                                                       \
        case 5: /* AOMSRC_ALPHA */                                                                                     \
          (RR) += (dr * (0x100 - sa)) >> 8;                                                                            \
          (GG) += (dg * (0x100 - sa)) >> 8;                                                                            \
          (BB) += (db * (0x100 - sa)) >> 8;                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 6: /* AOM_COLOR */                                                                                        \
          (RR) += (dr * (0x100 - sr)) >> 8;                                                                            \
          (GG) += (dg * (0x100 - sg)) >> 8;                                                                            \
          (BB) += (db * (0x100 - sb)) >> 8;                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 7: /* AOMDST_ALPHA */                                                                                     \
          (RR) += (dr * (0x100 - da)) >> 8;                                                                            \
          (GG) += (dg * (0x100 - da)) >> 8;                                                                            \
          (BB) += (db * (0x100 - da)) >> 8;                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 15: /* A_COLORBEFOREFOG */                                                                                \
          (RR) += (dr * (prefogr + 1)) >> 8;                                                                           \
          (GG) += (dg * (prefogg + 1)) >> 8;                                                                           \
          (BB) += (db * (prefogb + 1)) >> 8;                                                                           \
          break;                                                                                                       \
      }                                                                                                                \
                                                                                                                       \
      /* blend the source alpha */                                                                                     \
      (AA) = 0;                                                                                                        \
      if (ALPHAMODE_SRCALPHABLEND(ALPHAMODE) == 4)                                                                     \
        (AA) = sa;                                                                                                     \
                                                                                                                       \
      /* blend the dest alpha */                                                                                       \
      if (ALPHAMODE_DSTALPHABLEND(ALPHAMODE) == 4)                                                                     \
        (AA) += da;                                                                                                    \
                                                                                                                       \
      /* clamp */                                                                                                      \
      CLAMP((RR), 0x00, 0xff);                                                                                         \
      CLAMP((GG), 0x00, 0xff);                                                                                         \
      CLAMP((BB), 0x00, 0xff);                                                                                         \
      CLAMP((AA), 0x00, 0xff);                                                                                         \
    }                                                                                                                  \
  } while (0)

  /*************************************
   *
   *  Fogging macro
   *
   *************************************/

#define APPLY_FOGGING(VV, FOGMODE, FBZCP, XX, DITHER4, RR, GG, BB, ITERZ, ITERW, ITERAXXX)                             \
  do                                                                                                                   \
  {                                                                                                                    \
    if (FOGMODE_ENABLE_FOG(FOGMODE))                                                                                   \
    {                                                                                                                  \
      rgb_union fogcolor = (VV)->reg[fogColor];                                                                        \
      s32 fr, fg, fb;                                                                                                  \
                                                                                                                       \
      /* constant fog bypasses everything else */                                                                      \
      if (FOGMODE_FOG_CONSTANT(FOGMODE))                                                                               \
      {                                                                                                                \
        fr = fogcolor.rgb.r;                                                                                           \
        fg = fogcolor.rgb.g;                                                                                           \
        fb = fogcolor.rgb.b;                                                                                           \
      }                                                                                                                \
                                                                                                                       \
      /* non-constant fog comes from several sources */                                                                \
      else                                                                                                             \
      {                                                                                                                \
        s32 fogblend = 0;                                                                                              \
                                                                                                                       \
        /* if fog_add is zero, we start with the fog color */                                                          \
        if (FOGMODE_FOG_ADD(FOGMODE) == 0)                                                                             \
        {                                                                                                              \
          fr = fogcolor.rgb.r;                                                                                         \
          fg = fogcolor.rgb.g;                                                                                         \
          fb = fogcolor.rgb.b;                                                                                         \
        }                                                                                                              \
        else                                                                                                           \
          fr = fg = fb = 0;                                                                                            \
                                                                                                                       \
        /* if fog_mult is zero, we subtract the incoming color */                                                      \
        if (FOGMODE_FOG_MULT(FOGMODE) == 0)                                                                            \
        {                                                                                                              \
          fr -= (RR);                                                                                                  \
          fg -= (GG);                                                                                                  \
          fb -= (BB);                                                                                                  \
        }                                                                                                              \
                                                                                                                       \
        /* fog blending mode */                                                                                        \
        switch (FOGMODE_FOG_ZALPHA(FOGMODE))                                                                           \
        {                                                                                                              \
          case 0: /* fog table */                                                                                      \
          {                                                                                                            \
            s32 delta = (VV)->fbi.fogdelta[fogdepth >> 10];                                                            \
            s32 deltaval;                                                                                              \
                                                                                                                       \
            /* perform the multiply against lower 8 bits of wfloat */                                                  \
            deltaval = (delta & (VV)->fbi.fogdelta_mask) * ((fogdepth >> 2) & 0xff);                                   \
                                                                                                                       \
            /* fog zones allow for negating this value */                                                              \
            if (FOGMODE_FOG_ZONES(FOGMODE) && (delta & 2))                                                             \
              deltaval = -deltaval;                                                                                    \
            deltaval >>= 6;                                                                                            \
                                                                                                                       \
            /* apply dither */                                                                                         \
            if (FOGMODE_FOG_DITHER(FOGMODE))                                                                           \
              if (DITHER4)                                                                                             \
                deltaval += DITHER4[(XX)&3];                                                                           \
            deltaval >>= 4;                                                                                            \
                                                                                                                       \
            /* add to the blending factor */                                                                           \
            fogblend = (VV)->fbi.fogblend[fogdepth >> 10] + deltaval;                                                  \
            break;                                                                                                     \
          }                                                                                                            \
                                                                                                                       \
          case 1: /* iterated A */                                                                                     \
            fogblend = ITERAXXX.rgb.a;                                                                                 \
            break;                                                                                                     \
                                                                                                                       \
          case 2: /* iterated Z */                                                                                     \
            CLAMPED_Z((ITERZ), FBZCP, fogblend);                                                                       \
            fogblend >>= 8;                                                                                            \
            break;                                                                                                     \
                                                                                                                       \
          case 3: /* iterated W - Voodoo 2 only */                                                                     \
            CLAMPED_W((ITERW), FBZCP, fogblend);                                                                       \
            break;                                                                                                     \
        }                                                                                                              \
                                                                                                                       \
        /* perform the blend */                                                                                        \
        fogblend++;                                                                                                    \
        fr = (fr * fogblend) >> 8;                                                                                     \
        fg = (fg * fogblend) >> 8;                                                                                     \
        fb = (fb * fogblend) >> 8;                                                                                     \
      }                                                                                                                \
                                                                                                                       \
      /* if fog_mult is 0, we add this to the original color */                                                        \
      if (FOGMODE_FOG_MULT(FOGMODE) == 0)                                                                              \
      {                                                                                                                \
        (RR) += fr;                                                                                                    \
        (GG) += fg;                                                                                                    \
        (BB) += fb;                                                                                                    \
      }                                                                                                                \
                                                                                                                       \
      /* otherwise this just becomes the new color */                                                                  \
      else                                                                                                             \
      {                                                                                                                \
        (RR) = fr;                                                                                                     \
        (GG) = fg;                                                                                                     \
        (BB) = fb;                                                                                                     \
      }                                                                                                                \
                                                                                                                       \
      /* clamp */                                                                                                      \
      CLAMP((RR), 0x00, 0xff);                                                                                         \
      CLAMP((GG), 0x00, 0xff);                                                                                         \
      CLAMP((BB), 0x00, 0xff);                                                                                         \
    }                                                                                                                  \
  } while (0)

  /*************************************
   *
   *  Texture pipeline macro
   *
   *************************************/

#define TEXTURE_PIPELINE(TT, XX, DITHER4, TEXMODE, COTHER, LOOKUP, LODBASE, ITERS, ITERT, ITERW, RESULT)               \
  do                                                                                                                   \
  {                                                                                                                    \
    s32 blendr, blendg, blendb, blenda;                                                                                \
    s32 tr, tg, tb, ta;                                                                                                \
    s32 s, t, lod, ilod;                                                                                               \
    s64 oow;                                                                                                           \
    s32 smax, tmax;                                                                                                    \
    u32 texbase;                                                                                                       \
    rgb_union c_local;                                                                                                 \
                                                                                                                       \
    /* determine the S/T/LOD values for this texture */                                                                \
    if (TEXMODE_ENABLE_PERSPECTIVE(TEXMODE))                                                                           \
    {                                                                                                                  \
      oow = fast_reciplog((ITERW), &lod);                                                                              \
      s = (s32)((oow * (ITERS)) >> 29);                                                                                \
      t = (s32)((oow * (ITERT)) >> 29);                                                                                \
      lod = (LODBASE);                                                                                                 \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      s = (s32)((ITERS) >> 14);                                                                                        \
      t = (s32)((ITERT) >> 14);                                                                                        \
      lod = (LODBASE);                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    /* clamp W */                                                                                                      \
    if (TEXMODE_CLAMP_NEG_W(TEXMODE) && (ITERW) < 0)                                                                   \
      s = t = 0;                                                                                                       \
                                                                                                                       \
    /* clamp the LOD */                                                                                                \
    lod += (TT)->lodbias;                                                                                              \
    if (TEXMODE_ENABLE_LOD_DITHER(TEXMODE))                                                                            \
      if (DITHER4)                                                                                                     \
        lod += DITHER4[(XX)&3] << 4;                                                                                   \
    if (lod < (TT)->lodmin)                                                                                            \
      lod = (TT)->lodmin;                                                                                              \
    if (lod > (TT)->lodmax)                                                                                            \
      lod = (TT)->lodmax;                                                                                              \
                                                                                                                       \
    /* now the LOD is in range; if we don't own this LOD, take the next one */                                         \
    ilod = lod >> 8;                                                                                                   \
    if (!(((TT)->lodmask >> ilod) & 1))                                                                                \
      ilod++;                                                                                                          \
                                                                                                                       \
    /* fetch the texture base */                                                                                       \
    texbase = (TT)->lodoffset[ilod];                                                                                   \
                                                                                                                       \
    /* compute the maximum s and t values at this LOD */                                                               \
    smax = (s32)((TT)->wmask >> ilod);                                                                                 \
    tmax = (s32)((TT)->hmask >> ilod);                                                                                 \
                                                                                                                       \
    /* determine whether we are point-sampled or bilinear */                                                           \
    if ((lod == (TT)->lodmin && !TEXMODE_MAGNIFICATION_FILTER(TEXMODE)) ||                                             \
        (lod != (TT)->lodmin && !TEXMODE_MINIFICATION_FILTER(TEXMODE)))                                                \
    {                                                                                                                  \
      /* point sampled */                                                                                              \
                                                                                                                       \
      u32 texel0;                                                                                                      \
                                                                                                                       \
      /* adjust S/T for the LOD and strip off the fractions */                                                         \
      s >>= ilod + 18;                                                                                                 \
      t >>= ilod + 18;                                                                                                 \
                                                                                                                       \
      /* clamp/wrap S/T if necessary */                                                                                \
      if (TEXMODE_CLAMP_S(TEXMODE))                                                                                    \
        CLAMP(s, 0, smax);                                                                                             \
      if (TEXMODE_CLAMP_T(TEXMODE))                                                                                    \
        CLAMP(t, 0, tmax);                                                                                             \
      s &= smax;                                                                                                       \
      t &= tmax;                                                                                                       \
      t *= smax + 1;                                                                                                   \
                                                                                                                       \
      /* fetch texel data */                                                                                           \
      if (TEXMODE_FORMAT(TEXMODE) < 8)                                                                                 \
      {                                                                                                                \
        texel0 = *(u8*)&(TT)                                                                                           \
                    ->ram[(unsigned long)((unsigned long)texbase + (unsigned long)t + (unsigned long)s) & (TT)->mask]; \
        c_local.u = (LOOKUP)[texel0];                                                                                  \
      }                                                                                                                \
      else                                                                                                             \
      {                                                                                                                \
        texel0 =                                                                                                       \
          *(u16*)&(TT)->ram[(unsigned long)((unsigned long)texbase + 2ul * ((unsigned long)t + (unsigned long)s)) &    \
                            (TT)->mask];                                                                               \
        if (TEXMODE_FORMAT(TEXMODE) >= 10 && TEXMODE_FORMAT(TEXMODE) <= 12)                                            \
          c_local.u = (LOOKUP)[texel0];                                                                                \
        else                                                                                                           \
          c_local.u = ((LOOKUP)[texel0 & 0xff] & 0xffffff) | ((texel0 & 0xff00) << 16);                                \
      }                                                                                                                \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      /* bilinear filtered */                                                                                          \
                                                                                                                       \
      u32 texel0, texel1, texel2, texel3;                                                                              \
      u8 sfrac, tfrac;                                                                                                 \
      s32 s1, t1;                                                                                                      \
                                                                                                                       \
      /* adjust S/T for the LOD and strip off all but the low 8 bits of */                                             \
      /* the fraction */                                                                                               \
      s >>= ilod + 10;                                                                                                 \
      t >>= ilod + 10;                                                                                                 \
                                                                                                                       \
      /* also subtract 1/2 texel so that (0.5,0.5) = a full (0,0) texel */                                             \
      s -= 0x80;                                                                                                       \
      t -= 0x80;                                                                                                       \
                                                                                                                       \
      /* extract the fractions */                                                                                      \
      sfrac = (u8)(s & (TT)->bilinear_mask);                                                                           \
      tfrac = (u8)(t & (TT)->bilinear_mask);                                                                           \
                                                                                                                       \
      /* now toss the rest */                                                                                          \
      s >>= 8;                                                                                                         \
      t >>= 8;                                                                                                         \
      s1 = s + 1;                                                                                                      \
      t1 = t + 1;                                                                                                      \
                                                                                                                       \
      /* clamp/wrap S/T if necessary */                                                                                \
      if (TEXMODE_CLAMP_S(TEXMODE))                                                                                    \
      {                                                                                                                \
        CLAMP(s, 0, smax);                                                                                             \
        CLAMP(s1, 0, smax);                                                                                            \
      }                                                                                                                \
      if (TEXMODE_CLAMP_T(TEXMODE))                                                                                    \
      {                                                                                                                \
        CLAMP(t, 0, tmax);                                                                                             \
        CLAMP(t1, 0, tmax);                                                                                            \
      }                                                                                                                \
      s &= smax;                                                                                                       \
      s1 &= smax;                                                                                                      \
      t &= tmax;                                                                                                       \
      t1 &= tmax;                                                                                                      \
      t *= smax + 1;                                                                                                   \
      t1 *= smax + 1;                                                                                                  \
                                                                                                                       \
      /* fetch texel data */                                                                                           \
      if (TEXMODE_FORMAT(TEXMODE) < 8)                                                                                 \
      {                                                                                                                \
        texel0 = *(u8*)&(TT)->ram[((unsigned long)texbase + (unsigned long)t + (unsigned long)s) & (TT)->mask];        \
        texel1 = *(u8*)&(TT)->ram[((unsigned long)texbase + (unsigned long)t + (unsigned long)s1) & (TT)->mask];       \
        texel2 = *(u8*)&(TT)->ram[((unsigned long)texbase + (unsigned long)t1 + (unsigned long)s) & (TT)->mask];       \
        texel3 = *(u8*)&(TT)->ram[((unsigned long)texbase + (unsigned long)t1 + (unsigned long)s1) & (TT)->mask];      \
        texel0 = (LOOKUP)[texel0];                                                                                     \
        texel1 = (LOOKUP)[texel1];                                                                                     \
        texel2 = (LOOKUP)[texel2];                                                                                     \
        texel3 = (LOOKUP)[texel3];                                                                                     \
      }                                                                                                                \
      else                                                                                                             \
      {                                                                                                                \
        texel0 =                                                                                                       \
          *(u16*)&(TT)->ram[((unsigned long)texbase + 2ul * ((unsigned long)t + (unsigned long)s)) & (TT)->mask];      \
        texel1 =                                                                                                       \
          *(u16*)&(TT)->ram[((unsigned long)texbase + 2ul * ((unsigned long)t + (unsigned long)s1)) & (TT)->mask];     \
        texel2 =                                                                                                       \
          *(u16*)&(TT)->ram[((unsigned long)texbase + 2ul * ((unsigned long)t1 + (unsigned long)s)) & (TT)->mask];     \
        texel3 =                                                                                                       \
          *(u16*)&(TT)->ram[((unsigned long)texbase + 2ul * ((unsigned long)t1 + (unsigned long)s1)) & (TT)->mask];    \
        if (TEXMODE_FORMAT(TEXMODE) >= 10 && TEXMODE_FORMAT(TEXMODE) <= 12)                                            \
        {                                                                                                              \
          texel0 = (LOOKUP)[texel0];                                                                                   \
          texel1 = (LOOKUP)[texel1];                                                                                   \
          texel2 = (LOOKUP)[texel2];                                                                                   \
          texel3 = (LOOKUP)[texel3];                                                                                   \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
          texel0 = ((LOOKUP)[texel0 & 0xff] & 0xffffff) | ((texel0 & 0xff00) << 16);                                   \
          texel1 = ((LOOKUP)[texel1 & 0xff] & 0xffffff) | ((texel1 & 0xff00) << 16);                                   \
          texel2 = ((LOOKUP)[texel2 & 0xff] & 0xffffff) | ((texel2 & 0xff00) << 16);                                   \
          texel3 = ((LOOKUP)[texel3 & 0xff] & 0xffffff) | ((texel3 & 0xff00) << 16);                                   \
        }                                                                                                              \
      }                                                                                                                \
                                                                                                                       \
      /* weigh in each texel */                                                                                        \
      c_local.u = rgba_bilinear_filter(texel0, texel1, texel2, texel3, sfrac, tfrac);                                  \
    }                                                                                                                  \
                                                                                                                       \
    /* select zero/other for RGB */                                                                                    \
    if (!TEXMODE_TC_ZERO_OTHER(TEXMODE))                                                                               \
    {                                                                                                                  \
      tr = COTHER.rgb.r;                                                                                               \
      tg = COTHER.rgb.g;                                                                                               \
      tb = COTHER.rgb.b;                                                                                               \
    }                                                                                                                  \
    else                                                                                                               \
      tr = tg = tb = 0;                                                                                                \
                                                                                                                       \
    /* select zero/other for alpha */                                                                                  \
    if (!TEXMODE_TCA_ZERO_OTHER(TEXMODE))                                                                              \
      ta = COTHER.rgb.a;                                                                                               \
    else                                                                                                               \
      ta = 0;                                                                                                          \
                                                                                                                       \
    /* potentially subtract c_local */                                                                                 \
    if (TEXMODE_TC_SUB_CLOCAL(TEXMODE))                                                                                \
    {                                                                                                                  \
      tr -= c_local.rgb.r;                                                                                             \
      tg -= c_local.rgb.g;                                                                                             \
      tb -= c_local.rgb.b;                                                                                             \
    }                                                                                                                  \
    if (TEXMODE_TCA_SUB_CLOCAL(TEXMODE))                                                                               \
      ta -= c_local.rgb.a;                                                                                             \
                                                                                                                       \
    /* blend RGB */                                                                                                    \
    switch (TEXMODE_TC_MSELECT(TEXMODE))                                                                               \
    {                                                                                                                  \
      default: /* reserved */                                                                                          \
      case 0:  /* zero */                                                                                              \
        blendr = blendg = blendb = 0;                                                                                  \
        break;                                                                                                         \
                                                                                                                       \
      case 1: /* c_local */                                                                                            \
        blendr = c_local.rgb.r;                                                                                        \
        blendg = c_local.rgb.g;                                                                                        \
        blendb = c_local.rgb.b;                                                                                        \
        break;                                                                                                         \
                                                                                                                       \
      case 2: /* a_other */                                                                                            \
        blendr = blendg = blendb = COTHER.rgb.a;                                                                       \
        break;                                                                                                         \
                                                                                                                       \
      case 3: /* a_local */                                                                                            \
        blendr = blendg = blendb = c_local.rgb.a;                                                                      \
        break;                                                                                                         \
                                                                                                                       \
      case 4: /* LOD (detail factor) */                                                                                \
        if ((TT)->detailbias <= lod)                                                                                   \
          blendr = blendg = blendb = 0;                                                                                \
        else                                                                                                           \
        {                                                                                                              \
          blendr = ((((TT)->detailbias - lod) << (TT)->detailscale) >> 8);                                             \
          if (blendr > (TT)->detailmax)                                                                                \
            blendr = (TT)->detailmax;                                                                                  \
          blendg = blendb = blendr;                                                                                    \
        }                                                                                                              \
        break;                                                                                                         \
                                                                                                                       \
      case 5: /* LOD fraction */                                                                                       \
        blendr = blendg = blendb = lod & 0xff;                                                                         \
        break;                                                                                                         \
    }                                                                                                                  \
                                                                                                                       \
    /* blend alpha */                                                                                                  \
    switch (TEXMODE_TCA_MSELECT(TEXMODE))                                                                              \
    {                                                                                                                  \
      default: /* reserved */                                                                                          \
      case 0:  /* zero */                                                                                              \
        blenda = 0;                                                                                                    \
        break;                                                                                                         \
                                                                                                                       \
      case 1: /* c_local */                                                                                            \
        blenda = c_local.rgb.a;                                                                                        \
        break;                                                                                                         \
                                                                                                                       \
      case 2: /* a_other */                                                                                            \
        blenda = COTHER.rgb.a;                                                                                         \
        break;                                                                                                         \
                                                                                                                       \
      case 3: /* a_local */                                                                                            \
        blenda = c_local.rgb.a;                                                                                        \
        break;                                                                                                         \
                                                                                                                       \
      case 4: /* LOD (detail factor) */                                                                                \
        if ((TT)->detailbias <= lod)                                                                                   \
          blenda = 0;                                                                                                  \
        else                                                                                                           \
        {                                                                                                              \
          blenda = ((((TT)->detailbias - lod) << (TT)->detailscale) >> 8);                                             \
          if (blenda > (TT)->detailmax)                                                                                \
            blenda = (TT)->detailmax;                                                                                  \
        }                                                                                                              \
        break;                                                                                                         \
                                                                                                                       \
      case 5: /* LOD fraction */                                                                                       \
        blenda = lod & 0xff;                                                                                           \
        break;                                                                                                         \
    }                                                                                                                  \
                                                                                                                       \
    /* reverse the RGB blend */                                                                                        \
    if (!TEXMODE_TC_REVERSE_BLEND(TEXMODE))                                                                            \
    {                                                                                                                  \
      blendr ^= 0xff;                                                                                                  \
      blendg ^= 0xff;                                                                                                  \
      blendb ^= 0xff;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    /* reverse the alpha blend */                                                                                      \
    if (!TEXMODE_TCA_REVERSE_BLEND(TEXMODE))                                                                           \
      blenda ^= 0xff;                                                                                                  \
                                                                                                                       \
    /* do the blend */                                                                                                 \
    tr = (tr * (blendr + 1)) >> 8;                                                                                     \
    tg = (tg * (blendg + 1)) >> 8;                                                                                     \
    tb = (tb * (blendb + 1)) >> 8;                                                                                     \
    ta = (ta * (blenda + 1)) >> 8;                                                                                     \
                                                                                                                       \
    /* add clocal or alocal to RGB */                                                                                  \
    switch (TEXMODE_TC_ADD_ACLOCAL(TEXMODE))                                                                           \
    {                                                                                                                  \
      case 3: /* reserved */                                                                                           \
      case 0: /* nothing */                                                                                            \
        break;                                                                                                         \
                                                                                                                       \
      case 1: /* add c_local */                                                                                        \
        tr += c_local.rgb.r;                                                                                           \
        tg += c_local.rgb.g;                                                                                           \
        tb += c_local.rgb.b;                                                                                           \
        break;                                                                                                         \
                                                                                                                       \
      case 2: /* add_alocal */                                                                                         \
        tr += c_local.rgb.a;                                                                                           \
        tg += c_local.rgb.a;                                                                                           \
        tb += c_local.rgb.a;                                                                                           \
        break;                                                                                                         \
    }                                                                                                                  \
                                                                                                                       \
    /* add clocal or alocal to alpha */                                                                                \
    if (TEXMODE_TCA_ADD_ACLOCAL(TEXMODE))                                                                              \
      ta += c_local.rgb.a;                                                                                             \
                                                                                                                       \
    /* clamp */                                                                                                        \
    RESULT.rgb.r = (tr < 0) ? 0 : (tr > 0xff) ? 0xff : (u8)tr;                                                         \
    RESULT.rgb.g = (tg < 0) ? 0 : (tg > 0xff) ? 0xff : (u8)tg;                                                         \
    RESULT.rgb.b = (tb < 0) ? 0 : (tb > 0xff) ? 0xff : (u8)tb;                                                         \
    RESULT.rgb.a = (ta < 0) ? 0 : (ta > 0xff) ? 0xff : (u8)ta;                                                         \
                                                                                                                       \
    /* invert */                                                                                                       \
    if (TEXMODE_TC_INVERT_OUTPUT(TEXMODE))                                                                             \
      RESULT.u ^= 0x00ffffff;                                                                                          \
    if (TEXMODE_TCA_INVERT_OUTPUT(TEXMODE))                                                                            \
      RESULT.rgb.a ^= 0xff;                                                                                            \
  } while (0)

  /*************************************
   *
   *  Pixel pipeline macros
   *
   *************************************/

#define PIXEL_PIPELINE_BEGIN(VV, XX, YY, FBZCOLORPATH, FBZMODE, ITERZ, ITERW)                                          \
  s32 depthval, wfloat, fogdepth, biasdepth;                                                                           \
  s32 prefogr, prefogg, prefogb;                                                                                       \
  s32 r, g, b, a;                                                                                                      \
                                                                                                                       \
  /* apply clipping */                                                                                                 \
  /* note that for perf reasons, we assume the caller has done clipping */                                             \
                                                                                                                       \
  /* handle stippling */                                                                                               \
  if (FBZMODE_ENABLE_STIPPLE(FBZMODE))                                                                                 \
  {                                                                                                                    \
    /* rotate mode */                                                                                                  \
    if (FBZMODE_STIPPLE_PATTERN(FBZMODE) == 0)                                                                         \
    {                                                                                                                  \
      (VV)->reg[stipple].u = ((VV)->reg[stipple].u << 1) | ((VV)->reg[stipple].u >> 31);                               \
      if (((VV)->reg[stipple].u & 0x80000000) == 0)                                                                    \
      {                                                                                                                \
        goto skipdrawdepth;                                                                                            \
      }                                                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    /* pattern mode */                                                                                                 \
    else                                                                                                               \
    {                                                                                                                  \
      int stipple_index = (((YY)&3) << 3) | (~(XX)&7);                                                                 \
      if ((((VV)->reg[stipple].u >> stipple_index) & 1) == 0)                                                          \
      {                                                                                                                \
        goto skipdrawdepth;                                                                                            \
      }                                                                                                                \
    }                                                                                                                  \
  }                                                                                                                    \
                                                                                                                       \
  /* compute "floating point" W value (used for depth and fog) */                                                      \
  if ((ITERW)&INT64_C(0xffff00000000))                                                                                 \
    wfloat = 0x0000;                                                                                                   \
  else                                                                                                                 \
  {                                                                                                                    \
    u32 temp = (u32)(ITERW);                                                                                           \
    if ((temp & 0xffff0000) == 0)                                                                                      \
      wfloat = 0xffff;                                                                                                 \
    else                                                                                                               \
    {                                                                                                                  \
      int exp = count_leading_zeros(temp);                                                                             \
      wfloat = (s32)(((unsigned int)exp << 12) | ((~temp >> (19 - exp)) & 0xfff)) + 1;                                 \
    }                                                                                                                  \
  }                                                                                                                    \
  fogdepth = wfloat;                                                                                                   \
  /* add the bias for fog selection*/                                                                                  \
  if (FBZMODE_ENABLE_DEPTH_BIAS(FBZMODE))                                                                              \
  {                                                                                                                    \
    fogdepth += (s16)(VV)->reg[zaColor].u;                                                                             \
    CLAMP(fogdepth, 0, 0xffff);                                                                                        \
  }                                                                                                                    \
                                                                                                                       \
  /* compute depth value (W or Z) for this pixel */                                                                    \
  if (FBZMODE_WBUFFER_SELECT(FBZMODE) == 0)                                                                            \
  {                                                                                                                    \
    CLAMPED_Z(ITERZ, FBZCOLORPATH, depthval);                                                                          \
  }                                                                                                                    \
  else if (FBZMODE_DEPTH_FLOAT_SELECT(FBZMODE) == 0)                                                                   \
    depthval = wfloat;                                                                                                 \
  else                                                                                                                 \
  {                                                                                                                    \
    if ((ITERZ)&0xf0000000l)                                                                                           \
      depthval = 0x0000;                                                                                               \
    else                                                                                                               \
    {                                                                                                                  \
      u32 temp = (u32)(ITERZ << 4);                                                                                    \
      if (!(temp & 0xffff0000u))                                                                                       \
        depthval = 0xffff;                                                                                             \
      else                                                                                                             \
      {                                                                                                                \
        int exp = count_leading_zeros(temp);                                                                           \
        depthval = (s32)(((unsigned int)exp << 12) | ((~temp >> (19 - exp)) & 0xfff)) + 1;                             \
      }                                                                                                                \
    }                                                                                                                  \
  }                                                                                                                    \
  /* add the bias */                                                                                                   \
  biasdepth = depthval;                                                                                                \
  if (FBZMODE_ENABLE_DEPTH_BIAS(FBZMODE))                                                                              \
  {                                                                                                                    \
    biasdepth += (s16)(VV)->reg[zaColor].u;                                                                            \
    CLAMP(biasdepth, 0, 0xffff);                                                                                       \
  }

#define DEPTH_TEST(VV, STATS, XX, FBZMODE)                                                                             \
  do                                                                                                                   \
  {                                                                                                                    \
    /* handle depth buffer testing */                                                                                  \
    if (FBZMODE_ENABLE_DEPTHBUF(FBZMODE))                                                                              \
    {                                                                                                                  \
      s32 depthsource;                                                                                                 \
                                                                                                                       \
      /* the source depth is either the iterated W/Z+bias or a */                                                      \
      /* constant value */                                                                                             \
      if (FBZMODE_DEPTH_SOURCE_COMPARE(FBZMODE) == 0)                                                                  \
        depthsource = biasdepth;                                                                                       \
      else                                                                                                             \
        depthsource = (u16)(VV)->reg[zaColor].u;                                                                       \
                                                                                                                       \
      /* test against the depth buffer */                                                                              \
      switch (FBZMODE_DEPTH_FUNCTION(FBZMODE))                                                                         \
      {                                                                                                                \
        case 0: /* depthOP = never */                                                                                  \
          (STATS)->zfunc_fail++;                                                                                       \
          goto skipdrawdepth;                                                                                          \
                                                                                                                       \
        case 1: /* depthOP = less than */                                                                              \
          if (depthsource >= depth[XX])                                                                                \
          {                                                                                                            \
            (STATS)->zfunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 2: /* depthOP = equal */                                                                                  \
          if (depthsource != depth[XX])                                                                                \
          {                                                                                                            \
            (STATS)->zfunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 3: /* depthOP = less than or equal */                                                                     \
          if (depthsource > depth[XX])                                                                                 \
          {                                                                                                            \
            (STATS)->zfunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 4: /* depthOP = greater than */                                                                           \
          if (depthsource <= depth[XX])                                                                                \
          {                                                                                                            \
            (STATS)->zfunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 5: /* depthOP = not equal */                                                                              \
          if (depthsource == depth[XX])                                                                                \
          {                                                                                                            \
            (STATS)->zfunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 6: /* depthOP = greater than or equal */                                                                  \
          if (depthsource < depth[XX])                                                                                 \
          {                                                                                                            \
            (STATS)->zfunc_fail++;                                                                                     \
            goto skipdrawdepth;                                                                                        \
          }                                                                                                            \
          break;                                                                                                       \
                                                                                                                       \
        case 7: /* depthOP = always */                                                                                 \
          break;                                                                                                       \
      }                                                                                                                \
    }                                                                                                                  \
  } while (0)

#define PIXEL_PIPELINE_MODIFY(VV, DITHER, DITHER4, XX, FBZMODE, FBZCOLORPATH, ALPHAMODE, FOGMODE, ITERZ, ITERW,        \
                              ITERAXXX)                                                                                \
                                                                                                                       \
  /* perform fogging */                                                                                                \
  prefogr = r;                                                                                                         \
  prefogg = g;                                                                                                         \
  prefogb = b;                                                                                                         \
  APPLY_FOGGING(VV, FOGMODE, FBZCOLORPATH, XX, DITHER4, r, g, b, ITERZ, ITERW, ITERAXXX);                              \
                                                                                                                       \
  /* perform alpha blending */                                                                                         \
  APPLY_ALPHA_BLEND(FBZMODE, ALPHAMODE, XX, DITHER, r, g, b, a);

#define PIXEL_PIPELINE_FINISH(VV, DITHER_LOOKUP, XX, dest, depth, FBZMODE)                                             \
                                                                                                                       \
  /* write to framebuffer */                                                                                           \
  if (FBZMODE_RGB_BUFFER_MASK(FBZMODE))                                                                                \
  {                                                                                                                    \
    /* apply dithering */                                                                                              \
    APPLY_DITHER(FBZMODE, XX, DITHER_LOOKUP, r, g, b);                                                                 \
    dest[XX] = (u16)((r << 11) | (g << 5) | b);                                                                        \
  }                                                                                                                    \
                                                                                                                       \
  /* write to aux buffer */                                                                                            \
  if (depth && FBZMODE_AUX_BUFFER_MASK(FBZMODE))                                                                       \
  {                                                                                                                    \
    if (FBZMODE_ENABLE_ALPHA_PLANES(FBZMODE) == 0)                                                                     \
      depth[XX] = (u16)biasdepth;                                                                                      \
    else                                                                                                               \
      depth[XX] = (u16)a;                                                                                              \
  }

#define PIXEL_PIPELINE_END(STATS)                                                                                      \
                                                                                                                       \
  /* track pixel writes to the frame buffer regardless of mask */                                                      \
  (STATS)->pixels_out++;                                                                                               \
                                                                                                                       \
  skipdrawdepth:;
