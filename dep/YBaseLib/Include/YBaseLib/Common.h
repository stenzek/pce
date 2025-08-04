#pragma once

// Compiler detection
#if defined(_MSC_VER)
#define Y_COMPILER_MSVC 1
#elif defined(EMSCRIPTEN) || defined(__EMSCRIPTEN__)
#define Y_COMPILER_EMSCRIPTEN 1
#elif defined(__clang__)
#define Y_COMPILER_CLANG 1
#elif defined(__GNUC__)
#define Y_COMPILER_GCC 1
#else
#error Could not detect compiler.
#endif

// disable warnings that show up at warning level 4
#ifdef Y_COMPILER_MSVC
#pragma warning(disable : 4201) // warning C4201: nonstandard extension used : nameless struct/union
#pragma warning(disable : 4100) // warning C4100: 'Platform' : unreferenced formal parameter
#pragma warning(disable : 4355) // warning C4355: 'this' : used in base member initializer list
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS                                                                                        \
  1 // warning C4996: 'vsnprintf': This function or variable may be unsafe. Consider using vsnprintf_s instead. To
    // disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE                                                                                      \
  1 // warning C4996: 'stricmp': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name:
    // _stricmp. See online help for details.
#endif
#endif

// The one file we use from the CRT for all other includes.
#include <climits>
#include <cstdarg>
#include <cstddef>
#include <cstdlib>
#include <ctime>

// Standard Template Library includes, only pull in a minimal set of utility methods.
#include <algorithm>
#include <functional>
#include <utility>

// OS Detection
#if defined(EMSCRIPTEN) || defined(__EMSCRIPTEN__)
#define Y_PLATFORM_HTML5 1
#define Y_PLATFORM_STR "HTML5"
#elif defined(ANDROID) || defined(__ANDROID__)
#define Y_PLATFORM_ANDROID 1
#define Y_PLATFORM_STR "Android"
#elif defined(WIN32) || defined(_WIN32)
#define Y_PLATFORM_WINDOWS 1
#define Y_PLATFORM_STR "Windows"
#elif defined(__linux__)
#define Y_PLATFORM_POSIX 1
#define Y_PLATFORM_LINUX 1
#define Y_PLATFORM_STR "Linux"
#elif defined(__APPLE__)
#define Y_PLATFORM_POSIX 1
#define Y_PLATFORM_OSX 1
#define Y_PLATFORM_STR "OSX"
#else
#error Could not detect OS.
#endif

// CPU Detection
#if defined(Y_COMPILER_MSVC)
#ifdef _M_X64
#define Y_CPU_X64 1
#define Y_CPU_STR "x64"
//#define Y_CPU_SSE_LEVEL 2
#define Y_CPU_SSE_LEVEL 0
#else
#define Y_CPU_X86 1
#define Y_CPU_STR "x86"
/*#if _M_IX86_FP >= 2
    #define Y_CPU_SSE_LEVEL 2
#elif _M_IX86_FP == 1
    #define Y_CPU_SSE_LEVEL 1
#else
    #define Y_CPU_SSE_LEVEL 0
#endif*/
#define Y_CPU_SSE_LEVEL 0
#endif
#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG)
#if defined(__x86_64__)
#define Y_CPU_X64 1
#define Y_CPU_STR "x64"
//#define Y_CPU_SSE_LEVEL 2
#define Y_CPU_SSE_LEVEL 0
#elif defined(__i386__)
#define Y_CPU_X86 1
#define Y_CPU_STR "x86"
#define Y_CPU_SSE_LEVEL 0
#elif defined(__aarch64__)
#define Y_CPU_AARCH64 1
#define Y_CPU_STR "AArch64"
#define Y_CPU_SSE_LEVEL 0
#elif defined(__arm__)
#define Y_CPU_ARM 1
#define Y_CPU_STR "ARM"
#define Y_CPU_SSE_LEVEL 0
#else
#error Could not detect CPU.
#endif
#elif defined(Y_COMPILER_EMSCRIPTEN)
#define Y_CPU_STR "JS"
#define Y_CPU_SSE_LEVEL 0
#else
#error Could not detect CPU.
#endif

// CPU Features
#if defined(Y_CPU_X86) || defined(Y_CPU_X64)
#if Y_CPU_SSE_LEVEL > 0
#define Y_CPU_FEATURES_STR "+SSE"
#define Y_SSE_ALIGNMENT 16
#else
#define Y_CPU_FEATURES_STR ""
#endif
#else
#define Y_CPU_FEATURES_STR ""
#endif

// Config type
#if defined(_DEBUGFAST)
#define Y_BUILD_CONFIG_DEBUG 1
#define Y_BUILD_CONFIG_DEBUGFAST 1
#define Y_BUILD_CONFIG_STR "DebugFast"
#elif defined(_DEBUG)
#define Y_BUILD_CONFIG_DEBUG 1
#define Y_BUILD_CONFIG_STR "Debug"
#elif defined(_SHIPPING)
#define Y_BUILD_CONFIG_RELEASE 1
#define Y_BUILD_CONFIG_SHIPPING 1
#define Y_BUILD_CONFIG_STR "Shipping"
#else
#define Y_BUILD_CONFIG_RELEASE 1
#define Y_BUILD_CONFIG_STR "Release"
#endif

// Include pthread header on unix platforms.
#if defined(Y_PLATFORM_POSIX)
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#endif

// Pull in emscripten.h everywhere
#if defined(Y_PLATFORM_HTML5)
#include <emscripten.h>
#endif

// Pull in config header if there is one
#if defined(HAVE_MSVC_CONFIG_H)
#include "config-msvc.h"
#elif defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG) || defined(Y_COMPILER_EMSCRIPTEN)

// Provide the MemoryBarrier intrinsic
//#define MemoryBarrier() asm volatile("" ::: "memory")
#define MemoryBarrier() __sync_synchronize()

#endif

// Deprecated Macro
#ifdef Y_COMPILER_MSVC
#define DEPRECATED_DECL(Msg) __declspec(deprecated(Msg))
#else
#define DEPRECATED_DECL(Msg)
#endif

// Align Macro
#if defined(Y_COMPILER_MSVC)
#define ALIGN_DECL(__x) __declspec(align(__x))
#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG)
#define ALIGN_DECL(__x) __attribute__((aligned(__x)))
#elif defined(Y_COMPILER_EMSCRIPTEN)
#define ALIGN_DECL(__x)
#else
#error Invalid compiler
#endif

// Thread local declaration
#if defined(Y_COMPILER_MSVC)
#define Y_DECLARE_THREAD_LOCAL(decl) static __declspec(thread) decl
#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG)
#define Y_DECLARE_THREAD_LOCAL(decl) static __thread decl
#elif defined(Y_COMPILER_EMSCRIPTEN)
#define Y_DECLARE_THREAD_LOCAL(decl) static decl
#else
#error Invalid compiler
#endif

#ifdef Y_COMPILER_MSVC

#define MULTI_STATEMENT_MACRO_BEGIN                                                                                    \
  do                                                                                                                   \
  {

#define MULTI_STATEMENT_MACRO_END                                                                                      \
  __pragma(warning(push)) __pragma(warning(disable : 4127))                                                            \
  }                                                                                                                    \
  while (0)                                                                                                            \
  __pragma(warning(pop))

#else

#define MULTI_STATEMENT_MACRO_BEGIN                                                                                    \
  do                                                                                                                   \
  {

#define MULTI_STATEMENT_MACRO_END                                                                                      \
  }                                                                                                                    \
  while (0)

#endif

// compile assert macro
#define YStaticCheckSizeOf(type, expectedSize)                                                                         \
  static_assert(sizeof(type) == (expectedSize), "sizeof " #type " == " #expectedSize)
#define YStaticAssertMsg(condition, assertion_name) static_assert((condition), assertion_name)
#define YStaticAssert(condition) static_assert((condition), #condition)

// countof macro
#ifndef countof
#ifdef _countof
#define countof _countof
#else
template<typename T, size_t N>
char (&__countof_ArraySizeHelper(T (&array)[N]))[N];
#define countof(array) (sizeof(__countof_ArraySizeHelper(array)))
#endif
#endif

// offsetof macro
#ifndef offsetof
#define offsetof(st, m) ((size_t)((char*)&((st*)(0))->m - (char*)0))
#endif

// alignment macro
#define ALIGNED_SIZE(size, alignment) ((size + (decltype(size))((alignment)-1)) & ~((decltype(size))((alignment)-1)))

// containing structure address, in windows terms CONTAINING_RECORD. have to use C cast because otherwise const will
// break it.
#define CONTAINING_STRUCTURE(address, structure, field) ((structure*)(((byte*)(address)) - offsetof(structure, field)))

// stringify macro
#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif

// unreferenced parameter macro
#ifndef UNREFERENCED_PARAMETER
#if defined(Y_COMPILER_MSVC)
#define UNREFERENCED_PARAMETER(P) (P)
#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG) || defined(Y_COMPILER_EMSCRIPTEN)
#define UNREFERENCED_PARAMETER(P) (void)(P)
#else
#define UNREFERENCED_PARAMETER(P) (P)
#endif
#endif

// alloca on gcc requires alloca.h
#if defined(Y_PLATFORM_WINDOWS)
#include <malloc.h>
#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG) || defined(Y_COMPILER_EMSCRIPTEN)
#include <alloca.h>
#endif

// base types
#if defined(Y_PLATFORM_WINDOWS)
typedef unsigned __int8 byte;
typedef signed __int8 int8;
typedef signed __int16 int16;
typedef signed __int32 int32;
typedef signed __int64 int64;
typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG) || defined(Y_COMPILER_EMSCRIPTEN)
#include <stdint.h>
typedef uint8_t byte;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
#endif

// VC++ doesn't define ssize_t, so define it here.
#if defined(Y_PLATFORM_WINDOWS)
#ifdef _WIN64
typedef signed __int64 ssize_t;
#else
typedef signed int ssize_t;
#endif
#endif

// allow usage of std::move/forward without the namespace
using std::forward;
using std::move;

// templated helpers
template<typename T>
static inline T Min(T a, T b)
{
  return (a < b) ? a : b;
}
template<typename T>
static inline T Max(T a, T b)
{
  return (a > b) ? a : b;
}
template<typename T>
static inline void Swap(T& rt1, T& rt2)
{
  T tmp(move(rt1));
  rt1 = rt2;
  rt2 = tmp;
}

// helper to make a class non-copyable
#if 1 // C++11
#define DeclareNonCopyable(ClassType)                                                                                  \
  \
private:                                                                                                               \
  ClassType(const ClassType&) = delete;                                                                                \
  ClassType& operator=(const ClassType&) = delete;

#define DeclareFastCopyable(ClassType)                                                                                 \
  \
public:                                                                                                                \
  ClassType(const ClassType& _copy_) { std::memcpy(this, &_copy_, sizeof(*this)); }                                    \
  ClassType(const ClassType&& _copy_) { std::memcpy(this, &_copy_, sizeof(*this)); }                                   \
  ClassType& operator=(const ClassType& _copy_) { std::memcpy(this, &_copy_, sizeof(*this)); }                         \
  ClassType& operator=(const ClassType&& _copy_) { std::memcpy(this, &_copy_, sizeof(*this)); }

#else
#define DeclareNonCopyable(ClassType)                                                                                  \
  \
private:                                                                                                               \
  ClassType(const ClassType&);                                                                                         \
  ClassType& operator=(const ClassType&);

#define DeclareFastCopyable(ClassType)                                                                                 \
  \
public:                                                                                                                \
  ClassType(const ClassType& _copy_) { std::memcpy(this, &_copy_, sizeof(*this)); }                                    \
  ClassType& operator=(const ClassType& _copy_) { std::memcpy(this, &_copy_, sizeof(*this)); }

#endif

namespace Common {
template<typename T>
bool IsAligned(T value, unsigned int alignment)
{
  return (value % static_cast<T>(alignment)) == 0;
}
template<typename T>
T AlignUp(T value, unsigned int alignment)
{
  return (value + static_cast<T>(alignment - 1)) / static_cast<T>(alignment) * static_cast<T>(alignment);
}
template<typename T>
T AlignDown(T value, unsigned int alignment)
{
  return value / static_cast<T>(alignment) * static_cast<T>(alignment);
}
template<typename T>
bool IsAlignedPow2(T value, unsigned int alignment)
{
  return (value & static_cast<T>(alignment - 1)) == 0;
}
template<typename T>
T AlignUpPow2(T value, unsigned int alignment)
{
  return (value + static_cast<T>(alignment - 1)) & static_cast<T>(~static_cast<T>(alignment - 1));
}
template<typename T>
T AlignDownPow2(T value, unsigned int alignment)
{
  return value & static_cast<T>(~static_cast<T>(alignment - 1));
}
template<typename T>
bool IsPow2(T value)
{
  return (value & (value - 1)) == 0;
}
} // namespace Common
