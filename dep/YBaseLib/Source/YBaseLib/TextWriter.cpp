#include "YBaseLib/TextWriter.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/String.h"
#include <cstdio>

TextWriter::TextWriter(ByteStream* pStream, TEXT_ENCODING eSourceEncoding)
  : m_pStream(pStream), m_eSourceEncoding(eSourceEncoding)
{
}

void TextWriter::Write(const char* szCharacters)
{
  uint32 nCharacters = Y_strlen(szCharacters);
  if (nCharacters > 0)
    _Write(szCharacters, nCharacters);
}

void TextWriter::Write(const char* szCharacters, uint32 cbCharacters)
{
  if (cbCharacters > 0)
    _Write(szCharacters, cbCharacters);
}

void TextWriter::Write(const String& strCharacters)
{
  if (strCharacters.GetLength() > 0)
    _Write(strCharacters.GetCharArray(), strCharacters.GetLength());
}

void TextWriter::WriteFormattedString(const char* szFormat, ...)
{
  char Buffer[1024];
  va_list ap;
  va_start(ap, szFormat);
  Y_vsnprintf(Buffer, countof(Buffer), szFormat, ap);
  va_end(ap);

  Write(Buffer);
}

void TextWriter::WriteLine(const char* szCharacters)
{
  Write(szCharacters);
  _Write("\n", 1);
}

void TextWriter::WriteLine(const char* szCharacters, uint32 cbCharacters)
{
  Write(szCharacters, cbCharacters);
  _Write("\n", 1);
}

void TextWriter::WriteLine(const String& strCharacters)
{
  Write(strCharacters);
  _Write("\n", 1);
}

void TextWriter::WriteFormattedLine(const char* szFormat, ...)
{
  char Buffer[1024];
  va_list ap;
  va_start(ap, szFormat);
  Y_vsnprintf(Buffer, countof(Buffer), szFormat, ap);
  va_end(ap);

  Write(Buffer);
  Write("\n");
}

void TextWriter::WriteWithLineNumbers(const char* str)
{
  uint32 strLength = Y_strlen(str);
  if (strLength > 0)
    WriteWithLineNumbers(str, strLength);
}

void TextWriter::WriteWithLineNumbers(const char* str, uint32 strLength)
{
  // count the number of lines
  uint32 lineCount = 1;
  const char* pCurrent = str;
  const char* pEnd = str + strLength;
  while (pCurrent != pEnd)
  {
    if (*pCurrent == '\n')
      lineCount++;
    pCurrent++;
  }

  // determine the number of characters to reserve for the line numbers
  uint32 lineNumberSpaces = 0;
  uint32 temp = lineCount;
  while (temp > 0)
  {
    lineNumberSpaces++;
    temp /= 10;
  }

  // generate format string
  TinyString formatString;
  formatString.Format("%%%uu |", lineNumberSpaces);

  // start appending lines
  SmallString lineString;
  uint32 lineNumber = 1;
  pCurrent = str;
  while (pCurrent != pEnd)
  {
    char ch = *pCurrent++;
    if (ch == '\r')
      continue;

    if (ch == '\n')
    {
      WriteFormattedString(formatString, lineNumber);
      Write(lineString);
      Write("\n");
      lineNumber++;
      lineString.Clear();
      continue;
    }

    lineString.AppendCharacter(ch);
  }

  // write remaining line
  WriteFormattedString(formatString, lineNumber);
  Write(lineString);
  Write("\n");
}

bool TextWriter::WriteStream(ByteStream* pStream, bool restorePosition /*= false*/, bool rewindStream /*= false*/)
{
  char chunk[1024];
  uint64 oldPosition = pStream->GetPosition();
  if (rewindStream && !pStream->SeekAbsolute(0))
    return false;

  while (!pStream->InErrorState())
  {
    uint32 bytes = pStream->Read(chunk, sizeof(chunk));
    if (bytes == 0)
      break;

    Write(chunk, bytes);
  }

  if (restorePosition && !pStream->InErrorState())
    pStream->SeekAbsolute(oldPosition);

  return !pStream->InErrorState();
}

bool TextWriter::WriteStreamWithLineNumbers(ByteStream* pStream, bool restorePosition /*= false*/,
                                            bool rewindStream /*= false*/)
{
  uint64 oldPosition = pStream->GetPosition();
  if (rewindStream && !pStream->SeekAbsolute(0))
    return false;

  // get data size
  uint32 dataSize = (uint32)(pStream->GetSize() - pStream->GetPosition());
  char* temp = new char[dataSize];
  if (!pStream->Read2(temp, dataSize))
  {
    delete[] temp;
    return false;
  }

  WriteWithLineNumbers(temp, dataSize);
  delete[] temp;

  if (restorePosition && !pStream->InErrorState())
    pStream->SeekAbsolute(oldPosition);

  return !pStream->InErrorState();
}

TextWriter& TextWriter::operator<<(const char* szCharacters)
{
  Write(szCharacters);
  return *this;
}

TextWriter& TextWriter::operator<<(const String& strCharacters)
{
  Write(strCharacters);
  return *this;
}

void TextWriter::_Write(const char* szCharacters, uint32 nCharacters)
{
  uint32 FirstToWrite = 0;
  uint32 nToWrite = 0;
  uint32 i;

  for (i = 0; i < nCharacters; i++)
  {
    if (szCharacters[i] == '\n')
    {
      if (nToWrite > 0)
      {
        if (m_pStream->Write(szCharacters + FirstToWrite, nToWrite) != nToWrite)
        {
          printf("TextWriter::_Write: m_pStream->Write failed.\n");
          return;
        }
      }

#ifdef Y_PLATFORM_WINDOWS
      static const char* NewLineCharacters = "\r\n";
      uint32 NewLineCharactersLength = 2;
#else
      static const char* NewLineCharacters = "\n";
      uint32 NewLineCharactersLength = 1;
#endif
      if (m_pStream->Write(NewLineCharacters, NewLineCharactersLength) != NewLineCharactersLength)
      {
        printf("TextWriter::_Write: m_pStream->Write failed.\n");
        return;
      }

      FirstToWrite = i + 1;
      nToWrite = 0;
    }
    else
    {
      nToWrite++;
    }
  }

  if (nToWrite > 0)
  {
    if (m_pStream->Write(szCharacters + FirstToWrite, nToWrite) != nToWrite)
    {
      printf("TextWriter::_Write: m_pStream->Write failed.\n");
      return;
    }
  }
}
