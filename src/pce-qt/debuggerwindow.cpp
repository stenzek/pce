#include "pce-qt/debuggerwindow.h"
#include "pce-qt/displaywidget.h"
#include "pce/debugger_interface.h"
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

DebuggerWindow::DebuggerWindow(DebuggerInterface* debugger_interface, QWidget* parent /* = nullptr */)
  : QMainWindow(parent), m_debugger_interface(debugger_interface)
{
  m_ui = std::make_unique<Ui::DebuggerWindow>();
  m_ui->setupUi(this);
  connectSignals();
  createModels();
  setMonitorUIState(true);
  refreshAll();
}

DebuggerWindow::~DebuggerWindow() {}

void DebuggerWindow::onSimulationPaused()
{
  setMonitorUIState(true);
  refreshAll();
}

void DebuggerWindow::onSimulationResumed()
{
  setMonitorUIState(false);
}

void DebuggerWindow::refreshAll()
{
  m_registers_model->invalidateView();
  m_stack_model->invalidateView();
  int row = m_code_model->updateInstructionPointer();
  if (row >= 0)
    m_ui->codeView->scrollTo(m_code_model->index(row, 0));
}

void DebuggerWindow::onCloseActionTriggered() {}

void DebuggerWindow::onRunActionTriggered(bool checked)
{
  if (m_debugger_interface->IsStepping() == !checked)
    return;

  m_debugger_interface->SetStepping(checked);
}

void DebuggerWindow::onSingleStepActionTriggered()
{
  m_debugger_interface->SetStepping(true, 1);
}

void DebuggerWindow::connectSignals()
{
  connect(m_ui->actionPause_Continue, SIGNAL(triggered(bool)), this, SLOT(onRunActionTriggered(bool)));
  connect(m_ui->actionStep_Into, SIGNAL(triggered()), this, SLOT(onSingleStepActionTriggered()));
  connect(m_ui->action_Close, SIGNAL(triggered()), this, SLOT(onCloseActionTriggered()));
}

void DebuggerWindow::createModels()
{
  m_code_model = std::make_unique<DebuggerCodeModel>(m_debugger_interface);
  m_ui->codeView->setModel(m_code_model.get());

  m_registers_model = std::make_unique<DebuggerRegistersModel>(m_debugger_interface);
  m_ui->registerView->setModel(m_registers_model.get());
  // m_ui->registerView->resizeRowsToContents();

  m_stack_model = std::make_unique<DebuggerStackModel>(m_debugger_interface);
  m_ui->stackView->setModel(m_stack_model.get());
}

void DebuggerWindow::setMonitorUIState(bool enabled)
{
  // Disable all UI elements that depend on execution state
  m_ui->actionPause_Continue->setChecked(!enabled);
  m_ui->codeView->setDisabled(!enabled);
  m_ui->registerView->setDisabled(!enabled);
  m_ui->stackView->setDisabled(!enabled);
  // m_ui->tabMemoryView
}
