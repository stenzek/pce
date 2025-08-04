#pragma once
#include "YBaseLib/Assert.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Memory.h"

template<typename TYPE>
class BitSet
{
public:
  const size_t BITS_PER_VALUE = sizeof(TYPE) * 8;

public:
  BitSet() : m_values(nullptr), m_valueCount(0), m_bitCount(0) {}

  BitSet(const BitSet& copy)
  {
    m_values = (copy.m_values != nullptr) ? (TYPE*)Y_reallocarray(nullptr, copy.m_valueCount, sizeof(TYPE)) : nullptr;
    m_valueCount = copy.m_valueCount;
    m_bitCount = copy.m_bitCount;
  }

  BitSet(const BitSet&& from)
  {
    m_values = from.m_values;
    from.m_values = nullptr;

    m_valueCount = from.m_valueCount;
    from.m_valueCount = 0;

    m_bitCount = from.m_bitCount;
    from.m_bitCount = 0;
  }

  BitSet(size_t bits)
  {
    m_values = nullptr;
    m_valueCount = 0;
    m_bitCount = 0;
    Resize(bits);
  }

  ~BitSet() { std::free(m_values); }

  TYPE* GetValuesPointer() { return m_values; }
  const TYPE* GetValuesPointer() const { return m_values; }
  const size_t GetValueCount() const { return m_valueCount; }
  const size_t GetBitCount() const { return m_bitCount; }

  void Clear() { Y_memzero(m_values, sizeof(TYPE) * m_valueCount); }

  void Resize(size_t bits)
  {
    if (bits == m_bitCount)
      return;

    if (bits > 0)
    {
      size_t oldValues = m_valueCount;
      size_t newValues = bits / BITS_PER_VALUE + 1;
      m_bitCount = bits;
      m_valueCount = newValues;
      if (oldValues != newValues)
      {
        m_values = (TYPE*)Y_reallocarray(m_values, m_valueCount, sizeof(TYPE));
        if (m_valueCount > oldValues)
          Y_memzero(m_values + oldValues, sizeof(TYPE) * (m_valueCount - oldValues));
      }
    }
    else
    {
      std::free(m_values);
      m_values = nullptr;
      m_valueCount = m_bitCount = 0;
    }
  }

  bool TestBit(size_t bit) const
  {
    DebugAssert(bit < m_bitCount);
    return (m_values[bit / BITS_PER_VALUE] & (1 << (bit % BITS_PER_VALUE))) != 0;
  }

  void SetBit(size_t bit, bool on = true)
  {
    DebugAssert(bit < m_bitCount);
    if (on)
      m_values[bit / BITS_PER_VALUE] |= TYPE(1 << (bit % BITS_PER_VALUE));
    else
      m_values[bit / BITS_PER_VALUE] &= ~TYPE(1 << (bit % BITS_PER_VALUE));
  }

  void UnsetBit(size_t bit) { SetBit(bit, false); }

  void FlipBit(size_t bit)
  {
    DebugAssert(bit < m_bitCount);
    m_values[bit / BITS_PER_VALUE] ^= TYPE(1 << (bit % BITS_PER_VALUE));
  }

  bool TestSetMask(const BitSet& mask) const
  {
    if (mask.m_valueCount > m_valueCount)
    {
      // test the extra bits
      for (size_t i = m_valueCount; i < mask.m_valueCount; i++)
      {
        if (mask.m_values[i] != 0)
          return false;
      }
    }

    // test our bits
    for (size_t i = 0; i < m_valueCount; i++)
    {
      if ((m_values[i] & mask.m_values[i]) != mask.m_values[i])
        return false;
    }
    return true;
  }

  bool TestSetEqual(const BitSet& test) const
  {
    if (test.m_valueCount != m_valueCount)
      return false;

    for (size_t i = 0; i < m_valueCount; i++)
    {
      if (m_values[i] != mask.m_values[i])
        return false;
    }

    return true;
  }

  void Copy(const BitSet& other)
  {
    if (m_valueCount != other.m_bitCount)
      Resize(other.m_bitCount);

    DebugAssert(m_valueCount == other.m_valueCount);
    if (m_valueCount > 0)
      std::memcpy(m_values, other.m_values, sizeof(TYPE) * m_valueCount);
  }

  void Swap(BitSet& other)
  {
    ::Swap(m_bitCount, other.m_bitCount);
    ::Swap(m_valueCount, other.m_valueCount);
    ::Swap(m_values, other.m_values);
  }

  void Combine(const BitSet& other)
  {
    if (other.m_bitCount > m_bitCount)
      Resize(other.m_bitCount);

    size_t loopCount = Min(m_valueCount, other.m_valueCount);
    for (size_t i = 0; i < loopCount; i++)
      m_values[i] |= other.m_values[i];
  }

  void Mask(const BitSet& other)
  {
    if (other.m_valueCount < m_valueCount)
    {
      for (size_t i = 0; i < other.m_valueCount; i++)
        m_values[i] &= other.m_values[i];
      for (size_t i = other.m_valueCount; i < m_valueCount; i++)
        m_valueCount[i] = 0;
    }
    else
    {
      size_t loopCount = Min(m_valueCount, other.m_valueCount);
      for (size_t i = 0; i < loopCount; i++)
        m_values[i] &= other.m_values[i];
    }
  }

  void Flip()
  {
    for (size_t i = 0; i < m_valueCount; i++)
    {
      // careful not to flip out of bit range
      if ((i + 1) * BITS_PER_VALUE > m_bitCount)
      {
        size_t count = BITS_PER_VALUE - (((i + 1) * BITS_PER_VALUE) - m_bitCount);
        for (size_t j = 0; j < count; j++)
          m_values[i] ^= (1 << j);
      }
      else
      {
        m_values[i] = ~m_values[i];
      }
    }
  }

  bool FindFirstSetBit(size_t* foundIndex) const
  {
    for (size_t i = 0; i < m_valueCount; i++)
    {
      if (m_values[i] == 0)
        continue;

      uint32 index;
      Y_bitscanforward(m_values[i], &index);
      *foundIndex = i * BITS_PER_VALUE + index;
      return true;
    }

    return false;
  }

  bool FindFirstClearBit(size_t* foundIndex) const
  {
    for (size_t i = 0; i < m_valueCount; i++)
    {
      if (m_values[i] == ~(TYPE)0)
        continue;

      uint32 index;
      Y_bitscanforward(static_cast<uint32>(~m_values[i]), &index);
      *foundIndex = i * BITS_PER_VALUE + index;
      return true;
    }

    return false;
  }

  bool FindContiguousClearBits(size_t count, size_t* foundStartingIndex) const
  {
    for (size_t i = 0; i < m_valueCount; i++)
    {
      if (m_values[i] == ~(TYPE)0)
        continue;

      TYPE mask = TYPE(size_t(1 << Min(BITS_PER_VALUE, count)) - 1);
      TYPE value = m_values[i];
      size_t pos;
      for (pos = 0; pos < BITS_PER_VALUE; pos++)
      {
        if ((value & mask) == 0)
          break;

        mask <<= 1;
      }
      if (pos == BITS_PER_VALUE)
        continue;

      size_t bitsRemaining = ((BITS_PER_VALUE - pos) > count) ? 0 : (count - (BITS_PER_VALUE - pos));
      size_t adjacentIndex = i + 1;
      while (bitsRemaining > 0 && adjacentIndex < m_valueCount)
      {
        mask = TYPE(size_t(1 << Min(BITS_PER_VALUE, bitsRemaining)) - 1);
        if ((m_values[adjacentIndex] & mask) == 0)
        {
          bitsRemaining = (BITS_PER_VALUE > bitsRemaining) ? 0 : bitsRemaining - BITS_PER_VALUE;
          continue;
        }
        else
        {
          break;
        }
      }
      if (bitsRemaining == 0)
      {
        *foundStartingIndex = i * BITS_PER_VALUE + pos;
        return true;
      }
    }

    return false;
  }

  size_t Cardinality() const
  {
    size_t count = 0;
    for (size_t i = 0; i < m_valueCount; i++)
      count += (size_t)Y_popcnt(m_values[i]);
    return count;
  }

  // wrapper for operator[]
  class IndexAccessor
  {
  private:
    BitSet* bitset;
    size_t index;

  public:
    IndexAccessor(BitSet* bitset, size_t index) : bitset(bitset), index(index) {}
    operator bool() const { return bitset->TestBit(index); }
    IndexAccessor& operator=(bool value)
    {
      bitset->SetBit(index, value);
      return *this;
    }
  };

  // operators
  IndexAccessor operator[](size_t index)
  {
    DebugAssert(index < m_bitCount);
    return IndexAccessor(this, index);
  }
  bool operator[](size_t index) const { return TestBit(index); }
  bool operator==(const BitSet& rhs) const { return TestSetEqual(rhs); }
  bool operator!=(const BitSet& rhs) const { return !TestSetEqual(rhs); }
  BitSet& operator=(const BitSet& rhs)
  {
    Copy(rhs);
    return *this;
  }
  BitSet& operator=(const BitSet&& rhs)
  {
    Swap(rhs);
    return *this;
  }
  BitSet& operator|=(const BitSet& rhs)
  {
    Combine(rhs);
    return *this;
  }
  BitSet& operator&=(const BitSet& rhs)
  {
    Mask(rhs);
    return *this;
  }
  BitSet operator|(const BitSet& rhs) const
  {
    BitSet ret(*this);
    ret.Combine(rhs);
    return ret;
  }
  BitSet operator&(const BitSet& rhs) const
  {
    BitSet ret(*this);
    ret.Mask(rhs);
    return ret;
  }
  BitSet operator~() const
  {
    BitSet ret(*this);
    ret.Flip();
    return ret;
  }

private:
  TYPE* m_values;
  size_t m_valueCount;
  size_t m_bitCount;
};

typedef BitSet<uint8> BitSet8;
typedef BitSet<uint32> BitSet32;
typedef BitSet<uint64> BitSet64;
