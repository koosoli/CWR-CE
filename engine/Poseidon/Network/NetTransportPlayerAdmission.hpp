#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportPlayerQueue.hpp>
#include <Poseidon/Network/Legacy/NetApi.hpp>

namespace Poseidon
{

struct NetTransportAcceptedPlayerResult
{
    CreatePlayerInfo* info = nullptr;
    bool nameTruncated = false;
};

template <class UserMapT, class SupportMapT, class CreateAllocator, class SupportFactory>
NetTransportAcceptedPlayerResult
RegisterNetTransportAcceptedPlayer(UserMapT& users, SupportMapT& support,
                                   AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers, int& botId, int player,
                                   NetChannel* channel, const struct sockaddr_in& distant,
                                   const CreatePlayerPacket& request, SupportFactory&& makeSupport)
{
    users.put(player, channel);
    support.put(sockaddrKey(distant), makeSupport(player));

    CreatePlayerInfo& info = createPlayers[createPlayers.Add()];
    info = BuildNetTransportCreatePlayerInfo(player, request);
    if (info.botClient)
    {
        botId = player;
    }

    return {
        .info = &info,
        .nameTruncated = WasNetTransportCreatePlayerNameTruncated(request, info),
    };
}

} // namespace Poseidon
