#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/String.h"

#include <sys/types.h>
#include <unistd.h>

class Subprocess
{
public:
  class Connection
  {
  private:
    Connection(pid_t targetProcessID, int readPipe, int hWritePipe, bool isParentConnection,
               bool terminateOnParentLost);

  public:
    ~Connection();

  public:
    const bool IsParentConnection() const { return m_isParentConnection; }

    bool IsOtherSideAlive();
    bool IsExitRequested();

    bool WaitForData(int32 timeout = -1);

    uint32 ReadDataNonBlocking(void* pBuffer, uint32 byteCount);
    uint32 ReadDataBlocking(void* pBuffer, uint32 byteCount);

    uint32 WriteData(const void* pBuffer, uint32 byteCount);

  private:
    friend class Subprocess;
    pid_t m_targetProcess;
    int m_readPipe;
    int m_writePipe;
    bool m_isParentConnection;
    bool m_terminateOnParentLost;
  };

private:
  Subprocess(pid_t childProcessID, int readPipe, int writePipe);

public:
  ~Subprocess();

public:
  // Child process functions
  static bool IsSubprocess();
  static bool GetParameters(String* pDestination);
  static Connection* ConnectToParent(int32 connectionTimeout = -1);

  // Parent process functions
  static Subprocess* Create(const char* executableFileName, const char* parameters = NULL, bool openChannel = true,
                            bool newConsole = true, bool terminateOnParentLost = true);
  Connection* ConnectToChild(int32 connectionTimeout = -1);
  bool IsChildAlive();
  void RequestChildToExit();
  void WaitForChildToExit();
  void TerminateChild();

private:
  pid_t m_childProcessID;
  int m_readPipe;
  int m_writePipe;
  Connection* m_pChildConnection;
};
