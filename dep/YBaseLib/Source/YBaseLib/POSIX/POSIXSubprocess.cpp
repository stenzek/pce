#include "YBaseLib/Common.h"

#ifdef Y_PLATFORM_POSIX
#include "YBaseLib/Log.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/String.h"
#include "YBaseLib/StringConverter.h"
#include "YBaseLib/Subprocess.h"
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
Log_SetChannel(Subprocess);

// Environment variable keys
static const char* COMM_ENV_VAR_PARENT_PROCESS_ID = "__SUBPROCESS_PPID";
static const char* COMM_ENV_VAR_PARAMETERS = "__SUBPROCESS_PARAMETERS";
static const char* COMM_ENV_VAR_EVENT_HANDLE = "__SUBPROCESS_EVENTHANDLE";
static const char* COMM_ENV_VAR_READ_PIPE_HANDLE = "__SUBPROCESS_RDPIPE_HANDLE";
static const char* COMM_ENV_VAR_WRITE_PIPE_HANDLE = "__SUBPROCESS_WRPIPE_HANDLE";
static const char* COMM_ENV_VAR_TERMINATE_ON_PARENT_LOST = "__SUBPROCESS_TERMINATE_ON_PARENT_LOST";

// Messages sent during initialization
static const uint32 PARENT_READY_MESSAGE_VALUE = 0xaabbcc00;
static const uint32 CHILD_READY_MESSAGE_VALUE = 0xbbccdd01;
static const int PARENT_PROCESS_DIED_EXIT_CODE = 0xdeadf00d;

// SIGPIPE blocker
class SIGPIPEBlocker
{
public:
  SIGPIPEBlocker() { m_old = signal(SIGPIPE, SIG_IGN); }

  ~SIGPIPEBlocker()
  {
    if (m_old != SIG_IGN)
      signal(SIGPIPE, m_old);
  }

private:
  sighandler_t m_old;
};

static bool ReadEnvironmentVariable(const char* key, String* dest)
{
  char* value = getenv(key);
  if (value == NULL)
    return false;

  dest->Assign(value);
  return true;
}

static bool ReadEnvironmentVariableHandle(const char* key, int* dest)
{
  SmallString value;
  if (!ReadEnvironmentVariable(key, &value))
    return false;

  *dest = StringConverter::StringToInt32(value);
  return true;
}

static void SetEnvironmentVariable(const char* key, const char* value)
{
  setenv(key, value, 1);
}

static void SetEnvironmentVariableHandle(const char* key, int handle)
{
  TinyString str;
  str.Format("%d", handle);
  setenv(key, str, 1);
}

Subprocess::Subprocess(pid_t childProcessID, int readPipe, int writePipe)
  : m_childProcessID(childProcessID), m_readPipe(readPipe), m_writePipe(writePipe), m_pChildConnection(nullptr)
{
}

Subprocess::~Subprocess()
{
  // Free the connection object (which will close the handles)
  if (m_pChildConnection != nullptr)
  {
    delete m_pChildConnection;
  }
  else
  {
    // if we're here, it means there wasn't a connection made, but still knock out the handles
    int status;
    waitpid(m_childProcessID, &status, 0);
    if (m_writePipe >= 0)
      close(m_writePipe);
    if (m_readPipe >= 0)
      close(m_readPipe);
  }
}

bool Subprocess::IsSubprocess()
{
  TinyString parentProcessIDStr;
  if (!ReadEnvironmentVariable(COMM_ENV_VAR_PARENT_PROCESS_ID, &parentProcessIDStr))
  {
    Log_DevPrintf("Subprocess::IsSubprocess: Parent process ID not found.");
    return false;
  }

  Log_DevPrintf("Subprocess::IsSubprocess: Parent process ID: %s", parentProcessIDStr.GetCharArray());
  return true;
}

bool Subprocess::GetParameters(String* pDestination)
{
  return ReadEnvironmentVariable(COMM_ENV_VAR_PARAMETERS, pDestination);
}

Subprocess* Subprocess::Create(const char* executableFileName, const char* parameters /* = NULL */,
                               bool openChannel /* = true */, bool newConsole /* = true */,
                               bool terminateOnParentLost /* = true */)
{
  // Creating with a communication channel?
  if (openChannel)
  {
    // Create event and pipes in parent process. These are labelled in regards to the parent's view, ie read -
    // child-parent, write - parent-child
    // TODO: error check here
    int readPipeFDs[2], writePipeFDs[2];
    pipe(readPipeFDs);
    pipe(writePipeFDs);

    // Get ends of pipe
    int readPipeReadEnd = readPipeFDs[0];
    int readPipeWriteEnd = readPipeFDs[1];
    int writePipeReadEnd = writePipeFDs[0];
    int writePipeWriteEnd = writePipeFDs[1];

    /*
    if (hQuitEvent == NULL || !hasReadPipe || !hasWritePipe)
    {
        Log_ErrorPrint("Subprocess::Create: Failed to create one or more IPC objects.");
        return nullptr;
    }
    */

    // Fork the process.
    pid_t childProcessID = fork();
    if (childProcessID < 0)
    {
      // todo: cleanup pipes
      Log_ErrorPrint("Subprocess::Create: Fork failed");
      return nullptr;
    }

    // Child end?
    if (childProcessID == 0)
    {
      // Close the unused ends of the pipe
      close(readPipeReadEnd);
      close(writePipeWriteEnd);

      // Set the environment variables
      SetEnvironmentVariableHandle(COMM_ENV_VAR_PARENT_PROCESS_ID, getppid());
      if (parameters != NULL)
        SetEnvironmentVariable(COMM_ENV_VAR_PARAMETERS, parameters);

      // Read pipe write end
      SetEnvironmentVariableHandle(COMM_ENV_VAR_READ_PIPE_HANDLE, writePipeReadEnd);

      // Read pipe write end
      SetEnvironmentVariableHandle(COMM_ENV_VAR_WRITE_PIPE_HANDLE, readPipeWriteEnd);

      // Terminate on parent lost
      SetEnvironmentVariable(COMM_ENV_VAR_TERMINATE_ON_PARENT_LOST, terminateOnParentLost ? "1" : "0");

      // Execute the child process, if successful, this won't return
      int res = execl(executableFileName, executableFileName, NULL);
      Log_ErrorPrintf("Subprocess::Create: exec(%s) failed: %d", executableFileName, res);
      _exit(-1);
    }
    else
    {
      // Close the unused ends of the pipe
      close(readPipeWriteEnd);
      close(writePipeReadEnd);

      // Construct subprocess object
      Log_InfoPrintf("SubProcess::Create: Spawned process %u.", childProcessID);
      return new Subprocess(childProcessID, readPipeReadEnd, writePipeWriteEnd);
    }
  }
  else
  {
    // Creating without communication channel, fork and exec
    int res = fork();
    if (res < 0)
    {
      Log_ErrorPrint("Subprocess::Create: vfork failed.");
      return nullptr;
    }

    if (res == 0)
    {
      // child
      res = execl(executableFileName, executableFileName, NULL);
      Log_ErrorPrintf("Subprocess::Create: exec failed: %d", res);
      _exit(-1);
    }

    // parent
    Log_InfoPrintf("SubProcess::Create: Spawned process %u.", res);
    return new Subprocess(res, -1, -1);
  }
}

Subprocess::Connection* Subprocess::ConnectToChild(int32 connectionTimeout /*= -1*/)
{
  // Can't connect if a communication channel wasn't created
  if (m_readPipe < 0)
  {
    Log_ErrorPrint("SubProcess::ConnectToChild: Process was not created with a communication channel.");
    return nullptr;
  }

  // Don't recreate connections
  if (m_pChildConnection != nullptr)
    return m_pChildConnection;

  // Send the ready message to the child
  Log_DevPrintf("Subprocess::ConnectToChild: Sending ready message...");
  if (write(m_writePipe, &PARENT_READY_MESSAGE_VALUE, sizeof(PARENT_READY_MESSAGE_VALUE)) !=
      sizeof(PARENT_READY_MESSAGE_VALUE))
  {
    Log_ErrorPrintf("Subprocess::ConnectToChild: Failed to write parent ready message to pipe. errno = %d", errno);
    return nullptr;
  }

  // Wait for the child to send its ready message
  Log_DevPrintf("Subprocess::ConnectToChild: Waiting for client response...");
  uint32 readyMessage;
  if (read(m_readPipe, &readyMessage, sizeof(readyMessage)) != sizeof(readyMessage) ||
      readyMessage != CHILD_READY_MESSAGE_VALUE)
  {
    Log_ErrorPrintf("Subprocess::ConnectToChild: Failed to read child ready message from pipe. errno = %d", errno);
    return nullptr;
  }

  // Create connection object
  Log_InfoPrintf("Subprocess::ConnectToChild: Connected with child process %u", m_childProcessID);
  m_pChildConnection = new Connection(m_childProcessID, m_readPipe, m_writePipe, false, false);
  return m_pChildConnection;
}

Subprocess::Connection* Subprocess::ConnectToParent(int32 connectionTimeout /*= -1*/)
{
  TinyString environmentVariableValue;
  if (!ReadEnvironmentVariable(COMM_ENV_VAR_PARENT_PROCESS_ID, &environmentVariableValue))
  {
    Log_DevPrintf("Subprocess::ConnectToParent: Not a subprocess.");
    return nullptr;
  }

  // Convert to a process ID
  pid_t parentProcessID = (pid_t)StringConverter::StringToInt32(environmentVariableValue);
  DebugAssert(parentProcessID == getppid());

  // Read the handle environment variables
  int readPipe, writePipe;
  if (!ReadEnvironmentVariableHandle(COMM_ENV_VAR_READ_PIPE_HANDLE, &readPipe) ||
      !ReadEnvironmentVariableHandle(COMM_ENV_VAR_WRITE_PIPE_HANDLE, &writePipe) ||
      !ReadEnvironmentVariable(COMM_ENV_VAR_TERMINATE_ON_PARENT_LOST, &environmentVariableValue))
  {
    Log_ErrorPrintf("Subprocess::ConnectToParent: Missing one or more environment variable handles.");
    return nullptr;
  }

  // Read terminate on lost string
  bool terminateOnParentLost = StringConverter::StringToBool(environmentVariableValue);

  // Wait for the parent ready message.
  uint32 readyMessage;
  Log_DevPrintf("Subprocess::ConnectToParent: Waiting for parent message...");
  if (read(readPipe, &readyMessage, sizeof(readyMessage)) != sizeof(readyMessage) ||
      readyMessage != PARENT_READY_MESSAGE_VALUE)
  {
    Log_ErrorPrint("Subprocess::ConnectToParent: Failed to read child ready message from pipe.");
    return nullptr;
  }

  // Send the client ready message.
  Log_DevPrintf("Subprocess::ConnectToParent: Sending ready message...");
  if (write(writePipe, &CHILD_READY_MESSAGE_VALUE, sizeof(CHILD_READY_MESSAGE_VALUE)) !=
      sizeof(CHILD_READY_MESSAGE_VALUE))
  {
    Log_ErrorPrint("Subprocess::ConnectToParent: Failed to write parent ready message to pipe.");
    return nullptr;
  }

  // Create connection object.
  return new Connection(parentProcessID, readPipe, writePipe, true, terminateOnParentLost);
}

void Subprocess::RequestChildToExit()
{
  // Flag the event
  // SetEvent(m_hQuitEvent);
  kill(m_childProcessID, SIGTERM);
}

void Subprocess::WaitForChildToExit()
{
  // Wait for the child to exit
  int status;
  waitpid(m_childProcessID, &status, 0);
}

void Subprocess::TerminateChild()
{
  kill(m_childProcessID, SIGKILL);
}

Subprocess::Connection::Connection(pid_t targetProcess, int readPipe, int writePipe, bool isParentConnection,
                                   bool terminateOnParentLost)
  : m_targetProcess(targetProcess), m_readPipe(readPipe), m_writePipe(writePipe),
    m_isParentConnection(isParentConnection), m_terminateOnParentLost(terminateOnParentLost)
{
}

Subprocess::Connection::~Connection()
{
  close(m_readPipe);
  close(m_writePipe);
}

bool Subprocess::Connection::IsOtherSideAlive()
{
  if (m_isParentConnection)
  {
    // we can't use wait here, since we're not talking to a child
    // send signal 0 to parent, if it's still alive this will return 0
    return (kill(m_targetProcess, 0) == 0);
  }
  else
  {
    // wait for child
    int status;
    return (waitpid(m_targetProcess, &status, WNOHANG) == 0);
  }
}

bool Subprocess::Connection::IsExitRequested()
{
  // Child process can't request termination of parent process
  if (!m_isParentConnection)
    return false;
  else if (!IsOtherSideAlive())
    return true;

  // TODO implement this properly
  // return (WaitForSingleObject(m_hQuitEvent, 0) == WAIT_OBJECT_0);
  return false;
}

bool Subprocess::Connection::WaitForData(int32 timeout /*= -1*/)
{
  pollfd fd;
  fd.fd = m_readPipe;
  fd.events = POLLIN;
  fd.revents = 0;
  return (poll(&fd, 1, timeout) > 0);
}

uint32 Subprocess::Connection::ReadDataNonBlocking(void* pBuffer, uint32 byteCount)
{
  pollfd fd;
  fd.fd = m_readPipe;
  fd.events = POLLIN;
  fd.revents = 0;
  int res = poll(&fd, 1, 0);
  if (res == 0)
    return 0;
  if (res < 0)
  {
    // broken pipe?
    Log_ErrorPrintf("Subprocess::Connection::ReadDataNonBlocking: poll returned %d, errno = %d (%s)", res, errno,
                    strerror(errno));
    if (m_terminateOnParentLost)
      abort();
    else
      return 0;
  }

  SIGPIPEBlocker blocker;
  res = recv(m_readPipe, pBuffer, byteCount, MSG_NOSIGNAL);
  if (res < 0)
  {
    Log_ErrorPrintf("Subprocess::Connection::ReadDataNonBlocking: read returned %d, errno = %d (%s)", res, errno,
                    strerror(errno));
    if (m_terminateOnParentLost)
      abort();
    else
      return 0;
  }

  return (uint32)res;
}

uint32 Subprocess::Connection::ReadDataBlocking(void* pBuffer, uint32 byteCount)
{
  SIGPIPEBlocker blocker;
  int res = read(m_readPipe, pBuffer, byteCount);
  if (res < 0)
  {
    Log_ErrorPrintf("Subprocess::Connection::ReadData: read returned %d, errno = %d (%s)", res, errno, strerror(errno));
    if (m_terminateOnParentLost)
      abort();
    else
      return 0;
  }

  return (uint32)res;
}

uint32 Subprocess::Connection::WriteData(const void* pBuffer, uint32 byteCount)
{
  SIGPIPEBlocker blocker;
  int res = write(m_writePipe, pBuffer, byteCount);
  if (res < 0)
  {
    Log_ErrorPrintf("Subprocess::Connection::WriteData: write returned %d, errno = %d (%s)", res, errno,
                    strerror(errno));
    if (m_terminateOnParentLost)
      abort();
    else
      return 0;
  }

  return (uint32)res;
}

#endif // Y_PLATFORM_POSIX
