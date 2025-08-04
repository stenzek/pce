#include "YBaseLib/CString.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"

// call into system CRT
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef Y_PLATFORM_WINDOWS
#define strtoll _strtoi64
#define strtoull _strtoui64
#endif

char* Y_strdup(const char* Str)
{
  size_t len = strlen(Str);
  char* pNewStr = (char*)std::malloc(len + 1);
  memcpy(pNewStr, Str, len + 1);
  return pNewStr;
}

char* Y_strnew(const char* Str)
{
  size_t len = strlen(Str);
  char* pNewStr = new char[len + 1];
  memcpy(pNewStr, Str, len + 1);
  return pNewStr;
}

uint32 Y_strlen(const char* szSource)
{
  return (uint32)strlen(szSource);
}

#if defined(Y_PLATFORM_WINDOWS)

void Y_strncpy(char* pszDestination, uint32 cbDestination, const char* szSource)
{
  strncpy_s(pszDestination, cbDestination, szSource, _TRUNCATE);
}

void Y_strncat(char* pszDestination, uint32 cbDestination, const char* szSource)
{
  strncat_s(pszDestination, cbDestination, szSource, _TRUNCATE);
}

#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG) || defined(Y_COMPILER_EMSCRIPTEN)

void Y_strncpy(char* pszDestination, uint32 cbDestination, const char* szSource)
{
  size_t sourceLength = strlen(szSource);
  strncpy(pszDestination, szSource, cbDestination);
  if (sourceLength >= cbDestination)
    pszDestination[cbDestination - 1] = '\0';
}

void Y_strncat(char* pszDestination, uint32 cbDestination, const char* szSource)
{
  size_t currentLength = strlen(pszDestination);
  size_t sourceLength = strlen(szSource);

  strncat(pszDestination, szSource, cbDestination);
  if ((currentLength + sourceLength) >= cbDestination)
    pszDestination[cbDestination - 1] = '\0';
}

#endif

void Y_strnsub(char* Destination, uint32 cbDestination, const char* Source, uint32 SourceOffset, int32 SourceCount)
{
  Panic("Todo");
}

int32 Y_strcmp(const char* S1, const char* S2)
{
  return strcmp(S1, S2);
}

int32 Y_strncmp(const char* S1, const char* S2, uint32 Count)
{
  return strncmp(S1, S2, Count);
}

#if defined(Y_PLATFORM_WINDOWS)

int32 Y_stricmp(const char* S1, const char* S2)
{
  return _stricmp(S1, S2);
}

int32 Y_strnicmp(const char* S1, const char* S2, uint32 Count)
{
  return _strnicmp(S1, S2, Count);
}

#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG) || defined(Y_COMPILER_EMSCRIPTEN)

int32 Y_stricmp(const char* S1, const char* S2)
{
  return strcasecmp(S1, S2);
}

int32 Y_strnicmp(const char* S1, const char* S2, uint32 Count)
{
  return strncasecmp(S1, S2, Count);
}

#endif

uint32 Y_snprintf(char* pszDestination, uint32 cbDestination, const char* szFormat, ...)
{
  va_list ap;
  va_start(ap, szFormat);
  uint32 r = Y_vsnprintf(pszDestination, cbDestination, szFormat, ap);
  va_end(ap);
  return r;
}

uint32 Y_scprintf(const char* Format, ...)
{
  va_list ap;
  va_start(ap, Format);
  uint32 r = Y_vscprintf(Format, ap);
  va_end(ap);
  return r;
}

uint32 Y_sscanf(const char* buffer, const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  uint32 r = Y_vscanf(buffer, format, ap);
  va_end(ap);
  return r;
}

#if defined(Y_PLATFORM_WINDOWS)

uint32 Y_vsnprintf(char* pszDestination, uint32 cbDestination, const char* szFormat, va_list ArgPointer)
{
  int r = _vsnprintf_s(pszDestination, cbDestination, _TRUNCATE, szFormat, ArgPointer);
  return (r < 0) ? Y_strlen(pszDestination) : r;
}

uint32 Y_vscprintf(const char* szFormat, va_list ArgPointer)
{
  int r = _vscprintf(szFormat, ArgPointer);
  return (r < 0) ? 0 : r;
}

uint32 Y_vscanf(const char* buffer, const char* format, va_list argptr)
{
#ifdef Y_COMPILER_MSVC
  int r = vsscanf_s(buffer, format, argptr);
#else
  int r = vsscanf(buffer, format, argptr);
#endif
  return (r < 0) ? 0 : (uint32)r;
}

#elif defined(Y_COMPILER_GCC) || defined(Y_COMPILER_CLANG) || defined(Y_COMPILER_EMSCRIPTEN)

uint32 Y_vsnprintf(char* pszDestination, uint32 cbDestination, const char* szFormat, va_list ArgPointer)
{
  int r = vsnprintf(pszDestination, cbDestination, szFormat, ArgPointer);
  return static_cast<uint32>(r);
}

uint32 Y_vscprintf(const char* szFormat, va_list ArgPointer)
{
  int r = vsnprintf(nullptr, 0, szFormat, ArgPointer);
  return (r < 0) ? 0 : r;
}

uint32 Y_vscanf(const char* buffer, const char* format, va_list argptr)
{
  int r = vsscanf(buffer, format, argptr);
  return (r < 0) ? 0 : (uint32)r;
}

#endif

const char* Y_strchr(const char* SearchString, char Character)
{
  return strchr(SearchString, Character);
}

const char* Y_strrchr(const char* SearchString, char Character)
{
  return strrchr(SearchString, Character);
}

const char* Y_strstr(const char* SearchString, const char* SearchTerm)
{
  return strstr(SearchString, SearchTerm);
}

const char* Y_strpbrk(const char* SearchString, const char* SearchTerms)
{
  return strpbrk(SearchString, SearchTerms);
}

const char* Y_strrpbrk(const char* SearchString, const char* SearchTerms)
{
  const char* searchPos = SearchString;
  const char* lastResult = NULL;
  for (;;)
  {
    const char* Result = Y_strpbrk(searchPos, SearchTerms);
    if (Result == NULL)
      break;

    lastResult = Result;
    searchPos = Result + 1;
  }

  return lastResult;
}

char* Y_strchr(char* SearchString, char Character)
{
  return strchr(SearchString, Character);
}

char* Y_strrchr(char* SearchString, char Character)
{
  return strrchr(SearchString, Character);
}

char* Y_strstr(char* SearchString, const char* SearchTerm)
{
  return strstr(SearchString, SearchTerm);
}

char* Y_strpbrk(char* SearchString, const char* SearchTerms)
{
  return strpbrk(SearchString, SearchTerms);
}

char* Y_strrpbrk(char* SearchString, const char* SearchTerms)
{
  char* searchPos = SearchString;
  char* lastResult = NULL;
  for (;;)
  {
    char* Result = Y_strpbrk(searchPos, SearchTerms);
    if (Result == NULL)
      break;

    lastResult = Result;
    searchPos = Result + 1;
  }

  return lastResult;
}

char* Y_substr(char* Destination, uint32 cbDestination, const char* Source, int32 Offset, int32 Count /* = -1 */)
{
  const char* start = Source + Offset;
  const char* end = (Count < 0) ? start + Y_strlen(start) : start + Count;

  uint32 copylen = uint32(end - start);
  if (copylen >= cbDestination)
  {
    std::memcpy(Destination, start, cbDestination);
    Destination[cbDestination - 1] = '\0';
  }
  else
  {
    std::memcpy(Destination, start, copylen);
    Destination[copylen] = '\0';
  }

  return Destination;
}

// conversion functions
char Y_tolower(char Character)
{
  return (char)tolower(Character);
}

char Y_toupper(char Character)
{
  return (char)toupper(Character);
}

void Y_strlwr(char* Str, uint32 Length)
{
  for (uint32 i = 0; i < Length; i++)
    Str[i] = Y_tolower(Str[i]);
}

void Y_strupr(char* Str, uint32 Length)
{
  for (uint32 i = 0; i < Length; i++)
    Str[i] = Y_toupper(Str[i]);
}

uint32 Y_strsplit(char* Str, char Separator, char** Tokens, uint32 MaxTokens)
{
  uint32 NumTokens = 0;
  char* pStart = Str;
  char* p = Str;
  while (NumTokens < MaxTokens && *p != '\0')
  {
    if (*p == Separator)
    {
      *p = '\0';
      Tokens[NumTokens++] = pStart;
      pStart = ++p;
    }
    else
    {
      ++p;
    }
  }

  if (NumTokens < MaxTokens && pStart != p)
    Tokens[NumTokens++] = pStart;

  return NumTokens;
}

uint32 Y_strsplit2(char* Str, char Separator, char** Tokens, uint32 MaxTokens)
{
  uint32 TokenLength = 0;
  uint32 NumTokens = 0;
  char* pStart = Str;
  char* p = Str;
  while (NumTokens < MaxTokens && *p != '\0')
  {
    if (*p == Separator)
    {
      if (TokenLength > 0)
      {
        Tokens[NumTokens++] = pStart;
        TokenLength = 0;
      }

      *p = 0;
      pStart = ++p;
    }
    else
    {
      ++p;
      TokenLength++;
    }
  }

  if (NumTokens < MaxTokens && pStart != p && TokenLength > 0)
    Tokens[NumTokens++] = pStart;

  return NumTokens;
}

uint32 Y_strsplit3(char* Str, char Separator, char** Tokens, uint32 MaxTokens)
{
  uint32 TokenLength = 0;
  uint32 NumTokens = 0;
  char* pStart = Str;
  char* p = Str;
  while (NumTokens < MaxTokens && *p != '\0')
  {
    if (*p == Separator)
    {
      if (TokenLength > 0)
      {
        Tokens[NumTokens++] = pStart;
        TokenLength = 0;
      }

      if (NumTokens < MaxTokens)
        *p = '\0';

      pStart = ++p;
    }
    else
    {
      ++p;
      TokenLength++;
    }
  }

  if (NumTokens < MaxTokens && pStart != p && TokenLength > 0)
    Tokens[NumTokens++] = pStart;

  return NumTokens;
}

uint32 Y_strlstrip(char* Str, const char* StripCharacters)
{
  uint32 Length = Y_strlen(Str);
  uint32 NumStripCharacters = Y_strlen(StripCharacters);
  uint32 i;

  uint32 StartNumSkip = 0;

  // Now remove from the start.
  while (StartNumSkip < Length)
  {
    for (i = 0; i < NumStripCharacters; i++)
    {
      if (Str[StartNumSkip] == StripCharacters[i])
      {
        StartNumSkip++;
        break;
      }
    }

    if (i == NumStripCharacters)
      break;
  }

  // Remove nothing?
  if (StartNumSkip == 0)
    return Length;

  // Empty string case, again
  if (StartNumSkip == Length)
  {
#if Y_BUILD_CONFIG_DEBUG
    Y_memzero(Str, StartNumSkip);
#else
    Str[0] = '\0';
#endif
    return 0;
  }
  else
  {
    // Remove the characters from the front using memmove, then zero out the remaining characters.
    Length -= StartNumSkip;
    std::memmove(Str, Str + StartNumSkip, Length);
#if Y_BUILD_CONFIG_DEBUG
    Y_memzero(Str + Length, StartNumSkip);
#else
    Str[Length] = 0;
#endif
    return Length;
  }
}

uint32 Y_strrstrip(char* Str, const char* StripCharacters)
{
  uint32 Length = Y_strlen(Str);
  uint32 NumStripCharacters = Y_strlen(StripCharacters);
  uint32 i;

  // remove from end first
  while (Length > 0)
  {
    for (i = 0; i < NumStripCharacters; i++)
    {
      if (Str[Length - 1] == StripCharacters[i])
      {
        Str[--Length] = '\0';
        break;
      }
    }

    if (i == NumStripCharacters)
      break;
  }

  // Check for empty string.
  if (Length == 0)
    return 0;
  else
    return Length;
}

uint32 Y_strstrip(char* Str, const char* StripCharacters)
{
  uint32 Length = Y_strlen(Str);
  uint32 NumStripCharacters = Y_strlen(StripCharacters);
  uint32 i;

  uint32 StartNumSkip = 0;

  // remove from end first
  while (Length > 0)
  {
    for (i = 0; i < NumStripCharacters; i++)
    {
      if (Str[Length - 1] == StripCharacters[i])
      {
        Str[--Length] = '\0';
        break;
      }
    }

    if (i == NumStripCharacters)
      break;
  }

  // Check for empty string.
  if (Length == 0)
    return 0;

  // Now remove from the start.
  while (StartNumSkip < Length)
  {
    for (i = 0; i < NumStripCharacters; i++)
    {
      if (Str[StartNumSkip] == StripCharacters[i])
      {
        StartNumSkip++;
        break;
      }
    }

    if (i == NumStripCharacters)
      break;
  }

  // Remove nothing?
  if (StartNumSkip == 0)
    return Length;

  // Empty string case, again
  if (StartNumSkip == Length)
  {
#if Y_BUILD_CONFIG_DEBUG
    Y_memzero(Str, StartNumSkip);
#else
    Str[0] = '\0';
#endif
    return 0;
  }
  else
  {
    // Remove the characters from the front using memmove, then zero out the remaining characters.
    Length -= StartNumSkip;
    std::memmove(Str, Str + StartNumSkip, Length);
#if Y_BUILD_CONFIG_DEBUG
    Y_memzero(Str + Length, StartNumSkip);
#else
    Str[Length] = 0;
#endif
    return Length;
  }
}

bool Y_strwildcmp(const char* Subject, const char* Mask)
{
  const char* cp = NULL;
  const char* mp = NULL;

  while ((*Subject) && (*Mask != '*'))
  {
    if ((*Mask != *Subject) && (*Mask != '?'))
      return false;

    Mask++;
    Subject++;
  }

  while (*Subject)
  {
    if (*Mask == '*')
    {
      if (*++Mask == 0)
        return true;

      mp = Mask;
      cp = Subject + 1;
    }
    else
    {
      if ((*Mask == *Subject) || (*Mask == '?'))
      {
        Mask++;
        Subject++;
      }
      else
      {
        Mask = mp;
        Subject = cp++;
      }
    }
  }

  while (*Mask == '*')
  {
    Mask++;
  }

  return *Mask == 0;
}

bool Y_striwildcmp(const char* Subject, const char* Mask)
{
  const char* cp = NULL;
  const char* mp = NULL;

  while ((*Subject) && (*Mask != '*'))
  {
    if ((*Mask != '?') && (Y_tolower(*Mask) != Y_tolower(*Subject)))
      return false;

    Mask++;
    Subject++;
  }

  while (*Subject)
  {
    if (*Mask == '*')
    {
      if (*++Mask == 0)
        return true;

      mp = Mask;
      cp = Subject + 1;
    }
    else
    {
      if ((*Mask == '?') || (Y_tolower(*Mask) == Y_tolower(*Subject)))
      {
        Mask++;
        Subject++;
      }
      else
      {
        Mask = mp;
        Subject = cp++;
      }
    }
  }

  while (*Mask == '*')
  {
    Mask++;
  }

  return *Mask == 0;
}

int32 Y_selectstring(const char* SelectionString, const char* ValueString)
{
  uint32 valueLen = Y_strlen(ValueString);
  uint32 len = Y_strlen(SelectionString);
  uint32 i = 0;
  uint32 start = 0;
  uint32 tokenIndex = 0;

  for (; i < len; i++)
  {
    if (SelectionString[i] == '|')
    {
      if ((i - start) == valueLen && Y_strnicmp(SelectionString + start, ValueString, i - start) == 0)
        return tokenIndex;

      start = i + 1;
      tokenIndex++;
    }
  }

  if (start < len)
  {
    if ((len - start) == valueLen && Y_strnicmp(SelectionString + start, ValueString, len - start) == 0)
      return tokenIndex;
  }

  return -1;
}

uint32 Y_parsehexstring(const char* hexString, void* pOutBytes, uint32 maxOutBytes, bool exitOnInvalidCharacter,
                        bool* pAtEnd)
{
  uint32 i = 0;
  uint32 n = 0;
  byte last = 0;
  bool lb = false;

  while (hexString[i] != '\0' && n < maxOutBytes)
  {
    byte b = 0;
    char ch = hexString[i];

    // hex characters
    if (ch >= '0' && ch <= '9')
      b = ch - '0';
    else if (ch >= 'a' && ch <= 'f')
      b = ch - 'a' + 0xa;
    else if (ch >= 'A' && ch <= 'F')
      b = ch - 'A' + 0xa;
    else
    {
      // skip whitespace
      if (exitOnInvalidCharacter)
        break;
      else
        continue;

      // else if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
    }

    if (lb)
    {
      ((byte*)pOutBytes)[n++] = (last << 4) | b;
      lb = false;
    }
    else
    {
      last = b;
      lb = true;
    }

    i++;
  }

  if (pAtEnd != NULL)
    *pAtEnd = (hexString[i] == '\0');

  return n;
}

uint32 Y_makehexstring(const void* pInBytes, uint32 nInBytes, char* outBuffer, uint32 bufferSize)
{
  uint32 i = 0;
  uint32 n = 0;

  while (i < nInBytes && n < bufferSize)
  {
    byte b = ((const byte*)pInBytes)[i++];

    byte c = (b >> 4) & 0xF;
    if (c >= 0xa)
      outBuffer[n++] = 'a' + (c - 0xa);
    else
      outBuffer[n++] = '0' + c;

    c = b & 0xF;
    if (c >= 0xa)
      outBuffer[n++] = 'a' + (c - 0xa);
    else
      outBuffer[n++] = '0' + c;
  }

  if (n == bufferSize)
    outBuffer[bufferSize - 1] = '\0';
  else
    outBuffer[n] = '\0';

  return n;
}

uint32 Y_getencodedbase64length(uint32 byteLength)
{
  uint32 inMod3 = byteLength % 3;
  uint32 currentSize = byteLength;
  if (inMod3 != 0)
    currentSize += 3 - inMod3;

  DebugAssert((currentSize % 3) == 0);
  return (currentSize / 3) * 4;
}

uint32 Y_getdecodedbase64length(uint32 base64Length)
{
  if (base64Length > 0)
    return (base64Length / 4 + 1) * 3;
  else
    return 0;
}

uint32 Y_parsebase64(const char* base64String, void* pOutBytes, uint32 maxOutBytes,
                     bool exitOnInvalidCharacter /*= true*/, bool* pAtEnd /*= NULL*/)
{
  static const byte base64DecodingTable[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 00-0F
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 10-1F
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255, 255, 255, 63,  // 20-2F
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255, 255, 255, 255, 255, // 30-3F
    255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  // 40-4F
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  255, 255, 255, 255, 255, // 50-5F
    255, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  // 60-6F
    41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  255, 255, 255, 255, 255, // 70-7F
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 80-8F
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 90-9F
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // A0-AF
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // B0-BF
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // C0-CF
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // D0-DF
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // E0-EF
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // F0-FF
  };

  const char* inPointer = base64String;
  const char* inPointerEnd = base64String + Y_strlen(base64String);
  byte* outPointer = reinterpret_cast<byte*>(pOutBytes);
  byte* outPointerStart = outPointer;
  byte* outPointerEnd = outPointer + maxOutBytes;

  while (outPointer != outPointerEnd)
  {
    if ((inPointerEnd - inPointer) < 4)
      break;

    // get the base64 values
    uint32 i;
    byte base64Values[4];
    uint32 nBase64Values = 0;
    for (i = 0; i < 4; i++)
    {
      byte base64Character = *reinterpret_cast<const byte*>(inPointer++);
      if ((base64Values[nBase64Values] = base64DecodingTable[base64Character]) == 255)
      {
        if (base64Character == '=')
          break;

        // if (exitOnInvalidCharacter)
        break;
      }

      nBase64Values++;
    }

    // end of stream or error?
    if (nBase64Values == 0)
      continue;

    // decode base64 values
    uint32 nOutputBytes = Min(nBase64Values - 1, (uint32)(outPointerEnd - outPointer));
    switch (nOutputBytes)
    {
      case 1:
        *(outPointer++) = base64Values[0] << 2 | ((base64Values[1] >> 4) & 3);
        break;

      case 2:
        *(outPointer++) = base64Values[0] << 2 | ((base64Values[1] >> 4) & 3);
        *(outPointer++) = (base64Values[1] & 15) << 4 | ((base64Values[2] >> 2));
        break;

      case 3:
        *(outPointer++) = base64Values[0] << 2 | ((base64Values[1] >> 4) & 3);
        *(outPointer++) = (base64Values[1] & 15) << 4 | ((base64Values[2] >> 2));
        *(outPointer++) = (base64Values[2] & 3) << 6 | (base64Values[3] & 63);
        break;
    }
  }

  return uint32(outPointer - outPointerStart);
}

uint32 Y_makebase64(const void* pInBytes, uint32 nInBytes, char* outBuffer, uint32 bufferSize)
{
  static const char base64EncodingTable[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
    's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

  const byte* inPointer = reinterpret_cast<const byte*>(pInBytes);
  const byte* inPointerEnd = inPointer + nInBytes;
  char* outPointer = outBuffer;
  char* outPointerStart = outBuffer;
  char* outPointerEnd = outPointer + bufferSize;

  for (; inPointer < inPointerEnd; inPointer += 3)
  {
    // space for another sequence?
    if ((outPointerEnd - outPointer) < 5)
      break;

    // calculate number of bytes in sequence
    uint32 nBytesInSequence = Min((uint32)3, (uint32)(inPointerEnd - inPointer));
    switch (nBytesInSequence)
    {
      case 1:
        *(outPointer++) = base64EncodingTable[(*(inPointer + 0) >> 2) & 63];
        *(outPointer++) = base64EncodingTable[(*(inPointer + 0) & 3) << 4];
        *(outPointer++) = '=';
        *(outPointer++) = '=';
        break;

      case 2:
        *(outPointer++) = base64EncodingTable[(*(inPointer + 0) >> 2) & 63];
        *(outPointer++) = base64EncodingTable[((*(inPointer + 0) & 3) << 4) | ((*(inPointer + 1) >> 4) & 15)];
        *(outPointer++) = base64EncodingTable[(*(inPointer + 1) & 15) << 2];
        *(outPointer++) = '=';
        break;

      case 3:
        *(outPointer++) = base64EncodingTable[((*inPointer + 0) >> 2) & 63];
        *(outPointer++) = base64EncodingTable[((*(inPointer + 0) & 3) << 4) | ((*(inPointer + 1) >> 4) & 15)];
        *(outPointer++) = base64EncodingTable[((*(inPointer + 1) & 15) << 2) | ((*(inPointer + 2) >> 6) & 3)];
        *(outPointer++) = base64EncodingTable[*(inPointer + 2) & 63];
        break;
    }
  }

  if ((outPointer - outPointerStart) > 0)
    *(outPointer) = '\0';

  return uint32(outPointer - outPointerStart);
}

//---------------------------------------------------------------------------------------------------------------------
bool Y_strtobool(const char* Str, char** pEndPtr /* = NULL */)
{
  const char* LocalEndPtr;
  bool Result;
  if (Str[0] == '1')
  {
    LocalEndPtr = Str + 1;
    Result = true;
  }
  else if (Str[0] == '0')
  {
    LocalEndPtr = Str + 1;
    Result = false;
  }
  else if (!Y_stricmp(Str, "True"))
  {
    LocalEndPtr = Str + 4;
    Result = true;
  }
  else if (!Y_stricmp(Str, "False"))
  {
    LocalEndPtr = Str + 5;
    Result = false;
  }
  else if (!Y_stricmp(Str, "Yes"))
  {
    LocalEndPtr = Str + 3;
    Result = true;
  }
  else if (!Y_stricmp(Str, "No"))
  {
    LocalEndPtr = Str + 2;
    Result = false;
  }
  else
  {
    LocalEndPtr = Str;
    Result = false;
  }

  if (pEndPtr != NULL)
    *pEndPtr = const_cast<char*>(LocalEndPtr);

  return Result;
}

float Y_strtofloat(const char* Str, char** pEndPtr /* = NULL */)
{
  return (float)strtod(Str, pEndPtr);
}

double Y_strtodouble(const char* Str, char** pEndPtr /* = NULL */)
{
  return strtod(Str, pEndPtr);
}

byte Y_strtobyte(const char* Str, char** pEndPtr /* = NULL */)
{
  if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X'))
    return (byte)strtoul(Str + 2, pEndPtr, 16);
  else
    return (byte)strtoul(Str, pEndPtr, 10);
}

int8 Y_strtoint8(const char* Str, char** pEndPtr /* = NULL */)
{
  if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X'))
    return (int8)strtol(Str + 2, pEndPtr, 16);
  else
    return (int8)strtol(Str, pEndPtr, 10);
}

int16 Y_strtoint16(const char* Str, char** pEndPtr /* = NULL */)
{
  if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X'))
    return (int16)strtol(Str + 2, pEndPtr, 16);
  else
    return (int16)strtol(Str, pEndPtr, 10);
}

int32 Y_strtoint32(const char* Str, char** pEndPtr /* = NULL */)
{
  if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X'))
    return (int32)strtol(Str + 2, pEndPtr, 16);
  else
    return (int32)strtol(Str, pEndPtr, 10);
}

int64 Y_strtoint64(const char* Str, char** pEndPtr /* = NULL */)
{
  if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X'))
    return (int64)strtoll(Str + 2, pEndPtr, 16);
  else
    return (int64)strtoll(Str, pEndPtr, 10);
}

uint8 Y_strtouint8(const char* Str, char** pEndPtr /* = NULL */)
{
  if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X'))
    return (uint8)strtoul(Str + 2, pEndPtr, 16);
  else
    return (uint8)strtoul(Str, pEndPtr, 10);
}

uint16 Y_strtouint16(const char* Str, char** pEndPtr /* = NULL */)
{
  if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X'))
    return (uint16)strtoul(Str + 2, pEndPtr, 16);
  else
    return (uint16)strtoul(Str, pEndPtr, 10);
}

uint32 Y_strtouint32(const char* Str, char** pEndPtr /* = NULL */)
{
  if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X'))
    return (uint32)strtoul(Str + 2, pEndPtr, 16);
  else
    return (uint32)strtoul(Str, pEndPtr, 10);
}

uint64 Y_strtouint64(const char* Str, char** pEndPtr /* = NULL */)
{
  if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X'))
    return (uint64)strtoull(Str + 2, pEndPtr, 16);
  else
    return (uint64)strtoull(Str, pEndPtr, 10);
}

//---------------------------------------------------------------------------------------------------------------------
void Y_strfrombool(char* Str, uint32 MaxLength, bool Value)
{
  Y_strncpy(Str, MaxLength, Value ? "true" : "false");
}

void Y_strfromfloat(char* Str, uint32 MaxLength, float Value)
{
  Y_snprintf(Str, MaxLength, "%f", Value);
}

void Y_strfromdouble(char* Str, uint32 MaxLength, double Value)
{
  Y_snprintf(Str, MaxLength, "%Lf", Value);
}

void Y_strfrombyte(char* Str, uint32 MaxLength, byte Value)
{
  Y_snprintf(Str, MaxLength, "%02X", (uint32)Value);
}

void Y_strfromint8(char* Str, uint32 MaxLength, int8 Value, uint32 Base)
{
  Y_snprintf(Str, MaxLength, (Base == 10) ? "%d" : "%02X", (int32)Value);
}

void Y_strfromint16(char* Str, uint32 MaxLength, int16 Value, uint32 Base)
{
  Y_snprintf(Str, MaxLength, (Base == 10) ? "%d" : "%04X", (int32)Value);
}

void Y_strfromint32(char* Str, uint32 MaxLength, int32 Value, uint32 Base)
{
  Y_snprintf(Str, MaxLength, (Base == 10) ? "%d" : "%08X", (int32)Value);
}

void Y_strfromint64(char* Str, uint32 MaxLength, int64 Value, uint32 Base)
{
  Y_snprintf(Str, MaxLength, (Base == 10) ? "%I64" : "%I64X", (int64)Value);
}

void Y_strfromuint8(char* Str, uint32 MaxLength, uint8 Value, uint32 Base)
{
  Y_snprintf(Str, MaxLength, (Base == 10) ? "%d" : "%02X", (uint32)Value);
}

void Y_strfromuint16(char* Str, uint32 MaxLength, uint16 Value, uint32 Base)
{
  Y_snprintf(Str, MaxLength, (Base == 10) ? "%d" : "%04X", (uint32)Value);
}

void Y_strfromuint32(char* Str, uint32 MaxLength, uint32 Value, uint32 Base)
{
  Y_snprintf(Str, MaxLength, (Base == 10) ? "%d" : "%08X", (uint32)Value);
}

void Y_strfromuint64(char* Str, uint32 MaxLength, uint64 Value, uint32 Base)
{
  Y_snprintf(Str, MaxLength, (Base == 10) ? "%I64u" : "%I64X", (uint64)Value);
}
