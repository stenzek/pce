#include "TestSuite.h"
#include "YBaseLib/BitSet.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/Log.h"
Log_SetChannel(TestBase64);

static bool BitSet8Tests()
{
  BitSet8 bs(100);
  bs.SetBit(10, true);
  if (!bs.TestBit(10) || bs.TestBit(20))
    return false;

  bs.SetBit(12, true);
  if (!bs[12])
    return false;

  bs[12] = false;
  if (bs.TestBit(12))
    return false;

  size_t index;
  bs.FindFirstSetBit(&index);
  bs.SetBit(0);
  bs.FindFirstClearBit(&index);
  bs.SetBit(1);
  bs.SetBit(3);
  bs.SetBit(4);
  bs.FindContiguousClearBits(4, &index);
  return true;
}

DEFINE_TEST_SUITE(BitSet)
{
  if (!BitSet8Tests())
    return false;

  return true;
}
