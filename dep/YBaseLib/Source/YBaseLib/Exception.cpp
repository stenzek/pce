#include "YBaseLib/Exception.h"

#ifndef Y_PLATFORM_HTML5

Exception::Exception(const char* SourceFileName, uint32 SourceLineNumber)
  : m_szSourceFileName(SourceFileName), m_uSourceLineNumber(SourceLineNumber)
{
}

Exception::~Exception() {}

#endif // Y_PLATFORM_HTML5
