#pragma once
#include "YBaseLib/CString.h"
#include "YBaseLib/MemArray.h"
#include "YBaseLib/PODArray.h"
#include "common/types.h"

#define INVALID_TYPE_INDEX 0xFFFFFFFF

template<class T>
class TypeRegistry
{
public:
  struct RegisteredTypeInfo
  {
    T* pTypeInfo;
    const char* TypeName;
    uint32 InheritanceDepth;
  };

public:
  TypeRegistry() {}
  ~TypeRegistry() {}

  uint32 RegisterTypeInfo(T* pTypeInfo, const char* TypeName, uint32 InheritanceDepth)
  {
    uint32 Index;
    DebugAssert(pTypeInfo != nullptr);

    for (Index = 0; Index < m_arrTypes.GetSize(); Index++)
    {
      if (m_arrTypes[Index].pTypeInfo == pTypeInfo)
        Panic("Attempting to register type multiple times.");
    }

    for (Index = 0; Index < m_arrTypes.GetSize(); Index++)
    {
      if (m_arrTypes[Index].pTypeInfo == nullptr)
      {
        m_arrTypes[Index].pTypeInfo = pTypeInfo;
        m_arrTypes[Index].TypeName = TypeName;
        m_arrTypes[Index].InheritanceDepth = InheritanceDepth;
        break;
      }
    }
    if (Index == m_arrTypes.GetSize())
    {
      RegisteredTypeInfo t;
      t.pTypeInfo = pTypeInfo;
      t.TypeName = TypeName;
      t.InheritanceDepth = InheritanceDepth;
      m_arrTypes.Add(t);
    }

    CalculateMaxInheritanceDepth();
    return Index;
  }

  void UnregisterTypeInfo(T* pTypeInfo)
  {
    uint32 i;
    for (i = 0; i < m_arrTypes.GetSize(); i++)
    {
      if (m_arrTypes[i].pTypeInfo == pTypeInfo)
      {
        m_arrTypes[i].pTypeInfo = nullptr;
        m_arrTypes[i].TypeName = nullptr;
        m_arrTypes[i].InheritanceDepth = 0;
        break;
      }
    }
  }

  const uint32 GetNumTypes() const { return m_arrTypes.GetSize(); }
  const uint32 GetMaxInheritanceDepth() const { return m_iMaxInheritanceDepth; }

  const RegisteredTypeInfo& GetRegisteredTypeInfoByIndex(uint32 TypeIndex) const
  {
    return m_arrTypes.GetElement(TypeIndex);
  }

  const T* GetTypeInfoByIndex(uint32 TypeIndex) const { return m_arrTypes.GetElement(TypeIndex).pTypeInfo; }

  const T* GetTypeInfoByName(const char* TypeName) const
  {
    for (uint32 i = 0; i < m_arrTypes.GetSize(); i++)
    {
      if (m_arrTypes[i].pTypeInfo != nullptr && !Y_stricmp(m_arrTypes[i].TypeName, TypeName))
        return m_arrTypes[i].pTypeInfo;
    }

    return nullptr;
  }

private:
  typedef MemArray<RegisteredTypeInfo> TypeArray;
  TypeArray m_arrTypes;
  uint32 m_iMaxInheritanceDepth;

  void CalculateMaxInheritanceDepth()
  {
    uint32 i;
    m_iMaxInheritanceDepth = 0;

    for (i = 0; i < m_arrTypes.GetSize(); i++)
    {
      if (m_arrTypes[i].pTypeInfo != nullptr)
        m_iMaxInheritanceDepth = Max(m_iMaxInheritanceDepth, m_arrTypes[i].InheritanceDepth);
    }
  }
};
