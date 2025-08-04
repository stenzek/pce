#pragma once
#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsConditionVariable.h"
#elif defined(Y_PLATFORM_ANDROID)
#include "YBaseLib/Android/AndroidConditionVariable.h"
#elif defined(Y_PLATFORM_HTML5)
#include "YBaseLib/HTML5/HTML5ConditionVariable.h"
#elif defined(Y_PLATFORM_POSIX)
#include "YBaseLib/POSIX/POSIXConditionVariable.h"
#else
#error Unknown platform.
#endif
