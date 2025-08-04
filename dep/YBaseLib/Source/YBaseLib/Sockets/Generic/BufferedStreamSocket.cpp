#include "YBaseLib/Sockets/BufferedStreamSocket.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Sockets/SocketMultiplexer.h"
Log_SetChannel(BufferedStreamSocket);

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

BufferedStreamSocket::BufferedStreamSocket(size_t receiveBufferSize /*= 16384*/, size_t sendBufferSize /*= 16384*/)
  : m_receiveBuffer(receiveBufferSize), m_sendBuffer(sendBufferSize)
{
}

BufferedStreamSocket::~BufferedStreamSocket() {}

size_t BufferedStreamSocket::Read(void* pBuffer, size_t bufferSize)
{
  m_lock.Lock();

  // Read from receive buffer.
  size_t bytesToRead = Min(m_receiveBuffer.GetContiguousUsedBytes(), bufferSize);
  if (bytesToRead > 0)
  {
    if (!m_receiveBuffer.Read(pBuffer, bytesToRead))
      bytesToRead = 0;
  }

  m_lock.Unlock();
  return bytesToRead;
}

size_t BufferedStreamSocket::Write(const void* pBuffer, size_t bufferSize)
{
  m_lock.Lock();

  // Write buffer currently being used?
  // Don't send out-of-order, push to the write buffer.
  size_t writtenBytes = 0;
  if (m_sendBuffer.GetBufferUsed() == 0)
  {
    // Send as many bytes as possible immediately. If this fails, push to the write buffer.
    ssize_t res = send(m_fileDescriptor, (const char*)pBuffer, SIZE_CAST(bufferSize), 0);
    writtenBytes = (size_t)Max(res, (ssize_t)0);

    // Socket error?
    if (res < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
    {
      CloseWithError();
      m_lock.Unlock();
      return 0;
    }
  }

  // Any bytes left over?
  if (writtenBytes < bufferSize)
  {
    // Copy remaining bytes into write buffer, and register for write notifications.
    size_t bytesRemaining = bufferSize - writtenBytes;
    size_t bufferSpace = m_sendBuffer.GetContiguousBufferSpace();
    bool registerForWrites = (m_sendBuffer.GetBufferUsed() == 0);

    // Write to buffer.
    size_t bytesToWriteToBuffer = Min(bufferSpace, bytesRemaining);
    if (m_sendBuffer.Write((const byte*)pBuffer + writtenBytes, bytesToWriteToBuffer))
      writtenBytes += bytesToWriteToBuffer;

    // Register for write notifications.
    if (registerForWrites)
      m_pMultiplexer->SetNotificationMask(this, m_fileDescriptor,
                                          SocketMultiplexer::EventType_Read | SocketMultiplexer::EventType_Write);
  }

  m_lock.Unlock();
  return writtenBytes;
}

size_t BufferedStreamSocket::WriteVector(const void** ppBuffers, const size_t* pBufferLengths, size_t numBuffers)
{
  m_lock.Lock();

  if (!m_connected || numBuffers == 0)
  {
    m_lock.Unlock();
    return 0;
  }

  // Write buffer currently being used?
  // Don't send out-of-order, push to the write buffer.
  size_t writtenBytes = 0;
  if (m_sendBuffer.GetBufferUsed() == 0)
  {
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

    writtenBytes = (size_t)bytesSent;

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

    writtenBytes = (size_t)res;

#endif // Y_PLATFORM_WINDOWS
  }

  // Find the buffer that we got up to
  size_t currentOffset = 0;
  bool registerForWrites = false;
  for (size_t i = 0; i < numBuffers; i++)
  {
    // Was this buffer only partially completed?
    if ((currentOffset + pBufferLengths[i]) <= writtenBytes)
    {
      currentOffset += pBufferLengths[i];
      continue;
    }

    // Copy remaining bytes into write buffer, and register for write notifications.
    size_t bytesWrittenInBuffer = (writtenBytes - currentOffset);
    size_t bytesRemainingInBuffer = pBufferLengths[i] - bytesWrittenInBuffer;
    size_t bufferSpace = m_sendBuffer.GetContiguousBufferSpace();
    if (m_sendBuffer.GetBufferUsed() == 0)
      registerForWrites = true;

    // Write to buffer.
    size_t bytesToWriteToBuffer = Min(bytesRemainingInBuffer, bufferSpace);
    if (m_sendBuffer.Write((const byte*)ppBuffers[i] + bytesWrittenInBuffer, bytesToWriteToBuffer))
      writtenBytes += bytesToWriteToBuffer;

    // Was this a complete write?
    if (bytesToWriteToBuffer == bytesRemainingInBuffer)
      currentOffset += pBufferLengths[i];
    else
      break;
  }

  // Register for write notifications.
  if (registerForWrites)
    m_pMultiplexer->SetNotificationMask(this, m_fileDescriptor,
                                        SocketMultiplexer::EventType_Read | SocketMultiplexer::EventType_Write);

  m_lock.Unlock();
  return writtenBytes;
}

bool BufferedStreamSocket::AcquireReadBuffer(const void** ppBuffer, size_t* pBytesAvailable)
{
  m_lock.Lock();
  if (!m_receiveBuffer.GetReadPointer(ppBuffer, pBytesAvailable))
  {
    m_lock.Unlock();
    return false;
  }

  return true;
}

void BufferedStreamSocket::ReleaseReadBuffer(size_t bytesConsumed)
{
  DebugAssert(bytesConsumed <= m_receiveBuffer.GetContiguousBufferSpace());
  if (bytesConsumed >= 0)
    m_receiveBuffer.MoveReadPointer(bytesConsumed);

  m_lock.Unlock();
}

void BufferedStreamSocket::OnConnected()
{
  StreamSocket::OnConnected();
}

void BufferedStreamSocket::OnDisconnected(Error* pError)
{
  StreamSocket::OnDisconnected(pError);
}

void BufferedStreamSocket::OnRead()
{
  StreamSocket::OnRead();
}

void BufferedStreamSocket::OnReadEvent()
{
  m_lock.Lock();

  if (m_connected)
  {
    // Pull as many bytes as possible into the read buffer.
    while (m_receiveBuffer.GetBufferSpace() > 0)
    {
      void* pBuffer;
      size_t contiguousBytes = 1;
      if (m_receiveBuffer.GetWritePointer(&pBuffer, &contiguousBytes))
      {
        ssize_t res = recv(m_fileDescriptor, (char*)pBuffer, SIZE_CAST(contiguousBytes), 0);
        if (res <= 0 && WSAGetLastError() != WSAEWOULDBLOCK)
        {
          CloseWithError();
          m_lock.Unlock();
          return;
        }

        if (res > 0)
        {
          m_receiveBuffer.MoveWritePointer((size_t)res);
          if ((size_t)res == contiguousBytes)
            continue;
        }

        break;
      }
    }

    OnRead();
  }

  m_lock.Unlock();
}

void BufferedStreamSocket::OnWriteEvent()
{
  m_lock.Lock();

  // Try to send as many bytes as possible from the write buffer.
  while (m_sendBuffer.GetBufferUsed() > 0)
  {
    size_t contiguousBytes;
    const void* pBuffer;
    if (m_sendBuffer.GetReadPointer(&pBuffer, &contiguousBytes))
    {
      ssize_t res = send(m_fileDescriptor, (const char*)pBuffer, SIZE_CAST(contiguousBytes), 0);
      if (res < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
      {
        CloseWithError();
        m_lock.Unlock();
        return;
      }

      // Any bytes written?
      if (res > 0)
      {
        m_sendBuffer.MoveWritePointer((size_t)res);

        // Try again only if we did a whole write.
        if ((size_t)res == contiguousBytes)
          continue;
      }
    }

    // Don't try again.
    break;
  }

  // Do we still have bytes? If not, unregister for read notifications.
  if (m_sendBuffer.GetBufferUsed() == 0)
    m_pMultiplexer->SetNotificationMask(this, m_fileDescriptor, SocketMultiplexer::EventType_Read);

  m_lock.Unlock();
}

#endif // #ifdef Y_SOCKET_IMPLEMENTATION_GENERIC
