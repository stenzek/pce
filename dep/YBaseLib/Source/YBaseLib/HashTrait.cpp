#include "YBaseLib/HashTrait.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/String.h"

HashType HashTrait<uint8>::GetHash(const uint8 Value)
{
  return (HashType)Value;
}

HashType HashTrait<uint16>::GetHash(const uint16 Value)
{
  return (HashType)Value;
}

HashType HashTrait<uint32>::GetHash(const uint32 Value)
{
  return (HashType)Value;
}

HashType HashTrait<uint64>::GetHash(const uint64 Value)
{
  return (HashType)((uint32)(Value & 0xffffffff) ^ ((uint32)(Value >> 32)));
}

HashType HashTrait<int8>::GetHash(const int8 Value)
{
  return (HashType)Value;
}

HashType HashTrait<int16>::GetHash(const int16 Value)
{
  return (HashType)Value;
}

HashType HashTrait<int32>::GetHash(const int32 Value)
{
  return (HashType)Value;
}

HashType HashTrait<int64>::GetHash(const int64 Value)
{
  return (HashType)((int32)(Value & 0xffffffff) ^ ((int32)(Value >> 32)));
}

HashType HashTrait<String>::GetHash(const String& Value)
{
  // http://en.wikipedia.org/wiki/Jenkins_hash_function
  uint32 hash = 0;
  uint32 i = 0;
  uint32 len = Value.GetLength();
  const char* pStr = Value.GetCharArray();
  for (; i < len; i++)
  {
    hash += pStr[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }

  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return (HashType)hash;
}

/*
HashType HashTrait<char *>::GetHash(const char *Value)
{
    // http://en.wikipedia.org/wiki/Jenkins_hash_function
    uint32 hash = 0;
    uint32 i = 0;
    uint32 len = Y_strlen(Value);
    for (; i < len; i++)
    {
        hash += Value[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return (HashType)hash;
}
*/
