#include "YBaseLib/Math.h"

// for now just use the CRT for these
#include <cmath>
#include <cstdlib>
#include <limits>

float Y_fclamp(float v, float min, float max)
{
  return (v > max) ? max : (v < min) ? min : v;
}

float Y_fsaturate(float v)
{
  return Y_fclamp(v, 0.0f, 1.0f);
}

float Y_fsnap(float v, float step)
{
  float r = Y_fmodf(v, step);
  if (r >= (step * 0.5f))
    return v + (step - r);
  else
    return v - r;
}

bool Y_fnearequal(float f1, float f2, float fEpsilon)
{
  return Y_fabs(f1 - f2) <= fEpsilon;
}

float Y_fabs(float v)
{
  return fabs(v);
}

//////////////////////////////////////////////////////////////////////////
// template overrides
//////////////////////////////////////////////////////////////////////////

// math functions
float Y_sqrtf(float v)
{
  return sqrtf(v);
}

float Y_rsqrtf(float v)
{
  return 1.0f / sqrtf(v);
}

float Y_sinf(float v)
{
  return sinf(v);
}

float Y_cosf(float v)
{
  return cosf(v);
}

float Y_tanf(float v)
{
  return tanf(v);
}

float Y_floorf(float v)
{
  return floorf(v);
}

float Y_ceilf(float v)
{
  return ceilf(v);
}

float Y_asinf(float v)
{
  return asinf(v);
}

float Y_acosf(float v)
{
  return acosf(v);
}

void Y_sincosf(float v, float* pSinv, float* pCosv)
{
  *pSinv = Y_sinf(v);
  *pCosv = Y_cosf(v);
}

float Y_squaref(float v)
{
  return v * v;
}

bool Y_isfinitef(float v)
{
  return v != Y_finf();
}

float Y_signf(float v)
{
  return (v < 0.0f) ? -1.0f : ((v > 0.0f) ? 1.0f : 0.0f);
}

int32 Y_truncf(float v)
{
  return (int32)v;
}

float Y_roundf(float v)
{
  return Y_floorf(v + 0.5f);
}

float Y_fmodf(float x, float y)
{
  return fmodf(x, y);
}

float Y_fracf(float v)
{
  return v - Y_floorf(v);
}

void Y_splitf(float* pRealPart, float* pFractionalPart, float v)
{
  float realPart = Y_floorf(v);

  *pRealPart = realPart;
  *pFractionalPart = v - realPart;
}

float Y_powf(float v, float p)
{
  return powf(v, p);
}

bool Y_ispow2(uint32 v)
{
  return (v & (v - 1)) == 0;
}

uint32 Y_nextpow2(uint32 v)
{
  uint32 r = v;
  r--;
  r |= r >> 16;
  r |= r >> 8;
  r |= r >> 4;
  r |= r >> 2;
  r |= r >> 1;
  r++;
  return r;
}

int32 Y_nextpow2i(int32 v)
{
  int32 r = v;
  r--;
  r |= r >> 16;
  r |= r >> 8;
  r |= r >> 4;
  r |= r >> 2;
  r |= r >> 1;
  r++;
  return r;
}

int32 Y_abs(int32 v)
{
  // return (v & 0x7FFFFFFF);
  return abs((int)v);
}

int64 Y_abs64(int64 v)
{
  // return (v & 0x7FFFFFFFFFFFFFFF);
  return llabs((long long)v);
}

int32 Y_isign(int32 v)
{
  if (v == 0)
    return 0;
  else if (v > 0)
    return 1;
  else
    return -1;
}

int64 Y_isign64(int64 v)
{
  if (v == 0)
    return 0;
  else if (v > 0)
    return 1;
  else
    return -1;
}

float Y_atanf(float v)
{
  return atanf(v);
}

float Y_atan2f(float u, float v)
{
  return atan2f(u, v);
}

float Y_lerpf(float start, float end, float factor)
{
  return (end - start) * factor + start;
}

int32 Y_compareresultf(float l, float r)
{
  if (l == r)
    return 0;
  else if (l < r)
    return -1;
  else
    return 1;
}

bool Y_inrangef(float val, float min, float max)
{
  return (val >= min) && (val <= max);
}

uint16 Y_floattohalf(float v)
{
  union
  {
    float vAsFloat;
    uint32 vAsUInt;
  };
  vAsFloat = v;

  int32 e = ((vAsUInt >> 23) & 0xFF) - 112;
  int32 m = vAsUInt & 0x7FFFFF;

  uint16 retVal = (vAsUInt >> 16) & 0x8000;
  if (e <= 0)
  {
    m = ((m | 0x800000) >> (1 - e)) + 0x1000;
    retVal |= (m >> 13);
  }
  else if (e == 143)
  {
    retVal |= 0x7C00;
    if (m != 0)
    {
      // NaN
      m >>= 13;
      retVal |= (m | (m == 0));
    }
  }
  else
  {
    m += 0x1000;
    if (m & 0x800000)
    {
      // mantissa overflow
      m = 0;
      e++;
    }
    if (e >= 31)
    {
      // exponent overflow
      retVal |= 0x7C00;
    }
    else
    {
      retVal |= (e << 10) | (m >> 13);
    }
  }

  return retVal;
}

float Y_halftofloat(uint16 v)
{
  union
  {
    float resultAsFloat;
    uint32 resultAsUInt;
  };

  resultAsUInt = (v & 0x8000) << 16;
  uint32 e = (v >> 10) & 0x1F;
  uint32 m = v & 0x3FF;
  if (e == 0)
  {
    if (m == 0)
      return resultAsFloat;

    while ((m & 0x400) == 0)
    {
      m += m;
      e--;
    }

    e++;
    m &= ~0x400;
  }
  else if (e == 31)
  {
    // inf/nan
    resultAsUInt |= 0x7F800000 | (m << 13);
    return resultAsFloat;
  }

  resultAsUInt |= ((e + 112) << 23) | (m << 13);
  return resultAsFloat;
}
