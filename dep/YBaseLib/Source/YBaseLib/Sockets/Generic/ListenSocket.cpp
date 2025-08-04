#include "YBaseLib/Sockets/ListenSocket.h"
#include "YBaseLib/Error.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Sockets/SocketMultiplexer.h"
#include "YBaseLib/Sockets/StreamSocket.h"
Log_SetChannel(StreamSocket);

#ifdef Y_SOCKET_IMPLEMENTATION_GENERIC

// Windows-isms
#ifndef Y_PLATFORM_WINDOWS
#define ioctlsocket ioctl
#define closesocket close
#define WSAEWOULDBLOCK EAGAIN
#define WSAGetLastError() errno
#endif

ListenSocket::ListenSocket(SocketMultiplexer* pMultiplexer,
                           SocketMultiplexer::CreateStreamSocketCallback acceptCallback, SOCKET fileDescriptor)
  : m_pMultiplexer(pMultiplexer), m_acceptCallback(acceptCallback), m_numConnectionsAccepted(0),
    m_fileDescriptor(fileDescriptor)
{
  // add to list
  pMultiplexer->AddOpenSocket(this);

  // get local address
  sockaddr_storage sa;
  socklen_t salen = sizeof(sa);
  if (getsockname(m_fileDescriptor, (sockaddr*)&sa, &salen) == 0)
    m_localAddress.SetFromSockaddr(&sa, salen);
  else
    m_localAddress.SetUnknown();

  // register for reads
  pMultiplexer->SetNotificationMask(this, fileDescriptor, SocketMultiplexer::EventType_Read);
}

ListenSocket::~ListenSocket()
{
  DebugAssert(m_fileDescriptor < 0);
}

void ListenSocket::Close()
{
  if (m_fileDescriptor < 0)
    return;

  m_pMultiplexer->SetNotificationMask(this, m_fileDescriptor, 0);
  m_pMultiplexer->RemoveOpenSocket(this);
  closesocket(m_fileDescriptor);
  m_fileDescriptor = INVALID_SOCKET;
}

void ListenSocket::OnReadEvent()
{
  // connection incoming
  sockaddr_storage sa;
  socklen_t salen = sizeof(sa);
  SOCKET newFileDescriptor = accept(m_fileDescriptor, (sockaddr*)&sa, &salen);
  if (newFileDescriptor < 0)
    return;

  // create socket, we release our own reference.
  StreamSocket* pStreamSocket = m_acceptCallback();
  pStreamSocket->InitializeSocket(m_pMultiplexer, newFileDescriptor, nullptr);
  pStreamSocket->Release();
  m_numConnectionsAccepted++;
}

void ListenSocket::OnWriteEvent() {}

#endif // Y_SOCKET_IMPLEMENTATION_GENERIC
