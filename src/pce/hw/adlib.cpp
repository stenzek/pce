#include "pce/hw/adlib.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Timer.h"
#include "pce/bus.h"
#include "pce/host_interface.h"
#include <cmath>
Log_SetChannel(HW::AdLib);

namespace HW {

DEFINE_OBJECT_TYPE_INFO(AdLib);
DEFINE_GENERIC_COMPONENT_FACTORY(AdLib);
BEGIN_OBJECT_PROPERTY_MAP(AdLib)
PROPERTY_TABLE_MEMBER_UINT("IOBase", 0, offsetof(AdLib, m_io_base), nullptr, 0)
END_OBJECT_PROPERTY_MAP()

AdLib::AdLib(const String& identifier, const ObjectTypeInfo* type_info /* = &s_type_info */)
  : BaseClass(identifier, type_info), m_chip(YMF262::Mode::OPL2, "AdLib ")
{
}

AdLib::~AdLib() = default;

bool AdLib::Initialize(System* system, Bus* bus)
{
  if (!BaseClass::Initialize(system, bus) || !m_chip.Initialize(system))
    return false;

  // IO port connections
  for (uint32 i = 0; i < 18; i++)
  {
    bus->ConnectIOPortRead(m_io_base + i, this,
                           std::bind(&AdLib::IOPortRead, this, std::placeholders::_1, std::placeholders::_2));
    bus->ConnectIOPortWrite(m_io_base + i, this,
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

void AdLib::IOPortRead(uint32 port, uint8* value)
{
  switch (port)
  {
    case 0x0388:
      *value = m_chip.ReadAddressPort(0);
      break;

    case 0x0399:
      *value = m_chip.ReadDataPort(0);
      break;
  }
}

void AdLib::IOPortWrite(uint32 port, uint8 value)
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