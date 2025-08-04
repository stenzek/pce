#include "YBaseLib/NumericLimits.h"

// for now just use the CRT for these
#include <cmath>
#include <cstdlib>
#include <limits>
// const float mlFloatEpsilon = std::numeric_limits<float>::epsilon();
// const float mlFloatInfinite = std::numeric_limits<float>::infinity();
// const mlFloatUnion mlFloatEpsilon = { 0x34000000 };
// const mlFloatUnion mlFloatInfinite = { 0x7f800000 };
// const static uint32 mlFloatEpsilonUInt32 = 0x34000000;
// const static uint32 mlFloatInfiniteUInt32 = 0x7f800000;
/// const float mlFloatEpsilon = *(float *)&mlFloatEpsilonUInt32;
// const float mlFloatInfinite = *(float *)&mlFloatInfiniteUInt32;
// const float mlFloatEpsilon = (1.0f / 0.0f);
// const float mlFloatInfinite = 1.1920929e-007f;

float Y_finf()
{
  return std::numeric_limits<float>::infinity();
}

float Y_fepsilon()
{
  return std::numeric_limits<float>::epsilon();
}

float Y_fmin()
{
  return std::numeric_limits<float>::min();
}

float Y_fmax()
{
  return std::numeric_limits<float>::max();
}
