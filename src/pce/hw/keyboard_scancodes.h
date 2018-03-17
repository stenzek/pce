#pragma once

#include "pce/scancodes.h"
#include "pce/types.h"

namespace HW {

class KeyboardScanCodes
{
public:
  static const size_t MAX_SCAN_CODE_LENGTH = 10;
  static const uint8 Set1Mapping[NumGenScanCodes][2][MAX_SCAN_CODE_LENGTH];
  static const uint8 Set2Mapping[NumGenScanCodes][2][MAX_SCAN_CODE_LENGTH];
};

} // namespace HW
