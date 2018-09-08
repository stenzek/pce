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

  std::unique_ptr<MainWindow> window = std::make_unique<MainWindow>();
  window->show();

  // window->onEnableDebuggerActionToggled(true);

  return app.exec();
}