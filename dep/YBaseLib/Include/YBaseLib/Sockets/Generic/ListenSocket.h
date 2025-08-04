#pragma once
#include "YBaseLib/ReferenceCounted.h"
#include "YBaseLib/Sockets/BaseSocket.h"
#include "YBaseLib/Sockets/Common.h"
#include "YBaseLib/Sockets/SocketAddress.h"
#include "YBaseLib/Sockets/SocketMultiplexer.h"

#ifdef Y_SOCKET_IMPLEMENTATION_GENERIC

class ListenSocket : public BaseSocket
{
  friend SocketMultiplexer;

public:
  ListenSocket(SocketMultiplexer* pMultiplexer, SocketMultiplexer::CreateStreamSocketCallback acceptCallback,
               SOCKET fileDescriptor);
  virtual ~ListenSocket();

  const SocketAddress* GetLocalAddress() const { return &m_localAddress; }
  uint32 GetConnectionsAccepted() const { return m_numConnectionsAccepted; }

  virtual void Close() override final;

private:
  virtual void OnReadEvent() override final;
  virtual void OnWriteEvent() override final;

private:
  SocketMultiplexer* m_pMultiplexer;
  SocketMultiplexer::CreateStreamSocketCallback m_acceptCallback;
  SocketAddress m_localAddress;
  uint32 m_numConnectionsAccepted;
  SOCKET m_fileDescriptor;
};

#endif // Y_SOCKET_IMPLEMENTATION_GENERIC
