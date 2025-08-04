#include "YBaseLib/Memory.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/String.h"
#include <cstdlib>
#include <cstring>

#if defined(Y_PLATFORM_WINDOWS)
#include <intrin.h>
#endif

#if defined(Y_PLATFORM_LINUX)
#include <malloc.h>
#endif

static void MemoryAllocationOverflowHandler(size_t num, size_t size)
{
  char msg[128];
  Y_snprintf(msg, countof(msg), "heap overflow when allocating %u of %u elements", (uint32)num, (uint32)size);
  Y_OnPanicReached(msg, __FUNCTION__, __FILE__, __LINE__);
}

// allocation overflow helper
#define REQUEST_SIZE_OVERFLOW_CHECK(num, size)                                                                         \
  MULTI_STATEMENT_MACRO_BEGIN                                                                                          \
  if (size > 0 && num > SIZE_MAX / size)                                                                               \
    MemoryAllocationOverflowHandler(num, size);                                                                        \
  MULTI_STATEMENT_MACRO_END

void Y_memzero(void* pDestination, size_t ByteCount)
{
  memset(pDestination, 0, ByteCount);
}

void Y_memcpy_stride(void* pDestination, size_t DestinationStride, const void* pSource, size_t SourceStride,
                     size_t Size, size_t Count)
{
  // Optimize me!
  const byte* pByteSource = (const byte*)pSource;
  byte* pByteDestination = (byte*)pDestination;

  for (size_t i = 0; i < Count; i++)
  {
    std::memcpy(pByteDestination, pByteSource, Size);
    pByteSource += SourceStride;
    pByteDestination += DestinationStride;
  }
}

int32 Y_memcmp_stride(const void* pMem1, size_t mem1Stride, const void* pMem2, size_t mem2Stride, size_t elementSize,
                      size_t count)
{
  // Optimize me!
  const byte* pByteMem1 = (const byte*)pMem1;
  const byte* pByteMem2 = (const byte*)pMem2;

  for (size_t i = 0; i < count; i++)
  {
    int res = memcmp(pByteMem1, pByteMem2, elementSize);
    if (res != 0)
      return res;

    pByteMem1 += mem1Stride;
    pByteMem2 += mem2Stride;
  }

  return 0;
}

void* Y_malloczero(size_t size)
{
  void* ptr = std::malloc(size);
  if (ptr != nullptr)
    Y_memzero(ptr, size);
  return ptr;
}

void* Y_mallocarray(size_t num, size_t size)
{
  REQUEST_SIZE_OVERFLOW_CHECK(num, size);
  return std::malloc(num * size);
}

void* Y_reallocarray(void* ptr, size_t num, size_t size)
{
  REQUEST_SIZE_OVERFLOW_CHECK(num, size);
  return std::realloc(ptr, num * size);
}

#if defined(Y_PLATFORM_WINDOWS)

void* Y_aligned_malloc(size_t size, size_t alignment)
{
  return _aligned_malloc(size, alignment);
}

void* Y_aligned_malloczero(size_t size, size_t alignment)
{
  void* ptr = _aligned_malloc(size, alignment);
  if (ptr != nullptr)
    Y_memzero(ptr, size);
  return ptr;
}

void* Y_aligned_mallocarray(size_t num, size_t size, size_t alignment)
{
  REQUEST_SIZE_OVERFLOW_CHECK(num, size);
  return Y_aligned_malloc(num * size, alignment);
}

void* Y_aligned_realloc(void* ptr, size_t size, size_t alignment)
{
  return _aligned_realloc(ptr, size, alignment);
}

void* Y_aligned_reallocarray(void* ptr, size_t num, size_t size, size_t alignment)
{
  REQUEST_SIZE_OVERFLOW_CHECK(num, size);
  return _aligned_realloc(ptr, num * size, alignment);
}

void Y_aligned_free(void* ptr)
{
  return _aligned_free(ptr);
}

#elif defined(Y_PLATFORM_LINUX)

void* Y_aligned_malloc(size_t size, size_t alignment)
{
  return memalign(alignment, size);
}

void* Y_aligned_malloczero(size_t size, size_t alignment)
{
  void* ptr = memalign(alignment, size);
  if (ptr != nullptr)
    Y_memzero(ptr, size);
  return ptr;
}

void* Y_aligned_mallocarray(size_t num, size_t size, size_t alignment)
{
  REQUEST_SIZE_OVERFLOW_CHECK(num, size);
  return Y_aligned_malloc(num * size, alignment);
}

void* Y_aligned_realloc(void* ptr, size_t size, size_t alignment)
{
  void* new_ptr = memalign(size, alignment);
  if (new_ptr != nullptr && ptr != nullptr)
  {
    size_t copy_size = malloc_usable_size(ptr);
    if (copy_size > 0)
      std::memcpy(new_ptr, ptr, copy_size);

    std::free(ptr);
  }

  return new_ptr;
}

void* Y_aligned_reallocarray(void* ptr, size_t num, size_t size, size_t alignment)
{
  REQUEST_SIZE_OVERFLOW_CHECK(num, size);
  return Y_aligned_realloc(ptr, num * size, alignment);
}

void Y_aligned_free(void* ptr)
{
  std::free(ptr);
}

#elif defined(Y_PLATFORM_OSX)

void* Y_aligned_malloc(size_t size, size_t alignment)
{
  // OSX guarantees 16 byte alignment.
  DebugAssert(alignment <= 16);
  return std::malloc(size);
}

void* Y_aligned_malloczero(size_t size, size_t alignment)
{
  DebugAssert(alignment <= 16);
  void* ptr = Y_aligned_malloc(alignment);
  if (ptr != nullptr)
    Y_memzero(ptr, size);
  return ptr;
}

void* Y_aligned_mallocarray(size_t num, size_t size, size_t alignment)
{
  REQUEST_SIZE_OVERFLOW_CHECK(num, size);
  return Y_aligned_malloc(num * size, alignment);
}

void* Y_aligned_realloc(void* ptr, size_t size, size_t alignment)
{
  DebugAssert(alignment <= 16);
  return std::realloc(ptr, size, alignment);
}

void* Y_aligned_reallocarray(void* ptr, size_t num, size_t size, size_t alignment)
{
  REQUEST_SIZE_OVERFLOW_CHECK(num, size);
  return Y_aligned_realloc(ptr, num * size, alignment);
}

void Y_aligned_free(void* ptr)
{
  std::free(ptr);
}

#endif

void Y_qsort(void* pBase, size_t nElements, size_t ElementSize, int (*CompareFunction)(const void*, const void*))
{
  qsort(pBase, nElements, ElementSize, CompareFunction);
}

#if defined(Y_PLATFORM_WINDOWS)

void Y_qsort_r(void* pBase, size_t nElements, size_t ElementSize, void* pContext,
               int (*CompareFunction)(void*, const void*, const void*))
{
  qsort_s(pBase, nElements, ElementSize, CompareFunction, pContext);
}

#elif defined(Y_PLATFORM_OSX)

void Y_qsort_r(void* pBase, size_t nElements, size_t ElementSize, void* pContext,
               int (*CompareFunction)(void*, const void*, const void*))
{
  qsort_r(pBase, nElements, ElementSize, pContext, CompareFunction);
}

#elif defined(Y_PLATFORM_HTML5) || defined(Y_PLATFORM_ANDROID)

// have to remap it.. this really needs a better solution :/
struct RecursiveQSortData
{
  int (*CompareFunction)(void*, const void*, const void*);
  void* pRealUserData;
};

Y_DECLARE_THREAD_LOCAL(RecursiveQSortData* g_pCurrentQSortData) = nullptr;

static int Y_qsort_trampoline(const void* pLeft, const void* pRight)
{
  return g_pCurrentQSortData->CompareFunction(g_pCurrentQSortData->pRealUserData, pLeft, pRight);
}

void Y_qsort_r(void* pBase, size_t nElements, size_t ElementSize, void* pContext,
               int (*CompareFunction)(void*, const void*, const void*))
{
  RecursiveQSortData* pOldValue = g_pCurrentQSortData;

  RecursiveQSortData data;
  data.CompareFunction = CompareFunction;
  data.pRealUserData = pContext;
  g_pCurrentQSortData = &data;

  qsort(pBase, nElements, ElementSize, Y_qsort_trampoline);

  g_pCurrentQSortData = pOldValue;
}

#else

// have to remap it.. this really needs a better solution :/
struct RecursiveQSortData
{
  int (*CompareFunction)(void*, const void*, const void*);
  void* pRealUserData;
};

static int Y_qsort_r_trampoline(const void* pLeft, const void* pRight, void* pUserData)
{
  RecursiveQSortData* pData = reinterpret_cast<RecursiveQSortData*>(pUserData);
  return pData->CompareFunction(pData->pRealUserData, pLeft, pRight);
}

void Y_qsort_r(void* pBase, size_t nElements, size_t ElementSize, void* pContext,
               int (*CompareFunction)(void*, const void*, const void*))
{
  RecursiveQSortData data;
  data.CompareFunction = CompareFunction;
  data.pRealUserData = pContext;
  qsort_r(pBase, nElements, ElementSize, Y_qsort_r_trampoline, reinterpret_cast<void*>(&data));
}

#endif

void* Y_bsearch(const void* pKey, const void* pBase, size_t nElements, size_t ElementSize,
                int (*CompareFunction)(const void*, const void*))
{
  return bsearch(pKey, pBase, nElements, ElementSize, CompareFunction);
}

bool Y_bitscanforward(uint8 mask, uint8* index)
{
  // possibly use this implementation:
  // http://stackoverflow.com/questions/355967/how-to-use-msvc-intrinsics-to-get-the-equivalent-of-this-gcc-code
  if (mask & 0x0F)
  {
    // bottom half
    if (mask & 0x03)
    {
      if (mask & 0x01)
      {
        *index = 0;
        return true;
      }
      else // 0x02
      {
        *index = 1;
        return true;
      }
    }
    else
    {
      if (mask & 0x04)
      {
        *index = 2;
        return true;
      }
      else // 0x08
      {
        *index = 3;
        return true;
      }
    }
  }
  else if (mask & 0xF0)
  {
    // top half
    if (mask & 0x30)
    {
      if (mask & 0x10)
      {
        *index = 4;
        return true;
      }
      else // 0x20
      {
        *index = 5;
        return true;
      }
    }
    else
    {
      if (mask & 0x40)
      {
        *index = 6;
        return true;
      }
      else // 0x80
      {
        *index = 7;
        return true;
      }
    }
  }

  return false;
}

bool Y_bitscanforward(uint16 mask, uint16* index)
{
  uint8 temp;
  if (Y_bitscanforward((uint8)mask, &temp))
  {
    *index = temp;
    return true;
  }

  if (Y_bitscanforward((uint8)(mask >> 8), &temp))
  {
    *index = temp + 8;
    return true;
  }

  return false;
}

bool Y_bitscanreverse(uint8 mask, uint8* index)
{
  // possibly use this implementation:
  // http://stackoverflow.com/questions/355967/how-to-use-msvc-intrinsics-to-get-the-equivalent-of-this-gcc-code
  if (mask & 0xF0)
  {
    // top half
    if (mask & 0xC0)
    {
      if (mask & 0x80)
      {
        *index = 7;
        return true;
      }
      else // 0x40
      {
        *index = 6;
        return true;
      }
    }
    else
    {
      if (mask & 0x20)
      {
        *index = 5;
        return true;
      }
      else // 0x10
      {
        *index = 4;
        return true;
      }
    }
  }
  else if (mask & 0x0F)
  {
    // bottom half
    if (mask & 0x0C)
    {
      if (mask & 0x08)
      {
        *index = 3;
        return true;
      }
      else // 0x04
      {
        *index = 2;
        return true;
      }
    }
    else
    {
      if (mask & 0x02)
      {
        *index = 1;
        return true;
      }
      else // 0x01
      {
        *index = 0;
        return true;
      }
    }
  }

  return false;
}

bool Y_bitscanreverse(uint16 mask, uint16* index)
{
  uint8 temp;
  if (Y_bitscanreverse((uint8)(mask >> 8), &temp))
  {
    *index = temp + 8;
    return true;
  }

  if (Y_bitscanreverse((uint8)mask, &temp))
  {
    *index = temp;
    return true;
  }

  return false;
}

// TODO: Use __builtin_popcount
// Adapted from qemu host-utils.h
uint32 Y_popcnt(uint8 value)
{
  value = (value & 0x55) + ((value >> 1) & 0x55);
  value = (value & 0x33) + ((value >> 2) & 0x33);
  value = (value & 0x0f) + ((value >> 4) & 0x0f);
  return static_cast<uint32>(value);
}

uint32 Y_popcnt(uint16 value)
{
  value = (value & 0x5555) + ((value >> 1) & 0x5555);
  value = (value & 0x3333) + ((value >> 2) & 0x3333);
  value = (value & 0x0f0f) + ((value >> 4) & 0x0f0f);
  value = (value & 0x00ff) + ((value >> 8) & 0x00ff);
  return static_cast<uint32>(value);
}

uint32 Y_popcnt(uint32 value)
{
  value = (value & 0x55555555) + ((value >> 1) & 0x55555555);
  value = (value & 0x33333333) + ((value >> 2) & 0x33333333);
  value = (value & 0x0f0f0f0f) + ((value >> 4) & 0x0f0f0f0f);
  value = (value & 0x00ff00ff) + ((value >> 8) & 0x00ff00ff);
  value = (value & 0x0000ffff) + ((value >> 16) & 0x0000ffff);
  return static_cast<uint32>(value);
}

uint32 Y_popcnt(uint64 value)
{
  value = (value & 0x5555555555555555ULL) + ((value >> 1) & 0x5555555555555555ULL);
  value = (value & 0x3333333333333333ULL) + ((value >> 2) & 0x3333333333333333ULL);
  value = (value & 0x0f0f0f0f0f0f0f0fULL) + ((value >> 4) & 0x0f0f0f0f0f0f0f0fULL);
  value = (value & 0x00ff00ff00ff00ffULL) + ((value >> 8) & 0x00ff00ff00ff00ffULL);
  value = (value & 0x0000ffff0000ffffULL) + ((value >> 16) & 0x0000ffff0000ffffULL);
  value = (value & 0x00000000ffffffffULL) + ((value >> 32) & 0x00000000ffffffffULL);
  return static_cast<uint32>(value);
}

#ifdef Y_PLATFORM_WINDOWS

bool Y_bitscanforward(uint32 mask, uint32* index)
{
  // unsigned long is 32bit on x86+x64, so this cast is okay
  return (_BitScanForward((unsigned long*)index, mask) != 0);
}

bool Y_bitscanforward(uint64 mask, uint64* index)
{
  unsigned long temp;
#ifdef Y_CPU_X64
  if (!_BitScanForward64(&temp, mask))
    return false;
  *index = temp;
  return true;
#else
  if (_BitScanForward(&temp, (uint32)mask))
  {
    *index = temp;
    return true;
  }

  if (_BitScanForward(&temp, (uint32)(mask >> 32)))
  {
    *index = temp + 32;
    return true;
  }

  return false;
#endif
}

bool Y_bitscanreverse(uint32 mask, uint32* index)
{
  // unsigned long is 32bit on x86+x64, so this cast is okay
  return (_BitScanReverse((unsigned long*)index, mask) != 0);
}

bool Y_bitscanreverse(uint64 mask, uint64* index)
{
  unsigned long temp;
#ifdef Y_CPU_X64
  if (!_BitScanReverse64(&temp, mask))
    return false;
  *index = temp;
  return true;
#else
  if (_BitScanReverse(&temp, (uint32)(mask >> 32)))
  {
    *index = temp + 32;
    return true;
  }

  if (_BitScanReverse(&temp, (uint32)mask))
  {
    *index = temp;
    return true;
  }

  return false;
#endif
}

#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG) || defined(Y_COMPILER_EMSCRIPTEN)

bool Y_bitscanforward(uint32 mask, uint32* index)
{
  if (mask == 0)
    return false;

  *index = __builtin_ctz(mask);
  return true;
}

bool Y_bitscanforward(uint64 mask, uint64* index)
{
#ifdef Y_CPU_X64
  if (mask == 0)
    return false;

  *index = __builtin_ctz(mask);
  return true;
#else
  if ((uint32)mask != 0)
  {
    *index = __builtin_ctz((uint32)mask);
    return true;
  }
  if ((uint32)(mask >> 32) != 0)
  {
    *index = __builtin_ctz((uint32)(mask >> 32)) + 32;
    return true;
  }
  return false;
#endif
}

bool Y_bitscanreverse(uint32 mask, uint32* index)
{
  if (mask == 0)
    return false;

  *index = 31u - __builtin_clz(mask);
  return true;
}

bool Y_bitscanreverse(uint64 mask, uint64* index)
{
#ifdef Y_CPU_X64
  if (mask == 0)
    return false;

  *index = 63u - __builtin_clz(mask);
  return true;
#else
  if ((uint32)(mask >> 32) != 0)
  {
    *index = 31u - __builtin_clz((uint32)(mask >> 32)) + 32;
    return true;
  }
  if ((uint32)mask != 0)
  {
    *index = 31u - __builtin_clz((uint32)mask);
    return true;
  }
  return false;
#endif
}

#endif
