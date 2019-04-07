#include "system.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
Log_SetChannel(CPU_8086_TestSystem);

DEFINE_OBJECT_TYPE_INFO(CPU_8086_TestSystem);

CPU_8086_TestSystem::CPU_8086_TestSystem(CPU_8086::Model cpu_model /* = CPU_8086::MODEL_8086 */,
                                         float cpu_frequency /* = 1000000.0f */, u32 ram_size /* = 1024 * 1024 */)
  : System()
{
  m_bus = new Bus(20);
  m_bus->AllocateRAM(ram_size);
  m_cpu = CreateComponent<CPU_8086::CPU>("CPU", cpu_model, cpu_frequency);
  AddComponents();
}

CPU_8086_TestSystem::~CPU_8086_TestSystem() {}

bool CPU_8086_TestSystem::Initialize()
{
  if (!BaseClass::Initialize())
    return false;

  // Fill memory regions.
  m_bus->CreateRAMRegion(UINT32_C(0x00000000), UINT32_C(0x0009FFFF));

  for (const ROMFile& rom : m_rom_files)
  {
    if (!m_bus->CreateROMRegionFromFile(rom.filename, 0, rom.load_address, rom.expected_size))
    {
      Log_ErrorPrintf("Failed to load ROM file from '%s'.", rom.filename.GetCharArray());
      return false;
    }
  }

  return true;
}

void CPU_8086_TestSystem::AddROMFile(const char* filename, PhysicalMemoryAddress load_address, u32 expected_size)
{
  m_rom_files.push_back({filename, load_address, expected_size});
}

bool CPU_8086_TestSystem::Ready()
{
  if (!Initialize())
    return false;

  Reset();
  SetState(State::Running);
  return true;
}

void CPU_8086_TestSystem::AddComponents()
{
  m_interrupt_controller = CreateComponent<HW::i8259_PIC>("InterruptController");
}
