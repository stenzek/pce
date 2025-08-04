#pragma once
#include "YBaseLib/Common.h"
#include "YBaseLib/String.h"

struct SocketAddress
{
  enum Type
  {
    Type_Unknown,
    Type_IPv4,
    Type_IPv6,
    Type_NamedPipe,
    Type_SharedMemory
  };

  // constructors/operators
  SocketAddress() {}
  SocketAddress(const SocketAddress& copy);
  SocketAddress& operator=(const SocketAddress& copy);

  // accessors
  Type GetType() const { return m_type; }
  const void* GetData() const { return m_data; }

  // parse interface
  static bool Parse(Type type, const char* address, uint32 port, SocketAddress* pOutAddress);

  // resolve interface
  static bool Resolve(const char* address, uint32 port, SocketAddress* pOutAddress);

  // to string interface
  void ToString(String& destination) const;
  SmallString ToString() const;

  // initializers
  void SetUnknown();
  void SetFromSockaddr(const void* pSockaddr, size_t sockaddrLength);

private:
  Type m_type;
  byte m_data[128];
};

// struct SocketAddressIPv4 : public SocketAddress
// {
//     virtual Type GetType() { return SocketAddress::Type_IPv4; }
//     virtual void ToString(String &destination) const override final;
//     virtual void
// };