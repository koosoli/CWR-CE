#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>

namespace Poseidon
{

template <class PoolT, class PeerT>
NetChannel* CreateNetTransportClientChannel(PoolT* pool, const struct sockaddr_in& distant, PeerT* peer,
                                            NetCallBack* processRoutine)
{
    if (!pool)
    {
        return nullptr;
    }

    sockaddr_in mutableDistant = distant;
    NetChannel* channel = pool->createChannel(mutableDistant, peer);
    if (!channel)
    {
        return nullptr;
    }

    channel->setProcessRoutine(processRoutine, channel);
    return channel;
}

} // namespace Poseidon
