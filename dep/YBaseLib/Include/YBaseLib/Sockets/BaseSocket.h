#include "YBaseLib/Sockets/Common.h"

#if defined(Y_SOCKET_IMPLEMENTATION_GENERIC)
#include "YBaseLib/Sockets/Generic/BaseSocket.h"
#else
#error Unknown socket implementation.
#endif
