#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>

#include <Poseidon/Network/NetTransportMessageAge.hpp>
#include <Poseidon/Foundation/Containers/Maps.hpp>
#include <Poseidon/Network/NetTransport.hpp>
#include <utility>
using Poseidon::Foundation::IteratorState;

namespace Poseidon
{

template <class UserMapT>
float ReadNetTransportServerLastMsgAgeReliable(UserMapT& users, int player, unsigned64 now)
{
    RefD<NetChannel> channel;
    if (!users.get(player, channel))
    {
        return 1.0f;
    }

    return ComputeNetTransportMessageAge(now, channel->getLastMessageArrival(), "NetServer::GetLastMsgAgeReliable",
                                         "NetServer::GetLastMsgAgeReliable");
}

template <class UserMapT, class EnterFn, class LeaveFn, class NowFn>
float ReadNetTransportServerLastMsgAgeReliableLocked(UserMapT& users, int player, EnterFn&& enterLock,
                                                     LeaveFn&& leaveLock, NowFn&& getNow)
{
    std::forward<EnterFn>(enterLock)();
    const float result = ReadNetTransportServerLastMsgAgeReliable(users, player, std::forward<NowFn>(getNow)());
    std::forward<LeaveFn>(leaveLock)();
    return result;
}

template <class UserMapT>
void ApplyNetTransportServerNetworkParams(UserMapT& users, const NetworkParams& params)
{
    IteratorState it;
    RefD<NetChannel> channel;
    if (!users.getFirst(it, channel))
    {
        return;
    }

    channel->setNetworkParams(params);
}

template <class UserMapT, class EnterFn, class LeaveFn>
void ApplyNetTransportServerNetworkParamsLocked(UserMapT& users, const NetworkParams& params, EnterFn&& enterLock,
                                                LeaveFn&& leaveLock)
{
    std::forward<EnterFn>(enterLock)();
    ApplyNetTransportServerNetworkParams(users, params);
    std::forward<LeaveFn>(leaveLock)();
}

template <class UserMapT, class DestroyPlayerFn>
bool TryKickOffNetTransportServerPlayer(UserMapT& users, int player, DestroyPlayerFn&& destroyPlayer)
{
    RefD<NetChannel> channel;
    if (!users.get(player, channel))
    {
        return false;
    }

    destroyPlayer(channel);
    return true;
}

template <class UserMapT, class DestroyPlayerFn, class LogMissingFn>
bool ProcessNetTransportServerKickOff(UserMapT& users, int player, DestroyPlayerFn&& destroyPlayer,
                                      LogMissingFn&& logMissingPlayer)
{
    if (TryKickOffNetTransportServerPlayer(users, player, std::forward<DestroyPlayerFn>(destroyPlayer)))
    {
        return true;
    }

    logMissingPlayer(player);
    return false;
}

template <class UserMapT, class EnterUsrFn, class LeaveUsrFn, class DestroyPlayerFn, class LogMissingFn>
void ProcessNetTransportServerKickOffLocked(UserMapT& users, int player, EnterUsrFn&& enterUsers,
                                            LeaveUsrFn&& leaveUsers, DestroyPlayerFn&& destroyPlayer,
                                            LogMissingFn&& logMissingPlayer)
{
    std::forward<EnterUsrFn>(enterUsers)();
    ProcessNetTransportServerKickOff(users, player, std::forward<DestroyPlayerFn>(destroyPlayer),
                                     std::forward<LogMissingFn>(logMissingPlayer));
    std::forward<LeaveUsrFn>(leaveUsers)();
}

template <class UserMapT, class EnterUsrFn, class LeaveUsrFn, class DestroyPlayerFn>
void ProcessNetTransportServerKickOffReasonLocked(UserMapT& users, int player, NetTerminationReason reason,
                                                  EnterUsrFn&& enterUsers, LeaveUsrFn&& leaveUsers,
                                                  DestroyPlayerFn&& destroyPlayer)
{
    ProcessNetTransportServerKickOffLocked(
        users, player, std::forward<EnterUsrFn>(enterUsers), std::forward<LeaveUsrFn>(leaveUsers),
        [reason, &destroyPlayer](NetChannel* channel) { destroyPlayer(channel, reason); },
        [](int missingPlayer) { RptF("NetServer::KickOff: player=%d - !users.get", missingPlayer); });
}

} // namespace Poseidon
