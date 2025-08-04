#pragma once
#include "YBaseLib/Common.h"

//---------------------------------------------------------------------------------------------------------------------
// defines
//---------------------------------------------------------------------------------------------------------------------
#define Y_INT8_MIN (-126)
#define Y_INT8_MAX (127)
#define Y_UINT8_MIN (0)
#define Y_UINT8_MAX (255)
#define Y_INT16_MIN (-32766)
#define Y_INT16_MAX (32767)
#define Y_UINT16_MIN (0U)
#define Y_UINT16_MAX (65535)
#define Y_INT32_MIN (-2147483647)
#define Y_INT32_MAX (2147483647)
#define Y_UINT32_MIN (0U)
#define Y_UINT32_MAX (4294967295U)
#define Y_INT64_MIN (-9223372036854775806)
#define Y_INT64_MAX (9223372036854775807)
#define Y_UINT64_MIN (0U)
#define Y_UINT64_MAX (0xFFFFFFFFFFFFFFFFU)

#define Y_FLT_EPSILON (Y_fepsilon())
#define Y_FLT_INFINITE (Y_finf())
#define Y_FLT_MIN (Y_fmin())
#define Y_FLT_MAX (Y_fmax())

// has to be functions
float Y_finf();
float Y_fepsilon();
float Y_fmin();
float Y_fmax();
