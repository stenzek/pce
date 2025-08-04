#pragma once
#include "YBaseLib/ReferenceCounted.h"

//
// AutoReleasePtr
//

template<class T>
class AutoReleasePtr
{
public:
  AutoReleasePtr() : m_pPtr(NULL) {}
  AutoReleasePtr(const AutoReleasePtr<T>& Other) : m_pPtr(Other.m_pPtr)
  {
    if (m_pPtr != NULL)
    {
      m_pPtr->AddRef();
    }
  }
  AutoReleasePtr(T* pPtr) : m_pPtr(pPtr) {}
  ~AutoReleasePtr() { Release(); }

  const T& operator*() const { return *m_pPtr; }
  T& operator*() { return *m_pPtr; }
  const T* operator->() const { return m_pPtr; }
  T* operator->() { return m_pPtr; }
  operator const T*() const { return m_pPtr; }
  operator T*() { return m_pPtr; }

  bool operator==(const AutoReleasePtr<T>& Other) const { return (m_pPtr == Other.m_pPtr); }
  bool operator!=(const AutoReleasePtr<T>& Other) const { return (m_pPtr != Other.m_pPtr); }
  bool operator==(const T* pPtr) const { return (m_pPtr == pPtr); }
  bool operator!=(const T* pPtr) const { return (m_pPtr != pPtr); }

  AutoReleasePtr<T>& operator=(const AutoReleasePtr<T>& Other)
  {
    if (m_pPtr != NULL)
      m_pPtr->Release();

    if ((m_pPtr = Other.m_pPtr) != NULL)
      m_pPtr->AddRef();

    return *this;
  }

  AutoReleasePtr<T>& operator=(T* pPtr)
  {
    if (m_pPtr != NULL)
      m_pPtr->Release();

    m_pPtr = pPtr;
    return *this;
  }

  bool IsNull() const { return (m_pPtr != NULL); }

  void Release()
  {
    if (m_pPtr != NULL)
    {
      m_pPtr->Release();
      m_pPtr = NULL;
    }
  }

private:
  T* m_pPtr;
};
