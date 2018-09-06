#pragma once
#include "../types.h"

struct CMOSInfo
{
  static const u32 NumFloppies = 2;
  static const u32 NumATAChannels = 2;
  static const u32 ATADevicesPerChannel = 2;

  enum class ATADeviceType
  {
    None,
    HDD,
    CDROM,
    Other
  };

  enum class FloppyType
  {
    None,
    F5_25,
    F3_5,
  };

  u32 memory_in_kb;
  FloppyType floppy_types[2];

  struct
  {
    ATADeviceType type;
    u32 cylinders;
    u32 heads;
    u32 sectors;
  } m_ata_devices[NumATAChannels][ATADevicesPerChannel];
};
