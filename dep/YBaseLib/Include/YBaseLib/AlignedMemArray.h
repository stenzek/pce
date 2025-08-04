#pragma once
#include "YBaseLib/Assert.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Memory.h"

// array class suitable for POD types (int, float, etc), or simple structs with no dependance on memory location
template<class T, int ALIGNMENT = 16>
class AlignedMemArray
{
public:
  static_assert((sizeof(T) % ALIGNMENT) == 0, "size of T must be divisable by ALIGNMENT");

  AlignedMemArray()
  {
    m_pElements = NULL;
    m_uSize = m_uReserve = 0;
  }

  AlignedMemArray(uint32 nReserve)
  {
    m_pElements = NULL;
    m_uSize = m_uReserve = 0;
  }

  AlignedMemArray(const AlignedMemArray& c)
  {
    m_pElements = NULL;
    m_uSize = m_uReserve = 0;
    Copy(c);
  }

  ~AlignedMemArray() { Y_aligned_free(m_pElements); }

  void Clear()
  {
    if (m_pElements == NULL)
      return;

    m_uSize = 0;
  }

  void Copy(const AlignedMemArray& c)
  {
    Clear();
    Reserve(c.m_uReserve);
    m_uSize = c.m_uSize;
    if (m_uSize > 0)
      std::memcpy(m_pElements, c.m_pElements, sizeof(T) * m_uSize);
  }

  void Swap(AlignedMemArray& c)
  {
    ::Swap(m_pElements, c.m_pElements);
    ::Swap(m_uSize, c.m_uSize);
    ::Swap(m_uReserve, c.m_uReserve);
  }

  void Obliterate()
  {
    if (m_pElements == NULL)
      return;

    Clear();
    Y_aligned_free(m_pElements);
    m_pElements = NULL;
    m_uReserve = 0;
  }

  bool Equals(const AlignedMemArray& c) const
  {
    if (c.m_uSize != m_uSize)
      return false;

    return memcmp(m_pElements, c.m_pElements, sizeof(T) * m_uSize) == 0;
  }

  void Reserve(uint32 nElements)
  {
    if (m_uReserve >= nElements)
      return;

    if (m_pElements != NULL)
    {
      T* pNewElements = (T*)Y_aligned_malloc(sizeof(T) * nElements, ALIGNMENT);
      Assert(pNewElements != NULL);
      if (m_uSize > 0)
        std::memcpy(pNewElements, m_pElements, sizeof(T) * m_uSize);

      Y_aligned_free(m_pElements);
      m_pElements = pNewElements;
    }
    else
    {
      m_pElements = (T*)Y_aligned_malloc(sizeof(T) * nElements, ALIGNMENT);
      Assert(m_pElements != NULL);
    }
    m_uReserve = nElements;
  }

  void Resize(uint32 newSize)
  {
    // expanding?
    if (newSize < m_uSize)
    {
      m_uSize = newSize;
    }
    else
    {
      if (newSize > m_uReserve)
        Reserve(newSize);

      m_uSize = newSize;
    }
  }

  void Shrink()
  {
    if (m_uReserve == 0)
      return;

    // if reserve > size, resize down to size
    if (m_uSize == 0)
    {
      Y_aligned_free(m_pElements);
      m_pElements = NULL;
      m_uReserve = 0;
    }
    else if (m_uReserve > m_uSize)
    {
      // m_pElements = (T *)Y_aligned_realloc(m_pElements, SIZEOF_T * m_uSize, ALIGNMENT);
      T* pNewElements = (T*)Y_aligned_malloc(sizeof(T) * m_uSize, ALIGNMENT);
      Assert(pNewElements != NULL);
      std::memcpy(pNewElements, m_pElements, sizeof(T) * m_uSize);
      Y_aligned_free(m_pElements);
      m_pElements = pNewElements;
      m_uReserve = m_uSize;
    }
  }

  void Add(const T& value)
  {
    // autosize
    if (m_uSize == m_uReserve)
      Reserve(Max(m_uSize + 1, m_uSize * 2));

    std::memcpy(m_pElements + m_uSize, &value, sizeof(T));
    m_uSize++;
  }

  void Pop()
  {
    DebugAssert(m_uSize > 0);

    m_uSize--;
  }

  void FastRemove(uint32 index)
  {
    DebugAssert(index < m_uSize);

    // test if it's the end elements
    if (index == (m_uSize - 1))
    {
      Pop();
    }
    else
    {
      // swap end and index elements
      std::memcpy(&m_pElements[index], &m_pElements[m_uSize - 1], sizeof(T));
      m_uSize--;
    }
  }

  void OrderedRemove(uint32 index)
  {
    Assert(index < m_uSize);

    // test if it's the end element
    if (index == (m_uSize - 1))
    {
      Pop();
    }
    else
    {
      // loop and move everything from index+1 back one index
      std::memmove(m_pElements + index, m_pElements + index + 1, sizeof(T) * (m_uSize - index - 1));
      m_uSize--;
    }
  }

  void DetachArray(T** pBasePointer, uint32* pSize)
  {
    DebugAssert(pBasePointer != NULL && pSize != NULL);
    *pBasePointer = m_pElements;
    *pSize = m_uSize;
    m_pElements = NULL;
    m_uSize = m_uReserve = 0;
  }

  uint32 GetSize() const { return m_uSize; }

  uint32 GetReserve() const { return m_uReserve; }

  const T& GetElement(uint32 i) const
  {
    DebugAssert(i < m_uSize);
    return m_pElements[i];
  }

  T& GetElement(uint32 i)
  {
    DebugAssert(i < m_uSize);
    return m_pElements[i];
  }

  // pointer readers, use with care
  const T* GetBasePointer() const { return m_pElements; }
  T* GetBasePointer() { return m_pElements; }

  // assignment operator
  AlignedMemArray<T, ALIGNMENT>& operator=(const AlignedMemArray<T, ALIGNMENT>& c)
  {
    Assign(c);
    return *this;
  }

  // operator wrappers
  bool operator==(const AlignedMemArray<T, ALIGNMENT>& c) const { return Equals(c); }
  bool operator!=(const AlignedMemArray<T, ALIGNMENT>& c) const { return !Equals(c); }
  const T& operator[](uint32 index) const { return GetElement(index); }
  T& operator[](uint32 index) { return GetElement(index); }

private:
  T* m_pElements;
  uint32 m_uSize;
  uint32 m_uReserve;
};
