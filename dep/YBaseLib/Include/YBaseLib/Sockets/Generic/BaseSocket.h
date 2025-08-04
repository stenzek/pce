#pragma once
#include "YBaseLib/ReferenceCounted.h"
#include "YBaseLib/Sockets/Common.h"

#ifdef Y_SOCKET_IMPLEMENTATION_GENERIC

class SocketMultiplexer;

class BaseSocket : public ReferenceCounted
{
public:
  BaseSocket() {}
  virtual ~BaseSocket() {}

  virtual void Close() = 0;

private:
  virtual void OnReadEvent() = 0;
  virtual void OnWriteEvent() = 0;

  // Ugly, but needed in order to call the events.
  friend SocketMultiplexer;
};

#endif // Y_SOCKET_IMPLEMENTATION_GENERIC
