#pragma once
#include "YBaseLib/Atomic.h"
#include "YBaseLib/Common.h"

// ReferenceCountedObject is NonCopyable.
class ReferenceCounted
{
  // we protect the constructor and destructor so they cannot be deleted from outside
protected:
  ReferenceCounted();
  virtual ~ReferenceCounted();

public:
  void AddRef() const;
  uint32 Release() const;

private:
  mutable Y_ATOMIC_DECL uint32 m_uReferenceCount;
  mutable uint32 m_uReferenceCountValid;

  // make noncopyable
private:
  ReferenceCounted(const ReferenceCounted&);
  ReferenceCounted& operator=(const ReferenceCounted&);
};

// Helper method to add/release a reference and return the same pointer
template<class T>
T AddRefAndReturn(T pPointer)
{
  pPointer->AddRef();
  return pPointer;
}
template<class T>
T ReleaseReferenceAndReturn(T pPointer)
{
  pPointer->Release();
  return pPointer;
}

#define SAFE_RELEASE(x)                                                                                                \
  MULTI_STATEMENT_MACRO_BEGIN                                                                                          \
  if ((x) != nullptr)                                                                                                  \
  {                                                                                                                    \
    (x)->Release();                                                                                                    \
    (x) = nullptr;                                                                                                     \
  }                                                                                                                    \
  MULTI_STATEMENT_MACRO_END

#define FAST_RELEASE(x)                                                                                                \
  MULTI_STATEMENT_MACRO_BEGIN { (x)->Release(); }                                                                      \
  MULTI_STATEMENT_MACRO_END

#define SAFE_RELEASE_LAST(x)                                                                                           \
  MULTI_STATEMENT_MACRO_BEGIN                                                                                          \
  if ((x) != nullptr)                                                                                                  \
  {                                                                                                                    \
    uint32 refCount = (x)->Release();                                                                                  \
    (x) = nullptr;                                                                                                     \
    if (refCount != 0)                                                                                                 \
    {                                                                                                                  \
      Panic("Expecting to release last reference to object " #x ", object is still alive.");                           \
    }                                                                                                                  \
  }                                                                                                                    \
  MULTI_STATEMENT_MACRO_END
