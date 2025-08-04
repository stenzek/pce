#include "YBaseLib/NameTable.h"
#include "YBaseLib/CString.h"

const NameTableEntry* NameTable_LookupByName(const NameTableEntry* pNameTable, const char* sName,
                                             bool bCaseInsensitive /* = false */)
{
  const NameTableEntry* pCurrentEntry = pNameTable;
  if (bCaseInsensitive)
  {
    for (; pCurrentEntry->sName != NULL; pCurrentEntry++)
    {
      if (!Y_stricmp(sName, pCurrentEntry->sName))
        return pCurrentEntry;
    }
  }
  else
  {
    for (; pCurrentEntry->sName != NULL; pCurrentEntry++)
    {
      if (!Y_strcmp(sName, pCurrentEntry->sName))
        return pCurrentEntry;
    }
  }

  return NULL;
}

const NameTableEntry* NameTable_LookupByValue(const NameTableEntry* pNameTable, int iValue)
{
  const NameTableEntry* pCurrentEntry = pNameTable;
  for (; pCurrentEntry->sName != NULL; pCurrentEntry++)
  {
    if (pCurrentEntry->iValue == iValue)
      return pCurrentEntry;
  }

  return NULL;
}