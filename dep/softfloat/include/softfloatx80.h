/*============================================================================
This source file is an extension to the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 2b, written for Bochs (x86 achitecture simulator)
floating point emulation.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
EFFECTIVELY INDEMNIFY JOHN HAUSER AND THE INTERNATIONAL COMPUTER SCIENCE
INSTITUTE (possibly via similar legal warning) AGAINST ALL LOSSES, COSTS, OR
OTHER PROBLEMS INCURRED BY THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) the source code for the derivative work includes prominent notice that
the work is derivative, and (2) the source code includes prominent notice with
these four paragraphs for those parts of this code that are retained.
=============================================================================*/

/*============================================================================
 * Written for Bochs (x86 achitecture simulator) by
 *            Stanislav Shwartsman [sshwarts at sourceforge net]
 * ==========================================================================*/

#ifndef _SOFTFLOATX80_EXTENSIONS_H_
#define _SOFTFLOATX80_EXTENSIONS_H_

#include "softfloat.h"
#include "softfloat-specialize.h"

/*----------------------------------------------------------------------------
| Software IEC/IEEE integer-to-floating-point conversion routines.
*----------------------------------------------------------------------------*/

int16_t floatx80_to_int16(floatx80, float_status_t &status);
int16_t floatx80_to_int16_round_to_zero(floatx80, float_status_t &status);

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision operations.
*----------------------------------------------------------------------------*/

float_class_t floatx80_class(floatx80);
floatx80 floatx80_extract(floatx80 &a, float_status_t &status);
floatx80 floatx80_scale(floatx80 a, floatx80 b, float_status_t &status);
int floatx80_remainder(floatx80 a, floatx80 b, floatx80 &r, uint64_t &q, float_status_t &status);
int floatx80_ieee754_remainder(floatx80 a, floatx80 b, floatx80 &r, uint64_t &q, float_status_t &status);
floatx80 f2xm1(floatx80 a, float_status_t &status);
floatx80 fyl2x(floatx80 a, floatx80 b, float_status_t &status);
floatx80 fyl2xp1(floatx80 a, floatx80 b, float_status_t &status);
floatx80 fpatan(floatx80 a, floatx80 b, float_status_t &status);

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision trigonometric functions.
*----------------------------------------------------------------------------*/

int fsincos(floatx80 a, floatx80 *sin_a, floatx80 *cos_a, float_status_t &status);
int fsin(floatx80 &a, float_status_t &status);
int fcos(floatx80 &a, float_status_t &status);
int ftan(floatx80 &a, float_status_t &status);

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision compare.
*----------------------------------------------------------------------------*/

int floatx80_compare(floatx80, floatx80, int quiet, float_status_t &status);

inline int floatx80_compare(floatx80 a, floatx80 b, float_status_t &status)
{
    return floatx80_compare(a, b, 0, status);
}

inline int floatx80_compare_quiet(floatx80 a, floatx80 b, float_status_t &status)
{
    return floatx80_compare(a, b, 1, status);
}

/*-----------------------------------------------------------------------------
| Calculates the absolute value of the extended double-precision floating-point
| value `a'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

constexpr floatx80 floatx80_abs(const floatx80 &reg)
{
    return { reg.fraction, uint16_t(reg.exp & UINT16_C(0x7FFF)) };
}

/*-----------------------------------------------------------------------------
| Changes the sign of the extended double-precision floating-point value 'a'.
| The operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

constexpr floatx80 floatx80_chs(const floatx80 &reg)
{
    return { reg.fraction, uint16_t(reg.exp ^ UINT16_C(0x8000)) };
}

/*-----------------------------------------------------------------------------
| Commonly used extended double-precision floating-point constants.
*----------------------------------------------------------------------------*/

extern const floatx80 Const_QNaN;
extern const floatx80 Const_Z;
extern const floatx80 Const_1;
extern const floatx80 Const_L2T;
extern const floatx80 Const_L2E;
extern const floatx80 Const_PI;
extern const floatx80 Const_LG2;
extern const floatx80 Const_LN2;
extern const floatx80 Const_INF;

constexpr floatx80 floatx80_round_const(const floatx80& a, int adj)
{
  return { a.fraction + adj, a.exp };
}

#endif
