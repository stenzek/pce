#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/NumericLimits.h"

//---------------------------------------------------------------------------------------------------------------------
// defines
//---------------------------------------------------------------------------------------------------------------------

// pi
#define Y_PI (3.14159265358979323846f)
// pi * 2
#define Y_DOUBLE_PI (6.283185307179586f)
// pi / 2
#define Y_HALF_PI (1.57079632679489661923f)
// pi / 4
#define Y_QUARTER_PI (0.785398163397448309616f)
// pi / 180
#define Y_PI_OVER180 (0.0174532925199433f)

//---------------------------------------------------------------------------------------------------------------------
// stdlib math functions
//---------------------------------------------------------------------------------------------------------------------
int32 Y_abs(int32 v);
int64 Y_abs64(int64 v);
int32 Y_isign(int32 v);
int64 Y_isign64(int64 v);
float Y_fclamp(float v, float min, float max);
float Y_fsaturate(float v);
float Y_fsnap(float v, float step);
bool Y_fnearequal(float f1, float f2, float fEpsilon = Y_FLT_EPSILON);
float Y_fabs(float v);
float Y_sqrtf(float v);
float Y_rsqrtf(float v);
float Y_sinf(float v);
float Y_cosf(float v);
float Y_tanf(float v);
int32 Y_truncf(float v);
float Y_floorf(float v);
float Y_ceilf(float v);
float Y_asinf(float v);
float Y_acosf(float v);
void Y_sincosf(float v, float* pSinv, float* pCosv);
float Y_squaref(float v);
bool Y_isfinitef(float v);
float Y_signf(float v);
float Y_roundf(float v);
float Y_fmodf(float x, float y);
float Y_fracf(float v);
void Y_splitf(float* pRealPart, float* pFractionalPart, float v);
float Y_powf(float v, float p);
float Y_atanf(float v);
float Y_atan2f(float u, float v);
float Y_lerpf(float start, float end, float factor);
int32 Y_compareresultf(float l, float r);
bool Y_inrangef(float val, float min, float max);

bool Y_ispow2(uint32 v);
uint32 Y_nextpow2(uint32 v);
int32 Y_nextpow2i(int32 v);

uint16 Y_floattohalf(float v);
float Y_halftofloat(uint16 v);

namespace Math {

//#define DegreesToRadians(deg) ((deg) * (Y_PI / 180.0f))
//#define RadiansToDegrees(deg) ((deg) * (180.0f / Y_PI))
static inline float DegreesToRadians(float fDegrees)
{
  return fDegrees * (Y_PI / 180.0f);
}
static inline float RadiansToDegrees(float fRadians)
{
  return fRadians * (180.0f / Y_PI);
}

static inline float NormalizeAngle(float fRadians)
{
  while (fRadians < 0)
    fRadians += Y_DOUBLE_PI;
  while (fRadians > Y_DOUBLE_PI)
    fRadians -= Y_DOUBLE_PI;

  return fRadians;
}

static inline float NormalizeAngleDegrees(float fRadians)
{
  while (fRadians < 0)
    fRadians += 360.0f;
  while (fRadians > 360.0f)
    fRadians -= 360.0f;

  return fRadians;
}

//---------------------------------------------------------------------------------------------------------------------
// templated functions
//---------------------------------------------------------------------------------------------------------------------
template<typename T>
T Epsilon();
template<typename T>
static inline T Clamp(T v, T min, T max)
{
  return (v > max) ? max : (v < min) ? min : v;
}
template<typename T>
static inline T Abs(T v)
{
  return (v < 0) ? -v : v;
}
template<typename T>
static inline T Sign(T v)
{
  return (v < 0) ? -1 : ((v == 0) ? 0 : 1);
}
template<typename T>
static inline bool NearEqual(T v1, T v2, T epsilon)
{
  return Abs<T>(v1 - v2) <= epsilon;
}
template<typename T>
static inline T Snap(T v, T snap)
{
  T temp = v % snap;
  return ((temp >= (snap / 2)) ? v + (snap - temp) : v - temp);
}
template<typename T>
static inline T Square(T v)
{
  return (v * v);
}
template<typename T>
static inline int32 CompareResult(T l, T r)
{
  return ((l < r) ? -1 : ((l > r) ? 1 : 0));
}
template<typename T>
static inline bool InRange(T val, T min, T max)
{
  return (val >= min) && (val <= max);
}

//////////////////////////////////////////////////////////////////////////
// template overrides
//////////////////////////////////////////////////////////////////////////
template<>
inline float Epsilon<float>()
{
  return Y_FLT_EPSILON;
}
template<>
inline int32 Abs<int32>(int32 v)
{
  return Y_abs(v);
}
template<>
inline int64 Abs<int64>(int64 v)
{
  return Y_abs64(v);
}
template<>
inline int32 Sign<int32>(int32 v)
{
  return Y_isign(v);
}
template<>
inline int64 Sign<int64>(int64 v)
{
  return Y_isign64(v);
}
template<>
inline float Sign<float>(float v)
{
  return Y_signf(v);
}
template<>
inline float Clamp<float>(float v, float min, float max)
{
  return Y_fclamp(v, min, max);
}
template<>
inline float Abs<float>(float v)
{
  return Y_fabs(v);
}
template<>
inline bool NearEqual<float>(float v1, float v2, float epsilon)
{
  return Y_fnearequal(v1, v2, epsilon);
}
template<>
inline float Snap<float>(float v, float snap)
{
  return Y_fsnap(v, snap);
}
template<>
inline float Square<float>(float v)
{
  return Y_squaref(v);
}
template<>
inline int32 CompareResult<float>(float l, float r)
{
  return Y_compareresultf(l, r);
}
template<>
inline bool InRange<float>(float val, float min, float max)
{
  return Y_inrangef(val, min, max);
}

//---------------------------------------------------------------------------------------------------------------------
// redirectors
//---------------------------------------------------------------------------------------------------------------------
static inline float Sqrt(float v)
{
  return Y_sqrtf(v);
}
static inline float RSqrt(float v)
{
  return Y_rsqrtf(v);
}
static inline float Sin(float v)
{
  return Y_sinf(v);
}
static inline float Cos(float v)
{
  return Y_cosf(v);
}
static inline float Tan(float v)
{
  return Y_tanf(v);
}
static inline void SinCos(float v, float* pSinv, float* pCosv)
{
  return Y_sincosf(v, pSinv, pCosv);
}
static inline float ArcTan(float v)
{
  return Y_atanf(v);
}
static inline float ArcTan(float u, float v)
{
  return Y_atan2f(u, v);
}
static inline float Floor(float v)
{
  return Y_floorf(v);
}
static inline float Ceil(float v)
{
  return Y_ceilf(v);
}
static inline float Round(float v)
{
  return Y_roundf(v);
}
static inline int32 Truncate(float v)
{
  return Y_truncf(v);
}
static inline float FractionalPart(float v)
{
  return Y_fracf(v);
}
static inline float Pow(float v, float p)
{
  return Y_powf(v, p);
}
static inline float ArcTangent(float v)
{
  return Y_atanf(v);
}
static inline float ArcTangent(float u, float v)
{
  return Y_atan2f(u, v);
}
static inline float Lerp(float start, float end, float factor)
{
  return Y_lerpf(start, end, factor);
}
static inline uint16 FloatToHalf(float v)
{
  return Y_floattohalf(v);
}
static inline float HalfToFloat(uint16 v)
{
  return Y_halftofloat(v);
}
static inline bool IsPowerOfTwo(uint32 v)
{
  return Y_ispow2(v);
}
static inline float Saturate(float v)
{
  return Y_fsaturate(v);
}

} // namespace Math
