#pragma once

#include <Poseidon/Network/NetTransport.hpp>
#include <Poseidon/Network/NetTransportPlayerQueuePolicy.hpp>
#include <Poseidon/Network/NetTransportTermination.hpp>
#include <Poseidon/Network/Legacy/NetApi.hpp>
#include <Poseidon/Network/Legacy/NetMessage.hpp>
using Poseidon::Foundation::nsNoMoreCallbacks;
using Poseidon::Foundation::nsOK;
using Poseidon::Foundation::nsOutputSent;

namespace Poseidon
{

struct NetTransportDestroyedPlayerResult
{
    bool found = false;
    bool cancelledPendingCreate = false;
    DeletePlayerInfo* queuedDeleteInfo = nullptr;
};

enum class NetTransportFinishDestroyStatus
{
    MissingPlayer,
    CancelledPendingCreate,
    QueuedDelete,
};

struct NetTransportFinishDestroyResult
{
    NetTransportFinishDestroyStatus status = NetTransportFinishDestroyStatus::MissingPlayer;
    int player = -1;
};

struct NetTransportTerminatePlayerMessageSetup
{
    TerminateSessionPacket packet{};
    unsigned16 flags = MSG_MAGIC_FLAG;
    NetCallBack* callback = nullptr;
    NetStatus callbackEvent = nsOutputSent;
    void* callbackData = nullptr;
};

inline NetTransportTerminatePlayerMessageSetup
BuildNetTransportTerminatePlayerMessageSetup(NetTerminationReason reason, NetCallBack* callback, void* callbackData)
{
    return {
        .packet = BuildNetTransportTerminateSessionPacket(reason),
        .flags = MSG_MAGIC_FLAG,
        .callback = callback,
        .callbackEvent = nsOutputSent,
        .callbackData = callbackData,
    };
}

inline void PrepareNetTransportTerminatePlayerMessage(NetMessage& message,
                                                      const NetTransportTerminatePlayerMessageSetup& setup)
{
    message.setFlags(MSG_ALL_FLAGS, setup.flags);
    message.setData((unsigned8*)&setup.packet, sizeof(setup.packet));
    message.setCallback(setup.callback, setup.callbackEvent, setup.callbackData);
}

template <class MessageFactory, class SendFn>
bool DispatchNetTransportTerminatePlayerMessage(NetChannel* channel, NetTerminationReason reason, int player,
                                                NetCallBack* callback, MessageFactory&& newMessage,
                                                SendFn&& sendMessage)
{
    auto out = newMessage(sizeof(TerminateSessionPacket), channel);
    if (!out)
    {
        return false;
    }

    const NetTransportTerminatePlayerMessageSetup setup =
        BuildNetTransportTerminatePlayerMessageSetup(reason, callback, (void*)(intptr_t)player);
    out->setFlags(MSG_ALL_FLAGS, setup.flags);
    out->setData((unsigned8*)&setup.packet, sizeof(setup.packet));
    out->setCallback(setup.callback, setup.callbackEvent, setup.callbackData);
    sendMessage(*out);
    return true;
}

template <class ChannelToPlayerFn, class MessageFactory, class SendFn>
void ProcessNetTransportServerDestroyPlayer(NetChannel* channel, NetTerminationReason reason, NetCallBack* callback,
                                            ChannelToPlayerFn&& channelToPlayer, MessageFactory&& newMessage,
                                            SendFn&& sendMessage)
{
    NET_ERROR(channel);
    const int player = std::forward<ChannelToPlayerFn>(channelToPlayer)(channel);
    DispatchNetTransportTerminatePlayerMessage(
        channel, reason, player, callback, std::forward<MessageFactory>(newMessage), std::forward<SendFn>(sendMessage));
}

template <class ChannelToPlayerFn, class MessageFactory>
void ProcessNetTransportServerDestroyPlayerWithGuaranteedSend(NetChannel* channel, NetTerminationReason reason,
                                                              NetCallBack* callback,
                                                              ChannelToPlayerFn&& channelToPlayer,
                                                              MessageFactory&& newMessage)
{
    ProcessNetTransportServerDestroyPlayer(channel, reason, callback, std::forward<ChannelToPlayerFn>(channelToPlayer),
                                           std::forward<MessageFactory>(newMessage),
                                           [](auto& message) { message.send(true); });
}

template <class EnterPeerLockFn, class LeavePeerLockFn, class FinishDestroyFn>
NetStatus ProcessNetTransportDestroyPlayerCallback(void* callbackData, EnterPeerLockFn&& enterPeerLock,
                                                   LeavePeerLockFn&& leavePeerLock,
                                                   FinishDestroyFn&& finishDestroyPlayer)
{
    enterPeerLock();
    finishDestroyPlayer((int)(intptr_t)callbackData);
    leavePeerLock();
    return nsNoMoreCallbacks;
}

template <class OwnerT, class GetOwnerFn, class EnterPeerLockFn, class LeavePeerLockFn, class FinishDestroyFn>
NetStatus ProcessNetTransportDestroyPlayerOwnerCallback(void* callbackData, GetOwnerFn&& getOwner,
                                                        EnterPeerLockFn&& enterPeerLock,
                                                        LeavePeerLockFn&& leavePeerLock,
                                                        FinishDestroyFn&& finishDestroyPlayer)
{
    auto* owner = std::forward<GetOwnerFn>(getOwner)();
    if (!owner)
    {
        return nsNoMoreCallbacks;
    }

    return ProcessNetTransportDestroyPlayerCallback(
        callbackData, std::forward<EnterPeerLockFn>(enterPeerLock), std::forward<LeavePeerLockFn>(leavePeerLock),
        [owner, &finishDestroyPlayer](int player) { finishDestroyPlayer(*owner, player); });
}

template <class UserMapT, class SupportMapT, class CreateAllocator, class DeleteAllocator, class PoolT>
NetTransportDestroyedPlayerResult FinalizeNetTransportDestroyedPlayer(
    UserMapT& users, SupportMapT& support, AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers,
    AutoArray<DeletePlayerInfo, DeleteAllocator>& deletePlayers, PoolT* pool, int player)
{
    NetTransportDestroyedPlayerResult result;
    RefD<NetChannel> channel;
    result.found = users.get(player, channel);
    if (!result.found)
    {
        return result;
    }

    users.remove(player);

    struct sockaddr_in distantAddress
    {
    };
    channel->getDistantAddress(distantAddress);
    support.remove(sockaddrKey(distantAddress));
    pool->deleteChannel(channel);

    result.cancelledPendingCreate = CancelNetTransportPendingCreatePlayer(createPlayers, player);
    if (!result.cancelledPendingCreate)
    {
        result.queuedDeleteInfo = &AppendNetTransportDeletePlayer(deletePlayers, player);
    }

    return result;
}

template <class UserMapT, class SupportMapT, class CreateAllocator, class DeleteAllocator, class PoolT>
NetTransportFinishDestroyResult ProcessNetTransportFinishedDestroyPlayer(
    UserMapT& users, SupportMapT& support, AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers,
    AutoArray<DeletePlayerInfo, DeleteAllocator>& deletePlayers, PoolT* pool, int player)
{
    const NetTransportDestroyedPlayerResult result =
        FinalizeNetTransportDestroyedPlayer(users, support, createPlayers, deletePlayers, pool, player);
    if (!result.found)
    {
        return {};
    }
    if (result.cancelledPendingCreate)
    {
        return {
            .status = NetTransportFinishDestroyStatus::CancelledPendingCreate,
            .player = player,
        };
    }

    NET_ERROR(result.queuedDeleteInfo);
    return {
        .status = NetTransportFinishDestroyStatus::QueuedDelete,
        .player = result.queuedDeleteInfo->player,
    };
}

template <class UserMapT, class SupportMapT, class CreateAllocator, class DeleteAllocator, class PoolT,
          class EnterUsrFn, class LeaveUsrFn, class CheckIntegrityFn, class OnMissingFn, class OnCancelledFn,
          class OnQueuedDeleteFn>
void FinishNetTransportDestroyedPlayer(UserMapT& users, SupportMapT& support,
                                       AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers,
                                       AutoArray<DeletePlayerInfo, DeleteAllocator>& deletePlayers, PoolT* pool,
                                       int player, EnterUsrFn&& enterUsers, LeaveUsrFn&& leaveUsers,
                                       CheckIntegrityFn&& checkIntegrity, OnMissingFn&& onMissing,
                                       OnCancelledFn&& onCancelled, OnQueuedDeleteFn&& onQueuedDelete)
{
    std::forward<EnterUsrFn>(enterUsers)();
    const NetTransportFinishDestroyResult result =
        ProcessNetTransportFinishedDestroyPlayer(users, support, createPlayers, deletePlayers, pool, player);
    if (result.status == NetTransportFinishDestroyStatus::MissingPlayer)
    {
        std::forward<OnMissingFn>(onMissing)(player);
        std::forward<CheckIntegrityFn>(checkIntegrity)();
        std::forward<LeaveUsrFn>(leaveUsers)();
        return;
    }

    std::forward<CheckIntegrityFn>(checkIntegrity)();
    if (result.status == NetTransportFinishDestroyStatus::CancelledPendingCreate)
    {
        std::forward<OnCancelledFn>(onCancelled)(player);
        std::forward<LeaveUsrFn>(leaveUsers)();
        return;
    }

    std::forward<OnQueuedDeleteFn>(onQueuedDelete)(result.player);
    std::forward<LeaveUsrFn>(leaveUsers)();
}

template <class UserMapT, class SupportMapT, class CreateAllocator, class DeleteAllocator, class PoolT,
          class EnterUsrFn, class LeaveUsrFn, class CheckIntegrityFn, class GetSessionPlayersFn, class GetUserCountFn>
void ProcessNetTransportServerFinishedDestroyPlayer(UserMapT& users, SupportMapT& support,
                                                    AutoArray<CreatePlayerInfo, CreateAllocator>& createPlayers,
                                                    AutoArray<DeletePlayerInfo, DeleteAllocator>& deletePlayers,
                                                    PoolT* pool, int player, EnterUsrFn&& enterUsers,
                                                    LeaveUsrFn&& leaveUsers, CheckIntegrityFn&& checkIntegrity,
                                                    GetSessionPlayersFn&& getSessionPlayers,
                                                    GetUserCountFn&& getUserCount)
{
    auto&& sessionPlayers = getSessionPlayers;
    auto&& userCount = getUserCount;
    FinishNetTransportDestroyedPlayer(
        users, support, createPlayers, deletePlayers, pool, player, std::forward<EnterUsrFn>(enterUsers),
        std::forward<LeaveUsrFn>(leaveUsers), std::forward<CheckIntegrityFn>(checkIntegrity),
        [](int missingPlayer) { RptF("NetServer::finishDestroyPlayer(%d): users.get failed", missingPlayer); },
        [](int cancelledPlayer) {
            RptF("NetServer::finishDestroyPlayer(%d): DESTROY immediately after CREATE, both cancelled",
                 cancelledPlayer);
        },
        [&](int queuedPlayer)
        {
            RptF("NetServer:finishDestroyPlayer (waiting for ProcessPlayers) - session.numPlayers=%d, playerId=%d, "
                 "|users|=%u",
                 sessionPlayers(), queuedPlayer, userCount());
        });
}

} // namespace Poseidon
