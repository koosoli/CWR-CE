#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>
#include <Poseidon/Foundation/Containers/Maps.hpp>
#include <utility>
using Poseidon::Foundation::IteratorState;

namespace Poseidon
{

template <class UserMapT, class ChannelT>
int FindNetTransportPlayerByChannel(UserMapT& users, ChannelT* channel)
{
    if (channel == nullptr)
    {
        return -1;
    }

    IteratorState it;
    int player = -1;
    RefD<NetChannel> current;
    if (users.getFirst(it, current, &player))
    {
        do
        {
            if ((NetChannel*)current == channel)
            {
                return player;
            }
        } while (users.getNext(it, current, &player));
    }

    return -1;
}

} // namespace Poseidon
