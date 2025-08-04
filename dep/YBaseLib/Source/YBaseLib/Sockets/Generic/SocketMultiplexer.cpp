#include "YBaseLib/Sockets/SocketMultiplexer.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/NumericLimits.h"
#include "YBaseLib/Platform.h"
#include "YBaseLib/Sockets/ListenSocket.h"
#include "YBaseLib/Sockets/StreamSocket.h"
#include "YBaseLib/Thread.h"
Log_SetChannel(StreamSocket);

#ifdef Y_SOCKET_IMPLEMENTATION_GENERIC

#ifndef Y_PLATFORM_WINDOWS
#define ioctlsocket ioctl
#define closesocket close
#define WSAEWOULDBLOCK EAGAIN
#define WSAGetLastError() errno
#endif

SocketMultiplexer::SocketMultiplexer() : m_pWorkerThread(nullptr) {}

SocketMultiplexer::~SocketMultiplexer()
{
  StopWorkerThreads();
  CloseAll();
}

SocketMultiplexer* SocketMultiplexer::Create(Error* pError)
{
  if (!Platform::InitializeSocketSupport(pError))
    return nullptr;

  return new SocketMultiplexer();
}

ListenSocket* SocketMultiplexer::InternalCreateListenSocket(const SocketAddress* pAddress,
                                                            CreateStreamSocketCallback callback, Error* pError)
{
  // create and bind socket
  SOCKET fileDescriptor = INVALID_SOCKET;
  switch (pAddress->GetType())
  {
    case SocketAddress::Type_IPv4:
    {
      fileDescriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (fileDescriptor == INVALID_SOCKET)
      {
        if (pError != nullptr)
          pError->SetErrorSocket(WSAGetLastError());

        return nullptr;
      }

      if (bind(fileDescriptor, (const sockaddr*)pAddress->GetData(), sizeof(sockaddr_in)) < 0)
      {
        if (pError != nullptr)
          pError->SetErrorSocket(WSAGetLastError());

        closesocket(fileDescriptor);
        return nullptr;
      }

      if (listen(fileDescriptor, 5) < 0)
      {
        if (pError != nullptr)
          pError->SetErrorSocket(WSAGetLastError());

        closesocket(fileDescriptor);
        return nullptr;
      }

      break;
    }

    case SocketAddress::Type_IPv6:
    {
      fileDescriptor = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
      if (fileDescriptor == INVALID_SOCKET)
      {
        if (pError != nullptr)
          pError->SetErrorSocket(WSAGetLastError());

        return nullptr;
      }

      if (bind(fileDescriptor, (const sockaddr*)pAddress->GetData(), sizeof(sockaddr_in6)) < 0)
      {
        if (pError != nullptr)
          pError->SetErrorSocket(WSAGetLastError());

        closesocket(fileDescriptor);
        return nullptr;
      }

      if (listen(fileDescriptor, 5) < 0)
      {
        if (pError != nullptr)
          pError->SetErrorSocket(WSAGetLastError());

        closesocket(fileDescriptor);
        return nullptr;
      }

      break;
    }

    default:
    {
      if (pError != nullptr)
        pError->SetErrorUser((int32)0, "Unknown address type.");

      return nullptr;
    }
  }

  // create listensocket
  return new ListenSocket(this, callback, fileDescriptor);
}

StreamSocket* SocketMultiplexer::InternalConnectStreamSocket(const SocketAddress* pAddress,
                                                             CreateStreamSocketCallback callback, Error* pError)
{
  // create and bind socket
  SOCKET fileDescriptor = INVALID_SOCKET;
  switch (pAddress->GetType())
  {
    case SocketAddress::Type_IPv4:
    {
      fileDescriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (fileDescriptor == INVALID_SOCKET)
      {
        if (pError != nullptr)
          pError->SetErrorSocket(WSAGetLastError());

        return nullptr;
      }

      if (connect(fileDescriptor, (const sockaddr*)pAddress->GetData(), sizeof(sockaddr_in)) < 0)
      {
        if (pError != nullptr)
          pError->SetErrorSocket(WSAGetLastError());

        closesocket(fileDescriptor);
        return nullptr;
      }

      break;
    }

    case SocketAddress::Type_IPv6:
    {
      fileDescriptor = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
      if (fileDescriptor == INVALID_SOCKET)
      {
        if (pError != nullptr)
          pError->SetErrorSocket(WSAGetLastError());

        return nullptr;
      }

      if (connect(fileDescriptor, (const sockaddr*)pAddress->GetData(), sizeof(sockaddr_in6)) < 0)
      {
        if (pError != nullptr)
          pError->SetErrorSocket(WSAGetLastError());

        closesocket(fileDescriptor);
        return nullptr;
      }

      break;
    }

    default:
    {
      if (pError != nullptr)
        pError->SetErrorUser((int32)0, "Unknown address type.");

      return nullptr;
    }
  }

  // create stream socket
  StreamSocket* pSocket = callback();
  if (!pSocket->InitializeSocket(this, fileDescriptor, pError))
  {
    pSocket->Release();
    return nullptr;
  }

  return pSocket;
}

void SocketMultiplexer::AddOpenSocket(BaseSocket* pSocket)
{
  m_openSocketLock.Lock();

  DebugAssert(m_openSockets.IndexOf(pSocket) < 0);
  m_openSockets.Add(pSocket);
  pSocket->AddRef();

  m_openSocketLock.Unlock();
}

void SocketMultiplexer::RemoveOpenSocket(BaseSocket* pSocket)
{
  m_openSocketLock.Lock();

#ifdef Y_BUILD_CONFIG_DEBUG
  // double-locking, living dangerously!
  m_boundSocketLock.Lock();
  for (BoundSocket& boundSocket : m_boundSockets)
    DebugAssert(boundSocket.pSocket != pSocket);
  m_boundSocketLock.Unlock();
#endif

  DebugAssert(m_openSockets.IndexOf(pSocket) >= 0);
  m_openSockets.FastRemoveItem(pSocket);
  pSocket->Release();

  m_openSocketLock.Unlock();
}

void SocketMultiplexer::CloseAll()
{
  m_openSocketLock.Lock();

  if (m_openSockets.GetSize() > 0)
  {
    // pull everything into a list first
    BaseSocket** ppSockets = (BaseSocket**)alloca(sizeof(BaseSocket*) * m_openSockets.GetSize());
    uint32 nSockets = m_openSockets.GetSize();
    for (uint32 i = 0; i < nSockets; i++)
    {
      ppSockets[i] = m_openSockets[i];
      ppSockets[i]->AddRef();
    }

    // unlock the list
    m_openSocketLock.Unlock();

    // close all sockets
    for (uint32 i = 0; i < nSockets; i++)
    {
      ppSockets[i]->Close();
      ppSockets[i]->Release();
    }
  }
  else
  {
    // no open sockets
    m_openSocketLock.Unlock();
  }
}

void SocketMultiplexer::SetNotificationMask(BaseSocket* pSocket, SOCKET fileDescriptor, uint32 mask)
{
  m_boundSocketLock.Lock();

  for (uint32 i = 0; i < m_boundSockets.GetSize(); i++)
  {
    BoundSocket& boundSocket = m_boundSockets[i];
    if (boundSocket.FileDescriptor == fileDescriptor)
    {
      DebugAssert(boundSocket.pSocket == pSocket);

      // unbinding?
      if (mask != 0)
        boundSocket.EventMask = mask;
      else
        m_boundSockets.FastRemove(i);

      m_boundSocketLock.Unlock();
      return;
    }
  }

  // don't create entries for null masks
  if (mask != 0)
  {
    BoundSocket boundSocket;
    boundSocket.pSocket = pSocket;
    boundSocket.FileDescriptor = fileDescriptor;
    boundSocket.EventMask = mask;
    m_boundSockets.Add(boundSocket);
  }

  m_wakeupEvent.Signal();
  m_boundSocketLock.Unlock();
}

void SocketMultiplexer::PollEvents(uint32 milliseconds)
{
  fd_set readFds;
  fd_set writeFds;
  SOCKET maxFileDescriptor = 0;
  uint32 setCount = 0;

  // loop until we get some sockets
  for (;;)
  {
    // clear set
    FD_ZERO(&readFds);
    FD_ZERO(&writeFds);

    // fill stuff
    m_boundSocketLock.Lock();
    for (BoundSocket& boundSocket : m_boundSockets)
    {
      if (boundSocket.EventMask & EventType_Read)
        FD_SET(boundSocket.FileDescriptor, &readFds);
      if (boundSocket.EventMask & EventType_Write)
        FD_SET(boundSocket.FileDescriptor, &writeFds);

      maxFileDescriptor = Max(boundSocket.FileDescriptor, maxFileDescriptor);
      setCount++;
    }
    m_boundSocketLock.Unlock();
    if (setCount > 0)
      break;

    // win32 select doesn't seem to sleep as one would expect.
    if (milliseconds == 0)
      return;

    // in case a socket is added/removed in the meantime.
    if (!m_wakeupEvent.TryWait(milliseconds))
      return;

    // exit immediately after this next poll, don't wait.
    m_wakeupEvent.Reset();
    milliseconds = 0;
  }

  // call select
  timeval tv;
  tv.tv_sec = milliseconds / 1000;
  tv.tv_usec = (milliseconds % 1000) * 1000;
  int result =
    select((int)maxFileDescriptor + 1, &readFds, &writeFds, nullptr, (milliseconds != Y_UINT32_MAX) ? &tv : nullptr);
  if (result <= 0)
    return;

  // find sockets that triggered, we use an array here so we can avoid holding the lock, and if a socket disconnects
  BoundSocket* pTriggeredSockets = (BoundSocket*)alloca(sizeof(BoundSocket) * setCount);
  uint32 nTriggeredSockets = 0;
  m_boundSocketLock.Lock();
  for (BoundSocket& boundSocket : m_boundSockets)
  {
    uint32 eventMask = 0;
    if (FD_ISSET(boundSocket.FileDescriptor, &readFds))
      eventMask |= EventType_Read;
    if (FD_ISSET(boundSocket.FileDescriptor, &writeFds))
      eventMask |= EventType_Write;

    if (eventMask != 0)
    {
      pTriggeredSockets[nTriggeredSockets].pSocket = boundSocket.pSocket;
      pTriggeredSockets[nTriggeredSockets].EventMask = eventMask;
      nTriggeredSockets++;
    }
  }
  m_boundSocketLock.Unlock();

  // fire events
  for (uint32 i = 0; i < nTriggeredSockets; i++)
  {
    // we add a reference here in case the read kills it with a write pending, or something like that
    BaseSocket* pSocket = pTriggeredSockets[i].pSocket;
    uint32 eventMask = pTriggeredSockets[i].EventMask;
    pSocket->AddRef();

    // fire events
    if (eventMask & EventType_Read)
      pSocket->OnReadEvent();
    if (eventMask & EventType_Write)
      pSocket->OnWriteEvent();

    // release temporary reference
    pSocket->Release();
  }
}

SocketMultiplexer::WorkerThread::WorkerThread(SocketMultiplexer* pThis) : m_pThis(pThis), m_stopFlag(false) {}

int SocketMultiplexer::WorkerThread::ThreadEntryPoint()
{
  while (!m_stopFlag)
    m_pThis->PollEvents(1000);

  return 0;
}

uint32 SocketMultiplexer::CreateWorkerThreads(uint32 threadCount /*= 0*/)
{
  // Only a single worker thread permitted.
  threadCount = 1;

  // Create worker thread.
  m_pWorkerThread = new WorkerThread(this);
  if (!m_pWorkerThread->Start())
  {
    Log_ErrorPrint("Failed to start worker thread.");
    delete m_pWorkerThread;
    return 0;
  }

  // Return created amount.
  return threadCount;
}

void SocketMultiplexer::StopWorkerThreads()
{
  if (m_pWorkerThread != nullptr)
  {
    m_pWorkerThread->m_stopFlag = true;
    m_wakeupEvent.Signal();
    m_pWorkerThread->Join();
    delete m_pWorkerThread;
    m_pWorkerThread = nullptr;
  }
}

#endif // Y_SOCKET_IMPLEMENTATION_GENERIC
