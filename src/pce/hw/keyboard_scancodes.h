#pragma once

#include "pce/scancodes.h"
#include "pce/types.h"

namespace HW {

class KeyboardScanCodes
{
public:
  static constexpr size_t MAX_SCAN_CODE_LENGTH = 10;
  static const u8 Set1Mapping[NumGenScanCodes][2][MAX_SCAN_CODE_LENGTH];
  static const u8 Set2Mapping[NumGenScanCodes][2][MAX_SCAN_CODE_LENGTH];
};

} // namespace HW
