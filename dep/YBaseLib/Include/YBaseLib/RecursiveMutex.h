#pragma once
#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsRecursiveMutex.h"
#elif defined(Y_PLATFORM_ANDROID)
#include "YBaseLib/Android/AndroidRecursiveMutex.h"
#elif defined(Y_PLATFORM_HTML5)
#include "YBaseLib/HTML5/HTML5RecursiveMutex.h"
#elif defined(Y_PLATFORM_POSIX)
#include "YBaseLib/POSIX/POSIXRecursiveMutex.h"
#else
#error Unknown platform.
#endif
