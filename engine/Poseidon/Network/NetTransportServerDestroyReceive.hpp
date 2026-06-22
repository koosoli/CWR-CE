#pragma once

#include <Poseidon/Network/NetTransportClientSession.hpp>
#include <Poseidon/Network/NetTransportPlayerChannelLookup.hpp>
#include <utility>
using Poseidon::Foundation::IteratorState;

namespace Poseidon
{

struct NetTransportDestroyPlayerReceiveResult
{
    bool handled = false;
    int player = -1;
};

template <class UserMapT, class FinishDestroyFn>
NetTransportDestroyPlayerReceiveResult
TryHandleNetTransportDestroyPlayerControlMessage(unsigned32 magic, unsigned length, UserMapT& users,
                                                 NetChannel* channel, FinishDestroyFn&& finishDestroy)
{
    if (magic != MAGIC_DESTROY_PLAYER || length < sizeof(MagicPacket))
    {
        return {};
    }

    const int player = FindNetTransportPlayerByChannel(users, channel);
    if (player >= 0)
    {
        finishDestroy(player);
    }

    return {
        .handled = true,
        .player = player,
    };
}

template <class UserMapT, class EnterUsrFn, class LeaveUsrFn, class FinishDestroyFn>
NetTransportDestroyPlayerReceiveResult
ProcessNetTransportDestroyPlayerControlMessage(unsigned32 magic, unsigned length, UserMapT& users, NetChannel* channel,
                                               EnterUsrFn&& enterUsr, LeaveUsrFn&& leaveUsr,
                                               FinishDestroyFn&& finishDestroy)
{
    enterUsr();
    NetTransportDestroyPlayerReceiveResult result = TryHandleNetTransportDestroyPlayerControlMessage(
        magic, length, users, channel, std::forward<FinishDestroyFn>(finishDestroy));
    leaveUsr();
    return result;
}

template <class UserMapT, class EnterUsrFn, class LeaveUsrFn, class FinishDestroyFn, class LogMissingFn>
NetTransportDestroyPlayerReceiveResult
ProcessNetTransportDestroyPlayerControlMessageWithLog(unsigned32 magic, unsigned length, UserMapT& users,
                                                      NetChannel* channel, EnterUsrFn&& enterUsr, LeaveUsrFn&& leaveUsr,
                                                      FinishDestroyFn&& finishDestroy, LogMissingFn&& logMissing)
{
    NetTransportDestroyPlayerReceiveResult result = ProcessNetTransportDestroyPlayerControlMessage(
        magic, length, users, channel, std::forward<EnterUsrFn>(enterUsr), std::forward<LeaveUsrFn>(leaveUsr),
        std::forward<FinishDestroyFn>(finishDestroy));
    if (result.handled && result.player < 0)
    {
        std::forward<LogMissingFn>(logMissing)(channel);
    }
    return result;
}

} // namespace Poseidon
