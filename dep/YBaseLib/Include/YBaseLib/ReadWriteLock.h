#pragma once
#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsReadWriteLock.h"
#elif defined(Y_PLATFORM_ANDROID)
#include "YBaseLib/Android/AndroidReadWriteLock.h"
#elif defined(Y_PLATFORM_HTML5)
#include "YBaseLib/HTML5/HTML5ReadWriteLock.h"
#elif defined(Y_PLATFORM_POSIX)
#include "YBaseLib/POSIX/POSIXReadWriteLock.h"
#else
#error Unknown platform.
#endif
