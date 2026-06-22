#pragma once

#include <Poseidon/Network/NetTransportPlayerAckResponse.hpp>
#include <Poseidon/Network/NetTransportPlayerCreation.hpp>
#include <Poseidon/Network/NetTransportPlayerReconnect.hpp>
#include <Poseidon/Network/Legacy/netpch.hpp>
using Poseidon::Foundation::nsNoMoreCallbacks;
using Poseidon::Foundation::nsOK;
using Poseidon::Foundation::nsOutputSent;

namespace Poseidon
{

struct NetTransportServerControlAckPlan
{
    NetTransportPlayerAckResponseSetup response{};
    NetChannel* sendChannel = nullptr;
    NetChannel* connectivityChannel = nullptr;
};

inline NetTransportServerControlAckPlan
BuildNetTransportServerControlAckPlan(const NetTransportCreatePlayerAttemptResult& attempt,
                                      const struct sockaddr_in& distant, NetChannel* fallbackChannel)
{
    const bool success = attempt.channel != nullptr;
    return {
        .response = BuildNetTransportPlayerAckResponseSetup(attempt.result, attempt.player, distant, success),
        .sendChannel = success ? attempt.channel : fallbackChannel,
        .connectivityChannel = success ? attempt.channel : nullptr,
    };
}

inline NetTransportServerControlAckPlan
BuildNetTransportServerControlAckPlan(const NetTransportPlayerReconnectResult& reconnect, int playerNo,
                                      const struct sockaddr_in& distant, NetChannel* fallbackChannel)
{
    const bool success = reconnect.status == CROK;
    return {
        .response = BuildNetTransportPlayerAckResponseSetup(reconnect.status, playerNo, distant, success),
        .sendChannel = success ? reconnect.channel : fallbackChannel,
        .connectivityChannel = success ? reconnect.channel : nullptr,
    };
}

template <class MessageFactory>
void DispatchNetTransportServerControlAck(const NetTransportServerControlAckPlan& plan, MessageFactory&& newMessage)
{
    if (plan.sendChannel)
    {
        Ref<NetMessage> out = newMessage(sizeof(plan.response.packet), plan.sendChannel);
        if (out)
        {
            PrepareNetTransportPlayerAckResponse(*out, plan.response);
            out->send(true);
        }
    }

    if (plan.response.checkConnectivity && plan.connectivityChannel)
    {
        plan.connectivityChannel->checkConnectivity(0);
    }
}

} // namespace Poseidon
