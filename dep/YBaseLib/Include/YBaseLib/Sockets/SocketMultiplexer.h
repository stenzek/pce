#pragma once
#include "YBaseLib/Sockets/Common.h"

enum SOCKET_MULTIPLEXER_TYPE
{
  SOCKET_MULTIPLEXER_TYPE_GENERIC,
  // SOCKET_MULTIPLEXER_TYPE_EPOLL,
  // SOCKET_MULTIPLEXER_TYPE_KQUEUE,
  // SOCKET_MULTIPLEXER_TYPE_IOCP,
  NUM_SOCKET_MULTIPLEXER_TYPES
};

#if defined(Y_SOCKET_IMPLEMENTATION_GENERIC)
#include "YBaseLib/Sockets/Generic/SocketMultiplexer.h"
#else
#error Unknown socket implementation.
#endif
