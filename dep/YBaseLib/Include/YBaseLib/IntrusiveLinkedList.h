#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/NonCopyable.h"

// List implements a double-linked list of type T.
template<class T>
class IntrusiveLinkedList : private NonCopyable
{
public:
  // Node type
  struct Node
  {
    T data;
    Node* next;
    Node* prev;
  };

  inline void SetNode(Node* n, const T& value) { n->data = value; }

  // deallocates a node from the list
  inline void ClearNode(Node* n)
  {
    if (n->prev != NULL)
      n->prev->next = n->next;
    if (n->next != NULL)
      n->next->prev = n->prev;
    if (m_head == n)
      m_head = n->next;
    if (m_tail == n)
      m_tail = n->prev;

    m_size--;
  }

public:
  // forward declare types
  class Iterator;
  class ConstIterator;

  // Iterator type
  class Iterator
  {
    friend class IntrusiveLinkedList;

  public:
    Iterator(Node* node) : m_node(node) {}
    Iterator(const Iterator& c) : m_node(c.m_node) {}
    Iterator(const ConstIterator& c) : m_node(c.m_node) {}

    inline void Forward() { m_node = m_node->next; }
    inline void Back() { m_node = m_node->prev; }
    inline bool AtEnd() { return (m_node == NULL); }

    inline bool operator==(const ConstIterator& rhs) { return (m_node == rhs.m_node); }
    inline bool operator==(const Iterator& rhs) { return (m_node == rhs.m_node); }
    inline bool operator!=(const ConstIterator& rhs) { return (m_node != rhs.m_node); }
    inline bool operator!=(const Iterator& rhs) { return (m_node != rhs.m_node); }

    inline Iterator& operator=(const Iterator& rhs)
    {
      m_node = rhs.m_node;
      return *this;
    }
    inline Iterator& operator=(const ConstIterator& rhs)
    {
      m_node = rhs.m_node;
      return *this;
    }

    // pre-increment/decrement
    inline Iterator& operator++()
    {
      Forward();
      return *this;
    }
    inline Iterator& operator--()
    {
      Back();
      return *this;
    }

    // post-increment/decrement
    Iterator operator++(int)
    {
      Iterator i(m_node);
      this->Forward();
      return i;
    }
    Iterator operator--(int)
    {
      Iterator i(m_node);
      this->Back();
      return i;
    }

    // dereference
    inline T& operator*() { return *m_node->data; }
    inline T* operator->() { return m_node->data; }

  private:
    // pointer to current node of iterator
    Node* m_node;
  };

  // Const iterator type.
  class ConstIterator
  {
    friend class IntrusiveLinkedList;

  public:
    ConstIterator(Node* node) : m_node(node) {}
    ConstIterator(const Iterator& c) : m_node(c.m_node) {}
    ConstIterator(const ConstIterator& c) : m_node(c.m_node) {}

    inline void Forward() { m_node = m_node->next; }
    inline void Back() { m_node = m_node->prev; }
    inline bool AtEnd() { return (m_node == NULL); }

    inline bool operator==(const ConstIterator& rhs) { return (m_node == rhs.m_node); }
    inline bool operator==(const Iterator& rhs) { return (m_node == rhs.m_node); }
    inline bool operator!=(const ConstIterator& rhs) { return (m_node != rhs.m_node); }
    inline bool operator!=(const Iterator& rhs) { return (m_node != rhs.m_node); }

    inline ConstIterator& operator=(const Iterator& rhs)
    {
      m_node = rhs.m_node;
      return *this;
    }
    inline ConstIterator& operator=(const ConstIterator& rhs)
    {
      m_node = rhs.m_node;
      return *this;
    }

    // pre-increment/decrement
    inline ConstIterator& operator++()
    {
      Forward();
      return *this;
    }
    inline ConstIterator& operator--()
    {
      Back();
      return *this;
    }

    // post-increment/decrement
    ConstIterator operator++(int)
    {
      Iterator i(m_node);
      this->Forward();
      return i;
    }
    ConstIterator operator--(int)
    {
      Iterator i(m_node);
      this->Back();
      return i;
    }

    // dereference
    inline T& operator*() { return *m_node->data; }
    inline T* operator->() { return m_node->data; }

  private:
    // pointer to current node of iterator
    Node* m_node;
  };

public:
  IntrusiveLinkedList()
  {
    m_head = NULL;
    m_tail = NULL;
    m_size = 0;
  }

  ~IntrusiveLinkedList(){DebugAssert(m_size > 0)}

  // returns an iterator pointing to the head of the list.
  Iterator Begin()
  {
    return Iterator(m_head);
  }
  ConstIterator Begin() const { return ConstIterator(m_head); }

  // returns an iterator pointing to the back of the list.
  Iterator Back() { return Iterator(m_tail); }
  ConstIterator Back() const { return ConstIterator(m_tail); }

  // returns an iterator at the end of the list [whether it be forward or backwards-incrementing]
  Iterator End() { return Iterator(NULL); }
  ConstIterator End() const { return ConstIterator(NULL); }

  // adds an item to the front of the list
  void PushFront(Node* n, const T& item)
  {
    SetNode(n, item);
    if (m_head == NULL)
    {
      n->next = NULL;
      n->prev = NULL;
      m_head = m_tail = n;
    }
    else
    {
      n->next = m_head;
      n->prev = NULL;
      m_head->prev = n;
      m_head = n;
    }

    m_size++;
  }

  // adds an item to the back of the list
  void PushBack(Node* n, const T& item)
  {
    SetNode(n, item);
    if (m_head == NULL)
    {
      n->next = NULL;
      n->prev = NULL;
      m_head = m_tail = n;
    }
    else
    {
      n->next = NULL;
      n->prev = m_tail;
      m_tail->next = n;
      m_tail = n;
    }

    m_size++;
  }

  // adds an item to the list, and returns an iterator to it
  Iterator PushFrontIterator(const T& item)
  {
    PushFront(item);
    return Iterator(m_head);
  }
  Iterator PushBackIterator(const T& item)
  {
    PushBack(item);
    return Iterator(m_tail);
  }

  // erases a certain spot in the list
  void Erase(Iterator& i) { DeallocNode(i.m_node); }
  void Erase(ConstIterator& i) { DeallocNode(i.m_node); }
  void Erase(Node* n) { ClearNode(n); }

  // erases a certain spot in the list, and returns an iterator pointing to the element after it
  Iterator EraseIterator(Iterator& i)
  {
    Iterator r(i.m_node->next);
    DeallocNode(i.m_node);
    return r;
  }
  ConstIterator EraseIterator(ConstIterator& i)
  {
    ConstIterator r(i.m_node->next);
    DeallocNode(i.m_node);
    return r;
  }

  // get the current size of the list
  inline size_t GetSize() const { return m_size; }

private:
  // list head/tail
  Node* m_head;
  Node* m_tail;

  // count of list nodes
  size_t m_size;
};
