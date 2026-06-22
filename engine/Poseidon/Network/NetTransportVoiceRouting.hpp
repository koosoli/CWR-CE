#pragma once

#include <Poseidon/Network/NetTransport.hpp>

#include <unordered_map>
#include <vector>

namespace Poseidon
{

template <class Allocator>
void ReadNetTransportVoiceTargets(const std::unordered_map<int, std::vector<int>>& vonTargets, int from,
                                  AutoArray<int, Allocator>& to)
{
    const auto it = vonTargets.find(from);
    if (it == vonTargets.end())
    {
        return;
    }

    for (const int target : it->second)
    {
        to.Add(target);
    }
}

template <class Allocator>
std::vector<uint32_t> WriteNetTransportVoiceTargets(std::unordered_map<int, std::vector<int>>& vonTargets, int from,
                                                    const AutoArray<int, Allocator>& to)
{
    std::vector<uint32_t> routedTargets;
    routedTargets.reserve(to.Size());

    std::vector<int>& storedTargets = vonTargets[from];
    storedTargets.clear();
    storedTargets.reserve(to.Size());

    for (int index = 0; index < to.Size(); ++index)
    {
        storedTargets.push_back(to[index]);
        routedTargets.push_back(static_cast<uint32_t>(to[index]));
    }

    return routedTargets;
}

} // namespace Poseidon
