#pragma once

#include <cstdint>

namespace Poseidon
{

template <class SpeakerMap, class CreateSpeakerFn>
auto* EnsureNetTransportVoiceSpeaker(SpeakerMap& speakers, uint32_t channel, CreateSpeakerFn&& createSpeaker)
{
    auto& speaker = speakers[channel];
    if (!speaker)
    {
        speaker = createSpeaker(channel);
    }
    return speaker.get();
}

template <class SpeakerMap, class FeedSpeakerFn>
void FeedNetTransportVoiceSpeakers(SpeakerMap& speakers, FeedSpeakerFn&& feedSpeaker)
{
    for (auto& [channel, speaker] : speakers)
    {
        if (speaker)
        {
            feedSpeaker(channel, *speaker);
        }
    }
}

template <class SpeakerMap, class DestroySpeakerFn>
void ClearNetTransportVoiceSpeakers(SpeakerMap& speakers, DestroySpeakerFn&& destroySpeaker)
{
    for (auto& [channel, speaker] : speakers)
    {
        if (speaker)
        {
            destroySpeaker(*speaker);
        }
    }
    speakers.clear();
}

} // namespace Poseidon
