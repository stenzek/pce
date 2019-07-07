#pragma once
#include "pce/host_interface.h"

template<class T>
class StubSystemPointer
{
public:
  StubSystemPointer(T* system);
  StubSystemPointer(StubSystemPointer<T>&& p);
  ~StubSystemPointer();

  operator T*() { return m_system; }
  T* operator->() { return m_system; }

  StubSystemPointer<T>& operator=(StubSystemPointer<T>&& p);

private:
  T* m_system;
};

class StubHostInterface : public HostInterface
{
public:
  StubHostInterface();
  ~StubHostInterface();

  DisplayRenderer* GetDisplayRenderer() const override;
  Audio::Mixer* GetAudioMixer() const override;

  template<typename T, typename... Args>
  static StubSystemPointer<T> CreateSystem(Args... args)
  {
    std::unique_ptr<T> system = std::make_unique<T>(args...);
    T* system_ptr = system.get();
    SetSystem(std::move(system));
    return StubSystemPointer<T>(system_ptr);
  }

  static StubHostInterface* GetInstance();
  static void SetSystem(std::unique_ptr<System> system);
  static void ReleaseSystem();

protected:
  static void DisplayCPUStats(CPU* cpu);

  std::unique_ptr<DisplayRenderer> m_display_renderer;
  std::unique_ptr<Audio::Mixer> m_audio_mixer;
};

template<class T>
StubSystemPointer<T>::StubSystemPointer(T* system)
{
  m_system = system;
}

template<class T>
StubSystemPointer<T>::StubSystemPointer(StubSystemPointer<T>&& p)
{
  m_system = p.m_system;
  p.m_system = nullptr;
}

template<class T>
StubSystemPointer<T>::~StubSystemPointer()
{
  if (StubHostInterface::GetInstance()->GetSystem() == m_system)
    StubHostInterface::ReleaseSystem();
}

template<class T>
StubSystemPointer<T>& StubSystemPointer<T>::operator=(StubSystemPointer<T>&& p)
{
  m_system = p.m_system;
  p.m_system = nullptr;
  return *this;
}
