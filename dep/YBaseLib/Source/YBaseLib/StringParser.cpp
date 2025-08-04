#include "YBaseLib/StringParser.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/String.h"

// should only be sizeof(ptr), so it can be used as an array
YStaticCheckSizeOf(StringParser, sizeof(const char*));

StringParser::StringParser(const char* szValue) : m_szValue(szValue) {}

StringParser::StringParser(const String& strValue) : m_szValue(strValue.GetCharArray()) {}

bool StringParser::ParseBool() const
{
  return (m_szValue != NULL) ? Y_strtobool(m_szValue, NULL) : false;
}

float StringParser::ParseFloat() const
{
  return (m_szValue != NULL) ? Y_strtofloat(m_szValue, NULL) : 0.0f;
}

double StringParser::ParseDouble() const
{
  return (m_szValue != NULL) ? Y_strtodouble(m_szValue, NULL) : 0.0;
}

int8 StringParser::ParseInt8() const
{
  return (m_szValue != NULL) ? Y_strtoint8(m_szValue, NULL) : 0;
}

int16 StringParser::ParseInt16() const
{
  return (m_szValue != NULL) ? Y_strtoint16(m_szValue, NULL) : 0;
}

int32 StringParser::ParseInt32() const
{
  return (m_szValue != NULL) ? Y_strtoint32(m_szValue, NULL) : 0;
}

int64 StringParser::ParseInt64() const
{
  return (m_szValue != NULL) ? Y_strtoint64(m_szValue, NULL) : 0;
}

uint8 StringParser::ParseUInt8() const
{
  return (m_szValue != NULL) ? Y_strtouint8(m_szValue, NULL) : 0;
}

uint16 StringParser::ParseUInt16() const
{
  return (m_szValue != NULL) ? Y_strtouint16(m_szValue, NULL) : 0;
}

uint32 StringParser::ParseUInt32() const
{
  return (m_szValue != NULL) ? Y_strtouint32(m_szValue, NULL) : 0;
}

uint64 StringParser::ParseUInt64() const
{
  return (m_szValue != NULL) ? Y_strtouint64(m_szValue, NULL) : 0;
}
