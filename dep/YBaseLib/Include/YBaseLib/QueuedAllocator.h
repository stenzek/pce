#pragma once
#include "YBaseLib/Assert.h"
#include "YBaseLib/Common.h"

template<class T>
class QueuedAllocator
{
  DeclareNonCopyable(QueuedAllocator);

  struct Node
  {
    T Obj;
    Node* pPrev;
    Node* pNext;
  };

public:
  QueuedAllocator();
  ~QueuedAllocator();

  const size_t GetFreeListSize() const { return m_freeListSize; }
  const size_t GetAllocatedListSize() const { return m_allocatedListSize; }

  T* Allocate();
  T* Allocate(const T& copy);
  T* Allocate(const T&& move);

  void Free(T* pObject);

private:
  Node* AllocateNode();

  Node* m_pFreeListHead;
  Node* m_pFreeListTail;
  Node* m_pAllocatedListHead;
  Node* m_pAllocatedListTail;
  size_t m_freeListSize;
  size_t m_allocatedListSize;
};

template<class T>
void QueuedAllocator<T>::QueuedAllocator()
  : m_pFreeListHead(nullptr), m_pFreeListTail(nullptr), m_pAllocatedListHead(nullptr), m_pAllocatedListTail(nullptr),
    m_freeListSize(0), m_allocatedListSize(0)
{
}

template<class T>
QueuedAllocator<T>::~QueuedAllocator()
{
  Assert(m_allocatedListSize == 0);

  Node* pNode = m_pFreeListHead;
  while (pNode != nullptr)
  {
    Node* pTemp = pNode;
    pNode = pNode->pNext;
    delete pTemp;
  }
}

template<class T>
T* QueuedAllocator<T>::Allocate()
{
  Node* pNode = AllocateNode();
  new (&pNode->Obj) T();
  return &pNode->Obj;
}

template<class T>
T* QueuedAllocator<T>::Allocate(const T& copy)
{
  Node* pNode = AllocateNode();
  new (&pNode->Obj) T(copy);
  return &pNode->Obj;
}

template<class T>
T* QueuedAllocator<T>::Allocate(const T&& move)
{
  Node* pNode = AllocateNode();
  new (&pNode->Obj) T(move);
  return &pNode->Obj;
}

template<class T>
QueuedAllocator<T>::Node* QueuedAllocator<T>::AllocateNode()
{
  Node* pNode;
  if (m_pFreeListHead != nullptr)
  {
    pNode = m_pFreeListHead;
    if (pNode->pNext != nullptr)
    {
      pNode->pNext->pPrev = nullptr;
      m_pFreeListHead = pNode->pNext;
    }
    else
    {
      m_pFreeListHead = nullptr;
      m_pFreeListTail = nullptr;
    }
    m_freeListSize--;
  }
  else
  {
    pNode = new Node();
  }

  pNode->pPrev = m_pAllocatedListTail;
  pNode->pNext = nullptr;
  if (m_pAllocatedListTail == nullptr)
    m_pAllocatedListHead = m_pAllocatedListTail = pNode;
  else
    m_pAllocatedListTail = pNode;

  m_allocatedListSize++;
  return pNode;
}

template<class T>
void QueuedAllocator<T>::Free(T* pObject)
{
  Node* pNode = CONTAINING_STRUCTURE(pObject, Node, Obj);
  pNode->Obj.~T();

  if (pNode->pNext == nullptr)
  {
    DebugAssert(m_pAllocatedListTail == pNode);
    m_pAllocatedListTail = pNode->pPrev;
    if (pNode->pPrev != nullptr)
      pNode->pPrev->pNext = nullptr;
  }
  else
  {
    pNode->pNext->pPrev = pNode->pPrev;
  }
  if (pNode->pPrev == nullptr)
  {
    DebugAssert(m_pAllocatedListHead == pNode);
    m_pAllocatedListHead = pNode->pNext;
    if (pNode->pNext != nullptr)
      pNode->pNext->pPrev = pNode->pNext;
  }
  m_allocatedListSize--;

  pNode->pPrev = m_pFreeListTail;
  pNode->pNext = nullptr;
  if (m_pFreeListTail == nullptr)
    m_pFreeListHead = m_pFreeListTail = pNode;
  else
    m_pFreeListTail = pNode;

  m_freeListSize++;
}
