#pragma once
#include "YBaseLib/CircularBuffer.h"
#include "YBaseLib/Sockets/StreamSocket.h"

#ifdef Y_SOCKET_IMPLEMENTATION_GENERIC

class BufferedStreamSocket : public StreamSocket
{
public:
  BufferedStreamSocket(size_t receiveBufferSize = 16384, size_t sendBufferSize = 16384);
  virtual ~BufferedStreamSocket();

  // Hide StreamSocket read/write methods.
  size_t Read(void* pBuffer, size_t bufferSize);
  size_t Write(const void* pBuffer, size_t bufferSize);
  size_t WriteVector(const void** ppBuffers, const size_t* pBufferLengths, size_t numBuffers);

  // Access to read buffer.
  bool AcquireReadBuffer(const void** ppBuffer, size_t* pBytesAvailable);
  void ReleaseReadBuffer(size_t bytesConsumed);

protected:
  virtual void OnConnected() override;
  virtual void OnDisconnected(Error* pError) override;
  virtual void OnRead() override;

private:
  virtual void OnReadEvent() override;
  virtual void OnWriteEvent() override;

private:
  CircularBuffer m_receiveBuffer;
  CircularBuffer m_sendBuffer;
};

#endif // #ifdef Y_SOCKET_IMPLEMENTATION_GENERIC