#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>

#include <Poseidon/Network/NetTransportAddress.hpp>
#include <Poseidon/Network/NetTransportEnumRequest.hpp>
#include <Poseidon/Network/NetTransportEnumResponse.hpp>

namespace Poseidon
{

template <class SetEnumFn>
void InitializeNetTransportSessionEnumerationState(bool& running, int32& magicApp, SetEnumFn&& setEnum)
{
    running = false;
    magicApp = 0;
    std::forward<SetEnumFn>(setEnum)();
}

template <class SetEnumFn>
bool InitializeNetTransportSessionEnumeration(int32& magicApp, int magic, SetEnumFn&& setEnum)
{
    magicApp = magic;
    std::forward<SetEnumFn>(setEnum)();
    return true;
}

template <class UnsetEnumFn, class DoneFn, class CheckPoolFn>
void FinalizeNetTransportSessionEnumeration(UnsetEnumFn&& unsetEnum, DoneFn&& done, CheckPoolFn&& checkPool)
{
    std::forward<UnsetEnumFn>(unsetEnum)();
    std::forward<DoneFn>(done)();
    std::forward<CheckPoolFn>(checkPool)();
}

template <class EnterFn, class LeaveFn, class StopFn, class ClearFn>
void ResetNetTransportSessionEnumerationLocked(EnterFn&& enter, LeaveFn&& leave, StopFn&& stop, ClearFn&& clear)
{
    std::forward<EnterFn>(enter)();
    std::forward<StopFn>(stop)();
    std::forward<ClearFn>(clear)();
    std::forward<LeaveFn>(leave)();
}

template <class SessionsT, class EnterFn, class LeaveFn, class NowFn>
bool ReadNetTransportSessionNeedsRequestLocked(SessionsT& sessions, struct sockaddr_in& addr, unsigned16 port,
                                               EnterFn&& enter, LeaveFn&& leave, NowFn&& getNowSeconds)
{
    std::forward<EnterFn>(enter)();
    const unsigned32 ip = ntohl(addr.sin_addr.s_addr);
    const bool result = NetTransportNeedsEnumRequest(sessions, ip, port, std::forward<NowFn>(getNowSeconds)());
    std::forward<LeaveFn>(leave)();
    return result;
}

template <class SessionsT, class EnterFn, class LeaveFn>
int CountNetTransportSessionsLocked(SessionsT& sessions, EnterFn&& enter, LeaveFn&& leave)
{
    std::forward<EnterFn>(enter)();
    const int count = sessions.Size();
    std::forward<LeaveFn>(leave)();
    return count;
}

template <class SessionsT, class ExportSessionsT, class EnterFn, class LeaveFn, class NowFn, class TickFn>
void ExportNetTransportSessionsLocked(SessionsT& catalog, ExportSessionsT& exported, unsigned32 maxAgeSeconds,
                                      EnterFn&& enter, LeaveFn&& leave, NowFn&& getNowSeconds, TickFn&& getTickCount)
{
    std::forward<EnterFn>(enter)();
    const unsigned32 now = std::forward<NowFn>(getNowSeconds)();
    catalog.RemoveExpired(now, maxAgeSeconds);
    catalog.ExportSessions(exported, now, std::forward<TickFn>(getTickCount)());
    std::forward<LeaveFn>(leave)();
}

template <class MessageFactory>
unsigned DispatchNetTransportEnumRequest(const EnumPacket& request, const struct sockaddr_in& distant,
                                         unsigned16 firstPort, NetChannel* channel, MessageFactory&& newMessage)
{
    unsigned sentCount = 0;
    for (unsigned16 port : BuildNetTransportEnumPortList(firstPort))
    {
        struct sockaddr_in requestDistant = distant;
        requestDistant.sin_port = htons(port);
        auto message = newMessage(sizeof(EnumPacket), channel);
        if (!message)
        {
            continue;
        }

        message->setDistant(requestDistant);
        message->setFlags(MSG_ALL_FLAGS, MSG_TO_BCAST_FLAG | MSG_MAGIC_FLAG);
        message->setData((unsigned8*)&request, sizeof(EnumPacket));
        message->send();
        ++sentCount;
    }

    return sentCount;
}

template <class MessageFactory>
unsigned DispatchNetTransportSessionEnumRequest(int32 magicApplication, const struct sockaddr_in& distant,
                                                unsigned16 firstPort, NetChannel* channel, MessageFactory&& newMessage)
{
    const EnumPacket request = BuildNetTransportEnumRequest(magicApplication);
    return DispatchNetTransportEnumRequest(request, distant, firstPort, channel,
                                           std::forward<MessageFactory>(newMessage));
}

template <class ResolveAddressFn, class NeedsRequestFn, class SendRequestFn>
bool TryDispatchNetTransportEnumAddressIfNeeded(unsigned16 port, struct sockaddr_in& distant,
                                                ResolveAddressFn&& resolveAddress, NeedsRequestFn&& needsRequest,
                                                SendRequestFn&& sendRequest)
{
    if (!resolveAddress(distant, port))
    {
        return false;
    }
    if (!needsRequest(distant, port))
    {
        return false;
    }

    sendRequest(distant, port);
    return true;
}

template <class ResolveLocalFn, class ResolveHostFn, class NeedsRequestFn, class SendRequestFn>
bool DispatchNetTransportSessionEnumAddress(unsigned16 port, bool localAddress, struct sockaddr_in& distant,
                                            ResolveLocalFn&& resolveLocalAddress, ResolveHostFn&& resolveHostAddress,
                                            NeedsRequestFn&& needsRequest, SendRequestFn&& sendRequest)
{
    return TryDispatchNetTransportEnumAddressIfNeeded(
        port, distant,
        [&](struct sockaddr_in& nextDistant, unsigned16 nextPort)
        {
            if (localAddress)
            {
                std::forward<ResolveLocalFn>(resolveLocalAddress)(nextDistant, nextPort);
                return true;
            }
            return std::forward<ResolveHostFn>(resolveHostAddress)(nextDistant, nextPort);
        },
        std::forward<NeedsRequestFn>(needsRequest), std::forward<SendRequestFn>(sendRequest));
}

template <class ResolveHostFn, class NeedsRequestFn, class SendRequestFn>
bool TryDispatchNetTransportEnumUrlAddressIfNeeded(RString address, int defaultPort, struct sockaddr_in& distant,
                                                   ResolveHostFn&& resolveHost, NeedsRequestFn&& needsRequest,
                                                   SendRequestFn&& sendRequest)
{
    RString ip;
    DecodeNetTransportUrlAddress(address, ip, defaultPort);
    return TryDispatchNetTransportEnumAddressIfNeeded(
        static_cast<unsigned16>(defaultPort), distant, [&](struct sockaddr_in& nextDistant, unsigned16 port)
        { return resolveHost(nextDistant, ip, static_cast<int>(port)); }, std::forward<NeedsRequestFn>(needsRequest),
        std::forward<SendRequestFn>(sendRequest));
}

template <class ResolveHostFn, class NeedsRequestFn, class SendRequestFn>
bool DispatchNetTransportSessionEnumUrl(RString address, int defaultPort, struct sockaddr_in& distant,
                                        ResolveHostFn&& resolveHost, NeedsRequestFn&& needsRequest,
                                        SendRequestFn&& sendRequest)
{
    return TryDispatchNetTransportEnumUrlAddressIfNeeded(
        address, defaultPort, distant, std::forward<ResolveHostFn>(resolveHost),
        std::forward<NeedsRequestFn>(needsRequest), std::forward<SendRequestFn>(sendRequest));
}

template <class SetEnumFn, class GetBroadcastChannelFn, class DispatchAddressFn, class DispatchUrlFn>
bool StartNetTransportSessionEnumeration(bool& running, RString ip, int port, AutoArray<RemoteHostAddress>* hosts,
                                         SetEnumFn&& setEnum, GetBroadcastChannelFn&& getBroadcastChannel,
                                         DispatchAddressFn&& dispatchAddress, DispatchUrlFn&& dispatchUrl)
{
    running = true;
    bool anything = false;
    std::forward<SetEnumFn>(setEnum)();

    auto* broadcastChannel = std::forward<GetBroadcastChannelFn>(getBroadcastChannel)();
    if (!broadcastChannel)
    {
        running = false;
        return false;
    }

    if (!ip.GetLength())
    {
        anything |= dispatchAddress(*broadcastChannel, static_cast<unsigned16>(port), false);
        anything |= dispatchAddress(*broadcastChannel, static_cast<unsigned16>(port), true);
    }
    else
    {
        anything |= dispatchUrl(*broadcastChannel, ip, port);
    }

    if (hosts)
    {
        for (int i = 0; running && i < hosts->Size(); ++i)
        {
            const RemoteHostAddress& address = hosts->Get(i);
            anything |= dispatchUrl(*broadcastChannel, address.ip, address.port);
        }
    }

    return anything;
}

template <class SetEnumFn, class GetBroadcastChannelFn, class ResolveLocalFn, class ResolveHostPortFn,
          class ResolveHostUrlFn, class NeedsRequestFn, class SendRequestFn>
bool StartNetTransportSessionEnumerationWithDispatch(bool& running, RString ip, int port,
                                                     AutoArray<RemoteHostAddress>* hosts, SetEnumFn&& setEnum,
                                                     GetBroadcastChannelFn&& getBroadcastChannel,
                                                     ResolveLocalFn&& resolveLocalAddress,
                                                     ResolveHostPortFn&& resolveHostPort,
                                                     ResolveHostUrlFn&& resolveHostUrl, NeedsRequestFn&& needsRequest,
                                                     SendRequestFn&& sendRequest)
{
    struct sockaddr_in addr
    {
    };
    auto&& resolveLocal = resolveLocalAddress;
    auto&& resolvePort = resolveHostPort;
    auto&& resolveUrl = resolveHostUrl;
    auto&& shouldRequest = needsRequest;
    auto&& requestSender = sendRequest;
    return StartNetTransportSessionEnumeration(
        running, ip, port, hosts, std::forward<SetEnumFn>(setEnum),
        std::forward<GetBroadcastChannelFn>(getBroadcastChannel),
        [&](auto& broadcastChannel, unsigned16 nextPort, bool localAddress)
        {
            return DispatchNetTransportSessionEnumAddress(
                nextPort, localAddress, addr, resolveLocal,
                [&](struct sockaddr_in& distantAddress, unsigned16 resolvedPort)
                { return resolvePort(distantAddress, resolvedPort); },
                [&](const struct sockaddr_in& distantAddress, unsigned16 resolvedPort)
                { return shouldRequest(distantAddress, resolvedPort); },
                [&](const struct sockaddr_in& distantAddress, unsigned16 resolvedPort)
                { requestSender(broadcastChannel, distantAddress, resolvedPort); });
        },
        [&](auto& broadcastChannel, RString addressText, int nextPort)
        {
            return DispatchNetTransportSessionEnumUrl(
                addressText, nextPort, addr, [&](struct sockaddr_in& distantAddress, RString host, int resolvedPort)
                { return resolveUrl(distantAddress, host, resolvedPort); },
                [&](const struct sockaddr_in& distantAddress, unsigned16 resolvedPort)
                { return shouldRequest(distantAddress, resolvedPort); },
                [&](const struct sockaddr_in& distantAddress, unsigned16 resolvedPort)
                { requestSender(broadcastChannel, distantAddress, resolvedPort); });
        });
}

template <class SetEnumFn, class GetBroadcastChannelFn, class ResolveLocalFn, class ResolveHostPortFn,
          class ResolveHostUrlFn, class NeedsMutableRequestFn, class SendMutableRequestFn>
bool StartNetTransportSessionEnumerationWithRequests(
    bool& running, RString ip, int port, AutoArray<RemoteHostAddress>* hosts, SetEnumFn&& setEnum,
    GetBroadcastChannelFn&& getBroadcastChannel, ResolveLocalFn&& resolveLocalAddress,
    ResolveHostPortFn&& resolveHostPort, ResolveHostUrlFn&& resolveHostUrl, NeedsMutableRequestFn&& needsMutableRequest,
    SendMutableRequestFn&& sendMutableRequest)
{
    return StartNetTransportSessionEnumerationWithDispatch(
        running, ip, port, hosts, std::forward<SetEnumFn>(setEnum),
        std::forward<GetBroadcastChannelFn>(getBroadcastChannel), std::forward<ResolveLocalFn>(resolveLocalAddress),
        std::forward<ResolveHostPortFn>(resolveHostPort), std::forward<ResolveHostUrlFn>(resolveHostUrl),
        [&needsMutableRequest](const struct sockaddr_in& distantAddress, unsigned16 resolvedPort)
        { return needsMutableRequest(const_cast<struct sockaddr_in&>(distantAddress), resolvedPort); },
        [&sendMutableRequest](auto& broadcastChannel, const struct sockaddr_in& distantAddress, unsigned16 resolvedPort)
        {
            struct sockaddr_in nextDistant = distantAddress;
            sendMutableRequest(broadcastChannel, nextDistant, resolvedPort);
        });
}

template <class GetPeerFn>
auto GetNetTransportEnumBroadcastChannel(GetPeerFn&& getPeer) -> decltype(getPeer()->getBroadcastChannel())
{
    auto* peer = getPeer();
    if (!peer)
    {
        return nullptr;
    }

    auto* broadcastChannel = peer->getBroadcastChannel();
    NET_ERROR(broadcastChannel);
    return broadcastChannel;
}

template <class SessionsT, class EnterFn, class LeaveFn, class GetMessageTimeFn>
bool ProcessNetTransportEnumResponse(SessionsT& sessions, const void* data, size_t length, unsigned32 ip,
                                     unsigned16 port, const char* address, unsigned64 receiveTime,
                                     unsigned32 nowSeconds, EnterFn&& enter, LeaveFn&& leave,
                                     GetMessageTimeFn&& getMessageTime)
{
    NetTransportEnumResponseData response;
    if (!TryParseNetTransportEnumResponse(data, length, response))
    {
        return false;
    }

    enter();
    const unsigned64 requestTime = getMessageTime(response.packet.request);
    const unsigned pingTime = ComputeNetTransportEnumPingMs(receiveTime, requestTime);
    const int found = sessions.UpsertFromResponse(ip, port, address, response.packet, response.mod, response.versionTag,
                                                  response.equalModRequired, nowSeconds, pingTime);
    leave();
    return found >= 0;
}

template <class MessageT, class SessionsT, class EnterFn, class LeaveFn, class NowFn, class AddressToStringFn>
bool ProcessNetTransportEnumResponseMessage(const MessageT& message, SessionsT& sessions, EnterFn&& enter,
                                            LeaveFn&& leave, NowFn&& getNowSeconds, AddressToStringFn&& addressToString)
{
    struct sockaddr_in distant
    {
    };
    message.getDistant(distant);
    const unsigned32 ip = ntohl(distant.sin_addr.s_addr);
    const unsigned16 port = ntohs(distant.sin_port);
    return ProcessNetTransportEnumResponse(
        sessions, message.getData(), message.getLength(), ip, port,
        std::forward<AddressToStringFn>(addressToString)(distant), message.getTime(),
        std::forward<NowFn>(getNowSeconds)(), std::forward<EnterFn>(enter), std::forward<LeaveFn>(leave),
        [channel = message.getChannel()](MsgSerial request) { return channel->getMessageTime(request); });
}

} // namespace Poseidon
