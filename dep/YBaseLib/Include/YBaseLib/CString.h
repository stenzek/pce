#pragma once

#include "YBaseLib/Common.h"

// varargs
#include <cstdarg>

// init functions
char* Y_strdup(const char* Str);
char* Y_strnew(const char* Str);
uint32 Y_strlen(const char* Source);
void Y_strncpy(char* Destination, uint32 cbDestination, const char* Source);
void Y_strncat(char* Destination, uint32 cbDestination, const char* Source);
void Y_strnsub(char* Destination, uint32 cbDestination, const char* Source, uint32 SourceOffset, int32 SourceCount);

// compare functions
int32 Y_strcmp(const char* S1, const char* S2);
int32 Y_strncmp(const char* S1, const char* S2, uint32 Count);
int32 Y_stricmp(const char* S1, const char* S2);
int32 Y_strnicmp(const char* S1, const char* S2, uint32 Count);

// format functions
uint32 Y_snprintf(char* Destination, uint32 cbDestination, const char* Format, ...);
uint32 Y_vsnprintf(char* Destination, uint32 cbDestination, const char* Format, va_list ArgPointer);
uint32 Y_scprintf(const char* Format, ...);
uint32 Y_vscprintf(const char* Format, va_list ArgPointer);
uint32 Y_sscanf(const char* buffer, const char* format, ...);
uint32 Y_vscanf(const char* buffer, const char* format, va_list argptr);

// search functions
const char* Y_strchr(const char* SearchString, char Character);
const char* Y_strrchr(const char* SearchString, char Character);
const char* Y_strstr(const char* SearchString, const char* SearchTerm);
const char* Y_strpbrk(const char* SearchString, const char* SearchTerms);
const char* Y_strrpbrk(const char* SearchString, const char* SearchTerms);
char* Y_strchr(char* SearchString, char Character);
char* Y_strrchr(char* SearchString, char Character);
char* Y_strstr(char* SearchString, const char* SearchTerm);
char* Y_strpbrk(char* SearchString, const char* SearchTerms);
char* Y_strrpbrk(char* SearchString, const char* SearchTerms);

// substring functions
char* Y_substr(char* Destination, uint32 cbDestination, const char* Source, int32 Offset, int32 Count = -1);

// conversion functions
char Y_tolower(char Character);
char Y_toupper(char Character);
void Y_strlwr(char* Str, uint32 Length);
void Y_strupr(char* Str, uint32 Length);

// split string using Separator, consecutive separators count as multiple tokens
uint32 Y_strsplit(char* Str, char Separator, char** Tokens, uint32 MaxTokens);

// split string using Separator, consecutive separators do not count as tokens
uint32 Y_strsplit2(char* Str, char Separator, char** Tokens, uint32 MaxTokens);

// split string using Separator, consecutive separators do not count as tokens, last token will not be cut if MaxTokens
// is hit
uint32 Y_strsplit3(char* Str, char Separator, char** Tokens, uint32 MaxTokens);

// modification functions
uint32 Y_strlstrip(char* Str, const char* StripCharacters);
uint32 Y_strrstrip(char* Str, const char* StripCharacters);
uint32 Y_strstrip(char* Str, const char* StripCharacters);

// wildcard functions
bool Y_strwildcmp(const char* Subject, const char* Mask);
bool Y_striwildcmp(const char* Subject, const char* Mask);

// select string
int32 Y_selectstring(const char* SelectionString, const char* ValueString);

// hex strings
uint32 Y_parsehexstring(const char* hexString, void* pOutBytes, uint32 maxOutBytes, bool exitOnInvalidCharacter = true,
                        bool* pAtEnd = NULL);
uint32 Y_makehexstring(const void* pInBytes, uint32 nInBytes, char* outBuffer, uint32 bufferSize);

// base64 strings. bufferSize must be (nInBytes / 4 + 1) * 3 long at a minimum
uint32 Y_getencodedbase64length(uint32 byteLength);
uint32 Y_getdecodedbase64length(uint32 base64Length);
uint32 Y_parsebase64(const char* base64String, void* pOutBytes, uint32 maxOutBytes, bool exitOnInvalidCharacter = true,
                     bool* pAtEnd = NULL);
uint32 Y_makebase64(const void* pInBytes, uint32 nInBytes, char* outBuffer, uint32 bufferSize);

// convert from string to specific type
/*bool SStringParseBool(bool *pOut, const char *Str, char **pEndPtr = NULL);
bool SStringParseFloat(float *pOut, const char *Str, char **pEndPtr = NULL);
bool SStringParseDouble(double *pOut, const char *Str, char **pEndPtr = NULL);
bool SStringParseInt8(SInt8 *pOut, const char *Str, char **pEndPtr = NULL);
bool SStringParseInt16(SInt16 *pOut, const char *Str, char **pEndPtr = NULL);
bool SStringParseInt32(SInt32 *pOut, const char *Str, char **pEndPtr = NULL);
bool SStringParseInt64(SInt64 *pOut, const char *Str, char **pEndPtr = NULL);
bool SStringParseUInt8(SUInt8 *pOut, const char *Str, char **pEndPtr = NULL);
bool SStringParseUInt16(SUInt16 *pOut, const char *Str, char **pEndPtr = NULL);
bool SStringParseUInt32(SUInt32 *pOut, const char *Str, char **pEndPtr = NULL);
bool SStringParseUInt64(SUInt64 *pOut, const char *Str, char **pEndPtr = NULL);*/

// simpler versions of the above without any error checking
bool Y_strtobool(const char* Str, char** pEndPtr = NULL);
float Y_strtofloat(const char* Str, char** pEndPtr = NULL);
double Y_strtodouble(const char* Str, char** pEndPtr = NULL);
byte Y_strtobyte(const char* Str, char** pEndPtr = NULL);
int8 Y_strtoint8(const char* Str, char** pEndPtr = NULL);
int16 Y_strtoint16(const char* Str, char** pEndPtr = NULL);
int32 Y_strtoint32(const char* Str, char** pEndPtr = NULL);
int64 Y_strtoint64(const char* Str, char** pEndPtr = NULL);
uint8 Y_strtouint8(const char* Str, char** pEndPtr = NULL);
uint16 Y_strtouint16(const char* Str, char** pEndPtr = NULL);
uint32 Y_strtouint32(const char* Str, char** pEndPtr = NULL);
uint64 Y_strtouint64(const char* Str, char** pEndPtr = NULL);

// types -> str
void Y_strfrombool(char* Str, uint32 MaxLength, bool Value);
void Y_strfromfloat(char* Str, uint32 MaxLength, float Value);
void Y_strfromdouble(char* Str, uint32 MaxLength, double Value);
void Y_strfrombyte(char* Str, uint32 MaxLength, byte Value); // <-- hex
void Y_strfromint8(char* Str, uint32 MaxLength, int8 Value, uint32 Base = 10);
void Y_strfromint16(char* Str, uint32 MaxLength, int16 Value, uint32 Base = 10);
void Y_strfromint32(char* Str, uint32 MaxLength, int32 Value, uint32 Base = 10);
void Y_strfromint64(char* Str, uint32 MaxLength, int64 Value, uint32 Base = 10);
void Y_strfromuint8(char* Str, uint32 MaxLength, uint8 Value, uint32 Base = 10);
void Y_strfromuint16(char* Str, uint32 MaxLength, uint16 Value, uint32 Base = 10);
void Y_strfromuint32(char* Str, uint32 MaxLength, uint32 Value, uint32 Base = 10);
void Y_strfromuint64(char* Str, uint32 MaxLength, uint64 Value, uint32 Base = 10);
