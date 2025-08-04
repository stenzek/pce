#include "YBaseLib/XMLReader.h"

#ifdef HAVE_LIBXML2
#include "YBaseLib/Assert.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/Log.h"
#include <libxml/threads.h>
#include <libxml/xmlreader.h>
Log_SetChannel(XMLReader);

// Ensure libxml gets pulled in
#if Y_COMPILER_MSVC
#pragma comment(lib, "libxml2.lib")
#endif

static void XMLReaderErrorFunction(void* arg, const char* msg, xmlParserSeverities severity,
                                   xmlTextReaderLocatorPtr locator)
{
  const char* FileName = ((XMLReader*)arg)->GetFileName();
  int line = xmlTextReaderLocatorLineNumber(locator);

  if (severity == XML_PARSER_SEVERITY_VALIDITY_WARNING || severity == XML_PARSER_SEVERITY_WARNING)
    Log_WarningPrintf("%s:%d: %s", FileName, line, msg);
  else
    Log_ErrorPrintf("%s:%d: %s", FileName, line, msg);
}

static int XMLReaderReadCallback(void* context, char* buffer, int len)
{
  uint32 got = ((ByteStream*)context)->Read(buffer, len);
  return got;
}

static int XMLReaderCloseCallback(void* context)
{
  return 0;
}

// hack to initialize the libxml library
struct libxml2Initializer
{
  libxml2Initializer() { xmlInitParser(); }
  ~libxml2Initializer() { xmlCleanupParser(); }
};
static libxml2Initializer s_libxml2Initializer;

XMLReader::XMLReader()
{
  m_pInputStream = NULL;
  m_szFileName = NULL;
  m_pReader = NULL;
  m_eCurrentNodeType = XMLREADER_TOKEN_COUNT;
  m_bErrorState = true;
}

XMLReader::~XMLReader()
{
  if (m_pReader != NULL)
    xmlFreeTextReader(m_pReader);
}

bool XMLReader::Create(ByteStream* pInputStream, const char* FileName)
{
  if (m_pReader != NULL)
    xmlFreeTextReader(m_pReader);

  m_pInputStream = pInputStream;
  m_szFileName = FileName;
  m_eCurrentNodeType = XMLREADER_TOKEN_COUNT;
  m_bErrorState = true;

  m_pReader = xmlReaderForIO(XMLReaderReadCallback, XMLReaderCloseCallback, (void*)pInputStream, m_szFileName, NULL,
                             XML_PARSE_NOCDATA);
  if (m_pReader == NULL)
  {
    Log_ErrorPrintf("XMLReader::Create: xmlReaderForIO(%s) failed.", FileName);
    return false;
  }

  xmlTextReaderSetErrorHandler(m_pReader, XMLReaderErrorFunction, (void*)this);
  m_bErrorState = false;
  return true;
}

bool XMLReader::NextToken()
{
  if (m_bErrorState)
    return false;

  for (;;)
  {
    int ret = xmlTextReaderRead(m_pReader);
    if (ret <= 0)
    {
      if (ret != 0)
        m_bErrorState = true;

      return false;
    }

    int type = xmlTextReaderNodeType(m_pReader);
    switch (type)
    {
      case XML_READER_TYPE_ELEMENT:
        m_eCurrentNodeType = XMLREADER_TOKEN_ELEMENT;
        break;

      case XML_READER_TYPE_END_ELEMENT:
        m_eCurrentNodeType = XMLREADER_TOKEN_END_ELEMENT;
        break;

      case XML_READER_TYPE_TEXT:
        m_eCurrentNodeType = XMLREADER_TOKEN_TEXT;
        break;

      case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
      case XML_READER_TYPE_COMMENT:
        continue;

      default:
      {
        m_bErrorState = true;
        Log_ErrorPrintf("Unhandled xmlreader node type %d", type);
        return false;
      }
      break;
    }

    return true;
  }
}

const char* XMLReader::GetNodeName()
{
  return (const char*)xmlTextReaderConstName(m_pReader);
}

const char* XMLReader::GetNodeValue()
{
  return (const char*)xmlTextReaderConstValue(m_pReader);
}

const char* XMLReader::GetNodeValueString()
{
  const char* v = (const char*)xmlTextReaderConstValue(m_pReader);
  return (v != NULL) ? v : "";
}

bool XMLReader::MoveToFirstAttribute()
{
  int ret = xmlTextReaderMoveToFirstAttribute(m_pReader);
  if (ret == 1)
  {
    m_eCurrentNodeType = XMLREADER_TOKEN_ATTRIBUTE;
    return true;
  }

  if (ret < 0)
    m_bErrorState = true;

  return false;
}

bool XMLReader::MoveToNextAttribute()
{
  int ret = xmlTextReaderMoveToNextAttribute(m_pReader);
  if (ret == 1)
  {
    m_eCurrentNodeType = XMLREADER_TOKEN_ATTRIBUTE;
    return true;
  }

  if (ret < 0)
    m_bErrorState = true;

  return false;
}

bool XMLReader::MoveToAttribute(const char* AttributeName)
{
  int ret = xmlTextReaderMoveToAttribute(m_pReader, BAD_CAST AttributeName);
  if (ret == 1)
  {
    m_eCurrentNodeType = XMLREADER_TOKEN_ATTRIBUTE;
    return true;
  }

  if (ret < 0)
    m_bErrorState = true;

  return false;
}

bool XMLReader::MoveToElement()
{
  if (m_eCurrentNodeType == XMLREADER_TOKEN_ELEMENT)
    return true;

  int ret = xmlTextReaderMoveToElement(m_pReader);
  if (ret >= 0)
  {
    m_eCurrentNodeType = XMLREADER_TOKEN_ELEMENT;
    return true;
  }
  else
  {
    m_bErrorState = true;
  }

  return false;
}

const char* XMLReader::FetchAttribute(const char* AttributeName)
{
  return (MoveToAttribute(AttributeName)) ? GetNodeValue() : NULL;
}

const char* XMLReader::FetchAttributeString(const char* AttributeName)
{
  return (MoveToAttribute(AttributeName)) ? GetNodeValueString() : "";
}

const char* XMLReader::FetchAttributeDefault(const char* attributeName, const char* defaultValue)
{
  return (MoveToAttribute(attributeName)) ? GetNodeValueString() : defaultValue;
}

bool XMLReader::IsEmptyElement()
{
  DebugAssert(m_eCurrentNodeType == XMLREADER_TOKEN_ELEMENT || m_eCurrentNodeType == XMLREADER_TOKEN_ATTRIBUTE);
  if (m_eCurrentNodeType == XMLREADER_TOKEN_ATTRIBUTE)
  {
    if (!MoveToElement())
      return false;
  }

  return (xmlTextReaderIsEmptyElement(m_pReader) == 1);
}

void XMLReader::PrintWarning(const char* Format, ...)
{
  char buf[512];

  va_list ap;
  va_start(ap, Format);
  Y_vsnprintf(buf, countof(buf), Format, ap);
  va_end(ap);

  uint32 line = xmlTextReaderGetParserLineNumber(m_pReader);
  uint32 col = xmlTextReaderGetParserColumnNumber(m_pReader);

  Log_WarningPrintf("%s:%u:%u: %s", m_szFileName, line, col, buf);
}

void XMLReader::PrintError(const char* Format, ...)
{
  char buf[512];

  va_list ap;
  va_start(ap, Format);
  Y_vsnprintf(buf, countof(buf), Format, ap);
  va_end(ap);

  uint32 line = xmlTextReaderGetParserLineNumber(m_pReader);
  uint32 col = xmlTextReaderGetParserColumnNumber(m_pReader);

  Log_ErrorPrintf("%s:%u:%u: %s", m_szFileName, line, col, buf);
}

bool XMLReader::SkipToElement(const char* ElementName)
{
  for (;;)
  {
    if (m_eCurrentNodeType == XMLREADER_TOKEN_ELEMENT && !Y_stricmp(GetNodeName(), ElementName))
      return true;

    if (!NextToken())
      return false;
  }
}

bool XMLReader::SkipCurrentElement()
{
  // if we're on the attribute node, switch back to the element node.
  if (m_eCurrentNodeType == XMLREADER_TOKEN_ATTRIBUTE)
  {
    if (xmlTextReaderMoveToElement(m_pReader) < 0)
      return false;

    m_eCurrentNodeType = XMLREADER_TOKEN_ELEMENT;
  }

  // if we're an empty node, exit out, since the next node will be the next one in the tree anyway.
  if (xmlTextReaderIsEmptyElement(m_pReader) == 1)
    return true;

  uint32 depth = 1;
  for (;;)
  {
    if (!NextToken())
      return false;

    if (m_eCurrentNodeType == XMLREADER_TOKEN_ELEMENT)
    {
      int r = xmlTextReaderIsEmptyElement(m_pReader);
      if (r == 1)
        continue;
      else if (r == 0)
      {
        depth++;
        continue;
      }

      return false;
    }
    else if (m_eCurrentNodeType == XMLREADER_TOKEN_END_ELEMENT)
    {
      depth--;
      if (depth == 0)
        return true;
    }
  }
}

int32 XMLReader::Select(const char* SelectionString, bool AbortOnError /* = true */)
{
  int32 s = Y_selectstring(SelectionString, GetNodeName());
  if (s < 0 && AbortOnError)
  {
    PrintError("expected '%s', got '%s'", SelectionString, GetNodeName());
    m_bErrorState = true;
  }

  return s;
}

int32 XMLReader::SelectAttribute(const char* AttributeName, const char* SelectionString, bool AbortOnError /* = true */)
{
  const char* attributeValue = FetchAttributeString(AttributeName);
  int32 s = Y_selectstring(SelectionString, attributeValue);
  if (s < 0 && AbortOnError)
  {
    PrintError("for attribute '%s': expected '%s', got '%s'", AttributeName, SelectionString, attributeValue);
    m_bErrorState = true;
  }

  return s;
}

String XMLReader::GetElementText(bool skipElement /*= false*/)
{
  String retString;
  const char* nodeValue;

  if (!m_bErrorState)
  {
    DebugAssert(m_eCurrentNodeType == XMLREADER_TOKEN_ATTRIBUTE || m_eCurrentNodeType == XMLREADER_TOKEN_ELEMENT ||
                m_eCurrentNodeType == XMLREADER_TOKEN_TEXT);
    if (m_eCurrentNodeType == XMLREADER_TOKEN_TEXT ||
        (MoveToElement() && NextToken() && m_eCurrentNodeType == XMLREADER_TOKEN_TEXT))
    {
      nodeValue = (const char*)xmlTextReaderConstValue(m_pReader);
      if (nodeValue != NULL)
        retString = nodeValue;

      if (skipElement)
        SkipCurrentElement();
    }
  }

  return retString;
}

bool XMLReader::ExpectEndOfElement()
{
  if (m_eCurrentNodeType != XMLREADER_TOKEN_END_ELEMENT)
  {
    PrintError("expected end of element, current node is not end of element.");
    m_bErrorState = true;
    return false;
  }

  return true;
}

bool XMLReader::ExpectEndOfElementName(const char* nodeName)
{
  if (m_eCurrentNodeType != XMLREADER_TOKEN_END_ELEMENT)
  {
    PrintError("expected end of element, current node is not end of element.");
    m_bErrorState = true;
    return false;
  }

  const char* actualNodeName = (const char*)xmlTextReaderConstName(m_pReader);
  if (actualNodeName == nullptr || Y_stricmp(nodeName, actualNodeName) != 0)
  {
    PrintError("expecting end of element type '%s', got '%s'", nodeName,
               (actualNodeName != nullptr) ? actualNodeName : "(null)");
    m_bErrorState = true;
    return false;
  }

  return true;
}

#endif // HAVE_LIBXML2
