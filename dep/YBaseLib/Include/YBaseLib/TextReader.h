#pragma once
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Common.h"

class String;

enum TEXT_ENCODING
{
  TEXT_ENCODING_UTF8,
  TEXT_ENCODING_UTF16,
  TEXT_ENCODING_UTF32,
  TEXT_ENCODING_AUTODETECT,
};

class TextReader
{
public:
  TextReader(ByteStream* pStream, TEXT_ENCODING eSourceEncoding = TEXT_ENCODING_UTF8);

  bool ReadCharacter(char* pDestination);
  bool ReadLine(char* pDestination, uint32 cbDestination, uint32* pLineLength = NULL, bool* pEndOfLine = NULL);
  bool ReadLine(String* pDestString);

private:
  ByteStream* m_pStream;
  TEXT_ENCODING m_eSourceEncoding;
  DeclareNonCopyable(TextReader);
};
