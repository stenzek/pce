#include "YBaseLib/Common.h"

#ifdef Y_PLATFORM_WINDOWS
#include "YBaseLib/Log.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/String.h"
#include "YBaseLib/StringConverter.h"
#include "YBaseLib/Subprocess.h"
Log_SetChannel(Subprocess);

// Environment variable keys
static const char* COMM_ENV_VAR_PARENT_PROCESS_ID = "__SUBPROCESS_PPID";
static const char* COMM_ENV_VAR_PARAMETERS = "__SUBPROCESS_PARAMETERS";
static const char* COMM_ENV_VAR_EVENT_HANDLE = "__SUBPROCESS_EVENTHANDLE";
static const char* COMM_ENV_VAR_READ_PIPE_HANDLE = "__SUBPROCESS_RDPIPE_HANDLE";
static const char* COMM_ENV_VAR_WRITE_PIPE_HANDLE = "__SUBPROCESS_WRPIPE_HANDLE";
static const char* COMM_ENV_VAR_TERMINATE_ON_PARENT_LOST = "__SUBPROCESS_TERMINATE_ON_PARENT_LOST";

// Messages sent during initialization
static const DWORD PARENT_READY_MESSAGE_VALUE = 0xaabbcc00;
static const DWORD CHILD_READY_MESSAGE_VALUE = 0xbbccdd01;
static const DWORD PARENT_PROCESS_DIED_EXIT_CODE = 0xdeadf00d;

static bool ReadEnvironmentVariable(const char* key, String* dest)
{
  DWORD variableSize = GetEnvironmentVariableA(key, NULL, 0);
  if (variableSize == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND)
    return false;

  dest->Resize(variableSize);
  GetEnvironmentVariableA(key, dest->GetWriteableCharArray(), variableSize);
  return true;
}

static bool ReadEnvironmentVariableHandle(const char* key, HANDLE* dest)
{
  SmallString value;
  if (!ReadEnvironmentVariable(key, &value))
    return false;

  uint64 as64 = StringConverter::StringToUInt64(value);
  *dest = reinterpret_cast<void*>(as64);
  return true;
}

static void CreateEnvironmentVariableStringHandle(const char* key, HANDLE handle, String* dest)
{
  SmallString handleString;
  StringConverter::UInt64ToString(handleString, reinterpret_cast<uint64>(handle));
  dest->Format("%s=%s", key, handleString.GetCharArray());
}

Subprocess::Subprocess(HANDLE hChildProcess, DWORD childProcessID, HANDLE hQuitEvent, HANDLE hReadPipe,
                       HANDLE hWritePipe)
  : m_hChildProcess(hChildProcess), m_childProcessID(childProcessID), m_hQuitEvent(hQuitEvent), m_hReadPipe(hReadPipe),
    m_hWritePipe(hWritePipe), m_pChildConnection(nullptr)
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
    CloseHandle(m_hChildProcess);
    if (m_hQuitEvent != NULL)
      CloseHandle(m_hQuitEvent);
    if (m_hReadPipe != NULL)
      CloseHandle(m_hReadPipe);
    if (m_hWritePipe != NULL)
      CloseHandle(m_hWritePipe);
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
  // Create flags
  DWORD creationFlags = 0;
  if (newConsole)
    creationFlags |= CREATE_NEW_CONSOLE;

  // Destination process startup info
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);

  // Destination process information
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  // Creating with a communication channel?
  if (openChannel)
  {
    // Create event and pipes in parent process. These are labelled in regards to the parent's view, ie read -
    // child-parent, write - parent-child
    HANDLE hQuitEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    HANDLE hReadPipeReadEnd = NULL, hReadPipeWriteEnd = NULL;
    HANDLE hWritePipeReadEnd = NULL, hWritePipeWriteEnd = NULL;
    BOOL hasReadPipe = CreatePipe(&hReadPipeReadEnd, &hReadPipeWriteEnd, NULL, 0);
    BOOL hasWritePipe = CreatePipe(&hWritePipeReadEnd, &hWritePipeWriteEnd, NULL, 0);
    if (hQuitEvent == NULL || !hasReadPipe || !hasWritePipe)
    {
      Log_ErrorPrint("Subprocess::Create: Failed to create one or more IPC objects.");
      CloseHandle(hWritePipeWriteEnd);
      CloseHandle(hWritePipeReadEnd);
      CloseHandle(hReadPipeWriteEnd);
      CloseHandle(hReadPipeReadEnd);
      CloseHandle(hQuitEvent);
      return nullptr;
    }

    // Inherit only the handles that are necessary.
    SetHandleInformation(hQuitEvent, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(hReadPipeWriteEnd, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(hWritePipeReadEnd, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    // Clone the environment variables for this process, and add the communication values
    char* targetEnvironmentString = nullptr;
    uint32 targetEnvironmentSize = 0;
    {
      // Find the double-null terminator
      char* sourceStrings = GetEnvironmentStringsA();
      uint32 sourceStringsSize = 0;
      for (;; sourceStringsSize++)
      {
        if (sourceStrings[sourceStringsSize] == '\0' && sourceStrings[sourceStringsSize + 1] == '\0')
        {
          // Found the end
          sourceStringsSize++;
          break;
        }
      }

      // Copy into something we can work with.
      PODArray<char> targetEnvironment;
      targetEnvironment.AddRange(sourceStrings, sourceStringsSize);
      FreeEnvironmentStringsA(sourceStrings);

      // Create our new environment variables.
      SmallString newVar;

      // Parent process ID
      newVar.Format("%s=%u", COMM_ENV_VAR_PARENT_PROCESS_ID, GetCurrentProcessId());
      targetEnvironment.AddRange(newVar.GetCharArray(), newVar.GetLength() + 1);

      // Parameters
      if (parameters != NULL)
      {
        newVar.Format("%s=%s", COMM_ENV_VAR_PARAMETERS, parameters);
        targetEnvironment.AddRange(newVar.GetCharArray(), newVar.GetLength() + 1);
      }

      // Event handle
      CreateEnvironmentVariableStringHandle(COMM_ENV_VAR_EVENT_HANDLE, hQuitEvent, &newVar);
      targetEnvironment.AddRange(newVar.GetCharArray(), newVar.GetLength() + 1);

      // Read pipe write end
      CreateEnvironmentVariableStringHandle(COMM_ENV_VAR_READ_PIPE_HANDLE, hWritePipeReadEnd, &newVar);
      targetEnvironment.AddRange(newVar.GetCharArray(), newVar.GetLength() + 1);

      // Read pipe write end
      CreateEnvironmentVariableStringHandle(COMM_ENV_VAR_WRITE_PIPE_HANDLE, hReadPipeWriteEnd, &newVar);
      targetEnvironment.AddRange(newVar.GetCharArray(), newVar.GetLength() + 1);

      // Terminate on parent lost
      newVar.Format("%s=%s", COMM_ENV_VAR_TERMINATE_ON_PARENT_LOST, terminateOnParentLost ? "1" : "0");
      targetEnvironment.AddRange(newVar.GetCharArray(), newVar.GetLength() + 1);

      // End of environment block
      targetEnvironment.Add('\0');
      targetEnvironment.DetachArray(&targetEnvironmentString, &targetEnvironmentSize);
    }

    // Spawn the child process
    if (!CreateProcessA(executableFileName, NULL, NULL, NULL, TRUE, creationFlags, targetEnvironmentString, NULL, &si,
                        &pi))
    {
      Log_ErrorPrint("Subprocess::Create: CreateProcessA failed.");
      std::free(targetEnvironmentString);
      CloseHandle(hWritePipeWriteEnd);
      CloseHandle(hWritePipeReadEnd);
      CloseHandle(hReadPipeWriteEnd);
      CloseHandle(hReadPipeReadEnd);
      CloseHandle(hQuitEvent);
      return nullptr;
    }

    // Remove the inherit flags on the pipes, and close the ends we don't need
    std::free(targetEnvironmentString);
    SetHandleInformation(hQuitEvent, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hReadPipeWriteEnd, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hWritePipeReadEnd, HANDLE_FLAG_INHERIT, 0);
    CloseHandle(hReadPipeWriteEnd);
    CloseHandle(hWritePipeReadEnd);

    // Construct subprocess object
    Log_InfoPrintf("SubProcess::Create: Spawned process %u.", pi.dwProcessId);
    return new Subprocess(pi.hProcess, pi.dwProcessId, hQuitEvent, hReadPipeReadEnd, hWritePipeWriteEnd);
  }
  else
  {
    // Creating without communication channel
    if (!CreateProcessA(executableFileName, NULL, NULL, NULL, TRUE, creationFlags, NULL, NULL, &si, &pi))
    {
      Log_ErrorPrint("Subprocess::Create: CreateProcessA failed.");
      return nullptr;
    }

    // Construct subprocess object
    Log_InfoPrintf("SubProcess::Create: Spawned process %u.", pi.dwProcessId);
    return new Subprocess(pi.hProcess, pi.dwProcessId, NULL, NULL, NULL);
  }
}

Subprocess::Connection* Subprocess::ConnectToChild(int32 connectionTimeout /*= -1*/)
{
  DWORD transferred;
  DWORD readyMessage;

  // Can't connect if a communication channel wasn't created
  if (m_hReadPipe == NULL)
  {
    Log_ErrorPrint("SubProcess::ConnectToChild: Process was not created with a communication channel.");
    return nullptr;
  }

  // Don't recreate connections
  if (m_pChildConnection != nullptr)
    return m_pChildConnection;

  // Send the ready message to the child
  Log_DevPrintf("Subprocess::ConnectToChild: Sending ready message...");
  if (!WriteFile(m_hWritePipe, &PARENT_READY_MESSAGE_VALUE, sizeof(PARENT_READY_MESSAGE_VALUE), &transferred, NULL) ||
      transferred != sizeof(PARENT_READY_MESSAGE_VALUE))
  {
    Log_ErrorPrint("Subprocess::ConnectToChild: Failed to write parent ready message to pipe.");
    return nullptr;
  }

  // Wait for the child to send its ready message
  Log_DevPrintf("Subprocess::ConnectToChild: Waiting for client response...");
  if (!ReadFile(m_hReadPipe, &readyMessage, sizeof(readyMessage), &transferred, NULL) ||
      transferred != sizeof(readyMessage) || readyMessage != CHILD_READY_MESSAGE_VALUE)
  {
    Log_ErrorPrint("Subprocess::ConnectToChild: Failed to read child ready message from pipe.");
    return nullptr;
  }

  // Create connection object
  Log_InfoPrintf("Subprocess::ConnectToChild: Connected with child process %u", m_childProcessID);
  m_pChildConnection = new Connection(m_hChildProcess, m_hQuitEvent, m_hReadPipe, m_hWritePipe, false, false);
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
  DWORD parentProcessID = StringConverter::StringToUInt32(environmentVariableValue);

  // Open the parent process, so we can wait for it
  HANDLE hParentProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, parentProcessID);
  if (hParentProcess == NULL)
  {
    Log_DevPrintf("Subprocess::ConnectToParent: Failed to open parent process.");
    return nullptr;
  }

  // Read the handle environment variables
  HANDLE hQuitEvent, hReadPipe, hWritePipe;
  if (!ReadEnvironmentVariableHandle(COMM_ENV_VAR_EVENT_HANDLE, &hQuitEvent) ||
      !ReadEnvironmentVariableHandle(COMM_ENV_VAR_READ_PIPE_HANDLE, &hReadPipe) ||
      !ReadEnvironmentVariableHandle(COMM_ENV_VAR_WRITE_PIPE_HANDLE, &hWritePipe) ||
      !ReadEnvironmentVariable(COMM_ENV_VAR_TERMINATE_ON_PARENT_LOST, &environmentVariableValue))
  {
    Log_ErrorPrintf("Subprocess::ConnectToParent: Missing one or more environment variable handles.");
    return nullptr;
  }

  // Read terminate on lost string
  bool terminateOnParentLost = StringConverter::StringToBool(environmentVariableValue);

  // Wait for the parent ready message.
  DWORD readyMessage, transferred;
  Log_DevPrintf("Subprocess::ConnectToParent: Waiting for parent message...");
  if (!ReadFile(hReadPipe, &readyMessage, sizeof(readyMessage), &transferred, NULL) ||
      transferred != sizeof(readyMessage) || readyMessage != PARENT_READY_MESSAGE_VALUE)
  {
    Log_ErrorPrint("Subprocess::ConnectToParent: Failed to read child ready message from pipe.");
    return nullptr;
  }

  // Send the client ready message.
  Log_DevPrintf("Subprocess::ConnectToParent: Sending ready message...");
  if (!WriteFile(hWritePipe, &CHILD_READY_MESSAGE_VALUE, sizeof(CHILD_READY_MESSAGE_VALUE), &transferred, NULL) ||
      transferred != sizeof(CHILD_READY_MESSAGE_VALUE))
  {
    Log_ErrorPrint("Subprocess::ConnectToParent: Failed to write parent ready message to pipe.");
    return nullptr;
  }

  // Create connection object.
  return new Connection(hParentProcess, hQuitEvent, hReadPipe, hWritePipe, true, terminateOnParentLost);
}

void Subprocess::RequestChildToExit()
{
  // Flag the event
  SetEvent(m_hQuitEvent);
}

void Subprocess::WaitForChildToExit()
{
  // Wait for the child to exit
  DWORD exitCode;
  while (GetExitCodeProcess(m_hChildProcess, &exitCode) && exitCode == STILL_ACTIVE)
    ;
}

void Subprocess::TerminateChild()
{
  // Kill the child process
  TerminateProcess(m_hChildProcess, 0xDEADBEEF);

  // wait for it to exit
  WaitForChildToExit();
}

Subprocess::Connection::Connection(HANDLE hTargetProcess, HANDLE hQuitEvent, HANDLE hReadPipe, HANDLE hWritePipe,
                                   bool isParentConnection, bool terminateOnParentLost)
  : m_hTargetProcess(hTargetProcess), m_hQuitEvent(hQuitEvent), m_hReadPipe(hReadPipe), m_hWritePipe(hWritePipe),
    m_isParentConnection(isParentConnection), m_terminateOnParentLost(terminateOnParentLost)
{
}

Subprocess::Connection::~Connection()
{
  CloseHandle(m_hWritePipe);
  CloseHandle(m_hReadPipe);
  CloseHandle(m_hQuitEvent);
  CloseHandle(m_hTargetProcess);
}

bool Subprocess::Connection::IsOtherSideAlive()
{
  DWORD exitCode;
  if (GetExitCodeProcess(m_hTargetProcess, &exitCode) && exitCode == STILL_ACTIVE)
    return true;

  if (m_isParentConnection && m_terminateOnParentLost)
    TerminateProcess(GetCurrentProcess(), PARENT_PROCESS_DIED_EXIT_CODE);

  return false;
}

bool Subprocess::Connection::IsExitRequested()
{
  // Child process can't request termination of parent process
  if (!m_isParentConnection)
    return false;
  else if (!IsOtherSideAlive())
    return true;

  return (WaitForSingleObject(m_hQuitEvent, 0) == WAIT_OBJECT_0);
}

bool Subprocess::Connection::WaitForData(int32 timeout /*= -1*/)
{
  DWORD start = GetTickCount();

  for (;;)
  {
    DWORD available;
    if (!PeekNamedPipe(m_hReadPipe, NULL, 0, NULL, &available, NULL))
    {
      if (GetLastError() == ERROR_BROKEN_PIPE && m_isParentConnection && m_terminateOnParentLost)
        TerminateProcess(GetCurrentProcess(), PARENT_PROCESS_DIED_EXIT_CODE);
    }

    if (available > 0)
      return true;

    if (timeout < 0)
      continue;

    if ((GetTickCount() - start) > (DWORD)timeout)
      return false;
  }
}

uint32 Subprocess::Connection::ReadDataNonBlocking(void* pBuffer, uint32 byteCount)
{
  DWORD available;
  if (!PeekNamedPipe(m_hReadPipe, NULL, 0, NULL, &available, NULL))
  {
    if (GetLastError() == ERROR_BROKEN_PIPE && m_isParentConnection && m_terminateOnParentLost)
      TerminateProcess(GetCurrentProcess(), PARENT_PROCESS_DIED_EXIT_CODE);

    return 0;
  }

  if (available == 0)
    return 0;

  DWORD transferred;
  if (!ReadFile(m_hReadPipe, pBuffer, byteCount, &transferred, NULL))
  {
    if (GetLastError() == ERROR_BROKEN_PIPE && m_isParentConnection && m_terminateOnParentLost)
      TerminateProcess(GetCurrentProcess(), PARENT_PROCESS_DIED_EXIT_CODE);

    return 0;
  }

  return (uint32)transferred;
}

uint32 Subprocess::Connection::ReadDataBlocking(void* pBuffer, uint32 byteCount)
{
  DWORD transferred;
  if (!ReadFile(m_hReadPipe, pBuffer, byteCount, &transferred, NULL))
  {
    if (GetLastError() == ERROR_BROKEN_PIPE && m_isParentConnection && m_terminateOnParentLost)
      TerminateProcess(GetCurrentProcess(), PARENT_PROCESS_DIED_EXIT_CODE);

    return 0;
  }

  return (uint32)transferred;
}

uint32 Subprocess::Connection::WriteData(const void* pBuffer, uint32 byteCount)
{
  DWORD transferred;
  if (!WriteFile(m_hWritePipe, pBuffer, byteCount, &transferred, NULL))
  {
    if (GetLastError() == ERROR_BROKEN_PIPE && m_isParentConnection && m_terminateOnParentLost)
      TerminateProcess(GetCurrentProcess(), PARENT_PROCESS_DIED_EXIT_CODE);

    return 0;
  }

  return (uint32)transferred;
}

#endif // Y_PLATFORM_WINDOWS
