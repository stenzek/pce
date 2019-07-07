#pragma once
#include "pce/host_interface.h"
#include <QtCore/QThread>
#include <memory>

class QKeyEvent;

class MainWindow;
class DisplayWidget;
class DebuggerInterface;
class DebuggerWindow;

class QtHostInterface : public QThread, public HostInterface
{
  Q_OBJECT

public:
  QtHostInterface() = default;
  ~QtHostInterface() = default;

  static std::unique_ptr<QtHostInterface> Create(MainWindow* main_window, DisplayWidget* display_widget);

  DisplayWidget* getDisplayWidget() const { return m_display_widget; }
  bool isDebuggerEnabled() const { return (m_debugger_interface != nullptr); }

  bool HandleQKeyEvent(const QKeyEvent* event);

  DisplayRenderer* GetDisplayRenderer() const override;
  Audio::Mixer* GetAudioMixer() const override;

  void ReportMessage(const char* message) override;

public Q_SLOTS:
  void startSimulation(const QString filename, bool start_paused);
  void pauseSimulation(bool paused);
  void resetSimulation();
  void stopSimulation();
  void sendCtrlAltDel();
  void enableDebugger(bool enabled);

Q_SIGNALS:
  void onSystemInitialized();
  void onSystemDestroy();
  void onSimulationPaused();
  void onSimulationResumed();
  void onSimulationSpeedUpdate(float speed_percent, float vps);
  void onStatusMessage(QString message);
  void onDebuggerEnabled(bool enabled);

protected:
  void OnSystemInitialized() override;
  void OnSystemDestroy() override;
  void OnSimulationResumed() override;
  void OnSimulationPaused() override;
  void OnSimulationStatsUpdate(const SimulationStats& stats) override;
  void YieldToUI() override;
  void run() override;

  MainWindow* m_main_window = nullptr;

  DisplayWidget* m_display_widget = nullptr;

  std::unique_ptr<Audio::Mixer> m_audio_mixer;

  DebuggerInterface* m_debugger_interface = nullptr;
  DebuggerWindow* m_debugger_window = nullptr;

  QThread* m_ui_thread;
};
