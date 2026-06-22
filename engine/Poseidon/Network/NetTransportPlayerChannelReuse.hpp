#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportPlayerChannelLookup.hpp>
#include <utility>
using Poseidon::Foundation::IteratorState;

namespace Poseidon
{

struct NetTransportCreatePlayerChannelReuseResult
{
    bool duplicate = false;
    int previousPlayer = -1;
};

template <class UserMapT>
bool IsNetTransportDuplicateCreatePlayer(UserMapT& users, NetChannel* channel, int uniqueID, int uniqueToTry)
{
    if (!channel)
    {
        return false;
    }

    RefD<NetChannel> foundChannel;
    int player = uniqueID;
    const int playerEnd = player - uniqueToTry;
    do
    {
        if (users.get(player, foundChannel) && (NetChannel*)foundChannel == channel)
        {
            return true;
        }
    } while (--player != playerEnd);

    return false;
}

template <class UserMapT>
NetTransportCreatePlayerChannelReuseResult
AssessNetTransportCreatePlayerChannelReuse(UserMapT& users, NetChannel* channel, int uniqueID, int uniqueToTry)
{
    if (!channel)
    {
        return {};
    }

    if (IsNetTransportDuplicateCreatePlayer(users, channel, uniqueID, uniqueToTry))
    {
        return {
            .duplicate = true,
            .previousPlayer = -1,
        };
    }

    return {
        .duplicate = false,
        .previousPlayer = FindNetTransportPlayerByChannel(users, channel),
    };
}

} // namespace Poseidon
