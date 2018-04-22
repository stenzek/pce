#pragma once

#include "YBaseLib/Common.h"
#include <cstring>
#include <type_traits>

// Physical memory addresses are 32-bits wide
using PhysicalMemoryAddress = uint32;
using LinearMemoryAddress = uint32;

// Use a signed number for cycle counting
using CycleCount = int64;

// Use int64 for time tracking
using SimulationTime = int64;

// IO port read/write sizes
enum IOPortDataSize : uint32
{
  IOPortDataSize_8,
  IOPortDataSize_16
};

enum class CPUBackendType
{
  Interpreter,
  FastInterpreter,
  CachedInterpreter,
  NewInterpreter,
  Recompiler
};

// Zero-extending helper
template<typename TReturn, typename TValue>
constexpr TReturn ZeroExtend(TValue value)
{
  // auto unsigned_val = static_cast<typename std::make_unsigned<TValue>::type>(value);
  // auto extended_val = static_cast<typename std::make_unsigned<TReturn>::type>(unsigned_val);
  // return static_cast<TReturn>(extended_val);
  return static_cast<TReturn>(static_cast<typename std::make_unsigned<TReturn>::type>(
    static_cast<typename std::make_unsigned<TValue>::type>(value)));
}
// Sign-extending helper
template<typename TReturn, typename TValue>
constexpr TReturn SignExtend(TValue value)
{
  // auto signed_val = static_cast<typename std::make_signed<TValue>::type>(value);
  // auto extended_val = static_cast<typename std::make_signed<TReturn>::type>(signed_val);
  // return static_cast<TReturn>(extended_val);
  return static_cast<TReturn>(
    static_cast<typename std::make_signed<TReturn>::type>(static_cast<typename std::make_signed<TValue>::type>(value)));
}
// Truncating helper
template<typename TReturn, typename TValue>
constexpr TReturn TruncateBits(TValue value)
{
  return static_cast<TReturn>(value & ~static_cast<TReturn>(0));
}

// Type-specific helpers
template<typename TValue>
constexpr uint16 ZeroExtend16(TValue value)
{
  return ZeroExtend<uint16, TValue>(value);
}
template<typename TValue>
constexpr uint32 ZeroExtend32(TValue value)
{
  return ZeroExtend<uint32, TValue>(value);
}
template<typename TValue>
constexpr uint64 ZeroExtend64(TValue value)
{
  return ZeroExtend<uint64, TValue>(value);
}
template<typename TValue>
constexpr uint16 SignExtend16(TValue value)
{
  return SignExtend<uint16, TValue>(value);
}
template<typename TValue>
constexpr uint32 SignExtend32(TValue value)
{
  return SignExtend<uint32, TValue>(value);
}
template<typename TValue>
constexpr uint64 SignExtend64(TValue value)
{
  return SignExtend<uint64, TValue>(value);
}
template<typename TValue>
constexpr uint8 Truncate8(TValue value)
{
  return TruncateBits<uint8, TValue>(value);
}
template<typename TValue>
constexpr uint16 Truncate16(TValue value)
{
  return TruncateBits<uint16, TValue>(value);
}
template<typename TValue>
constexpr uint32 Truncate32(TValue value)
{
  return TruncateBits<uint32, TValue>(value);
}

// BCD helpers
inline uint8 DecimalToBCD(uint8 value)
{
  return ((value / 10) << 4) + (value % 10);
}

inline uint8 BCDToDecimal(uint8 value)
{
  return ((value >> 4) * 10) + (value % 16);
}

// Boolean to integer
constexpr uint8 BoolToUInt8(bool value)
{
  return static_cast<uint8>(value);
}
constexpr uint16 BoolToUInt16(bool value)
{
  return static_cast<uint16>(value);
}
constexpr uint32 BoolToUInt32(bool value)
{
  return static_cast<uint32>(value);
}
constexpr uint64 BoolToUInt64(bool value)
{
  return static_cast<uint64>(value);
}

// Integer to boolean
template<typename TValue>
constexpr bool ConvertToBool(TValue value)
{
  return static_cast<bool>(value);
}

// Unsafe integer to boolean
template<typename TValue>
constexpr bool ConvertToBoolUnchecked(TValue value)
{
  static_assert(sizeof(uint8) == sizeof(bool));
  bool ret;
  std::memcpy(&ret, &value, sizeof(bool));
  return ret;
}

// Enum class bitwise operators
#define IMPLEMENT_ENUM_CLASS_BITWISE_OPERATORS(type_)                                                                  \
  inline type_ operator&(type_ lhs, type_ rhs)                                                                         \
  {                                                                                                                    \
    return static_cast<type_>(static_cast<std::underlying_type<type_>::type>(lhs) &                                    \
                              static_cast<std::underlying_type<type_>::type>(rhs));                                    \
  }                                                                                                                    \
  inline type_ operator|(type_ lhs, type_ rhs)                                                                         \
  {                                                                                                                    \
    return static_cast<type_>(static_cast<std::underlying_type<type_>::type>(lhs) |                                    \
                              static_cast<std::underlying_type<type_>::type>(rhs));                                    \
  }                                                                                                                    \
  inline type_ operator^(type_ lhs, type_ rhs)                                                                         \
  {                                                                                                                    \
    return static_cast<type_>(static_cast<std::underlying_type<type_>::type>(lhs) ^                                    \
                              static_cast<std::underlying_type<type_>::type>(rhs));                                    \
  }                                                                                                                    \
  inline type_ operator~(type_ val)                                                                                    \
  {                                                                                                                    \
    return static_cast<type_>(~static_cast<std::underlying_type<type_>::type>(val));                                   \
  }                                                                                                                    \
  inline type_& operator&=(type_& lhs, type_ rhs)                                                                      \
  {                                                                                                                    \
    lhs = static_cast<type_>(static_cast<std::underlying_type<type_>::type>(lhs) &                                     \
                             static_cast<std::underlying_type<type_>::type>(rhs));                                     \
    return lhs;                                                                                                        \
  }                                                                                                                    \
  inline type_& operator|=(type_& lhs, type_ rhs)                                                                      \
  {                                                                                                                    \
    lhs = static_cast<type_>(static_cast<std::underlying_type<type_>::type>(lhs) |                                     \
                             static_cast<std::underlying_type<type_>::type>(rhs));                                     \
    return lhs;                                                                                                        \
  }                                                                                                                    \
  inline type_& operator^=(type_& lhs, type_ rhs)                                                                      \
  {                                                                                                                    \
    lhs = static_cast<type_>(static_cast<std::underlying_type<type_>::type>(lhs) ^                                     \
                             static_cast<std::underlying_type<type_>::type>(rhs));                                     \
    return lhs;                                                                                                        \
  }
