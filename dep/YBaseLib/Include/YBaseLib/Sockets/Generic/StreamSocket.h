#pragma once
#include "YBaseLib/Error.h"
#include "YBaseLib/RecursiveMutex.h"
#include "YBaseLib/Sockets/BaseSocket.h"
#include "YBaseLib/Sockets/Common.h"
#include "YBaseLib/Sockets/SocketAddress.h"

#ifdef Y_SOCKET_IMPLEMENTATION_GENERIC

class ListenSocket;
class BufferedStreamSocket;

class StreamSocket : public BaseSocket
{
public:
  StreamSocket();
  virtual ~StreamSocket();

  virtual void Close() override final;

  // Accessors
  const SocketAddress* GetLocalAddress() const { return &m_localAddress; }
  const SocketAddress* GetRemoteAddress() const { return &m_remoteAddress; }
  bool IsConnected() const { return m_connected; }

  // Read/write
  size_t Read(void* pBuffer, size_t bufferSize);
  size_t Write(const void* pBuffer, size_t bufferSize);
  size_t WriteVector(const void** ppBuffers, const size_t* pBufferLengths, size_t numBuffers);

protected:
  virtual void OnConnected();
  virtual void OnDisconnected(Error* pError);
  virtual void OnRead();

private:
  virtual void OnReadEvent() override;
  virtual void OnWriteEvent() override;

  bool InitializeSocket(SocketMultiplexer* pMultiplexer, SOCKET fileDescriptor, Error* pError);
  void CloseWithError();

private:
  SocketMultiplexer* m_pMultiplexer;
  SocketAddress m_localAddress;
  SocketAddress m_remoteAddress;
  RecursiveMutex m_lock;
  SOCKET m_fileDescriptor;
  bool m_connected;

  // Ugly, but needed in order to call the events.
  friend SocketMultiplexer;
  friend ListenSocket;
  friend BufferedStreamSocket;
};

#endif // Y_SOCKET_IMPLEMENTATION_GENERIC
