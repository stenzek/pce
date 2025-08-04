#pragma once

#include "YBaseLib/Assert.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Memory.h"
#include <new>

// array class suitable for any data type including classes
template<class T>
class Array
{
public:
  Array()
  {
    m_pElements = NULL;
    m_size = m_reserve = 0;
  }

  Array(uint32 nReserve)
  {
    m_pElements = NULL;
    m_size = m_reserve = 0;
  }

  Array(const Array& c)
  {
    m_pElements = NULL;
    m_size = m_reserve = 0;
    Assign(c);
  }

  Array(Array&& c)
  {
    m_pElements = c.m_pElements;
    m_size = c.m_size;
    m_reserve = c.m_reserve;
    c.m_pElements = NULL;
    c.m_reserve = 0;
    c.m_size = 0;
  }

  ~Array()
  {
    uint32 i;
    for (i = 0; i < m_size; i++)
      _Destruct(i);

    std::free(m_pElements);
  }

  void Clear()
  {
    if (m_pElements == NULL)
      return;

    for (uint32 i = 0; i < m_size; i++)
      _Destruct(i);

    m_size = 0;
  }

  void Assign(const Array<T>& c)
  {
    uint32 i;

    Clear();
    if (c.m_size > m_reserve)
      Reserve(c.m_size);

    for (i = 0; i < c.m_size; i++)
      _Construct(i, c._GetElement(i));

    m_size = c.m_size;
  }

  bool Equals(const Array<T>& c) const
  {
    if (c.m_size != m_size)
      return false;

    uint32 i;
    for (i = 0; i < m_size; i++)
    {
      if (m_pElements[i] != c.m_pElements[i])
        return false;
    }

    return true;
  }

  void Reserve(uint32 nElements)
  {
    if (m_reserve >= nElements)
      return;

    uint32 i;
    T* pOldElements = m_pElements;
    m_pElements = (T*)Y_mallocarray(sizeof(T), nElements);
    Assert(m_pElements != NULL);
    m_reserve = nElements;

    for (i = 0; i < m_size; i++)
      _Construct(i, pOldElements[i]);

    std::free(pOldElements);
  }

  void Shrink()
  {
    if (m_size == m_reserve || m_pElements == NULL)
      return;

    if (m_size > 0)
    {
      uint32 i;
      T* pOldElements = m_pElements;
      m_pElements = (T*)Y_mallocarray(sizeof(T), m_size);
      Assert(m_pElements != NULL);
      m_reserve = m_size;

      for (i = 0; i < m_size; i++)
        _Construct(i, pOldElements[i]);

      std::free(pOldElements);
    }
    else
    {
      std::free(m_pElements);
      m_pElements = NULL;
      m_reserve = 0;
    }
  }

  void Resize(uint32 newSize)
  {
    // expanding?
    if (newSize < m_size)
    {
      while (m_size > newSize)
      {
        _Destruct(m_size);
        m_size--;
      }
    }
    else
    {
      if (newSize > m_reserve)
        Reserve(newSize);

      while (m_size < newSize)
      {
        _Construct(m_size);
        m_size++;
      }
    }
  }

  void Swap(Array& s)
  {
    ::Swap(m_pElements, s.m_pElements);
    ::Swap(m_size, s.m_size);
    ::Swap(m_reserve, s.m_reserve);
  }

  void Add(const T& value)
  {
    // autosize
    if (m_size == m_reserve)
      Reserve(Max(m_size + 1, m_size * 2));

    _Construct(m_size, value);
    m_size++;
  }

  template<typename... Args>
  void Emplace(Args... args)
  {
    // autosize
    if (m_size == m_reserve)
      Reserve(Max(m_size + 1, m_size * 2));

    new (&m_pElements[m_size]) T(args...);
    m_size++;
  }

  void Pop()
  {
    DebugAssert(m_size > 0);

    _Destruct(m_size - 1);
    m_size--;
  }

  void FastRemove(uint32 index)
  {
    DebugAssert(index < m_size);

    // test if it's the end elements
    if (index == (m_size - 1))
    {
      Pop();
    }
    else
    {
      // swap end and index elements
      _Destruct(index);
      _Construct(index, _GetElement(m_size - 1));
      _Destruct(m_size - 1);
      m_size--;
    }
  }

  void OrderedRemove(uint32 index)
  {
    DebugAssert(index < m_size);

    // test if it's the end element
    if (index == (m_size - 1))
    {
      Pop();
    }
    else
    {
      // destruct index
      _Destruct(index);

      // loop and move everything from index+1 back one index
      uint32 i;
      for (i = index + 1; i < m_size; i++)
      {
        _Construct(i - 1, _GetElement(i));
        _Destruct(i);
      }
    }
  }

  void Obliterate()
  {
    if (m_pElements != NULL)
    {
      Clear();
      std::free(m_pElements);
      m_pElements = NULL;
      m_reserve = 0;
    }
  }

  uint32 GetSize() const { return m_size; }

  uint32 GetReserve() const { return m_reserve; }

  // element accessors
  const T& FirstElement() const
  {
    DebugAssert(m_size > 0);
    return m_pElements[0];
  }
  const T& LastElement() const
  {
    DebugAssert(m_size > 0);
    return m_pElements[m_size - 1];
  }
  T& FirstElement()
  {
    DebugAssert(m_size > 0);
    return m_pElements[0];
  }
  T& LastElement()
  {
    DebugAssert(m_size > 0);
    return m_pElements[m_size - 1];
  }

  // element accessor

  const T& GetElement(uint32 i) const
  {
    DebugAssert(i < m_size);
    return _GetElement(i);
  }

  T& GetElement(uint32 i)
  {
    DebugAssert(i < m_size);
    return _GetElement(i);
  }

  // ensure space exists for the next n elements
  void EnsureReserve(uint32 n)
  {
    if ((m_size + n) > m_reserve)
      Reserve(Max(m_size + n, m_size * 2));
  }

  // exists in array?
  bool Contains(const T& item) const { return (IndexOf(item) >= 0); }
  bool IsEmpty() const { return (m_size == 0); }

  // base ptr
  const T* GetBasePointer() const { return m_pElements; }

  T* GetBasePointer() { return m_pElements; }

  // mainly for range-based for
  const T* begin() const { return m_pElements; }
  const T* end() const { return (m_size > 0) ? &m_pElements[m_size] : m_pElements; }
  T* begin() { return m_pElements; }
  T* end() { return (m_size > 0) ? &m_pElements[m_size] : m_pElements; }

  // assignment operator
  Array<T>& operator=(const Array<T>& c)
  {
    Assign(c);
    return *this;
  }
  Array<T>& operator=(Array<T>&& c)
  {
    Swap(c);
    return *this;
  }

  // operator wrappers
  bool operator==(const Array<T>& c) const { return Equals(c); }
  bool operator!=(const Array<T>& c) const { return !Equals(c); }
  const T& operator[](uint32 index) const { return GetElement(index); }
  T& operator[](uint32 index) { return GetElement(index); }

private:
  T* m_pElements;
  uint32 m_size;
  uint32 m_reserve;

  // helper functions
  inline void _Construct(uint32 i) { new (&m_pElements[i]) T; }

  inline void _Construct(uint32 i, const T& v) { new (&m_pElements[i]) T(v); }

  inline void _Destruct(uint32 i) { m_pElements[i].~T(); }

  inline void _Replace(uint32 i, const T& v)
  {
    _Destruct(i);
    _Construct(i, v);
  }

  inline const T& _GetElement(uint32 i) const { return m_pElements[i]; }

  inline T& _GetElement(uint32 i) { return m_pElements[i]; }
};
