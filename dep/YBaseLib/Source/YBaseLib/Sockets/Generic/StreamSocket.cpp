#include "YBaseLib/Sockets/StreamSocket.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Sockets/SocketMultiplexer.h"
Log_SetChannel(StreamSocket);

#ifdef Y_SOCKET_IMPLEMENTATION_GENERIC

// Windows-isms
#ifndef Y_PLATFORM_WINDOWS
#define ioctlsocket ioctl
#define closesocket close
#define WSAEWOULDBLOCK EAGAIN
#define WSAGetLastError() errno
#define SIZE_CAST(x) x
#else
#define SIZE_CAST(x) static_cast<int>(x)
#endif

StreamSocket::StreamSocket()
  : BaseSocket(), m_pMultiplexer(nullptr), m_fileDescriptor(INVALID_SOCKET), m_connected(false)
{
}

StreamSocket::~StreamSocket()
{
  DebugAssert(m_fileDescriptor < 0);
}

size_t StreamSocket::Read(void* pBuffer, size_t bufferSize)
{
  m_lock.Lock();
  if (!m_connected)
    return 0;

  // try a read
  ssize_t len = recv(m_fileDescriptor, (char*)pBuffer, SIZE_CAST(bufferSize), 0);
  if (len <= 0)
  {
    // Check for EAGAIN
    if (len < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
    {
      // Not an error. Just means no data is available.
      m_lock.Unlock();
      return 0;
    }

    // error
    CloseWithError();
    m_lock.Unlock();
    return 0;
  }

  m_lock.Unlock();
  return len;
}

size_t StreamSocket::Write(const void* pBuffer, size_t bufferLength)
{
  m_lock.Lock();

  if (!m_connected)
  {
    m_lock.Unlock();
    return 0;
  }

  // try a write
  ssize_t len = send(m_fileDescriptor, (const char*)pBuffer, SIZE_CAST(bufferLength), 0);
  if (len <= 0)
  {
    // Check for EAGAIN
    if (len < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
    {
      // Not an error. Just means no data is available.
      m_lock.Unlock();
      return 0;
    }

    // error
    CloseWithError();
    m_lock.Unlock();
    return 0;
  }

  m_lock.Unlock();
  return len;
}

size_t StreamSocket::WriteVector(const void** ppBuffers, const size_t* pBufferLengths, size_t numBuffers)
{
  m_lock.Lock();

  if (!m_connected || numBuffers == 0)
  {
    m_lock.Unlock();
    return 0;
  }

#ifdef Y_PLATFORM_WINDOWS

  WSABUF* bufs = (WSABUF*)alloca(sizeof(WSABUF) * numBuffers);
  for (size_t i = 0; i < numBuffers; i++)
  {
    bufs[i].buf = (CHAR*)ppBuffers[i];
    bufs[i].len = (ULONG)pBufferLengths[i];
  }

  DWORD bytesSent = 0;
  if (WSASend(m_fileDescriptor, bufs, (DWORD)numBuffers, &bytesSent, 0, nullptr, nullptr) == SOCKET_ERROR)
  {
    if (WSAGetLastError() != WSAEWOULDBLOCK)
    {
      // Socket error.
      CloseWithError();
      m_lock.Unlock();
      return 0;
    }
  }

  m_lock.Unlock();
  return (size_t)bytesSent;

#else // Y_PLATFORM_WINDOWS

  iovec* bufs = (iovec*)alloca(sizeof(iovec) * numBuffers);
  for (size_t i = 0; i < numBuffers; i++)
  {
    bufs[i].iov_base = (void*)ppBuffers[i];
    bufs[i].iov_len = pBufferLengths[i];
  }

  ssize_t res = writev(m_fileDescriptor, bufs, numBuffers);
  if (res < 0)
  {
    if (errno != EAGAIN)
    {
      // Socket error.
      CloseWithError();
      m_lock.Unlock();
      return 0;
    }

    res = 0;
  }

  m_lock.Unlock();
  return (size_t)res;

#endif // Y_PLATFORM_WINDOWS
}

void StreamSocket::Close()
{
  m_lock.Lock();

  if (!m_connected)
  {
    m_lock.Unlock();
    return;
  }

  m_pMultiplexer->SetNotificationMask(this, m_fileDescriptor, 0);
  closesocket(m_fileDescriptor);
  m_fileDescriptor = INVALID_SOCKET;
  m_connected = false;

  Error error;
  error.SetErrorUser((int32)0, "Connection explicitly closed.");
  OnDisconnected(&error);

  m_lock.Unlock();
}

void StreamSocket::CloseWithError()
{
  m_lock.Lock();

  DebugAssert(m_connected);

  Error error;
  int errorCode = WSAGetLastError();
  if (errorCode == 0)
    error.SetErrorUser((int32)0, "Connection closed by peer.");
  else
    error.SetErrorSocket(errorCode);

  m_pMultiplexer->SetNotificationMask(this, m_fileDescriptor, 0);
  closesocket(m_fileDescriptor);
  m_fileDescriptor = INVALID_SOCKET;
  m_connected = false;

  OnDisconnected(&error);

  m_lock.Unlock();

  // Remove the open socket last. This is because it may be the last reference holder.
  m_pMultiplexer->RemoveOpenSocket(this);
}

void StreamSocket::OnConnected()
{
  m_connected = true;
}

void StreamSocket::OnDisconnected(Error* pError)
{
  m_connected = false;
}

void StreamSocket::OnRead() {}

void StreamSocket::OnReadEvent()
{
  // forward through
  m_lock.Lock();

  if (m_connected)
    OnRead();

  m_lock.Unlock();
}

void StreamSocket::OnWriteEvent()
{
  // shouldn't be called
}

bool StreamSocket::InitializeSocket(SocketMultiplexer* pMultiplexer, SOCKET fileDescriptor, Error* pError)
{
  DebugAssert(m_pMultiplexer == nullptr);
  m_pMultiplexer = pMultiplexer;
  m_fileDescriptor = fileDescriptor;
  m_connected = true;

  // get local address
  sockaddr_storage sa;
  socklen_t salen = sizeof(sa);
  if (getsockname(m_fileDescriptor, (sockaddr*)&sa, &salen) == 0)
    m_localAddress.SetFromSockaddr(&sa, salen);
  else
    m_localAddress.SetUnknown();

  // get remote address
  salen = sizeof(sockaddr_storage);
  if (getpeername(m_fileDescriptor, (sockaddr*)&sa, &salen) == 0)
    m_remoteAddress.SetFromSockaddr(&sa, salen);
  else
    m_remoteAddress.SetUnknown();

  // switch to nonblocking mode
  unsigned long value = 1;
  if (ioctlsocket(m_fileDescriptor, FIONBIO, &value) < 0)
  {
    if (pError != nullptr)
      pError->SetErrorSocket(WSAGetLastError());

    return false;
  }

  // register for notifications
  m_pMultiplexer->AddOpenSocket(this);
  m_pMultiplexer->SetNotificationMask(this, m_fileDescriptor, SocketMultiplexer::EventType_Read);

  // trigger connected notitifcation
  m_lock.Lock();
  OnConnected();
  m_lock.Unlock();
  return true;
}

#endif // Y_SOCKET_IMPLEMENTATION_GENERIC
