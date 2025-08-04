#include "YBaseLib/Log.h"
#include "YBaseLib/Platform.h"
#include "YBaseLib/Subprocess.h"
#include "YBaseLib/Thread.h"
Log_SetChannel(TestSubprocess);

static void parent()
{
  printf("parent process\n");

  String filename;
  Platform::GetProgramFileName(filename);
  Subprocess* p = Subprocess::Create(filename);
  if (p == nullptr)
  {
    Log_ErrorPrintf("Failed to create child");
    return;
  }

  Subprocess::Connection* conn = p->ConnectToChild();
  if (conn == nullptr)
  {
    Log_ErrorPrintf("failed to connect to child");
    delete p;
    return;
  }

  printf("connected with child\n");

  conn->IsOtherSideAlive();

  uint32 msg;
  uint32 b = conn->ReadDataNonBlocking(&msg, sizeof(msg));
  UNREFERENCED_PARAMETER(b);

  Thread::Sleep(5000);

  printf("requesting child exit\n");
  p->RequestChildToExit();
  p->WaitForChildToExit();
  printf("child exited\n");
}

static void child()
{
  printf("child process\n");

  Subprocess::Connection* conn = Subprocess::ConnectToParent();
  if (conn == nullptr)
  {
    Log_ErrorPrintf("Failed to connect to parent");
    return;
  }

  printf("Connected with parent\n");
  while (!conn->IsExitRequested())
    Thread::Sleep(1);

  printf("Exiting\n");
  delete conn;
}

int main(int argc, char* argv[])
{
  Log::GetInstance().SetConsoleOutputParams(true);
  Log::GetInstance().SetDebugOutputParams(true);

  if (!Subprocess::IsSubprocess())
    parent();
  else
    child();

  return 0;
}
