#include "YBaseLib/ReferenceCounted.h"
#include "YBaseLib/Assert.h"

static const uint32 ReferenceCountValidValue = 0x11C0FFEE;
static const uint32 ReferenceCountInvalidValue = 0xDEADC0DE;

ReferenceCounted::ReferenceCounted()
{
  m_uReferenceCount = 1;
  m_uReferenceCountValid = ReferenceCountValidValue;
}

ReferenceCounted::~ReferenceCounted()
{
  // TODO: fix this handling with exception throwing, maybe with tls bool of exception being thrown
  DebugAssert(m_uReferenceCount == 0);
  m_uReferenceCountValid = ReferenceCountInvalidValue;
}

void ReferenceCounted::AddRef() const
{
  Y_AtomicIncrement(m_uReferenceCount);
}

uint32 ReferenceCounted::Release() const
{
  DebugAssert(m_uReferenceCount > 0 && m_uReferenceCountValid == ReferenceCountValidValue);
  uint32 NewRefCount = Y_AtomicDecrement(m_uReferenceCount);
  if (NewRefCount == 0)
    delete const_cast<ReferenceCounted*>(this);

  return NewRefCount;
}
