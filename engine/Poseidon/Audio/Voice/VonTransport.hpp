#pragma once
// VoN UDP transport — P2P voice socket + send/receive

#include <cstdint>


namespace Poseidon
{
#ifdef _WIN32
} // namespace Poseidon
#include <winsock2.h>
#include <ws2tcpip.h>
namespace Poseidon
{
#else
} // namespace Poseidon
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
namespace Poseidon
{
using SOCKET = int;
#ifndef INVALID_SOCKET
static constexpr SOCKET INVALID_SOCKET = -1;
#endif
#endif

class VoNTransport
{
  public:
    VoNTransport() = default;
    ~VoNTransport();

    VoNTransport(const VoNTransport&) = delete;
    VoNTransport& operator=(const VoNTransport&) = delete;

    // Bind a UDP socket for voice P2P. port=0 for OS-assigned.
    bool open(int port = 0);
    void close();

    // Send raw data to a peer address
    bool sendTo(const sockaddr_in& peer, const void* data, int size);

    // Non-blocking receive. Returns bytes received, 0 if nothing, -1 on error.
    int recvFrom(void* buffer, int maxSize, sockaddr_in& from);

    bool isOpen() const { return _socket != INVALID_SOCKET; }
    int localPort() const;

    // Utility
    static sockaddr_in makeAddr(const char* ip, int port);

  private:
    SOCKET _socket = INVALID_SOCKET;
};

} // namespace Poseidon
