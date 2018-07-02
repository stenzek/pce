#include "pce-qt/mainwindow.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Timer.h"
#include "pce-qt/debuggerwindow.h"
#include "pce-qt/displaywidget.h"
#include "pce-qt/scancodes_qt.h"
#include "pce/audio.h"
#include "pce/cpu_x86/cpu.h"
#include "pce/debugger_interface.h"
#include "pce/hw/vga.h"
#include "pce/systems/pcbochs.h"
#include <QtGui/QKeyEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
Log_SetChannel(MainWindow);

static bool LoadBIOS(const char* filename, std::function<bool(ByteStream*)> callback)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED, &stream))
  {
    Log_ErrorPrintf("Failed to open code file %s", filename);
    return false;
  }

  bool result = callback(stream);
  stream->Release();
  return result;
}

static bool LoadFloppy(HW::FDC* fdc, uint32 disk, const char* path)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(path, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE, &stream))
  {
    Log_ErrorPrintf("Failed to load floppy at %s", path);
    return false;
  }

  HW::FDC::DiskType disk_type = HW::FDC::DetectDiskType(stream);
  if (disk_type == HW::FDC::DiskType_None)
  {
    stream->Release();
    return false;
  }

  bool result = fdc->InsertDisk(disk, disk_type, stream);
  stream->Release();
  return result;
}

static bool LoadHDD(HW::HDC* hdc, uint32 drive, const char* path, uint32 cylinders, uint32 heads, uint32 sectors)
{
  ByteStream* stream;
  if (!ByteStream_OpenFileStream(path, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE, &stream))
  {
    Log_ErrorPrintf("Failed to load HDD at %s", path);
    return false;
  }

  bool result = hdc->AttachDrive(drive, stream, cylinders, heads, sectors);
  stream->Release();
  return result;
}

MainWindow::MainWindow(QWidget* parent /*= nullptr*/) : QMainWindow(parent)
{
  m_ui = std::make_unique<Ui::MainWindow>();
  m_ui->setupUi(this);

  m_display_widget = new DisplayWidget(this);
  setCentralWidget(m_display_widget);

  m_ui->centralwidget->deleteLater();
  m_ui->centralwidget = nullptr;

  m_status_message = new QLabel(this);
  m_ui->statusbar->addWidget(m_status_message, 1);
  m_status_speed = new QLabel(this);
  m_ui->statusbar->addWidget(m_status_speed, 0);
  m_status_fps = new QLabel(this);
  m_ui->statusbar->addWidget(m_status_fps, 0);

  connectSignals();
  adjustSize();
}

MainWindow::~MainWindow() {}

bool MainWindow::createTestSystem(uint32 cpu_model, uint32 ram_mb, const char* bios_filename,
                                  const char* vgabios_filename)
{
  m_host_interface = HostInterface::Create(this, m_display_widget);

  CPU_X86::Model real_cpu_model;
  float real_cpu_frequency;
  if (cpu_model == 0)
  {
    real_cpu_model = CPU_X86::MODEL_386;
    real_cpu_frequency = 4000000.0f;
  }
  else
  {
    real_cpu_model = CPU_X86::MODEL_486;
    real_cpu_frequency = 8000000.0f;
  }

  m_system = std::unique_ptr<Systems::PCBochs>(
    new Systems::PCBochs(m_host_interface.get(), real_cpu_model, real_cpu_frequency, ram_mb * 1024 * 1024));

  if (!LoadBIOS(bios_filename, [this](ByteStream* stream) {
        return m_system->AddROM(0xF0000, stream) && m_system->AddROM(0xFFFF0000u, stream);
      }))
  {
    return false;
  }

  HW::VGA* vga = new HW::VGA();
  m_system->AddComponent(vga);
  if (!LoadBIOS(vgabios_filename, [vga](ByteStream* stream) { return vga->SetBIOSROM(stream); }))
    return false;

  m_display_widget->SetDisplayAspectRatio(4, 3);
  return true;
}

bool MainWindow::setTestSystemStorage(const char* floppy_a_filename, const char* floppy_b_filename,
                                      const char* hdd_filename, uint32 hdd_cylinders, uint32 hdd_heads,
                                      uint32 hdd_sectors)
{
  // TODO: Fix this crap
  HW::FDC* fdc = m_system->GetFDDController();
  HW::HDC* hdc = m_system->GetHDDController();
  if (floppy_a_filename && !LoadFloppy(fdc, 0, floppy_a_filename))
    return false;
  if (floppy_b_filename && !LoadFloppy(fdc, 0, floppy_b_filename))
    return false;
  if (hdd_filename && !LoadHDD(hdc, 0, hdd_filename, hdd_cylinders, hdd_heads, hdd_sectors))
    return false;

  return true;
}

bool MainWindow::isSystemStopped() const
{
  return (m_system->GetState() == System::State::Stopped);
}

bool MainWindow::startSystem(bool start_paused /* = false */)
{
  // Since we use the core notification for knowing when the system starts, we have to clear the simulation
  // thread ID. Otherwise SetState() will assume that the UI thread is the simulation thread.
  m_system->ClearSimulationThreadID();

  // Start the worker thread.
  const System::State initial_state = start_paused ? System::State::Paused : System::State::Running;
  m_system_thread =
    std::make_unique<SystemThread>(m_system.get(), m_display_widget, QThread::currentThread(), initial_state);
  m_system_thread->start();

  // Transfer the OpenGL widget to the worker thread before starting the system, so it can render.
  m_display_widget->moveGLContextToThread(m_system_thread.get());

  // Ensure input goes to the simulated PC.
  m_display_widget->setFocus();
  return true;
}

void MainWindow::pauseSystem()
{
  Assert(m_system->GetState() == System::State::Running);
  m_system->SetState(System::State::Paused);
}

void MainWindow::stopSystem()
{
  Assert(m_system->GetState() != System::State::Stopped);
  m_system->SetState(System::State::Stopped);

  // Clean up the worker thread, when we start again it'll be recreated.
  m_system_thread->wait();
  m_system_thread.reset();
}

void MainWindow::runMainLoop()
{
#if 0
    // Poll input at 60hz
    Timer exec_timer;
    const float exec_time = 1.0f / 60.0f;
    System::State last_state = m_system->GetState();

    for (;;)
    {
        exec_timer.Reset();
        m_system->Run();
        if (last_state != m_system->GetState())
        {
            last_state = m_system->GetState();
            if (last_state == System::State::Stopped)
                break;

            if (m_debugger_window)
            {
                if (last_state == System::State::Paused)
                    m_debugger_window->onExecutionStopped();
                else
                    m_debugger_window->onExecutionContinued();
            }                
        }

        QApplication::sendPostedEvents();

        float sleep_time = exec_time - float(exec_timer.GetTimeSeconds());
        if (sleep_time > 0.0f)
            QApplication::processEvents(QEventLoop::AllEvents, uint32(sleep_time * 1000.0f));
        else
            QApplication::processEvents(QEventLoop::AllEvents);
    }
#endif
}

MainWindow::SystemThread::SystemThread(System* system, DisplayWidget* display_widget, QThread* ui_thread,
                                       System::State initial_state)
  : m_system(system), m_display_widget(display_widget), m_ui_thread(ui_thread), m_initial_state(initial_state)
{
}

void MainWindow::SystemThread::run()
{
  // Run the system simulation, sleeping when paused.
  m_system->Start(true);
  m_system->SetState(m_initial_state);
  m_system->Run(false);

  // Move the OpenGL thread back to the UI thread before exiting.
  m_display_widget->moveGLContextToThread(m_ui_thread);
}

void MainWindow::onEnableDebuggerActionToggled(bool checked)
{
  if (checked)
    enableDebugger();
  else
    disableDebugger();
}

void MainWindow::onResetActionTriggered() {}

void MainWindow::onAboutActionTriggered()
{
  QMessageBox::about(this, tr("PC Emulator"), tr("Blah!"));
}

void MainWindow::onChangeFloppyATriggered()
{
  QString filename =
    QFileDialog::getOpenFileName(this, "Select disk image", QString(), QString(), nullptr, QFileDialog::ReadOnly);

  QMessageBox::information(this, filename, filename);
}

void MainWindow::onChangeFloppyBTriggered() {}

void MainWindow::connectSignals()
{
  connect(m_ui->actionEnableDebugger, SIGNAL(toggled(bool)), this, SLOT(onEnableDebuggerActionToggled(bool)));
  connect(m_ui->action_About, SIGNAL(triggered()), this, SLOT(onAboutActionTriggered()));
  connect(m_ui->actionChange_Floppy_A, SIGNAL(triggered()), this, SLOT(onChangeFloppyATriggered()));

  connect(m_display_widget, SIGNAL(onKeyPressed(QKeyEvent*)), this, SLOT(onDisplayWidgetKeyPressed(QKeyEvent*)));
  connect(m_display_widget, SIGNAL(onKeyReleased(QKeyEvent*)), this, SLOT(onDisplayWidgetKeyReleased(QKeyEvent*)));
}

void MainWindow::enableDebugger()
{
  Assert(!m_debugger_window);

  DebuggerInterface* debugger_interface = m_system->GetCPU()->GetDebuggerInterface();
  if (!debugger_interface)
  {
    QMessageBox::critical(this, "Error", "Failed to get debugger interface", QMessageBox::Ok);
    return;
  }

  // Pause execution in its current state, the debugger assumes this when it opens
  m_debugger_interface = debugger_interface;
  m_debugger_interface->SetStepping(true);

  m_debugger_window = std::make_unique<DebuggerWindow>(m_debugger_interface);
  m_debugger_window->show();
}

void MainWindow::disableDebugger()
{
  Assert(m_debugger_window);

  m_debugger_interface->SetStepping(false);
  m_debugger_window->close();
  m_debugger_window.reset();
  m_debugger_interface = nullptr;
}

void MainWindow::onDisplayWidgetKeyPressed(QKeyEvent* event)
{
  m_host_interface->HandleQKeyEvent(event);
}

void MainWindow::onDisplayWidgetKeyReleased(QKeyEvent* event)
{
  m_host_interface->HandleQKeyEvent(event);
}

std::unique_ptr<MainWindow::HostInterface> MainWindow::HostInterface::Create(MainWindow* main_window,
                                                                             DisplayWidget* display_widget)
{
  auto audio_mixer = Audio::NullMixer::Create();
  if (!audio_mixer)
    Panic("Failed to create audio mixer");

  auto hi = std::make_unique<HostInterface>();
  hi->m_main_window = main_window;
  hi->m_display_widget = display_widget;
  hi->m_audio_mixer = std::move(audio_mixer);
  return hi;
}

bool MainWindow::HostInterface::HandleQKeyEvent(const QKeyEvent* event)
{
  GenScanCode scancode;
  if (!MapQTKeyToGenScanCode(&scancode, static_cast<Qt::Key>(event->key())))
    return false;

  this->ExecuteKeyboardCallback(scancode, event->type() == QEvent::KeyPress);
  return true;
}

Display* MainWindow::HostInterface::GetDisplay() const
{
  return static_cast<Display*>(m_display_widget);
}

Audio::Mixer* MainWindow::HostInterface::GetAudioMixer() const
{
  return m_audio_mixer.get();
}

void MainWindow::HostInterface::ReportMessage(const char* message)
{
  m_main_window->m_status_message->setText(message);
}

void MainWindow::HostInterface::OnSimulationResumed() {}

void MainWindow::HostInterface::OnSimulationPaused() {}

void MainWindow::HostInterface::OnSimulationStopped() {}

void MainWindow::HostInterface::OnSimulationSpeedUpdate(float speed_percent)
{
  m_main_window->m_status_speed->setText(QString::asprintf("Emulation Speed: %.2f%%", speed_percent));
  m_main_window->m_status_fps->setText(QString::asprintf("VPS: %.1f", m_display_widget->GetFramesPerSecond()));
}
