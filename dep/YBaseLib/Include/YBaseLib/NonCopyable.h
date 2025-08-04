#pragma once
#include "YBaseLib/Common.h"

class NonCopyable
{
public:
  NonCopyable() {}

private:
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
};
