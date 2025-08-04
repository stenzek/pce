#include "YBaseLib/Error.h"
#include "YBaseLib/CString.h"

// Platform-specific includes
#if defined(Y_PLATFORM_WINDOWS)
#include "YBaseLib/Windows/WindowsHeaders.h"
static_assert(std::is_same<DWORD, unsigned long>::value, "DWORD is unsigned long");
static_assert(std::is_same<HRESULT, long>::value, "HRESULT is long");
#endif

#include <cstdlib>
#include <cstring>
#include <type_traits>

Error::Error() : m_type(ERROR_TYPE_NONE)
{
  m_error.none = 0;
}

Error::Error(const Error& c)
{
  m_type = c.m_type;
  std::memcpy(&m_error, &c.m_error, sizeof(m_error));
  m_codeString.AppendString(c.m_codeString);
  m_message.AppendString(c.m_message);
}

Error::~Error() {}

ERROR_TYPE Error::GetType() const
{
  return m_type;
}

int Error::GetErrorCodeErrno() const
{
  return m_error.errno_f;
}

int Error::GetErrorCodeSocket() const
{
  return m_error.socketerr;
}

int32 Error::GetErrorCodeUser() const
{
  return m_error.user;
}

#ifdef Y_PLATFORM_WINDOWS
unsigned long Error::GetErrorCodeWin32() const
{
  return m_error.win32;
}

long Error::GetErrorCodeHResult() const
{
  return m_error.hresult;
}
#endif

// setter functions
void Error::SetErrorNone()
{
  m_type = ERROR_TYPE_NONE;
  m_error.none = 0;
  m_codeString.Clear();
  m_message.Clear();
}

void Error::SetErrorErrno(int err)
{
  m_type = ERROR_TYPE_ERRNO;
  m_error.errno_f = err;

  m_codeString.Format("%i", err);

  const char* messageStr = strerror(err);
  if (messageStr != nullptr)
    m_message = messageStr;
  else
    m_message = "<Could not get error message>";
}

void Error::SetErrorSocket(int err)
{
// Socket errors are win32 errors on windows
#ifdef Y_PLATFORM_WINDOWS
  SetErrorCodeWin32(err);
#else
  SetErrorErrno(err);
#endif
}

void Error::SetErrorUser(int32 err, const char* msg)
{
  m_type = ERROR_TYPE_USER;
  m_error.user = err;
  m_codeString.Format("%i", err);
  m_message = msg;
}

void Error::SetErrorUser(const char* code, const char* message)
{
  m_type = ERROR_TYPE_USER;
  m_error.user = 0;
  m_codeString = code;
  m_message = message;
}

void Error::SetErrorUserFormatted(int32 err, const char* format, ...)
{
  va_list ap;
  va_start(ap, format);

  m_type = ERROR_TYPE_USER;
  m_error.user = err;
  m_codeString.Format("%i", err);
  m_message.FormatVA(format, ap);
  va_end(ap);
}

void Error::SetErrorUserFormatted(const char* code, const char* format, ...)
{
  va_list ap;
  va_start(ap, format);

  m_type = ERROR_TYPE_USER;
  m_error.user = 0;
  m_codeString = code;
  m_message.FormatVA(format, ap);
  va_end(ap);
}

#ifdef Y_PLATFORM_WINDOWS

void Error::SetErrorCodeWin32(unsigned long err)
{
  m_type = ERROR_TYPE_WIN32;
  m_error.win32 = err;
  m_codeString.Format("%u", (uint32)err);
  m_message.Clear();

  DWORD r = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, m_error.win32, 0, m_message.GetWriteableCharArray(),
                           m_message.GetWritableBufferSize(), NULL);
  if (r > 0)
  {
    m_message.Resize(r);
    m_message.RStrip();
  }
  else
  {
    m_message = "<Could not resolve system error ID>";
  }
}

void Error::SetErrorCodeHResult(long err)
{
  m_type = ERROR_TYPE_HRESULT;
  m_error.win32 = err;
  m_codeString.Format("%08X", (uint32)err);
  m_message.Clear();

  DWORD r = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, m_error.win32, 0, m_message.GetWriteableCharArray(),
                           m_message.GetWritableBufferSize(), NULL);
  if (r > 0)
  {
    m_message.Resize(r);
    m_message.RStrip();
  }
  else
  {
    m_message = "<Could not resolve system error ID>";
  }
}

#endif

// constructors
Error Error::CreateErrorNone()
{
  Error ret;
  ret.SetErrorNone();
  return ret;
}

Error Error::CreateErrorErrno(int err)
{
  Error ret;
  ret.SetErrorErrno(err);
  return ret;
}

Error Error::CreateErrorSocket(int err)
{
  Error ret;
  ret.SetErrorSocket(err);
  return ret;
}

Error Error::CreateErrorUser(int32 err, const char* msg)
{
  Error ret;
  ret.SetErrorUser(err, msg);
  return ret;
}

Error Error::CreateErrorUser(const char* code, const char* message)
{
  Error ret;
  ret.SetErrorUser(code, message);
  return ret;
}

Error Error::CreateErrorUserFormatted(int32 err, const char* format, ...)
{
  va_list ap;
  va_start(ap, format);

  Error ret;
  ret.m_type = ERROR_TYPE_USER;
  ret.m_error.user = err;
  ret.m_codeString.Format("%i", err);
  ret.m_message.FormatVA(format, ap);

  va_end(ap);

  return ret;
}

Error Error::CreateErrorUserFormatted(const char* code, const char* format, ...)
{
  va_list ap;
  va_start(ap, format);

  Error ret;
  ret.m_type = ERROR_TYPE_USER;
  ret.m_error.user = 0;
  ret.m_codeString = code;
  ret.m_message.FormatVA(format, ap);

  va_end(ap);

  return ret;
}

#ifdef Y_PLATFORM_WINDOWS
Error Error::CreateErrorWin32(unsigned long err)
{
  Error ret;
  ret.SetErrorCodeWin32(err);
  return ret;
}

Error Error::CreateErrorHResult(long err)
{
  Error ret;
  ret.SetErrorCodeHResult(err);
  return ret;
}

#endif

Error& Error::operator=(const Error& e)
{
  m_type = e.m_type;
  std::memcpy(&m_error, &e.m_error, sizeof(m_error));
  m_codeString.Clear();
  m_codeString.AppendString(e.m_codeString);
  m_message.Clear();
  m_message.AppendString(e.m_message);
  return *this;
}

bool Error::operator==(const Error& e) const
{
  switch (m_type)
  {
    case ERROR_TYPE_NONE:
      return true;

    case ERROR_TYPE_ERRNO:
      return m_error.errno_f == e.m_error.errno_f;

    case ERROR_TYPE_SOCKET:
      return m_error.socketerr == e.m_error.socketerr;

    case ERROR_TYPE_USER:
      return m_error.user == e.m_error.user;

#ifdef Y_PLATFORM_WINDOWS

    case ERROR_TYPE_WIN32:
      return m_error.win32 == e.m_error.win32;

    case ERROR_TYPE_HRESULT:
      return m_error.hresult == e.m_error.hresult;

#endif
  }

  return false;
}

bool Error::operator!=(const Error& e) const
{
  switch (m_type)
  {
    case ERROR_TYPE_NONE:
      return false;

    case ERROR_TYPE_ERRNO:
      return m_error.errno_f != e.m_error.errno_f;

    case ERROR_TYPE_SOCKET:
      return m_error.socketerr != e.m_error.socketerr;

    case ERROR_TYPE_USER:
      return m_error.user != e.m_error.user;

#ifdef Y_PLATFORM_WINDOWS

    case ERROR_TYPE_WIN32:
      return m_error.win32 != e.m_error.win32;

    case ERROR_TYPE_HRESULT:
      return m_error.hresult != e.m_error.hresult;

#endif
  }

  return true;
}

SmallString Error::GetErrorCodeAndDescription() const
{
  SmallString ret;
  GetErrorCodeAndDescription(ret);
  return ret;
}

void Error::GetErrorCodeAndDescription(String& dest) const
{
  dest.Format("[%s]: %s", m_codeString.GetCharArray(), m_message.GetCharArray());
}
