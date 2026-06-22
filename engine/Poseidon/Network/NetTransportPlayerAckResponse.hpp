#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportPlayerHandshake.hpp>
#include <Poseidon/Network/Legacy/NetMessage.hpp>

namespace Poseidon
{

struct NetTransportPlayerAckResponseSetup
{
    AckPlayerPacket packet{};
    unsigned16 flags = MSG_MAGIC_FLAG;
    bool setDistant = false;
    struct sockaddr_in distant
    {
    };
    bool checkConnectivity = false;
};

inline NetTransportPlayerAckResponseSetup BuildNetTransportPlayerAckResponseSetup(ConnectResult result, int32 playerNo,
                                                                                  const struct sockaddr_in& distant,
                                                                                  bool success)
{
    return {
        .packet = BuildAckPlayerPacket(result, playerNo),
        .flags = (unsigned16)(MSG_MAGIC_FLAG | (success ? MSG_VIM_FLAG : MSG_FROM_BCAST_FLAG)),
        .setDistant = !success,
        .distant = distant,
        .checkConnectivity = success,
    };
}

inline void PrepareNetTransportPlayerAckResponse(NetMessage& message, const NetTransportPlayerAckResponseSetup& setup)
{
    if (setup.setDistant)
    {
        struct sockaddr_in distant = setup.distant;
        message.setDistant(distant);
    }

    message.setFlags(MSG_ALL_FLAGS, setup.flags);
    message.setData((unsigned8*)&setup.packet, sizeof(setup.packet));
}

} // namespace Poseidon
