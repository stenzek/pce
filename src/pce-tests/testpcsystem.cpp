#include "pce-tests/testpcsystem.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "common/audio.h"
#include "common/display.h"
#include "pce-tests/helpers.h"
#include "pce/bus.h"
#include "pce/host_interface.h"
#include "pce/mmio.h"
Log_SetChannel(TestPCSystem);

// This leaks at the moment, but whatever.
class DummyHostInterface : public HostInterface
{
public:
  DummyHostInterface()
  {
    m_display = NullDisplay::Create();
    m_audio_mixer = Audio::NullMixer::Create();
  }

  ~DummyHostInterface() {}

  Display* GetDisplay() const override { return m_display.get(); }
  Audio::Mixer* GetAudioMixer() const override { return m_audio_mixer.get(); }

protected:
  std::unique_ptr<Display> m_display;
  std::unique_ptr<Audio::Mixer> m_audio_mixer;
};

TestPCSystem::TestPCSystem(CPU_X86::Model cpu_model /* = CPU_X86::MODEL_486 */, float cpu_frequency /* = 1000000.0f */,
                           CPUBackendType cpu_backend /* = CPUBackendType::Interpreter */,
                           uint32 ram_size /* = 1024 * 1024 */)
  : System(new DummyHostInterface())
{
  m_cpu = new CPU_X86::CPU(cpu_model, cpu_frequency, cpu_backend);
  m_bus = new Bus((cpu_model >= CPU_X86::MODEL_386) ? 32 : 20);
  // AllocatePhysicalMemory(640 * 1024, false);
  m_bus->AllocateRAM(ram_size);
  AddComponents();
}

TestPCSystem::~TestPCSystem()
{
  // Host interface shouldn't be called into, but we'll set it to null so we catch it if it does.
  delete m_host_interface;
  m_host_interface = nullptr;
}

bool TestPCSystem::Initialize()
{
  if (!System::Initialize())
    return false;

  // Fill memory regions.
  m_bus->CreateRAMRegion(uint32(0), uint32(0xFFFFFFFF));

  // Remove the throttle event, it's not needed for the test cases.
  m_throttle_event->Deactivate();
  return true;
}

bool TestPCSystem::AddMMIOROMFromFile(const char* filename, PhysicalMemoryAddress address)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &stream))
  {
    Log_ErrorPrintf("Failed to open ROM image");
    return false;
  }

  size_t size = static_cast<size_t>(stream->GetSize());
  if (size == 0)
  {
    Log_ErrorPrintf("Attempting to add empty file as ROM");
    stream->Release();
    return false;
  }

#if 0
    auto data = std::make_unique<byte[]>(size);
    if (!stream->Read2(data.get(), uint32(size)))
    {
        Log_ErrorPrintf("Failed to read ROM image");
        return false;
    }

    MMIO* mmio = MMIO::CreateDirect(address, uint32(size), data.get(), true, false);
    Assert(mmio);
    m_bus->RegisterMMIO(mmio);
    mmio->Release();

    m_rom_data.push_back(std::move(data));
    Log_InfoPrintf("%s mapped in ROM at address 0x%08X", filename, uint32(address));
#else
  // Align size to page size.
  PhysicalMemoryAddress aligned_address = address & Bus::MEMORY_PAGE_MASK;
  PhysicalMemoryAddress start_padding = aligned_address - address;
  size_t aligned_size = (size + start_padding + Bus::MEMORY_PAGE_SIZE - 1) & Bus::MEMORY_PAGE_MASK;
  uint32 allocated_size = m_bus->CreateRAMRegion(PhysicalMemoryAddress(aligned_address),
                                                 PhysicalMemoryAddress(aligned_address + (aligned_size - 1)));
  Assert(allocated_size >= size);

  PhysicalMemoryAddress current_address = address + start_padding;
  size_t remaining_data = size;
  while (remaining_data > 0)
  {
    byte data = 0;
    if (!stream->ReadByte(&data))
      break;
    m_bus->WriteMemoryByte(current_address, data);
    current_address++;
    remaining_data--;
  }
#endif

  return true;
}

bool TestPCSystem::Ready()
{
  if (!Initialize())
    return false;

  Reset();
  SetState(State::Running);
  return true;
}

void TestPCSystem::AddComponents()
{
  m_interrupt_controller = new HW::i8259_PIC();
  AddComponent(m_interrupt_controller);
}
