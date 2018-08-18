#include "YBaseLib/Log.h"
#include <cstdio>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
  // LOGLEVEL filter_level = LOGLEVEL_PROFILE;
  LOGLEVEL filter_level = LOGLEVEL_INFO;
  // LOGLEVEL filter_level = LOGLEVEL_ERROR;
  Log::GetInstance().SetFilterLevel(filter_level);
  Log::GetInstance().SetConsoleOutputParams(true, nullptr, filter_level);
  // Log::GetInstance().SetDebugOutputParams(true, nullptr, filter_level);

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
