#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/String.h"

typedef uint32 HashType;

template<typename KEYTYPE>
struct HashTrait
{
};

// built-ins
#define DECLARE_HASHTRAIT_BYVAL(type)                                                                                  \
  template<>                                                                                                           \
  struct HashTrait<type>                                                                                               \
  {                                                                                                                    \
    static HashType GetHash(const type Value);                                                                         \
  };
#define DECLARE_HASHTRAIT_BYREF(type)                                                                                  \
  template<>                                                                                                           \
  struct HashTrait<type>                                                                                               \
  {                                                                                                                    \
    static HashType GetHash(const type& Value);                                                                        \
  };

DECLARE_HASHTRAIT_BYVAL(uint8);
DECLARE_HASHTRAIT_BYVAL(uint16);
DECLARE_HASHTRAIT_BYVAL(uint32);
DECLARE_HASHTRAIT_BYVAL(uint64);
DECLARE_HASHTRAIT_BYVAL(int8);
DECLARE_HASHTRAIT_BYVAL(int16);
DECLARE_HASHTRAIT_BYVAL(int32);
DECLARE_HASHTRAIT_BYVAL(int64);

class String;
DECLARE_HASHTRAIT_BYREF(String);
// DECLARE_HASHTRAIT_BYVAL(char *);

// built-in hash function for a pointer type
template<typename KEYTYPE>
struct HashTrait<KEYTYPE*>
{
  static HashType GetHash(const KEYTYPE* Value) { return (HashType)Value; }
};
