#pragma once
#include "YBaseLib/Common.h"

#ifdef HAVE_LIBXML2
#include "YBaseLib/ByteStream.h"

typedef struct _xmlTextWriter* xmlTextWriterPtr;
typedef struct _xmlOutputBuffer* xmlOutputBufferPtr;

class XMLWriter
{
public:
  XMLWriter();
  ~XMLWriter();

  bool InErrorState() const { return m_bErrorState; }

  bool Create(ByteStream* pOutputStream);
  void Close();

  void WriteEmptyElement(const char* elementName);
  void StartElement(const char* elementName);
  void EndElement();

  void WriteAttribute(const char* attributeName, const char* attributeValue);
  void WriteAttributef(const char* attributeName, const char* format, ...);

  // does special character encoding
  void WriteString(const char* data);

  // writes as cdata
  void WriteCDATA(const char* data);

  // ignores it
  void WriteRaw(const char* data, uint32 dataLength);

  // writes data as base64 encoded
  void WriteBase64EncodedData(const void* pData, uint32 dataLength);

  //     bool MoveToFirstAttribute();
  //     bool MoveToNextAttribute();
  //     bool MoveToAttribute(const char *AttributeName);
  //     bool MoveToElement();
  //
  //     bool NextToken();
  //     XMLREADER_TOKEN_TYPE GetTokenType() const { return m_eCurrentNodeType; }
  //     const char *GetFileName() const { return m_szFileName; }
  //
  //     const char *GetNodeName();
  //     const char *GetNodeValue();
  //     const char *GetNodeValueString();               // same as GetNodeValue, except will never return null
  //
  //     const char *FetchAttribute(const char *AttributeName);
  //     const char *FetchAttributeString(const char *AttributeName);
  //
  //     bool InErrorState() const { return m_bErrorState; }
  //
  //     void PrintWarning(const char *Format, ...);
  //     void PrintError(const char *Format, ...);
  //
  //     bool SkipToElement(const char *ElementName);
  //     bool SkipCurrentElement();
  //
  //     int32 Select(const char *SelectionString, bool AbortOnError = true);

private:
  ByteStream* m_pOutputStream;
  xmlTextWriterPtr m_pWriter;
  xmlOutputBufferPtr m_pOutputBuffer;
  bool m_bErrorState;
};

#endif // HAVE_LIBXML2
