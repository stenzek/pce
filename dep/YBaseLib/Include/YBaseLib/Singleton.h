#pragma once
#include "YBaseLib/Common.h"

template<class T>
class Singleton
{
public:
  static T& GetInstance()
  {
    static T staticInstance;
    return staticInstance;
  }
};
