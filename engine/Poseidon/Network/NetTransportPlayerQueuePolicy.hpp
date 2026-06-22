#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportPlayerQueue.hpp>

namespace Poseidon
{

template <class Allocator>
int FindNetTransportPendingCreatePlayer(const AutoArray<CreatePlayerInfo, Allocator>& createPlayers, int player)
{
    for (int index = 0; index < createPlayers.Size(); ++index)
    {
        if (createPlayers[index].player == player)
        {
            return index;
        }
    }
    return -1;
}

template <class Allocator>
bool CancelNetTransportPendingCreatePlayer(AutoArray<CreatePlayerInfo, Allocator>& createPlayers, int player)
{
    const int index = FindNetTransportPendingCreatePlayer(createPlayers, player);
    if (index < 0)
    {
        return false;
    }

    createPlayers.Delete(index);
    return true;
}

template <class Allocator>
DeletePlayerInfo& AppendNetTransportDeletePlayer(AutoArray<DeletePlayerInfo, Allocator>& deletePlayers, int player)
{
    const int index = deletePlayers.Add();
    deletePlayers[index] = BuildNetTransportDeletePlayerInfo(player);
    return deletePlayers[index];
}

} // namespace Poseidon
