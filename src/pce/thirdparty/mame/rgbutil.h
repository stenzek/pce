// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    rgbutil.h

    Utility definitions for RGB manipulation. Allows RGB handling to be
    performed in an abstracted fashion and optimized with SIMD.

***************************************************************************/
#pragma once
#include "../../types.h"

// use SSE on 64-bit implementations, where it can be assumed
#if !defined(DEBUG) && (defined(__SSE2__) || defined(_MSC_VER))
#include "rgbsse.h"
#elif defined(__ALTIVEC__)
#include "rgbvmx.h"
#else
#include "rgbgen.h"
#endif
