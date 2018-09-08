#pragma once
#include "pce/host_interface.h"
#include <QtCore/QThread>
#include <memory>

class QKeyEvent;

class MainWindow;
class DisplayWidget;

class QtHostInterface : public QThread, public HostInterface
{
  Q_OBJECT

public:
  QtHostInterface() = default;
  ~QtHostInterface() = default;

  static std::unique_ptr<QtHostInterface> Create(MainWindow* main_window, DisplayWidget* display_widget);

  DisplayWidget* getDisplayWidget() const { return m_display_widget; }

  bool HandleQKeyEvent(const QKeyEvent* event);

  Display* GetDisplay() const override;
  Audio::Mixer* GetAudioMixer() const override;

  void ReportMessage(const char* message) override;

protected:
  void OnSystemDestroy() override;
  void OnSimulationResumed() override;
  void OnSimulationPaused() override;
  void OnSimulationSpeedUpdate(float speed_percent) override;
  void YieldToUI() override;
  void run() override;

  MainWindow* m_main_window = nullptr;
  DisplayWidget* m_display_widget = nullptr;
  std::unique_ptr<Audio::Mixer> m_audio_mixer;
  QThread* m_ui_thread;
};
