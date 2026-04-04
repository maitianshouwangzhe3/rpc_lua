#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCK2API_
#include <winsock2.h>
#endif
#include <ws2tcpip.h>

typedef SOCKET net_socket_t;

inline int net_get_errno() { return WSAGetLastError(); }

#define NET_EWOULDBLOCK     WSAEWOULDBLOCK
#define NET_EINPROGRESS     WSAEINPROGRESS
#define NET_EINTR           WSAEINTR

#define NET_CLOSE_SOCKET(s) closesocket(s)
#define NET_INVALID_SOCKET  INVALID_SOCKET

#else

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <cerrno>

typedef int net_socket_t;

inline int net_get_errno() { return errno; }

#define NET_EWOULDBLOCK     EWOULDBLOCK
#define NET_EINPROGRESS     EINPROGRESS
#define NET_EINTR           EINTR

#define NET_CLOSE_SOCKET(s) ::close(s)
#define NET_INVALID_SOCKET  (-1)

#endif
