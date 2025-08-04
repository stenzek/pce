#pragma once
#include "YBaseLib/Common.h"

struct NameTableEntry
{
  const char* sName;
  int iValue;
};

#define Y_Declare_NameTable(Name) extern const NameTableEntry Name[];
#define Y_Define_NameTable(Name) const NameTableEntry Name[] = {
#define Y_NameTable_Entry(Name, Value) {Name, Value},
#define Y_NameTable_VEntry(Value, Name) {Name, Value},
#define Y_NameTable_End()                                                                                              \
  {                                                                                                                    \
    NULL, 0                                                                                                            \
  }                                                                                                                    \
  }                                                                                                                    \
  ;

const NameTableEntry* NameTable_LookupByName(const NameTableEntry* pNameTable, const char* sName,
                                             bool bCaseInsensitive = false);
const NameTableEntry* NameTable_LookupByValue(const NameTableEntry* pNameTable, int iValue);

template<typename T>
inline bool NameTable_TranslateType(const NameTableEntry* pNameTable, const char* sName, T* pDestination,
                                    bool bCaseInsensitive = false)
{
  const NameTableEntry* pFoundEntry = NameTable_LookupByName(pNameTable, sName, bCaseInsensitive);
  if (pFoundEntry != NULL)
  {
    *pDestination = (T)pFoundEntry->iValue;
    return true;
  }

  return false;
}

template<typename T>
inline const char* NameTable_GetNameString(const NameTableEntry* pNameTable, const T Value,
                                           const char* DefaultReturn = "UNKNOWN")
{
  const NameTableEntry* pFoundEntry = NameTable_LookupByValue(pNameTable, (int)Value);
  return pFoundEntry != NULL ? pFoundEntry->sName : DefaultReturn;
}
