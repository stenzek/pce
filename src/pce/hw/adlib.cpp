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

AdLib::AdLib() : m_chip(YMF262::Mode::OPL2, "AdLib ") {}

AdLib::~AdLib() {}

void AdLib::Initialize(System* system, Bus* bus)
{
  m_chip.Initialize(system);

  // IO port connections
  for (uint32 i = 0x0388; i <= 0x0399; i++)
  {
    bus->ConnectIOPortRead(i, this, std::bind(&AdLib::IOPortRead, this, std::placeholders::_1, std::placeholders::_2));
    bus->ConnectIOPortWrite(i, this,
                            std::bind(&AdLib::IOPortWrite, this, std::placeholders::_1, std::placeholders::_2));
  }
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