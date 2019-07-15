#include "system.h"
#include "YBaseLib/Log.h"
#include "pce/bus.h"
#include "pce/mmio.h"
Log_SetChannel(CPU_X86_TestSystem);

DEFINE_OBJECT_TYPE_INFO(CPU_X86_TestSystem);

CPU_X86_TestSystem::CPU_X86_TestSystem(CPU_X86::Model cpu_model /* = CPU_X86::MODEL_486 */,
                                       float cpu_frequency /* = 1000000.0f */,
                                       CPU::BackendType cpu_backend /* = CPU::BackendType::Interpreter */,
                                       u32 ram_size /* = 1024 * 1024 */)
  : System()
{
  m_bus = new Bus(32);
  m_bus->AllocateRAM(ram_size);
  m_cpu = CreateComponent<CPU_X86::CPU>("CPU", cpu_model, cpu_frequency, cpu_backend);
  AddComponents();
}

CPU_X86_TestSystem::~CPU_X86_TestSystem() {}

bool CPU_X86_TestSystem::Initialize()
{
  if (!System::Initialize())
    return false;

  // Fill memory regions.
  m_bus->CreateRAMRegion(UINT32_C(0x00000000), UINT32_C(0x0009FFFF));
  m_bus->CreateRAMRegion(UINT32_C(0x00100000), UINT32_C(0xFFFEFFFF));

  for (const ROMFile& rom : m_rom_files)
  {
    if (!m_bus->CreateROMRegionFromFile(rom.filename, 0, rom.load_address, rom.expected_size))
    {
      Log_ErrorPrintf("Failed to load ROM file from '%s'.", rom.filename.GetCharArray());
      return false;
    }
  }

  // Mirror top 64KB.
  m_bus->MirrorRegion(UINT32_C(0xF0000), 0x10000, UINT32_C(0xFFFF0000));
  return true;
}

void CPU_X86_TestSystem::AddROMFile(const char* filename, PhysicalMemoryAddress load_address, u32 expected_size)
{
  m_rom_files.push_back({filename, load_address, expected_size});
}

bool CPU_X86_TestSystem::Execute(SimulationTime timeout /* = SecondsToSimulationTime(60) */)
{
  if (!Initialize())
    return false;

  Reset();

  bool timed_out = false;
  std::unique_ptr<TimingEvent> timeout_event;
  if (timeout != 0)
  {
    timeout_event = CreateNanosecondEvent("Test Timeout", timeout,
                                          [this, &timed_out](TimingEvent*, CycleCount, CycleCount) {
                                            SetState(System::State::Stopped);
                                            timed_out = true;
                                          },
                                          true);
  }

  SetState(State::Running);

  Run();

  if (m_cpu->Cast<CPU_X86::CPU>()->IsHalted())
    timed_out = false;

  return !timed_out;
}

void CPU_X86_TestSystem::AddComponents()
{
  m_interrupt_controller = CreateComponent<HW::i8259_PIC>("InterruptController");
}
