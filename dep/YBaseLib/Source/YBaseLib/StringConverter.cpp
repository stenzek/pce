#include "YBaseLib/StringConverter.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/CString.h"

bool StringConverter::StringToBool(const char* Source)
{
  return Y_strtobool(Source);
}

byte StringConverter::StringToByte(const char* Source)
{
  return Y_strtobyte(Source);
}

int8 StringConverter::StringToInt8(const char* Source)
{
  return Y_strtoint8(Source);
}

int16 StringConverter::StringToInt16(const char* Source)
{
  return Y_strtoint16(Source);
}

int32 StringConverter::StringToInt32(const char* Source)
{
  return Y_strtoint32(Source);
}

int64 StringConverter::StringToInt64(const char* Source)
{
  return Y_strtoint64(Source);
}

uint8 StringConverter::StringToUInt8(const char* Source)
{
  return Y_strtouint8(Source);
}

uint16 StringConverter::StringToUInt16(const char* Source)
{
  return Y_strtouint16(Source);
}

uint32 StringConverter::StringToUInt32(const char* Source)
{
  return Y_strtouint32(Source);
}

uint64 StringConverter::StringToUInt64(const char* Source)
{
  return Y_strtouint64(Source);
}

float StringConverter::StringToFloat(const char* Source)
{
  return Y_strtofloat(Source);
}

double StringConverter::StringToDouble(const char* Source)
{
  return Y_strtodouble(Source);
}

uint32 StringConverter::StringToColor(const char* Source)
{
  if (Source[0] != '#')
    return 0;

  byte sourceBytes[4];
  uint32 nComponents = Y_parsehexstring(Source + 1, sourceBytes, countof(sourceBytes), true);
  uint32 outColor = 0;
  switch (nComponents)
  {
    case 1:
      outColor = (uint32)sourceBytes[0] | (uint32)0xff << 24;
      break;

    case 2:
      outColor = (uint32)sourceBytes[0] | (uint32)sourceBytes[1] << 8 | (uint32)0xff << 24;
      break;

    case 3:
      outColor =
        (uint32)sourceBytes[0] | (uint32)sourceBytes[1] << 8 | (uint32)sourceBytes[2] << 16 | (uint32)0xff << 24;
      break;

    case 4:
      outColor = (uint32)sourceBytes[0] | (uint32)sourceBytes[1] << 8 | (uint32)sourceBytes[2] << 16 |
                 (uint32)sourceBytes[3] << 24;
      break;

    default:
      UnreachableCode();
      break;
  }

  return outColor;
}

uint32 StringConverter::HexStringToBytes(void* pDestination, uint32 cbDestination, const char* Source)
{
  return Y_parsehexstring(Source, pDestination, cbDestination, true, NULL);
}

void StringConverter::StringToStream(ByteStream* pStream, const String& Source)
{
  if (Source.GetLength() > 0)
    pStream->Write2(Source.GetCharArray(), Source.GetLength());
}

void StringConverter::StringToStream(ByteStream* pStream, const char* Source, uint32 Length)
{
  if (Length > 0)
    pStream->Write2(Source, Length);
}

void StringConverter::BoolToString(String& Destination, const bool Value)
{
  char str[64];
  Y_strfrombool(str, countof(str), Value);
  Destination = str;
}

void StringConverter::Int8ToString(String& Destination, const int8 Value)
{
  char str[64];
  Y_strfromint8(str, countof(str), Value);
  Destination = str;
}

void StringConverter::Int16ToString(String& Destination, const int16 Value)
{
  char str[64];
  Y_strfromint16(str, countof(str), Value);
  Destination = str;
}

void StringConverter::Int32ToString(String& Destination, const int32 Value)
{
  char str[64];
  Y_strfromint32(str, countof(str), Value);
  Destination = str;
}

void StringConverter::Int64ToString(String& Destination, const int64 Value)
{
  char str[64];
  Y_strfromint64(str, countof(str), Value);
  Destination = str;
}

void StringConverter::UInt8ToString(String& Destination, const uint8 Value)
{
  char str[64];
  Y_strfromuint8(str, countof(str), Value);
  Destination = str;
}

void StringConverter::UInt16ToString(String& Destination, const uint16 Value)
{
  char str[64];
  Y_strfromuint16(str, countof(str), Value);
  Destination = str;
}

void StringConverter::UInt32ToString(String& Destination, const uint32 Value)
{
  char str[64];
  Y_strfromuint32(str, countof(str), Value);
  Destination = str;
}

void StringConverter::UInt64ToString(String& Destination, const uint64 Value)
{
  char str[64];
  Y_strfromuint64(str, countof(str), Value);
  Destination = str;
}

void StringConverter::FloatToString(String& Destination, const float Value)
{
  char str[64];
  Y_strfromfloat(str, countof(str), Value);
  Destination = str;
}

void StringConverter::DoubleToString(String& Destination, const double Value)
{
  char str[64];
  Y_strfromdouble(str, countof(str), Value);
  Destination = str;
}

void StringConverter::ColorToString(String& Destination, const uint32 Value)
{
  char str[64];
  if (((Value >> 24) & 0xff) == 0xff)
    Y_snprintf(str, countof(str), "#%02x%02x%02x", (Value & 0xff), ((Value >> 8) & 0xff), ((Value >> 16) & 0xff));
  else
    Y_snprintf(str, countof(str), "#%02x%02x%02x%02x", (Value & 0xff), ((Value >> 8) & 0xff), ((Value >> 16) & 0xff),
               ((Value >> 24) & 0xff));

  Destination = str;
}

void StringConverter::BytesToHexString(String& Destination, const void* pData, uint32 cbData)
{
  Destination.Resize(cbData * 2);
  Y_makehexstring(pData, cbData, Destination.GetWriteableCharArray(), cbData * 2 + 1);
}

bool StringConverter::StreamToString(String& Destination, ByteStream* pStream)
{
  uint32 streamLength = static_cast<uint32>(pStream->GetSize());
  Destination.Resize(streamLength);
  if (streamLength > 0)
    return pStream->Read2(Destination.GetWriteableCharArray(), streamLength);

  return true;
}

bool StringConverter::AppendStreamToString(String& Destination, ByteStream* pStream)
{
  uint32 streamLength = static_cast<uint32>(pStream->GetSize());
  if (streamLength > 0)
  {
    uint32 currentLength = Destination.GetLength();
    Destination.Resize(currentLength + streamLength);
    return pStream->Read2(Destination.GetWriteableCharArray() + currentLength, streamLength);
  }

  return true;
}

void StringConverter::SizeToHumanReadableString(String& Destination, uint64 nBytes)
{
  static const uint64 OneKiB = 1024;
  static const uint64 OneMiB = (OneKiB * 1024);
  static const uint64 OneGiB = (OneMiB * 1024);
  static const uint64 OneTiB = (OneGiB * 1024);

  double dBytes = (double)nBytes;
  if (nBytes >= OneTiB)
    Destination.Format("%.2f TiB", dBytes / (double)OneTiB);
  else if (nBytes >= OneGiB)
    Destination.Format("%.2f GiB", dBytes / (double)OneGiB);
  else if (nBytes >= OneMiB)
    Destination.Format("%.2f MiB", dBytes / (double)OneMiB);
  else if (nBytes >= OneKiB)
    Destination.Format("%.2f KiB", dBytes / (double)OneKiB);
  else
    Destination.Format("%u B", nBytes);
}

TinyString StringConverter::BoolToString(const bool Value)
{
  TinyString ret;
  BoolToString(ret, Value);
  return ret;
}

TinyString StringConverter::Int8ToString(const int8 Value)
{
  TinyString ret;
  Int8ToString(ret, Value);
  return ret;
}

TinyString StringConverter::Int16ToString(const int16 Value)
{
  TinyString ret;
  Int16ToString(ret, Value);
  return ret;
}

TinyString StringConverter::Int32ToString(const int32 Value)
{
  TinyString ret;
  Int32ToString(ret, Value);
  return ret;
}

TinyString StringConverter::Int64ToString(const int64 Value)
{
  TinyString ret;
  Int64ToString(ret, Value);
  return ret;
}

TinyString StringConverter::UInt8ToString(const uint8 Value)
{
  TinyString ret;
  UInt8ToString(ret, Value);
  return ret;
}

TinyString StringConverter::UInt16ToString(const uint16 Value)
{
  TinyString ret;
  UInt16ToString(ret, Value);
  return ret;
}

TinyString StringConverter::UInt32ToString(const uint32 Value)
{
  TinyString ret;
  UInt32ToString(ret, Value);
  return ret;
}

TinyString StringConverter::UInt64ToString(const uint64 Value)
{
  TinyString ret;
  UInt64ToString(ret, Value);
  return ret;
}

TinyString StringConverter::FloatToString(const float Value)
{
  TinyString ret;
  FloatToString(ret, Value);
  return ret;
}

TinyString StringConverter::DoubleToString(const double Value)
{
  TinyString ret;
  DoubleToString(ret, Value);
  return ret;
}

TinyString StringConverter::ColorToString(const uint32 Value)
{
  TinyString ret;
  ColorToString(ret, Value);
  return ret;
}

String StringConverter::BytesToHexString(const void* pData, uint32 cbData)
{
  String ret;
  BytesToHexString(ret, pData, cbData);
  return ret;
}

String StringConverter::StreamToString(ByteStream* pStream)
{
  String ret;
  StreamToString(ret, pStream);
  return ret;
}

TinyString StringConverter::SizeToHumanReadableString(uint64 nBytes)
{
  TinyString ret;
  SizeToHumanReadableString(ret, nBytes);
  return ret;
}
