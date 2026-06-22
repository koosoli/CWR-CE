#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>

namespace Poseidon
{

inline void DecodeNetTransportUrlAddress(RString address, RString& ip, int& port)
{
    const char* ptr = strrchr(address, ':');
    if (!ptr)
    {
        ip = address;
        return;
    }
    ip = address.Substring(0, ptr - address);
    port = atoi(ptr + 1);
}

template <class ResolveHostFn>
bool TryResolveNetTransportClientAddress(RString address, int& port, struct sockaddr_in& distant,
                                         ResolveHostFn&& resolveHost)
{
    if (address.GetLength() == 0)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "127.0.0.1:%d", port);
        address = buf;
    }

    RString ip;
    DecodeNetTransportUrlAddress(address, ip, port);
    return resolveHost(distant, (const char*)ip, port);
}

inline void ApplyNetTransportClientHandshakeSuccess(NetChannel* channel, bool acknowledged, int* port)
{
    if (!acknowledged || !channel)
    {
        return;
    }

    channel->checkConnectivity(0);
    if (port)
    {
        struct sockaddr_in distant
        {
        };
        channel->getDistantAddress(distant);
        *port = ntohs(distant.sin_port);
    }
}

} // namespace Poseidon
