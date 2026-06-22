#pragma once
// Shared socket helpers used by harness, VoN, and other transport layers.

#ifdef _WIN32
#include <winsock2.h>
using socket_t = SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
using socket_t = int;
#endif

// Returns the port the OS assigned after bind(), 0 on error.
inline int sockBoundPort(socket_t s)
{
    sockaddr_in addr{};
#ifdef _WIN32
    int len = sizeof(addr);
#else
    socklen_t len = sizeof(addr);
#endif
    return getsockname(s, reinterpret_cast<sockaddr*>(&addr), &len) == 0 ? ntohs(addr.sin_port) : 0;
}

// Sliding window for I/O peer/channel statistics. Number of items = SLIDING_WINDOW / 2.
const int SLIDING_WINDOW = 64;

// Different sliding-window size for sent messages.
const int SLIDING_WINDOW_SEND = 256;
