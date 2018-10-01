#include "voodoo_emu.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Log.h"
#include "common/display.h"
#include "common/timing.h"
#include "voodoo_funcs.h"
#include "voodoo_regs.h"
#include "voodoo_types.h"
#include <cmath>
Log_SetChannel(voodoo_emu);

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

****************************************************************************

    3dfx Voodoo Graphics SST-1/2 emulator

    emulator by Aaron Giles

    --------------------------

    Specs:

    Voodoo 1 (SST1):
        2,4MB frame buffer RAM
        1,2,4MB texture RAM
        50MHz clock frequency
        clears @ 2 pixels/clock (RGB and depth simultaneously)
        renders @ 1 pixel/clock
        64 entry PCI FIFO
        memory FIFO up to 65536 entries

    Voodoo 2:
        2,4MB frame buffer RAM
        2,4,8,16MB texture RAM
        90MHz clock frquency
        clears @ 2 pixels/clock (RGB and depth simultaneously)
        renders @ 1 pixel/clock
        ultrafast clears @ 16 pixels/clock
        128 entry PCI FIFO
        memory FIFO up to 65536 entries

    --------------------------


iterated RGBA = 12.12 [24 bits]
iterated Z    = 20.12 [32 bits]
iterated W    = 18.32 [48 bits]

**************************************************************************/

// voodoo_state* v;

#define LOG_VBLANK_SWAP (0)
#define LOG_REGISTERS (0)
#define LOG_LFB (0)
#define LOG_TEXTURE_RAM (0)
#define LOG_RASTERIZERS (0)

/* fast dither lookup */
static u8 dither4_lookup[256 * 16 * 2];
static u8 dither2_lookup[256 * 16 * 2];

/* fast reciprocal+log2 lookup */
u32 voodoo_reciplog[(2 << RECIPLOG_LOOKUP_BITS) + 2];

/*************************************
 *
 *  Prototypes
 *
 *************************************/

/* command handlers */
static void fastfill(voodoo_state* v);
static void swapbuffer(voodoo_state* v, u32 data);
static void triangle(voodoo_state* v);
static void begin_triangle(voodoo_state* v);
static void draw_triangle(voodoo_state* v);

/* triangle helpers */
static void setup_and_draw_triangle(voodoo_state* v);
static void triangle_create_work_item(voodoo_state* v, u16* drawbuf, int texcount);

/* rasterizer management */
static raster_info* add_rasterizer(voodoo_state* v, const raster_info* cinfo);
static raster_info* find_rasterizer(voodoo_state* v, int texcount);

/* generic rasterizers */
static void raster_fastfill(void* dest, s32 scanline, const poly_extent* extent, const void* extradata);

/***************************************************************************
    RASTERIZER MANAGEMENT
***************************************************************************/

void raster_generic(u32 TMUS, u32 TEXMODE0, u32 TEXMODE1, void* destbase, s32 y, const poly_extent* extent,
                    const void* extradata)
{
  const poly_extra_data* extra = (const poly_extra_data*)extradata;
  voodoo_state* v = extra->state;
  stats_block* stats = &v->thread_stats[0];
  DECLARE_DITHER_POINTERS;
  s32 startx = extent->startx;
  s32 stopx = extent->stopx;
  s32 iterr, iterg, iterb, itera;
  s32 iterz;
  s64 iterw, iterw0 = 0, iterw1 = 0;
  s64 iters0 = 0, iters1 = 0;
  s64 itert0 = 0, itert1 = 0;
  u16* depth;
  u16* dest;
  s32 dx, dy;
  s32 scry;
  s32 x;

  /* determine the screen Y */
  scry = y;
  if (FBZMODE_Y_ORIGIN(v->reg[fbzMode].u))
    scry = (s32)(((int)v->fbi.yorigin - y) & 0x3ff);

  /* compute the dithering pointers */
  if (FBZMODE_ENABLE_DITHERING(v->reg[fbzMode].u))
  {
    dither4 = &dither_matrix_4x4[(y & 3) * 4];
    if (FBZMODE_DITHER_TYPE(v->reg[fbzMode].u) == 0)
    {
      dither = dither4;
      dither_lookup = &dither4_lookup[(y & 3) << 11];
    }
    else
    {
      dither = &dither_matrix_2x2[(y & 3) * 4];
      dither_lookup = &dither2_lookup[(y & 3) << 11];
    }
  }

  /* apply clipping */
  if (FBZMODE_ENABLE_CLIPPING(v->reg[fbzMode].u))
  {
    s32 tempclip;

    /* Y clipping buys us the whole scanline */
    if (scry < (s32)((v->reg[clipLowYHighY].u >> 16) & 0x3ff) || scry >= (s32)(v->reg[clipLowYHighY].u & 0x3ff))
    {
      stats->pixels_in += stopx - startx;
      stats->clip_fail += stopx - startx;
      return;
    }

    /* X clipping */
    tempclip = (v->reg[clipLeftRight].u >> 16) & 0x3ff;
    if (startx < tempclip)
    {
      stats->pixels_in += tempclip - startx;
      startx = tempclip;
    }
    tempclip = v->reg[clipLeftRight].u & 0x3ff;
    if (stopx >= tempclip)
    {
      stats->pixels_in += stopx - tempclip;
      stopx = tempclip - 1;
    }
  }

  /* get pointers to the target buffer and depth buffer */
  dest = (u16*)destbase + (unsigned int)scry * (unsigned long)v->fbi.rowpixels;
  depth =
    (v->fbi.auxoffs != (u32)(~0)) ?
      ((u16*)(v->fbi.ram + (unsigned int)v->fbi.auxoffs) + (unsigned long)scry * (unsigned long)v->fbi.rowpixels) :
      NULL;

  /* compute the starting parameters */
  dx = startx - (extra->ax >> 4);
  dy = y - (extra->ay >> 4);
  iterr = extra->startr + dy * extra->drdy + dx * extra->drdx;
  iterg = extra->startg + dy * extra->dgdy + dx * extra->dgdx;
  iterb = extra->startb + dy * extra->dbdy + dx * extra->dbdx;
  itera = extra->starta + dy * extra->dady + dx * extra->dadx;
  iterz = extra->startz + dy * extra->dzdy + dx * extra->dzdx;
  iterw = extra->startw + dy * extra->dwdy + dx * extra->dwdx;
  if (TMUS >= 1)
  {
    iterw0 = extra->startw0 + dy * extra->dw0dy + dx * extra->dw0dx;
    iters0 = extra->starts0 + dy * extra->ds0dy + dx * extra->ds0dx;
    itert0 = extra->startt0 + dy * extra->dt0dy + dx * extra->dt0dx;
  }
  if (TMUS >= 2)
  {
    iterw1 = extra->startw1 + dy * extra->dw1dy + dx * extra->dw1dx;
    iters1 = extra->starts1 + dy * extra->ds1dy + dx * extra->ds1dx;
    itert1 = extra->startt1 + dy * extra->dt1dy + dx * extra->dt1dx;
  }

  /* loop in X */
  for (x = startx; x < stopx; x++)
  {
    rgb_union iterargb;
    rgb_union texel = {0};
    rgb_union color;

    /* unused variable? */
    (void)color;

    /* pixel pipeline part 1 handles depth testing and stippling */
    PIXEL_PIPELINE_BEGIN(v, x, y, v->reg[fbzColorPath].u, v->reg[fbzMode].u, iterz, iterw);

    /* depth testing */
    DEPTH_TEST(v, stats, x, v->reg[fbzMode].u);

    /* run the texture pipeline on TMU1 to produce a value in texel */
    /* note that they set LOD min to 8 to "disable" a TMU */

    if (TMUS >= 2 && v->tmu[1].lodmin < (8 << 8))
    {
      TEXTURE_PIPELINE(&v->tmu[1], x, dither4, TEXMODE1, texel, v->tmu[1].lookup, extra->lodbase1, iters1, itert1,
                       iterw1, texel);
    }

    /* run the texture pipeline on TMU0 to produce a final */
    /* result in texel */
    /* note that they set LOD min to 8 to "disable" a TMU */
    if (TMUS >= 1 && v->tmu[0].lodmin < (8 << 8))
    {
      if (!v->send_config)
      {
        TEXTURE_PIPELINE(&v->tmu[0], x, dither4, TEXMODE0, texel, v->tmu[0].lookup, extra->lodbase0, iters0, itert0,
                         iterw0, texel);
      }
      else
      { /* send config data to the frame buffer */
        texel.u = v->tmu_config;
      }
    }

    /* colorpath pipeline selects source colors and does blending */
    CLAMPED_ARGB(iterr, iterg, iterb, itera, v->reg[fbzColorPath].u, iterargb);

    s32 blendr, blendg, blendb, blenda;
    rgb_union c_other;
    rgb_union c_local;

    /* compute c_other */
    switch (FBZCP_CC_RGBSELECT(v->reg[fbzColorPath].u))
    {
      case 0: /* iterated RGB */
        c_other.u = iterargb.u;
        break;
      case 1: /* texture RGB */
        c_other.u = texel.u;
        break;
      case 2: /* color1 RGB */
        c_other.u = v->reg[color1].u;
        break;
      default: /* reserved */
        c_other.u = 0;
        break;
    }

    /* handle chroma key */
    APPLY_CHROMAKEY(v, stats, v->reg[fbzMode].u, c_other);

    /* compute a_other */
    switch (FBZCP_CC_ASELECT(v->reg[fbzColorPath].u))
    {
      case 0: /* iterated alpha */
        c_other.rgb.a = iterargb.rgb.a;
        break;
      case 1: /* texture alpha */
        c_other.rgb.a = texel.rgb.a;
        break;
      case 2: /* color1 alpha */
        c_other.rgb.a = v->reg[color1].rgb.a;
        break;
      default: /* reserved */
        c_other.rgb.a = 0;
        break;
    }

    /* handle alpha mask */
    APPLY_ALPHAMASK(v, stats, v->reg[fbzMode].u, c_other.rgb.a);

    /* compute a_local */
    switch (FBZCP_CCA_LOCALSELECT(v->reg[fbzColorPath].u))
    {
      default:
      case 0: /* iterated alpha */
        c_local.rgb.a = iterargb.rgb.a;
        break;
      case 1: /* color0 alpha */
        c_local.rgb.a = v->reg[color0].rgb.a;
        break;
      case 2: /* clamped iterated Z[27:20] */
      {
        int temp;
        CLAMPED_Z(iterz, v->reg[fbzColorPath].u, temp);
        c_local.rgb.a = (u8)temp;
        break;
      }
      case 3: /* clamped iterated W[39:32] */
      {
        int temp;
        CLAMPED_W(iterw, v->reg[fbzColorPath].u, temp); /* Voodoo 2 only */
        c_local.rgb.a = (u8)temp;
        break;
      }
    }

    /* select zero or a_other */
    if (FBZCP_CCA_ZERO_OTHER(v->reg[fbzColorPath].u) == 0)
      a = c_other.rgb.a;
    else
      a = 0;

    /* subtract a_local */
    if (FBZCP_CCA_SUB_CLOCAL(v->reg[fbzColorPath].u))
      a -= c_local.rgb.a;

    /* blend alpha */
    switch (FBZCP_CCA_MSELECT(v->reg[fbzColorPath].u))
    {
      default: /* reserved */
      case 0:  /* 0 */
        blenda = 0;
        break;

      case 1: /* a_local */
        blenda = c_local.rgb.a;
        break;

      case 2: /* a_other */
        blenda = c_other.rgb.a;
        break;

      case 3: /* a_local */
        blenda = c_local.rgb.a;
        break;

      case 4: /* texture alpha */
        blenda = texel.rgb.a;
        break;
    }

    /* reverse the alpha blend */
    if (!FBZCP_CCA_REVERSE_BLEND(v->reg[fbzColorPath].u))
      blenda ^= 0xff;

    /* do the blend */
    a = (a * (blenda + 1)) >> 8;

    /* add clocal or alocal to alpha */
    if (FBZCP_CCA_ADD_ACLOCAL(v->reg[fbzColorPath].u))
      a += c_local.rgb.a;

    /* clamp */
    CLAMP(a, 0x00, 0xff);

    /* invert */
    if (FBZCP_CCA_INVERT_OUTPUT(v->reg[fbzColorPath].u))
      a ^= 0xff;

    /* handle alpha test */
    APPLY_ALPHATEST(v, stats, v->reg[alphaMode].u, a);

    /* compute c_local */
    if (FBZCP_CC_LOCALSELECT_OVERRIDE(v->reg[fbzColorPath].u) == 0)
    {
      if (FBZCP_CC_LOCALSELECT(v->reg[fbzColorPath].u) == 0) /* iterated RGB */
        c_local.u = iterargb.u;
      else /* color0 RGB */
        c_local.u = v->reg[color0].u;
    }
    else
    {
      if (!(texel.rgb.a & 0x80)) /* iterated RGB */
        c_local.u = iterargb.u;
      else /* color0 RGB */
        c_local.u = v->reg[color0].u;
    }

    /* select zero or c_other */
    if (FBZCP_CC_ZERO_OTHER(v->reg[fbzColorPath].u) == 0)
    {
      r = c_other.rgb.r;
      g = c_other.rgb.g;
      b = c_other.rgb.b;
    }
    else
      r = g = b = 0;

    /* subtract c_local */
    if (FBZCP_CC_SUB_CLOCAL(v->reg[fbzColorPath].u))
    {
      r -= c_local.rgb.r;
      g -= c_local.rgb.g;
      b -= c_local.rgb.b;
    }

    /* blend RGB */
    switch (FBZCP_CC_MSELECT(v->reg[fbzColorPath].u))
    {
      default: /* reserved */
      case 0:  /* 0 */
        blendr = blendg = blendb = 0;
        break;
      case 1: /* c_local */
        blendr = c_local.rgb.r;
        blendg = c_local.rgb.g;
        blendb = c_local.rgb.b;
        break;
      case 2: /* a_other */
        blendr = blendg = blendb = c_other.rgb.a;
        break;
      case 3: /* a_local */
        blendr = blendg = blendb = c_local.rgb.a;
        break;
      case 4: /* texture alpha */
        blendr = blendg = blendb = texel.rgb.a;
        break;
      case 5: /* texture RGB (Voodoo 2 only) */
        blendr = texel.rgb.r;
        blendg = texel.rgb.g;
        blendb = texel.rgb.b;
        break;
    }

    /* reverse the RGB blend */
    if (!FBZCP_CC_REVERSE_BLEND(v->reg[fbzColorPath].u))
    {
      blendr ^= 0xff;
      blendg ^= 0xff;
      blendb ^= 0xff;
    }

    /* do the blend */
    r = (r * (blendr + 1)) >> 8;
    g = (g * (blendg + 1)) >> 8;
    b = (b * (blendb + 1)) >> 8;

    /* add clocal or alocal to RGB */
    switch (FBZCP_CC_ADD_ACLOCAL(v->reg[fbzColorPath].u))
    {
      case 3: /* reserved */
      case 0: /* nothing */
        break;
      case 1: /* add c_local */
        r += c_local.rgb.r;
        g += c_local.rgb.g;
        b += c_local.rgb.b;
        break;
      case 2: /* add_alocal */
        r += c_local.rgb.a;
        g += c_local.rgb.a;
        b += c_local.rgb.a;
        break;
    }

    /* clamp */
    CLAMP(r, 0x00, 0xff);
    CLAMP(g, 0x00, 0xff);
    CLAMP(b, 0x00, 0xff);

    /* invert */
    if (FBZCP_CC_INVERT_OUTPUT(v->reg[fbzColorPath].u))
    {
      r ^= 0xff;
      g ^= 0xff;
      b ^= 0xff;
    }

    /* pixel pipeline part 2 handles fog, alpha, and final output */
    PIXEL_PIPELINE_MODIFY(v, dither, dither4, x, v->reg[fbzMode].u, v->reg[fbzColorPath].u, v->reg[alphaMode].u,
                          v->reg[fogMode].u, iterz, iterw, iterargb);
    PIXEL_PIPELINE_FINISH(v, dither_lookup, x, dest, depth, v->reg[fbzMode].u);
    PIXEL_PIPELINE_END(stats);

    /* update the iterated parameters */
    iterr += extra->drdx;
    iterg += extra->dgdx;
    iterb += extra->dbdx;
    itera += extra->dadx;
    iterz += extra->dzdx;
    iterw += extra->dwdx;
    if (TMUS >= 1)
    {
      iterw0 += extra->dw0dx;
      iters0 += extra->ds0dx;
      itert0 += extra->dt0dx;
    }
    if (TMUS >= 2)
    {
      iterw1 += extra->dw1dx;
      iters1 += extra->ds1dx;
      itert1 += extra->dt1dx;
    }
  }
}

/***************************************************************************
    RASTERIZER MANAGEMENT
***************************************************************************/

void raster_generic_0tmu(void* destbase, s32 y, const poly_extent* extent, const void* extradata)
{
  raster_generic(0, 0, 0, destbase, y, extent, extradata);
}

void raster_generic_1tmu(void* destbase, s32 y, const poly_extent* extent, const void* extradata)
{
  const poly_extra_data* extra = (const poly_extra_data*)extradata;
  voodoo_state* v = extra->state;
  raster_generic(1, v->tmu[0].reg[textureMode].u, 0, destbase, y, extent, extradata);
}

void raster_generic_2tmu(void* destbase, s32 y, const poly_extent* extent, const void* extradata)
{
  const poly_extra_data* extra = (const poly_extra_data*)extradata;
  voodoo_state* v = extra->state;
  raster_generic(2, v->tmu[0].reg[textureMode].u, v->tmu[1].reg[textureMode].u, destbase, y, extent, extradata);
}

/*************************************
 *
 *  Common initialization
 *
 *************************************/

void init_fbi(voodoo_state* v, fbi_state* f, int fbmem)
{
  if (fbmem <= 1)
    Panic("VOODOO: invalid frame buffer memory size requested");
  /* allocate frame buffer RAM and set pointers */
  f->ram = (u8*)malloc((size_t)fbmem);
  f->mask = (u32)(fbmem - 1);
  f->rgboffs[0] = f->rgboffs[1] = f->rgboffs[2] = 0;
  f->auxoffs = (u32)(~0);

  /* default to 0x0 */
  f->frontbuf = 0;
  f->backbuf = 1;
  f->width = 640;
  f->height = 480;
  //	f->xoffs = 0;
  //	f->yoffs = 0;

  //	f->vsyncscan = 0;

  /* init the pens */
  /*	for (u8 pen = 0; pen < 32; pen++)
      v->fbi.clut[pen] = MAKE_ARGB(pen, pal5bit(pen), pal5bit(pen), pal5bit(pen));
    v->fbi.clut[32] = MAKE_ARGB(32,0xff,0xff,0xff); */

  /* allocate a VBLANK timer */
  f->vblank = false;

  /* initialize the memory FIFO */
  f->fifo.size = 0;

  /* set the fog delta mask */
  f->fogdelta_mask = (v->type < VOODOO_2) ? 0xff : 0xfc;

  f->yorigin = 0;

  f->sverts = 0;

  memset(&f->lfb_stats, 0, sizeof(f->lfb_stats));
  memset(&f->fogblend, 0, sizeof(f->fogblend));
  memset(&f->fogdelta, 0, sizeof(f->fogdelta));
}

void init_tmu_shared(tmu_shared_state* s)
{
  int val;

  /* build static 8-bit texel tables */
  for (val = 0; val < 256; val++)
  {
    int r, g, b, a;

    /* 8-bit RGB (3-3-2) */
    EXTRACT_332_TO_888(val, r, g, b);
    s->rgb332[val] = MAKE_ARGB(0xff, r, g, b);

    /* 8-bit alpha */
    s->alpha8[val] = MAKE_ARGB(val, val, val, val);

    /* 8-bit intensity */
    s->int8[val] = MAKE_ARGB(0xff, val, val, val);

    /* 8-bit alpha, intensity */
    a = ((val >> 0) & 0xf0) | ((val >> 4) & 0x0f);
    r = ((val << 4) & 0xf0) | ((val << 0) & 0x0f);
    s->ai44[val] = MAKE_ARGB(a, r, r, r);
  }

  /* build static 16-bit texel tables */
  for (val = 0; val < 65536; val++)
  {
    int r, g, b, a;

    /* table 10 = 16-bit RGB (5-6-5) */
    EXTRACT_565_TO_888(val, r, g, b);
    s->rgb565[val] = MAKE_ARGB(0xff, r, g, b);

    /* table 11 = 16 ARGB (1-5-5-5) */
    EXTRACT_1555_TO_8888(val, a, r, g, b);
    s->argb1555[val] = MAKE_ARGB(a, r, g, b);

    /* table 12 = 16-bit ARGB (4-4-4-4) */
    EXTRACT_4444_TO_8888(val, a, r, g, b);
    s->argb4444[val] = MAKE_ARGB(a, r, g, b);
  }
}

void init_tmu(voodoo_state* v, tmu_state* t, voodoo_reg* reg, int tmem)
{
  if (tmem <= 1)
    Panic("VOODOO: invalid texture buffer memory size requested");
  /* allocate texture RAM */
  t->ram = (u8*)malloc((size_t)tmem);
  t->mask = (u32)(tmem - 1);
  t->reg = reg;
  t->regdirty = true;
  t->bilinear_mask = (v->type >= VOODOO_2) ? 0xff : 0xf0;

  /* mark the NCC tables dirty and configure their registers */
  t->ncc[0].dirty = t->ncc[1].dirty = true;
  t->ncc[0].reg = &t->reg[nccTable + 0];
  t->ncc[1].reg = &t->reg[nccTable + 12];

  /* create pointers to all the tables */
  t->texel[0] = v->tmushare.rgb332;
  t->texel[1] = t->ncc[0].texel;
  t->texel[2] = v->tmushare.alpha8;
  t->texel[3] = v->tmushare.int8;
  t->texel[4] = v->tmushare.ai44;
  t->texel[5] = t->palette;
  t->texel[6] = (v->type >= VOODOO_2) ? t->palettea : NULL;
  t->texel[7] = NULL;
  t->texel[8] = v->tmushare.rgb332;
  t->texel[9] = t->ncc[0].texel;
  t->texel[10] = v->tmushare.rgb565;
  t->texel[11] = v->tmushare.argb1555;
  t->texel[12] = v->tmushare.argb4444;
  t->texel[13] = v->tmushare.int8;
  t->texel[14] = t->palette;
  t->texel[15] = NULL;
  t->lookup = t->texel[0];

  /* attach the palette to NCC table 0 */
  t->ncc[0].palette = t->palette;
  if (v->type >= VOODOO_2)
    t->ncc[0].palettea = t->palettea;

  /* set up texture address calculations */
  t->texaddr_mask = 0x0fffff;
  t->texaddr_shift = 3;

  t->lodmin = 0;
  t->lodmax = 0;
}

/*************************************
 *
 *  VBLANK management
 *
 *************************************/

void voodoo_swap_buffers(voodoo_state* v)
{
  //	if (LOG_VBLANK_SWAP) LOG(LOG_VOODOO,LOG_WARN)("--- swap_buffers @ %d\n", video_screen_get_vpos(v->screen));

  /* keep a history of swap intervals */
  v->reg[fbiSwapHistory].u = (v->reg[fbiSwapHistory].u << 4);

  /* rotate the buffers */
  if (v->type < VOODOO_2 || !v->fbi.vblank_dont_swap)
  {
    if (v->fbi.rgboffs[2] == (u32)(~0))
    {
      v->fbi.frontbuf = (u8)(1 - v->fbi.frontbuf);
      v->fbi.backbuf = (u8)(1 - v->fbi.frontbuf);
    }
    else
    {
      v->fbi.frontbuf = (v->fbi.frontbuf + 1) % 3;
      v->fbi.backbuf = (v->fbi.frontbuf + 1) % 3;
    }
  }
}

/*************************************
 *
 *  Chip reset
 *
 *************************************/

void reset_counters(voodoo_state* v)
{
  v->reg[fbiPixelsIn].u = 0;
  v->reg[fbiChromaFail].u = 0;
  v->reg[fbiZfuncFail].u = 0;
  v->reg[fbiAfuncFail].u = 0;
  v->reg[fbiPixelsOut].u = 0;
}

void soft_reset(voodoo_state* v)
{
  reset_counters(v);
  v->reg[fbiTrianglesOut].u = 0;
}

/*************************************
 *
 *  Recompute video memory layout
 *
 *************************************/

void recompute_video_memory(voodoo_state* v)
{
  u32 buffer_pages = FBIINIT2_VIDEO_BUFFER_OFFSET(v->reg[fbiInit2].u);
  u32 fifo_start_page = FBIINIT4_MEMORY_FIFO_START_ROW(v->reg[fbiInit4].u);
  u32 fifo_last_page = FBIINIT4_MEMORY_FIFO_STOP_ROW(v->reg[fbiInit4].u);
  u32 memory_config;
  int buf;

  /* memory config is determined differently between V1 and V2 */
  memory_config = FBIINIT2_ENABLE_TRIPLE_BUF(v->reg[fbiInit2].u);
  if (v->type == VOODOO_2 && memory_config == 0)
    memory_config = FBIINIT5_BUFFER_ALLOCATION(v->reg[fbiInit5].u);

  /* tiles are 64x16/32; x_tiles specifies how many half-tiles */
  v->fbi.tile_width = (v->type < VOODOO_2) ? 64 : 32;
  v->fbi.tile_height = (v->type < VOODOO_2) ? 16 : 32;
  v->fbi.x_tiles = FBIINIT1_X_VIDEO_TILES(v->reg[fbiInit1].u);
  if (v->type == VOODOO_2)
  {
    v->fbi.x_tiles = (v->fbi.x_tiles << 1) | (FBIINIT1_X_VIDEO_TILES_BIT5(v->reg[fbiInit1].u) << 5) |
                     (FBIINIT6_X_VIDEO_TILES_BIT0(v->reg[fbiInit6].u));
  }
  v->fbi.rowpixels = v->fbi.tile_width * v->fbi.x_tiles;

  //  logerror("VOODOO.%d.VIDMEM: buffer_pages=%X  fifo=%X-%X  tiles=%X  rowpix=%d\n", v->index, buffer_pages,
  //  fifo_start_page, fifo_last_page, v->fbi.x_tiles, v->fbi.rowpixels);

  /* first RGB buffer always starts at 0 */
  v->fbi.rgboffs[0] = 0;

  /* second RGB buffer starts immediately afterwards */
  v->fbi.rgboffs[1] = buffer_pages * 0x1000;

  /* remaining buffers are based on the config */
  switch (memory_config)
  {
    case 3: /* reserved */
      Panic("Unexpected memory configuration in recompute_video_memory!");
      break;

    case 0: /* 2 color buffers, 1 aux buffer */
      v->fbi.rgboffs[2] = (u32)(~0);
      v->fbi.auxoffs = 2 * buffer_pages * 0x1000;
      break;

    case 1: /* 3 color buffers, 0 aux buffers */
      v->fbi.rgboffs[2] = 2 * buffer_pages * 0x1000;
      v->fbi.auxoffs = (u32)(~0);
      break;

    case 2: /* 3 color buffers, 1 aux buffers */
      v->fbi.rgboffs[2] = 2 * buffer_pages * 0x1000;
      v->fbi.auxoffs = 3 * buffer_pages * 0x1000;
      break;
  }

  /* clamp the RGB buffers to video memory */
  for (buf = 0; buf < 3; buf++)
    if (v->fbi.rgboffs[buf] != (u32)(~0) && v->fbi.rgboffs[buf] > v->fbi.mask)
      v->fbi.rgboffs[buf] = v->fbi.mask;

  /* clamp the aux buffer to video memory */
  if (v->fbi.auxoffs != (u32)(~0) && v->fbi.auxoffs > v->fbi.mask)
    v->fbi.auxoffs = v->fbi.mask;

  /* compute the memory FIFO location and size */
  if (fifo_last_page > v->fbi.mask / 0x1000)
    fifo_last_page = v->fbi.mask / 0x1000;

  /* is it valid and enabled? */
  if (fifo_start_page <= fifo_last_page && FBIINIT0_ENABLE_MEMORY_FIFO(v->reg[fbiInit0].u))
  {
    v->fbi.fifo.size = (s32)(((unsigned int)fifo_last_page + 1u - (unsigned int)fifo_start_page) * 0x1000u / 4u);
    if (v->fbi.fifo.size > 65536 * 2)
      v->fbi.fifo.size = 65536 * 2;
  }
  else /* if not, disable the FIFO */
  {
    v->fbi.fifo.size = 0;
  }

  /* reset our front/back buffers if they are out of range */
  if (v->fbi.rgboffs[2] == (u32)(~0))
  {
    if (v->fbi.frontbuf == 2)
      v->fbi.frontbuf = 0;
    if (v->fbi.backbuf == 2)
      v->fbi.backbuf = 0;
  }
}

/*************************************
 *
 *  NCC table management
 *
 *************************************/

void ncc_table_write(ncc_table* n, u32 regnum, u32 data)
{
  /* I/Q entries reference the palette if the high bit is set */
  if (regnum >= 4 && (data & 0x80000000) && n->palette)
  {
    u32 index = ((data >> 23) & 0xfe) | (regnum & 1);

    rgb_t palette_entry = 0xff000000 | data;

    if (n->palette[index] != palette_entry)
    {
      /* set the ARGB for this palette index */
      n->palette[index] = palette_entry;
    }

    /* if we have an ARGB palette as well, compute its value */
    if (n->palettea)
    {
      u32 a = ((data >> 16) & 0xfc) | ((data >> 22) & 0x03);
      u32 r = ((data >> 10) & 0xfc) | ((data >> 16) & 0x03);
      u32 g = ((data >> 4) & 0xfc) | ((data >> 10) & 0x03);
      u32 b = ((data << 2) & 0xfc) | ((data >> 4) & 0x03);
      n->palettea[index] = MAKE_ARGB(a, r, g, b);
    }

    /* this doesn't dirty the table or go to the registers, so bail */
    return;
  }

  /* if the register matches, don't update */
  if (data == n->reg[regnum].u)
    return;
  n->reg[regnum].u = data;

  /* first four entries are packed Y values */
  if (regnum < 4)
  {
    regnum *= 4;
    n->y[regnum + 0] = (s32)(data >> 0) & 0xff;
    n->y[regnum + 1] = (s32)(data >> 8) & 0xff;
    n->y[regnum + 2] = (s32)(data >> 16) & 0xff;
    n->y[regnum + 3] = (s32)(data >> 24) & 0xff;
  }

  /* the second four entries are the I RGB values */
  else if (regnum < 8)
  {
    regnum &= 3;
    n->ir[regnum] = (s32)(data << 5) >> 23;
    n->ig[regnum] = (s32)(data << 14) >> 23;
    n->ib[regnum] = (s32)(data << 23) >> 23;
  }

  /* the final four entries are the Q RGB values */
  else
  {
    regnum &= 3;
    n->qr[regnum] = (s32)(data << 5) >> 23;
    n->qg[regnum] = (s32)(data << 14) >> 23;
    n->qb[regnum] = (s32)(data << 23) >> 23;
  }

  /* mark the table dirty */
  n->dirty = true;
}

void ncc_table_update(ncc_table* n)
{
  int r, g, b, i;

  /* generte all 256 possibilities */
  for (i = 0; i < 256; i++)
  {
    int vi = (i >> 2) & 0x03;
    int vq = (i >> 0) & 0x03;

    /* start with the intensity */
    r = g = b = n->y[(i >> 4) & 0x0f];

    /* add the coloring */
    r += n->ir[vi] + n->qr[vq];
    g += n->ig[vi] + n->qg[vq];
    b += n->ib[vi] + n->qb[vq];

    /* clamp */
    CLAMP(r, 0, 255);
    CLAMP(g, 0, 255);
    CLAMP(b, 0, 255);

    /* fill in the table */
    n->texel[i] = MAKE_ARGB(0xff, r, g, b);
  }

  /* no longer dirty */
  n->dirty = false;
}

/*************************************
 *
 *  Faux DAC implementation
 *
 *************************************/

static void dacdata_w(dac_state* d, u8 regnum, u8 data)
{
  d->reg[regnum] = data;
}

static void dacdata_r(dac_state* d, u8 regnum)
{
  u8 result = 0xff;

  /* switch off the DAC register requested */
  switch (regnum)
  {
    case 5:
      /* this is just to make startup happy */
      switch (d->reg[7])
      {
        case 0x01:
          result = 0x55;
          break;
        case 0x07:
          result = 0x71;
          break;
        case 0x0b:
          result = 0x79;
          break;
      }
      break;

    default:
      result = d->reg[regnum];
      break;
  }

  /* remember the read result; it is fetched elsewhere */
  d->read_result = result;
}

/*************************************
 *
 *  Texuture parameter computation
 *
 *************************************/

static void recompute_texture_params(tmu_state* t)
{
  int bppscale;
  u32 base;
  int lod;

  /* extract LOD parameters */
  t->lodmin = (s32)TEXLOD_LODMIN(t->reg[tLOD].u) << 6;
  t->lodmax = (s32)TEXLOD_LODMAX(t->reg[tLOD].u) << 6;
  t->lodbias = (s8)(TEXLOD_LODBIAS(t->reg[tLOD].u) << 2) << 4;

  /* determine which LODs are present */
  t->lodmask = 0x1ff;
  if (TEXLOD_LOD_TSPLIT(t->reg[tLOD].u))
  {
    if (!TEXLOD_LOD_ODD(t->reg[tLOD].u))
      t->lodmask = 0x155;
    else
      t->lodmask = 0x0aa;
  }

  /* determine base texture width/height */
  t->wmask = t->hmask = 0xff;
  if (TEXLOD_LOD_S_IS_WIDER(t->reg[tLOD].u))
    t->hmask >>= TEXLOD_LOD_ASPECT(t->reg[tLOD].u);
  else
    t->wmask >>= TEXLOD_LOD_ASPECT(t->reg[tLOD].u);

  /* determine the bpp of the texture */
  bppscale = (int)TEXMODE_FORMAT(t->reg[textureMode].u) >> 3;

  /* start with the base of LOD 0 */
  if (t->texaddr_shift == 0 && (t->reg[texBaseAddr].u & 1))
    Log_WarningPrintf("Tiled texture");
  base = (t->reg[texBaseAddr].u & t->texaddr_mask) << t->texaddr_shift;
  t->lodoffset[0] = base & t->mask;

  /* LODs 1-3 are different depending on whether we are in multitex mode */
  /* Several Voodoo 2 games leave the upper bits of TLOD == 0xff, meaning we think */
  /* they want multitex mode when they really don't -- disable for now */
  if (0) // TEXLOD_TMULTIBASEADDR(t->reg[tLOD].u))
  {
    base = (t->reg[texBaseAddr_1].u & t->texaddr_mask) << t->texaddr_shift;
    t->lodoffset[1] = base & t->mask;
    base = (t->reg[texBaseAddr_2].u & t->texaddr_mask) << t->texaddr_shift;
    t->lodoffset[2] = base & t->mask;
    base = (t->reg[texBaseAddr_3_8].u & t->texaddr_mask) << t->texaddr_shift;
    t->lodoffset[3] = base & t->mask;
  }
  else
  {
    if (t->lodmask & (1 << 0))
      base += (((t->wmask >> 0) + 1) * ((t->hmask >> 0) + 1)) << bppscale;
    t->lodoffset[1] = base & t->mask;
    if (t->lodmask & (1 << 1))
      base += (((t->wmask >> 1) + 1) * ((t->hmask >> 1) + 1)) << bppscale;
    t->lodoffset[2] = base & t->mask;
    if (t->lodmask & (1 << 2))
      base += (((t->wmask >> 2) + 1) * ((t->hmask >> 2) + 1)) << bppscale;
    t->lodoffset[3] = base & t->mask;
  }

  /* remaining LODs make sense */
  for (lod = 4; lod <= 8; lod++)
  {
    if (t->lodmask & (1u << (lod - 1)))
    {
      u32 size = ((t->wmask >> (lod - 1)) + 1) * ((t->hmask >> (lod - 1)) + 1);
      if (size < 4)
        size = 4;
      base += size << bppscale;
    }
    t->lodoffset[lod] = base & t->mask;
  }

  /* set the NCC lookup appropriately */
  t->texel[1] = t->texel[9] = t->ncc[TEXMODE_NCC_TABLE_SELECT(t->reg[textureMode].u)].texel;

  /* pick the lookup table */
  t->lookup = t->texel[TEXMODE_FORMAT(t->reg[textureMode].u)];

  /* compute the detail parameters */
  t->detailmax = TEXDETAIL_DETAIL_MAX(t->reg[tDetail].u);
  t->detailbias = (s8)(TEXDETAIL_DETAIL_BIAS(t->reg[tDetail].u) << 2) << 6;
  t->detailscale = TEXDETAIL_DETAIL_SCALE(t->reg[tDetail].u);

  /* no longer dirty */
  t->regdirty = false;

  /* check for separate RGBA filtering */
  if (TEXDETAIL_SEPARATE_RGBA_FILTER(t->reg[tDetail].u))
    Panic("Separate RGBA filters!");
}

inline s32 prepare_tmu(tmu_state* t)
{
  s64 texdx, texdy;
  s32 lodbase;

  /* if the texture parameters are dirty, update them */
  if (t->regdirty)
  {
    recompute_texture_params(t);

    /* ensure that the NCC tables are up to date */
    if ((TEXMODE_FORMAT(t->reg[textureMode].u) & 7) == 1)
    {
      ncc_table* n = &t->ncc[TEXMODE_NCC_TABLE_SELECT(t->reg[textureMode].u)];
      t->texel[1] = t->texel[9] = n->texel;
      if (n->dirty)
        ncc_table_update(n);
    }
  }

  /* compute (ds^2 + dt^2) in both X and Y as 28.36 numbers */
  texdx = (s64)(t->dsdx >> 14) * (s64)(t->dsdx >> 14) + (s64)(t->dtdx >> 14) * (s64)(t->dtdx >> 14);
  texdy = (s64)(t->dsdy >> 14) * (s64)(t->dsdy >> 14) + (s64)(t->dtdy >> 14) * (s64)(t->dtdy >> 14);

  /* pick whichever is larger and shift off some high bits -> 28.20 */
  if (texdx < texdy)
    texdx = texdy;
  texdx >>= 16;

  /* use our fast reciprocal/log on this value; it expects input as a */
  /* 16.32 number, and returns the log of the reciprocal, so we have to */
  /* adjust the result: negative to get the log of the original value */
  /* plus 12 to account for the extra exponent, and divided by 2 to */
  /* get the log of the square root of texdx */
  (void)fast_reciplog(texdx, &lodbase);
  return (-lodbase + (12 << 8)) / 2;
}

inline s32 round_coordinate(float value)
{
  s32 result = (s32)floor(value);
  return result + (value - (float)result > 0.5f);
}

void poly_render_triangle(void* dest, poly_draw_scanline_func callback, const poly_vertex* v1, const poly_vertex* v2,
                          const poly_vertex* v3, poly_extra_data* extra)
{
  float dxdy_v1v2, dxdy_v1v3, dxdy_v2v3;
  const poly_vertex* tv;
  s32 curscan, scaninc = 1;

  s32 v1yclip, v3yclip;
  s32 v1y, v3y; //, v1x;

  /* first sort by Y */
  if (v2->y < v1->y)
  {
    tv = v1;
    v1 = v2;
    v2 = tv;
  }
  if (v3->y < v2->y)
  {
    tv = v2;
    v2 = v3;
    v3 = tv;
    if (v2->y < v1->y)
    {
      tv = v1;
      v1 = v2;
      v2 = tv;
    }
  }

  /* compute some integral X/Y vertex values */
  //	v1x = round_coordinate(v1->x);
  v1y = round_coordinate(v1->y);
  v3y = round_coordinate(v3->y);

  /* clip coordinates */
  v1yclip = v1y;
  v3yclip = v3y; // + ((poly->flags & POLYFLAG_INCLUDE_BOTTOM_EDGE) ? 1 : 0);
  if (v3yclip - v1yclip <= 0)
    return;

  /* compute the slopes for each portion of the triangle */
  dxdy_v1v2 = (v2->y == v1->y) ? 0.0f : (v2->x - v1->x) / (v2->y - v1->y);
  dxdy_v1v3 = (v3->y == v1->y) ? 0.0f : (v3->x - v1->x) / (v3->y - v1->y);
  dxdy_v2v3 = (v3->y == v2->y) ? 0.0f : (v3->x - v2->x) / (v3->y - v2->y);

  poly_extent* extent = new poly_extent;
  int extnum = 0;
  for (curscan = v1yclip; curscan < v3yclip; curscan += scaninc)
  {
    {
      float fully = (float)(curscan + extnum) + 0.5f;
      float startx = v1->x + (fully - v1->y) * dxdy_v1v3;
      float stopx;
      s32 istartx, istopx;

      /* compute the ending X based on which part of the triangle we're in */
      if (fully < v2->y)
        stopx = v1->x + (fully - v1->y) * dxdy_v1v2;
      else
        stopx = v2->x + (fully - v2->y) * dxdy_v2v3;

      /* clamp to full pixels */
      istartx = round_coordinate(startx);
      istopx = round_coordinate(stopx);

      /* force start < stop */
      if (istartx > istopx)
      {
        s32 temp = istartx;
        istartx = istopx;
        istopx = temp;
      }

      /* set the extent and update the total pixel count */
      if (istartx >= istopx)
        istartx = istopx = 0;

      extent->startx = istartx;
      extent->stopx = istopx;
      (callback)(dest, curscan, extent, extra);
    }
  }

  delete extent;
}

void poly_render_triangle_custom(void* dest, int startscanline, int numscanlines, const poly_extent* extents,
                                 poly_extra_data* extra)
{
  s32 curscan, scaninc;
  s32 v1yclip, v3yclip;

  v1yclip = startscanline;
  v3yclip = startscanline + numscanlines;

  if (v3yclip - v1yclip <= 0)
    return;

  for (curscan = v1yclip; curscan < v3yclip; curscan += scaninc)
  {
    tri_work_unit* unit = new tri_work_unit;
    int extnum = 0;

    /* determine how much to advance to hit the next bucket */
    scaninc = 1;

    {
      const poly_extent* extent = &extents[(curscan + extnum) - startscanline];
      s32 istartx = extent->startx, istopx = extent->stopx;

      /* force start < stop */
      if (istartx > istopx)
      {
        s32 temp = istartx;
        istartx = istopx;
        istopx = temp;
      }

      /* set the extent and update the total pixel count */
      unit->extent[extnum].startx = (s16)istartx;
      unit->extent[extnum].stopx = (s16)istopx;
      raster_fastfill(dest, curscan, extent, extra);
    }
    delete unit;
  }
}

/*************************************
 *
 *  Statistics management
 *
 *************************************/

static void accumulate_statistics(voodoo_state* v, const stats_block* stats)
{
  /* apply internal voodoo statistics */
  v->reg[fbiPixelsIn].u += (u32)stats->pixels_in;
  v->reg[fbiPixelsOut].u += (u32)stats->pixels_out;
  v->reg[fbiChromaFail].u += (u32)stats->chroma_fail;
  v->reg[fbiZfuncFail].u += (u32)stats->zfunc_fail;
  v->reg[fbiAfuncFail].u += (u32)stats->afunc_fail;
}

static void update_statistics(voodoo_state* v, bool accumulate)
{
  /* accumulate/reset statistics from all units */
  if (accumulate)
    accumulate_statistics(v, &v->thread_stats[0]);
  memset(&v->thread_stats[0], 0, sizeof(v->thread_stats[0]));

  /* accumulate/reset statistics from the LFB */
  if (accumulate)
    accumulate_statistics(v, &v->fbi.lfb_stats);
  memset(&v->fbi.lfb_stats, 0, sizeof(v->fbi.lfb_stats));
}

/*************************************
 *
 *  Voodoo register writes
 *
 *************************************/
static void voodoo_register_w(voodoo_state* v, u32 offset, u32 data)
{
  //	voodoo_reg reg;
  u32 regnum = (offset)&0xff;
  u32 chips = (offset >> 8) & 0xf;
  //	reg.u = data;

  s64 data64;

  Log_DebugPrintf("V3D:WR chip %x reg %x value %08x(%s)", chips, regnum << 2, data, voodoo_reg_name[regnum]);

  if (chips == 0)
    chips = 0xf;
  chips &= v->chipmask;

  /* the first 64 registers can be aliased differently */
  if ((offset & 0x800c0) == 0x80000 && v->alt_regmap)
    regnum = register_alias_map[offset & 0x3f];
  else
    regnum = offset & 0xff;

  /* first make sure this register is readable */
  if (!(v->regaccess[regnum] & REGISTER_WRITE))
  {
    if (regnum <= 0xe0)
      Log_ErrorPrintf("Invalid attempt to write %s", voodoo_reg_name[regnum]);
    else
      Log_ErrorPrintf("Invalid attempt to write #%x", regnum);

    return;
  }

  /* switch off the register */
  switch (regnum)
  {
    /* Vertex data is 12.4 formatted fixed point */
    case fvertexAx:
      data = (u32)float_to_int32(data, 4);
    case vertexAx:
      if (chips & 1)
        v->fbi.ax = (s16)(data & 0xffff);
      break;

    case fvertexAy:
      data = (u32)float_to_int32(data, 4);
    case vertexAy:
      if (chips & 1)
        v->fbi.ay = (s16)(data & 0xffff);
      break;

    case fvertexBx:
      data = (u32)float_to_int32(data, 4);
    case vertexBx:
      if (chips & 1)
        v->fbi.bx = (s16)(data & 0xffff);
      break;

    case fvertexBy:
      data = (u32)float_to_int32(data, 4);
    case vertexBy:
      if (chips & 1)
        v->fbi.by = (s16)(data & 0xffff);
      break;

    case fvertexCx:
      data = (u32)float_to_int32(data, 4);
    case vertexCx:
      if (chips & 1)
        v->fbi.cx = (s16)(data & 0xffff);
      break;

    case fvertexCy:
      data = (u32)float_to_int32(data, 4);
    case vertexCy:
      if (chips & 1)
        v->fbi.cy = (s16)(data & 0xffff);
      break;

    /* RGB data is 12.12 formatted fixed point */
    case fstartR:
      data = (u32)float_to_int32(data, 12);
    case startR:
      if (chips & 1)
        v->fbi.startr = (s32)(data << 8) >> 8;
      break;

    case fstartG:
      data = (u32)float_to_int32(data, 12);
    case startG:
      if (chips & 1)
        v->fbi.startg = (s32)(data << 8) >> 8;
      break;

    case fstartB:
      data = (u32)float_to_int32(data, 12);
    case startB:
      if (chips & 1)
        v->fbi.startb = (s32)(data << 8) >> 8;
      break;

    case fstartA:
      data = (u32)float_to_int32(data, 12);
    case startA:
      if (chips & 1)
        v->fbi.starta = (s32)(data << 8) >> 8;
      break;

    case fdRdX:
      data = (u32)float_to_int32(data, 12);
    case dRdX:
      if (chips & 1)
        v->fbi.drdx = (s32)(data << 8) >> 8;
      break;

    case fdGdX:
      data = (u32)float_to_int32(data, 12);
    case dGdX:
      if (chips & 1)
        v->fbi.dgdx = (s32)(data << 8) >> 8;
      break;

    case fdBdX:
      data = (u32)float_to_int32(data, 12);
    case dBdX:
      if (chips & 1)
        v->fbi.dbdx = (s32)(data << 8) >> 8;
      break;

    case fdAdX:
      data = (u32)float_to_int32(data, 12);
    case dAdX:
      if (chips & 1)
        v->fbi.dadx = (s32)(data << 8) >> 8;
      break;

    case fdRdY:
      data = (u32)float_to_int32(data, 12);
    case dRdY:
      if (chips & 1)
        v->fbi.drdy = (s32)(data << 8) >> 8;
      break;

    case fdGdY:
      data = (u32)float_to_int32(data, 12);
    case dGdY:
      if (chips & 1)
        v->fbi.dgdy = (s32)(data << 8) >> 8;
      break;

    case fdBdY:
      data = (u32)float_to_int32(data, 12);
    case dBdY:
      if (chips & 1)
        v->fbi.dbdy = (s32)(data << 8) >> 8;
      break;

    case fdAdY:
      data = (u32)float_to_int32(data, 12);
    case dAdY:
      if (chips & 1)
        v->fbi.dady = (s32)(data << 8) >> 8;
      break;

    /* Z data is 20.12 formatted fixed point */
    case fstartZ:
      data = (u32)float_to_int32(data, 12);
    case startZ:
      if (chips & 1)
        v->fbi.startz = (s32)data;
      break;

    case fdZdX:
      data = (u32)float_to_int32(data, 12);
    case dZdX:
      if (chips & 1)
        v->fbi.dzdx = (s32)data;
      break;

    case fdZdY:
      data = (u32)float_to_int32(data, 12);
    case dZdY:
      if (chips & 1)
        v->fbi.dzdy = (s32)data;
      break;

    /* S,T data is 14.18 formatted fixed point, converted to 16.32 internally */
    case fstartS:
      data64 = float_to_int64(data, 32);
      if (chips & 2)
        v->tmu[0].starts = data64;
      if (chips & 4)
        v->tmu[1].starts = data64;
      break;
    case startS:
      if (chips & 2)
        v->tmu[0].starts = (s64)(s32)data << 14;
      if (chips & 4)
        v->tmu[1].starts = (s64)(s32)data << 14;
      break;

    case fstartT:
      data64 = float_to_int64(data, 32);
      if (chips & 2)
        v->tmu[0].startt = data64;
      if (chips & 4)
        v->tmu[1].startt = data64;
      break;
    case startT:
      if (chips & 2)
        v->tmu[0].startt = (s64)(s32)data << 14;
      if (chips & 4)
        v->tmu[1].startt = (s64)(s32)data << 14;
      break;

    case fdSdX:
      data64 = float_to_int64(data, 32);
      if (chips & 2)
        v->tmu[0].dsdx = data64;
      if (chips & 4)
        v->tmu[1].dsdx = data64;
      break;
    case dSdX:
      if (chips & 2)
        v->tmu[0].dsdx = (s64)(s32)data << 14;
      if (chips & 4)
        v->tmu[1].dsdx = (s64)(s32)data << 14;
      break;

    case fdTdX:
      data64 = float_to_int64(data, 32);
      if (chips & 2)
        v->tmu[0].dtdx = data64;
      if (chips & 4)
        v->tmu[1].dtdx = data64;
      break;
    case dTdX:
      if (chips & 2)
        v->tmu[0].dtdx = (s64)(s32)data << 14;
      if (chips & 4)
        v->tmu[1].dtdx = (s64)(s32)data << 14;
      break;

    case fdSdY:
      data64 = float_to_int64(data, 32);
      if (chips & 2)
        v->tmu[0].dsdy = data64;
      if (chips & 4)
        v->tmu[1].dsdy = data64;
      break;
    case dSdY:
      if (chips & 2)
        v->tmu[0].dsdy = (s64)(s32)data << 14;
      if (chips & 4)
        v->tmu[1].dsdy = (s64)(s32)data << 14;
      break;

    case fdTdY:
      data64 = float_to_int64(data, 32);
      if (chips & 2)
        v->tmu[0].dtdy = data64;
      if (chips & 4)
        v->tmu[1].dtdy = data64;
      break;
    case dTdY:
      if (chips & 2)
        v->tmu[0].dtdy = (s64)(s32)data << 14;
      if (chips & 4)
        v->tmu[1].dtdy = (s64)(s32)data << 14;
      break;

    /* W data is 2.30 formatted fixed point, converted to 16.32 internally */
    case fstartW:
      data64 = float_to_int64(data, 32);
      if (chips & 1)
        v->fbi.startw = data64;
      if (chips & 2)
        v->tmu[0].startw = data64;
      if (chips & 4)
        v->tmu[1].startw = data64;
      break;
    case startW:
      if (chips & 1)
        v->fbi.startw = (s64)(s32)data << 2;
      if (chips & 2)
        v->tmu[0].startw = (s64)(s32)data << 2;
      if (chips & 4)
        v->tmu[1].startw = (s64)(s32)data << 2;
      break;

    case fdWdX:
      data64 = float_to_int64(data, 32);
      if (chips & 1)
        v->fbi.dwdx = data64;
      if (chips & 2)
        v->tmu[0].dwdx = data64;
      if (chips & 4)
        v->tmu[1].dwdx = data64;
      break;
    case dWdX:
      if (chips & 1)
        v->fbi.dwdx = (s64)(s32)data << 2;
      if (chips & 2)
        v->tmu[0].dwdx = (s64)(s32)data << 2;
      if (chips & 4)
        v->tmu[1].dwdx = (s64)(s32)data << 2;
      break;

    case fdWdY:
      data64 = float_to_int64(data, 32);
      if (chips & 1)
        v->fbi.dwdy = data64;
      if (chips & 2)
        v->tmu[0].dwdy = data64;
      if (chips & 4)
        v->tmu[1].dwdy = data64;
      break;
    case dWdY:
      if (chips & 1)
        v->fbi.dwdy = (s64)(s32)data << 2;
      if (chips & 2)
        v->tmu[0].dwdy = (s64)(s32)data << 2;
      if (chips & 4)
        v->tmu[1].dwdy = (s64)(s32)data << 2;
      break;

    /* setup bits */
    case sARGB:
      if (chips & 1)
      {
        v->reg[sAlpha].f = (float)RGB_ALPHA(data);
        v->reg[sRed].f = (float)RGB_RED(data);
        v->reg[sGreen].f = (float)RGB_GREEN(data);
        v->reg[sBlue].f = (float)RGB_BLUE(data);
      }
      break;

    /* mask off invalid bits for different cards */
    case fbzColorPath:
      if (v->type < VOODOO_2)
        data &= 0x0fffffff;
      if (chips & 1)
        v->reg[fbzColorPath].u = data;
      break;

    case fbzMode:
      if (v->type < VOODOO_2)
        data &= 0x001fffff;
      if (chips & 1)
        v->reg[fbzMode].u = data;
      break;

    case fogMode:
      if (v->type < VOODOO_2)
        data &= 0x0000003f;
      if (chips & 1)
        v->reg[fogMode].u = data;
      break;

    /* triangle drawing */
    case triangleCMD:
      triangle(v);
      break;

    case ftriangleCMD:
      triangle(v);
      break;

    case sBeginTriCMD:
      //			AssertMsg(false, "begin tri");
      begin_triangle(v);
      break;

    case sDrawTriCMD:
      //			AssertMsg(false, "draw tri");
      draw_triangle(v);
      break;

    /* other commands */
    case nopCMD:
      if (data & 1)
        reset_counters(v);
      if (data & 2)
        v->reg[fbiTrianglesOut].u = 0;
      break;

    case fastfillCMD:
      fastfill(v);
      break;

    case swapbufferCMD:
      swapbuffer(v, data);
      break;

    /* gamma table access -- Voodoo/Voodoo2 only */
    case clutData:
      /*			if (chips & 1)
            {
              if (!FBIINIT1_VIDEO_TIMING_RESET(v->reg[fbiInit1].u))
              {
                int index = data >> 24;
                if (index <= 32)
                {
      //						v->fbi.clut[index] = data;
                }
              }
              else
                LOG(LOG_VOODOO,LOG_WARN)("clutData ignored because video timing reset = 1\n");
            } */
      break;

    /* external DAC access -- Voodoo/Voodoo2 only */
    case dacData:
      if (chips & 1)
      {
        if (!(data & 0x800))
          dacdata_w(&v->dac, (data >> 8) & 7, data & 0xff);
        else
          dacdata_r(&v->dac, (data >> 8) & 7);
      }
      break;

    /* vertical sync rate -- Voodoo/Voodoo2 only */
    case hSync:
    case vSync:
    case backPorch:
    case videoDimensions:
      if (chips & 1)
      {
        v->reg[regnum].u = data;
        if (v->reg[hSync].u != 0 && v->reg[vSync].u != 0 && v->reg[videoDimensions].u != 0)
        {
          int htotal = (int)(((v->reg[hSync].u >> 16) & 0x3ff) + 1u + (v->reg[hSync].u & 0xff) + 1u);
          int vtotal = (int)(((v->reg[vSync].u >> 16) & 0xfff) + (v->reg[vSync].u & 0xfff));
          int hvis = v->reg[videoDimensions].u & 0x3ff;
          int vvis = (v->reg[videoDimensions].u >> 16) & 0x3ff;
          int hbp = (int)((v->reg[backPorch].u & 0xff) + 2u);
          int vbp = (int)((v->reg[backPorch].u >> 16) & 0xff);
          rectangle visarea;

          /* create a new visarea */
          visarea.min_x = hbp;
          visarea.max_x = hbp + hvis - 1;
          visarea.min_y = vbp;
          visarea.max_y = vbp + vvis - 1;

          /* keep within bounds */
          visarea.max_x = MIN(visarea.max_x, htotal - 1);
          visarea.max_y = MIN(visarea.max_y, vtotal - 1);

          Log_DebugPrintf("hSync=%08X  vSync=%08X  backPorch=%08X  videoDimensions=%08X\n", v->reg[hSync].u,
                          v->reg[vSync].u, v->reg[backPorch].u, v->reg[videoDimensions].u);
          Log_DebugPrintf("Horiz: %d-%d (%d total)  Vert: %d-%d (%d total) -- ", visarea.min_x, visarea.max_x, htotal,
                          visarea.min_y, visarea.max_y, vtotal);

          /* configure the new framebuffer info */
          u32 new_width = ((u32)hvis + 1u) & ~1u;
          u32 new_height = ((u32)vvis + 1u) & ~1u;
          if ((v->fbi.width != new_width) || (v->fbi.height != new_height))
          {
            Log_DebugPrintf("Display resized %ux%u", new_width, new_height);
            v->fbi.width = new_width;
            v->fbi.height = new_height;
            v->display->ResizeFramebuffer(new_width, new_height);
          }
          //					v->fbi.xoffs = hbp;
          //					v->fbi.yoffs = vbp;
          //					v->fbi.vsyncscan = (v->reg[vSync].u >> 16) & 0xfff;

          // TODO: better handle this.. use the PLL config
          if (new_height <= 480)
            v->display_timing.SetPixelClock(25.175 * 1000000.0); // 640x480 @ 60hz
          else if (new_height <= 600)
            v->display_timing.SetPixelClock(40.000 * 1000000.0); // 800x600 @ 60hz
          else                                                   // if (new_height <= 1024)
            v->display_timing.SetPixelClock(65.000 * 1000000.0); // 1024x768 @ 60hz

          v->display_timing.SetHorizontalRange(hvis + 1, hvis + 1, htotal - hbp, htotal);
          v->display_timing.SetVerticalRange(vvis + 1, hvis + 1, vtotal - vbp, vtotal);
          v->display_timing.LogFrequencies("voodoo");

          /* recompute the time of VBLANK */
          if (v->display_timing.IsValid())
          {
            v->retrace_event->SetFrequency(v->display_timing.GetVerticalFrequency());
            v->retrace_event->SetActive(v->clock_enabled);
            if (v->clock_enabled)
              v->display_timing.ResetClock(v->timing_manager->GetTotalEmulatedTime());
          }
          else
          {
            v->retrace_event->SetActive(false);
          }

          /* if changing dimensions, update video memory layout */
          if (regnum == videoDimensions)
            recompute_video_memory(v);
        }
      }
      break;

    /* fbiInit0 can only be written if initEnable says we can -- Voodoo/Voodoo2 only */
    case fbiInit0:
      if ((chips & 1) && INITEN_ENABLE_HW_INIT(v->pci.init_enable))
      {
        v->reg[fbiInit0].u = data;
        v->output_on = FBIINIT0_VGA_PASSTHRU(data);
        v->display->SetEnable(v->output_on);
        if (FBIINIT0_GRAPHICS_RESET(data))
          soft_reset(v);
        recompute_video_memory(v);
      }
      break;

    /* fbiInit5-7 are Voodoo 2-only; ignore them on anything else */
    case fbiInit5:
    case fbiInit6:
      if (v->type < VOODOO_2)
        break;
      /* else fall through... */

    /* fbiInitX can only be written if initEnable says we can -- Voodoo/Voodoo2 only */
    /* most of these affect memory layout, so always recompute that when done */
    case fbiInit1:
    case fbiInit2:
    case fbiInit4:
      if ((chips & 1) && INITEN_ENABLE_HW_INIT(v->pci.init_enable))
      {
        v->reg[regnum].u = data;
        recompute_video_memory(v);
      }
      break;

    case fbiInit3:
      if ((chips & 1) && INITEN_ENABLE_HW_INIT(v->pci.init_enable))
      {
        v->reg[regnum].u = data;
        v->alt_regmap = (FBIINIT3_TRI_REGISTER_REMAP(data) > 0);
        v->fbi.yorigin = FBIINIT3_YORIGIN_SUBTRACT(v->reg[fbiInit3].u);
        recompute_video_memory(v);
      }
      break;

    /* nccTable entries are processed and expanded immediately */
    case nccTable + 0:
    case nccTable + 1:
    case nccTable + 2:
    case nccTable + 3:
    case nccTable + 4:
    case nccTable + 5:
    case nccTable + 6:
    case nccTable + 7:
    case nccTable + 8:
    case nccTable + 9:
    case nccTable + 10:
    case nccTable + 11:
      if (chips & 2)
        ncc_table_write(&v->tmu[0].ncc[0], regnum - nccTable, data);
      if (chips & 4)
        ncc_table_write(&v->tmu[1].ncc[0], regnum - nccTable, data);
      break;

    case nccTable + 12:
    case nccTable + 13:
    case nccTable + 14:
    case nccTable + 15:
    case nccTable + 16:
    case nccTable + 17:
    case nccTable + 18:
    case nccTable + 19:
    case nccTable + 20:
    case nccTable + 21:
    case nccTable + 22:
    case nccTable + 23:
      if (chips & 2)
        ncc_table_write(&v->tmu[0].ncc[1], regnum - (nccTable + 12), data);
      if (chips & 4)
        ncc_table_write(&v->tmu[1].ncc[1], regnum - (nccTable + 12), data);
      break;

    /* fogTable entries are processed and expanded immediately */
    case fogTable + 0:
    case fogTable + 1:
    case fogTable + 2:
    case fogTable + 3:
    case fogTable + 4:
    case fogTable + 5:
    case fogTable + 6:
    case fogTable + 7:
    case fogTable + 8:
    case fogTable + 9:
    case fogTable + 10:
    case fogTable + 11:
    case fogTable + 12:
    case fogTable + 13:
    case fogTable + 14:
    case fogTable + 15:
    case fogTable + 16:
    case fogTable + 17:
    case fogTable + 18:
    case fogTable + 19:
    case fogTable + 20:
    case fogTable + 21:
    case fogTable + 22:
    case fogTable + 23:
    case fogTable + 24:
    case fogTable + 25:
    case fogTable + 26:
    case fogTable + 27:
    case fogTable + 28:
    case fogTable + 29:
    case fogTable + 30:
    case fogTable + 31:
      if (chips & 1)
      {
        int base = 2 * (int)(regnum - fogTable);
        v->fbi.fogdelta[base + 0] = (data >> 0) & 0xff;
        v->fbi.fogblend[base + 0] = (data >> 8) & 0xff;
        v->fbi.fogdelta[base + 1] = (data >> 16) & 0xff;
        v->fbi.fogblend[base + 1] = (data >> 24) & 0xff;
      }
      break;

    /* texture modifications cause us to recompute everything */
    case textureMode:
    case tLOD:
    case tDetail:
    case texBaseAddr:
    case texBaseAddr_1:
    case texBaseAddr_2:
    case texBaseAddr_3_8:
      if (chips & 2)
      {
        v->tmu[0].reg[regnum].u = data;
        v->tmu[0].regdirty = true;
      }
      if (chips & 4)
      {
        v->tmu[1].reg[regnum].u = data;
        v->tmu[1].regdirty = true;
      }
      break;

    case trexInit1:
      /* send tmu config data to the frame buffer */
      v->send_config = (TREXINIT_SEND_TMU_CONFIG(data) > 0);
      goto default_case;
      break;

    case clipLowYHighY:
    case clipLeftRight:
      if (chips & 1)
        v->reg[0x000 + regnum].u = data;
      break;

    /* these registers are referenced in the renderer; we must wait for pending work before changing */
    case chromaRange:
    case chromaKey:
    case alphaMode:
    case fogColor:
    case stipple:
    case zaColor:
    case color1:
    case color0:
      /* fall through to default implementation */

    /* by default, just feed the data to the chips */
    default:
    default_case:
      if (chips & 1)
        v->reg[0x000 + regnum].u = data;
      if (chips & 2)
        v->reg[0x100 + regnum].u = data;
      if (chips & 4)
        v->reg[0x200 + regnum].u = data;
      if (chips & 8)
        v->reg[0x300 + regnum].u = data;
      break;
  }
}

/*************************************
 *
 *  Voodoo LFB writes
 *
 *************************************/
static void voodoo_lfb_w(voodoo_state* v, u32 offset, u32 data, u32 mem_mask)
{
  Log_DevPrintf("V3D:WR LFB offset %X value %08X", offset, data);
  u16 *dest, *depth;
  u32 destmax, depthmax;

  int sr[2], sg[2], sb[2], sa[2], sw[2];
  int x, y, scry, mask;
  int pix, destbuf;

  /* byte swizzling */
  if (LFBMODE_BYTE_SWIZZLE_WRITES(v->reg[lfbMode].u))
  {
    data = FLIPENDIAN_INT32(data);
    mem_mask = FLIPENDIAN_INT32(mem_mask);
  }

  /* word swapping */
  if (LFBMODE_WORD_SWAP_WRITES(v->reg[lfbMode].u))
  {
    data = (data << 16) | (data >> 16);
    mem_mask = (mem_mask << 16) | (mem_mask >> 16);
  }

  /* extract default depth and alpha values */
  sw[0] = sw[1] = (int)(v->reg[zaColor].u & 0xffff);
  sa[0] = sa[1] = (int)(v->reg[zaColor].u >> 24);

  /* first extract A,R,G,B from the data */
  switch (LFBMODE_WRITE_FORMAT(v->reg[lfbMode].u) + 16 * LFBMODE_RGBA_LANES(v->reg[lfbMode].u))
  {
    case 16 * 0 + 0: /* ARGB, 16-bit RGB 5-6-5 */
    case 16 * 2 + 0: /* RGBA, 16-bit RGB 5-6-5 */
      EXTRACT_565_TO_888(data, sr[0], sg[0], sb[0]);
      EXTRACT_565_TO_888(data >> 16, sr[1], sg[1], sb[1]);
      mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
      offset <<= 1;
      break;
    case 16 * 1 + 0: /* ABGR, 16-bit RGB 5-6-5 */
    case 16 * 3 + 0: /* BGRA, 16-bit RGB 5-6-5 */
      EXTRACT_565_TO_888(data, sb[0], sg[0], sr[0]);
      EXTRACT_565_TO_888(data >> 16, sb[1], sg[1], sr[1]);
      mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
      offset <<= 1;
      break;

    case 16 * 0 + 1: /* ARGB, 16-bit RGB x-5-5-5 */
      EXTRACT_x555_TO_888(data, sr[0], sg[0], sb[0]);
      EXTRACT_x555_TO_888(data >> 16, sr[1], sg[1], sb[1]);
      mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
      offset <<= 1;
      break;
    case 16 * 1 + 1: /* ABGR, 16-bit RGB x-5-5-5 */
      EXTRACT_x555_TO_888(data, sb[0], sg[0], sr[0]);
      EXTRACT_x555_TO_888(data >> 16, sb[1], sg[1], sr[1]);
      mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
      offset <<= 1;
      break;
    case 16 * 2 + 1: /* RGBA, 16-bit RGB x-5-5-5 */
      EXTRACT_555x_TO_888(data, sr[0], sg[0], sb[0]);
      EXTRACT_555x_TO_888(data >> 16, sr[1], sg[1], sb[1]);
      mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
      offset <<= 1;
      break;
    case 16 * 3 + 1: /* BGRA, 16-bit RGB x-5-5-5 */
      EXTRACT_555x_TO_888(data, sb[0], sg[0], sr[0]);
      EXTRACT_555x_TO_888(data >> 16, sb[1], sg[1], sr[1]);
      mask = LFB_RGB_PRESENT | (LFB_RGB_PRESENT << 4);
      offset <<= 1;
      break;

    case 16 * 0 + 2: /* ARGB, 16-bit ARGB 1-5-5-5 */
      EXTRACT_1555_TO_8888(data, sa[0], sr[0], sg[0], sb[0]);
      EXTRACT_1555_TO_8888(data >> 16, sa[1], sr[1], sg[1], sb[1]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | ((LFB_RGB_PRESENT | LFB_ALPHA_PRESENT) << 4);
      offset <<= 1;
      break;
    case 16 * 1 + 2: /* ABGR, 16-bit ARGB 1-5-5-5 */
      EXTRACT_1555_TO_8888(data, sa[0], sb[0], sg[0], sr[0]);
      EXTRACT_1555_TO_8888(data >> 16, sa[1], sb[1], sg[1], sr[1]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | ((LFB_RGB_PRESENT | LFB_ALPHA_PRESENT) << 4);
      offset <<= 1;
      break;
    case 16 * 2 + 2: /* RGBA, 16-bit ARGB 1-5-5-5 */
      EXTRACT_5551_TO_8888(data, sr[0], sg[0], sb[0], sa[0]);
      EXTRACT_5551_TO_8888(data >> 16, sr[1], sg[1], sb[1], sa[1]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | ((LFB_RGB_PRESENT | LFB_ALPHA_PRESENT) << 4);
      offset <<= 1;
      break;
    case 16 * 3 + 2: /* BGRA, 16-bit ARGB 1-5-5-5 */
      EXTRACT_5551_TO_8888(data, sb[0], sg[0], sr[0], sa[0]);
      EXTRACT_5551_TO_8888(data >> 16, sb[1], sg[1], sr[1], sa[1]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | ((LFB_RGB_PRESENT | LFB_ALPHA_PRESENT) << 4);
      offset <<= 1;
      break;

    case 16 * 0 + 4: /* ARGB, 32-bit RGB x-8-8-8 */
      EXTRACT_x888_TO_888(data, sr[0], sg[0], sb[0]);
      mask = LFB_RGB_PRESENT;
      break;
    case 16 * 1 + 4: /* ABGR, 32-bit RGB x-8-8-8 */
      EXTRACT_x888_TO_888(data, sb[0], sg[0], sr[0]);
      mask = LFB_RGB_PRESENT;
      break;
    case 16 * 2 + 4: /* RGBA, 32-bit RGB x-8-8-8 */
      EXTRACT_888x_TO_888(data, sr[0], sg[0], sb[0]);
      mask = LFB_RGB_PRESENT;
      break;
    case 16 * 3 + 4: /* BGRA, 32-bit RGB x-8-8-8 */
      EXTRACT_888x_TO_888(data, sb[0], sg[0], sr[0]);
      mask = LFB_RGB_PRESENT;
      break;

    case 16 * 0 + 5: /* ARGB, 32-bit ARGB 8-8-8-8 */
      EXTRACT_8888_TO_8888(data, sa[0], sr[0], sg[0], sb[0]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
      break;
    case 16 * 1 + 5: /* ABGR, 32-bit ARGB 8-8-8-8 */
      EXTRACT_8888_TO_8888(data, sa[0], sb[0], sg[0], sr[0]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
      break;
    case 16 * 2 + 5: /* RGBA, 32-bit ARGB 8-8-8-8 */
      EXTRACT_8888_TO_8888(data, sr[0], sg[0], sb[0], sa[0]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
      break;
    case 16 * 3 + 5: /* BGRA, 32-bit ARGB 8-8-8-8 */
      EXTRACT_8888_TO_8888(data, sb[0], sg[0], sr[0], sa[0]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT;
      break;

    case 16 * 0 + 12: /* ARGB, 32-bit depth+RGB 5-6-5 */
    case 16 * 2 + 12: /* RGBA, 32-bit depth+RGB 5-6-5 */
      sw[0] = (int)(data >> 16);
      EXTRACT_565_TO_888(data, sr[0], sg[0], sb[0]);
      mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
      break;
    case 16 * 1 + 12: /* ABGR, 32-bit depth+RGB 5-6-5 */
    case 16 * 3 + 12: /* BGRA, 32-bit depth+RGB 5-6-5 */
      sw[0] = (int)(data >> 16);
      EXTRACT_565_TO_888(data, sb[0], sg[0], sr[0]);
      mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
      break;

    case 16 * 0 + 13: /* ARGB, 32-bit depth+RGB x-5-5-5 */
      sw[0] = (int)(data >> 16);
      EXTRACT_x555_TO_888(data, sr[0], sg[0], sb[0]);
      mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
      break;
    case 16 * 1 + 13: /* ABGR, 32-bit depth+RGB x-5-5-5 */
      sw[0] = (int)(data >> 16);
      EXTRACT_x555_TO_888(data, sb[0], sg[0], sr[0]);
      mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
      break;
    case 16 * 2 + 13: /* RGBA, 32-bit depth+RGB x-5-5-5 */
      sw[0] = (int)(data >> 16);
      EXTRACT_555x_TO_888(data, sr[0], sg[0], sb[0]);
      mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
      break;
    case 16 * 3 + 13: /* BGRA, 32-bit depth+RGB x-5-5-5 */
      sw[0] = (int)(data >> 16);
      EXTRACT_555x_TO_888(data, sb[0], sg[0], sr[0]);
      mask = LFB_RGB_PRESENT | LFB_DEPTH_PRESENT_MSW;
      break;

    case 16 * 0 + 14: /* ARGB, 32-bit depth+ARGB 1-5-5-5 */
      sw[0] = (int)(data >> 16);
      EXTRACT_1555_TO_8888(data, sa[0], sr[0], sg[0], sb[0]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT_MSW;
      break;
    case 16 * 1 + 14: /* ABGR, 32-bit depth+ARGB 1-5-5-5 */
      sw[0] = (int)(data >> 16);
      EXTRACT_1555_TO_8888(data, sa[0], sb[0], sg[0], sr[0]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT_MSW;
      break;
    case 16 * 2 + 14: /* RGBA, 32-bit depth+ARGB 1-5-5-5 */
      sw[0] = (int)(data >> 16);
      EXTRACT_5551_TO_8888(data, sr[0], sg[0], sb[0], sa[0]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT_MSW;
      break;
    case 16 * 3 + 14: /* BGRA, 32-bit depth+ARGB 1-5-5-5 */
      sw[0] = (int)(data >> 16);
      EXTRACT_5551_TO_8888(data, sb[0], sg[0], sr[0], sa[0]);
      mask = LFB_RGB_PRESENT | LFB_ALPHA_PRESENT | LFB_DEPTH_PRESENT_MSW;
      break;

    case 16 * 0 + 15: /* ARGB, 16-bit depth */
    case 16 * 1 + 15: /* ARGB, 16-bit depth */
    case 16 * 2 + 15: /* ARGB, 16-bit depth */
    case 16 * 3 + 15: /* ARGB, 16-bit depth */
      sw[0] = (int)(data & 0xffff);
      sw[1] = (int)(data >> 16);
      mask = LFB_DEPTH_PRESENT | (LFB_DEPTH_PRESENT << 4);
      offset <<= 1;
      break;

    default: /* reserved */
      return;
  }

  /* compute X,Y */
  x = (offset << 0) & ((1 << 10) - 1);
  y = (offset >> 10) & ((1 << 10) - 1);

  /* adjust the mask based on which half of the data is written */
  if (!ACCESSING_BITS_0_15)
    mask &= ~(0x0f - LFB_DEPTH_PRESENT_MSW);
  if (!ACCESSING_BITS_16_31)
    mask &= ~(0xf0 + LFB_DEPTH_PRESENT_MSW);

  /* select the target buffer */
  destbuf = LFBMODE_WRITE_BUFFER_SELECT(v->reg[lfbMode].u);
  //	LOG(LOG_VOODOO,LOG_WARN)("destbuf %X lfbmode %X",destbuf, v->reg[lfbMode].u);
  switch (destbuf)
  {
    case 0: /* front buffer */
      dest = (u16*)(v->fbi.ram + v->fbi.rgboffs[v->fbi.frontbuf]);
      destmax = (v->fbi.mask + 1 - v->fbi.rgboffs[v->fbi.frontbuf]) / 2;
      break;

    case 1: /* back buffer */
      dest = (u16*)(v->fbi.ram + v->fbi.rgboffs[v->fbi.backbuf]);
      destmax = (v->fbi.mask + 1 - v->fbi.rgboffs[v->fbi.backbuf]) / 2;
      break;

    default: /* reserved */
      Panic("reserved lfb write");
      return;
  }
  depth = (u16*)(v->fbi.ram + v->fbi.auxoffs);
  depthmax = (v->fbi.mask + 1 - v->fbi.auxoffs) / 2;

  /* simple case: no pipeline */
  if (!LFBMODE_ENABLE_PIXEL_PIPELINE(v->reg[lfbMode].u))
  {
    DECLARE_DITHER_POINTERS_NO_DITHER_VAR;
    u32 bufoffs;

    if (LOG_LFB)
    {
      Log_DevPrintf("VOODOO.LFB:write raw mode %X (%d,%d) = %08X & %08X\n", LFBMODE_WRITE_FORMAT(v->reg[lfbMode].u), x,
                    y, data, mem_mask);
    }

    /* determine the screen Y */
    scry = y;
    if (LFBMODE_Y_ORIGIN(v->reg[lfbMode].u))
      scry = ((int)v->fbi.yorigin - y) & 0x3ff;

    /* advance pointers to the proper row */
    bufoffs = (unsigned long)((long)scry * (long)v->fbi.rowpixels + (long)x);

    /* compute dithering */
    COMPUTE_DITHER_POINTERS_NO_DITHER_VAR(v->reg[fbzMode].u, y);

    /* loop over up to two pixels */
    for (pix = 0; mask; pix++)
    {
      /* make sure we care about this pixel */
      if (mask & 0x0f)
      {
        bool has_rgb = (mask & LFB_RGB_PRESENT) > 0;
        bool has_alpha = ((mask & LFB_ALPHA_PRESENT) > 0) && (FBZMODE_ENABLE_ALPHA_PLANES(v->reg[fbzMode].u) > 0);
        bool has_depth =
          ((mask & (LFB_DEPTH_PRESENT | LFB_DEPTH_PRESENT_MSW)) && !FBZMODE_ENABLE_ALPHA_PLANES(v->reg[fbzMode].u));

        /* write to the RGB buffer */
        if (has_rgb && bufoffs < destmax)
        {
          /* apply dithering and write to the screen */
          APPLY_DITHER(v->reg[fbzMode].u, x, dither_lookup, sr[pix], sg[pix], sb[pix]);
          dest[bufoffs] = (u16)((sr[pix] << 11) | (sg[pix] << 5) | sb[pix]);
        }

        /* make sure we have an aux buffer to write to */
        if (depth && bufoffs < depthmax)
        {
          /* write to the alpha buffer */
          if (has_alpha)
            depth[bufoffs] = (u16)sa[pix];

          /* write to the depth buffer */
          if (has_depth)
            depth[bufoffs] = (u16)sw[pix];
        }

        /* track pixel writes to the frame buffer regardless of mask */
        v->reg[fbiPixelsOut].u++;
      }

      /* advance our pointers */
      bufoffs++;
      x++;
      mask >>= 4;
    }
  }
  /* tricky case: run the full pixel pipeline on the pixel */
  else
  {
    DECLARE_DITHER_POINTERS;

    if (LOG_LFB)
    {
      Log_DevPrintf("LFB:write pipelined mode %X (%d,%d) = %08X & %08X", LFBMODE_WRITE_FORMAT(v->reg[lfbMode].u), x, y,
                    data, mem_mask);
    }

    /* determine the screen Y */
    scry = y;
    if (FBZMODE_Y_ORIGIN(v->reg[fbzMode].u))
      scry = ((int)v->fbi.yorigin - y) & 0x3ff;

    /* advance pointers to the proper row */
    dest += (unsigned long)((long)scry * (long)v->fbi.rowpixels);
    if (depth)
      depth += (unsigned long)((long)scry * (long)v->fbi.rowpixels);

    /* compute dithering */
    COMPUTE_DITHER_POINTERS(v->reg[fbzMode].u, y);

    /* loop over up to two pixels */
    for (pix = 0; mask; pix++)
    {
      /* make sure we care about this pixel */
      if (mask & 0x0f)
      {
        stats_block* stats = &v->fbi.lfb_stats;
        s64 iterw;
        if (LFBMODE_WRITE_W_SELECT(v->reg[lfbMode].u))
        {
          iterw = (u32)v->reg[zaColor].u << 16;
        }
        else
        {
          iterw = (u32)sw[pix] << 16;
        }
        s32 iterz = sw[pix] << 12;
        rgb_union color;
        rgb_union iterargb = {0};

        /* apply clipping */
        if (FBZMODE_ENABLE_CLIPPING(v->reg[fbzMode].u))
        {
          if (x < (s32)((v->reg[clipLeftRight].u >> 16) & 0x3ff) || x >= (s32)(v->reg[clipLeftRight].u & 0x3ff) ||
              scry < (s32)((v->reg[clipLowYHighY].u >> 16) & 0x3ff) || scry >= (s32)(v->reg[clipLowYHighY].u & 0x3ff))
          {
            stats->pixels_in++;
            stats->clip_fail++;
            goto nextpixel;
          }
        }

        /* pixel pipeline part 1 handles depth testing and stippling */
        // TODO: in the v->ogl case this macro doesn't really work with depth testing
        // PIXEL_PIPELINE_BEGIN(v, x, y, v->reg[fbzColorPath].u, v->reg[fbzMode].u, iterz, iterw);
        // Start PIXEL_PIPE_BEGIN copy
        s32 fogdepth, biasdepth;
        s32 prefogr, prefogg, prefogb;
        s32 r, g, b, a;

        (stats)->pixels_in++;

        /* apply clipping */
        /* note that for perf reasons, we assume the caller has done clipping */

        /* handle stippling */
        if (FBZMODE_ENABLE_STIPPLE(v->reg[fbzMode].u))
        {
          /* rotate mode */
          if (FBZMODE_STIPPLE_PATTERN(v->reg[fbzMode].u) == 0)
          {
            v->reg[stipple].u = (v->reg[stipple].u << 1) | (v->reg[stipple].u >> 31);
            if ((v->reg[stipple].u & 0x80000000) == 0)
            {
              goto skipdrawdepth;
            }
          }

          /* pattern mode */
          else
          {
            int stipple_index = ((y & 3) << 3) | (~x & 7);
            if (((v->reg[stipple].u >> stipple_index) & 1) == 0)
            {
              goto nextpixel;
            }
          }
        }
        // End PIXEL_PIPELINE_BEGIN COPY

        // Depth testing value for lfb pipeline writes is directly from write data, no biasing is used
        fogdepth = biasdepth = (s32)sw[pix];

        color.rgb.r = sr[pix];
        color.rgb.g = sg[pix];
        color.rgb.b = sb[pix];
        color.rgb.a = sa[pix];

        /* Perform depth testing */
        DEPTH_TEST(v, stats, x, v->reg[fbzMode].u);

        /* apply chroma key */
        APPLY_CHROMAKEY(v, stats, v->reg[fbzMode].u, color);

        /* apply alpha mask, and alpha testing */
        APPLY_ALPHAMASK(v, stats, v->reg[fbzMode].u, color.rgb.a);
        APPLY_ALPHATEST(v, stats, v->reg[alphaMode].u, color.rgb.a);

        if (FBZCP_CC_MSELECT(v->reg[fbzColorPath].u) != 0)
          Log_DevPrintf("lfbw fpp mselect %8x", FBZCP_CC_MSELECT(v->reg[fbzColorPath].u));
        if (FBZCP_CCA_MSELECT(v->reg[fbzColorPath].u) > 1)
          Log_DevPrintf("lfbw fpp mselect alpha %8x", FBZCP_CCA_MSELECT(v->reg[fbzColorPath].u));

        if (FBZCP_CC_REVERSE_BLEND(v->reg[fbzColorPath].u) != 0)
        {
          if (FBZCP_CC_MSELECT(v->reg[fbzColorPath].u) != 0)
            Log_DevPrintf("lfbw fpp rblend %8x", FBZCP_CC_REVERSE_BLEND(v->reg[fbzColorPath].u));
        }
        if (FBZCP_CCA_REVERSE_BLEND(v->reg[fbzColorPath].u) != 0)
        {
          if (FBZCP_CC_MSELECT(v->reg[fbzColorPath].u) != 0)
            Log_DevPrintf("lfbw fpp rblend alpha %8x", FBZCP_CCA_REVERSE_BLEND(v->reg[fbzColorPath].u));
        }

        s32 blendr, blendg, blendb, blenda;
        rgb_union c_local;

        /* compute c_local */
        if (FBZCP_CC_LOCALSELECT_OVERRIDE(v->reg[fbzColorPath].u) == 0)
        {
          if (FBZCP_CC_LOCALSELECT(v->reg[fbzColorPath].u) == 0) /* iterated RGB */
          {
            //						c_local.u = iterargb.u;
            c_local.rgb.r = sr[pix];
            c_local.rgb.g = sg[pix];
            c_local.rgb.b = sb[pix];
          }
          else /* color0 RGB */
          {
            c_local.u = v->reg[color0].u;
          }
        }
        else
        {
          Log_DevPrintf("lfbw fpp FBZCP_CC_LOCALSELECT_OVERRIDE set!");
          /*					if (!(texel.rgb.a & 0x80))					// iterated RGB
                      c_local.u = iterargb.u;
                    else											// color0 RGB
                      c_local.u = v->reg[color0].u; */
        }

        /* compute a_local */
        switch (FBZCP_CCA_LOCALSELECT(v->reg[fbzColorPath].u))
        {
          default:
          case 0: /* iterated alpha */
                  //						c_local.rgb.a = iterargb.rgb.a;
            c_local.rgb.a = sa[pix];
            break;
          case 1: /* color0 alpha */
            c_local.rgb.a = v->reg[color0].rgb.a;
            break;
          case 2: /* clamped iterated Z[27:20] */
          {
            int temp;
            CLAMPED_Z(iterz, v->reg[fbzColorPath].u, temp);
            c_local.rgb.a = (u8)temp;
            break;
          }
          case 3: /* clamped iterated W[39:32] */
          {
            int temp;
            CLAMPED_W(iterw, v->reg[fbzColorPath].u, temp); /* Voodoo 2 only */
            c_local.rgb.a = (u8)temp;
            break;
          }
        }

        /* select zero or c_other */
        if (FBZCP_CC_ZERO_OTHER(v->reg[fbzColorPath].u) == 0)
        {
          r = sr[pix];
          g = sg[pix];
          b = sb[pix];
        }
        else
        {
          r = g = b = 0;
        }

        /* select zero or a_other */
        if (FBZCP_CCA_ZERO_OTHER(v->reg[fbzColorPath].u) == 0)
        {
          a = sa[pix];
        }
        else
        {
          a = 0;
        }

        /* subtract c_local */
        if (FBZCP_CC_SUB_CLOCAL(v->reg[fbzColorPath].u))
        {
          r -= c_local.rgb.r;
          g -= c_local.rgb.g;
          b -= c_local.rgb.b;
        }

        /* subtract a_local */
        if (FBZCP_CCA_SUB_CLOCAL(v->reg[fbzColorPath].u))
          a -= c_local.rgb.a;

        /* blend RGB */
        switch (FBZCP_CC_MSELECT(v->reg[fbzColorPath].u))
        {
          default: /* reserved */
          case 0:  /* 0 */
            blendr = blendg = blendb = 0;
            break;
          case 1: /* c_local */
            blendr = c_local.rgb.r;
            blendg = c_local.rgb.g;
            blendb = c_local.rgb.b;
            Log_DebugPrintf("blend RGB c_local");
            break;
          case 2:                         /* a_other */
            blendr = blendg = blendb = 0; // HACK: Gotta fill them with something --J.C
                                          //						blendr = blendg = blendb = c_other.rgb.a;
            Log_DebugPrintf("blend RGB a_other");
            break;
          case 3: /* a_local */
            blendr = blendg = blendb = c_local.rgb.a;
            Log_DebugPrintf("blend RGB a_local");
            break;
          case 4:                         /* texture alpha */
            blendr = blendg = blendb = 0; // HACK: Gotta fill them with something --J.C
                                          //						blendr = blendg = blendb = texel.rgb.a;
            Log_DebugPrintf("blend RGB texture alpha");
            break;
          case 5:                         /* texture RGB (Voodoo 2 only) */
            blendr = blendg = blendb = 0; // HACK: Gotta fill them with something --J.C
                                          /*						blendr = texel.rgb.r;
                                                      blendg = texel.rgb.g;
                                                      blendb = texel.rgb.b; */
            Log_DebugPrintf("blend RGB texture RGB");
            break;
        }

        /* blend alpha */
        switch (FBZCP_CCA_MSELECT(v->reg[fbzColorPath].u))
        {
          default: /* reserved */
          case 0:  /* 0 */
            blenda = 0;
            break;
          case 1: /* a_local */
            blenda = c_local.rgb.a;
            //						LOG_MSG("blend alpha a_local");
            break;
          case 2:       /* a_other */
            blenda = 0; /* HACK: gotta fill it with something */
                        //						blenda = c_other.rgb.a;
            Log_DebugPrintf("blend alpha a_other");
            break;
          case 3: /* a_local */
            blenda = c_local.rgb.a;
            Log_DebugPrintf("blend alpha a_local");
            break;
          case 4:       /* texture alpha */
            blenda = 0; /* HACK: gotta fill it with something */
                        //						blenda = texel.rgb.a;
            Log_DebugPrintf("blend alpha texture alpha");
            break;
        }

        /* reverse the RGB blend */
        if (!FBZCP_CC_REVERSE_BLEND(v->reg[fbzColorPath].u))
        {
          blendr ^= 0xff;
          blendg ^= 0xff;
          blendb ^= 0xff;
        }

        /* reverse the alpha blend */
        if (!FBZCP_CCA_REVERSE_BLEND(v->reg[fbzColorPath].u))
          blenda ^= 0xff;

        /* do the blend */
        r = (r * (blendr + 1)) >> 8;
        g = (g * (blendg + 1)) >> 8;
        b = (b * (blendb + 1)) >> 8;
        a = (a * (blenda + 1)) >> 8;

        /* add clocal or alocal to RGB */
        switch (FBZCP_CC_ADD_ACLOCAL(v->reg[fbzColorPath].u))
        {
          case 3: /* reserved */
          case 0: /* nothing */
            break;
          case 1: /* add c_local */
            r += c_local.rgb.r;
            g += c_local.rgb.g;
            b += c_local.rgb.b;
            break;
          case 2: /* add_alocal */
            r += c_local.rgb.a;
            g += c_local.rgb.a;
            b += c_local.rgb.a;
            break;
        }

        /* add clocal or alocal to alpha */
        if (FBZCP_CCA_ADD_ACLOCAL(v->reg[fbzColorPath].u))
          a += c_local.rgb.a;

        /* clamp */
        CLAMP(r, 0x00, 0xff);
        CLAMP(g, 0x00, 0xff);
        CLAMP(b, 0x00, 0xff);
        CLAMP(a, 0x00, 0xff);

        /* invert */
        if (FBZCP_CC_INVERT_OUTPUT(v->reg[fbzColorPath].u))
        {
          r ^= 0xff;
          g ^= 0xff;
          b ^= 0xff;
        }
        if (FBZCP_CCA_INVERT_OUTPUT(v->reg[fbzColorPath].u))
          a ^= 0xff;

        /* pixel pipeline part 2 handles color combine, fog, alpha, and final output */
        PIXEL_PIPELINE_MODIFY(v, dither, dither4, x, v->reg[fbzMode].u, v->reg[fbzColorPath].u, v->reg[alphaMode].u,
                              v->reg[fogMode].u, iterz, iterw, iterargb);

        PIXEL_PIPELINE_FINISH(v, dither_lookup, x, dest, depth, v->reg[fbzMode].u);

        PIXEL_PIPELINE_END(stats);
      nextpixel:
        /* advance our pointers */
        x++;
        mask >>= 4;
      }
    }
  }
}

/*************************************
 *
 *  Voodoo texture RAM writes
 *
 *************************************/

static s32 voodoo_texture_w(voodoo_state* v, u32 offset, u32 data)
{
  int tmunum = (offset >> 19) & 0x03;
  Log_DevPrintf("V3D:write TMU%x offset %X value %X", tmunum, offset, data);

  tmu_state* t;

  /* point to the right TMU */
  if (!(v->chipmask & (2 << tmunum)))
    return 0;
  t = &v->tmu[tmunum];

  if (TEXLOD_TDIRECT_WRITE(t->reg[tLOD].u))
    Panic("Texture direct write!");

  /* update texture info if dirty */
  if (t->regdirty)
    recompute_texture_params(t);

  /* swizzle the data */
  if (TEXLOD_TDATA_SWIZZLE(t->reg[tLOD].u))
    data = FLIPENDIAN_INT32(data);
  if (TEXLOD_TDATA_SWAP(t->reg[tLOD].u))
    data = (data >> 16) | (data << 16);

  /* 8-bit texture case */
  if (TEXMODE_FORMAT(t->reg[textureMode].u) < 8)
  {
    int lod, tt, ts;
    u32 tbaseaddr;
    u8* dest;

    /* extract info */
    lod = (offset >> 15) & 0x0f;
    tt = (offset >> 7) & 0xff;

    /* old code has a bit about how this is broken in gauntleg unless we always look at TMU0 */
    if (TEXMODE_SEQ_8_DOWNLD(v->tmu[0].reg /*t->reg*/[textureMode].u))
      ts = (offset << 2) & 0xfc;
    else
      ts = (offset << 1) & 0xfc;

    /* validate parameters */
    if (lod > 8)
      return 0;

    /* compute the base address */
    tbaseaddr = t->lodoffset[lod];
    tbaseaddr += (u32)((long)tt * (((long)t->wmask >> lod) + 1l) + (long)ts);

    if (LOG_TEXTURE_RAM)
      Log_DevPrintf("Texture 8-bit w: lod=%d s=%d t=%d data=%08X\n", lod, ts, tt, data);

    /* write the four bytes in little-endian order */
    dest = t->ram;
    tbaseaddr &= t->mask;

    bool changed = false;
    if (dest[BYTE4_XOR_LE(tbaseaddr + 0)] != ((data >> 0) & 0xff))
    {
      dest[BYTE4_XOR_LE(tbaseaddr + 0)] = (data >> 0) & 0xff;
      changed = true;
    }
    if (dest[BYTE4_XOR_LE(tbaseaddr + 1)] != ((data >> 8) & 0xff))
    {
      dest[BYTE4_XOR_LE(tbaseaddr + 1)] = (data >> 8) & 0xff;
      changed = true;
    }
    if (dest[BYTE4_XOR_LE(tbaseaddr + 2)] != ((data >> 16) & 0xff))
    {
      dest[BYTE4_XOR_LE(tbaseaddr + 2)] = (data >> 16) & 0xff;
      changed = true;
    }
    if (dest[BYTE4_XOR_LE(tbaseaddr + 3)] != ((data >> 24) & 0xff))
    {
      dest[BYTE4_XOR_LE(tbaseaddr + 3)] = (data >> 24) & 0xff;
      changed = true;
    }
  }

  /* 16-bit texture case */
  else
  {
    int lod, tt, ts;
    u32 tbaseaddr;
    u16* dest;

    /* extract info */
    // tmunum = (offset >> 19) & 0x03;
    lod = (offset >> 15) & 0x0f;
    tt = (offset >> 7) & 0xff;
    ts = (offset << 1) & 0xfe;

    /* validate parameters */
    if (lod > 8)
      return 0;

    /* compute the base address */
    tbaseaddr = t->lodoffset[lod];
    tbaseaddr += (u32)(2l * ((long)tt * (((long)t->wmask >> lod) + 1l) + (long)ts));

    if (LOG_TEXTURE_RAM)
      Log_DevPrintf("Texture 16-bit w: lod=%d s=%d t=%d data=%08X\n", lod, ts, tt, data);

    /* write the two words in little-endian order */
    dest = (u16*)t->ram;
    tbaseaddr &= t->mask;
    tbaseaddr >>= 1;

    bool changed = false;
    if (dest[BYTE_XOR_LE(tbaseaddr + 0)] != ((data >> 0) & 0xffff))
    {
      dest[BYTE_XOR_LE(tbaseaddr + 0)] = (data >> 0) & 0xffff;
      changed = true;
    }
    if (dest[BYTE_XOR_LE(tbaseaddr + 1)] != ((data >> 16) & 0xffff))
    {
      dest[BYTE_XOR_LE(tbaseaddr + 1)] = (data >> 16) & 0xffff;
      changed = true;
    }
  }

  return 0;
}

/*************************************
 *
 *  Handle a register read
 *
 *************************************/

static u32 voodoo_register_r(voodoo_state* v, u32 offset)
{
  u32 regnum = (offset)&0xff;

  //	LOG(LOG_VOODOO,LOG_WARN)("Voodoo:read chip %x reg %x (%s)", chips, regnum<<2, voodoo_reg_name[regnum]);

  /* first make sure this register is readable */
  if (!(v->regaccess[regnum] & REGISTER_READ))
  {
    return 0xffffffff;
  }

  u32 result;

  /* default result is the FBI register value */
  result = v->reg[regnum].u;

  /* some registers are dynamic; compute them */
  switch (regnum)
  {
    case status:
    {
      /* start with a blank slate */
      result = 0;

      /* bits 5:0 are the PCI FIFO free space */
      result |= 0x3fu << 0u;

      /* bit 6 is the vertical retrace */
      if (v->display_timing.InVerticalBlank(v->timing_manager->GetTotalEmulatedTime()))
      {
        result |= (1 << 6);
      }

      /* bit 7 is FBI graphics engine busy */
      if (v->pci.op_pending)
        result |= 1 << 7;

      /* bit 8 is TREX busy */
      if (v->pci.op_pending)
        result |= 1 << 8;

      /* bit 9 is overall busy */
      if (v->pci.op_pending)
        result |= 1 << 9;

      /* bits 11:10 specifies which buffer is visible */
      result |= (u32)(v->fbi.frontbuf << 10);

      /* bits 27:12 indicate memory FIFO freespace */
      result |= 0xffffu << 12u;

      /* bits 30:28 are the number of pending swaps */
      result |= 0u << 28u;

      /* bit 31 is not used */
    }
    break;

    case hvRetrace:
    {
      if (v->type < VOODOO_2)
        break;

      /* start with a blank slate */
      result = 0;

      auto ss = v->display_timing.GetSnapshot(v->timing_manager->GetTotalEmulatedTime());
      result |= u32(ss.current_line & 0x1FFF);
      result |= (u32(ss.current_pixel & 0x7FF) << 16);
    }
    break;

    /* bit 2 of the initEnable register maps this to dacRead */
    case fbiInit2:
      if (INITEN_REMAP_INIT_TO_DAC(v->pci.init_enable))
        result = v->dac.read_result;
      break;

      /*		case fbiInit3:
            if (INITEN_REMAP_INIT_TO_DAC(v->pci.init_enable))
              result = 0;
            break;

          case fbiInit6:
            if (v->type < VOODOO_2)
              break;
            result &= 0xffffe7ff;
            result |= 0x1000;
            break; */

    /* all counters are 24-bit only */
    case fbiPixelsIn:
    case fbiChromaFail:
    case fbiZfuncFail:
    case fbiAfuncFail:
    case fbiPixelsOut:
      update_statistics(v, true);
    case fbiTrianglesOut:
      result = v->reg[regnum].u & 0xffffff;
      break;
  }

  return result;
}

/*************************************
 *
 *  Handle an LFB read
 *
 *************************************/
static u32 voodoo_lfb_r(voodoo_state* v, u32 offset)
{
  Log_DevPrintf("Voodoo:read LFB offset %X", offset);
  u16* buffer;
  u32 bufmax;
  u32 bufoffs;
  u32 data;
  int x, y, scry;
  u32 destbuf;

  /* compute X,Y */
  x = (offset << 1) & 0x3fe;
  y = (offset >> 9) & 0x3ff;

  /* select the target buffer */
  destbuf = LFBMODE_READ_BUFFER_SELECT(v->reg[lfbMode].u);
  switch (destbuf)
  {
    case 0: /* front buffer */
      buffer = (u16*)(v->fbi.ram + v->fbi.rgboffs[v->fbi.frontbuf]);
      bufmax = (v->fbi.mask + 1 - v->fbi.rgboffs[v->fbi.frontbuf]) / 2;
      break;

    case 1: /* back buffer */
      buffer = (u16*)(v->fbi.ram + v->fbi.rgboffs[v->fbi.backbuf]);
      bufmax = (v->fbi.mask + 1 - v->fbi.rgboffs[v->fbi.backbuf]) / 2;
      break;

    case 2: /* aux buffer */
      if (v->fbi.auxoffs == (u32)(~0))
        return 0xffffffff;
      buffer = (u16*)(v->fbi.ram + v->fbi.auxoffs);
      bufmax = (v->fbi.mask + 1 - v->fbi.auxoffs) / 2;
      break;

    default: /* reserved */
      return 0xffffffff;
  }

  /* determine the screen Y */
  scry = y;
  if (LFBMODE_Y_ORIGIN(v->reg[lfbMode].u))
    scry = ((int)v->fbi.yorigin - y) & 0x3ff;

  /* advance pointers to the proper row */
  bufoffs = (unsigned long)((long)scry * (long)v->fbi.rowpixels + (long)x);
  if (bufoffs >= bufmax)
  {
    Log_WarningPrintf("LFB_R: Buffer offset out of bounds x=%i y=%i offset=%08X bufoffs=%08X", x, y, offset,
                      (u32)bufoffs);
    return 0xffffffff;
  }

  /* compute the data */
  data = (unsigned int)buffer[bufoffs + 0u] | (unsigned int)(buffer[bufoffs + 1u] << 16u);

  /* word swapping */
  if (LFBMODE_WORD_SWAP_READS(v->reg[lfbMode].u))
    data = (data << 16u) | (data >> 16u);

  /* byte swizzling */
  if (LFBMODE_BYTE_SWIZZLE_READS(v->reg[lfbMode].u))
    data = FLIPENDIAN_INT32(data);

  if (LOG_LFB)
    Log_DevPrintf("VOODOO.LFB:read (%d,%d) = %08X\n", x, y, data);
  return data;
}

void voodoo_w(voodoo_state* v, u32 offset, u32 data, u32 mask)
{
  if ((offset & (0xc00000 / 4)) == 0)
  {
    // Log_DebugPrintf("voodoo_register_w: %08X %08X %08X", offset, data, mask);
    voodoo_register_w(v, offset, data);
  }
  else if ((offset & (0x800000 / 4)) == 0)
  {
    Log_DebugPrintf("voodoo_lfb_w: %08X %08X %08X", offset, data, mask);
    voodoo_lfb_w(v, offset, data, mask);
  }
  else
  {
    Log_DebugPrintf("voodoo_texture_w: %08X %08X %08X", offset, data, mask);
    voodoo_texture_w(v, offset, data);
  }
}

u32 voodoo_r(voodoo_state* v, u32 offset)
{
  if ((offset & (0xc00000 / 4)) == 0)
  {
    u32 val = voodoo_register_r(v, offset);
    // Log_DebugPrintf("voodoo_register_r: %08X -> %08X", offset, val);
    return val;
  }
  else if ((offset & (0x800000 / 4)) == 0)
  {
    u32 val = voodoo_lfb_r(v, offset);
    Log_DebugPrintf("voodoo_lfb_r: %08X -> %08X", offset, val);
    return val;
  }

  Log_WarningPrintf("voodoo_read: unknown range %08X", offset);
  return 0xffffffff;
}

/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/

/*-------------------------------------------------
    device start callback
-------------------------------------------------*/

voodoo_state* voodoo_init(int type, Display* display, TimingManager* timing_manager, TimingEvent* retrace_event)
{
  voodoo_state* v = new voodoo_state;
  v->display = display;
  v->timing_manager = timing_manager;
  v->retrace_event = retrace_event;

  switch (type)
  {
    case VOODOO_1:
      v->type = VOODOO_1;
      break;
    case VOODOO_1_DTMU:
      v->type = VOODOO_1_DTMU;
      break;
    case VOODOO_2:
      v->type = VOODOO_2;
      break;
    default:
      Panic("invalid voodoo card type initialization");
      break;
  }

  memset(v->reg, 0, sizeof(v->reg));

  v->pci.op_pending = false;
  v->dac.read_result = 0;

  v->output_on = false;
  v->clock_enabled = false;
  v->send_config = false;

  memset(v->dac.reg, 0, sizeof(v->dac.reg));

  v->next_rasterizer = 0;
  for (u32 rct = 0; rct < MAX_RASTERIZERS; rct++)
    v->rasterizer[rct] = raster_info();

  v->thread_stats = new stats_block[1];
  v->thread_stats[0].pixels_in = 0;
  v->thread_stats[0].pixels_out = 0;
  v->thread_stats[0].chroma_fail = 0;
  v->thread_stats[0].zfunc_fail = 0;
  v->thread_stats[0].afunc_fail = 0;
  v->thread_stats[0].clip_fail = 0;
  v->thread_stats[0].stipple_count = 0;

  v->alt_regmap = false;
  v->regnames = voodoo_reg_name;

  /* create a table of precomputed 1/n and log2(n) values */
  /* n ranges from 1.0000 to 2.0000 */
  for (u32 val = 0; val <= (1 << RECIPLOG_LOOKUP_BITS); val++)
  {
    u32 value = (1 << RECIPLOG_LOOKUP_BITS) + val;
    voodoo_reciplog[val * 2 + 0] = (1u << (RECIPLOG_LOOKUP_PREC + RECIPLOG_LOOKUP_BITS)) / value;
    voodoo_reciplog[val * 2 + 1] =
      (u32)(LOGB2((double)value / (double)(1u << RECIPLOG_LOOKUP_BITS)) * (double)(1u << RECIPLOG_LOOKUP_PREC));
  }

  for (u32 val = 0; val < RASTER_HASH_SIZE; val++)
    v->raster_hash[val] = NULL;

  /* create dithering tables */
  for (u32 val = 0; val < 256 * 16 * 2; val++)
  {
    int g = (val >> 0) & 1;
    int x = (val >> 1) & 3;
    int color = (val >> 3) & 0xff;
    int y = (val >> 11) & 3;

    if (!g)
    {
      dither4_lookup[val] = (u8)(DITHER_RB(color, dither_matrix_4x4[y * 4 + x]) >> 3);
      dither2_lookup[val] = (u8)(DITHER_RB(color, dither_matrix_2x2[y * 4 + x]) >> 3);
    }
    else
    {
      dither4_lookup[val] = (u8)(DITHER_G(color, dither_matrix_4x4[y * 4 + x]) >> 2);
      dither2_lookup[val] = (u8)(DITHER_G(color, dither_matrix_2x2[y * 4 + x]) >> 2);
    }
  }

  v->tmu_config = 0x11; // revision 1

  u32 fbmemsize = 0;
  u32 tmumem0 = 0;
  u32 tmumem1 = 0;

  /* configure type-specific values */
  switch (v->type)
  {
    case VOODOO_1:
      v->regaccess = voodoo_register_access;
      fbmemsize = 2;
      tmumem0 = 2;
      break;

    case VOODOO_1_DTMU:
      v->regaccess = voodoo_register_access;
      fbmemsize = 4;
      tmumem0 = 4;
      tmumem1 = 4;
      break;

    case VOODOO_2:
      v->regaccess = voodoo2_register_access;
      fbmemsize = 4;
      tmumem0 = 4;
      tmumem1 = 4;
      v->tmu_config |= 0x800;
      break;

    default:
      Panic("Unsupported voodoo card in voodoo_start!");
      break;
  }

  if (tmumem1 != 0)
    v->tmu_config |= 0xc0; // two TMUs

  v->chipmask = 0x01;

  /* set up the PCI FIFO */
  v->pci.fifo.size = 64 * 2;

  /* set up frame buffer */
  init_fbi(v, &v->fbi, (int)(fbmemsize << 20));

  v->fbi.rowpixels = v->fbi.width;

  v->tmu[0].ncc[0].palette = NULL;
  v->tmu[0].ncc[1].palette = NULL;
  v->tmu[1].ncc[0].palette = NULL;
  v->tmu[1].ncc[1].palette = NULL;
  v->tmu[0].ncc[0].palettea = NULL;
  v->tmu[0].ncc[1].palettea = NULL;
  v->tmu[1].ncc[0].palettea = NULL;
  v->tmu[1].ncc[1].palettea = NULL;

  v->tmu[0].ram = NULL;
  v->tmu[1].ram = NULL;
  v->tmu[0].lookup = NULL;
  v->tmu[1].lookup = NULL;

  /* build shared TMU tables */
  init_tmu_shared(&v->tmushare);

  /* set up the TMUs */
  init_tmu(v, &v->tmu[0], &v->reg[0x100], (int)(tmumem0 << 20));
  v->chipmask |= 0x02;
  if (tmumem1 != 0)
  {
    init_tmu(v, &v->tmu[1], &v->reg[0x200], (int)(tmumem1 << 20));
    v->chipmask |= 0x04;
    v->tmu_config |= 0x40;
  }

  /* initialize some registers */
  v->pci.init_enable = 0;
  v->reg[fbiInit0].u = (u32)((1 << 4) | (0x10 << 6));
  v->reg[fbiInit1].u = (u32)((1 << 1) | (1 << 8) | (1 << 12) | (2 << 20));
  v->reg[fbiInit2].u = (u32)((1 << 6) | (0x100 << 23));
  v->reg[fbiInit3].u = (u32)((2 << 13) | (0xf << 17));
  v->reg[fbiInit4].u = (u32)(1 << 0);

  /* do a soft reset to reset everything else */
  soft_reset(v);

  recompute_video_memory(v);
  return v;
}

void voodoo_set_pci_enable(voodoo_state* v, u32 value)
{
  v->pci.init_enable = value;
}

void voodoo_set_clock_enable(voodoo_state* v, bool enable)
{
  if (v->clock_enabled == enable)
    return;

  v->clock_enabled = enable;
  v->display_timing.SetClockEnable(enable);
  if (enable)
  {
    v->display_timing.ResetClock(v->timing_manager->GetTotalEmulatedTime());
    if (v->display_timing.IsValid())
      v->retrace_event->SetActive(true);
  }
}

void voodoo_retrace_event(voodoo_state* v)
{
  if (!v->output_on)
  {
    // framebuffer is not rendered anyway
    return;
  }

  if (!v->clock_enabled)
  {
    // no clock, no signal
    v->display->ClearFramebuffer();
    return;
  }

  Log_DebugPrintf("Voodoo render");

  const u32 width = v->fbi.width;
  const u32 height = v->fbi.height;
  v->display->ResizeFramebuffer(width, height);
  v->display->ChangeFramebufferFormat(Display::FramebufferFormat::BGR565);

  // TODO: Bounds check
  const u8* pixels_in = v->fbi.ram + v->fbi.rgboffs[v->fbi.frontbuf];
  const u32 stride_in = v->fbi.rowpixels * sizeof(u16);
  u8* pixels_out = v->display->GetFramebufferPointer();
  const u32 stride_out = v->display->GetFramebufferStride();
  Y_memcpy_stride(pixels_out, stride_out, pixels_in, stride_in, std::min(stride_in, stride_out), height);
  v->display->SwapFramebuffer();
}

void voodoo_shutdown(voodoo_state* v)
{
  free(v->fbi.ram);
  if (v->tmu[0].ram != NULL)
  {
    free(v->tmu[0].ram);
    v->tmu[0].ram = NULL;
  }
  if (v->tmu[1].ram != NULL)
  {
    free(v->tmu[1].ram);
    v->tmu[1].ram = NULL;
  }
  delete[] v->thread_stats;
  delete v;
}

/***************************************************************************
    COMMAND HANDLERS
***************************************************************************/

/*-------------------------------------------------
    fastfill - execute the 'fastfill'
    command
-------------------------------------------------*/

void fastfill(voodoo_state* v)
{
  int sx = (v->reg[clipLeftRight].u >> 16) & 0x3ff;
  int ex = (v->reg[clipLeftRight].u >> 0) & 0x3ff;
  int sy = (v->reg[clipLowYHighY].u >> 16) & 0x3ff;
  int ey = (v->reg[clipLowYHighY].u >> 0) & 0x3ff;

  poly_extent extents[64];
  u16 dithermatrix[16];
  u16* drawbuf = NULL;
  int extnum, x, y;

  /* if we're not clearing either, take no time */
  if (!FBZMODE_RGB_BUFFER_MASK(v->reg[fbzMode].u) && !FBZMODE_AUX_BUFFER_MASK(v->reg[fbzMode].u))
    return;

  /* are we clearing the RGB buffer? */
  if (FBZMODE_RGB_BUFFER_MASK(v->reg[fbzMode].u))
  {
    /* determine the draw buffer */
    int destbuf = FBZMODE_DRAW_BUFFER(v->reg[fbzMode].u);
    switch (destbuf)
    {
      case 0: /* front buffer */
        drawbuf = (u16*)(v->fbi.ram + v->fbi.rgboffs[v->fbi.frontbuf]);
        break;

      case 1: /* back buffer */
        drawbuf = (u16*)(v->fbi.ram + v->fbi.rgboffs[v->fbi.backbuf]);
        break;

      default: /* reserved */
        break;
    }

    /* determine the dither pattern */
    for (y = 0; y < 4; y++)
    {
      DECLARE_DITHER_POINTERS_NO_DITHER_VAR;
      COMPUTE_DITHER_POINTERS_NO_DITHER_VAR(v->reg[fbzMode].u, y);
      for (x = 0; x < 4; x++)
      {
        int r = v->reg[color1].rgb.r;
        int g = v->reg[color1].rgb.g;
        int b = v->reg[color1].rgb.b;

        APPLY_DITHER(v->reg[fbzMode].u, x, dither_lookup, r, g, b);
        dithermatrix[y * 4 + x] = (u16)((r << 11) | (g << 5) | b);
      }
    }
  }

  /* fill in a block of extents */
  extents[0].startx = sx;
  extents[0].stopx = ex;
  for (extnum = 1; (size_t)extnum < ARRAY_LENGTH(extents); extnum++)
    extents[extnum] = extents[0];

  poly_extra_data* extra = new poly_extra_data;

  /* iterate over blocks of extents */
  for (y = sy; y < ey; y += (int)ARRAY_LENGTH(extents))
  {
    int count = MIN(((size_t)(ey - y)), ARRAY_LENGTH(extents));

    extra->state = v;
    memcpy(extra->dither, dithermatrix, sizeof(extra->dither));

    poly_render_triangle_custom(drawbuf, y, count, extents, extra);
  }

  delete extra;
}

/*-------------------------------------------------
    swapbuffer - execute the 'swapbuffer'
    command
-------------------------------------------------*/

void swapbuffer(voodoo_state* v, u32 data)
{
  /* set the don't swap value for Voodoo 2 */
  v->fbi.vblank_dont_swap = ((data >> 9) & 1) > 0;

  voodoo_swap_buffers(v);
}

/*-------------------------------------------------
    triangle - execute the 'triangle'
    command
-------------------------------------------------*/

void triangle(voodoo_state* v)
{
  int texcount = 0;
  u16* drawbuf;
  int destbuf;

  /* determine the number of TMUs involved */
  texcount = 0;
  if (!FBIINIT3_DISABLE_TMUS(v->reg[fbiInit3].u) && FBZCP_TEXTURE_ENABLE(v->reg[fbzColorPath].u))
  {
    texcount = 1;
    if (v->chipmask & 0x04)
      texcount = 2;
  }

  /* perform subpixel adjustments */
  if (FBZCP_CCA_SUBPIXEL_ADJUST(v->reg[fbzColorPath].u))
  {
    s32 dx = 8 - (v->fbi.ax & 15);
    s32 dy = 8 - (v->fbi.ay & 15);

    /* adjust iterated R,G,B,A and W/Z */
    v->fbi.startr += (dy * v->fbi.drdy + dx * v->fbi.drdx) >> 4;
    v->fbi.startg += (dy * v->fbi.dgdy + dx * v->fbi.dgdx) >> 4;
    v->fbi.startb += (dy * v->fbi.dbdy + dx * v->fbi.dbdx) >> 4;
    v->fbi.starta += (dy * v->fbi.dady + dx * v->fbi.dadx) >> 4;
    v->fbi.startw += (dy * v->fbi.dwdy + dx * v->fbi.dwdx) >> 4;
    v->fbi.startz += mul_32x32_shift(dy, v->fbi.dzdy, 4) + mul_32x32_shift(dx, v->fbi.dzdx, 4);

    /* adjust iterated W/S/T for TMU 0 */
    if (texcount >= 1)
    {
      v->tmu[0].startw += (dy * v->tmu[0].dwdy + dx * v->tmu[0].dwdx) >> 4;
      v->tmu[0].starts += (dy * v->tmu[0].dsdy + dx * v->tmu[0].dsdx) >> 4;
      v->tmu[0].startt += (dy * v->tmu[0].dtdy + dx * v->tmu[0].dtdx) >> 4;

      /* adjust iterated W/S/T for TMU 1 */
      if (texcount >= 2)
      {
        v->tmu[1].startw += (dy * v->tmu[1].dwdy + dx * v->tmu[1].dwdx) >> 4;
        v->tmu[1].starts += (dy * v->tmu[1].dsdy + dx * v->tmu[1].dsdx) >> 4;
        v->tmu[1].startt += (dy * v->tmu[1].dtdy + dx * v->tmu[1].dtdx) >> 4;
      }
    }
  }

  /* determine the draw buffer */
  destbuf = FBZMODE_DRAW_BUFFER(v->reg[fbzMode].u);
  switch (destbuf)
  {
    case 0: /* front buffer */
      drawbuf = (u16*)(v->fbi.ram + v->fbi.rgboffs[v->fbi.frontbuf]);
      break;

    case 1: /* back buffer */
      drawbuf = (u16*)(v->fbi.ram + v->fbi.rgboffs[v->fbi.backbuf]);
      break;

    default: /* reserved */
      return;
  }

  /* find a rasterizer that matches our current state */
  triangle_create_work_item(v, drawbuf, texcount);

  /* update stats */
  v->reg[fbiTrianglesOut].u++;
}

/*-------------------------------------------------
    begin_triangle - execute the 'beginTri'
    command
-------------------------------------------------*/

static void begin_triangle(voodoo_state* v)
{
  setup_vertex* sv = &v->fbi.svert[2];

  /* extract all the data from registers */
  sv->x = v->reg[sVx].f;
  sv->y = v->reg[sVy].f;
  sv->wb = v->reg[sWb].f;
  sv->w0 = v->reg[sWtmu0].f;
  sv->s0 = v->reg[sS_W0].f;
  sv->t0 = v->reg[sT_W0].f;
  sv->w1 = v->reg[sWtmu1].f;
  sv->s1 = v->reg[sS_Wtmu1].f;
  sv->t1 = v->reg[sT_Wtmu1].f;
  sv->a = v->reg[sAlpha].f;
  sv->r = v->reg[sRed].f;
  sv->g = v->reg[sGreen].f;
  sv->b = v->reg[sBlue].f;

  /* spread it across all three verts and reset the count */
  v->fbi.svert[0] = v->fbi.svert[1] = v->fbi.svert[2];
  v->fbi.sverts = 1;
}

/*-------------------------------------------------
    draw_triangle - execute the 'DrawTri'
    command
-------------------------------------------------*/

static void draw_triangle(voodoo_state* v)
{
  setup_vertex* sv = &v->fbi.svert[2];

  /* for strip mode, shuffle vertex 1 down to 0 */
  if (!(v->reg[sSetupMode].u & (1 << 16)))
    v->fbi.svert[0] = v->fbi.svert[1];

  /* copy 2 down to 1 regardless */
  v->fbi.svert[1] = v->fbi.svert[2];

  /* extract all the data from registers */
  sv->x = v->reg[sVx].f;
  sv->y = v->reg[sVy].f;
  sv->wb = v->reg[sWb].f;
  sv->w0 = v->reg[sWtmu0].f;
  sv->s0 = v->reg[sS_W0].f;
  sv->t0 = v->reg[sT_W0].f;
  sv->w1 = v->reg[sWtmu1].f;
  sv->s1 = v->reg[sS_Wtmu1].f;
  sv->t1 = v->reg[sT_Wtmu1].f;
  sv->a = v->reg[sAlpha].f;
  sv->r = v->reg[sRed].f;
  sv->g = v->reg[sGreen].f;
  sv->b = v->reg[sBlue].f;

  /* if we have enough verts, go ahead and draw */
  if (++v->fbi.sverts >= 3)
    setup_and_draw_triangle(v);
}

/*-------------------------------------------------
    setup_and_draw_triangle - process the setup
    parameters and render the triangle
-------------------------------------------------*/

static void setup_and_draw_triangle(voodoo_state* v)
{
  float dx1, dy1, dx2, dy2;
  float divisor, tdiv;

  /* grab the X/Ys at least */
  v->fbi.ax = (s16)(v->fbi.svert[0].x * 16.0);
  v->fbi.ay = (s16)(v->fbi.svert[0].y * 16.0);
  v->fbi.bx = (s16)(v->fbi.svert[1].x * 16.0);
  v->fbi.by = (s16)(v->fbi.svert[1].y * 16.0);
  v->fbi.cx = (s16)(v->fbi.svert[2].x * 16.0);
  v->fbi.cy = (s16)(v->fbi.svert[2].y * 16.0);

  /* compute the divisor */
  divisor = 1.0f / ((v->fbi.svert[0].x - v->fbi.svert[1].x) * (v->fbi.svert[0].y - v->fbi.svert[2].y) -
                    (v->fbi.svert[0].x - v->fbi.svert[2].x) * (v->fbi.svert[0].y - v->fbi.svert[1].y));

  /* backface culling */
  if (v->reg[sSetupMode].u & 0x20000)
  {
    int culling_sign = (v->reg[sSetupMode].u >> 18) & 1;
    int divisor_sign = (divisor < 0);

    /* if doing strips and ping pong is enabled, apply the ping pong */
    if ((v->reg[sSetupMode].u & 0x90000) == 0x00000)
      culling_sign ^= (v->fbi.sverts - 3) & 1;

    /* if our sign matches the culling sign, we're done for */
    if (divisor_sign == culling_sign)
      return;
  }

  /* compute the dx/dy values */
  dx1 = v->fbi.svert[0].y - v->fbi.svert[2].y;
  dx2 = v->fbi.svert[0].y - v->fbi.svert[1].y;
  dy1 = v->fbi.svert[0].x - v->fbi.svert[1].x;
  dy2 = v->fbi.svert[0].x - v->fbi.svert[2].x;

  /* set up R,G,B */
  tdiv = divisor * 4096.0f;
  if (v->reg[sSetupMode].u & (1 << 0))
  {
    v->fbi.startr = (s32)(v->fbi.svert[0].r * 4096.0f);
    v->fbi.drdx =
      (s32)(((v->fbi.svert[0].r - v->fbi.svert[1].r) * dx1 - (v->fbi.svert[0].r - v->fbi.svert[2].r) * dx2) * tdiv);
    v->fbi.drdy =
      (s32)(((v->fbi.svert[0].r - v->fbi.svert[2].r) * dy1 - (v->fbi.svert[0].r - v->fbi.svert[1].r) * dy2) * tdiv);
    v->fbi.startg = (s32)(v->fbi.svert[0].g * 4096.0f);
    v->fbi.dgdx =
      (s32)(((v->fbi.svert[0].g - v->fbi.svert[1].g) * dx1 - (v->fbi.svert[0].g - v->fbi.svert[2].g) * dx2) * tdiv);
    v->fbi.dgdy =
      (s32)(((v->fbi.svert[0].g - v->fbi.svert[2].g) * dy1 - (v->fbi.svert[0].g - v->fbi.svert[1].g) * dy2) * tdiv);
    v->fbi.startb = (s32)(v->fbi.svert[0].b * 4096.0f);
    v->fbi.dbdx =
      (s32)(((v->fbi.svert[0].b - v->fbi.svert[1].b) * dx1 - (v->fbi.svert[0].b - v->fbi.svert[2].b) * dx2) * tdiv);
    v->fbi.dbdy =
      (s32)(((v->fbi.svert[0].b - v->fbi.svert[2].b) * dy1 - (v->fbi.svert[0].b - v->fbi.svert[1].b) * dy2) * tdiv);
  }

  /* set up alpha */
  if (v->reg[sSetupMode].u & (1 << 1))
  {
    v->fbi.starta = (s32)(v->fbi.svert[0].a * 4096.0);
    v->fbi.dadx =
      (s32)(((v->fbi.svert[0].a - v->fbi.svert[1].a) * dx1 - (v->fbi.svert[0].a - v->fbi.svert[2].a) * dx2) * tdiv);
    v->fbi.dady =
      (s32)(((v->fbi.svert[0].a - v->fbi.svert[2].a) * dy1 - (v->fbi.svert[0].a - v->fbi.svert[1].a) * dy2) * tdiv);
  }

  /* set up Z */
  if (v->reg[sSetupMode].u & (1 << 2))
  {
    v->fbi.startz = (s32)(v->fbi.svert[0].z * 4096.0);
    v->fbi.dzdx =
      (s32)(((v->fbi.svert[0].z - v->fbi.svert[1].z) * dx1 - (v->fbi.svert[0].z - v->fbi.svert[2].z) * dx2) * tdiv);
    v->fbi.dzdy =
      (s32)(((v->fbi.svert[0].z - v->fbi.svert[2].z) * dy1 - (v->fbi.svert[0].z - v->fbi.svert[1].z) * dy2) * tdiv);
  }

  /* set up Wb */
  tdiv = divisor * 65536.0f * 65536.0f;
  if (v->reg[sSetupMode].u & (1 << 3))
  {
    v->fbi.startw = v->tmu[0].startw = v->tmu[1].startw = (s64)(v->fbi.svert[0].wb * 65536.0f * 65536.0f);
    v->fbi.dwdx = v->tmu[0].dwdx = v->tmu[1].dwdx =
      (s64)(((v->fbi.svert[0].wb - v->fbi.svert[1].wb) * dx1 - (v->fbi.svert[0].wb - v->fbi.svert[2].wb) * dx2) * tdiv);
    v->fbi.dwdy = v->tmu[0].dwdy = v->tmu[1].dwdy =
      (s64)(((v->fbi.svert[0].wb - v->fbi.svert[2].wb) * dy1 - (v->fbi.svert[0].wb - v->fbi.svert[1].wb) * dy2) * tdiv);
  }

  /* set up W0 */
  if (v->reg[sSetupMode].u & (1 << 4))
  {
    v->tmu[0].startw = v->tmu[1].startw = (s64)(v->fbi.svert[0].w0 * 65536.0f * 65536.0f);
    v->tmu[0].dwdx = v->tmu[1].dwdx =
      (s64)(((v->fbi.svert[0].w0 - v->fbi.svert[1].w0) * dx1 - (v->fbi.svert[0].w0 - v->fbi.svert[2].w0) * dx2) * tdiv);
    v->tmu[0].dwdy = v->tmu[1].dwdy =
      (s64)(((v->fbi.svert[0].w0 - v->fbi.svert[2].w0) * dy1 - (v->fbi.svert[0].w0 - v->fbi.svert[1].w0) * dy2) * tdiv);
  }

  /* set up S0,T0 */
  if (v->reg[sSetupMode].u & (1 << 5))
  {
    v->tmu[0].starts = v->tmu[1].starts = (s64)(v->fbi.svert[0].s0 * 65536.0f * 65536.0f);
    v->tmu[0].dsdx = v->tmu[1].dsdx =
      (s64)(((v->fbi.svert[0].s0 - v->fbi.svert[1].s0) * dx1 - (v->fbi.svert[0].s0 - v->fbi.svert[2].s0) * dx2) * tdiv);
    v->tmu[0].dsdy = v->tmu[1].dsdy =
      (s64)(((v->fbi.svert[0].s0 - v->fbi.svert[2].s0) * dy1 - (v->fbi.svert[0].s0 - v->fbi.svert[1].s0) * dy2) * tdiv);
    v->tmu[0].startt = v->tmu[1].startt = (s64)(v->fbi.svert[0].t0 * 65536.0f * 65536.0f);
    v->tmu[0].dtdx = v->tmu[1].dtdx =
      (s64)(((v->fbi.svert[0].t0 - v->fbi.svert[1].t0) * dx1 - (v->fbi.svert[0].t0 - v->fbi.svert[2].t0) * dx2) * tdiv);
    v->tmu[0].dtdy = v->tmu[1].dtdy =
      (s64)(((v->fbi.svert[0].t0 - v->fbi.svert[2].t0) * dy1 - (v->fbi.svert[0].t0 - v->fbi.svert[1].t0) * dy2) * tdiv);
  }

  /* set up W1 */
  if (v->reg[sSetupMode].u & (1 << 6))
  {
    v->tmu[1].startw = (s64)(v->fbi.svert[0].w1 * 65536.0f * 65536.0f);
    v->tmu[1].dwdx =
      (s64)(((v->fbi.svert[0].w1 - v->fbi.svert[1].w1) * dx1 - (v->fbi.svert[0].w1 - v->fbi.svert[2].w1) * dx2) * tdiv);
    v->tmu[1].dwdy =
      (s64)(((v->fbi.svert[0].w1 - v->fbi.svert[2].w1) * dy1 - (v->fbi.svert[0].w1 - v->fbi.svert[1].w1) * dy2) * tdiv);
  }

  /* set up S1,T1 */
  if (v->reg[sSetupMode].u & (1 << 7))
  {
    v->tmu[1].starts = (s64)(v->fbi.svert[0].s1 * 65536.0f * 65536.0f);
    v->tmu[1].dsdx =
      (s64)(((v->fbi.svert[0].s1 - v->fbi.svert[1].s1) * dx1 - (v->fbi.svert[0].s1 - v->fbi.svert[2].s1) * dx2) * tdiv);
    v->tmu[1].dsdy =
      (s64)(((v->fbi.svert[0].s1 - v->fbi.svert[2].s1) * dy1 - (v->fbi.svert[0].s1 - v->fbi.svert[1].s1) * dy2) * tdiv);
    v->tmu[1].startt = (s64)(v->fbi.svert[0].t1 * 65536.0f * 65536.0f);
    v->tmu[1].dtdx =
      (s64)(((v->fbi.svert[0].t1 - v->fbi.svert[1].t1) * dx1 - (v->fbi.svert[0].t1 - v->fbi.svert[2].t1) * dx2) * tdiv);
    v->tmu[1].dtdy =
      (s64)(((v->fbi.svert[0].t1 - v->fbi.svert[2].t1) * dy1 - (v->fbi.svert[0].t1 - v->fbi.svert[1].t1) * dy2) * tdiv);
  }

  /* draw the triangle */
  triangle(v);
}

/*-------------------------------------------------
    triangle_create_work_item - finish triangle
    setup and create the work item
-------------------------------------------------*/

void triangle_create_work_item(voodoo_state* v, u16* drawbuf, int texcount)
{
  poly_extra_data* extra = new poly_extra_data;
  raster_info* info = find_rasterizer(v, texcount);
  poly_vertex vert[3];

  /* fill in the vertex data */
  vert[0].x = (float)v->fbi.ax * (1.0f / 16.0f);
  vert[0].y = (float)v->fbi.ay * (1.0f / 16.0f);
  vert[1].x = (float)v->fbi.bx * (1.0f / 16.0f);
  vert[1].y = (float)v->fbi.by * (1.0f / 16.0f);
  vert[2].x = (float)v->fbi.cx * (1.0f / 16.0f);
  vert[2].y = (float)v->fbi.cy * (1.0f / 16.0f);

  /* fill in the extra data */
  extra->state = v;
  extra->info = info;

  /* fill in triangle parameters */
  extra->ax = v->fbi.ax;
  extra->ay = v->fbi.ay;
  extra->startr = v->fbi.startr;
  extra->startg = v->fbi.startg;
  extra->startb = v->fbi.startb;
  extra->starta = v->fbi.starta;
  extra->startz = v->fbi.startz;
  extra->startw = v->fbi.startw;
  extra->drdx = v->fbi.drdx;
  extra->dgdx = v->fbi.dgdx;
  extra->dbdx = v->fbi.dbdx;
  extra->dadx = v->fbi.dadx;
  extra->dzdx = v->fbi.dzdx;
  extra->dwdx = v->fbi.dwdx;
  extra->drdy = v->fbi.drdy;
  extra->dgdy = v->fbi.dgdy;
  extra->dbdy = v->fbi.dbdy;
  extra->dady = v->fbi.dady;
  extra->dzdy = v->fbi.dzdy;
  extra->dwdy = v->fbi.dwdy;

  /* fill in texture 0 parameters */
  if (texcount > 0)
  {
    extra->starts0 = v->tmu[0].starts;
    extra->startt0 = v->tmu[0].startt;
    extra->startw0 = v->tmu[0].startw;
    extra->ds0dx = v->tmu[0].dsdx;
    extra->dt0dx = v->tmu[0].dtdx;
    extra->dw0dx = v->tmu[0].dwdx;
    extra->ds0dy = v->tmu[0].dsdy;
    extra->dt0dy = v->tmu[0].dtdy;
    extra->dw0dy = v->tmu[0].dwdy;
    extra->lodbase0 = prepare_tmu(&v->tmu[0]);

    /* fill in texture 1 parameters */
    if (texcount > 1)
    {
      extra->starts1 = v->tmu[1].starts;
      extra->startt1 = v->tmu[1].startt;
      extra->startw1 = v->tmu[1].startw;
      extra->ds1dx = v->tmu[1].dsdx;
      extra->dt1dx = v->tmu[1].dtdx;
      extra->dw1dx = v->tmu[1].dwdx;
      extra->ds1dy = v->tmu[1].dsdy;
      extra->dt1dy = v->tmu[1].dtdy;
      extra->dw1dy = v->tmu[1].dwdy;
      extra->lodbase1 = prepare_tmu(&v->tmu[1]);
    }
  }

  extra->texcount = (unsigned int)texcount;
  extra->r_fbzColorPath = v->reg[fbzColorPath].u;
  extra->r_fbzMode = v->reg[fbzMode].u;
  extra->r_alphaMode = v->reg[alphaMode].u;
  extra->r_fogMode = v->reg[fogMode].u;
  extra->r_textureMode0 = (s32)v->tmu[0].reg[textureMode].u;
  if (v->tmu[1].ram != NULL)
    extra->r_textureMode1 = (s32)v->tmu[1].reg[textureMode].u;

  info->polys++;

  poly_render_triangle(drawbuf, info->callback, &vert[0], &vert[1], &vert[2], extra);
  delete extra;
}

/***************************************************************************
    RASTERIZER MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    add_rasterizer - add a rasterizer to our
    hash table
-------------------------------------------------*/

static raster_info* add_rasterizer(voodoo_state* v, const raster_info* cinfo)
{
  raster_info* info = &v->rasterizer[v->next_rasterizer++];
  unsigned int hash = compute_raster_hash(cinfo);

  if (v->next_rasterizer > MAX_RASTERIZERS)
    Panic("Out of space for new rasterizers!");

  /* make a copy of the info */
  *info = *cinfo;

  /* fill in the data */
  info->hits = 0;
  info->polys = 0;

  /* hook us into the hash table */
  info->next = v->raster_hash[hash];
  v->raster_hash[hash] = info;

  if (LOG_RASTERIZERS)
  {
    Log_DevPrintf("Adding rasterizer @ %p : %08X %08X %08X %08X %08X %08X (hash=%d)\n",
                  (void*)((uintptr_t)(info->callback)), info->eff_color_path, info->eff_alpha_mode, info->eff_fog_mode,
                  info->eff_fbz_mode, info->eff_tex_mode_0, info->eff_tex_mode_1, hash);
  }

  return info;
}

/*-------------------------------------------------
    find_rasterizer - find a rasterizer that
    matches  our current parameters and return
    it, creating a new one if necessary
-------------------------------------------------*/

static raster_info* find_rasterizer(voodoo_state* v, int texcount)
{
  raster_info *info, *prev = NULL;
  raster_info curinfo;
  unsigned int hash;

  /* build an info struct with all the parameters */
  curinfo.eff_color_path = normalize_color_path(v->reg[fbzColorPath].u);
  curinfo.eff_alpha_mode = normalize_alpha_mode(v->reg[alphaMode].u);
  curinfo.eff_fog_mode = normalize_fog_mode(v->reg[fogMode].u);
  curinfo.eff_fbz_mode = normalize_fbz_mode(v->reg[fbzMode].u);
  curinfo.eff_tex_mode_0 = (texcount >= 1) ? normalize_tex_mode(v->tmu[0].reg[textureMode].u) : 0xffffffff;
  curinfo.eff_tex_mode_1 = (texcount >= 2) ? normalize_tex_mode(v->tmu[1].reg[textureMode].u) : 0xffffffff;

  /* compute the hash */
  hash = compute_raster_hash(&curinfo);

  /* find the appropriate hash entry */
  for (info = v->raster_hash[hash]; info; prev = info, info = info->next)
    if (info->eff_color_path == curinfo.eff_color_path && info->eff_alpha_mode == curinfo.eff_alpha_mode &&
        info->eff_fog_mode == curinfo.eff_fog_mode && info->eff_fbz_mode == curinfo.eff_fbz_mode &&
        info->eff_tex_mode_0 == curinfo.eff_tex_mode_0 && info->eff_tex_mode_1 == curinfo.eff_tex_mode_1)
    {
      /* got it, move us to the head of the list */
      if (prev)
      {
        prev->next = info->next;
        info->next = v->raster_hash[hash];
        v->raster_hash[hash] = info;
      }

      /* return the result */
      return info;
    }

  /* generate a new one using the generic entry */
  curinfo.callback =
    (texcount == 0) ? raster_generic_0tmu : (texcount == 1) ? raster_generic_1tmu : raster_generic_2tmu;
  curinfo.is_generic = true;
  curinfo.display = 0;
  curinfo.polys = 0;
  curinfo.hits = 0;
  curinfo.next = 0;

  return add_rasterizer(v, &curinfo);
}

/***************************************************************************
    GENERIC RASTERIZERS
***************************************************************************/

/*-------------------------------------------------
    raster_fastfill - per-scanline
    implementation of the 'fastfill' command
-------------------------------------------------*/

static void raster_fastfill(void* destbase, s32 y, const poly_extent* extent, const void* extradata)
{
  const poly_extra_data* extra = (const poly_extra_data*)extradata;
  voodoo_state* v = extra->state;
  stats_block* stats = &v->thread_stats[0];
  s32 startx = extent->startx;
  s32 stopx = extent->stopx;
  int scry, x;

  /* determine the screen Y */
  scry = y;
  if (FBZMODE_Y_ORIGIN(v->reg[fbzMode].u))
    scry = ((int)v->fbi.yorigin - y) & 0x3ff;

  /* fill this RGB row */
  if (FBZMODE_RGB_BUFFER_MASK(v->reg[fbzMode].u))
  {
    const u16* ditherow = &extra->dither[(y & 3) * 4];
    u64 expanded = *(u64*)ditherow;
    u16* dest = (u16*)destbase + ((uintptr_t)((long)scry * (long)v->fbi.rowpixels));

    for (x = startx; x < stopx && (x & 3) != 0; x++)
      dest[x] = ditherow[x & 3];
    for (; x < (stopx & ~3); x += 4)
      *(u64*)&dest[x] = expanded;
    for (; x < stopx; x++)
      dest[x] = ditherow[x & 3];
    stats->pixels_out += stopx - startx;
  }

  /* fill this dest buffer row */
  if (FBZMODE_AUX_BUFFER_MASK(v->reg[fbzMode].u) && v->fbi.auxoffs != (u32)(~0))
  {
    u16 color = (u16)(v->reg[zaColor].u & 0xffff);
    u64 expanded = ((u64)color << 48ull) | ((u64)color << 32ull) | ((u64)color << 16ull) | (u64)color;
    u16* dest = (u16*)(v->fbi.ram + v->fbi.auxoffs) + ((uintptr_t)((long)scry * (long)v->fbi.rowpixels));

    if ((unsigned long)((long)v->fbi.auxoffs + 2l * ((long)scry * (long)v->fbi.rowpixels + (long)stopx)) >= v->fbi.mask)
    {
      stopx = ((long)v->fbi.mask - (long)v->fbi.auxoffs) / 2l - (long)scry * (long)v->fbi.rowpixels;
      if ((stopx < 0) || (stopx < startx))
        return;
    }

    for (x = startx; x < stopx && (x & 3) != 0; x++)
      dest[x] = color;
    for (; x < (stopx & ~3); x += 4)
      *(u64*)&dest[x] = expanded;
    for (; x < stopx; x++)
      dest[x] = color;
  }
}
