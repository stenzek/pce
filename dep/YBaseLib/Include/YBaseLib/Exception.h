#pragma once
#include "YBaseLib/Common.h"

#ifndef Y_PLATFORM_HTML5

class Exception
{
public:
  Exception(const char* SourceFileName, uint32 SourceLineNumber);
  virtual ~Exception();

  const char* GetSourceFileName() const { return m_szSourceFileName; }
  uint32 GetSourceLineNumber() const { return m_uSourceLineNumber; }

  virtual const char* GetName() const { return "Exception"; }
  virtual const char* GetDescription() const { return ""; }

private:
  const char* m_szSourceFileName;
  uint32 m_uSourceLineNumber;
};

#define DECLARE_EXCEPTION(Name)                                                                                        \
  \
public:                                                                                                                \
  const char* GetTypeName() const { return #Name; }

#define Y_THROW(ExceptionClass, ...) throw ExceptionClass(__FILE__, __LINE__, __VA_ARGS__)

#endif // Y_PLATFORM_HTML5
