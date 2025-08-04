#pragma once
#include "YBaseLib/ReferenceCounted.h"

//
// ReferenceCountedHolder
//

template<class T>
class ReferenceCountedHolder
{
public:
  ReferenceCountedHolder() : m_pPtr(nullptr) {}
  ReferenceCountedHolder(const ReferenceCountedHolder<T>& Other) : m_pPtr(Other.m_pPtr)
  {
    if (m_pPtr != nullptr)
    {
      m_pPtr->AddRef();
    }
  }
  ReferenceCountedHolder(T* pPtr) : m_pPtr(pPtr)
  {
    if (m_pPtr != nullptr)
    {
      m_pPtr->AddRef();
    }
  }
  ~ReferenceCountedHolder() { Release(); }

  T& operator*() const { return *m_pPtr; }
  T* operator->() const { return m_pPtr; }
  operator T*() const { return m_pPtr; }

  bool operator==(const ReferenceCountedHolder<T>& Other) const { return (m_pPtr == Other.m_pPtr); }
  bool operator!=(const ReferenceCountedHolder<T>& Other) const { return (m_pPtr != Other.m_pPtr); }
  bool operator==(const T* pPtr) const { return (m_pPtr == pPtr); }
  bool operator!=(const T* pPtr) const { return (m_pPtr != pPtr); }

  ReferenceCountedHolder<T>& operator=(const ReferenceCountedHolder<T>& Other)
  {
    if (m_pPtr != nullptr)
      m_pPtr->Release();

    if ((m_pPtr = Other.m_pPtr) != nullptr)
      m_pPtr->AddRef();

    return *this;
  }

  ReferenceCountedHolder<T>& operator=(T* pPtr)
  {
    if (m_pPtr != nullptr)
      m_pPtr->Release();

    if ((m_pPtr = pPtr) != nullptr)
      m_pPtr->AddRef();

    return *this;
  }

  bool IsNull() const { return (m_pPtr != nullptr); }

  void Release()
  {
    if (m_pPtr != nullptr)
    {
      m_pPtr->Release();
      m_pPtr = nullptr;
    }
  }

  // move constructor
  ReferenceCountedHolder(ReferenceCountedHolder<T>&& Other)
  {
    // written in this order so that this = this has no effect
    T* ptr = Other.m_pPtr;
    Other.m_pPtr = nullptr;
    m_pPtr = ptr;
  }

  // move assignment
  ReferenceCountedHolder<T>& operator=(ReferenceCountedHolder<T>&& Other)
  {
    // written in this order so that this = this has no effect
    T* ptr = Other.m_pPtr;
    Other.m_pPtr = nullptr;
    m_pPtr = ptr;
  }

private:
  T* m_pPtr;
};
