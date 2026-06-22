#pragma once

#include <Poseidon/Network/NetTransportPlayerChannelLookup.hpp>

#include <Poseidon/Network/Legacy/netpch.hpp>
#include <utility>
using Poseidon::Foundation::IteratorState;

namespace Poseidon
{

template <class UserMapT>
int ReadNetTransportServerPlayerByChannel(UserMapT& users, NetChannel* channel)
{
    return FindNetTransportPlayerByChannel(users, channel);
}

template <class UserMapT, class EnterFn, class LeaveFn>
int ReadNetTransportServerPlayerByChannelLocked(UserMapT& users, NetChannel* channel, EnterFn&& enterUser,
                                                LeaveFn&& leaveUser)
{
    enterUser();
    const int player = ReadNetTransportServerPlayerByChannel(users, channel);
    leaveUser();
    return player;
}

template <class UserMapT>
unsigned ReadNetTransportServerChannelId(UserMapT& users, int player)
{
#ifdef NET_LOG
    RefD<NetChannel> channel;
    return users.get(player, channel) && channel ? channel->getChannelId() : 0;
#else
    (void)users;
    (void)player;
    return 0;
#endif
}

} // namespace Poseidon
