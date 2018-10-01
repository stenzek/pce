#pragma once
#include "voodoo_types.h"

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

/*************************************************************************

    3dfx Voodoo Graphics SST-1/2 emulator

    emulator by Aaron Giles

**************************************************************************/
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
#define status (0x000 / 4)   /* R  P  */
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

/*************************************
 *
 *  Alias map of the first 64
 *  registers when remapped
 *
 *************************************/

static const u8 register_alias_map[0x40] = {
  status,      0x004 / 4, vertexAx,  vertexAy,  vertexBx,  vertexBy,  vertexCx,  vertexCy,  startR,  dRdX,    dRdY,
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
