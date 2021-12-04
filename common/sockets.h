// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
#ifndef COMMON_SOCKETS_H
#define COMMON_SOCKETS_H

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>

#define LastSocketError (WSAGetLastError())

static inline int socket_startup(void)
{
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data);
}

static inline int socket_cleanup(void)
{
    return WSACleanup();
}

#else /* Assume posix style sockets on non-windows */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h> // for getaddrinfo() and freeaddrinfo()
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // for close()
typedef int SOCKET;
#define INVALID_SOCKET       (-1)
#define SOCKET_ERROR         (-1)
#define closesocket(x)       close(x)
#define ioctlsocket(x, y, z) ioctl(x, y, z)
#define LastSocketError      (errno)

#define WSAEISCONN       EISCONN
#define WSAEINPROGRESS   EINPROGRESS
#define WSAEALREADY      EALREADY
#define WSAEADDRINUSE    EADDRINUSE
#define WSAEADDRNOTAVAIL EADDRNOTAVAIL
#define WSAEBADF         EBADF
#define WSAECONNREFUSED  ECONNREFUSED
#define WSAEINTR         EINTR
#define WSAENOTSOCK      ENOTSOCK
#define WSAEWOULDBLOCK   EWOULDBLOCK
#define WSAEINVAL        EINVAL
#define WSAETIMEDOUT     ETIMEDOUT

static inline int socket_startup(void)
{
    return 0;
}

static inline int socket_cleanup(void)
{
    return 0;
}

#endif /* _WIN32 */

#endif /* COMMON_SOCKETS_H */
