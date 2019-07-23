#include "pce/hw/adlib.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Timer.h"
#include "pce/bus.h"
#include "pce/host_interface.h"
#include <cmath>

namespace HW {

DEFINE_OBJECT_TYPE_INFO(AdLib);
DEFINE_GENERIC_COMPONENT_FACTORY(AdLib);
BEGIN_OBJECT_PROPERTY_MAP(AdLib)
PROPERTY_TABLE_MEMBER_UINT("IOBase", 0, offsetof(AdLib, m_io_base), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

AdLib::AdLib(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_chip(YMF262::Mode::OPL2)
{
}

AdLib::~AdLib() = default;

bool AdLib::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus) || !m_chip.Initialize(system))
    return false;

  // IO port connections
  for (u32 i = 0; i < 18; i++)
  {
    bus->ConnectIOPortRead(Truncate16(m_io_base + i), this, std::bind(&AdLib::IOPortRead, this, std::placeholders::_1));
    bus->ConnectIOPortWrite(Truncate16(m_io_base + i), this,
                            std::bind(&AdLib::IOPortWrite, this, std::placeholders::_1, std::placeholders::_2));
  }

  return true;
}

void AdLib::Reset()
{
  m_chip.Reset();
}

bool AdLib::LoadState(BinaryReader& reader)
{
  return m_chip.LoadState(reader);
}

bool AdLib::SaveState(BinaryWriter& writer)
{
  return m_chip.SaveState(writer);
}

u8 AdLib::IOPortRead(u16 port)
{
  switch (port)
  {
    case 0x0388:
      return m_chip.ReadAddressPort(0);

    case 0x0399:
      return m_chip.ReadDataPort(0);

    default:
      return 0xFF;
  }
}

void AdLib::IOPortWrite(u16 port, u8 value)
{
  switch (port)
  {
    case 0x0388:
      m_chip.WriteAddressPort(0, value);
      break;

    case 0x0389:
      m_chip.WriteDataPort(0, value);
      break;
  }
}

} // namespace HW