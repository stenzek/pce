#pragma once
#include "YBaseLib/Assert.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/HashTrait.h"
#include "YBaseLib/Memory.h"
#include <new>

template<typename ValueType>
class CIStringHashTable
{
public:
  struct Member
  {
    char* Key;
    uint32 KeyLength;
    ValueType Value;
    HashType Hash;

    Member* PrevMember;
    Member* NextMember;
  };

  class Iterator;
  class ConstIterator;

  class Iterator
  {
    friend class ConstIterator;

  public:
    Iterator() : m_pCurrentMember(NULL) {}
    Iterator(Member* pCurrentMember) : m_pCurrentMember(pCurrentMember) {}
    Iterator(const Iterator& c) : m_pCurrentMember(c.m_pCurrentMember) {}
    Iterator(const ConstIterator& c) : m_pCurrentMember(c.m_pCurrentMember) {}

    inline void Forward() { m_pCurrentMember = m_pCurrentMember->NextMember; }
    inline void Back() { m_pCurrentMember = m_pCurrentMember->PrevMember; }
    inline bool AtEnd() { return (m_pCurrentMember == NULL); }

    inline bool operator==(const ConstIterator& rhs) { return (m_pCurrentMember == rhs.m_pCurrentMember); }
    inline bool operator==(const Iterator& rhs) { return (m_pCurrentMember == rhs.m_pCurrentMember); }
    inline bool operator!=(const ConstIterator& rhs) { return (m_pCurrentMember != rhs.m_pCurrentMember); }
    inline bool operator!=(const Iterator& rhs) { return (m_pCurrentMember != rhs.m_pCurrentMember); }

    inline Iterator& operator=(const Iterator& rhs)
    {
      m_pCurrentMember = rhs.m_pCurrentMember;
      return *this;
    }
    inline Iterator& operator=(const ConstIterator& rhs)
    {
      m_pCurrentMember = rhs.m_pCurrentMember;
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
      Iterator i(m_pCurrentMember);
      this->Forward();
      return i;
    }
    Iterator operator--(int)
    {
      Iterator i(m_pCurrentMember);
      this->Back();
      return i;
    }

    // dereference
    inline Member& operator*() { return *m_pCurrentMember; }
    inline Member* operator->() { return m_pCurrentMember; }

  private:
    // pointer to current node of iterator
    Member* m_pCurrentMember;
  };

  class ConstIterator
  {
    friend class Iterator;

  public:
    ConstIterator() : m_pCurrentMember(NULL) {}
    ConstIterator(Member* pCurrentMember) : m_pCurrentMember(pCurrentMember) {}
    ConstIterator(const Iterator& c) : m_pCurrentMember(c.m_pCurrentMember) {}
    ConstIterator(const ConstIterator& c) : m_pCurrentMember(c.m_pCurrentMember) {}

    inline void Forward() { m_pCurrentMember = m_pCurrentMember->NextMember; }
    inline void Back() { m_pCurrentMember = m_pCurrentMember->PrevMember; }
    inline bool AtEnd() { return (m_pCurrentMember == NULL); }

    inline bool operator==(const ConstIterator& rhs) { return (m_pCurrentMember == rhs.m_pCurrentMember); }
    inline bool operator==(const Iterator& rhs) { return (m_pCurrentMember == rhs.m_pCurrentMember); }
    inline bool operator!=(const ConstIterator& rhs) { return (m_pCurrentMember != rhs.m_pCurrentMember); }
    inline bool operator!=(const Iterator& rhs) { return (m_pCurrentMember != rhs.m_pCurrentMember); }

    inline Iterator& operator=(const Iterator& rhs)
    {
      m_pCurrentMember = rhs.m_pCurrentMember;
      return *this;
    }
    inline Iterator& operator=(const ConstIterator& rhs)
    {
      m_pCurrentMember = rhs.m_pCurrentMember;
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
      Iterator i(m_pCurrentMember);
      this->Forward();
      return i;
    }
    Iterator operator--(int)
    {
      Iterator i(m_pCurrentMember);
      this->Back();
      return i;
    }

    // dereference
    inline const Member& operator*() { return *m_pCurrentMember; }
    inline const Member* operator->() { return m_pCurrentMember; }

  private:
    // pointer to current node of iterator
    Member* m_pCurrentMember;
  };

  static HashType GetStringHash(const char* Str, uint32 Length)
  {
    // http://en.wikipedia.org/wiki/Jenkins_hash_function
    uint32 hash = 0;
    uint32 i = 0;
    for (; i < Length; i++)
    {
      hash += Y_tolower(Str[i]);
      hash += (hash << 10);
      hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return (HashType)hash;
  }

  CIStringHashTable(uint32 nBuckets = 4, uint32 uBucketSize = 16)
  {
    m_pBuckets = NULL;
    m_nBuckets = 0;
    m_pFirstMember = m_pLastMember = NULL;
    m_nMembers = 0;
    InitBuckets(nBuckets, uBucketSize);
  }

  CIStringHashTable(const CIStringHashTable& Copy)
  {
    // init to same bucket size/count as copy
    m_pBuckets = NULL;
    m_nBuckets = 0;
    m_pFirstMember = m_pLastMember = NULL;
    m_nMembers = 0;
    InitBuckets(Copy.m_nBuckets, Copy.m_uBucketSize);

    // insert all its members
    Member* member = Copy.m_pFirstMember;
    while (member != NULL)
      Insert(member->Key, member->Value);
  }

  ~CIStringHashTable()
  {
    FreeBuckets();
    FreeMembers();
  }

  uint32 GetMemberCount() const { return m_nMembers; }

  Member* Insert(const char* Key, const ValueType& Value)
  {
    bool IsNew;
    Member* pMember = _Insert(Key, Value, &IsNew);
    AssertMsg(IsNew, "Attempting to insert an already-existing key to hash table.");
    return pMember;
  }

  Member* Set(const char* Key, const ValueType& Value, bool* IsNew)
  {
    bool IsNew_;
    Member* pMember = _Insert(Key, Value, &IsNew_);
    if (!IsNew_)
    {
      // overwrite member value
      pMember->Value = Value;
    }

    if (IsNew != NULL)
      *IsNew = IsNew_;

    return pMember;
  }

  Member* Find(const char* Key)
  {
    uint32 KeyLength = Y_strlen(Key);
    HashType Hash = GetStringHash(Key, KeyLength);
    return m_pBuckets[Hash % m_nBuckets].Find(Key, KeyLength, Hash);
  }

  const Member* Find(const char* Key) const
  {
    uint32 KeyLength = Y_strlen(Key);
    HashType Hash = GetStringHash(Key, KeyLength);
    return (const Member*)const_cast<Bucket&>(m_pBuckets[Hash % m_nBuckets]).Find(Key, KeyLength, Hash);
  }

  bool Remove(const char* Key)
  {
    Member* pMember = Find(Key);
    if (pMember != NULL)
    {
      Remove(pMember);
      return true;
    }
    else
    {
      return false;
    }
  }

  void Remove(Member* pMember)
  {
    DebugAssert(pMember != NULL);

    // determine bucket member is associated with
    Bucket* bucket = &m_pBuckets[pMember->Hash % m_nBuckets];
    bool r = bucket->Remove(pMember);
    AssertMsg(r, "HashTable corrupted.");

    if (pMember->PrevMember != NULL)
      pMember->PrevMember->NextMember = pMember->NextMember;
    if (pMember->NextMember != NULL)
      pMember->NextMember->PrevMember = pMember->PrevMember;
    if (pMember == m_pFirstMember)
      m_pFirstMember = pMember->NextMember;
    if (pMember == m_pLastMember)
      m_pLastMember = pMember->PrevMember;

    // free the member itself
    FreeMember(pMember);
    m_nMembers--;
  }

  void Clear()
  {
    Member* pNext = m_pFirstMember;
    while (pNext != NULL)
    {
      Member* pCur = pNext;
      pNext = pCur->NextMember;
      Remove(pCur);
    }
  }

  // operators
  CIStringHashTable& operator=(const CIStringHashTable& Assign)
  {
    FreeMembers();
    FreeBuckets();
    InitBuckets(Assign.m_nBuckets, Assign.m_uBucketSize);

    for (Member* member = Assign.m_pFirstMember; member != NULL; member = member->NextMember)
      Insert(member->Key, member->Value);

    return *this;
  }

  // returns an iterator pointing to the head of the list.
  Iterator Begin() { return Iterator(m_pFirstMember); }
  ConstIterator Begin() const { return ConstIterator(m_pFirstMember); }

  // returns an iterator pointing to the back of the list.
  Iterator Back() { return Iterator(m_pLastMember); }
  ConstIterator Back() const { return ConstIterator(m_pLastMember); }

  // returns an iterator at the end of the list [whether it be forward or backwards-incrementing]
  Iterator End() { return Iterator(NULL); }
  ConstIterator End() const { return ConstIterator(NULL); }

private:
  // disable equality operators
  bool operator==(const CIStringHashTable& Comp);
  bool operator!=(const CIStringHashTable& Comp);

private:
  // TODO improve allocation, allocate buckets + bucket members in one shot
  class Bucket
  {
  public:
    void Init(uint32 uBucketSize)
    {
      // calloc automatically nulls
      m_pMembers = (Member**)calloc(uBucketSize, sizeof(Member*));
      m_uBucketSize = uBucketSize;
    }

    void Free() { free(m_pMembers); }

    bool Insert(Member* pMember)
    {
      // try for best position
      uint32 hpos = pMember->Hash % m_uBucketSize;
      uint32 pos = hpos;
      if (m_pMembers[pos] != NULL)
      {
        // search for another position
        for (;;)
        {
          pos = (pos + 1) % m_uBucketSize;
          if (pos == hpos)
          {
            // looped around
            return false;
          }

          if (m_pMembers[pos] == NULL)
            break;
        }
      }

      // set in list
      m_pMembers[pos] = pMember;
      return true;
    }

    Member* Find(const char* Key, uint32 KeyLength, HashType Hash)
    {
      // try best position
      uint32 hpos = Hash % m_uBucketSize;
      uint32 pos = hpos;
      Member* member = m_pMembers[pos];
      if (member != NULL && member->Hash == Hash && KeyLength == member->KeyLength && Y_stricmp(Key, member->Key) == 0)
        return member;

      for (;;)
      {
        pos = (pos + 1) % m_uBucketSize;
        if (pos == hpos)
        {
          // looped around
          return NULL;
        }

        member = m_pMembers[pos];
        if (member != NULL && member->Hash == Hash && Y_stricmp(Key, member->Key) == 0)
          return m_pMembers[pos];
      }
    }

    bool Remove(Member* pMember)
    {
      // try best position
      uint32 pos = pMember->Hash % m_uBucketSize;
      if (m_pMembers[pos] == pMember)
      {
        m_pMembers[pos] = NULL;
        return true;
      }

      for (;;)
      {
        pos = (pos + 1) % m_uBucketSize;
        if (pos == m_uBucketSize)
          return false;

        if (m_pMembers[pos] == pMember)
        {
          m_pMembers[pos] = NULL;
          return true;
        }
      }
    }

  public:
    Member** m_pMembers;
    uint32 m_uBucketSize;
  };

  void InitBuckets(uint32 nBuckets, uint32 uBucketSize)
  {
    DebugAssert(nBuckets > 0);
    DebugAssert(uBucketSize > 0);

    m_pBuckets = (Bucket*)malloc(sizeof(Bucket) * nBuckets);
    m_nBuckets = nBuckets;
    m_uBucketSize = uBucketSize;
    for (uint32 i = 0; i < nBuckets; i++)
      m_pBuckets[i].Init(uBucketSize);
  }

  void FreeBuckets()
  {
    if (m_pBuckets != NULL)
    {
      for (uint32 i = 0; i < m_nBuckets; i++)
        m_pBuckets[i].Free();
    }

    free(m_pBuckets);
    m_pBuckets = NULL;
    m_nBuckets = 0;
    m_uBucketSize = 0;
  }

  void FreeMember(Member* member)
  {
    // invoke destructors
    member->Value.~ValueType();

    // free memory
    std::free(member->Key);
    std::free(member);
  }

  void FreeMembers()
  {
    Member* cur = m_pFirstMember;
    while (cur != NULL)
    {
      // store next
      Member* next = cur->NextMember;
      FreeMember(cur);

      // update cur
      cur = next;
      m_nMembers++;
    }

    m_pFirstMember = m_pLastMember = NULL;
    m_nMembers = 0;
  }

  void AutoResize()
  {
    // basically, we double the bucket size until it's the number of buckets * 8, then
    // we multiply the number of buckets by 2
    uint32 newBuckets;
    uint32 newBucketSize;
    if ((m_nBuckets * 8) > m_uBucketSize)
    {
      // grow bucket size
      newBuckets = m_nBuckets;
      newBucketSize = m_uBucketSize * 2;
    }
    else
    {
      // grow bucket count
      newBuckets = m_nBuckets * 2;
      newBucketSize = m_uBucketSize;
    }

    // do the resize
    Resize(newBuckets, newBucketSize);
  }

  void Resize(uint32 newBuckets, uint32 newBucketSize)
  {
    DebugAssert(newBuckets > 0);
    DebugAssert(newBucketSize > 0);
    DebugAssert((newBuckets * newBucketSize) >= m_nMembers);

    FreeBuckets();
    InitBuckets(newBuckets, newBucketSize);

    // re-insert all members
    for (Member* member = m_pFirstMember; member != NULL; member = member->NextMember)
    {
      bool r = m_pBuckets[member->Hash % m_nBuckets].Insert(member);
      DebugAssert(r);
      UNREFERENCED_PARAMETER(r);
    }
  }

  Member* _Insert(const char* Key, const ValueType& Value, bool* IsNew)
  {
    // hash the key
    uint32 KeyLength = Y_strlen(Key);
    HashType Hash = GetStringHash(Key, KeyLength);

    // look up
    Member* pMember = m_pBuckets[Hash % m_nBuckets].Find(Key, KeyLength, Hash);
    if (pMember != NULL)
    {
      *IsNew = false;
      return pMember;
    }

    // create member
    pMember = (Member*)malloc(sizeof(Member));
    pMember->KeyLength = KeyLength;
    pMember->Key = Y_mallocT<char>(pMember->KeyLength + 1);
    for (uint32 i = 0; i < KeyLength; i++)
      pMember->Key[i] = Key[i]; // Y_tolower(Key[i]);
    pMember->Key[KeyLength] = '\0';

    new (&pMember->Value) ValueType(Value);
    pMember->Hash = Hash;

    // insert into bucket
    for (;;)
    {
      if (m_pBuckets[Hash % m_nBuckets].Insert(pMember))
        break;

      AutoResize();
    }

    // insert into member chain
    if (m_pLastMember != NULL)
    {
      pMember->PrevMember = m_pLastMember;
      pMember->NextMember = NULL;
      m_pLastMember->NextMember = pMember;
      m_pLastMember = pMember;
    }
    else
    {
      m_pFirstMember = m_pLastMember = pMember;
      pMember->PrevMember = pMember->NextMember = NULL;
    }

    m_nMembers++;
    *IsNew = true;
    return pMember;
  }

  Member* m_pFirstMember;
  Member* m_pLastMember;
  uint32 m_nMembers;
  Bucket* m_pBuckets;
  uint32 m_nBuckets;
  uint32 m_uBucketSize;
};
