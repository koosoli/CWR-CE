#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/Legacy/NetApi.hpp>
using Poseidon::Foundation::nsNoMoreCallbacks;
using Poseidon::Foundation::nsOK;
using Poseidon::Foundation::nsOutputSent;

namespace Poseidon
{

struct NetTransportPlayerReconnectResult
{
    ConnectResult status = CRError;
    NetChannel* channel = nullptr;
};

template <class UserMapT>
NetTransportPlayerReconnectResult TryReconnectNetTransportPlayer(UserMapT& users, int playerNo,
                                                                 struct sockaddr_in& distant)
{
    RefD<NetChannel> channel;
    if (!users.get(playerNo, channel) || channel->reconnect(distant) != nsOK)
    {
        return {};
    }

    return {
        .status = CROK,
        .channel = channel,
    };
}

} // namespace Poseidon
