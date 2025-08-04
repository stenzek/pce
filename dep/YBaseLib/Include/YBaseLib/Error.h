#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/String.h"

enum ERROR_TYPE
{
  ERROR_TYPE_NONE = 0,   // Set by default constructor, returns 'No Error'.
  ERROR_TYPE_ERRNO = 1,  // Error that is set by system functions, such as open().
  ERROR_TYPE_SOCKET = 2, // Error that is set by socket functions, such as socket(). On Unix this is the same as errno.
  ERROR_TYPE_USER = 3,   // When translated, will return 'User Error %u' if no message is specified.
  ERROR_TYPE_WIN32 = 4, // Error that is returned by some Win32 functions, such as RegOpenKeyEx. Also used by other APIs
                        // through GetLastError().
  ERROR_TYPE_HRESULT = 5, // Error that is returned by Win32 COM methods, e.g. S_OK.
};

// this class provides enough storage room for all of these types
class Error
{
public:
  Error();
  Error(const Error& c);
  ~Error();

  // accessors
  ERROR_TYPE GetType() const;
  int GetErrorCodeErrno() const;
  int GetErrorCodeSocket() const;
  int32 GetErrorCodeUser() const;
#ifdef Y_PLATFORM_WINDOWS
  unsigned long GetErrorCodeWin32() const;
  long GetErrorCodeHResult() const;
#endif

  // setter functions
  void SetErrorNone();
  void SetErrorErrno(int err);
  void SetErrorSocket(int err);
  void SetErrorUser(int32 err, const char* msg);
  void SetErrorUser(const char* code, const char* message);
  void SetErrorUserFormatted(int32 err, const char* format, ...);
  void SetErrorUserFormatted(const char* code, const char* format, ...);
#ifdef Y_PLATFORM_WINDOWS
  void SetErrorCodeWin32(unsigned long err);
  void SetErrorCodeHResult(long err);
#endif

  // constructors
  static Error CreateErrorNone();
  static Error CreateErrorErrno(int err);
  static Error CreateErrorSocket(int err);
  static Error CreateErrorUser(int32 err, const char* msg);
  static Error CreateErrorUser(const char* code, const char* message);
  static Error CreateErrorUserFormatted(int32 err, const char* format, ...);
  static Error CreateErrorUserFormatted(const char* code, const char* format, ...);
#ifdef Y_PLATFORM_WINDOWS
  static Error CreateErrorWin32(unsigned long err);
  static Error CreateErrorHResult(long err);
#endif

  // get code, e.g. "0x00000002"
  const String& GetErrorCodeString() const { return m_codeString; }

  // get description, e.g. "File not Found"
  const String& GetErrorDescription() const { return m_message; }

  // get code and description, e.g. "[0x00000002]: File not Found"
  SmallString GetErrorCodeAndDescription() const;
  void GetErrorCodeAndDescription(String& dest) const;

  // operators
  Error& operator=(const Error& e);
  bool operator==(const Error& e) const;
  bool operator!=(const Error& e) const;

private:
  ERROR_TYPE m_type;
  union
  {
    int32 none;
    int errno_f; // renamed from errno to avoid conflicts with #define'd errnos.
    int socketerr;
    int32 user;
    uint32 external;
#ifdef Y_PLATFORM_WINDOWS
    unsigned long win32;
    long hresult;
#endif
  } m_error;
  TinyString m_codeString;
  SmallString m_message;
};
