#pragma once
#include "YBaseLib/Common.h"

class String;

// this class can be casted from a string or array of strings to provide conversion
class StringParser
{
public:
  StringParser(const char* szValue);
  StringParser(const String& strValue);

  bool ParseBool() const;
  float ParseFloat() const;
  double ParseDouble() const;
  int8 ParseInt8() const;
  int16 ParseInt16() const;
  int32 ParseInt32() const;
  int64 ParseInt64() const;
  uint8 ParseUInt8() const;
  uint16 ParseUInt16() const;
  uint32 ParseUInt32() const;
  uint64 ParseUInt64() const;

private:
  const char* m_szValue;
};
