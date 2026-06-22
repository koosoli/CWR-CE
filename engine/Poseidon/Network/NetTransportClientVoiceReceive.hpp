#pragma once

#include <Poseidon/Network/NetTransportClientReceive.hpp>
#include <Poseidon/Network/NetTransportVoiceSpeakerPool.hpp>

#include <Poseidon/Audio/Voice/VonNet.hpp>

#include <cstring>
#include <utility>
using Poseidon::VoNChatChannel;
using Poseidon::VoNDataPacket;
using Poseidon::vonIsDataPacket;
using Poseidon::Foundation::nsNoMoreCallbacks;

namespace Poseidon
{

template <class HandlePacketFn, class EnsureSpeakerFn>
bool TryHandleNetTransportClientVoicePacket(const void* data, int length, HandlePacketFn&& handlePacket,
                                            EnsureSpeakerFn&& ensureSpeaker)
{
    if (!vonIsDataPacket(data, length))
    {
        return false;
    }

    if (length < VoNDataPacket::HEADER_SIZE)
    {
        return true;
    }

    VoNDataPacket header{};
    std::memcpy(&header, data, VoNDataPacket::HEADER_SIZE);
    handlePacket(header, reinterpret_cast<const uint8_t*>(data) + VoNDataPacket::HEADER_SIZE);
    ensureSpeaker(header.channel);
    return true;
}

template <class GetClientFn, class EnsureSpeakerFn>
bool DispatchNetTransportClientVoicePacket(GetClientFn&& getClient, const void* data, int length,
                                           EnsureSpeakerFn&& ensureSpeaker)
{
    auto* client = getClient();
    return TryHandleNetTransportClientVoicePacket(
        data, length,
        [client](const VoNDataPacket& header, const uint8_t* payload)
        {
            if (!client)
            {
                return;
            }

            client->onDataPacket(header, payload);
            LOG_TRACE(Network, "VoN: rx voice pkt ch={} origin={} size={}B", header.channel, header.origin,
                      header.size);
        },
        [client, &ensureSpeaker](uint32_t channel)
        {
            if (!client)
            {
                return;
            }

            ensureSpeaker(channel);
        });
}

template <class UpdateClientFn, class FeedSpeakersFn>
void UpdateNetTransportClientVoicePlayback(bool hasClient, UpdateClientFn&& updateClient, FeedSpeakersFn&& feedSpeakers)
{
    if (!hasClient)
    {
        return;
    }

    updateClient();
    feedSpeakers();
}

template <class CallbackT, class MessageRef, class SpeakerMap, class EnterRcvFn, class LeaveRcvFn, class GetClientFn,
          class CreateSpeakerFn>
void ProcessNetTransportClientUserMessagesWithVoice(CallbackT* callback, void* context, MessageRef& received,
                                                    SpeakerMap& speakers, EnterRcvFn&& enterReceive,
                                                    LeaveRcvFn&& leaveReceive, GetClientFn&& getClient,
                                                    CreateSpeakerFn&& createSpeaker)
{
    auto&& clientAccessor = getClient;
    auto&& speakerFactory = createSpeaker;
    ProcessNetTransportClientUserMessages(
        callback, context, received, std::forward<EnterRcvFn>(enterReceive), std::forward<LeaveRcvFn>(leaveReceive),
        [&speakers, &clientAccessor, &speakerFactory](const NetMessage& currentMessage)
        {
            return DispatchNetTransportClientVoicePacket(
                clientAccessor, currentMessage.getData(), currentMessage.getLength(),
                [&speakers, &speakerFactory](uint32_t channel)
                { EnsureNetTransportVoiceSpeaker(speakers, channel, speakerFactory); });
        },
        [&speakers, &clientAccessor]()
        {
            auto* client = clientAccessor();
            UpdateNetTransportClientVoicePlayback(
                client != nullptr, [client]() { client->update(); },
                [&speakers, client]()
                {
                    FeedNetTransportVoiceSpeakers(speakers, [client](uint32_t channel, auto& speaker)
                                                  { speaker.feed(client, channel); });
                });
        });
}

template <class CreateSpeakerFn, class LogCreateFn>
auto CreateNetTransportClientAutoSpeaker(uint32_t channel, CreateSpeakerFn&& createSpeaker,
                                         LogCreateFn&& logCreate) -> decltype(createSpeaker(channel))
{
    auto speaker = createSpeaker(channel);
    if (speaker)
    {
        std::forward<LogCreateFn>(logCreate)(channel);
    }
    return speaker;
}

template <class CallbackT, class MessageRef, class SpeakerMap, class EnterRcvFn, class LeaveRcvFn, class GetClientFn,
          class CreateSpeakerFn, class LogCreateFn>
void ProcessNetTransportClientUserMessagesWithAutoSpeakers(CallbackT* callback, void* context, MessageRef& received,
                                                           SpeakerMap& speakers, EnterRcvFn&& enterReceive,
                                                           LeaveRcvFn&& leaveReceive, GetClientFn&& getClient,
                                                           CreateSpeakerFn&& createSpeaker, LogCreateFn&& logCreate)
{
    ProcessNetTransportClientUserMessagesWithVoice(
        callback, context, received, speakers, std::forward<EnterRcvFn>(enterReceive),
        std::forward<LeaveRcvFn>(leaveReceive), std::forward<GetClientFn>(getClient),
        [&createSpeaker, &logCreate](uint32_t channel)
        { return CreateNetTransportClientAutoSpeaker(channel, createSpeaker, logCreate); });
}

} // namespace Poseidon
