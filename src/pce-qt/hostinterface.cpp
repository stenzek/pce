#include "pce-qt/hostinterface.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/Log.h"
#include "common/audio.h"
#include "common/display_renderer.h"
#include "pce-qt/debuggerwindow.h"
#include "pce-qt/displaywidget.h"
#include "pce-qt/mainwindow.h"
#include "pce-qt/scancodes_qt.h"
#include "pce/cpu.h"
#include "pce/debugger_interface.h"
#include <QtGui/QKeyEvent>
#include <QtWidgets/QMessageBox>
Log_SetChannel(QtHostInterface);

std::unique_ptr<QtHostInterface> QtHostInterface::Create(MainWindow* main_window, DisplayWidget* display_widget)
{
  auto audio_mixer = Audio::NullMixer::Create();
  if (!audio_mixer)
    Panic("Failed to create audio mixer");

  const auto display_widget_size = display_widget->size();
  std::unique_ptr<DisplayRenderer> display_renderer = DisplayRenderer::Create(
    DisplayRenderer::BackendType::Null, reinterpret_cast<DisplayRenderer::WindowHandleType>(main_window->winId()),
    display_widget_size.width(), display_widget_size.height());
  if (!display_renderer)
    Panic("Failed to create display renderer");

  auto hi = std::make_unique<QtHostInterface>();
  hi->m_main_window = main_window;
  hi->m_display_widget = display_widget;
  hi->m_audio_mixer = std::move(audio_mixer);
  hi->m_ui_thread = QThread::currentThread();
  return hi;
}

bool QtHostInterface::HandleQKeyEvent(const QKeyEvent* event)
{
  GenScanCode scancode;
  if (!MapQTKeyToGenScanCode(&scancode, static_cast<Qt::Key>(event->key())))
    return false;

  ExecuteKeyboardCallbacks(scancode, event->type() == QEvent::KeyPress);
  return true;
}

DisplayRenderer* QtHostInterface::GetDisplayRenderer() const
{
  return m_display_widget->getDisplayRenderer();
}

Audio::Mixer* QtHostInterface::GetAudioMixer() const
{
  return m_audio_mixer.get();
}

void QtHostInterface::ReportMessage(const char* message)
{
  emit onStatusMessage(QString(message));
}

void QtHostInterface::startSimulation(const QString filename, bool start_paused)
{
  if (m_system)
    return;

  Error error;
  if (!CreateSystem(filename.toStdString().c_str(), &error))
  {
    QString message = error.GetErrorCodeAndDescription().GetCharArray();
    QMessageBox::critical(m_main_window, "System creation error", message);
    return;
  }

  if (!start_paused)
    ResumeSimulation();
}

void QtHostInterface::pauseSimulation(bool paused)
{
  if (!m_system)
    return;

  if (paused)
  {
    if (m_system->GetState() != System::State::Paused)
      PauseSimulation();
  }
  else
  {
    if (m_system->GetState() == System::State::Paused)
      ResumeSimulation();
  }
}

void QtHostInterface::resetSimulation()
{
  if (!m_system)
    return;

  ResetSystem();
}

void QtHostInterface::stopSimulation()
{
  if (!m_system)
    return;

  StopSimulation();
}

void QtHostInterface::sendCtrlAltDel()
{
  if (!m_system)
    return;

  SendCtrlAltDel();
}

void QtHostInterface::enableDebugger(bool enabled)
{
  if (!m_system || enabled == isDebuggerEnabled())
    return;

  const bool was_paused = (m_system->GetState() == System::State::Paused);

  if (enabled)
  {
    if (!was_paused)
      PauseSimulation();

    m_debugger_interface = m_system->GetCPU()->GetDebuggerInterface();
    if (!m_debugger_interface)
    {
      QMessageBox::critical(m_main_window, "Error", "Failed to get debugger interface", QMessageBox::Ok);
      if (!was_paused)
        ResumeSimulation();

      return;
    }

    m_debugger_window = new DebuggerWindow(m_debugger_interface, m_main_window);
    connect(this, SIGNAL(onSimulationPaused()), m_debugger_window, SLOT(onSimulationPaused()));
    connect(this, SIGNAL(onSimulationResumed()), m_debugger_window, SLOT(onSimulationResumed()));
    m_debugger_window->show();
  }
  else
  {
    if (!was_paused)
      PauseSimulation();

    // TODO: Improve this.
    m_debugger_interface = nullptr;
    delete m_debugger_window;
    m_debugger_window = nullptr;
    ResumeSimulation();
  }

  emit onDebuggerEnabled(enabled);
}

void QtHostInterface::OnSystemInitialized()
{
  HostInterface::OnSystemInitialized();
  emit onSystemInitialized();
}

void QtHostInterface::OnSystemDestroy()
{
  HostInterface::OnSystemDestroy();
  emit onSystemDestroy();
}

void QtHostInterface::OnSimulationResumed()
{
  HostInterface::OnSimulationResumed();
  emit onSimulationResumed();
}

void QtHostInterface::OnSimulationPaused()
{
  HostInterface::OnSimulationPaused();
  emit onSimulationPaused();
}

void QtHostInterface::OnSimulationSpeedUpdate(float speed_percent)
{
  HostInterface::OnSimulationSpeedUpdate(speed_percent);
  emit onSimulationSpeedUpdate(speed_percent, GetDisplayRenderer()->GetPrimaryDisplayFramesPerSecond());
}

void QtHostInterface::YieldToUI()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void QtHostInterface::run()
{
  // Run the system simulation, sleeping when paused.
  SimulationThreadRoutine();

  // Move the OpenGL thread back to the UI thread before exiting.
  // m_display_widget->moveGLContextToThread(m_ui_thread);
}
