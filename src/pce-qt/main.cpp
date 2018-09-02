#include "YBaseLib/Log.h"
#include "pce-qt/debuggerwindow.h"
#include "pce-qt/mainwindow.h"
#include <QtWidgets/QApplication>
#include <memory>

static void InitLogging()
{
  // set log flags
  // g_pLog->SetConsoleOutputParams(true);
  g_pLog->SetConsoleOutputParams(true, nullptr, LOGLEVEL_PROFILE);
  g_pLog->SetFilterLevel(LOGLEVEL_PROFILE);
  // g_pLog->SetDebugOutputParams(true);
}

int main(int argc, char* argv[])
{
  InitLogging();

  QApplication app(argc, argv);

  //     std::unique_ptr<MainWindow> main_window = std::make_unique<MainWindow>();
  //     main_window->show();
  //
  //     std::unique_ptr<CPU::Interpreter> interpreter = std::make_unique<CPU::Interpreter>(CPU::MODEL_386, 1.0f);
  //     std::unique_ptr<CPU::DebuggerInterface> debugger_interface =
  //     std::make_unique<CPU::DebuggerInterface>(interpreter.get(), nullptr); std::unique_ptr<DebuggerWindow>
  //     dbg_window = std::make_unique<DebuggerWindow>(debugger_interface.get()); dbg_window->show();

  std::unique_ptr<MainWindow> window = std::make_unique<MainWindow>();
  window->show();

  bool result = true;

  result = result && window->createTestSystem(1, 32, "romimages/BIOS-bochs-legacy", "romimages/VGABIOS-lgpl-latest");
  result = result && window->startSystem();

  if (!result)
    return 1;

  // window->onEnableDebuggerActionToggled(true);

  window->runMainLoop();

  return app.exec();
}