#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportPlayerAcceptance.hpp>
#include <Poseidon/Network/NetTransportPlayerAllocation.hpp>
#include <Poseidon/Network/NetTransportPlayerChannelReuse.hpp>
#include <Poseidon/Network/NetTransportPlayerValidation.hpp>

namespace Poseidon
{

struct NetTransportCreatePlayerAttemptResult
{
    ConnectResult result = CROK;
    int player = 0;
    NetChannel* channel = nullptr;
    NetTransportAcceptedPlayerResult accepted{};
    bool duplicate = false;
    bool retryAfterDestroy = false;
    int previousPlayer = -1;
};

template <class UserMapT, class SupportMapT, class CreateAllocator, class FindChannelFn, class CreateChannelFn,
          class SupportFactory>
NetTransportCreatePlayerAttemptResult ProcessNetTransportCreatePlayerAttempt(
    UserMapT& users, SupportMapT& support, AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers, int& botId,
    const SessionPacket& session, const char* serverPassword, int maxPlayers, int uniqueToTry,
    const struct sockaddr_in& distant, const CreatePlayerPacket& request, NetCallBack* processRoutine,
    FindChannelFn&& findChannel, CreateChannelFn&& createChannel, SupportFactory&& makeSupport)
{
    NetTransportCreatePlayerAttemptResult attempt{};
    attempt.result = ValidateNetTransportCreatePlayerRequest(session, serverPassword, request);
    if (attempt.result != CROK)
    {
        return attempt;
    }

    NetChannel* existingChannel = findChannel();
    if (existingChannel)
    {
        const NetTransportCreatePlayerChannelReuseResult reuse =
            AssessNetTransportCreatePlayerChannelReuse(users, existingChannel, request.uniqueID, uniqueToTry);
        attempt.duplicate = reuse.duplicate;
        attempt.retryAfterDestroy = !reuse.duplicate;
        attempt.previousPlayer = reuse.previousPlayer;
        return attempt;
    }

    const NetTransportCreatePlayerAllocationResult allocation =
        FindNetTransportCreatePlayerSlot(users, maxPlayers, request.uniqueID, uniqueToTry);
    attempt.player = allocation.player;
    attempt.result = allocation.result;
    if (attempt.result != CROK)
    {
        return attempt;
    }

    attempt.channel = createChannel();
    if (!attempt.channel)
    {
        attempt.result = CRError;
        return attempt;
    }

    attempt.accepted = AcceptNetTransportCreatePlayer(users, support, createPlayers, botId, attempt.player,
                                                      attempt.channel, distant, request, processRoutine, makeSupport);
    return attempt;
}

} // namespace Poseidon
