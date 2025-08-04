#pragma once
#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsSubprocess.h"
#elif defined(Y_PLATFORM_ANDROID)
#error Unsupported on this platform.
#elif defined(Y_PLATFORM_HTML5)
#error Unsupported on this platform.
#elif defined(Y_PLATFORM_POSIX)
#include "YBaseLib/POSIX/POSIXSubprocess.h"
#else
#error Unknown platform.
#endif
