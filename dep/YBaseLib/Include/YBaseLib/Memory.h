#pragma once

#include "YBaseLib/Common.h"
#include <cstdlib>
#include <cstring>

void Y_memzero(void* pDestination, size_t ByteCount);
void Y_memcpy_stride(void* pDestination, size_t DestinationStride, const void* pSource, size_t SourceStride,
                     size_t Size, size_t Count);
int32 Y_memcmp_stride(const void* pMem1, size_t mem1Stride, const void* pMem2, size_t mem2Stride, size_t elementSize,
                      size_t count);

void* Y_malloczero(size_t size);
void* Y_mallocarray(size_t num, size_t size);
void* Y_reallocarray(void* ptr, size_t num, size_t size);

// aligned malloc variants
void* Y_aligned_malloc(size_t size, size_t alignment);
void* Y_aligned_malloczero(size_t size, size_t alignment);
void* Y_aligned_mallocarray(size_t num, size_t size, size_t alignment);
void* Y_aligned_realloc(void* ptr, size_t size, size_t alignment);
void* Y_aligned_reallocarray(void* ptr, size_t num, size_t size, size_t alignment);
void Y_aligned_free(void* ptr);

#define Y_SSE_ALIGNMENT 16

void Y_qsort(void* pBase, size_t nElements, size_t ElementSize, int (*CompareFunction)(const void*, const void*));
void Y_qsort_r(void* pBase, size_t nElements, size_t ElementSize, void* pContext,
               int (*CompareFunction)(void*, const void*, const void*));
void* Y_bsearch(const void* pKey, const void* pBase, size_t nElements, size_t ElementSize,
                int (*CompareFunction)(const void*, const void*));

bool Y_bitscanforward(uint8 mask, uint8* index);
bool Y_bitscanforward(uint16 mask, uint16* index);
bool Y_bitscanforward(uint32 mask, uint32* index);
bool Y_bitscanforward(uint64 mask, uint64* index);
bool Y_bitscanreverse(uint8 mask, uint8* index);
bool Y_bitscanreverse(uint16 mask, uint16* index);
bool Y_bitscanreverse(uint32 mask, uint32* index);
bool Y_bitscanreverse(uint64 mask, uint64* index);
uint32 Y_popcnt(uint8 value);
uint32 Y_popcnt(uint16 value);
uint32 Y_popcnt(uint32 value);
uint32 Y_popcnt(uint64 value);

// templated copies
template<typename T>
void Y_memcpyT(T* pDestination, const T* pSource, size_t uCount)
{
  std::memcpy(pDestination, pSource, uCount * sizeof(T));
}
template<typename T>
void Y_memmoveT(T* pDestination, const T* pSource, size_t uCount)
{
  std::memmove(pDestination, pSource, uCount * sizeof(T));
}
template<typename T>
void Y_memzeroT(T* pDestination, uint32 uCount)
{
  Y_memzero(pDestination, uCount * sizeof(T));
}

// templated mallocs
template<typename T>
T* Y_mallocT(size_t nCount = 1)
{
  return (T*)std::malloc(sizeof(T) * nCount);
}
template<typename T>
T* Y_malloczeroT(size_t nCount = 1)
{
  return (T*)Y_malloczero(sizeof(T) * nCount);
}
template<typename T>
T* Y_reallocT(T* pMemory, size_t nNewCount)
{
  return (T*)std::realloc(pMemory, sizeof(T) * nNewCount);
}
template<typename T>
T* Y_aligned_mallocT(size_t nCount, size_t uAlignment)
{
  return (T*)Y_aligned_malloc(sizeof(T) * nCount, uAlignment);
}
template<typename T>
T* Y_aligned_malloczeroT(size_t nCount, size_t uAlignment)
{
  return (T*)Y_aligned_malloczero(sizeof(T) * nCount, uAlignment);
}

// other templates
template<typename T>
void Y_qsortT(T* pBase, size_t nElements, int (*CompareFunction)(const T*, const T*))
{
  Y_qsort(pBase, nElements, sizeof(T), reinterpret_cast<int (*)(const void*, const void*)>(CompareFunction));
}
template<typename K, typename T>
const T* Y_bsearchT(const K* pKey, const T* pBase, size_t nElements, int (*CompareFunction)(const K*, const T*))
{
  return reinterpret_cast<const T*>(
    Y_bsearch(pKey, pBase, nElements, sizeof(T), reinterpret_cast<int (*)(const void*, const void*)>(CompareFunction)));
}

// macro for defining an aligned class
#define DECLARE_ALIGNED_ALLOCATOR(Alignment)                                                                           \
  \
public:                                                                                                                \
  void* operator new[](size_t c) { return Y_aligned_malloc(c, Alignment); }                                            \
  void* operator new(size_t c) { return Y_aligned_malloc(c, Alignment); }                                              \
  void operator delete[](void* pMemory) { return Y_aligned_free(pMemory); }                                            \
  void operator delete(void* pMemory) { return Y_aligned_free(pMemory); }
