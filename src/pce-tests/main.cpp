#include "YBaseLib/Log.h"
#include <cstdio>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
#ifdef Y_BUILD_CONFIG_RELEASE
  LOGLEVEL filter_level = LOGLEVEL_INFO;
#else
  LOGLEVEL filter_level = LOGLEVEL_TRACE;
#endif
  Log::GetInstance().SetFilterLevel(filter_level);
  Log::GetInstance().SetConsoleOutputParams(true, nullptr, filter_level);
  // Log::GetInstance().SetDebugOutputParams(true, nullptr, filter_level);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
