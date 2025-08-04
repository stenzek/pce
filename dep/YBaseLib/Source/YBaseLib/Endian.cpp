#include "YBaseLib/Endian.h"

#define IS_HOST_ENDIAN_TYPE(x) ((x) == Y_HOST_ENDIAN_TYPE)

#if defined(Y_PLATFORM_WINDOWS)
#include <cstdlib>
#define flip16(x) _byteswap_ushort((x))
#define flip32(x) _byteswap_ulong((x))
#define flip64(x) _byteswap_uint64((x))
#else

#define flip16(x) (((x) >> 8) | (((x)&0xff) << 8))

#define flip32(x) (((x) >> 24) | ((((x) >> 16) & 0xff) << 8) | ((((x) >> 8) & 0xff) << 16) | (((x)&0xff) << 24))

#define flip64(x)                                                                                                      \
  (((x) >> 56) | ((((x) >> 48) & 0xffULL) << 8) | ((((x) >> 40) & 0xffULL) << 16) | ((((x) >> 32) & 0xffULL) << 32) |  \
   ((((x) >> 16) & 0xffULL) << 40) | ((((x) >> 8) & 0xffULL) << 48) | (((x)&0xffULL) << 56))

#endif

int16 Endian_ConvertInt16(uint8 FromType, uint8 ToType, int16 Value)
{
  return (FromType == ToType) ? Value : flip16(Value);
}

int32 Endian_ConvertInt32(uint8 FromType, uint8 ToType, int32 Value)
{
  return (FromType == ToType) ? Value : flip32(Value);
}

int64 Endian_ConvertInt64(uint8 FromType, uint8 ToType, int64 Value)
{
  return (FromType == ToType) ? Value : flip64(Value);
}

uint16 Endian_ConvertUInt16(uint8 FromType, uint8 ToType, uint16 Value)
{
  return (FromType == ToType) ? Value : flip16(Value);
}

uint32 Endian_ConvertUInt32(uint8 FromType, uint8 ToType, uint32 Value)
{
  return (FromType == ToType) ? Value : flip32(Value);
}

uint64 Endian_ConvertUInt64(uint8 FromType, uint8 ToType, uint64 Value)
{
  return (FromType == ToType) ? Value : flip64(Value);
}

float Endian_ConvertFloat(uint8 FromType, uint8 ToType, float Value)
{
  // bit of "pointer magic" needed here
  uint32 srcTmp = *(uint32*)&Value;
  uint32 tmp = flip32(srcTmp);
  return *(float*)&tmp;
}

double Endian_ConvertDouble(uint8 FromType, uint8 ToType, double Value)
{
  // bit of "pointer magic" needed here
  uint64 srcTmp = *(uint64*)&Value;
  uint64 tmp = flip64(srcTmp);
  return *(double*)&tmp;
}

uint16 Y_byteswap_uint16(uint16 uValue)
{
  return flip16(uValue);
}

uint32 Y_byteswap_uint32(uint32 uValue)
{
  return flip32(uValue);
}

uint64 Y_byteswap_uint64(uint64 uValue)
{
  return flip64(uValue);
}

void Y_byteswap_uint16(uint16* uValue)
{
  *uValue = flip16(*uValue);
}

void Y_byteswap_uint32(uint32* uValue)
{
  *uValue = flip32(*uValue);
}

void Y_byteswap_uint64(uint64* uValue)
{
  *uValue = flip64(*uValue);
}
