#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportSessionPolicy.hpp>
#include <Poseidon/Network/Legacy/NetApi.hpp>

namespace Poseidon
{

struct NetTransportCreatePlayerAllocationResult
{
    int player = 0;
    ConnectResult result = CROK;
};

template <class UserMapT>
NetTransportCreatePlayerAllocationResult FindNetTransportCreatePlayerSlot(UserMapT& users, int maxPlayers, int uniqueID,
                                                                          int uniqueToTry)
{
    RefD<NetChannel> channel;
    int player = uniqueID;
    const int playerEnd = player + uniqueToTry;
    while (player != playerEnd && users.get(player, channel))
    {
        ++player;
    }

    if (player == playerEnd || IsNetTransportSessionFull(maxPlayers, users.card()))
    {
        return {
            .player = player,
            .result = CRSessionFull,
        };
    }

    return {
        .player = player,
        .result = CROK,
    };
}

} // namespace Poseidon
