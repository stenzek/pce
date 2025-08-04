#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/String.h"
#include "YBaseLib/Windows/WindowsHeaders.h"

class Subprocess
{
public:
  class Connection
  {
  private:
    Connection(HANDLE hTargetProcess, HANDLE hQuitEvent, HANDLE hReadPipe, HANDLE hWritePipe, bool isParentConnection,
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
    HANDLE m_hTargetProcess;
    HANDLE m_hQuitEvent;
    HANDLE m_hReadPipe;
    HANDLE m_hWritePipe;
    bool m_isParentConnection;
    bool m_terminateOnParentLost;
  };

private:
  Subprocess(HANDLE hChildProcess, DWORD childProcessID, HANDLE hQuitEvent, HANDLE hReadPipe, HANDLE hWritePipe);

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
  HANDLE m_hChildProcess;
  DWORD m_childProcessID;
  HANDLE m_hQuitEvent;
  HANDLE m_hReadPipe;
  HANDLE m_hWritePipe;
  Connection* m_pChildConnection;
};
