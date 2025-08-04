#pragma once
#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsSemaphore.h"
#elif defined(Y_PLATFORM_ANDROID)
#include "YBaseLib/Android/AndroidSemaphore.h"
#elif defined(Y_PLATFORM_HTML5)
#include "YBaseLib/HTML5/HTML5Semaphore.h"
#elif defined(Y_PLATFORM_POSIX)
#include "YBaseLib/POSIX/POSIXSemaphore.h"
#else
#error Unknown platform.
#endif
