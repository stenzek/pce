#include "pce/component.h"
#include <chrono>
#include <cinttypes>
#include <random>

DEFINE_OBJECT_TYPE_INFO(Component);

Component::Component(const String& identifier, const ObjectTypeInfo* type_info)
  : BaseClass(type_info), m_identifier(identifier)
{
}

Component::~Component() = default;

String Component::GenerateIdentifier(const ObjectTypeInfo* type)
{
  static std::mt19937_64 r(std::chrono::system_clock::now().time_since_epoch().count());
  const u64 id_num = static_cast<u64>(r());
  return String::FromFormat("%s%s%" PRIx64, type ? type->GetTypeName() : "", type ? "-" : "", id_num);
}

bool Component::Initialize(System* system, Bus* bus)
{
  m_system = system;
  m_bus = bus;
  return true;
}

void Component::Reset() {}

bool Component::LoadState(BinaryReader& reader)
{
  return true;
}

bool Component::SaveState(BinaryWriter& writer)
{
  return true;
}
