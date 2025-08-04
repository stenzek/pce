#pragma once
#include "YBaseLib/Assert.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/Memory.h"

// array class suitable for POD types (int, float, etc)
template<class T>
class PODArray
{
public:
  PODArray()
  {
    m_pElements = NULL;
    m_size = m_reserve = 0;
  }

  PODArray(uint32 nReserve)
  {
    m_pElements = NULL;
    m_size = m_reserve = 0;
  }

  PODArray(const PODArray& c)
  {
    m_pElements = NULL;
    m_size = m_reserve = 0;
    Assign(c);
  }

  PODArray(PODArray&& c)
  {
    m_pElements = c.m_pElements;
    m_size = c.m_size;
    m_reserve = c.m_reserve;
    c.m_pElements = NULL;
    c.m_size = 0;
    c.m_reserve = 0;
  }

  ~PODArray() { std::free(m_pElements); }

  void Clear()
  {
    if (m_pElements == NULL)
      return;

    m_size = 0;
  }

  void Assign(const PODArray<T>& c)
  {
    Clear();
    Reserve(c.m_reserve);
    m_size = c.m_size;
    if (m_size > 0)
      std::memcpy(m_pElements, c.m_pElements, sizeof(T) * m_size);
  }

  void Swap(PODArray<T>& c)
  {
    ::Swap(m_pElements, c.m_pElements);
    ::Swap(m_size, c.m_size);
    ::Swap(m_reserve, c.m_reserve);
  }

  void Obliterate()
  {
    if (m_pElements == NULL)
      return;

    Clear();
    std::free(m_pElements);
    m_pElements = NULL;
    m_reserve = 0;
  }

  bool Equals(const PODArray<T>& c) const
  {
    if (c.m_size != m_size)
      return false;

    return std::memcmp(m_pElements, c.m_pElements, sizeof(T) * m_size) == 0;
  }

  void Reserve(uint32 nElements)
  {
    if (m_reserve >= nElements)
      return;

    m_pElements = (T*)Y_reallocarray(m_pElements, sizeof(T), nElements);
    m_reserve = nElements;
  }

  void Resize(uint32 newSize)
  {
    // expanding?
    if (newSize < m_size)
    {
      m_size = newSize;
    }
    else
    {
      if (newSize > m_reserve)
        Reserve(newSize);

      Y_memzero(m_pElements + m_size, sizeof(T) * (newSize - m_size));
      m_size = newSize;
    }
  }

  void Shrink()
  {
    if (m_reserve == 0)
      return;

    // if reserve > size, resize down to size
    if (m_size == 0)
    {
      std::free(m_pElements);
      m_pElements = NULL;
      m_reserve = 0;
    }
    else if (m_reserve > m_size)
    {
      m_pElements = (T*)Y_reallocarray(m_pElements, sizeof(T), m_size);
      m_reserve = m_size;
    }
  }

  void Add(const T& value)
  {
    // autosize
    if (m_size == m_reserve)
      Reserve(Max(m_size + 1, m_size * 2));

    m_pElements[m_size] = value;
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

  void AddRange(const T* pValues, uint32 count)
  {
    if (!count)
      return;

    if ((m_size + count) >= m_reserve)
      Reserve(Max(m_size + count, m_size * 2));

    std::memcpy(m_pElements + m_size, pValues, sizeof(T) * count);
    m_size += count;
  }

  void AddArray(const PODArray<T>& array)
  {
    if (array.GetSize() > 0)
      AddRange(array.GetBasePointer(), array.GetSize());
  }

  void RemoveFront()
  {
    DebugAssert(m_size > 0);
    std::memmove(m_pElements, m_pElements + 1, sizeof(T) * (m_size - 1));
    m_size--;
  }

  void RemoveBack()
  {
    DebugAssert(m_size > 0);
    m_size--;
  }

  T PopFront()
  {
    DebugAssert(m_size > 0);
    T returnValue = m_pElements[0];

    RemoveFront();

    return returnValue;
  }

  T PopBack()
  {
    DebugAssert(m_size > 0);

    T returnValue = m_pElements[m_size - 1];

    RemoveBack();

    return returnValue;
  }

  void FastRemove(uint32 index)
  {
    DebugAssert(index < m_size);

    // test if it's the end elements
    if (index == (m_size - 1))
    {
      m_size--;
    }
    else
    {
      // swap end and index elements
      m_pElements[index] = m_pElements[m_size - 1];
      m_size--;
    }
  }

  void OrderedRemove(uint32 index)
  {
    DebugAssert(index < m_size);

    // test if it's the end element
    if (index == (m_size - 1))
    {
      m_size--;
    }
    else
    {
      // loop and move everything from index+1 back one index
      std::memmove(m_pElements + index, m_pElements + index + 1, sizeof(T) * (m_size - index - 1));
      m_size--;
    }
  }

  void RemoveRange(uint32 first, uint32 count)
  {
    DebugAssert((first + count) <= m_size);
    if (first == 0 && count == m_size)
    {
      // erase the whole thing
      Clear();
      return;
    }

    // taking a slice off the end?
    if ((first + count) == m_size)
    {
      // just adjust the end
      m_size = first;
    }
    else
    {
      // move everything after first + count backwards
      std::memmove(m_pElements + first, m_pElements + first + count, sizeof(T) * (m_size - first - count));
      m_size -= count;
    }
  }

  bool FastRemoveItem(const T& item)
  {
    int32 index = IndexOf(item);
    if (index < 0)
      return false;

    FastRemove(index);
    return true;
  }

  bool OrderedRemoveItem(const T& item)
  {
    int32 index = IndexOf(item);
    if (index < 0)
      return false;

    OrderedRemove(index);
    return true;
  }

  void DetachArray(T** pBasePointer, uint32* pSize)
  {
    DebugAssert(pBasePointer != NULL && pSize != NULL);
    *pBasePointer = m_pElements;
    *pSize = m_size;
    m_pElements = NULL;
    m_size = m_reserve = 0;
  }

  uint32 GetSize() const { return m_size; }

  uint32 GetReserve() const { return m_reserve; }

  const T& GetElement(uint32 i) const
  {
    DebugAssert(i < m_size);
    return m_pElements[i];
  }

  uint32 GetStorageSizeInBytes() const { return m_size * sizeof(T); }

  uint32 GetStorageReserveInBytes() const { return m_reserve * sizeof(T); }

  template<class CB>
  void SortCB(CB callback)
  {
    struct __unused__
    {
      static int __cb_trampoline(void* context, const void* pLeft, const void* pRight)
      {
        return (*reinterpret_cast<CB*>(context))(*reinterpret_cast<const T*>(pLeft),
                                                 *reinterpret_cast<const T*>(pRight));
      }
    };

    if (m_size > 0)
      Y_qsort_r(m_pElements, m_size, sizeof(T), reinterpret_cast<void*>(&callback), __unused__::__cb_trampoline);
  }

  void Sort(int (*CompareFunction)(const T* pLeft, const T* pRight))
  {
    if (m_size > 0)
      Y_qsortT(m_pElements, m_size, CompareFunction);
  }

  bool BinarySearch(uint32* pOutIndex, const T& searchKey,
                    int (*CompareFunction)(const T* pLeft, const T* pRight)) const
  {
    if (m_size == 0)
      return false;

    const T* pResult = Y_bsearchT(&searchKey, m_pElements, m_size, CompareFunction);
    if (pResult == NULL)
      return false;

    if (pOutIndex != NULL)
      *pOutIndex = pResult - m_pElements;

    return true;
  }

  template<typename key_t>
  bool BinarySearchKey(uint32* pOutIndex, const key_t& searchKey,
                       int (*SearchCompareFunction)(const key_t* pSearchKey, const T* pElement)) const
  {
    if (m_size == 0)
      return false;

    const T* pResult = Y_bsearchT(&searchKey, m_pElements, m_size, SearchCompareFunction);
    if (pResult == NULL)
      return false;

    if (pOutIndex != NULL)
      *pOutIndex = uint32(pResult - m_pElements);

    return true;
  }

  template<typename key_t>
  const T* BinarySearchKey(const key_t& searchKey,
                           int (*SearchCompareFunction)(const key_t* pSearchKey, const T* pElement)) const
  {
    if (m_size == 0)
      return NULL;

    return Y_bsearchT(&searchKey, m_pElements, m_size, SearchCompareFunction);
  }

  template<typename key_t>
  T* BinarySearchKey(const key_t& searchKey, int (*SearchCompareFunction)(const key_t* pSearchKey, const T* pElement))
  {
    if (m_size == 0)
      return NULL;

    return const_cast<T*>(Y_bsearchT(&searchKey, m_pElements, m_size, SearchCompareFunction));
  }

  // linear search
  int32 IndexOf(const T& item) const
  {
    for (uint32 i = 0; i < m_size; i++)
    {
      if (m_pElements[i] == item)
        return (int32)i;
    }
    return -1;
  }

  // exists in array?
  bool Contains(const T& item) const { return (IndexOf(item) >= 0); }
  bool IsEmpty() const { return (m_size == 0); }

  // slow! insert at a specific position of the array
  void Insert(const T& item, uint32 index)
  {
    DebugAssert(index <= m_size);
    if (index == m_size)
    {
      // inserting at the end, fast path
      Add(item);
      return;
    }

    // ensure we have room
    if (m_size == m_reserve)
      Reserve(Max(m_size + 1, m_size * 2));

    // move everything forwards
    uint32 countAfter = m_size - index;
    std::memmove(m_pElements + index + 1, m_pElements + index, sizeof(T) * countAfter);

    // insert into place
    m_pElements[index] = item;
  }

  // zero everything
  void ZeroContents()
  {
    if (m_pElements != nullptr)
      Y_memzero(m_pElements, sizeof(T) * m_reserve);
  }

  // ensure space exists for the next n elements
  void EnsureReserve(uint32 n)
  {
    if ((m_size + n) > m_reserve)
      Reserve(Max(m_size + n, m_size * 2));
  }

  // pointer readers, use with care
  const T* GetBasePointer() const { return m_pElements; }
  T* GetBasePointer() { return m_pElements; }

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
  T& GetElement(uint32 i)
  {
    DebugAssert(i < m_size);
    return m_pElements[i];
  }

  // assignment operator
  PODArray<T>& operator=(const PODArray<T>& c)
  {
    Assign(c);
    return *this;
  }
  PODArray<T>& operator=(PODArray<T>&& c)
  {
    Swap(c);
    return *this;
  }

  // operator wrappers
  bool operator==(const PODArray<T>& c) const { return Equals(c); }
  bool operator!=(const PODArray<T>& c) const { return !Equals(c); }
  const T& operator[](uint32 index) const { return GetElement(index); }
  T& operator[](uint32 index) { return GetElement(index); }

protected:
  T* m_pElements;
  uint32 m_size;
  uint32 m_reserve;

public:
  class const_iterator
  {
    friend PODArray;

  public:
    const_iterator(const const_iterator& copy) : m_container(copy.m_container), m_index(copy.m_index) {}
    const const_iterator& operator=(const const_iterator& copy)
    {
      m_container = copy.m_container;
      m_index = copy.m_index;
    }

    // comparison operators
    bool operator==(const const_iterator& iter) { return (m_index == iter.m_index); }
    bool operator!=(const const_iterator& iter) { return (m_index != iter.m_index); }
    bool operator<(const const_iterator& iter) { return (m_index < iter.m_index); }
    bool operator>(const const_iterator& iter) { return (m_index > iter.m_index); }
    bool operator<=(const const_iterator& iter) { return (m_index <= iter.m_index); }
    bool operator>=(const const_iterator& iter) { return (m_index >= iter.m_index); }

    // increment operators
    const const_iterator& operator++()
    {
      m_index++;
      return *this;
    }
    const_iterator operator++(int)
    {
      const_iterator temp(*this);
      m_index++;
      return *this;
    }

    // dereference operators
    const T& operator*() const
    {
      DebugAssert(m_index < m_container.m_size);
      return m_container.m_pElements[m_index];
    }
    const T* operator->() const
    {
      DebugAssert(m_index < m_container.m_size);
      return &m_container.m_pElements[m_index];
    }

  private:
    const_iterator(const PODArray& container, uint32 index) : m_container(container), m_index(index) {}

    const PODArray& m_container;
    uint32 m_index;
  };

  class iterator
  {
    friend PODArray;

  public:
    iterator(const iterator& copy) : m_container(copy.m_container), m_index(copy.m_index) {}
    const iterator& operator=(const iterator& copy)
    {
      m_container = copy.m_container;
      m_index = copy.m_index;
    }

    // comparison operators
    bool operator==(const iterator& iter) { return (m_index == iter.m_index); }
    bool operator!=(const iterator& iter) { return (m_index != iter.m_index); }
    bool operator<(const iterator& iter) { return (m_index < iter.m_index); }
    bool operator>(const iterator& iter) { return (m_index > iter.m_index); }
    bool operator<=(const iterator& iter) { return (m_index <= iter.m_index); }
    bool operator>=(const iterator& iter) { return (m_index >= iter.m_index); }

    // increment operators
    const iterator& operator++()
    {
      m_index++;
      return *this;
    }
    iterator operator++(int)
    {
      iterator temp(*this);
      m_index++;
      return *this;
    }

    // dereference operators
    T& operator*() const
    {
      DebugAssert(m_index < m_container.m_size);
      return m_container.m_pElements[m_index];
    }
    T* operator->() const
    {
      DebugAssert(m_index < m_container.m_size);
      return &m_container.m_pElements[m_index];
    }

  private:
    iterator(PODArray& container, uint32 index) : m_container(container), m_index(index) {}

    PODArray& m_container;
    uint32 m_index;
  };

  // mainly for range-based for
#if 1
  const_iterator begin() const { return const_iterator(*this, 0); }
  const_iterator end() const { return const_iterator(*this, m_size); }
  iterator begin() { return iterator(*this, 0); }
  iterator end() { return iterator(*this, m_size); }
#elif 1
  const T* begin() const { return m_pElements; }
  const T* end() const { return (m_size > 0) ? &m_pElements[m_size] : m_pElements; }
  T* begin() { return m_pElements; }
  T* end() { return (m_size > 0) ? &m_pElements[m_size] : m_pElements; }
#endif
};
