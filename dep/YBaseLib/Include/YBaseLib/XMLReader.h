#pragma once
#include "YBaseLib/Common.h"

#ifdef HAVE_LIBXML2

#include "YBaseLib/ByteStream.h"
#include "YBaseLib/String.h"

typedef struct _xmlTextReader* xmlTextReaderPtr;

enum XMLREADER_TOKEN_TYPE
{
  XMLREADER_TOKEN_ELEMENT,
  XMLREADER_TOKEN_ATTRIBUTE,
  XMLREADER_TOKEN_TEXT,
  XMLREADER_TOKEN_END_ELEMENT,
  XMLREADER_TOKEN_COUNT,
};

class XMLReader
{
public:
  XMLReader();
  ~XMLReader();

  bool Create(ByteStream* pInputStream, const char* FileName);

  bool MoveToFirstAttribute();
  bool MoveToNextAttribute();
  bool MoveToAttribute(const char* AttributeName);
  bool MoveToElement();

  bool NextToken();
  XMLREADER_TOKEN_TYPE GetTokenType() const { return m_eCurrentNodeType; }
  const char* GetFileName() const { return m_szFileName; }

  const char* GetNodeName();
  const char* GetNodeValue();
  const char* GetNodeValueString(); // same as GetNodeValue, except will never return null

  const char* FetchAttribute(const char* AttributeName);
  const char* FetchAttributeString(const char* AttributeName);
  const char* FetchAttributeDefault(const char* attributeName, const char* defaultValue);

  String GetElementText(bool skipElement = false);

  bool IsEmptyElement();

  bool InErrorState() const { return m_bErrorState; }

  void PrintWarning(const char* Format, ...);
  void PrintError(const char* Format, ...);

  bool SkipToElement(const char* ElementName);
  bool SkipCurrentElement();

  int32 Select(const char* SelectionString, bool AbortOnError = true);
  int32 SelectAttribute(const char* AttributeName, const char* SelectionString, bool AbortOnError = true);

  bool ExpectEndOfElement();
  bool ExpectEndOfElementName(const char* nodeName);

private:
  ByteStream* m_pInputStream;
  const char* m_szFileName;
  xmlTextReaderPtr m_pReader;
  XMLREADER_TOKEN_TYPE m_eCurrentNodeType;
  bool m_bErrorState;
};

#endif // HAVE_LIBXML2
