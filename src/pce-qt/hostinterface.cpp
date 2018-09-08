#include "pce-qt/hostinterface.h"
#include "YBaseLib/Log.h"
#include "common/audio.h"
#include "pce-qt/displaywidget.h"
#include "pce-qt/mainwindow.h"
#include "pce-qt/scancodes_qt.h"
#include <QtGui/QKeyEvent>
Log_SetChannel(QtHostInterface);

std::unique_ptr<QtHostInterface> QtHostInterface::Create(MainWindow* main_window, DisplayWidget* display_widget)
{
  auto audio_mixer = Audio::NullMixer::Create();
  if (!audio_mixer)
    Panic("Failed to create audio mixer");

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

Display* QtHostInterface::GetDisplay() const
{
  return static_cast<Display*>(m_display_widget);
}

Audio::Mixer* QtHostInterface::GetAudioMixer() const
{
  return m_audio_mixer.get();
}

void QtHostInterface::ReportMessage(const char* message)
{
  m_main_window->m_status_message->setText(message);
}

void QtHostInterface::OnSimulationResumed() {}

void QtHostInterface::OnSimulationPaused() {}

void QtHostInterface::OnSystemDestroy() {}

void QtHostInterface::OnSimulationSpeedUpdate(float speed_percent)
{
  m_main_window->m_status_speed->setText(QString::asprintf("Emulation Speed: %.2f%%", speed_percent));
  m_main_window->m_status_fps->setText(QString::asprintf("VPS: %.1f", m_display_widget->GetFramesPerSecond()));
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
  m_display_widget->moveGLContextToThread(m_ui_thread);
}
