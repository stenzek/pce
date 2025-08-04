#include "YBaseLib/String.h"
#include "YBaseLib/Atomic.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Memory.h"

// globals
const String::StringData String::s_EmptyStringData = {const_cast<char*>(""), 0, 1, true, -1};
const String EmptyString;

// helper functions
static String::StringData* StringDataAllocate(uint32 allocSize)
{
  DebugAssert(allocSize > 0);

  String::StringData* pStringData =
    reinterpret_cast<String::StringData*>(std::malloc(sizeof(String::StringData) + allocSize));
  pStringData->pBuffer = reinterpret_cast<char*>(pStringData + 1);
  pStringData->StringLength = 0;
  pStringData->BufferSize = allocSize;
  pStringData->ReadOnly = false;
  pStringData->ReferenceCount = 1;

  // if in debug build, set all to zero, otherwise only the first to zero.
#if Y_BUILD_CONFIG_DEBUG
  Y_memzero(pStringData->pBuffer, allocSize);
#else
  pStringData->pBuffer[0] = 0;
  if (allocSize > 1)
    pStringData->pBuffer[allocSize - 1] = 0;
#endif

  return pStringData;
}

static inline void StringDataAddRef(String::StringData* pStringData)
{
  DebugAssert(pStringData->ReferenceCount > 0);
  Y_AtomicIncrement(pStringData->ReferenceCount);
}

static inline void StringDataRelease(String::StringData* pStringData)
{
  if (pStringData->ReferenceCount == -1)
    return;

  DebugAssert(pStringData->ReferenceCount > 0);
  uint32 newRefCount = Y_AtomicDecrement(pStringData->ReferenceCount);
  if (!newRefCount)
    std::free(pStringData);
}

static String::StringData* StringDataClone(const String::StringData* pStringData, uint32 newSize, bool copyPastString)
{
  DebugAssert(newSize >= 0);

  String::StringData* pClone = StringDataAllocate(newSize);
  if (pStringData->StringLength > 0)
  {
    uint32 copyLength;

    if (copyPastString)
    {
      copyLength = Min(newSize, pStringData->BufferSize);
      if (copyLength > 0)
      {
        std::memcpy(pClone->pBuffer, pStringData->pBuffer, copyLength);
        if (copyLength < pStringData->BufferSize)
          pClone->pBuffer[copyLength - 1] = 0;
      }
    }
    else
    {
      copyLength = Min(newSize, pStringData->StringLength);
      if (copyLength > 0)
      {
        std::memcpy(pClone->pBuffer, pStringData->pBuffer, copyLength);
        pClone->pBuffer[copyLength] = 0;
      }
    }

    pClone->StringLength = copyLength;
  }

  return pClone;
}

static String::StringData* StringDataReallocate(String::StringData* pStringData, uint32 newSize)
{
  DebugAssert(newSize > pStringData->StringLength);
  DebugAssert(pStringData->ReferenceCount == 1);

  // perform realloc
  pStringData = reinterpret_cast<String::StringData*>(std::realloc(pStringData, sizeof(String::StringData) + newSize));
  pStringData->pBuffer = reinterpret_cast<char*>(pStringData + 1);

  // zero bytes in debug
#if Y_BUILD_CONFIG_DEBUG
  if (newSize > pStringData->BufferSize)
  {
    uint32 bytesToZero = newSize - pStringData->BufferSize;
    Y_memzero(pStringData->pBuffer + (newSize - bytesToZero), bytesToZero);
  }
#else
  if (newSize > pStringData->BufferSize)
  {
    pStringData->pBuffer[newSize - 1] = 0;
  }
#endif

  // update size
  pStringData->BufferSize = newSize;
  return pStringData;
}

static bool StringDataIsSharable(const String::StringData* pStringData)
{
  return pStringData->ReferenceCount != -1;
}

static bool StringDataIsShared(const String::StringData* pStringData)
{
  return pStringData->ReferenceCount > 1;
}

String::String() : m_pStringData(const_cast<String::StringData*>(&s_EmptyStringData)) {}

String::String(const String& copyString)
{
  // special case: empty strings
  if (copyString.IsEmpty())
  {
    m_pStringData = const_cast<String::StringData*>(&s_EmptyStringData);
  }
  // is the string data sharable?
  else if (StringDataIsSharable(copyString.m_pStringData))
  {
    m_pStringData = copyString.m_pStringData;
    StringDataAddRef(m_pStringData);
  }
  // create a clone for ourselves
  else
  {
    // since we're going to the effort of creating a clone, we might as well create it as the smallest size possible
    m_pStringData = StringDataClone(copyString.m_pStringData, copyString.m_pStringData->StringLength + 1, false);
  }
}

String::String(const char* Text) : m_pStringData(const_cast<String::StringData*>(&s_EmptyStringData))
{
  Assign(Text);
}

String::~String()
{
  StringDataRelease(m_pStringData);
}

void String::EnsureOwnWritableCopy()
{
  if (StringDataIsShared(m_pStringData) || m_pStringData->ReadOnly)
  {
    StringData* pNewStringData = StringDataClone(m_pStringData, m_pStringData->StringLength + 1, false);
    StringDataRelease(m_pStringData);
    m_pStringData = pNewStringData;
  }
}

void String::EnsureRemainingSpace(uint32 spaceRequired)
{
  StringData* pNewStringData;
  uint32 requiredReserve = m_pStringData->StringLength + spaceRequired + 1;

  if (StringDataIsShared(m_pStringData) || m_pStringData->ReadOnly)
  {
    pNewStringData = StringDataClone(m_pStringData, Max(requiredReserve, m_pStringData->BufferSize), false);
    StringDataRelease(m_pStringData);
    m_pStringData = pNewStringData;
  }
  else if (m_pStringData->BufferSize < requiredReserve)
  {
    uint32 newSize = Max(requiredReserve, m_pStringData->BufferSize * 2);

    // if we are the only owner of the buffer, we can simply realloc it
    if (m_pStringData->ReferenceCount == 1)
    {
      // do realloc and update pointer
      m_pStringData = StringDataReallocate(m_pStringData, newSize);
    }
    else
    {
      // clone and release old
      pNewStringData = StringDataClone(m_pStringData, Max(requiredReserve, newSize), false);
      StringDataRelease(m_pStringData);
      m_pStringData = pNewStringData;
    }
  }
}

void String::InternalAppend(const char* pString, uint32 Length)
{
  EnsureRemainingSpace(Length);

  DebugAssert((Length + m_pStringData->StringLength) < m_pStringData->BufferSize);
  DebugAssert(m_pStringData->ReferenceCount <= 1 && !m_pStringData->ReadOnly);

  std::memcpy(m_pStringData->pBuffer + m_pStringData->StringLength, pString, Length);
  m_pStringData->StringLength += Length;
  m_pStringData->pBuffer[m_pStringData->StringLength] = 0;
}

void String::InternalPrepend(const char* pString, uint32 Length)
{
  EnsureRemainingSpace(Length);

  DebugAssert((Length + m_pStringData->StringLength) < m_pStringData->BufferSize);
  DebugAssert(m_pStringData->ReferenceCount <= 1 && !m_pStringData->ReadOnly);

  std::memmove(m_pStringData->pBuffer + Length, m_pStringData->pBuffer, m_pStringData->StringLength);
  std::memcpy(m_pStringData->pBuffer, pString, Length);
  m_pStringData->StringLength += Length;
  m_pStringData->pBuffer[m_pStringData->StringLength] = 0;
}

void String::AppendCharacter(char c)
{
  InternalAppend(&c, 1);
}

void String::AppendString(const String& appendStr)
{
  if (appendStr.GetLength() > 0)
    InternalAppend(appendStr.GetCharArray(), appendStr.GetLength());
}

void String::AppendString(const char* appendText)
{
  uint32 textLength = Y_strlen(appendText);
  if (textLength > 0)
    InternalAppend(appendText, textLength);
}

void String::AppendString(const char* appendString, uint32 Count)
{
  if (Count > 0)
    InternalAppend(appendString, Count);
}

void String::AppendSubString(const String& appendStr, int32 Offset /* = 0 */, int32 Count /* = INT_MAX */)
{
  uint32 appendStrLength = appendStr.GetLength();

  // calc real offset
  uint32 realOffset;
  if (Offset < 0)
    realOffset = (uint32)Max((int32)0, (int32)appendStrLength + Offset);
  else
    realOffset = Min((uint32)Offset, appendStrLength);

  // calc real count
  uint32 realCount;
  if (Count < 0)
    realCount = Min(appendStrLength - realOffset, (uint32)Max((int32)0, (int32)appendStrLength + Count));
  else
    realCount = Min(appendStrLength - realOffset, (uint32)Count);

  // should be safe
  DebugAssert((realOffset + realCount) <= appendStrLength);
  if (realCount > 0)
    InternalAppend(appendStr.GetCharArray() + realOffset, realCount);
}

void String::AppendSubString(const char* appendText, int32 Offset /* = 0 */, int32 Count /* = INT_MAX */)
{
  uint32 appendTextLength = Y_strlen(appendText);

  // calc real offset
  uint32 realOffset;
  if (Offset < 0)
    realOffset = (uint32)Max((int32)0, (int32)appendTextLength + Offset);
  else
    realOffset = Min((uint32)Offset, appendTextLength);

  // calc real count
  uint32 realCount;
  if (Count < 0)
    realCount = Min(appendTextLength - realOffset, (uint32)Max((int32)0, (int32)appendTextLength + Count));
  else
    realCount = Min(appendTextLength - realOffset, (uint32)Count);

  // should be safe
  DebugAssert((realOffset + realCount) <= appendTextLength);
  if (realCount > 0)
    InternalAppend(appendText + realOffset, realCount);
}

void String::AppendFormattedString(const char* FormatString, ...)
{
  va_list ap;
  va_start(ap, FormatString);
  AppendFormattedStringVA(FormatString, ap);
  va_end(ap);
}

void String::AppendFormattedStringVA(const char* FormatString, va_list ArgPtr)
{
  // We have a 1KB byte buffer on the stack here. If this is too little, we'll grow it via the heap,
  // but 1KB should be enough for most strings.
  char stackBuffer[1024];
  char* pHeapBuffer = NULL;
  char* pBuffer = stackBuffer;
  uint32 currentBufferSize = countof(stackBuffer);
  uint32 charsWritten;

  for (;;)
  {
    int ret = Y_vsnprintf(pBuffer, currentBufferSize, FormatString, ArgPtr);
    if (ret < 0 || ((uint32)ret >= (currentBufferSize - 1)))
    {
      currentBufferSize *= 2;
      pBuffer = pHeapBuffer = reinterpret_cast<char*>(std::realloc(pHeapBuffer, currentBufferSize));
      continue;
    }

    charsWritten = (uint32)ret;
    break;
  }

  InternalAppend(pBuffer, charsWritten);

  if (pHeapBuffer != NULL)
    std::free(pHeapBuffer);
}

void String::PrependCharacter(char c)
{
  InternalPrepend(&c, 1);
}

void String::PrependString(const String& appendStr)
{
  if (appendStr.GetLength() > 0)
    InternalPrepend(appendStr.GetCharArray(), appendStr.GetLength());
}

void String::PrependString(const char* appendText)
{
  uint32 textLength = Y_strlen(appendText);
  if (textLength > 0)
    InternalPrepend(appendText, textLength);
}

void String::PrependString(const char* appendString, uint32 Count)
{
  if (Count > 0)
    InternalPrepend(appendString, Count);
}

void String::PrependSubString(const String& appendStr, int32 Offset /* = 0 */, int32 Count /* = INT_MAX */)
{
  uint32 appendStrLength = appendStr.GetLength();

  // calc real offset
  uint32 realOffset;
  if (Offset < 0)
    realOffset = (uint32)Max((int32)0, (int32)appendStrLength + Offset);
  else
    realOffset = Min((uint32)Offset, appendStrLength);

  // calc real count
  uint32 realCount;
  if (Count < 0)
    realCount = Min(appendStrLength - realOffset, (uint32)Max((int32)0, (int32)appendStrLength + Count));
  else
    realCount = Min(appendStrLength - realOffset, (uint32)Count);

  // should be safe
  DebugAssert((realOffset + realCount) <= appendStrLength);
  if (realCount > 0)
    InternalPrepend(appendStr.GetCharArray() + realOffset, realCount);
}

void String::PrependSubString(const char* appendText, int32 Offset /* = 0 */, int32 Count /* = INT_MAX */)
{
  uint32 appendTextLength = Y_strlen(appendText);

  // calc real offset
  uint32 realOffset;
  if (Offset < 0)
    realOffset = (uint32)Max((int32)0, (int32)appendTextLength + Offset);
  else
    realOffset = Min((uint32)Offset, appendTextLength);

  // calc real count
  uint32 realCount;
  if (Count < 0)
    realCount = Min(appendTextLength - realOffset, (uint32)Max((int32)0, (int32)appendTextLength + Count));
  else
    realCount = Min(appendTextLength - realOffset, (uint32)Count);

  // should be safe
  DebugAssert((realOffset + realCount) <= appendTextLength);
  if (realCount > 0)
    InternalPrepend(appendText + realOffset, realCount);
}

void String::PrependFormattedString(const char* FormatString, ...)
{
  va_list ap;
  va_start(ap, FormatString);
  PrependFormattedStringVA(FormatString, ap);
  va_end(ap);
}

void String::PrependFormattedStringVA(const char* FormatString, va_list ArgPtr)
{
  // We have a 1KB byte buffer on the stack here. If this is too little, we'll grow it via the heap,
  // but 1KB should be enough for most strings.
  char stackBuffer[1024];
  char* pHeapBuffer = NULL;
  char* pBuffer = stackBuffer;
  uint32 currentBufferSize = countof(stackBuffer);
  uint32 charsWritten;

  for (;;)
  {
    int ret = Y_vsnprintf(pBuffer, currentBufferSize, FormatString, ArgPtr);
    if (ret < 0 || ((uint32)ret >= (currentBufferSize - 1)))
    {
      currentBufferSize *= 2;
      pBuffer = pHeapBuffer = reinterpret_cast<char*>(std::realloc(pHeapBuffer, currentBufferSize));
      continue;
    }

    charsWritten = (uint32)ret;
    break;
  }

  InternalPrepend(pBuffer, charsWritten);

  if (pHeapBuffer != NULL)
    std::free(pHeapBuffer);
}

void String::InsertString(int32 offset, const String& appendStr)
{
  InsertString(offset, appendStr, appendStr.GetLength());
}

void String::InsertString(int32 offset, const char* appendStr)
{
  InsertString(offset, appendStr, Y_strlen(appendStr));
}

void String::InsertString(int32 offset, const char* appendStr, uint32 appendStrLength)
{
  if (appendStrLength == 0)
    return;

  EnsureRemainingSpace(appendStrLength);

  // calc real offset
  uint32 realOffset;
  if (offset < 0)
    realOffset = (uint32)Max((int32)0, (int32)m_pStringData->StringLength + offset);
  else
    realOffset = Min((uint32)offset, m_pStringData->StringLength);

  // determine number of characters after offset
  DebugAssert(realOffset <= m_pStringData->StringLength);
  uint32 charactersAfterOffset = m_pStringData->StringLength - realOffset;
  if (charactersAfterOffset > 0)
    std::memmove(m_pStringData->pBuffer + offset + appendStrLength, m_pStringData->pBuffer + offset,
                 charactersAfterOffset);

  // insert the string
  std::memcpy(m_pStringData->pBuffer + realOffset, appendStr, appendStrLength);
  m_pStringData->StringLength += appendStrLength;

  // ensure null termination
  m_pStringData->pBuffer[m_pStringData->StringLength] = 0;
}

void String::Format(const char* FormatString, ...)
{
  va_list ap;
  va_start(ap, FormatString);
  FormatVA(FormatString, ap);
  va_end(ap);
}

void String::FormatVA(const char* FormatString, va_list ArgPtr)
{
  if (GetLength() > 0)
    Clear();

  AppendFormattedStringVA(FormatString, ArgPtr);
}

void String::Assign(const String& copyString)
{
  // special case: empty strings
  if (copyString.IsEmpty())
  {
    m_pStringData = const_cast<String::StringData*>(&s_EmptyStringData);
  }
  // is the string data sharable?
  else if (StringDataIsSharable(copyString.m_pStringData))
  {
    m_pStringData = copyString.m_pStringData;
    StringDataAddRef(m_pStringData);
  }
  // create a clone for ourselves
  else
  {
    // since we're going to the effort of creating a clone, we might as well create it as the smallest size possible
    m_pStringData = StringDataClone(copyString.m_pStringData, copyString.m_pStringData->StringLength + 1, false);
  }
}

void String::Assign(const char* copyText)
{
  Clear();
  AppendString(copyText);
}

void String::AssignCopy(const String& copyString)
{
  Clear();
  AppendString(copyString);
}

void String::Swap(String& swapString)
{
  ::Swap(m_pStringData, swapString.m_pStringData);
}

bool String::Compare(const String& otherString) const
{
  return (Y_strcmp(m_pStringData->pBuffer, otherString.m_pStringData->pBuffer) == 0);
}

bool String::Compare(const char* otherText) const
{
  return (Y_strcmp(m_pStringData->pBuffer, otherText) == 0);
}

bool String::SubCompare(const String& otherString, uint32 Length) const
{
  return (Y_strncmp(m_pStringData->pBuffer, otherString.m_pStringData->pBuffer, Length) == 0);
}

bool String::SubCompare(const char* otherText, uint32 Length) const
{
  return (Y_strncmp(m_pStringData->pBuffer, otherText, Length) == 0);
}

bool String::CompareInsensitive(const String& otherString) const
{
  return (Y_stricmp(m_pStringData->pBuffer, otherString.m_pStringData->pBuffer) == 0);
}

bool String::CompareInsensitive(const char* otherText) const
{
  return (Y_stricmp(m_pStringData->pBuffer, otherText) == 0);
}

bool String::SubCompareInsensitive(const String& otherString, uint32 Length) const
{
  return (Y_strnicmp(m_pStringData->pBuffer, otherString.m_pStringData->pBuffer, Length) == 0);
}

bool String::SubCompareInsensitive(const char* otherText, uint32 Length) const
{
  return (Y_strnicmp(m_pStringData->pBuffer, otherText, Length) == 0);
}

int String::NumericCompare(const String& otherString) const
{
  return Y_strcmp(m_pStringData->pBuffer, otherString.m_pStringData->pBuffer);
}

int String::NumericCompare(const char* otherText) const
{
  return Y_strcmp(m_pStringData->pBuffer, otherText);
}

int String::NumericCompareInsensitive(const String& otherString) const
{
  return Y_stricmp(m_pStringData->pBuffer, otherString.m_pStringData->pBuffer);
}

int String::NumericCompareInsensitive(const char* otherText) const
{
  return Y_stricmp(m_pStringData->pBuffer, otherText);
}

bool String::StartsWith(const char* compareString, bool caseSensitive /*= true*/) const
{
  uint32 compareStringLength = Y_strlen(compareString);
  if (compareStringLength > m_pStringData->StringLength)
    return false;

  return (caseSensitive) ? (Y_strncmp(compareString, m_pStringData->pBuffer, compareStringLength) == 0) :
                           (Y_strnicmp(compareString, m_pStringData->pBuffer, compareStringLength) == 0);
}

bool String::StartsWith(const String& compareString, bool caseSensitive /*= true*/) const
{
  uint32 compareStringLength = compareString.GetLength();
  if (compareStringLength > m_pStringData->StringLength)
    return false;

  return (caseSensitive) ?
           (Y_strncmp(compareString.m_pStringData->pBuffer, m_pStringData->pBuffer, compareStringLength) == 0) :
           (Y_strnicmp(compareString.m_pStringData->pBuffer, m_pStringData->pBuffer, compareStringLength) == 0);
}

bool String::EndsWith(const char* compareString, bool caseSensitive /*= true*/) const
{
  uint32 compareStringLength = Y_strlen(compareString);
  if (compareStringLength > m_pStringData->StringLength)
    return false;

  uint32 startOffset = m_pStringData->StringLength - compareStringLength;
  return (caseSensitive) ? (Y_strncmp(compareString, m_pStringData->pBuffer + startOffset, compareStringLength) == 0) :
                           (Y_strnicmp(compareString, m_pStringData->pBuffer + startOffset, compareStringLength) == 0);
}

bool String::EndsWith(const String& compareString, bool caseSensitive /*= true*/) const
{
  uint32 compareStringLength = compareString.GetLength();
  if (compareStringLength > m_pStringData->StringLength)
    return false;

  uint32 startOffset = m_pStringData->StringLength - compareStringLength;
  return (caseSensitive) ? (Y_strncmp(compareString.m_pStringData->pBuffer, m_pStringData->pBuffer + startOffset,
                                      compareStringLength) == 0) :
                           (Y_strnicmp(compareString.m_pStringData->pBuffer, m_pStringData->pBuffer + startOffset,
                                       compareStringLength) == 0);
}

void String::Clear()
{
  if (m_pStringData == &s_EmptyStringData)
    return;

  // Do we have a shared buffer? If so, cancel it and allocate a new one when we need to.
  // Otherwise, clear the current buffer.
  if (StringDataIsShared(m_pStringData) || m_pStringData->ReadOnly)
  {
    // replace with empty string data
    Obliterate();
  }
  else
  {
  // in debug, zero whole string, in release, zero only the first character
#if Y_BUILD_CONFIG_DEBUG
    Y_memzero(m_pStringData->pBuffer, m_pStringData->BufferSize);
#else
    m_pStringData->pBuffer[0] = '\0';
#endif
    m_pStringData->StringLength = 0;
  }
}

void String::Obliterate()
{
  if (m_pStringData == &s_EmptyStringData)
    return;

  // Force a release of the current buffer.
  StringDataRelease(m_pStringData);
  m_pStringData = const_cast<StringData*>(&s_EmptyStringData);
}

int32 String::Find(char c, uint32 Offset /* = 0*/) const
{
  DebugAssert(Offset <= m_pStringData->StringLength);
  char* pAt = Y_strchr(m_pStringData->pBuffer + Offset, c);
  return (pAt == NULL) ? -1 : int32(pAt - m_pStringData->pBuffer);
}

int32 String::RFind(char c, uint32 Offset /* = 0*/) const
{
  DebugAssert(Offset <= m_pStringData->StringLength);
  char* pAt = Y_strrchr(m_pStringData->pBuffer + Offset, c);
  return (pAt == NULL) ? -1 : int32(pAt - m_pStringData->pBuffer);
}

int32 String::Find(const char* str, uint32 Offset /* = 0 */) const
{
  DebugAssert(Offset <= m_pStringData->StringLength);
  char* pAt = Y_strstr(m_pStringData->pBuffer + Offset, str);
  return (pAt == NULL) ? -1 : int32(pAt - m_pStringData->pBuffer);
}

void String::Reserve(uint32 newReserve, bool Force /* = false */)
{
  DebugAssert(!Force || newReserve >= m_pStringData->StringLength);

  uint32 newSize = (Force) ? newReserve + 1 : Max(newReserve + 1, m_pStringData->BufferSize);
  StringData* pNewStringData;

  if (StringDataIsShared(m_pStringData) || m_pStringData->ReadOnly)
  {
    pNewStringData = StringDataClone(m_pStringData, newSize, false);
    StringDataRelease(m_pStringData);
    m_pStringData = pNewStringData;
  }
  else
  {
    // skip if smaller, and not forced
    if (newSize <= m_pStringData->BufferSize && !Force)
      return;

    // if we are the only owner of the buffer, we can simply realloc it
    if (m_pStringData->ReferenceCount == 1)
    {
      // do realloc and update pointer
      m_pStringData = StringDataReallocate(m_pStringData, newSize);
    }
    else
    {
      // clone and release old
      pNewStringData = StringDataClone(m_pStringData, newSize, false);
      StringDataRelease(m_pStringData);
      m_pStringData = pNewStringData;
    }
  }
}

void String::Resize(uint32 newSize, char fillerCharacter /* = ' ' */, bool skrinkIfSmaller /* = false */)
{
  StringData* pNewStringData;

  // if going larger, or we don't own the buffer, realloc
  if (StringDataIsShared(m_pStringData) || m_pStringData->ReadOnly || newSize >= m_pStringData->BufferSize)
  {
    pNewStringData = StringDataClone(m_pStringData, newSize + 1, true);
    StringDataRelease(m_pStringData);
    m_pStringData = pNewStringData;

    if (m_pStringData->StringLength < newSize)
      std::memset(m_pStringData->pBuffer + m_pStringData->StringLength, (byte)fillerCharacter,
                  m_pStringData->BufferSize - m_pStringData->StringLength - 1);

    m_pStringData->StringLength = newSize;
  }
  else
  {
    // owns the buffer, and going smaller
    DebugAssert(newSize < m_pStringData->BufferSize);

    // update length and terminator
#if Y_BUILD_CONFIG_DEBUG
    Y_memzero(m_pStringData->pBuffer + newSize, m_pStringData->BufferSize - newSize);
#else
    m_pStringData->pBuffer[newSize] = 0;
#endif
    m_pStringData->StringLength = newSize;

    // shrink if requested
    if (skrinkIfSmaller)
      Shrink(false);
  }
}

void String::UpdateSize()
{
  EnsureOwnWritableCopy();
  m_pStringData->StringLength = Y_strlen(m_pStringData->pBuffer);
}

void String::Shrink(bool Force /* = false */)
{
  // only shrink of we own the buffer, or forced
  if (Force || m_pStringData->ReferenceCount == 1)
    Reserve(m_pStringData->StringLength);
}

String String::SubString(int32 Offset, int32 Count /* = -1 */) const
{
  String returnStr;
  returnStr.AppendSubString(*this, Offset, Count);
  return returnStr;
}

void String::Erase(int32 Offset, int32 Count /* = INT_MAX */)
{
  uint32 currentLength = m_pStringData->StringLength;

  // calc real offset
  uint32 realOffset;
  if (Offset < 0)
    realOffset = (uint32)Max((int32)0, (int32)currentLength + Offset);
  else
    realOffset = Min((uint32)Offset, currentLength);

  // calc real count
  uint32 realCount;
  if (Count < 0)
    realCount = Min(currentLength - realOffset, (uint32)Max((int32)0, (int32)currentLength + Count));
  else
    realCount = Min(currentLength - realOffset, (uint32)Count);

  // Fastpath: offset == 0, count < 0, wipe whole string.
  if (realOffset == 0 && realCount == currentLength)
  {
    Clear();
    return;
  }

  // Fastpath: offset >= 0, count < 0, wipe everything after offset + count
  if ((realOffset + realCount) == m_pStringData->StringLength)
  {
    m_pStringData->StringLength -= realCount;
#if Y_BUILD_CONFIG_DEBUG
    Y_memzero(m_pStringData->pBuffer + m_pStringData->StringLength,
              m_pStringData->BufferSize - m_pStringData->StringLength);
#else
    m_pStringData->pBuffer[m_pStringData->StringLength] = 0;
#endif
  }
  // Slowpath: offset >= 0, count < length
  else
  {
    uint32 afterEraseBlock = m_pStringData->StringLength - realOffset - realCount;
    DebugAssert(afterEraseBlock > 0);

    std::memmove(m_pStringData->pBuffer + Offset, m_pStringData->pBuffer + realOffset + realCount, afterEraseBlock);
    m_pStringData->StringLength = m_pStringData->StringLength - realCount;

#if Y_BUILD_CONFIG_DEBUG
    Y_memzero(m_pStringData->pBuffer + m_pStringData->StringLength,
              m_pStringData->BufferSize - m_pStringData->StringLength);
#else
    m_pStringData->pBuffer[m_pStringData->StringLength] = 0;
#endif
  }
}

uint32 String::Replace(char searchCharacter, char replaceCharacter)
{
  uint32 nReplacements = 0;
  char* pCurrent = Y_strchr(m_pStringData->pBuffer, searchCharacter);
  while (pCurrent != NULL)
  {
    if ((nReplacements++) == 0)
      EnsureOwnWritableCopy();

    *pCurrent = replaceCharacter;
    pCurrent = Y_strchr(pCurrent + 1, searchCharacter);
  }

  return nReplacements;
}

uint32 String::Replace(const char* searchString, const char* replaceString)
{
  uint32 nReplacements = 0;
  uint32 searchStringLength = Y_strlen(searchString);

#if 0
    uint32 replaceStringLength = Y_strlen(replaceString);
    int32 lengthDifference = (int32)replaceStringLength - (int32)searchStringLength;

    char *pCurrent = Y_strchr(m_pStringData->pBuffer, searchString);
    while (pCurrent != NULL)
    {
        if ((nReplacements++) == 0)
        {
            if (lengthDifference > 0)
                EnsureRemainingSpace(lengthDifference);
            else
                EnsureOwnCopy();
        }
        else if (lengthDifference > 0)
            EnsureRemainingSpace(lengthDifference);
    }
#endif

  // TODO: Fastpath if strlen(searchString) == strlen(replaceString)

  String tempString;
  char* pStart = m_pStringData->pBuffer;
  char* pCurrent = Y_strstr(pStart, searchString);
  char* pLast = NULL;
  while (pCurrent != NULL)
  {
    if ((nReplacements++) == 0)
      tempString.Reserve(m_pStringData->StringLength);

    tempString.AppendSubString(*this, int32(pStart - pCurrent), int32(pStart - pCurrent - 1));
    tempString.AppendString(replaceString);
    pLast = pCurrent + searchStringLength;
    nReplacements++;

    pCurrent = Y_strstr(pLast, searchString);
  }

  if (pLast != NULL)
    tempString.AppendSubString(*this, int32(pLast - pStart));

  if (nReplacements)
    Swap(tempString);

  return nReplacements;
}

void String::ToLower()
{
  // fixme for utf8
  EnsureOwnWritableCopy();
  Y_strlwr(m_pStringData->pBuffer, m_pStringData->StringLength);
}

void String::ToUpper()
{
  // fixme for utf8
  EnsureOwnWritableCopy();
  Y_strupr(m_pStringData->pBuffer, m_pStringData->StringLength);
}

void String::LStrip(const char* szStripCharacters /* = " " */)
{
  uint32 stripCharactersLen = Y_strlen(szStripCharacters);
  uint32 removeCount = 0;
  uint32 i = 0;
  uint32 j;

  // for each character in str
  for (i = 0; i < m_pStringData->StringLength; i++)
  {
    char ch = m_pStringData->pBuffer[i];

    // if it exists in szStripCharacters
    for (j = 0; j < stripCharactersLen; j++)
    {
      if (ch == szStripCharacters[j])
      {
        removeCount++;
        goto OUTER;
      }
    }

    // not found, exit
    break;
  OUTER:
    continue;
  }

  // chars to remove?
  if (removeCount > 0)
    Erase(0, removeCount);
}

void String::RStrip(const char* szStripCharacters /* = " " */)
{
  uint32 stripCharactersLen = Y_strlen(szStripCharacters);
  uint32 removeCount = 0;
  uint32 i = 0;
  uint32 j;

  // for each character in str
  for (i = 0; i < m_pStringData->StringLength; i++)
  {
    char ch = m_pStringData->pBuffer[m_pStringData->StringLength - i - 1];

    // if it exists in szStripCharacters
    for (j = 0; j < stripCharactersLen; j++)
    {
      if (ch == szStripCharacters[j])
      {
        removeCount++;
        goto OUTER;
      }
    }

    // not found, exit
    break;
  OUTER:
    continue;
  }

  // chars to remove?
  if (removeCount > 0)
    Erase(m_pStringData->StringLength - removeCount);
}

void String::Strip(const char* szStripCharacters /* = " " */)
{
  RStrip(szStripCharacters);
  LStrip(szStripCharacters);
}

String String::FromFormat(const char* FormatString, ...)
{
  String returnStr;
  va_list ap;

  va_start(ap, FormatString);
  returnStr.FormatVA(FormatString, ap);
  va_end(ap);

  return returnStr;
}
