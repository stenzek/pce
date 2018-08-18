#pragma once

#include "YBaseLib/Common.h"
#include <type_traits>

// Disable MSVC warnings that we actually handle
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4800) // warning C4800: 'int': forcing value to bool 'true' or 'false' (performance warning)
#endif

template<typename BackingDataType, typename DataType, uint32 BitIndex, uint32 BitCount>
struct BitField
{
  constexpr BackingDataType GetMask() const
  {
    return ((static_cast<BackingDataType>(~0)) >> (8 * sizeof(BackingDataType) - BitCount)) << BitIndex;
  }

  operator DataType() const { return GetValue(); }

  BitField& operator=(const typename BitField<BackingDataType, DataType, BitIndex, BitCount>& value)
  {
    SetValue(value.GetValue());
    return *this;
  }

  BitField& operator=(DataType value)
  {
    SetValue(value);
    return *this;
  }

  DataType operator++()
  {
    DataType value = GetValue() + 1;
    SetValue(value);
    return GetValue();
  }

  DataType operator++(int)
  {
    DataType value = GetValue();
    SetValue(value + 1);
    return value;
  }

  DataType operator--()
  {
    DataType value = GetValue() - 1;
    SetValue(value);
    return GetValue();
  }

  DataType operator--(int)
  {
    DataType value = GetValue();
    SetValue(value - 1);
    return value;
  }

  //     BitField& operator^=(DataType value)
  //     {
  //         data &= GetMask();
  //         value = *this ^ value;
  //         data |= (static_cast<BackingDataType>(value) << BitIndex) & GetMask();
  //         return *this;
  //     }

  DataType GetValue() const
  {
    // TODO: Handle signed types
    if (std::is_same<DataType, bool>::value)
      return static_cast<DataType>(!!((data & GetMask()) >> BitIndex));
    else
      return static_cast<DataType>((data & GetMask()) >> BitIndex);
  }

  void SetValue(DataType value)
  {
    data &= ~GetMask();
    data |= (static_cast<BackingDataType>(value) << BitIndex) & GetMask();
  }

  BackingDataType data;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif