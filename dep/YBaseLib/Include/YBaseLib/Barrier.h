#pragma once
#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsBarrier.h"
#elif defined(Y_PLATFORM_ANDROID)
#include "YBaseLib/Android/AndroidBarrier.h"
#elif defined(Y_PLATFORM_HTML5)
#include "YBaseLib/HTML5/HTML5Barrier.h"
#elif defined(Y_PLATFORM_POSIX)
#include "YBaseLib/POSIX/POSIXBarrier.h"
#else
#error Unknown platform.
#endif
