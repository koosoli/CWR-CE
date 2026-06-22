#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>

#include <Poseidon/Network/NetTransportProtocol.hpp>

namespace Poseidon
{

struct NetTransportEnumResponseData
{
    SessionPacket packet{};
    char mod[MOD_LENGTH]{};
    char versionTag[VERSION_TAG_LENGTH]{};
    bool equalModRequired = false;
};

SessionPacket BuildNetTransportEnumResponse(const SessionPacket& session, unsigned32 magic, MsgSerial request,
                                            int32 numPlayers);

template <class MessageFactory>
void DispatchNetTransportEnumResponse(const SessionPacket& response, const struct sockaddr_in& distant,
                                      unsigned responseLength, NetChannel* channel, MessageFactory&& newMessage)
{
    auto out = newMessage(responseLength, channel);
    if (!out)
    {
        return;
    }

    struct sockaddr_in mutableDistant = distant;
    out->setDistant(mutableDistant);
    out->setFlags(MSG_ALL_FLAGS, MSG_TO_BCAST_FLAG | MSG_MAGIC_FLAG);
    out->setData((unsigned8*)&response, responseLength);
    out->send(true);
}

template <class HasReceiverFn, class HandleResponseFn>
NetStatus ProcessNetTransportEnumReceiveCallback(NetMessage* msg, HasReceiverFn&& hasReceiver,
                                                 HandleResponseFn&& handleResponse)
{
    if (!std::forward<HasReceiverFn>(hasReceiver)() || !msg || msg->getLength() < sizeof(unsigned32) ||
        !(msg->getFlags() & MSG_MAGIC_FLAG))
    {
        return Poseidon::Foundation::nsNoMoreCallbacks;
    }

    const unsigned32 magic = *static_cast<unsigned32*>(msg->getData());
    if (magic == MAGIC_ENUM_RESPONSE)
    {
        std::forward<HandleResponseFn>(handleResponse)(*msg);
    }

    return Poseidon::Foundation::nsNoMoreCallbacks;
}

bool IsNetTransportEnumResponseSize(size_t length);
bool TryParseNetTransportEnumResponse(const void* data, size_t length, NetTransportEnumResponseData& response);
unsigned ComputeNetTransportEnumPingMs(unsigned64 receiveTime, unsigned64 requestTime);

} // namespace Poseidon
