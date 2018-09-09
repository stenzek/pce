#include "YBaseLib/Log.h"
#include "debuggerwindow.h"
#include "hostinterface.h"
#include "mainwindow.h"
#include "pce/types.h"
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
  RegisterAllTypes();

  QApplication app(argc, argv);

  std::unique_ptr<MainWindow> window = std::make_unique<MainWindow>();
  window->show();

  if (argc > 1)
    window->GetHostInterface()->startSimulation(argv[1], false);

  // window->onEnableDebuggerActionToggled(true);

  return app.exec();
}