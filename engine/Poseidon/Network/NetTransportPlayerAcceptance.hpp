#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportPlayerAdmission.hpp>

namespace Poseidon
{

template <class UserMapT, class SupportMapT, class CreateAllocator, class SupportFactory>
NetTransportAcceptedPlayerResult AcceptNetTransportCreatePlayer(
    UserMapT& users, SupportMapT& support, AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers, int& botId,
    int player, NetChannel* channel, const struct sockaddr_in& distant, const CreatePlayerPacket& request,
    NetCallBack* processRoutine, SupportFactory&& makeSupport)
{
    NetTransportAcceptedPlayerResult accepted = RegisterNetTransportAcceptedPlayer(
        users, support, createPlayers, botId, player, channel, distant, request, makeSupport);
    channel->setProcessRoutine(processRoutine);
    return accepted;
}

} // namespace Poseidon
