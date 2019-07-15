// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    voodoo.h

    3dfx Voodoo Graphics SST-1/2 emulator.

***************************************************************************/
#pragma once
#include "common/display.h"
#include "common/display_timing.h"
#include "common/types.h"
#include "palette.h"
#include "pce/bus.h"
#include "polylgcy.h"
#include "rgbutil.h"
#include <cmath>

/*************************************
 *
 *  Misc. constants
 *
 *************************************/

/* enumeration describing reasons we might be stalled */
enum
{
  NOT_STALLED = 0,
  STALLED_UNTIL_FIFO_LWM,
  STALLED_UNTIL_FIFO_EMPTY
};

// Use old table lookup versus straight double divide
#define USE_FAST_RECIP 0

/* maximum number of TMUs */
#define MAX_TMU 2

/* accumulate operations less than this number of clocks */
#define ACCUMULATE_THRESHOLD 0

/* number of clocks to set up a triangle (just a guess) */
#define TRIANGLE_SETUP_CLOCKS 100

/* maximum number of rasterizers */
#define MAX_RASTERIZERS 1024

/* size of the rasterizer hash table */
#define RASTER_HASH_SIZE 97

/* flags for LFB writes */
#define LFB_RGB_PRESENT 1
#define LFB_ALPHA_PRESENT 2
#define LFB_DEPTH_PRESENT 4
#define LFB_DEPTH_PRESENT_MSW 8

/* flags for the register access array */
#define REGISTER_READ 0x01      /* reads are allowed */
#define REGISTER_WRITE 0x02     /* writes are allowed */
#define REGISTER_PIPELINED 0x04 /* writes are pipelined */
#define REGISTER_FIFO 0x08      /* writes go to FIFO */
#define REGISTER_WRITETHRU 0x10 /* writes are valid even for CMDFIFO */

/* shorter combinations to make the table smaller */
#define REG_R (REGISTER_READ)
#define REG_W (REGISTER_WRITE)
#define REG_WT (REGISTER_WRITE | REGISTER_WRITETHRU)
#define REG_RW (REGISTER_READ | REGISTER_WRITE)
#define REG_RWT (REGISTER_READ | REGISTER_WRITE | REGISTER_WRITETHRU)
#define REG_RP (REGISTER_READ | REGISTER_PIPELINED)
#define REG_WP (REGISTER_WRITE | REGISTER_PIPELINED)
#define REG_RWP (REGISTER_READ | REGISTER_WRITE | REGISTER_PIPELINED)
#define REG_RWPT (REGISTER_READ | REGISTER_WRITE | REGISTER_PIPELINED | REGISTER_WRITETHRU)
#define REG_RF (REGISTER_READ | REGISTER_FIFO)
#define REG_WF (REGISTER_WRITE | REGISTER_FIFO)
#define REG_RWF (REGISTER_READ | REGISTER_WRITE | REGISTER_FIFO)
#define REG_RPF (REGISTER_READ | REGISTER_PIPELINED | REGISTER_FIFO)
#define REG_WPF (REGISTER_WRITE | REGISTER_PIPELINED | REGISTER_FIFO)
#define REG_RWPF (REGISTER_READ | REGISTER_WRITE | REGISTER_PIPELINED | REGISTER_FIFO)

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
 *  Register constants
 *
 *************************************/

/* Codes to the right:
    R = readable
    W = writeable
    P = pipelined
    F = goes to FIFO
*/

/* 0x000 */
#define vdstatus (0x000 / 4) /* R  P  */
#define intrCtrl (0x004 / 4) /* RW P   -- Voodoo2/Banshee only */
#define vertexAx (0x008 / 4) /*  W PF */
#define vertexAy (0x00c / 4) /*  W PF */
#define vertexBx (0x010 / 4) /*  W PF */
#define vertexBy (0x014 / 4) /*  W PF */
#define vertexCx (0x018 / 4) /*  W PF */
#define vertexCy (0x01c / 4) /*  W PF */
#define startR (0x020 / 4)   /*  W PF */
#define startG (0x024 / 4)   /*  W PF */
#define startB (0x028 / 4)   /*  W PF */
#define startZ (0x02c / 4)   /*  W PF */
#define startA (0x030 / 4)   /*  W PF */
#define startS (0x034 / 4)   /*  W PF */
#define startT (0x038 / 4)   /*  W PF */
#define startW (0x03c / 4)   /*  W PF */

/* 0x040 */
#define dRdX (0x040 / 4) /*  W PF */
#define dGdX (0x044 / 4) /*  W PF */
#define dBdX (0x048 / 4) /*  W PF */
#define dZdX (0x04c / 4) /*  W PF */
#define dAdX (0x050 / 4) /*  W PF */
#define dSdX (0x054 / 4) /*  W PF */
#define dTdX (0x058 / 4) /*  W PF */
#define dWdX (0x05c / 4) /*  W PF */
#define dRdY (0x060 / 4) /*  W PF */
#define dGdY (0x064 / 4) /*  W PF */
#define dBdY (0x068 / 4) /*  W PF */
#define dZdY (0x06c / 4) /*  W PF */
#define dAdY (0x070 / 4) /*  W PF */
#define dSdY (0x074 / 4) /*  W PF */
#define dTdY (0x078 / 4) /*  W PF */
#define dWdY (0x07c / 4) /*  W PF */

/* 0x080 */
#define triangleCMD (0x080 / 4) /*  W PF */
#define fvertexAx (0x088 / 4)   /*  W PF */
#define fvertexAy (0x08c / 4)   /*  W PF */
#define fvertexBx (0x090 / 4)   /*  W PF */
#define fvertexBy (0x094 / 4)   /*  W PF */
#define fvertexCx (0x098 / 4)   /*  W PF */
#define fvertexCy (0x09c / 4)   /*  W PF */
#define fstartR (0x0a0 / 4)     /*  W PF */
#define fstartG (0x0a4 / 4)     /*  W PF */
#define fstartB (0x0a8 / 4)     /*  W PF */
#define fstartZ (0x0ac / 4)     /*  W PF */
#define fstartA (0x0b0 / 4)     /*  W PF */
#define fstartS (0x0b4 / 4)     /*  W PF */
#define fstartT (0x0b8 / 4)     /*  W PF */
#define fstartW (0x0bc / 4)     /*  W PF */

/* 0x0c0 */
#define fdRdX (0x0c0 / 4) /*  W PF */
#define fdGdX (0x0c4 / 4) /*  W PF */
#define fdBdX (0x0c8 / 4) /*  W PF */
#define fdZdX (0x0cc / 4) /*  W PF */
#define fdAdX (0x0d0 / 4) /*  W PF */
#define fdSdX (0x0d4 / 4) /*  W PF */
#define fdTdX (0x0d8 / 4) /*  W PF */
#define fdWdX (0x0dc / 4) /*  W PF */
#define fdRdY (0x0e0 / 4) /*  W PF */
#define fdGdY (0x0e4 / 4) /*  W PF */
#define fdBdY (0x0e8 / 4) /*  W PF */
#define fdZdY (0x0ec / 4) /*  W PF */
#define fdAdY (0x0f0 / 4) /*  W PF */
#define fdSdY (0x0f4 / 4) /*  W PF */
#define fdTdY (0x0f8 / 4) /*  W PF */
#define fdWdY (0x0fc / 4) /*  W PF */

/* 0x100 */
#define ftriangleCMD (0x100 / 4)  /*  W PF */
#define fbzColorPath (0x104 / 4)  /* RW PF */
#define fogMode (0x108 / 4)       /* RW PF */
#define alphaMode (0x10c / 4)     /* RW PF */
#define fbzMode (0x110 / 4)       /* RW  F */
#define lfbMode (0x114 / 4)       /* RW  F */
#define clipLeftRight (0x118 / 4) /* RW  F */
#define clipLowYHighY (0x11c / 4) /* RW  F */
#define nopCMD (0x120 / 4)        /*  W  F */
#define fastfillCMD (0x124 / 4)   /*  W  F */
#define swapbufferCMD (0x128 / 4) /*  W  F */
#define fogColor (0x12c / 4)      /*  W  F */
#define zaColor (0x130 / 4)       /*  W  F */
#define chromaKey (0x134 / 4)     /*  W  F */
#define chromaRange (0x138 / 4)   /*  W  F  -- Voodoo2/Banshee only */
#define userIntrCMD (0x13c / 4)   /*  W  F  -- Voodoo2/Banshee only */

/* 0x140 */
#define stipple (0x140 / 4)       /* RW  F */
#define color0 (0x144 / 4)        /* RW  F */
#define color1 (0x148 / 4)        /* RW  F */
#define fbiPixelsIn (0x14c / 4)   /* R     */
#define fbiChromaFail (0x150 / 4) /* R     */
#define fbiZfuncFail (0x154 / 4)  /* R     */
#define fbiAfuncFail (0x158 / 4)  /* R     */
#define fbiPixelsOut (0x15c / 4)  /* R     */
#define fogTable (0x160 / 4)      /*  W  F */

/* 0x1c0 */
#define cmdFifoBaseAddr (0x1e0 / 4) /* RW     -- Voodoo2 only */
#define cmdFifoBump (0x1e4 / 4)     /* RW     -- Voodoo2 only */
#define cmdFifoRdPtr (0x1e8 / 4)    /* RW     -- Voodoo2 only */
#define cmdFifoAMin (0x1ec / 4)     /* RW     -- Voodoo2 only */
#define colBufferAddr (0x1ec / 4)   /* RW     -- Banshee only */
#define cmdFifoAMax (0x1f0 / 4)     /* RW     -- Voodoo2 only */
#define colBufferStride (0x1f0 / 4) /* RW     -- Banshee only */
#define cmdFifoDepth (0x1f4 / 4)    /* RW     -- Voodoo2 only */
#define auxBufferAddr (0x1f4 / 4)   /* RW     -- Banshee only */
#define cmdFifoHoles (0x1f8 / 4)    /* RW     -- Voodoo2 only */
#define auxBufferStride (0x1f8 / 4) /* RW     -- Banshee only */

/* 0x200 */
#define fbiInit4 (0x200 / 4)        /* RW     -- Voodoo/Voodoo2 only */
#define clipLeftRight1 (0x200 / 4)  /* RW     -- Banshee only */
#define vRetrace (0x204 / 4)        /* R      -- Voodoo/Voodoo2 only */
#define clipTopBottom1 (0x204 / 4)  /* RW     -- Banshee only */
#define backPorch (0x208 / 4)       /* RW     -- Voodoo/Voodoo2 only */
#define videoDimensions (0x20c / 4) /* RW     -- Voodoo/Voodoo2 only */
#define fbiInit0 (0x210 / 4)        /* RW     -- Voodoo/Voodoo2 only */
#define fbiInit1 (0x214 / 4)        /* RW     -- Voodoo/Voodoo2 only */
#define fbiInit2 (0x218 / 4)        /* RW     -- Voodoo/Voodoo2 only */
#define fbiInit3 (0x21c / 4)        /* RW     -- Voodoo/Voodoo2 only */
#define hSync (0x220 / 4)           /*  W     -- Voodoo/Voodoo2 only */
#define vSync (0x224 / 4)           /*  W     -- Voodoo/Voodoo2 only */
#define clutData (0x228 / 4)        /*  W  F  -- Voodoo/Voodoo2 only */
#define dacData (0x22c / 4)         /*  W     -- Voodoo/Voodoo2 only */
#define maxRgbDelta (0x230 / 4)     /*  W     -- Voodoo/Voodoo2 only */
#define hBorder (0x234 / 4)         /*  W     -- Voodoo2 only */
#define vBorder (0x238 / 4)         /*  W     -- Voodoo2 only */
#define borderColor (0x23c / 4)     /*  W     -- Voodoo2 only */

/* 0x240 */
#define hvRetrace (0x240 / 4)       /* R      -- Voodoo2 only */
#define fbiInit5 (0x244 / 4)        /* RW     -- Voodoo2 only */
#define fbiInit6 (0x248 / 4)        /* RW     -- Voodoo2 only */
#define fbiInit7 (0x24c / 4)        /* RW     -- Voodoo2 only */
#define swapPending (0x24c / 4)     /*  W     -- Banshee only */
#define leftOverlayBuf (0x250 / 4)  /*  W     -- Banshee only */
#define rightOverlayBuf (0x254 / 4) /*  W     -- Banshee only */
#define fbiSwapHistory (0x258 / 4)  /* R      -- Voodoo2/Banshee only */
#define fbiTrianglesOut (0x25c / 4) /* R      -- Voodoo2/Banshee only */
#define sSetupMode (0x260 / 4)      /*  W PF  -- Voodoo2/Banshee only */
#define sVx (0x264 / 4)             /*  W PF  -- Voodoo2/Banshee only */
#define sVy (0x268 / 4)             /*  W PF  -- Voodoo2/Banshee only */
#define sARGB (0x26c / 4)           /*  W PF  -- Voodoo2/Banshee only */
#define sRed (0x270 / 4)            /*  W PF  -- Voodoo2/Banshee only */
#define sGreen (0x274 / 4)          /*  W PF  -- Voodoo2/Banshee only */
#define sBlue (0x278 / 4)           /*  W PF  -- Voodoo2/Banshee only */
#define sAlpha (0x27c / 4)          /*  W PF  -- Voodoo2/Banshee only */

/* 0x280 */
#define sVz (0x280 / 4)          /*  W PF  -- Voodoo2/Banshee only */
#define sWb (0x284 / 4)          /*  W PF  -- Voodoo2/Banshee only */
#define sWtmu0 (0x288 / 4)       /*  W PF  -- Voodoo2/Banshee only */
#define sS_W0 (0x28c / 4)        /*  W PF  -- Voodoo2/Banshee only */
#define sT_W0 (0x290 / 4)        /*  W PF  -- Voodoo2/Banshee only */
#define sWtmu1 (0x294 / 4)       /*  W PF  -- Voodoo2/Banshee only */
#define sS_Wtmu1 (0x298 / 4)     /*  W PF  -- Voodoo2/Banshee only */
#define sT_Wtmu1 (0x29c / 4)     /*  W PF  -- Voodoo2/Banshee only */
#define sDrawTriCMD (0x2a0 / 4)  /*  W PF  -- Voodoo2/Banshee only */
#define sBeginTriCMD (0x2a4 / 4) /*  W PF  -- Voodoo2/Banshee only */

/* 0x2c0 */
#define bltSrcBaseAddr (0x2c0 / 4)    /* RW PF  -- Voodoo2 only */
#define bltDstBaseAddr (0x2c4 / 4)    /* RW PF  -- Voodoo2 only */
#define bltXYStrides (0x2c8 / 4)      /* RW PF  -- Voodoo2 only */
#define bltSrcChromaRange (0x2cc / 4) /* RW PF  -- Voodoo2 only */
#define bltDstChromaRange (0x2d0 / 4) /* RW PF  -- Voodoo2 only */
#define bltClipX (0x2d4 / 4)          /* RW PF  -- Voodoo2 only */
#define bltClipY (0x2d8 / 4)          /* RW PF  -- Voodoo2 only */
#define bltSrcXY (0x2e0 / 4)          /* RW PF  -- Voodoo2 only */
#define bltDstXY (0x2e4 / 4)          /* RW PF  -- Voodoo2 only */
#define bltSize (0x2e8 / 4)           /* RW PF  -- Voodoo2 only */
#define bltRop (0x2ec / 4)            /* RW PF  -- Voodoo2 only */
#define bltColor (0x2f0 / 4)          /* RW PF  -- Voodoo2 only */
#define bltCommand (0x2f8 / 4)        /* RW PF  -- Voodoo2 only */
#define bltData (0x2fc / 4)           /*  W PF  -- Voodoo2 only */

/* 0x300 */
#define textureMode (0x300 / 4)     /*  W PF */
#define tLOD (0x304 / 4)            /*  W PF */
#define tDetail (0x308 / 4)         /*  W PF */
#define texBaseAddr (0x30c / 4)     /*  W PF */
#define texBaseAddr_1 (0x310 / 4)   /*  W PF */
#define texBaseAddr_2 (0x314 / 4)   /*  W PF */
#define texBaseAddr_3_8 (0x318 / 4) /*  W PF */
#define trexInit0 (0x31c / 4)       /*  W  F  -- Voodoo/Voodoo2 only */
#define trexInit1 (0x320 / 4)       /*  W  F */
#define nccTable (0x324 / 4)        /*  W  F */

// 2D registers
#define banshee2D_clip0Min (0x008 / 4)
#define banshee2D_clip0Max (0x00c / 4)
#define banshee2D_dstBaseAddr (0x010 / 4)
#define banshee2D_dstFormat (0x014 / 4)
#define banshee2D_srcColorkeyMin (0x018 / 4)
#define banshee2D_srcColorkeyMax (0x01c / 4)
#define banshee2D_dstColorkeyMin (0x020 / 4)
#define banshee2D_dstColorkeyMax (0x024 / 4)
#define banshee2D_bresError0 (0x028 / 4)
#define banshee2D_bresError1 (0x02c / 4)
#define banshee2D_rop (0x030 / 4)
#define banshee2D_srcBaseAddr (0x034 / 4)
#define banshee2D_commandExtra (0x038 / 4)
#define banshee2D_lineStipple (0x03c / 4)
#define banshee2D_lineStyle (0x040 / 4)
#define banshee2D_pattern0Alias (0x044 / 4)
#define banshee2D_pattern1Alias (0x048 / 4)
#define banshee2D_clip1Min (0x04c / 4)
#define banshee2D_clip1Max (0x050 / 4)
#define banshee2D_srcFormat (0x054 / 4)
#define banshee2D_srcSize (0x058 / 4)
#define banshee2D_srcXY (0x05c / 4)
#define banshee2D_colorBack (0x060 / 4)
#define banshee2D_colorFore (0x064 / 4)
#define banshee2D_dstSize (0x068 / 4)
#define banshee2D_dstXY (0x06c / 4)
#define banshee2D_command (0x070 / 4)

/*************************************
 *
 *  Alias map of the first 64
 *  registers when remapped
 *
 *************************************/

static const u8 register_alias_map[0x40] = {
  vdstatus,    0x004 / 4, vertexAx,  vertexAy,  vertexBx,  vertexBy,  vertexCx,  vertexCy,  startR,  dRdX,    dRdY,
  startG,      dGdX,      dGdY,      startB,    dBdX,      dBdY,      startZ,    dZdX,      dZdY,    startA,  dAdX,
  dAdY,        startS,    dSdX,      dSdY,      startT,    dTdX,      dTdY,      startW,    dWdX,    dWdY,

  triangleCMD, 0x084 / 4, fvertexAx, fvertexAy, fvertexBx, fvertexBy, fvertexCx, fvertexCy, fstartR, fdRdX,   fdRdY,
  fstartG,     fdGdX,     fdGdY,     fstartB,   fdBdX,     fdBdY,     fstartZ,   fdZdX,     fdZdY,   fstartA, fdAdX,
  fdAdY,       fstartS,   fdSdX,     fdSdY,     fstartT,   fdTdX,     fdTdY,     fstartW,   fdWdX,   fdWdY};

/*************************************
 *
 *  Table of per-register access rights
 *
 *************************************/

static const u8 voodoo_register_access[0x100] = {
  /* 0x000 */
  REG_RP, 0, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF,
  REG_WPF, REG_WPF,

  /* 0x040 */
  REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF,
  REG_WPF, REG_WPF, REG_WPF,

  /* 0x080 */
  REG_WPF, 0, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF,
  REG_WPF, REG_WPF, REG_WPF,

  /* 0x0c0 */
  REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF,
  REG_WPF, REG_WPF, REG_WPF,

  /* 0x100 */
  REG_WPF, REG_RWPF, REG_RWPF, REG_RWPF, REG_RWF, REG_RWF, REG_RWF, REG_RWF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF,
  REG_WF, 0, 0,

  /* 0x140 */
  REG_RWF, REG_RWF, REG_RWF, REG_R, REG_R, REG_R, REG_R, REG_R, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF,
  REG_WF,

  /* 0x180 */
  REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF,
  REG_WF, REG_WF,

  /* 0x1c0 */
  REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 0x200 */
  REG_RW, REG_R, REG_RW, REG_RW, REG_RW, REG_RW, REG_RW, REG_RW, REG_W, REG_W, REG_W, REG_W, REG_W, 0, 0, 0,

  /* 0x240 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 0x280 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 0x2c0 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* 0x300 */
  REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF,
  REG_WF, REG_WF,

  /* 0x340 */
  REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF,
  REG_WF, REG_WF,

  /* 0x380 */
  REG_WF};

static const u8 voodoo2_register_access[0x100] = {
  /* 0x000 */
  REG_RP, REG_RWPT, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF,
  REG_WPF, REG_WPF, REG_WPF,

  /* 0x040 */
  REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF,
  REG_WPF, REG_WPF, REG_WPF,

  /* 0x080 */
  REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF,
  REG_WPF, REG_WPF, REG_WPF,

  /* 0x0c0 */
  REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF,
  REG_WPF, REG_WPF, REG_WPF,

  /* 0x100 */
  REG_WPF, REG_RWPF, REG_RWPF, REG_RWPF, REG_RWF, REG_RWF, REG_RWF, REG_RWF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF,
  REG_WF, REG_WF, REG_WF,

  /* 0x140 */
  REG_RWF, REG_RWF, REG_RWF, REG_R, REG_R, REG_R, REG_R, REG_R, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF,
  REG_WF,

  /* 0x180 */
  REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF,
  REG_WF, REG_WF,

  /* 0x1c0 */
  REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_RWT, REG_RWT, REG_RWT, REG_RWT, REG_RWT, REG_RWT,
  REG_RWT, REG_RW,

  /* 0x200 */
  REG_RWT, REG_R, REG_RWT, REG_RWT, REG_RWT, REG_RWT, REG_RWT, REG_RWT, REG_WT, REG_WT, REG_WF, REG_WT, REG_WT, REG_WT,
  REG_WT, REG_WT,

  /* 0x240 */
  REG_R, REG_RWT, REG_RWT, REG_RWT, 0, 0, REG_R, REG_R, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF,
  REG_WPF,

  /* 0x280 */
  REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, 0, 0, 0, 0, 0, 0,

  /* 0x2c0 */
  REG_RWPF, REG_RWPF, REG_RWPF, REG_RWPF, REG_RWPF, REG_RWPF, REG_RWPF, REG_RWPF, REG_RWPF, REG_RWPF, REG_RWPF,
  REG_RWPF, REG_RWPF, REG_RWPF, REG_RWPF, REG_WPF,

  /* 0x300 */
  REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WPF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF,
  REG_WF, REG_WF,

  /* 0x340 */
  REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF, REG_WF,
  REG_WF, REG_WF,

  /* 0x380 */
  REG_WF};

/*************************************
 *
 *  Register string table for debug
 *
 *************************************/

static const char* const voodoo_reg_name[] = {
  /* 0x000 */
  "status", "{intrCtrl}", "vertexAx", "vertexAy", "vertexBx", "vertexBy", "vertexCx", "vertexCy", "startR", "startG",
  "startB", "startZ", "startA", "startS", "startT", "startW",
  /* 0x040 */
  "dRdX", "dGdX", "dBdX", "dZdX", "dAdX", "dSdX", "dTdX", "dWdX", "dRdY", "dGdY", "dBdY", "dZdY", "dAdY", "dSdY",
  "dTdY", "dWdY",
  /* 0x080 */
  "triangleCMD", "reserved084", "fvertexAx", "fvertexAy", "fvertexBx", "fvertexBy", "fvertexCx", "fvertexCy", "fstartR",
  "fstartG", "fstartB", "fstartZ", "fstartA", "fstartS", "fstartT", "fstartW",
  /* 0x0c0 */
  "fdRdX", "fdGdX", "fdBdX", "fdZdX", "fdAdX", "fdSdX", "fdTdX", "fdWdX", "fdRdY", "fdGdY", "fdBdY", "fdZdY", "fdAdY",
  "fdSdY", "fdTdY", "fdWdY",
  /* 0x100 */
  "ftriangleCMD", "fbzColorPath", "fogMode", "alphaMode", "fbzMode", "lfbMode", "clipLeftRight", "clipLowYHighY",
  "nopCMD", "fastfillCMD", "swapbufferCMD", "fogColor", "zaColor", "chromaKey", "{chromaRange}", "{userIntrCMD}",
  /* 0x140 */
  "stipple", "color0", "color1", "fbiPixelsIn", "fbiChromaFail", "fbiZfuncFail", "fbiAfuncFail", "fbiPixelsOut",
  "fogTable160", "fogTable164", "fogTable168", "fogTable16c", "fogTable170", "fogTable174", "fogTable178",
  "fogTable17c",
  /* 0x180 */
  "fogTable180", "fogTable184", "fogTable188", "fogTable18c", "fogTable190", "fogTable194", "fogTable198",
  "fogTable19c", "fogTable1a0", "fogTable1a4", "fogTable1a8", "fogTable1ac", "fogTable1b0", "fogTable1b4",
  "fogTable1b8", "fogTable1bc",
  /* 0x1c0 */
  "fogTable1c0", "fogTable1c4", "fogTable1c8", "fogTable1cc", "fogTable1d0", "fogTable1d4", "fogTable1d8",
  "fogTable1dc", "{cmdFifoBaseAddr}", "{cmdFifoBump}", "{cmdFifoRdPtr}", "{cmdFifoAMin}", "{cmdFifoAMax}",
  "{cmdFifoDepth}", "{cmdFifoHoles}", "reserved1fc",
  /* 0x200 */
  "fbiInit4", "vRetrace", "backPorch", "videoDimensions", "fbiInit0", "fbiInit1", "fbiInit2", "fbiInit3", "hSync",
  "vSync", "clutData", "dacData", "maxRgbDelta", "{hBorder}", "{vBorder}", "{borderColor}",
  /* 0x240 */
  "{hvRetrace}", "{fbiInit5}", "{fbiInit6}", "{fbiInit7}", "reserved250", "reserved254", "{fbiSwapHistory}",
  "{fbiTrianglesOut}", "{sSetupMode}", "{sVx}", "{sVy}", "{sARGB}", "{sRed}", "{sGreen}", "{sBlue}", "{sAlpha}",
  /* 0x280 */
  "{sVz}", "{sWb}", "{sWtmu0}", "{sS/Wtmu0}", "{sT/Wtmu0}", "{sWtmu1}", "{sS/Wtmu1}", "{sT/Wtmu1}", "{sDrawTriCMD}",
  "{sBeginTriCMD}", "reserved2a8", "reserved2ac", "reserved2b0", "reserved2b4", "reserved2b8", "reserved2bc",
  /* 0x2c0 */
  "{bltSrcBaseAddr}", "{bltDstBaseAddr}", "{bltXYStrides}", "{bltSrcChromaRange}", "{bltDstChromaRange}", "{bltClipX}",
  "{bltClipY}", "reserved2dc", "{bltSrcXY}", "{bltDstXY}", "{bltSize}", "{bltRop}", "{bltColor}", "reserved2f4",
  "{bltCommand}", "{bltData}",
  /* 0x300 */
  "textureMode", "tLOD", "tDetail", "texBaseAddr", "texBaseAddr_1", "texBaseAddr_2", "texBaseAddr_3_8", "trexInit0",
  "trexInit1", "nccTable0.0", "nccTable0.1", "nccTable0.2", "nccTable0.3", "nccTable0.4", "nccTable0.5", "nccTable0.6",
  /* 0x340 */
  "nccTable0.7", "nccTable0.8", "nccTable0.9", "nccTable0.A", "nccTable0.B", "nccTable1.0", "nccTable1.1",
  "nccTable1.2", "nccTable1.3", "nccTable1.4", "nccTable1.5", "nccTable1.6", "nccTable1.7", "nccTable1.8",
  "nccTable1.9", "nccTable1.A",
  /* 0x380 */
  "nccTable1.B"};

/*************************************
 *
 *  Dithering tables
 *
 *************************************/

static const u8 dither_matrix_4x4[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

// static const u8 dither_matrix_2x2[16] =
//{
//      2, 10,  2, 10,
//  14,  6, 14,  6,
//      2, 10,  2, 10,
//  14,  6, 14,  6
//};
// Using this matrix allows iteagle video memory tests to pass
static const u8 dither_matrix_2x2[16] = {8, 10, 8, 10, 11, 9, 11, 9, 8, 10, 8, 10, 11, 9, 11, 9};

// Dither 4x4 subtraction matrix used in alpha blending
static const u8 dither_subtract_4x4[16] = {(15 - 0) >> 1,  (15 - 8) >> 1,  (15 - 2) >> 1,  (15 - 10) >> 1,
                                           (15 - 12) >> 1, (15 - 4) >> 1,  (15 - 14) >> 1, (15 - 6) >> 1,
                                           (15 - 3) >> 1,  (15 - 11) >> 1, (15 - 1) >> 1,  (15 - 9) >> 1,
                                           (15 - 15) >> 1, (15 - 7) >> 1,  (15 - 13) >> 1, (15 - 5) >> 1};

// Dither 2x2 subtraction matrix used in alpha blending
static const u8 dither_subtract_2x2[16] = {(15 - 8) >> 1,  (15 - 10) >> 1, (15 - 8) >> 1,  (15 - 10) >> 1,
                                           (15 - 11) >> 1, (15 - 9) >> 1,  (15 - 11) >> 1, (15 - 9) >> 1,
                                           (15 - 8) >> 1,  (15 - 10) >> 1, (15 - 8) >> 1,  (15 - 10) >> 1,
                                           (15 - 11) >> 1, (15 - 9) >> 1,  (15 - 11) >> 1, (15 - 9) >> 1};

/*************************************
 *
 *  Macros for extracting pixels
 *
 *************************************/

#define EXTRACT_565_TO_888(val, a, b, c)                                                                               \
  (a) = (((val) >> 8) & 0xf8) | (((val) >> 13) & 0x07);                                                                \
  (b) = (((val) >> 3) & 0xfc) | (((val) >> 9) & 0x03);                                                                 \
  (c) = (((val) << 3) & 0xf8) | (((val) >> 2) & 0x07);
#define EXTRACT_x555_TO_888(val, a, b, c)                                                                              \
  (a) = (((val) >> 7) & 0xf8) | (((val) >> 12) & 0x07);                                                                \
  (b) = (((val) >> 2) & 0xf8) | (((val) >> 7) & 0x07);                                                                 \
  (c) = (((val) << 3) & 0xf8) | (((val) >> 2) & 0x07);
#define EXTRACT_555x_TO_888(val, a, b, c)                                                                              \
  (a) = (((val) >> 8) & 0xf8) | (((val) >> 13) & 0x07);                                                                \
  (b) = (((val) >> 3) & 0xf8) | (((val) >> 8) & 0x07);                                                                 \
  (c) = (((val) << 2) & 0xf8) | (((val) >> 3) & 0x07);
#define EXTRACT_1555_TO_8888(val, a, b, c, d)                                                                          \
  (a) = ((int16_t)(val) >> 15) & 0xff;                                                                                 \
  EXTRACT_x555_TO_888(val, b, c, d)
#define EXTRACT_5551_TO_8888(val, a, b, c, d) EXTRACT_555x_TO_888(val, a, b, c)(d) = ((val)&0x0001) ? 0xff : 0x00;
#define EXTRACT_x888_TO_888(val, a, b, c)                                                                              \
  (a) = ((val) >> 16) & 0xff;                                                                                          \
  (b) = ((val) >> 8) & 0xff;                                                                                           \
  (c) = ((val) >> 0) & 0xff;
#define EXTRACT_888x_TO_888(val, a, b, c)                                                                              \
  (a) = ((val) >> 24) & 0xff;                                                                                          \
  (b) = ((val) >> 16) & 0xff;                                                                                          \
  (c) = ((val) >> 8) & 0xff;
#define EXTRACT_8888_TO_8888(val, a, b, c, d)                                                                          \
  (a) = ((val) >> 24) & 0xff;                                                                                          \
  (b) = ((val) >> 16) & 0xff;                                                                                          \
  (c) = ((val) >> 8) & 0xff;                                                                                           \
  (d) = ((val) >> 0) & 0xff;
#define EXTRACT_4444_TO_8888(val, a, b, c, d)                                                                          \
  (a) = (((val) >> 8) & 0xf0) | (((val) >> 12) & 0x0f);                                                                \
  (b) = (((val) >> 4) & 0xf0) | (((val) >> 8) & 0x0f);                                                                 \
  (c) = (((val) >> 0) & 0xf0) | (((val) >> 4) & 0x0f);                                                                 \
  (d) = (((val) << 4) & 0xf0) | (((val) >> 0) & 0x0f);
#define EXTRACT_332_TO_888(val, a, b, c)                                                                               \
  (a) = (((val) >> 0) & 0xe0) | (((val) >> 3) & 0x1c) | (((val) >> 6) & 0x03);                                         \
  (b) = (((val) << 3) & 0xe0) | (((val) >> 0) & 0x1c) | (((val) >> 3) & 0x03);                                         \
  (c) = (((val) << 6) & 0xc0) | (((val) << 4) & 0x30) | (((val) << 2) & 0x0c) | (((val) << 0) & 0x03);

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
#define LOGB2(x) (std::log((double)(x)) / std::log(2.0))

/*************************************
 *
 *  Macros for extracting bitfields
 *
 *************************************/

#define INITEN_ENABLE_HW_INIT(val) (((val) >> 0) & 1)
#define INITEN_ENABLE_PCI_FIFO(val) (((val) >> 1) & 1)
#define INITEN_REMAP_INIT_TO_DAC(val) (((val) >> 2) & 1)
#define INITEN_ENABLE_SNOOP0(val) (((val) >> 4) & 1)
#define INITEN_SNOOP0_MEMORY_MATCH(val) (((val) >> 5) & 1)
#define INITEN_SNOOP0_READWRITE_MATCH(val) (((val) >> 6) & 1)
#define INITEN_ENABLE_SNOOP1(val) (((val) >> 7) & 1)
#define INITEN_SNOOP1_MEMORY_MATCH(val) (((val) >> 8) & 1)
#define INITEN_SNOOP1_READWRITE_MATCH(val) (((val) >> 9) & 1)
#define INITEN_SLI_BUS_OWNER(val) (((val) >> 10) & 1)
#define INITEN_SLI_ODD_EVEN(val) (((val) >> 11) & 1)
#define INITEN_SECONDARY_REV_ID(val) (((val) >> 12) & 0xf)       /* voodoo 2 only */
#define INITEN_MFCTR_FAB_ID(val) (((val) >> 16) & 0xf)           /* voodoo 2 only */
#define INITEN_ENABLE_PCI_INTERRUPT(val) (((val) >> 20) & 1)     /* voodoo 2 only */
#define INITEN_PCI_INTERRUPT_TIMEOUT(val) (((val) >> 21) & 1)    /* voodoo 2 only */
#define INITEN_ENABLE_NAND_TREE_TEST(val) (((val) >> 22) & 1)    /* voodoo 2 only */
#define INITEN_ENABLE_SLI_ADDRESS_SNOOP(val) (((val) >> 23) & 1) /* voodoo 2 only */
#define INITEN_SLI_SNOOP_ADDRESS(val) (((val) >> 24) & 0xff)     /* voodoo 2 only */

#define FBZCP_CC_RGBSELECT(val) (((val) >> 0) & 3)
#define FBZCP_CC_ASELECT(val) (((val) >> 2) & 3)
#define FBZCP_CC_LOCALSELECT(val) (((val) >> 4) & 1)
#define FBZCP_CCA_LOCALSELECT(val) (((val) >> 5) & 3)
#define FBZCP_CC_LOCALSELECT_OVERRIDE(val) (((val) >> 7) & 1)
#define FBZCP_CC_ZERO_OTHER(val) (((val) >> 8) & 1)
#define FBZCP_CC_SUB_CLOCAL(val) (((val) >> 9) & 1)
#define FBZCP_CC_MSELECT(val) (((val) >> 10) & 7)
#define FBZCP_CC_REVERSE_BLEND(val) (((val) >> 13) & 1)
#define FBZCP_CC_ADD_ACLOCAL(val) (((val) >> 14) & 3)
#define FBZCP_CC_INVERT_OUTPUT(val) (((val) >> 16) & 1)
#define FBZCP_CCA_ZERO_OTHER(val) (((val) >> 17) & 1)
#define FBZCP_CCA_SUB_CLOCAL(val) (((val) >> 18) & 1)
#define FBZCP_CCA_MSELECT(val) (((val) >> 19) & 7)
#define FBZCP_CCA_REVERSE_BLEND(val) (((val) >> 22) & 1)
#define FBZCP_CCA_ADD_ACLOCAL(val) (((val) >> 23) & 3)
#define FBZCP_CCA_INVERT_OUTPUT(val) (((val) >> 25) & 1)
#define FBZCP_CCA_SUBPIXEL_ADJUST(val) (((val) >> 26) & 1)
#define FBZCP_TEXTURE_ENABLE(val) (((val) >> 27) & 1)
#define FBZCP_RGBZW_CLAMP(val) (((val) >> 28) & 1) /* voodoo 2 only */
#define FBZCP_ANTI_ALIAS(val) (((val) >> 29) & 1)  /* voodoo 2 only */

#define ALPHAMODE_ALPHATEST(val) (((val) >> 0) & 1)
#define ALPHAMODE_ALPHAFUNCTION(val) (((val) >> 1) & 7)
#define ALPHAMODE_ALPHABLEND(val) (((val) >> 4) & 1)
#define ALPHAMODE_ANTIALIAS(val) (((val) >> 5) & 1)
#define ALPHAMODE_SRCRGBBLEND(val) (((val) >> 8) & 15)
#define ALPHAMODE_DSTRGBBLEND(val) (((val) >> 12) & 15)
#define ALPHAMODE_SRCALPHABLEND(val) (((val) >> 16) & 15)
#define ALPHAMODE_DSTALPHABLEND(val) (((val) >> 20) & 15)
#define ALPHAMODE_ALPHAREF(val) (((val) >> 24) & 0xff)

#define FOGMODE_ENABLE_FOG(val) (((val) >> 0) & 1)
#define FOGMODE_FOG_ADD(val) (((val) >> 1) & 1)
#define FOGMODE_FOG_MULT(val) (((val) >> 2) & 1)
#define FOGMODE_FOG_ZALPHA(val) (((val) >> 3) & 3)
#define FOGMODE_FOG_CONSTANT(val) (((val) >> 5) & 1)
#define FOGMODE_FOG_DITHER(val) (((val) >> 6) & 1) /* voodoo 2 only */
#define FOGMODE_FOG_ZONES(val) (((val) >> 7) & 1)  /* voodoo 2 only */

#define FBZMODE_ENABLE_CLIPPING(val) (((val) >> 0) & 1)
#define FBZMODE_ENABLE_CHROMAKEY(val) (((val) >> 1) & 1)
#define FBZMODE_ENABLE_STIPPLE(val) (((val) >> 2) & 1)
#define FBZMODE_WBUFFER_SELECT(val) (((val) >> 3) & 1)
#define FBZMODE_ENABLE_DEPTHBUF(val) (((val) >> 4) & 1)
#define FBZMODE_DEPTH_FUNCTION(val) (((val) >> 5) & 7)
#define FBZMODE_ENABLE_DITHERING(val) (((val) >> 8) & 1)
#define FBZMODE_RGB_BUFFER_MASK(val) (((val) >> 9) & 1)
#define FBZMODE_AUX_BUFFER_MASK(val) (((val) >> 10) & 1)
#define FBZMODE_DITHER_TYPE(val) (((val) >> 11) & 1)
#define FBZMODE_STIPPLE_PATTERN(val) (((val) >> 12) & 1)
#define FBZMODE_ENABLE_ALPHA_MASK(val) (((val) >> 13) & 1)
#define FBZMODE_DRAW_BUFFER(val) (((val) >> 14) & 3)
#define FBZMODE_ENABLE_DEPTH_BIAS(val) (((val) >> 16) & 1)
#define FBZMODE_Y_ORIGIN(val) (((val) >> 17) & 1)
#define FBZMODE_ENABLE_ALPHA_PLANES(val) (((val) >> 18) & 1)
#define FBZMODE_ALPHA_DITHER_SUBTRACT(val) (((val) >> 19) & 1)
#define FBZMODE_DEPTH_SOURCE_COMPARE(val) (((val) >> 20) & 1)
#define FBZMODE_DEPTH_FLOAT_SELECT(val) (((val) >> 21) & 1) /* voodoo 2 only */

#define LFBMODE_WRITE_FORMAT(val) (((val) >> 0) & 0xf)
#define LFBMODE_WRITE_BUFFER_SELECT(val) (((val) >> 4) & 3)
#define LFBMODE_READ_BUFFER_SELECT(val) (((val) >> 6) & 3)
#define LFBMODE_ENABLE_PIXEL_PIPELINE(val) (((val) >> 8) & 1)
#define LFBMODE_RGBA_LANES(val) (((val) >> 9) & 3)
#define LFBMODE_WORD_SWAP_WRITES(val) (((val) >> 11) & 1)
#define LFBMODE_BYTE_SWIZZLE_WRITES(val) (((val) >> 12) & 1)
#define LFBMODE_Y_ORIGIN(val) (((val) >> 13) & 1)
#define LFBMODE_WRITE_W_SELECT(val) (((val) >> 14) & 1)
#define LFBMODE_WORD_SWAP_READS(val) (((val) >> 15) & 1)
#define LFBMODE_BYTE_SWIZZLE_READS(val) (((val) >> 16) & 1)

#define CHROMARANGE_BLUE_EXCLUSIVE(val) (((val) >> 24) & 1)
#define CHROMARANGE_GREEN_EXCLUSIVE(val) (((val) >> 25) & 1)
#define CHROMARANGE_RED_EXCLUSIVE(val) (((val) >> 26) & 1)
#define CHROMARANGE_UNION_MODE(val) (((val) >> 27) & 1)
#define CHROMARANGE_ENABLE(val) (((val) >> 28) & 1)

#define FBIINIT0_VGA_PASSTHRU(val) (((val) >> 0) & 1)
#define FBIINIT0_GRAPHICS_RESET(val) (((val) >> 1) & 1)
#define FBIINIT0_FIFO_RESET(val) (((val) >> 2) & 1)
#define FBIINIT0_SWIZZLE_REG_WRITES(val) (((val) >> 3) & 1)
#define FBIINIT0_STALL_PCIE_FOR_HWM(val) (((val) >> 4) & 1)
#define FBIINIT0_PCI_FIFO_LWM(val) (((val) >> 6) & 0x1f)
#define FBIINIT0_LFB_TO_MEMORY_FIFO(val) (((val) >> 11) & 1)
#define FBIINIT0_TEXMEM_TO_MEMORY_FIFO(val) (((val) >> 12) & 1)
#define FBIINIT0_ENABLE_MEMORY_FIFO(val) (((val) >> 13) & 1)
#define FBIINIT0_MEMORY_FIFO_HWM(val) (((val) >> 14) & 0x7ff)
#define FBIINIT0_MEMORY_FIFO_BURST(val) (((val) >> 25) & 0x3f)

#define FBIINIT1_PCI_DEV_FUNCTION(val) (((val) >> 0) & 1)
#define FBIINIT1_PCI_WRITE_WAIT_STATES(val) (((val) >> 1) & 1)
#define FBIINIT1_MULTI_SST1(val) (((val) >> 2) & 1) /* not on voodoo 2 */
#define FBIINIT1_ENABLE_LFB(val) (((val) >> 3) & 1)
#define FBIINIT1_X_VIDEO_TILES(val) (((val) >> 4) & 0xf)
#define FBIINIT1_VIDEO_TIMING_RESET(val) (((val) >> 8) & 1)
#define FBIINIT1_SOFTWARE_OVERRIDE(val) (((val) >> 9) & 1)
#define FBIINIT1_SOFTWARE_HSYNC(val) (((val) >> 10) & 1)
#define FBIINIT1_SOFTWARE_VSYNC(val) (((val) >> 11) & 1)
#define FBIINIT1_SOFTWARE_BLANK(val) (((val) >> 12) & 1)
#define FBIINIT1_DRIVE_VIDEO_TIMING(val) (((val) >> 13) & 1)
#define FBIINIT1_DRIVE_VIDEO_BLANK(val) (((val) >> 14) & 1)
#define FBIINIT1_DRIVE_VIDEO_SYNC(val) (((val) >> 15) & 1)
#define FBIINIT1_DRIVE_VIDEO_DCLK(val) (((val) >> 16) & 1)
#define FBIINIT1_VIDEO_TIMING_VCLK(val) (((val) >> 17) & 1)
#define FBIINIT1_VIDEO_CLK_2X_DELAY(val) (((val) >> 18) & 3)
#define FBIINIT1_VIDEO_TIMING_SOURCE(val) (((val) >> 20) & 3)
#define FBIINIT1_ENABLE_24BPP_OUTPUT(val) (((val) >> 22) & 1)
#define FBIINIT1_ENABLE_SLI(val) (((val) >> 23) & 1)
#define FBIINIT1_X_VIDEO_TILES_BIT5(val) (((val) >> 24) & 1) /* voodoo 2 only */
#define FBIINIT1_ENABLE_EDGE_FILTER(val) (((val) >> 25) & 1)
#define FBIINIT1_INVERT_VID_CLK_2X(val) (((val) >> 26) & 1)
#define FBIINIT1_VID_CLK_2X_SEL_DELAY(val) (((val) >> 27) & 3)
#define FBIINIT1_VID_CLK_DELAY(val) (((val) >> 29) & 3)
#define FBIINIT1_DISABLE_FAST_READAHEAD(val) (((val) >> 31) & 1)

#define FBIINIT2_DISABLE_DITHER_SUB(val) (((val) >> 0) & 1)
#define FBIINIT2_DRAM_BANKING(val) (((val) >> 1) & 1)
#define FBIINIT2_ENABLE_TRIPLE_BUF(val) (((val) >> 4) & 1)
#define FBIINIT2_ENABLE_FAST_RAS_READ(val) (((val) >> 5) & 1)
#define FBIINIT2_ENABLE_GEN_DRAM_OE(val) (((val) >> 6) & 1)
#define FBIINIT2_ENABLE_FAST_READWRITE(val) (((val) >> 7) & 1)
#define FBIINIT2_ENABLE_PASSTHRU_DITHER(val) (((val) >> 8) & 1)
#define FBIINIT2_SWAP_BUFFER_ALGORITHM(val) (((val) >> 9) & 3)
#define FBIINIT2_VIDEO_BUFFER_OFFSET(val) (((val) >> 11) & 0x1ff)
#define FBIINIT2_ENABLE_DRAM_BANKING(val) (((val) >> 20) & 1)
#define FBIINIT2_ENABLE_DRAM_READ_FIFO(val) (((val) >> 21) & 1)
#define FBIINIT2_ENABLE_DRAM_REFRESH(val) (((val) >> 22) & 1)
#define FBIINIT2_REFRESH_LOAD_VALUE(val) (((val) >> 23) & 0x1ff)

#define FBIINIT3_TRI_REGISTER_REMAP(val) (((val) >> 0) & 1)
#define FBIINIT3_VIDEO_FIFO_THRESH(val) (((val) >> 1) & 0x1f)
#define FBIINIT3_DISABLE_TMUS(val) (((val) >> 6) & 1)
#define FBIINIT3_FBI_MEMORY_TYPE(val) (((val) >> 8) & 7)
#define FBIINIT3_VGA_PASS_RESET_VAL(val) (((val) >> 11) & 1)
#define FBIINIT3_HARDCODE_PCI_BASE(val) (((val) >> 12) & 1)
#define FBIINIT3_FBI2TREX_DELAY(val) (((val) >> 13) & 0xf)
#define FBIINIT3_TREX2FBI_DELAY(val) (((val) >> 17) & 0x1f)
#define FBIINIT3_YORIGIN_SUBTRACT(val) (((val) >> 22) & 0x3ff)

#define FBIINIT4_PCI_READ_WAITS(val) (((val) >> 0) & 1)
#define FBIINIT4_ENABLE_LFB_READAHEAD(val) (((val) >> 1) & 1)
#define FBIINIT4_MEMORY_FIFO_LWM(val) (((val) >> 2) & 0x3f)
#define FBIINIT4_MEMORY_FIFO_START_ROW(val) (((val) >> 8) & 0x3ff)
#define FBIINIT4_MEMORY_FIFO_STOP_ROW(val) (((val) >> 18) & 0x3ff)
#define FBIINIT4_VIDEO_CLOCKING_DELAY(val) (((val) >> 29) & 7) /* voodoo 2 only */

#define FBIINIT5_DISABLE_PCI_STOP(val) (((val) >> 0) & 1)       /* voodoo 2 only */
#define FBIINIT5_PCI_SLAVE_SPEED(val) (((val) >> 1) & 1)        /* voodoo 2 only */
#define FBIINIT5_DAC_DATA_OUTPUT_WIDTH(val) (((val) >> 2) & 1)  /* voodoo 2 only */
#define FBIINIT5_DAC_DATA_17_OUTPUT(val) (((val) >> 3) & 1)     /* voodoo 2 only */
#define FBIINIT5_DAC_DATA_18_OUTPUT(val) (((val) >> 4) & 1)     /* voodoo 2 only */
#define FBIINIT5_GENERIC_STRAPPING(val) (((val) >> 5) & 0xf)    /* voodoo 2 only */
#define FBIINIT5_BUFFER_ALLOCATION(val) (((val) >> 9) & 3)      /* voodoo 2 only */
#define FBIINIT5_DRIVE_VID_CLK_SLAVE(val) (((val) >> 11) & 1)   /* voodoo 2 only */
#define FBIINIT5_DRIVE_DAC_DATA_16(val) (((val) >> 12) & 1)     /* voodoo 2 only */
#define FBIINIT5_VCLK_INPUT_SELECT(val) (((val) >> 13) & 1)     /* voodoo 2 only */
#define FBIINIT5_MULTI_CVG_DETECT(val) (((val) >> 14) & 1)      /* voodoo 2 only */
#define FBIINIT5_SYNC_RETRACE_READS(val) (((val) >> 15) & 1)    /* voodoo 2 only */
#define FBIINIT5_ENABLE_RHBORDER_COLOR(val) (((val) >> 16) & 1) /* voodoo 2 only */
#define FBIINIT5_ENABLE_LHBORDER_COLOR(val) (((val) >> 17) & 1) /* voodoo 2 only */
#define FBIINIT5_ENABLE_BVBORDER_COLOR(val) (((val) >> 18) & 1) /* voodoo 2 only */
#define FBIINIT5_ENABLE_TVBORDER_COLOR(val) (((val) >> 19) & 1) /* voodoo 2 only */
#define FBIINIT5_DOUBLE_HORIZ(val) (((val) >> 20) & 1)          /* voodoo 2 only */
#define FBIINIT5_DOUBLE_VERT(val) (((val) >> 21) & 1)           /* voodoo 2 only */
#define FBIINIT5_ENABLE_16BIT_GAMMA(val) (((val) >> 22) & 1)    /* voodoo 2 only */
#define FBIINIT5_INVERT_DAC_HSYNC(val) (((val) >> 23) & 1)      /* voodoo 2 only */
#define FBIINIT5_INVERT_DAC_VSYNC(val) (((val) >> 24) & 1)      /* voodoo 2 only */
#define FBIINIT5_ENABLE_24BIT_DACDATA(val) (((val) >> 25) & 1)  /* voodoo 2 only */
#define FBIINIT5_ENABLE_INTERLACING(val) (((val) >> 26) & 1)    /* voodoo 2 only */
#define FBIINIT5_DAC_DATA_18_CONTROL(val) (((val) >> 27) & 1)   /* voodoo 2 only */
#define FBIINIT5_RASTERIZER_UNIT_MODE(val) (((val) >> 30) & 3)  /* voodoo 2 only */

#define FBIINIT6_WINDOW_ACTIVE_COUNTER(val) (((val) >> 0) & 7)  /* voodoo 2 only */
#define FBIINIT6_WINDOW_DRAG_COUNTER(val) (((val) >> 3) & 0x1f) /* voodoo 2 only */
#define FBIINIT6_SLI_SYNC_MASTER(val) (((val) >> 8) & 1)        /* voodoo 2 only */
#define FBIINIT6_DAC_DATA_22_OUTPUT(val) (((val) >> 9) & 3)     /* voodoo 2 only */
#define FBIINIT6_DAC_DATA_23_OUTPUT(val) (((val) >> 11) & 3)    /* voodoo 2 only */
#define FBIINIT6_SLI_SYNCIN_OUTPUT(val) (((val) >> 13) & 3)     /* voodoo 2 only */
#define FBIINIT6_SLI_SYNCOUT_OUTPUT(val) (((val) >> 15) & 3)    /* voodoo 2 only */
#define FBIINIT6_DAC_RD_OUTPUT(val) (((val) >> 17) & 3)         /* voodoo 2 only */
#define FBIINIT6_DAC_WR_OUTPUT(val) (((val) >> 19) & 3)         /* voodoo 2 only */
#define FBIINIT6_PCI_FIFO_LWM_RDY(val) (((val) >> 21) & 0x7f)   /* voodoo 2 only */
#define FBIINIT6_VGA_PASS_N_OUTPUT(val) (((val) >> 28) & 3)     /* voodoo 2 only */
#define FBIINIT6_X_VIDEO_TILES_BIT0(val) (((val) >> 30) & 1)    /* voodoo 2 only */

#define FBIINIT7_GENERIC_STRAPPING(val) (((val) >> 0) & 0xff)    /* voodoo 2 only */
#define FBIINIT7_CMDFIFO_ENABLE(val) (((val) >> 8) & 1)          /* voodoo 2 only */
#define FBIINIT7_CMDFIFO_MEMORY_STORE(val) (((val) >> 9) & 1)    /* voodoo 2 only */
#define FBIINIT7_DISABLE_CMDFIFO_HOLES(val) (((val) >> 10) & 1)  /* voodoo 2 only */
#define FBIINIT7_CMDFIFO_READ_THRESH(val) (((val) >> 11) & 0x1f) /* voodoo 2 only */
#define FBIINIT7_SYNC_CMDFIFO_WRITES(val) (((val) >> 16) & 1)    /* voodoo 2 only */
#define FBIINIT7_SYNC_CMDFIFO_READS(val) (((val) >> 17) & 1)     /* voodoo 2 only */
#define FBIINIT7_RESET_PCI_PACKER(val) (((val) >> 18) & 1)       /* voodoo 2 only */
#define FBIINIT7_ENABLE_CHROMA_STUFF(val) (((val) >> 19) & 1)    /* voodoo 2 only */
#define FBIINIT7_CMDFIFO_PCI_TIMEOUT(val) (((val) >> 20) & 0x7f) /* voodoo 2 only */
#define FBIINIT7_ENABLE_TEXTURE_BURST(val) (((val) >> 27) & 1)   /* voodoo 2 only */

#define TEXMODE_ENABLE_PERSPECTIVE(val) (((val) >> 0) & 1)
#define TEXMODE_MINIFICATION_FILTER(val) (((val) >> 1) & 1)
#define TEXMODE_MAGNIFICATION_FILTER(val) (((val) >> 2) & 1)
#define TEXMODE_CLAMP_NEG_W(val) (((val) >> 3) & 1)
#define TEXMODE_ENABLE_LOD_DITHER(val) (((val) >> 4) & 1)
#define TEXMODE_NCC_TABLE_SELECT(val) (((val) >> 5) & 1)
#define TEXMODE_CLAMP_S(val) (((val) >> 6) & 1)
#define TEXMODE_CLAMP_T(val) (((val) >> 7) & 1)
#define TEXMODE_FORMAT(val) (((val) >> 8) & 0xf)
#define TEXMODE_TC_ZERO_OTHER(val) (((val) >> 12) & 1)
#define TEXMODE_TC_SUB_CLOCAL(val) (((val) >> 13) & 1)
#define TEXMODE_TC_MSELECT(val) (((val) >> 14) & 7)
#define TEXMODE_TC_REVERSE_BLEND(val) (((val) >> 17) & 1)
#define TEXMODE_TC_ADD_ACLOCAL(val) (((val) >> 18) & 3)
#define TEXMODE_TC_INVERT_OUTPUT(val) (((val) >> 20) & 1)
#define TEXMODE_TCA_ZERO_OTHER(val) (((val) >> 21) & 1)
#define TEXMODE_TCA_SUB_CLOCAL(val) (((val) >> 22) & 1)
#define TEXMODE_TCA_MSELECT(val) (((val) >> 23) & 7)
#define TEXMODE_TCA_REVERSE_BLEND(val) (((val) >> 26) & 1)
#define TEXMODE_TCA_ADD_ACLOCAL(val) (((val) >> 27) & 3)
#define TEXMODE_TCA_INVERT_OUTPUT(val) (((val) >> 29) & 1)
#define TEXMODE_TRILINEAR(val) (((val) >> 30) & 1)
#define TEXMODE_SEQ_8_DOWNLD(val) (((val) >> 31) & 1)

#define TEXLOD_LODMIN(val) (((val) >> 0) & 0x3f)
#define TEXLOD_LODMAX(val) (((val) >> 6) & 0x3f)
#define TEXLOD_LODBIAS(val) (((val) >> 12) & 0x3f)
#define TEXLOD_LOD_ODD(val) (((val) >> 18) & 1)
#define TEXLOD_LOD_TSPLIT(val) (((val) >> 19) & 1)
#define TEXLOD_LOD_S_IS_WIDER(val) (((val) >> 20) & 1)
#define TEXLOD_LOD_ASPECT(val) (((val) >> 21) & 3)
#define TEXLOD_LOD_ZEROFRAC(val) (((val) >> 23) & 1)
#define TEXLOD_TMULTIBASEADDR(val) (((val) >> 24) & 1)
#define TEXLOD_TDATA_SWIZZLE(val) (((val) >> 25) & 1)
#define TEXLOD_TDATA_SWAP(val) (((val) >> 26) & 1)
#define TEXLOD_TDIRECT_WRITE(val) (((val) >> 27) & 1) /* Voodoo 2 only */

#define TEXDETAIL_DETAIL_MAX(val) (((val) >> 0) & 0xff)
#define TEXDETAIL_DETAIL_BIAS(val) (((val) >> 8) & 0x3f)
#define TEXDETAIL_DETAIL_SCALE(val) (((val) >> 14) & 7)
#define TEXDETAIL_RGB_MIN_FILTER(val) (((val) >> 17) & 1)       /* Voodoo 2 only */
#define TEXDETAIL_RGB_MAG_FILTER(val) (((val) >> 18) & 1)       /* Voodoo 2 only */
#define TEXDETAIL_ALPHA_MIN_FILTER(val) (((val) >> 19) & 1)     /* Voodoo 2 only */
#define TEXDETAIL_ALPHA_MAG_FILTER(val) (((val) >> 20) & 1)     /* Voodoo 2 only */
#define TEXDETAIL_SEPARATE_RGBA_FILTER(val) (((val) >> 21) & 1) /* Voodoo 2 only */

#define TREXINIT_SEND_TMU_CONFIG(val) (((val) >> 18) & 1)

union voodoo_reg
{
  s32 i;
  u32 u;
  float f;
  struct
  {
    // Assumes little endian.
    u8 b, g, r, a;
  } rgb;
};

/***************************************************************************
    CONSTANTS
***************************************************************************/
/* enumeration specifying which model of Voodoo we are emulating */
enum
{
  TYPE_VOODOO_1,
  TYPE_VOODOO_2
};

#define STD_VOODOO_1_CLOCK 50000000
#define STD_VOODOO_2_CLOCK 90000000

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* ----- device interface ----- */

class voodoo_device
{
public:
  voodoo_device(u32 clock, u8 vdt);
  ~voodoo_device();

  void initialize(System* system, Bus* bus, Display* display);
  void reset();

  void set_fbmem(int value) { m_fbmem = value; }
  void set_tmumem(int value1, int value2)
  {
    m_tmumem0 = value1;
    m_tmumem1 = value2;
  }

  u32 voodoo_r(u32 offset);
  void voodoo_w(u32 offset, u32 data, u32 mask);

  void pciint(bool state);

  u8 m_fbmem = 4;
  u8 m_tmumem0 = 8;
  u8 m_tmumem1 = 8;

  // This is for internally generated PCI interrupts in Voodoo3
  std::function<void()> m_pciint;

  void vblank_callback(CycleCount time_late);
  void vblank_off_callback(CycleCount time_late);

  void voodoo_postload();

  void voodoo_update(u32 end_line);
  int voodoo_get_type();
  void voodoo_set_init_enable(u32 newval);

protected:
  struct tmu_shared_state;

  struct voodoo_stats
  {
    u8 display;            // display stats?
    s32 swaps;             // total swaps
    s32 stalls;            // total stalls
    s32 total_triangles;   // total triangles
    s32 total_pixels_in;   // total pixels in
    s32 total_pixels_out;  // total pixels out
    s32 total_chroma_fail; // total chroma fail
    s32 total_zfunc_fail;  // total z func fail
    s32 total_afunc_fail;  // total a func fail
    s32 total_clipped;     // total clipped
    s32 total_stippled;    // total stippled
    s32 lfb_writes;        // LFB writes
    s32 lfb_reads;         // LFB reads
    s32 reg_writes;        // register writes
    s32 reg_reads;         // register reads
    s32 tex_writes;        // texture writes
    s32 texture_mode[16];  // 16 different texture modes
    u8 render_override;    // render override
    char buffer[1024];     // string
  };

  /* note that this structure is an even 64 bytes long */
  struct stats_block
  {
    s32 pixels_in;          // pixels in statistic
    s32 pixels_out;         // pixels out statistic
    s32 chroma_fail;        // chroma test fail statistic
    s32 zfunc_fail;         // z function test fail statistic
    s32 afunc_fail;         // alpha function test fail statistic
    s32 clip_fail;          // clipping fail statistic
    s32 stipple_count;      // stipple statistic
    s32 filler[64 / 4 - 7]; // pad this structure to 64 bytes
  };

  struct fifo_state
  {
    void reset() { in = out = 0; }
    void add(u32 data);
    u32 remove();
    u32 peek() { return base[out]; }
    bool empty() const { return in == out; }
    bool full() const { return ((in + 1) == out) || ((in == (size - 1)) && (out == 0)); }
    s32 items() const;
    s32 space() const { return size - 1 - items(); }

    u32* base; // base of the FIFO
    s32 size;  // size of the FIFO
    s32 in;    // input pointer
    s32 out;   // output pointer
  };

  struct cmdfifo_info
  {
    u8 enable;      // enabled?
    u8 count_holes; // count holes?
    u32 base;       // base address in framebuffer RAM
    u32 end;        // end address in framebuffer RAM
    u32 rdptr;      // current read pointer
    u32 amin;       // minimum address
    u32 amax;       // maximum address
    u32 depth;      // current depth
    u32 holes;      // number of holes
  };

  struct pci_state
  {
    fifo_state fifo;            // PCI FIFO
    u32 init_enable;            // initEnable value
    u8 op_pending;              // true if an operation is pending
    SimulationTime op_end_time; // time when the pending operation ends
    u32 fifo_mem[64 * 2];       // memory backing the PCI FIFO
  };

  struct tmu_state
  {
    class stw_t;
    void recompute_texture_params();
    void init(u8 vdt, tmu_shared_state& share, voodoo_reg* r, void* memory, int tmem);
    s32 prepare();
    static s32 new_log2(double& value, const int& offset);
    rgbaint_t genTexture(s32 x, const u8* dither4, const u32 TEXMODE, rgb_t* LOOKUP, s32 LODBASE, const stw_t& iterstw,
                         s32& lod);
    rgbaint_t combineTexture(const u32 TEXMODE, const rgbaint_t& c_local, const rgbaint_t& c_other, s32 lod);

    struct ncc_table
    {
      void write(u32 regnum, u32 data);
      void update();

      u8 dirty;                // is the texel lookup dirty?
      voodoo_reg* reg;         // pointer to our registers
      s32 ir[4], ig[4], ib[4]; // I values for R,G,B
      s32 qr[4], qg[4], qb[4]; // Q values for R,G,B
      s32 y[16];               // Y values
      rgb_t* palette;          // pointer to associated RGB palette
      rgb_t* palettea;         // pointer to associated ARGB palette
      rgb_t texel[256];        // texel lookup
    };

    u8* ram;         // pointer to our RAM
    u32 mask;        // mask to apply to pointers
    voodoo_reg* reg; // pointer to our register base
    u32 regdirty;    // true if the LOD/mode/base registers have changed

    u32 texaddr_mask; // mask for texture address
    u8 texaddr_shift; // shift for texture address

    s64 starts, startt; // starting S,T (14.18)
    s64 startw;         // starting W (2.30)
    s64 dsdx, dtdx;     // delta S,T per X
    s64 dwdx;           // delta W per X
    s64 dsdy, dtdy;     // delta S,T per Y
    s64 dwdy;           // delta W per Y

    s32 lodmin, lodmax; // min, max LOD values
    s32 lodbias;        // LOD bias
    u32 lodmask;        // mask of available LODs
    u32 lodoffset[9];   // offset of texture base for each LOD
    s32 detailmax;      // detail clamp
    s32 detailbias;     // detail bias
    u8 detailscale;     // detail scale

    u32 wmask; // mask for the current texture width
    u32 hmask; // mask for the current texture height

    u32 bilinear_mask; // mask for bilinear resolution (0xf0 for V1, 0xff for V2)

    ncc_table ncc[2]; // two NCC tables

    rgb_t* lookup;    // currently selected lookup
    rgb_t* texel[16]; // texel lookups for each format

    rgb_t palette[256];  // palette lookup table
    rgb_t palettea[256]; // palette+alpha lookup table
  };

  struct tmu_shared_state
  {
    void init();

    rgb_t rgb332[256]; // RGB 3-3-2 lookup table
    rgb_t alpha8[256]; // alpha 8-bit lookup table
    rgb_t int8[256];   // intensity 8-bit lookup table
    rgb_t ai44[256];   // alpha, intensity 4-4 lookup table

    rgb_t* rgb565;         // RGB 5-6-5 lookup table
    rgb_t argb1555[65536]; // ARGB 1-5-5-5 lookup table
    rgb_t argb4444[65536]; // ARGB 4-4-4-4 lookup table
  };

  struct fbi_state
  {
    struct setup_vertex
    {
      float x, y;       // X, Y coordinates
      float z, wb;      // Z and broadcast W values
      float r, g, b, a; // A, R, G, B values
      float s0, t0, w0; // W, S, T for TMU 0
      float s1, t1, w1; // W, S, T for TMU 1
    };

    u8* ram;        // pointer to frame buffer RAM
    u32 mask;       // mask to apply to pointers
    u32 rgboffs[3]; // word offset to 3 RGB buffers
    u32 auxoffs;    // word offset to 1 aux buffer

    u8 frontbuf;      // front buffer index
    u8 backbuf;       // back buffer index
    u8 swaps_pending; // number of pending swaps
    u8 video_changed; // did the frontbuffer video change?

    u32 yorigin;   // Y origin subtract value
    u32 lfb_base;  // base of LFB in memory
    u8 lfb_stride; // stride of LFB accesses in bits

    u32 width;       // width of current frame buffer
    u32 height;      // height of current frame buffer
    u32 rowpixels;   // pixels per row
    u32 tile_width;  // width of video tiles
    u32 tile_height; // height of video tiles
    u32 x_tiles;     // number of tiles in the X direction

    std::unique_ptr<TimingEvent> vsync_stop_timer;  // VBLANK End timer
    std::unique_ptr<TimingEvent> vsync_start_timer; // VBLANK timer
    u8 vblank;                              // VBLANK state
    u8 vblank_count;                        // number of VBLANKs since last swap
    u8 vblank_swap_pending;                 // a swap is pending, waiting for a vblank
    u8 vblank_swap;                         // swap when we hit this count
    u8 vblank_dont_swap;                    // don't actually swap when we hit this point

    /* triangle setup info */
    u8 cheating_allowed;                // allow cheating?
    s32 sign;                           // triangle sign
    int16_t ax, ay;                     // vertex A x,y (12.4)
    int16_t bx, by;                     // vertex B x,y (12.4)
    int16_t cx, cy;                     // vertex C x,y (12.4)
    s32 startr, startg, startb, starta; // starting R,G,B,A (12.12)
    s32 startz;                         // starting Z (20.12)
    s64 startw;                         // starting W (16.32)
    s32 drdx, dgdx, dbdx, dadx;         // delta R,G,B,A per X
    s32 dzdx;                           // delta Z per X
    s64 dwdx;                           // delta W per X
    s32 drdy, dgdy, dbdy, dady;         // delta R,G,B,A per Y
    s32 dzdy;                           // delta Z per Y
    s64 dwdy;                           // delta W per Y

    stats_block lfb_stats; // LFB-access statistics

    u8 sverts;             // number of vertices ready */
    setup_vertex svert[3]; // 3 setup vertices */

    fifo_state fifo;         // framebuffer memory fifo */
    cmdfifo_info cmdfifo[2]; // command FIFOs */

    u8 fogblend[64];  // 64-entry fog table */
    u8 fogdelta[64];  // 64-entry fog table */
    u8 fogdelta_mask; // mask for for delta (0xff for V1, 0xfc for V2) */

    rgb_t pen[65536];    // mapping from pixels to pens */
    rgb_t clut[512];     // clut gamma data */
    u8 clut_dirty;       // do we need to recompute? */
    rgb_t rgb565[65536]; // RGB 5-6-5 lookup table */
  };

  struct dac_state
  {
    void data_w(u8 regum, u8 data);
    void data_r(u8 regnum);

    u8 reg[8];      // 8 registers
    u8 read_result; // pending read result
  };

  struct raster_info
  {
    u32 compute_hash() const;

    raster_info* next;                // pointer to next entry with the same hash
    poly_draw_scanline_func callback; // callback pointer
    bool is_generic;                  // true if this is one of the generic rasterizers
    u8 display;                       // display index
    u32 hits;                         // how many hits (pixels) we've used this for
    u32 polys;                        // how many polys we've used this for
    u32 eff_color_path;               // effective fbzColorPath value
    u32 eff_alpha_mode;               // effective alphaMode value
    u32 eff_fog_mode;                 // effective fogMode value
    u32 eff_fbz_mode;                 // effective fbzMode value
    u32 eff_tex_mode_0;               // effective textureMode value for TMU #0
    u32 eff_tex_mode_1;               // effective textureMode value for TMU #1
    u32 hash;
  };

  struct poly_extra_data;

  static const raster_info predef_raster_table[];

  // not all of these need to be static, review.

  static void flush_fifos(voodoo_device* vd);
  static void init_fbi(voodoo_device* vd, fbi_state* f, void* memory, int fbmem);
  static s32 register_w(voodoo_device* vd, u32 offset, u32 data);
  static s32 swapbuffer(voodoo_device* vd, u32 data);
  static s32 lfb_w(voodoo_device* vd, u32 offset, u32 data, u32 mem_mask);
  static s32 texture_w(voodoo_device* vd, u32 offset, u32 data);
  s32 lfb_direct_w(u32 offset, u32 data, u32 mem_mask);
  void stall_cpu(int state);
  void soft_reset();
  void recompute_video_memory();
  static s32 fastfill(voodoo_device* vd);
  static s32 triangle(voodoo_device* vd);
  static s32 begin_triangle(voodoo_device* vd);
  static s32 draw_triangle(voodoo_device* vd);
  static s32 setup_and_draw_triangle(voodoo_device* vd);
  static s32 triangle_create_work_item(voodoo_device* vd, uint16_t* drawbuf, int texcount);
  static raster_info* add_rasterizer(voodoo_device* vd, const raster_info* cinfo);
  static raster_info* find_rasterizer(voodoo_device* vd, int texcount);
  static void dump_rasterizer_stats(voodoo_device* vd);
  static void blit(voodoo_device* vd);

  void accumulate_statistics(const stats_block& block);
  void update_statistics(bool accumulate);
  void reset_counters();

  static u32 register_r(voodoo_device* vd, u32 offset);

  static void swap_buffers(voodoo_device* vd);
  int cmdfifo_compute_expected_depth(cmdfifo_info& f);
  static u32 cmdfifo_execute(voodoo_device* vd, cmdfifo_info* f);
  s32 cmdfifo_execute_if_ready(cmdfifo_info& f);
  static void cmdfifo_w(voodoo_device* vd, cmdfifo_info* f, u32 offset, u32 data);

  static void init_save_state(voodoo_device* vd);

  static void raster_fastfill(void* dest, s32 scanline, const poly_extent* extent, const void* extradata, int threadid);
  static void raster_generic_0tmu(void* dest, s32 scanline, const poly_extent* extent, const void* extradata,
                                  int threadid);
  static void raster_generic_1tmu(void* dest, s32 scanline, const poly_extent* extent, const void* extradata,
                                  int threadid);
  static void raster_generic_2tmu(void* dest, s32 scanline, const poly_extent* extent, const void* extradata,
                                  int threadid);

#define RASTERIZER_HEADER(name)                                                                                        \
  static void raster_##name(void* destbase, s32 y, const poly_extent* extent, const void* extradata, int threadid);
#define RASTERIZER_ENTRY(fbzcp, alpha, fog, fbz, tex0, tex1)                                                           \
  RASTERIZER_HEADER(fbzcp##_##alpha##_##fog##_##fbz##_##tex0##_##tex1)
#include "voodoo_rast.hxx"

#undef RASTERIZER_ENTRY

  static bool chromaKeyTest(voodoo_device* vd, stats_block* stats, u32 fbzModeReg, rgbaint_t rgaIntColor);
  static bool alphaMaskTest(stats_block* stats, u32 fbzModeReg, u8 alpha);
  static bool alphaTest(u8 alpharef, stats_block* stats, u32 alphaModeReg, u8 alpha);
  static bool depthTest(uint16_t zaColorReg, stats_block* stats, s32 destDepth, u32 fbzModeReg, s32 biasdepth);
  static bool combineColor(voodoo_device* vd, stats_block* STATS, u32 FBZCOLORPATH, u32 FBZMODE, rgbaint_t TEXELARGB,
                           s32 ITERZ, s64 ITERW, rgbaint_t& srcColor);

  // FIXME: this stuff should not be public
public:
  u8 index;                             // index of board
  const u8 vd_type;                     // type of system
  u8 chipmask;                          // mask for which chips are available
  u32 freq;                             // operating frequency
  SimulationTime attoseconds_per_cycle; // attoseconds per cycle
  u32 extra_cycles;                     // extra cycles not yet accounted for

  voodoo_reg reg[0x400];       // raw registers
  const u8* regaccess;         // register access array
  const char* const* regnames; // register names array
  u8 alt_regmap;               // enable alternate register map?

  pci_state pci; // PCI state
  dac_state dac; // DAC state

  fbi_state fbi;             // FBI states
  tmu_state tmu[MAX_TMU];    // TMU states
  tmu_shared_state tmushare; // TMU shared state

  legacy_poly_manager* poly; // polygon manager
  stats_block* thread_stats; // per-thread statistics

  voodoo_stats stats; // internal statistics

  u32 last_status_pc;    // PC of last status description (for logging)
  u32 last_status_value; // value of last status read (for logging)

  int next_rasterizer;                        // next rasterizer index
  raster_info rasterizer[MAX_RASTERIZERS];    // array of rasterizers
  raster_info* raster_hash[RASTER_HASH_SIZE]; // hash table of rasterizers

  bool send_config;
  u32 tmu_config;

  System* m_system = nullptr;
  Bus* m_bus = nullptr;
  Display* m_display = nullptr;
  DisplayTiming m_display_timing;
  u32 m_last_rendered_line = 0;
};

// use SSE on 64-bit implementations, where it can be assumed
#if 1 && ((!defined(MAME_DEBUG) || defined(__OPTIMIZE__)) && (defined(__SSE2__) || defined(_MSC_VER)) && defined(PTR64))
#include <emmintrin.h>
#ifdef __SSE4_1__
#include <smmintrin.h>
#endif
class voodoo_device::tmu_state::stw_t
{
public:
  stw_t() {}
  stw_t(const stw_t& other) = default;
  stw_t& operator=(const stw_t& other) = default;

  void set(s64 s, s64 t, s64 w)
  {
    m_st = _mm_set_pd(s, t);
    m_w = _mm_set1_pd(w);
  }
  int is_w_neg() const { return _mm_comilt_sd(m_w, _mm_set1_pd(0.0)); }
  void get_st_shiftr(s32& s, s32& t, const s32& shift) const
  {
    s64 tmpS = _mm_cvtsd_si64(_mm_shuffle_pd(m_st, _mm_setzero_pd(), 1));
    s = tmpS >> shift;
    s64 tmpT = _mm_cvtsd_si64(m_st);
    t = tmpT >> shift;
  }
  void add(const stw_t& other)
  {
    m_st = _mm_add_pd(m_st, other.m_st);
    m_w = _mm_add_pd(m_w, other.m_w);
  }
  void calc_stow(s32& sow, s32& tow, s32& oowlog) const
  {
    __m128d tmp = _mm_div_pd(m_st, m_w);
    // Allow for 8 bits of decimal in integer
    tmp = _mm_mul_pd(tmp, _mm_set1_pd(256.0));
    __m128i tmp2 = _mm_cvttpd_epi32(tmp);
#ifdef __SSE4_1__
    sow = _mm_extract_epi32(tmp2, 1);
    tow = _mm_extract_epi32(tmp2, 0);
#else
    sow = _mm_cvtsi128_si32(_mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 0, 1)));
    tow = _mm_cvtsi128_si32(tmp2);
#endif
    double dW = _mm_cvtsd_f64(m_w);
    oowlog = -new_log2(dW, 0);
  }

private:
  __m128d m_st;
  __m128d m_w;
};
#else
class voodoo_device::tmu_state::stw_t
{
public:
  stw_t() {}
  stw_t(const stw_t& other) = default;
  stw_t& operator=(const stw_t& other) = default;

  void set(s64 s, s64 t, s64 w)
  {
    m_s = s;
    m_t = t;
    m_w = w;
  }
  int is_w_neg() const { return (m_w < 0) ? 1 : 0; }
  void get_st_shiftr(s32& s, s32& t, const s32& shift) const
  {
    s = m_s >> shift;
    t = m_t >> shift;
  }
  inline void add(const stw_t& other)
  {
    m_s += other.m_s;
    m_t += other.m_t;
    m_w += other.m_w;
  }
  // Computes s/w and t/w and returns log2 of 1/w
  // s, t and c are 16.32 values.  The results are 24.8.
  inline void calc_stow(s32& sow, s32& tow, s32& oowlog) const
  {
    double recip = double(1ULL << (47 - 39)) / m_w;
    double resAD = m_s * recip;
    double resBD = m_t * recip;
    oowlog = new_log2(recip, 56);
    sow = resAD;
    tow = resBD;
  }

private:
  s64 m_s, m_t, m_w;
};
#endif
