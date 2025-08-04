#include "TestSuite.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/Log.h"
Log_SetChannel(TestBase64);

static bool TestBase64Encode(const char* inputHexBytes, const char* expectedOutput)
{
  uint32 inputHexBytesLength = Y_strlen(inputHexBytes);
  byte* inputBytes = new byte[inputHexBytesLength / 2];
  uint32 encodedLength = ((inputHexBytesLength / 2 + 3) / 3) * 4;
  char* encodedBytes = new char[encodedLength + 1];

  Y_parsehexstring(inputHexBytes, inputBytes, inputHexBytesLength / 2, false, NULL);
  Y_makebase64(inputBytes, inputHexBytesLength / 2, encodedBytes, encodedLength + 1);

  bool result = (Y_strlen(encodedBytes) == Y_strlen(expectedOutput) && Y_strcmp(encodedBytes, expectedOutput) == 0);
  if (result)
    Log_InfoPrintf("PASS: Encode '%s' -> '%s'", inputHexBytes, encodedBytes);
  else
    Log_ErrorPrintf("FAIL: Encode '%s' -> '%s' (expecting '%s')", inputHexBytes, encodedBytes, expectedOutput);

  delete[] encodedBytes;
  delete[] inputBytes;
  return result;
}

static bool TestBase64Decode(const char* pInputData, const char* expectedOutputHexBytes)
{
  uint32 outputLength = Y_strlen(expectedOutputHexBytes) / 2;
  byte* decodedBytes = new byte[outputLength];
  char* decodedBytesHexString = new char[outputLength * 2 + 1];

  uint32 decodedBytesLength = Y_parsebase64(pInputData, decodedBytes, outputLength, true);
  Y_makehexstring(decodedBytes, decodedBytesLength, decodedBytesHexString, outputLength * 2 + 1);

  bool result = (decodedBytesLength == outputLength &&
                 Y_strnicmp(decodedBytesHexString, expectedOutputHexBytes, decodedBytesLength) == 0);
  if (result)
    Log_InfoPrintf("PASS: Decode '%s' -> '%s'", pInputData, decodedBytesHexString);
  else
    Log_ErrorPrintf("FAIL: Decode '%s' -> '%s' (%u) (expecting '%s' (%u))", pInputData, decodedBytesHexString,
                    decodedBytesLength, expectedOutputHexBytes, outputLength);

  delete[] decodedBytesHexString;
  delete[] decodedBytes;
  return result;
}

DEFINE_TEST_SUITE(Base64)
{
  struct TestCase
  {
    const char* hexString;
    const char* base64String;
  };
  static const TestCase testCases[] = {
    {"33326a6f646933326a68663937683732383368", "MzJqb2RpMzJqaGY5N2g3MjgzaA=="},
    {"32753965333268756979386672677537366967723839683432703075693132393065755c5d0931325c335c31323439303438753839333272",
     "MnU5ZTMyaHVpeThmcmd1NzZpZ3I4OWg0MnAwdWkxMjkwZXVcXQkxMlwzXDEyNDkwNDh1ODkzMnI="},
    {"3332726a33323738676838666233326830393233386637683938323139", "MzJyajMyNzhnaDhmYjMyaDA5MjM4ZjdoOTgyMTk="},
    {"9956967BE9C96E10B27FF8897A5B768A2F4B103CE934718D020FE6B5B770", "mVaWe+nJbhCyf/iJelt2ii9LEDzpNHGNAg/mtbdw"},
    {"BC94251814827A5D503D62D5EE6CBAB0FD55D2E2FCEDBB2261D6010084B95DD648766D8983F03AFA3908956D8201E26BB09FE52B515A61A9E"
     "1D3ADC207BD9E622128F22929CDED456B595A410F7168B0BA6370289E6291E38E47C18278561C79A7297C21D23C06BB2F694DC2F65FAAF994"
     "59E3FC14B1FA415A3320AF00ACE54C00BE",
     "vJQlGBSCel1QPWLV7my6sP1V0uL87bsiYdYBAIS5XdZIdm2Jg/A6+jkIlW2CAeJrsJ/"
     "lK1FaYanh063CB72eYiEo8ikpze1Fa1laQQ9xaLC6Y3AonmKR445HwYJ4Vhx5pyl8IdI8BrsvaU3C9l+q+ZRZ4/wUsfpBWjMgrwCs5UwAvg=="},
    {"192B42CB0F66F69BE8A5", "GStCyw9m9pvopQ=="},
    {"38ABD400F3BB6960EB60C056719B5362", "OKvUAPO7aWDrYMBWcZtTYg=="},
    {"776FAB27DC7F8DA86F298D55B69F8C278D53871F8CBCCF", "d2+rJ9x/jahvKY1Vtp+MJ41Thx+MvM8="},
    {"B1ED3EA2E35EE69C7E16707B05042A", "se0+ouNe5px+FnB7BQQq"},
  };

  uint32 i;
  bool result = true;
  for (i = 0; i < countof(testCases); i++)
  {
    if (!TestBase64Encode(testCases[i].hexString, testCases[i].base64String))
      result = false;
  }

  for (i = 0; i < countof(testCases); i++)
  {
    if (!TestBase64Decode(testCases[i].base64String, testCases[i].hexString))
      result = false;
  }

  return result;
}
