#pragma once
#include "YBaseLib/Common.h"

#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsEvent.h"
#elif defined(Y_PLATFORM_ANDROID)
#include "YBaseLib/Android/AndroidEvent.h"
#elif defined(Y_PLATFORM_HTML5)
#include "YBaseLib/HTML5/HTML5Event.h"
#elif defined(Y_PLATFORM_POSIX)
#include "YBaseLib/POSIX/POSIXEvent.h"
#else
#error Unknown platform.
#endif
