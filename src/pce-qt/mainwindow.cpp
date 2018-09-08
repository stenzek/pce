#include "pce-qt/mainwindow.h"
#include "YBaseLib/Log.h"
#include "pce-qt/debuggerwindow.h"
#include "pce-qt/displaywidget.h"
#include "pce-qt/hostinterface.h"
#include "pce/debugger_interface.h"
#include <QtGui/QKeyEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
Log_SetChannel(MainWindow);

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

  m_display_widget->SetDisplayAspectRatio(4, 3);
  m_host_interface = QtHostInterface::Create(this, m_display_widget);
  m_host_interface->start();

  // Transfer the OpenGL widget to the worker thread before starting the system, so it can render.
  m_display_widget->moveGLContextToThread(m_host_interface.get());

  // Ensure input goes to the simulated PC.
  // m_display_widget->setFocus();

  adjustSize();
}

MainWindow::~MainWindow()
{
  m_host_interface.reset();
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

  DebuggerInterface* debugger_interface = m_host_interface->GetSystem()->GetCPU()->GetDebuggerInterface();
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
