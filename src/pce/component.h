#pragma once
#include "YBaseLib/String.h"
#include "common/object.h"
#include "pce/types.h"

class BinaryReader;
class BinaryWriter;
class StateWrapper;

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
  static constexpr u32 MakeSerializationID(u8 a = 0, u8 b = 0, u8 c = 0, u8 d = 0)
  {
    return u32(d) | (u32(c) << 8) | (u32(b) << 16) | (u32(a) << 24);
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
  virtual bool DoState(StateWrapper& sw);

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
  Object* CreateObject(const String& identifier) override
  {
    return static_cast<typename T::BaseClass*>(new T(identifier));
  }
  Object* CreateObject() override { return CreateObject(Component::GenerateIdentifier(T::StaticTypeInfo())); }
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
