#pragma once
#include "YBaseLib/String.h"
#include "common/object.h"
#include "pce/types.h"

class BinaryReader;
class BinaryWriter;

class Bus;
class System;

class Component : public Object
{
  DECLARE_OBJECT_TYPE_INFO(Component, Object);
  DECLARE_OBJECT_NO_PROPERTIES(Component);
  DECLARE_OBJECT_NO_FACTORY(Component);

public:
  Component(const String& identifier, const ObjectTypeInfo* type_info);
  virtual ~Component();

  // Creates a serialization ID for identifying components in save states
  static constexpr uint32 MakeSerializationID(uint8 a = 0, uint8 b = 0, uint8 c = 0, uint8 d = 0)
  {
    return uint32(d) | (uint32(c) << 8) | (uint32(b) << 16) | (uint32(a) << 24);
  }

  // Generates a random component ID.
  static String GenerateIdentifier(const ObjectTypeInfo* type);

  const String& GetIdentifier() const { return m_identifier; }
  System* GetSystem() const { return m_system; }
  Bus* GetBus() const { return m_bus; }

  virtual bool Initialize(System* system, Bus* bus);
  virtual void Reset();

  virtual bool LoadState(BinaryReader& reader);
  virtual bool SaveState(BinaryWriter& writer);

protected:
  String m_identifier;
  System* m_system = nullptr;
  Bus* m_bus = nullptr;
};

//
// GenericComponentFactory<T>
//
template<class T>
struct GenericComponentFactory final : public ObjectFactory
{
  Object* CreateObject() override { return new T(Component::GenerateIdentifier(T::StaticTypeInfo())); }
  void DeleteObject(Object* object) override { delete object; }
};

#define DECLARE_GENERIC_COMPONENT_FACTORY(Type)                                                                        \
  \
private:                                                                                                               \
  static GenericComponentFactory<Type> s_GenericFactory;                                                               \
  \
public:                                                                                                                \
  static GenericComponentFactory<Type>* StaticFactory() { return &s_GenericFactory; }

#define DEFINE_GENERIC_COMPONENT_FACTORY(Type) GenericComponentFactory<Type> Type::s_GenericFactory;
