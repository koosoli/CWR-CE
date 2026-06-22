#include <Poseidon/Audio/Voice/VonTransport.hpp>
#include <Poseidon/Network/Legacy/SocketUtil.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <cstring>
#ifndef _WIN32
#include <arpa/inet.h>
#endif
#include <stdint.h>
#ifndef _WIN32
#include <sys/socket.h>
#endif

namespace Poseidon
{
#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#else
} // namespace Poseidon
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
namespace Poseidon
{

#endif

VoNTransport::~VoNTransport()
{
    close();
}

bool VoNTransport::open(int port)
{
    close();

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_socket == INVALID_SOCKET)
    {
        LOG_ERROR(Network, "VoN transport: socket() failed");
        return false;
    }

    // Non-blocking
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(_socket, FIONBIO, &mode);
#else
    int flags = fcntl(_socket, F_GETFL, 0);
    fcntl(_socket, F_SETFL, flags | O_NONBLOCK);
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        LOG_ERROR(Network, "VoN transport: bind failed on port {}", port);
        close();
        return false;
    }
    LOG_DEBUG(Network, "VoN transport: opened on port {}", localPort());
    return true;
}

void VoNTransport::close()
{
    if (_socket == INVALID_SOCKET)
        return;
    LOG_DEBUG(Network, "VoN transport: closing");
#ifdef _WIN32
    closesocket(_socket);
#else
    ::close(_socket);
#endif
    _socket = INVALID_SOCKET;
}

bool VoNTransport::sendTo(const sockaddr_in& peer, const void* data, int size)
{
    if (_socket == INVALID_SOCKET)
        return false;
    int ret = ::sendto(_socket, static_cast<const char*>(data), size, 0, reinterpret_cast<const sockaddr*>(&peer),
                       sizeof(peer));
    LOG_TRACE(Network, "VoN transport: sent {}B (ret={})", size, ret);
    return ret == size;
}

int VoNTransport::recvFrom(void* buffer, int maxSize, sockaddr_in& from)
{
    if (_socket == INVALID_SOCKET)
        return -1;
    socklen_t fromLen = sizeof(from);
    int ret = ::recvfrom(_socket, static_cast<char*>(buffer), maxSize, 0, reinterpret_cast<sockaddr*>(&from), &fromLen);
    if (ret < 0)
    {
#ifdef _WIN32
        if (WSAGetLastError() == WSAEWOULDBLOCK)
            return 0;
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
#endif
        return -1;
    }
    return ret;
}

int VoNTransport::localPort() const
{
    return _socket == INVALID_SOCKET ? 0 : sockBoundPort(_socket);
}

sockaddr_in VoNTransport::makeAddr(const char* ip, int port)
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, ip, &addr.sin_addr);
    return addr;
}

} // namespace Poseidon
