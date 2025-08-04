#pragma once
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Common.h"
#include "YBaseLib/TextReader.h" // <-- for TEXT_ENCODING

class String;

class TextWriter
{
public:
  TextWriter(ByteStream* pStream, TEXT_ENCODING eSourceEncoding = TEXT_ENCODING_UTF8);

  void Write(const char* szCharacters);
  void Write(const char* szCharacters, uint32 cbCharacters);
  void Write(const String& strCharacters);
  void WriteFormattedString(const char* szFormat, ...);
  void WriteLine(const char* szCharacters);
  void WriteLine(const char* szCharacters, uint32 cbCharacters);
  void WriteLine(const String& strCharacters);
  void WriteFormattedLine(const char* szFormat, ...);
  void WriteWithLineNumbers(const char* str);
  void WriteWithLineNumbers(const char* str, uint32 strLength);
  bool WriteStream(ByteStream* pStream, bool restorePosition = false, bool rewindStream = false);
  bool WriteStreamWithLineNumbers(ByteStream* pStream, bool restorePosition = false, bool rewindStream = false);

  TextWriter& operator<<(const char* szCharacters);
  TextWriter& operator<<(const String& strCharacters);

private:
  void _Write(const char* szCharacters, uint32 nCharacters);

  ByteStream* m_pStream;
  TEXT_ENCODING m_eSourceEncoding;
  DeclareNonCopyable(TextWriter);
};
