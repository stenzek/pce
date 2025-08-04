#pragma once
#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsThread.h"
#elif defined(Y_PLATFORM_ANDROID)
#include "YBaseLib/Android/AndroidThread.h"
#elif defined(Y_PLATFORM_HTML5)
#include "YBaseLib/HTML5/HTML5Thread.h"
#elif defined(Y_PLATFORM_POSIX)
#include "YBaseLib/POSIX/POSIXThread.h"
#else
#error Unknown platform.
#endif
