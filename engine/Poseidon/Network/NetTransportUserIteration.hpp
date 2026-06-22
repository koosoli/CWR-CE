#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>
#include <Poseidon/Foundation/Containers/Maps.hpp>
#include <utility>
using Poseidon::Foundation::IteratorState;

namespace Poseidon
{

template <class UserMapT, class Callback>
void ForEachNetTransportUserChannel(UserMapT& users, Callback&& callback)
{
    IteratorState it;
    RefD<NetChannel> channel;
    if (users.getFirst(it, channel))
    {
        do
        {
            callback((NetChannel*)channel);
        } while (users.getNext(it, channel));
    }
}

template <class UserMapT, class Callback>
void ForEachNetTransportUserPlayerChannel(UserMapT& users, Callback&& callback)
{
    IteratorState it;
    int player = -1;
    RefD<NetChannel> channel;
    if (users.getFirst(it, channel, &player))
    {
        do
        {
            callback(player, (NetChannel*)channel);
        } while (users.getNext(it, channel, &player));
    }
}

template <class UserMapT, class EnterUsrFn, class LeaveUsrFn, class DisconnectFn, class SleepFn>
void NotifyNetTransportServerDisconnectAll(UserMapT& users, EnterUsrFn&& enterUsers, LeaveUsrFn&& leaveUsers,
                                           DisconnectFn&& disconnectPlayer, unsigned32 waitMs, SleepFn&& sleepMs)
{
    std::forward<EnterUsrFn>(enterUsers)();
    ForEachNetTransportUserChannel(users, std::forward<DisconnectFn>(disconnectPlayer));
    std::forward<LeaveUsrFn>(leaveUsers)();
    std::forward<SleepFn>(sleepMs)(waitMs);
}

} // namespace Poseidon
