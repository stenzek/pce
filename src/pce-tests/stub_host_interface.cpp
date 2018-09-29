#include "stub_host_interface.h"
#include "common/audio.h"
#include "common/display_renderer.h"

static std::unique_ptr<StubHostInterface> s_host_interface;

StubHostInterface::StubHostInterface()
{
  m_display_renderer = DisplayRenderer::Create(DisplayRenderer::BackendType::Null, nullptr, 0, 0);
  m_audio_mixer = Audio::NullMixer::Create();
}

StubHostInterface::~StubHostInterface() {}

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
    s_host_interface->m_system->SetState(System::State::Stopped);

  s_host_interface->m_system.reset();
}

DisplayRenderer* StubHostInterface::GetDisplayRenderer() const
{
  return m_display_renderer.get();
}

Audio::Mixer* StubHostInterface::GetAudioMixer() const
{
  return m_audio_mixer.get();
}
