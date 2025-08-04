#pragma once
#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsMutex.h"
#elif defined(Y_PLATFORM_ANDROID)
#include "YBaseLib/Android/AndroidMutex.h"
#elif defined(Y_PLATFORM_HTML5)
#include "YBaseLib/HTML5/HTML5Mutex.h"
#elif defined(Y_PLATFORM_POSIX)
#include "YBaseLib/POSIX/POSIXMutex.h"
#else
#error Unknown platform.
#endif
