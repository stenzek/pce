#pragma once

#include "pce-qt/ui_mainwindow.h"
#include "pce/cpu.h"
#include "pce/host_interface.h"
#include "pce/system.h"
#include "pce/systems/pcbochs.h"
#include <QtCore/QThread>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <memory>

class Display;
class DisplayWidget;
class DebuggerWindow;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

  // TODO: Remove this and replace with config system
  bool createTestSystem(uint32 cpu_model, uint32 ram_mb, const char* bios_filename, const char* vgabios_filename);

  bool setTestSystemStorage(const char* floppy_a_filename = nullptr, const char* floppy_b_filename = nullptr,
                            const char* hdd_filename = nullptr, uint32 hdd_cylinders = 0, uint32 hdd_heads = 0,
                            uint32 hdd_sectors = 0);

  bool isSystemStopped() const;
  bool startSystem(bool start_paused = false);
  void pauseSystem();
  void stopSystem();

  void runMainLoop();

public Q_SLOTS:
  void onEnableDebuggerActionToggled(bool checked);
  void onResetActionTriggered();
  void onAboutActionTriggered();
  void onChangeFloppyATriggered();
  void onChangeFloppyBTriggered();

  void onDisplayWidgetKeyPressed(QKeyEvent* event);
  void onDisplayWidgetKeyReleased(QKeyEvent* event);

private:
  void connectSignals();
  void enableDebugger();
  void disableDebugger();

  class HostInterface;
  class SystemThread;

  std::unique_ptr<Ui::MainWindow> m_ui;
  DisplayWidget* m_display_widget = nullptr;
  QLabel* m_status_message = nullptr;
  QLabel* m_status_speed = nullptr;
  QLabel* m_status_fps = nullptr;

  std::unique_ptr<HostInterface> m_host_interface;

  DebuggerInterface* m_debugger_interface = nullptr;
  std::unique_ptr<DebuggerWindow> m_debugger_window;

  std::unique_ptr<Systems::PCBochs> m_system;
  std::unique_ptr<SystemThread> m_system_thread;

  class HostInterface : public ::HostInterface
  {
  public:
    HostInterface() = default;
    ~HostInterface() = default;

    static std::unique_ptr<HostInterface> Create(MainWindow* main_window, DisplayWidget* display_widget);

    DisplayWidget* getDisplayWidget() const { return m_display_widget; }

    bool HandleQKeyEvent(const QKeyEvent* event);

    Display* GetDisplay() const override;
    Audio::Mixer* GetAudioMixer() const override;

    void ReportMessage(const char* message) override;

    void OnSimulationResumed() override;
    void OnSimulationPaused() override;
    void OnSimulationStopped() override;

    void OnSimulationSpeedUpdate(float speed_percent) override;

  protected:
    MainWindow* m_main_window = nullptr;
    DisplayWidget* m_display_widget = nullptr;
    std::unique_ptr<Audio::Mixer> m_audio_mixer;
  };

  // We need this mess because cross-thread signals aren't possible.
  class SystemThread : public QThread
  {
  public:
    SystemThread(System* system, DisplayWidget* display_widget, QThread* ui_thread, System::State initial_state);
    void run() override;

  protected:
    System* m_system;
    DisplayWidget* m_display_widget;
    QThread* m_ui_thread;
    System::State m_initial_state;
  };
};
