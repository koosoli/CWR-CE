#pragma once
#include <utility>

#include <cstdint>

#include <Poseidon/Network/NetTransportVoiceSpeakerPool.hpp>

namespace Poseidon
{

template <class GetClientFn>
bool IsNetTransportClientVoicePlaying(GetClientFn&& getClient, uint32_t playerId)
{
    auto* client = getClient();
    if (!client)
    {
        return false;
    }

    return client->hasChannel(playerId);
}

template <class GetClientFn>
bool IsNetTransportClientVoiceRecording(GetClientFn&& getClient)
{
    auto* client = getClient();
    if (!client)
    {
        return false;
    }

    return client->isTransmitting();
}

template <class GetClientFn, class EnsureSpeakerFn>
auto CreateNetTransportClientVoiceSpeaker(GetClientFn&& getClient, uint32_t playerId,
                                          EnsureSpeakerFn&& ensureSpeaker) -> decltype(ensureSpeaker(playerId))
{
    using SpeakerPtr = decltype(ensureSpeaker(playerId));
    auto* client = getClient();
    if (!client)
    {
        return SpeakerPtr{};
    }

    client->createChannel(playerId, {});
    return ensureSpeaker(playerId);
}

template <class GetClientFn, class EnsureSpeakerFn, class CreateBufferFn>
auto CreateNetTransportClientVoiceBuffer(GetClientFn&& getClient, uint32_t playerId, EnsureSpeakerFn&& ensureSpeaker,
                                         CreateBufferFn&& createBuffer)
    -> decltype(createBuffer(ensureSpeaker(playerId)))
{
    using BufferT = decltype(createBuffer(ensureSpeaker(playerId)));
    auto speaker = CreateNetTransportClientVoiceSpeaker(std::forward<GetClientFn>(getClient), playerId,
                                                        std::forward<EnsureSpeakerFn>(ensureSpeaker));
    if (!speaker)
    {
        return BufferT{};
    }

    return createBuffer(speaker);
}

template <class GetClientFn, class SpeakerMap, class CreateSpeakerFn, class CreateBufferFn, class LogCreateFn>
auto CreateNetTransportClientVoiceBufferWithLog(GetClientFn&& getClient, uint32_t playerId, SpeakerMap& speakers,
                                                CreateSpeakerFn&& createSpeaker, CreateBufferFn&& createBuffer,
                                                LogCreateFn&& logCreate)
    -> decltype(createBuffer(EnsureNetTransportVoiceSpeaker(speakers, playerId, createSpeaker)))
{
    return CreateNetTransportClientVoiceBuffer(
        std::forward<GetClientFn>(getClient), playerId,
        [&speakers, &createSpeaker, &logCreate](uint32_t requestedPlayer)
        {
            const auto existingIt = speakers.find(requestedPlayer);
            auto* existing = existingIt != speakers.end() ? existingIt->second.get() : nullptr;
            auto* speaker = EnsureNetTransportVoiceSpeaker(speakers, requestedPlayer, createSpeaker);
            if (!existing && speaker)
            {
                std::forward<LogCreateFn>(logCreate)(requestedPlayer);
            }
            return speaker;
        },
        std::forward<CreateBufferFn>(createBuffer));
}

template <class GetClientFn, class LogFn>
void SetNetTransportClientVoiceTransmitWithLog(GetClientFn&& getClient, bool on, LogFn&& logSet)
{
    if (SetNetTransportClientVoiceTransmit(std::forward<GetClientFn>(getClient), on))
    {
        std::forward<LogFn>(logSet)(on);
    }
}

} // namespace Poseidon
