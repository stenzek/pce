#include "YBaseLib/Atomic.h"

#if defined(Y_PLATFORM_WINDOWS)

#include <intrin.h>
#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX 1
#include <windows.h>

// specialized atomic functions
// int16
template<>
int16 Y_AtomicIncrement(volatile int16& Value)
{
  return _InterlockedIncrement16(&Value);
}
template<>
int16 Y_AtomicDecrement(volatile int16& Value)
{
  return _InterlockedDecrement16(&Value);
}
template<>
int16 Y_AtomicCompareExchange(volatile int16& Result, int16 Exchange, int16 Comparand)
{
  return _InterlockedCompareExchange16(&Result, Exchange, Comparand);
}
template<>
int16 Y_AtomicExchange(volatile int16& Result, int16 Exchange)
{
#ifdef Y_COMPILER_MSVC
  return _InterlockedExchange16(&Result, Exchange);
#else
  // Appears to be missing on mingw...
  __sync_synchronize();
  int16 Old = Result;
  Result = Exchange;
  __sync_synchronize();
  return Old;
#endif
}
template<>
int16 Y_AtomicAnd(volatile int16& Value, int16 AndVal)
{
#ifdef Y_COMPILER_MSVC
  return _InterlockedAnd16((volatile short*)&Value, AndVal);
#else
  return __sync_and_and_fetch(&Value, AndVal);
#endif
}
template<>
uint16 Y_AtomicIncrement(volatile uint16& Value)
{
  return _InterlockedIncrement16((volatile short*)&Value);
}
template<>
uint16 Y_AtomicDecrement(volatile uint16& Value)
{
  return _InterlockedDecrement16((volatile short*)&Value);
}
template<>
uint16 Y_AtomicCompareExchange(volatile uint16& Result, uint16 Exchange, uint16 Comparand)
{
  return _InterlockedCompareExchange16((volatile short*)&Result, Exchange, Comparand);
}
template<>
uint16 Y_AtomicExchange(volatile uint16& Result, uint16 Exchange)
{
#ifdef Y_COMPILER_MSVC
  return _InterlockedExchange16((volatile short*)&Result, Exchange);
#else
  // Appears to be missing on mingw...
  __sync_synchronize();
  uint16 Old = Result;
  Result = Exchange;
  __sync_synchronize();
  return Old;
#endif
}
template<>
uint16 Y_AtomicAnd(volatile uint16& Value, uint16 AndVal)
{
#ifdef Y_COMPILER_MSVC
  return _InterlockedAnd16((volatile short*)&Value, AndVal);
#else
  return __sync_and_and_fetch(&Value, AndVal);
#endif
}

// int32
template<>
int32 Y_AtomicIncrement(volatile int32& Value)
{
  return _InterlockedIncrement((volatile long*)&Value);
}
template<>
int32 Y_AtomicDecrement(volatile int32& Value)
{
  return _InterlockedDecrement((volatile long*)&Value);
}
template<>
int32 Y_AtomicCompareExchange(volatile int32& Result, int32 Exchange, int32 Comparand)
{
  return _InterlockedCompareExchange((volatile long*)&Result, Exchange, Comparand);
}
template<>
int32 Y_AtomicExchange(volatile int32& Result, int32 Exchange)
{
  return _InterlockedExchange((volatile long*)&Result, Exchange);
}
template<>
int32 Y_AtomicAnd(volatile int32& Value, int32 AndVal)
{
  return _InterlockedAnd((volatile long*)&Value, AndVal);
}
template<>
int32 Y_AtomicAdd(volatile int32& Value, int32 AddVal)
{
  return InterlockedAdd((volatile long*)&Value, AddVal);
}
template<>
uint32 Y_AtomicIncrement(volatile uint32& Value)
{
  return _InterlockedIncrement((volatile long*)&Value);
}
template<>
uint32 Y_AtomicDecrement(volatile uint32& Value)
{
  return _InterlockedDecrement((volatile long*)&Value);
}
template<>
uint32 Y_AtomicCompareExchange(volatile uint32& Result, uint32 Exchange, uint32 Comparand)
{
  return _InterlockedCompareExchange((volatile long*)&Result, Exchange, Comparand);
}
template<>
uint32 Y_AtomicExchange(volatile uint32& Result, uint32 Exchange)
{
  return _InterlockedExchange((volatile long*)&Result, Exchange);
}
template<>
uint32 Y_AtomicAnd(volatile uint32& Value, uint32 AndVal)
{
  return _InterlockedAnd((volatile long*)&Value, AndVal);
}
template<>
uint32 Y_AtomicAdd(volatile uint32& Value, uint32 AddVal)
{
  return InterlockedAdd((volatile long*)&Value, AddVal);
}

// int64, double only supported on x64
#if Y_CPU_X64
template<>
int64 Y_AtomicIncrement(volatile int64& Value)
{
  return _InterlockedIncrement64((__int64*)&Value);
}
template<>
int64 Y_AtomicDecrement(volatile int64& Value)
{
  return _InterlockedDecrement64((__int64*)&Value);
}
template<>
int64 Y_AtomicCompareExchange(volatile int64& Result, int64 Exchange, int64 Comparand)
{
  return _InterlockedCompareExchange64((__int64*)&Result, Exchange, Comparand);
}
template<>
int64 Y_AtomicExchange(volatile int64& Result, int64 Exchange)
{
  return _InterlockedExchange64((__int64*)&Result, Exchange);
}
template<>
int64 Y_AtomicAnd(volatile int64& Value, int64 AndVal)
{
  return _InterlockedAnd64((__int64*)&Value, AndVal);
}
template<>
int64 Y_AtomicAdd(volatile int64& Value, int64 AddVal)
{
  return InterlockedAdd64((__int64*)&Value, AddVal);
}
template<>
uint64 Y_AtomicIncrement(volatile uint64& Value)
{
  return _InterlockedIncrement64((__int64*)&Value);
}
template<>
uint64 Y_AtomicDecrement(volatile uint64& Value)
{
  return _InterlockedDecrement64((__int64*)&Value);
}
template<>
uint64 Y_AtomicCompareExchange(volatile uint64& Result, uint64 Exchange, uint64 Comparand)
{
  return _InterlockedCompareExchange64((__int64*)&Result, Exchange, Comparand);
}
template<>
uint64 Y_AtomicExchange(volatile uint64& Result, uint64 Exchange)
{
  return _InterlockedExchange64((__int64*)&Result, Exchange);
}
template<>
uint64 Y_AtomicAnd(volatile uint64& Value, uint64 AndVal)
{
  return _InterlockedAnd64((__int64*)&Value, AndVal);
}
template<>
uint64 Y_AtomicAdd(volatile uint64& Value, uint64 AddVal)
{
  return InterlockedAdd64((__int64*)&Value, AddVal);
}
#endif

// pointers
void* Y_AtomicCompareExchangeVoidPointer(void* volatile& Pointer, void* Exchange, void* Comparand)
{
  return InterlockedCompareExchangePointer(&Pointer, Exchange, Comparand);
}
void* Y_AtomicExchangeVoidPointer(void* volatile& Pointer, void* Exchange)
{
  return InterlockedExchangePointer(&Pointer, Exchange);
}

#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG) || defined(Y_COMPILER_EMSCRIPTEN)

// specialized atomic functions
// int16
template<>
int16 Y_AtomicIncrement(volatile int16& Value)
{
  return __sync_add_and_fetch(&Value, 1);
}
template<>
int16 Y_AtomicDecrement(volatile int16& Value)
{
  return __sync_sub_and_fetch(&Value, 1);
}
template<>
int16 Y_AtomicCompareExchange(volatile int16& Result, int16 Exchange, int16 Comparand)
{
  return __sync_val_compare_and_swap(&Result, Comparand, Exchange);
}
template<>
int16 Y_AtomicExchange(volatile int16& Result, int16 Exchange)
{
  __sync_synchronize();
  int16 Old = Result;
  Result = Exchange;
  __sync_synchronize();
  return Old;
}
template<>
int16 Y_AtomicAnd(volatile int16& Value, int16 AndVal)
{
  return __sync_and_and_fetch(&Value, AndVal);
}
template<>
uint16 Y_AtomicIncrement(volatile uint16& Value)
{
  return __sync_add_and_fetch(&Value, 1);
}
template<>
uint16 Y_AtomicDecrement(volatile uint16& Value)
{
  return __sync_sub_and_fetch(&Value, 1);
}
template<>
uint16 Y_AtomicCompareExchange(volatile uint16& Result, uint16 Exchange, uint16 Comparand)
{
  return __sync_val_compare_and_swap(&Result, Comparand, Exchange);
}
template<>
uint16 Y_AtomicExchange(volatile uint16& Result, uint16 Exchange)
{
  __sync_synchronize();
  uint16 Old = Result;
  Result = Exchange;
  __sync_synchronize();
  return Old;
}
template<>
uint16 Y_AtomicAnd(volatile uint16& Value, uint16 AndVal)
{
  return __sync_and_and_fetch(&Value, AndVal);
}

// int32
template<>
int32 Y_AtomicIncrement(volatile int32& Value)
{
  return __sync_add_and_fetch(&Value, 1);
}
template<>
int32 Y_AtomicDecrement(volatile int32& Value)
{
  return __sync_sub_and_fetch(&Value, 1);
}
template<>
int32 Y_AtomicCompareExchange(volatile int32& Result, int32 Exchange, int32 Comparand)
{
  return __sync_val_compare_and_swap(&Result, Comparand, Exchange);
}
template<>
int32 Y_AtomicExchange(volatile int32& Result, int32 Exchange)
{
  __sync_synchronize();
  int32 Old = Result;
  Result = Exchange;
  __sync_synchronize();
  return Old;
}
template<>
int32 Y_AtomicAnd(volatile int32& Value, int32 AndVal)
{
  return __sync_and_and_fetch(&Value, AndVal);
}
template<>
uint32 Y_AtomicIncrement(volatile uint32& Value)
{
  return __sync_add_and_fetch(&Value, 1);
}
template<>
uint32 Y_AtomicDecrement(volatile uint32& Value)
{
  return __sync_sub_and_fetch(&Value, 1);
}
template<>
uint32 Y_AtomicCompareExchange(volatile uint32& Result, uint32 Exchange, uint32 Comparand)
{
  return __sync_val_compare_and_swap(&Result, Comparand, Exchange);
}
template<>
uint32 Y_AtomicExchange(volatile uint32& Result, uint32 Exchange)
{
  __sync_synchronize();
  Result = Exchange;
  __sync_synchronize();
  return Exchange;
}
template<>
uint32 Y_AtomicAnd(volatile uint32& Value, uint32 AndVal)
{
  return __sync_and_and_fetch(&Value, AndVal);
}

// int64, double only supported on x64
#if Y_CPU_X64
template<>
int64 Y_AtomicIncrement(volatile int64& Value)
{
  return __sync_add_and_fetch(&Value, 1);
}
template<>
int64 Y_AtomicDecrement(volatile int64& Value)
{
  return __sync_sub_and_fetch(&Value, 1);
}
template<>
int64 Y_AtomicCompareExchange(volatile int64& Result, int64 Exchange, int64 Comparand)
{
  return __sync_val_compare_and_swap(&Result, Comparand, Exchange);
}
template<>
int64 Y_AtomicExchange(volatile int64& Result, int64 Exchange)
{
  __sync_synchronize();
  int64 Old = Result;
  Result = Exchange;
  __sync_synchronize();
  return Old;
}
template<>
int64 Y_AtomicAnd(volatile int64& Value, int64 AndVal)
{
  return __sync_and_and_fetch(&Value, AndVal);
}
template<>
uint64 Y_AtomicIncrement(volatile uint64& Value)
{
  return __sync_add_and_fetch(&Value, 1);
}
template<>
uint64 Y_AtomicDecrement(volatile uint64& Value)
{
  return __sync_sub_and_fetch(&Value, 1);
}
template<>
uint64 Y_AtomicCompareExchange(volatile uint64& Result, uint64 Exchange, uint64 Comparand)
{
  return __sync_val_compare_and_swap(&Result, Comparand, Exchange);
}
template<>
uint64 Y_AtomicExchange(volatile uint64& Result, uint64 Exchange)
{
  __sync_synchronize();
  uint64 Old = Result;
  Result = Exchange;
  __sync_synchronize();
  return Old;
}
template<>
uint64 Y_AtomicAnd(volatile uint64& Value, uint64 AndVal)
{
  return __sync_and_and_fetch(&Value, AndVal);
}
#endif

// pointers
void* Y_AtomicCompareExchangeVoidPointer(void* volatile& Pointer, void* Exchange, void* Comparand)
{
  return (void*)(__sync_val_compare_and_swap(&Pointer, Comparand, Exchange));
}
void* Y_AtomicExchangeVoidPointer(void* volatile& Pointer, void* Exchange)
{
  __sync_synchronize();
  void* Old = Pointer;
  Pointer = Exchange;
  __sync_synchronize();
  return Old;
}

#else

#error Unsupported compiler.
/*
static inline AtomicInt32 AtomicIncrement(AtomicInt32 *atomic) { return __sync_add_and_fetch(atomic, 1); }
static inline AtomicInt32 AtomicDecrement(AtomicInt32 *atomic) { return __sync_sub_and_fetch(atomic, 1); }
static inline AtomicInt32 AtomicCompareExchange(AtomicInt32 *atomic, AtomicInt32 exchange, AtomicInt32 comparand) {
return __sync_val_compare_and_swap(atomic, comparand, exchange); } static inline AtomicInt32 AtomicExchange(AtomicInt32
*atomic, AtomicInt32 value) { return __sync_lock_test_and_set(atomic, value); } static inline AtomicInt32
AtomicAnd(AtomicInt32 *atomic, AtomicInt32 value) { return __sync_and_and_fetch(atomic, value); }

static inline void *AtomicCompareExchangePointer(volatile void **pointer, void *exchange, void *comparand) { return
__sync_val_compare_and_swap(atomic, comparand, exchange); } static inline void *AtomicExchangePointer(volatile void
**pointer, void *value) { return __sync_lock_test_and_set(atomic, value); }
*/
#endif
