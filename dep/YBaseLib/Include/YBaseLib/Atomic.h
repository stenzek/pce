#pragma once

#include "YBaseLib/Common.h"

// declaration macro
#define Y_ATOMIC_DECL ALIGN_DECL(4) volatile
#define Y_ATOMIC_DECL64 ALIGN_DECL(8) volatile
#ifdef Y_CPU_X64
#define Y_ATOMIC_PTR_DECL(type) ALIGN_DECL(8) type* volatile
#else
#define Y_ATOMIC_PTR_DECL(type) ALIGN_DECL(4) type* volatile
#endif

// atomic functions
template<typename T>
T Y_AtomicIncrement(volatile T& Value);
template<typename T>
T Y_AtomicDecrement(volatile T& Value);
template<typename T>
T Y_AtomicCompareExchange(volatile T& Result, T Exchange, T Comparand);
template<typename T>
T Y_AtomicExchange(volatile T& Result, T Exchange);
template<typename T>
T Y_AtomicAnd(volatile T& Value, T AndVal);
template<typename T>
T Y_AtomicAdd(volatile T& Value, T AddVal);

// for pointer types
void* Y_AtomicCompareExchangeVoidPointer(void* volatile& Pointer, void* Exchange, void* Comparand);
void* Y_AtomicExchangeVoidPointer(void* volatile& Pointer, void* Exchange);
template<typename T>
T Y_AtomicCompareExchangePointer(T volatile& Pointer, T Exchange, T Comparand)
{
  return reinterpret_cast<T>(Y_AtomicCompareExchangeVoidPointer(
    reinterpret_cast<void* volatile&>(Pointer), reinterpret_cast<void*>(Exchange), reinterpret_cast<void*>(Comparand)));
}

template<typename T>
T Y_AtomicExchangePointer(T volatile& Pointer, T Exchange)
{
  return reinterpret_cast<T>(
    Y_AtomicExchangeVoidPointer(reinterpret_cast<void* volatile&>(Pointer), reinterpret_cast<void*>(Exchange)));
}

// atomic pointer class
template<class T>
class AtomicPointer
{
public:
  inline AtomicPointer() : m_ptr(nullptr) {}
  inline AtomicPointer(T* ptr) : m_ptr(ptr) {}

  inline T* Load() const { return m_ptr; }

  inline bool Set(T* ptr) { return (Y_AtomicCompareExchangePointer<T*>(m_ptr, ptr, nullptr) == nullptr); }
  inline bool Clear(T* ptr) { return (Y_AtomicCompareExchangePointer<T*>(m_ptr, nullptr, ptr) == nullptr); }

  inline const T& operator*() const { return *Load(); }
  inline T& operator*() { return *Load(); }
  inline const T* operator->() const { return Load(); }
  inline T* operator->() { return Load(); }
  inline operator const T*() const { return Load(); }
  inline operator T*() { return Load(); }

  inline bool operator==(const AtomicPointer<T>& Other) const { return (Load() == Other.Load()); }
  inline bool operator!=(const AtomicPointer<T>& Other) const { return (Load() != Other.Load()); }
  inline bool operator==(const T* pPtr) const { return (Load() == Load()); }
  inline bool operator!=(const T* pPtr) const { return (Load() != Load()); }

  // remove assignment operators
  DeclareNonCopyable(AtomicPointer);

private:
  Y_ATOMIC_PTR_DECL(T) m_ptr;
};
