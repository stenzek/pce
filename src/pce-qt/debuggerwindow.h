#pragma once

#include "pce-qt/debuggermodels.h"
#include "pce-qt/ui_debuggerwindow.h"
#include <QtWidgets/QMainWindow>
#include <memory>

class DebuggerWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit DebuggerWindow(DebuggerInterface* debugger_interface, QWidget* parent = nullptr);
  ~DebuggerWindow();

  void onExecutionContinued();
  void onExecutionStopped();

public Q_SLOTS:
  void refreshAll();
  void onCloseActionTriggered();
  void onRunActionTriggered(bool checked);
  void onSingleStepActionTriggered();

private:
  void connectSignals();
  void createModels();
  void setMonitorUIState(bool enabled);

  DebuggerInterface* m_debugger_interface;

  std::unique_ptr<Ui::DebuggerWindow> m_ui;

  std::unique_ptr<DebuggerCodeModel> m_code_model;
  std::unique_ptr<DebuggerRegistersModel> m_registers_model;
  std::unique_ptr<DebuggerStackModel> m_stack_model;
};
