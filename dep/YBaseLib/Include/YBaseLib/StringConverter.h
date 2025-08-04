#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/String.h"

class String;
class ByteStream;

namespace StringConverter {
bool StringToBool(const char* Source);
byte StringToByte(const char* Source);
int8 StringToInt8(const char* Source);
int16 StringToInt16(const char* Source);
int32 StringToInt32(const char* Source);
int64 StringToInt64(const char* Source);
uint8 StringToUInt8(const char* Source);
uint16 StringToUInt16(const char* Source);
uint32 StringToUInt32(const char* Source);
uint64 StringToUInt64(const char* Source);
float StringToFloat(const char* Source);
double StringToDouble(const char* Source);
uint32 StringToColor(const char* Source);
uint32 HexStringToBytes(void* pDestination, uint32 cbDestination, const char* Source);
void StringToStream(ByteStream* pStream, const String& Source);
void StringToStream(ByteStream* pStream, const char* Source, uint32 Length);

void BoolToString(String& Destination, const bool Value);
void Int8ToString(String& Destination, const int8 Value);
void Int16ToString(String& Destination, const int16 Value);
void Int32ToString(String& Destination, const int32 Value);
void Int64ToString(String& Destination, const int64 Value);
void UInt8ToString(String& Destination, const uint8 Value);
void UInt16ToString(String& Destination, const uint16 Value);
void UInt32ToString(String& Destination, const uint32 Value);
void UInt64ToString(String& Destination, const uint64 Value);
void FloatToString(String& Destination, const float Value);
void DoubleToString(String& Destination, const double Value);
void ColorToString(String& Destination, const uint32 Value);
void BytesToHexString(String& Destination, const void* pData, uint32 cbData);
bool StreamToString(String& Destination, ByteStream* pStream);
bool AppendStreamToString(String& Destination, ByteStream* pStream);
void SizeToHumanReadableString(String& Destination, uint64 nBytes);

TinyString BoolToString(const bool Value);
TinyString Int8ToString(const int8 Value);
TinyString Int16ToString(const int16 Value);
TinyString Int32ToString(const int32 Value);
TinyString Int64ToString(const int64 Value);
TinyString UInt8ToString(const uint8 Value);
TinyString UInt16ToString(const uint16 Value);
TinyString UInt32ToString(const uint32 Value);
TinyString UInt64ToString(const uint64 Value);
TinyString FloatToString(const float Value);
TinyString DoubleToString(const double Value);
TinyString ColorToString(const uint32 Value);
String BytesToHexString(const void* pData, uint32 cbData);
String StreamToString(ByteStream* pStream);
TinyString SizeToHumanReadableString(uint64 nBytes);
} // namespace StringConverter
