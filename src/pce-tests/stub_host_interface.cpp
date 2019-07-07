#include "stub_host_interface.h"
#include "YBaseLib/Log.h"
#include "common/audio.h"
#include "common/display_renderer.h"
#include <cinttypes>
Log_SetChannel(StubHostInterface);

static std::unique_ptr<StubHostInterface> s_host_interface;

StubHostInterface::StubHostInterface()
{
  m_display_renderer = DisplayRenderer::Create(DisplayRenderer::BackendType::Null, nullptr, 0, 0);
  m_audio_mixer = Audio::NullMixer::Create();
}

StubHostInterface::~StubHostInterface() {}

StubHostInterface* StubHostInterface::GetInstance()
{
  if (!s_host_interface)
    s_host_interface = std::make_unique<StubHostInterface>();

  return s_host_interface.get();
}

void StubHostInterface::SetSystem(std::unique_ptr<System> system)
{
  if (!s_host_interface)
    s_host_interface = std::make_unique<StubHostInterface>();

  if (s_host_interface->m_system)
    ReleaseSystem();

  if (system)
    system->SetHostInterface(s_host_interface.get());

  s_host_interface->m_system = std::move(system);
}

void StubHostInterface::ReleaseSystem()
{
  if (s_host_interface->m_system)
  {
    s_host_interface->m_system->SetState(System::State::Stopped);
    DisplayCPUStats(s_host_interface->m_system->GetCPU());
  }

  s_host_interface->m_system.reset();
}

void StubHostInterface::DisplayCPUStats(CPU* cpu)
{
  CPU::ExecutionStats stats;
  s_host_interface->m_system->GetCPU()->GetExecutionStats(&stats);
  Log_DevPrintf("Cycles executed: %" PRIu64, stats.cycles_executed);
  Log_DevPrintf("Instructions executed: %" PRIu64 " (%" PRIu64 " cached)",
                stats.instructions_interpreted + stats.code_cache_instructions_executed,
                stats.code_cache_instructions_executed);
  Log_DevPrintf("Exceptions: %" PRIu64 ", interrupts: %" PRIu64 ", blocks: %" PRIu64, stats.exceptions_raised,
                stats.interrupts_serviced, stats.num_code_cache_blocks);
}

DisplayRenderer* StubHostInterface::GetDisplayRenderer() const
{
  return m_display_renderer.get();
}

Audio::Mixer* StubHostInterface::GetAudioMixer() const
{
  return m_audio_mixer.get();
}
