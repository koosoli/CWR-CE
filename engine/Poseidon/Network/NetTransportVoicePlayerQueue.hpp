#pragma once

#include <Poseidon/Network/NetTransport.hpp>

#include <unordered_map>
#include <vector>

namespace Poseidon
{

inline void NotifyNetTransportVoicePlayers(const std::unordered_map<int, std::vector<int>>& vonTargets,
                                           CreateVoicePlayerCallback* callback, void* context)
{
    if (!callback)
    {
        return;
    }

    for (const auto& [sender, targets] : vonTargets)
    {
        callback(sender, context);
    }
}

template <class RemovePlayerFn>
void ClearNetTransportVoicePlayers(std::unordered_map<int, std::vector<int>>& vonTargets, RemovePlayerFn&& removePlayer)
{
    for (const auto& [sender, targets] : vonTargets)
    {
        removePlayer(sender);
    }
    vonTargets.clear();
}

} // namespace Poseidon
