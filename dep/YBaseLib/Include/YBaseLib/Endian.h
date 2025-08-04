#pragma once

#include "YBaseLib/Common.h"

enum ENDIAN_TYPE
{
  ENDIAN_TYPE_LITTLE,
  ENDIAN_TYPE_BIG,
};

// TODO FIXME
#define Y_HOST_ENDIAN_TYPE ENDIAN_TYPE_LITTLE

int16 Endian_ConvertInt16(uint8 FromType, uint8 ToType, int16 Value);
int32 Endian_ConvertInt32(uint8 FromType, uint8 ToType, int32 Value);
int64 Endian_ConvertInt64(uint8 FromType, uint8 ToType, int64 Value);
uint16 Endian_ConvertUInt16(uint8 FromType, uint8 ToType, uint16 Value);
uint32 Endian_ConvertUInt32(uint8 FromType, uint8 ToType, uint32 Value);
uint64 Endian_ConvertUInt64(uint8 FromType, uint8 ToType, uint64 Value);
float Endian_ConvertFloat(uint8 FromType, uint8 ToType, float Value);
double Endian_ConvertDouble(uint8 FromType, uint8 ToType, double Value);

uint16 Y_byteswap_uint16(uint16 uValue);
uint32 Y_byteswap_uint32(uint32 uValue);
uint64 Y_byteswap_uint64(uint64 uValue);
void Y_byteswap_uint16(uint16* uValue);
void Y_byteswap_uint32(uint32* uValue);
void Y_byteswap_uint64(uint64* uValue);