#include "YBaseLib/XMLWriter.h"

#ifdef HAVE_LIBXML2
#include "YBaseLib/Assert.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Memory.h"
#include <libxml/xmlwriter.h>
Log_SetChannel(XMLWriter);

// Ensure libxml gets pulled in
#if Y_COMPILER_MSVC
#pragma comment(lib, "libxml2.lib")
#endif

static int XMLWriterWriteCallback(void* context, const char* buffer, int len)
{
  uint32 got = ((ByteStream*)context)->Write(buffer, len);
  return got;
}

static int XMLWriterCloseCallback(void* context)
{
  return 0;
}

XMLWriter::XMLWriter()
{
  m_pOutputStream = NULL;
  m_pWriter = NULL;
  m_pOutputBuffer = NULL;
  m_bErrorState = true;
}

XMLWriter::~XMLWriter()
{
  Close();
}

bool XMLWriter::Create(ByteStream* pOutputStream)
{
  Close();

  xmlOutputBufferPtr pOutputBuffer =
    xmlOutputBufferCreateIO(XMLWriterWriteCallback, XMLWriterCloseCallback, (void*)pOutputStream, NULL);
  if (pOutputBuffer == NULL)
  {
    Log_ErrorPrintf("XMLWriter::Create: xmlOutputBufferCreateIO failed.");
    return false;
  }

  xmlTextWriterPtr pWriter = xmlNewTextWriter(pOutputBuffer);
  if (pWriter == NULL)
  {
    Log_ErrorPrintf("XMLWriter::Create: xmlNewTextWriter failed.");
    xmlOutputBufferClose(pOutputBuffer);
    return false;
  }

  m_pOutputStream = pOutputStream;
  m_pOutputBuffer = pOutputBuffer;
  m_pWriter = pWriter;

  // set up writer
  xmlTextWriterSetIndent(pWriter, 1);
  xmlTextWriterSetIndentString(pWriter, BAD_CAST "    ");

  // write the start of the document
  if (xmlTextWriterStartDocument(m_pWriter, NULL, NULL, NULL) < 0)
    return false;

  m_bErrorState = false;
  return true;
}

void XMLWriter::Close()
{
  // not a typo, writer will free outputbuffer if it is set
  if (m_pWriter != NULL)
  {
    xmlTextWriterEndDocument(m_pWriter);
    xmlFreeTextWriter(m_pWriter);
  }
  else if (m_pOutputBuffer != NULL)
  {
    xmlOutputBufferClose(m_pOutputBuffer);
  }

  m_pOutputStream = NULL;
  m_pOutputBuffer = NULL;
  m_pWriter = NULL;
  m_bErrorState = true;
}

void XMLWriter::WriteEmptyElement(const char* elementName)
{
  if (m_bErrorState)
    return;

  if (xmlTextWriterStartElement(m_pWriter, BAD_CAST elementName) < 0)
  {
    m_bErrorState = true;
    return;
  }

  if (xmlTextWriterEndElement(m_pWriter) < 0)
  {
    m_bErrorState = true;
    return;
  }
}

void XMLWriter::StartElement(const char* elementName)
{
  if (m_bErrorState)
    return;

  if (xmlTextWriterStartElement(m_pWriter, BAD_CAST elementName) < 0)
  {
    m_bErrorState = true;
    return;
  }
}

void XMLWriter::EndElement()
{
  if (m_bErrorState)
    return;

  if (xmlTextWriterEndElement(m_pWriter) < 0)
  {
    m_bErrorState = true;
    return;
  }
}

void XMLWriter::WriteAttribute(const char* attributeName, const char* attributeValue)
{
  if (m_bErrorState)
    return;

  if (xmlTextWriterWriteAttribute(m_pWriter, BAD_CAST attributeName, BAD_CAST attributeValue) < 0)
  {
    m_bErrorState = true;
    return;
  }
}

void XMLWriter::WriteAttributef(const char* attributeName, const char* format, ...)
{
  if (m_bErrorState)
    return;

  char buf[512];
  va_list ap;
  va_start(ap, format);

  // todo fix length
  Y_vsnprintf(buf, countof(buf), format, ap);

  WriteAttribute(attributeName, buf);
}

void XMLWriter::WriteString(const char* data)
{
  if (m_bErrorState)
    return;

  if (xmlTextWriterWriteString(m_pWriter, BAD_CAST data) < 0)
  {
    m_bErrorState = true;
    return;
  }
}

void XMLWriter::WriteCDATA(const char* data)
{
  if (m_bErrorState)
    return;

  if (xmlTextWriterWriteCDATA(m_pWriter, BAD_CAST data) < 0)
  {
    m_bErrorState = true;
    return;
  }
}

void XMLWriter::WriteRaw(const char* data, uint32 dataLength)
{
  if (m_bErrorState)
    return;

  if (xmlTextWriterWriteRawLen(m_pWriter, BAD_CAST data, dataLength) < 0)
  {
    m_bErrorState = true;
    return;
  }
}

void XMLWriter::WriteBase64EncodedData(const void* pData, uint32 dataLength)
{
  if (m_bErrorState || dataLength == 0)
    return;

  uint32 tempStrLength = Y_getencodedbase64length(dataLength) + 1;
  char* tempStr = (char*)std::malloc(tempStrLength);

  uint32 nChars = Y_makebase64(pData, dataLength, tempStr, tempStrLength);
  DebugAssert(nChars > 0);
  WriteRaw(tempStr, nChars);

  std::free(tempStr);
}

#endif // HAVE_LIBXML2
