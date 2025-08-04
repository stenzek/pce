#include "YBaseLib/TextReader.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/String.h"
#include <cstdio>

TextReader::TextReader(ByteStream* pStream, TEXT_ENCODING eSourceEncoding)
  : m_pStream(pStream), m_eSourceEncoding(eSourceEncoding)
{
}

bool TextReader::ReadLine(char* pDestination, uint32 cbDestination, uint32* pLineLength /* = NULL */,
                          bool* pEndOfLine /* = NULL */)
{
  char LastCharacter = 0;
  char Character = 0;
  bool EndOfLine = false;
  bool EndOfFile = false;
  uint32 DestinationLength = 0;
  for (;;)
  {
    if ((DestinationLength + 1) >= cbDestination)
      break;

    if (!m_pStream->ReadByte((byte*)&Character))
    {
      EndOfLine = true;
      EndOfFile = true;
      break;
    }

    if (Character == '\n')
    {
      // strip carriage returns
      if (LastCharacter == '\r')
        DestinationLength--;

      EndOfLine = true;
      break;
    }

    pDestination[DestinationLength++] = Character;
    LastCharacter = Character;
  }

  pDestination[DestinationLength] = 0;
  if (pLineLength != NULL)
    *pLineLength = DestinationLength;
  if (pEndOfLine != NULL)
    *pEndOfLine = EndOfLine;

  if (DestinationLength == 0)
    return !EndOfFile;
  else
    return true;
}

bool TextReader::ReadLine(String* pDestString)
{
  char Buffer[256 + 1];
  pDestString->Clear();

  for (;;)
  {
    uint32 LineLength;
    bool EndOfLine;
    bool Result = ReadLine(Buffer, countof(Buffer), &LineLength, &EndOfLine);
    if (!Result)
      return (pDestString->GetLength() > 0) ? true : false;

    if (LineLength > 0)
      pDestString->AppendSubString(Buffer, 0, LineLength);

    if (EndOfLine)
      return true;
  }
}
